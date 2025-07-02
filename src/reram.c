#include <string.h>  // memset
#include "reram.h"

bool rr_rdid(int fd, uint8_t *buf, int *bufsize) {
   uint8_t tx_buffer = 0x9F;
   struct spi_ioc_transfer tr[2];

   *bufsize = 4;

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

   if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) < -1)
      return false;

   return true;
}

bool rr_rduid(int fd, uint8_t *buf, int *bufsize) {
   uint8_t tx_buffer = 0x83;
   struct spi_ioc_transfer tr[2];

   *bufsize = 12;

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

   if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) < -1)
      return false;

   return true;
}

bool rr_wren(int fd) {
   uint8_t tx_buffer = 0x06;
   struct spi_ioc_transfer tr[1];
   uint8_t reg;

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   
   if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < -1)
      return false;

   return true;
}

bool rr_wrdi(int fd) {
   uint8_t tx_buffer = 0x04;
   struct spi_ioc_transfer tr[1];
   uint8_t reg;

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   
   if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < -1)
      return false;

   return true;
}

bool rr_rdsr(int fd, uint8_t *sr) {
   uint8_t tx_buffer = 0x05;
   struct spi_ioc_transfer tr[2];
   uint8_t reg;

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   tr[0].cs_change = 0;
   
   tr[1].tx_buf = (unsigned long)NULL;
   tr[1].rx_buf = (unsigned long)sr;
   tr[1].len = 1;
   tr[1].delay_usecs = 0;

   if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) < -1)
      return false;

   return true;
}

bool rr_wrsr(int fd, uint8_t value) {
   uint8_t tx_buffer[2] = {0x01, value};
   struct spi_ioc_transfer tr[1];
   uint8_t reg;

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   
   if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < -1)
      return false;

   return true;
}

bool rr_read(int fd, uint32_t address, uint8_t *value) {
   uint8_t tx_buffer[4] = {0x03, 
      (address & 0x00FF0000) >> 16,
      (address & 0x0000FF00) >> 8,
      (address & 0x000000FF)
   };
   struct spi_ioc_transfer tr[2];

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   tr[0].cs_change = 0;
   
   tr[1].tx_buf = (unsigned long)NULL;
   tr[1].rx_buf = (unsigned long)value;
   tr[1].len = 1;
   tr[1].delay_usecs = 0;

   if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) < -1)
      return false;

   return true;
}

bool rr_write(int fd, uint32_t address, uint8_t value) {
   uint8_t tx_buffer[5] = {0x02, 
      (address & 0x00FF0000) >> 16,
      (address & 0x0000FF00) >> 8,
      (address & 0x000000FF),
      value
   };
   struct spi_ioc_transfer tr[1];

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   tr[0].cs_change = 0;
   
   if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < -1)
      return false;

   return true;
}

bool rr_read_buffer(int fd, uint32_t address, uint8_t *buf, int bufsize) {
   uint8_t tx_buffer[4] = {0x03, 
      (address & 0x00FF0000) >> 16,
      (address & 0x0000FF00) >> 8,
      (address & 0x000000FF)
   };
   struct spi_ioc_transfer tr[2];

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   tr[0].cs_change = 0;
   
   tr[1].tx_buf = (unsigned long)NULL;
   tr[1].rx_buf = (unsigned long)buf;
   tr[1].len = bufsize;
   tr[1].delay_usecs = 0;

   if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) < -1)
      return false;

   return true;
}

bool rr_write_buffer(int fd, uint32_t address, uint8_t *buf, int bufsize) {
   uint8_t tx_buffer[5] = {0x02, 
      (address & 0x00FF0000) >> 16,
      (address & 0x0000FF00) >> 8,
      (address & 0x000000FF),
      buf[0]
   };
   struct spi_ioc_transfer tr[2];

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   tr[0].cs_change = 0;

   tr[1].tx_buf = (unsigned long)buf+1;
   tr[1].rx_buf = (unsigned long)NULL;
   tr[1].len = bufsize;
   tr[1].delay_usecs = 0;
   
   if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) < -1)
      return false;

   return true;

}
