#include "mb85as12mt.h"
#include <iostream>
#include <string>
#include <cstring>  // memset
#include <unistd.h> // close
#include <fstream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

MB85AS12MT::MB85AS12MT(const int bus, const int cs, const int speed) {

   std::string filename = "/dev/spidev" + std::to_string(bus) + "." + std::to_string(cs);

   fd = open(filename.c_str(), O_RDWR);
   if (fd < 0)
      throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error opening " + filename);

   // fixme
   int mode = SPI_MODE_3 | SPI_3WIRE;
   if(ioctl(fd, SPI_IOC_WR_MODE32, &mode) == -1)
      throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error setting SPI mode");

   if(ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1)	
      throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error setting SPI speed");

   int bits = 8;
   if(ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1)
      throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error setting SPI bits per word");

   std::ifstream f;
   std::string line;
   f.open("/sys/module/spidev/parameters/bufsiz");
   getline(f, line, '\n');
   if (line.empty())
      throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error reading spidev buffer size");
   spidev_bufsize = std::stoi(line);
   f.close();
}

MB85AS12MT::~MB85AS12MT(void) {
   if(fd)
      close(fd);
}

MB85AS12MT::DeviceId MB85AS12MT::getDeviceId(void) {

   uint8_t tx_buffer = 0x9F;
   struct spi_ioc_transfer tr[2];
   uint8_t buf[4];
   MB85AS12MT::DeviceId id;

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   tr[0].cs_change = 0;

   tr[1].tx_buf = (unsigned long)NULL;
   tr[1].rx_buf = (unsigned long)buf;
   tr[1].len = 4;
   tr[1].delay_usecs = 0;

   if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) == -1)
      throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error reading id");

   id.manufacturer_id = buf[0];
   id.continuation_code = buf[1];
   id.product_id = (buf[2] << 8) + buf[3];

   return id;
}

MB85AS12MT::UID MB85AS12MT::getUID(void) {

   uint8_t tx_buffer = 0x83;
   struct spi_ioc_transfer tr[2];
   uint8_t buf[12];
   MB85AS12MT::UID uid;

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   tr[0].cs_change = 0;

   tr[1].tx_buf = (unsigned long)NULL;
   tr[1].rx_buf = (unsigned long)buf;
   tr[1].len = 12;
   tr[1].delay_usecs = 0;

   if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) == -1)
      throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error reading uid");

   uid.manufacturer_id = buf[0];
   uid.continuation_code = buf[1];
   uid.product_id = (buf[2] << 8) + buf[3];
   for(int i=0;i<5;i++)
      uid.lot_id[i] = buf[4+i];
   uid.lot_id[5] = '\0';
   uid.wafer_id = buf[9];
   uid.chip_id = (buf[10] << 8) + buf[11];

   return uid;	
}

uint64_t MB85AS12MT::getUniqueId(void) {

   MB85AS12MT::UID uid = getUID();
   uint64_t unique_id = 
      ((uint64_t)uid.lot_id[0] << 56) +
      ((uint64_t)uid.lot_id[1] << 48) +
      ((uint64_t)uid.lot_id[2] << 40) +
      ((uint64_t)uid.lot_id[3] << 32) +
      ((uint64_t)uid.lot_id[4] << 24) +
      (uid.wafer_id << 16) +
      (uid.chip_id);

   return unique_id; 
}

void MB85AS12MT::printInfo(void) {

   MB85AS12MT::UID uid = getUID();
   printf("ReRAM MB85AS12MT\n");
   printf("Device ID info:\n");
   printf("   manufacturer id: 0x%02X\n", uid.manufacturer_id);
   printf("   continuation code: 0x%02X\n", uid.continuation_code);
   printf("   product id: 0x%04X\n", uid.product_id);

   printf("Unique ID info:\n");	
   printf("   lot id: %s\n", uid.lot_id);
   printf("   wafer id: 0x%02X\n", uid.wafer_id);
   printf("   chip id: 0x%04X\n", uid.chip_id);
   printf("   number: 0x%llX\n", getUniqueId());
}

void MB85AS12MT::writeEnable(void) {

   uint8_t tx_buffer = 0x06;
   struct spi_ioc_transfer tr[1];

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;

   if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == -1)
      throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error");
}

void MB85AS12MT::writeDisable(void) {

   uint8_t tx_buffer = 0x04;
   struct spi_ioc_transfer tr[1];

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;

   if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < -1)
      throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error");
}

