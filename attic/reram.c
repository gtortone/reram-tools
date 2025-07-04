#include <stdio.h>	// printf
#include <string.h>  // memset
#include <unistd.h>	// sleep
#include "reram.h"

// from /sys/module/spidev/parameters/bufsiz
#define SPIDEV_BUFSIZE	131072

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

	if(!rr_wren(fd))
		return false;

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   
   if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < -1)
      return false;

	while(rr_wip(fd))
		;

	if(!rr_wrdi(fd))
		return false;

   return true;
}

bool rr_wip(int fd) {
	uint8_t sr;
	rr_rdsr(fd, &sr);
	return (sr & 0x01);
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

	if(!rr_wren(fd))
		return false;

   memset(&tr, 0, sizeof(tr));
   tr[0].tx_buf = (unsigned long)&tx_buffer;
   tr[0].rx_buf = (unsigned long)NULL;
   tr[0].len = sizeof(tx_buffer);
   tr[0].delay_usecs = 0;
   tr[0].cs_change = 0;
   
   if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < -1)
      return false;

	while(rr_wip(fd))
		;

	if(!rr_wrdi(fd))
		return false;

   return true;
}

bool rr_read_buffer(int fd, uint32_t offset, uint8_t *buf, int bufsize) {
   struct spi_ioc_transfer tr[2];
	uint8_t tx_buffer[4];

	int nbuf, remainder;
	uint32_t m_address = offset;		// ReRAM address
	uint32_t b_address = 0;				// buffer address

	if (bufsize < SPIDEV_BUFSIZE) {
		nbuf = 1;
		remainder = 0;
	} else {
		nbuf = (bufsize / SPIDEV_BUFSIZE);
		remainder = bufsize % SPIDEV_BUFSIZE;
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
		if (bufsize > SPIDEV_BUFSIZE)
			tr[1].len = SPIDEV_BUFSIZE;
		else
			tr[1].len = bufsize;
		tr[1].delay_usecs = 0;

		if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) < -1)
			return false;

		m_address += SPIDEV_BUFSIZE;
		b_address += SPIDEV_BUFSIZE;
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
			return false;

		while(rr_wip(fd))
			;
	}

   return true;
}

bool rr_write_buffer(int fd, uint32_t offset, uint8_t *buf, int bufsize) {
   struct spi_ioc_transfer tr[2];
	uint8_t tx_buffer[5];

	int nbuf, remainder;
	uint32_t m_address = offset;		// ReRAM address
	uint32_t b_address = 0;				// buffer address

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

		if(!rr_wren(fd))
			return false;

		if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) < -1)
			return false;

		while(rr_wip(fd))
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

		if(!rr_wren(fd))
			return false;

		if (ioctl(fd, SPI_IOC_MESSAGE(2), &tr) < -1)
			return false;

		while(rr_wip(fd))
			;
	}

	if(!rr_wrdi(fd))
		return false;

	//printf("rr_write_buffer DONE\n");

   return true;
}