uint8_t MB85AS12MT::readStatusRegister(void) {

   uint8_t tx_buffer = 0x05;
   struct spi_ioc_transfer tr[2];
   uint8_t sr;

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   tr[0].cs_change = 0;

   tr[1].tx_buf = (unsigned long)NULL;
   tr[1].rx_buf = (unsigned long)&sr;
   tr[1].len = 1;
   tr[1].delay_usecs = 0;

   if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) == -1)
      throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error");

   return sr;	
}

void MB85AS12MT::writeStatusRegister(const uint8_t value) {

   uint8_t tx_buffer[2] = {0x01, value};
   struct spi_ioc_transfer tr[1];

   writeEnable();

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;

   if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == -1)
      throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error");

   while(writeInProgress())
      ;
}

bool MB85AS12MT::writeInProgress(void) {

   return(readStatusRegister() & 0x01);
}

uint8_t MB85AS12MT::read(const uint32_t address) {

   uint8_t tx_buffer[4] = {0x03,
      (uint8_t)((address & 0x00FF0000) >> 16),
      (uint8_t)((address & 0x0000FF00) >> 8),
      (uint8_t)((address & 0x000000FF))
   };
   struct spi_ioc_transfer tr[2];
   uint8_t value;

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   tr[0].cs_change = 0;

   tr[1].tx_buf = (unsigned long)NULL;
   tr[1].rx_buf = (unsigned long)&value;
   tr[1].len = 1;
   tr[1].delay_usecs = 0;

   if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) == -1)
      throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error");

   return value;
}

void MB85AS12MT::write(const uint32_t address, const uint8_t value) {

   uint8_t tx_buffer[5] = {0x02,
      (uint8_t)((address & 0x00FF0000) >> 16),
      (uint8_t)((address & 0x0000FF00) >> 8),
      (uint8_t)((address & 0x000000FF)),
      value
   };
   struct spi_ioc_transfer tr[1];

   writeEnable();

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   tr[0].cs_change = 0;

   if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == -1)
      throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error");

   while(writeInProgress())
      ;

   writeDisable();
}

void MB85AS12MT::readBuffer(uint32_t offset, uint8_t *buf, uint32_t bufsize) {

   struct spi_ioc_transfer tr[2];
   uint8_t tx_buffer[4];

   int nbuf, remainder;
   uint32_t m_address = offset;     // ReRAM address
   uint32_t b_address = 0;          // buffer address

   if (bufsize < spidev_bufsize) {
      nbuf = 1;
      remainder = 0;
   } else {
      nbuf = (bufsize / spidev_bufsize);
      remainder = bufsize % spidev_bufsize;
   }

   //printf("bufsize: %d, nbuf: %d, remainder: %d\n", bufsize, nbuf, remainder);

   for(int i=0; i<nbuf; i++) {
      //printf("m_address: %d, b_address: %d\n", m_address, b_address);

      memset(tx_buffer, 0, sizeof(tx_buffer));
      tx_buffer[0] = 0x03;
      tx_buffer[1] = (m_address & 0x00FF0000) >> 16;
      tx_buffer[2] = (m_address & 0x0000FF00) >> 8,
      tx_buffer[3] = (m_address & 0x000000FF);

      memset(&tr, 0, sizeof(tr));
      tr[0].tx_buf = (unsigned long)&tx_buffer;
      tr[0].rx_buf = (unsigned long)NULL;
      tr[0].len = sizeof(tx_buffer);
      tr[0].delay_usecs = 0;
      tr[0].cs_change = 0;

      tr[1].tx_buf = (unsigned long)NULL;
      tr[1].rx_buf = (unsigned long)buf+b_address;
      if (bufsize > spidev_bufsize)
         tr[1].len = spidev_bufsize;
      else
         tr[1].len = bufsize;
      tr[1].delay_usecs = 0;

      if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) == -1)
         throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error");

      m_address += spidev_bufsize;
      b_address += spidev_bufsize;
   }

   if (remainder > 0) {
      //printf("remainder: %d\n", remainder);
      memset(tx_buffer, 0, sizeof(tx_buffer));
      tx_buffer[0] = 0x03;
      tx_buffer[1] = (m_address & 0x00FF0000) >> 16;
      tx_buffer[2] = (m_address & 0x0000FF00) >> 8,
      tx_buffer[3] = (m_address & 0x000000FF);

      memset(&tr, 0, sizeof(tr));
      tr[0].tx_buf = (unsigned long)&tx_buffer;
      tr[0].rx_buf = (unsigned long)NULL;
      tr[0].len = sizeof(tx_buffer);
      tr[0].delay_usecs = 0;
      tr[0].cs_change = 0;

      tr[1].tx_buf = (unsigned long)NULL;
      tr[1].rx_buf = (unsigned long)buf+b_address;
      tr[1].len = remainder;
      tr[1].delay_usecs = 0;

      if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) < -1)
         throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error");
   }
}

void MB85AS12MT::writeBuffer(uint32_t offset, uint8_t *buf, uint32_t bufsize) {

   struct spi_ioc_transfer tr[2];
   uint8_t tx_buffer[5];

   int nbuf, remainder;
   uint32_t m_address = offset;     // ReRAM address
   uint32_t b_address = 0;          // buffer address

   if (bufsize < 256) {
      nbuf = 1;
      remainder = 0;
   } else {
      nbuf = (bufsize / 256);
      remainder = bufsize % 256;
   }

   //printf("bufsize: %d, nbuf: %d, remainder: %d\n", bufsize, nbuf, remainder);

   for(int i=0; i<nbuf; i++) {
      //printf("m_address: %d, b_address: %d\n", m_address, b_address);

      memset(tx_buffer, 0, sizeof(tx_buffer));
      tx_buffer[0] = 0x02;
      tx_buffer[1] = (m_address & 0x00FF0000) >> 16;
      tx_buffer[2] = (m_address & 0x0000FF00) >> 8,
      tx_buffer[3] = (m_address & 0x000000FF);
      tx_buffer[4] = buf[b_address];

      memset(&tr, 0, sizeof(tr));
      tr[0].tx_buf = (unsigned long)&tx_buffer;
      tr[0].rx_buf = (unsigned long)NULL;
      tr[0].len = sizeof(tx_buffer);
      tr[0].delay_usecs = 0;
      tr[0].cs_change = 0;

      tr[1].tx_buf = (unsigned long)buf+b_address+1;
      tr[1].rx_buf = (unsigned long)NULL;
      if (bufsize > 256)
         tr[1].len = 255;
      else
         tr[1].len = bufsize-1;
      tr[1].delay_usecs = 0;

      writeEnable();

      if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) == -1)
         throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error");

      while(writeInProgress())
         sleep(0.1);

      m_address += 256;
      b_address += 256;
   }

   if (remainder > 0) {
      //printf("remainder: %d\n", remainder);

      memset(tx_buffer, 0, sizeof(tx_buffer));
      tx_buffer[0] = 0x02;
      tx_buffer[1] = (m_address & 0x00FF0000) >> 16;
      tx_buffer[2] = (m_address & 0x0000FF00) >> 8,
      tx_buffer[3] = (m_address & 0x000000FF);
      tx_buffer[4] = buf[b_address];

      memset(&tr, 0, sizeof(tr));
      tr[0].tx_buf = (unsigned long)&tx_buffer;
      tr[0].rx_buf = (unsigned long)NULL;
      tr[0].len = sizeof(tx_buffer);
      tr[0].delay_usecs = 0;
      tr[0].cs_change = 0;

      tr[1].tx_buf = (unsigned long)buf+b_address+1;
      tr[1].rx_buf = (unsigned long)NULL;
      tr[1].len = remainder-1;
      tr[1].delay_usecs = 0;

      writeEnable();

      if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) == -1)
         throw std::runtime_error("E: " + std::string(__PRETTY_FUNCTION__) + ": error");

      while(writeInProgress())
         ;
   }

   writeDisable();	
}

std::vector<uint8_t> MB85AS12MT::readBuffer(uint32_t offset, uint32_t nbytes) {

   std::vector<uint8_t> buf(nbytes);

   readBuffer(offset, buf.data(), buf.size());

   return buf;
}

void MB85AS12MT::writeBuffer(uint32_t offset, std::vector<uint8_t> buffer) {

   writeBuffer(offset, buffer.data(), buffer.size());
}

void MB85AS12MT::dump(uint32_t start, uint32_t end) {

   std::vector<uint8_t> data = readBuffer(start, end-start+1);

   for(unsigned int i=0; i<data.size(); i++) {
      if(i%16 == 0) {
         printf("%06X-%06X: ", 
            i+start, (i+15)<data.size()-1?i+start+15:data.size()-1);
         printf("%02X ", data[i]);
      } else {
         printf("%02X ", data[i]);
         if((i+1) % 16 == 0 && i+1 < data.size())
            printf("\n");
         else if((i+1) % 8 == 0 && i+1 < data.size())
            printf(". ");
      }
   }
   printf("\n");
}
