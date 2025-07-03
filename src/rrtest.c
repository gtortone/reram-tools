#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include "reram.h"

#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_SPEED 1000000
#define SPI_MODE  (SPI_MODE_0 | SPI_3WIRE)
#define SPI_BITS_PER_WORD 8

//#define RERAM_SIZE	1024
//#define RERAM_OFFSET	0
//
#define RERAM_SIZE	1572864
#define RERAM_OFFSET	0
//
#define BYTE			0xFF

int main(void) {

   int fd, ret;
   uint8_t sr_value;
   uint8_t sr;
   uint32_t m_address;
   uint8_t m_value;

   uint8_t memory[158];

   // setup SPI device
   fd = open(SPI_DEVICE, O_RDWR);
   if (fd < 0) {
      perror("open error");
      return -1;
   }

   int mode = SPI_MODE;
   ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
   if (ret == -1) {
      perror("setup error");
      close(fd);
      return -1;
   }

   int speed = SPI_SPEED;
   ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
   if (ret == -1) {
      perror("speed error");
      close(fd);
      return -1;
   }

   int bits = SPI_BITS_PER_WORD;
   ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
      if (ret == -1) {
         perror("bit error");
         close(fd);
         return -1;
      }

   uint8_t buf[16];
   int bsize;

   // RDID
   printf("RDID command (0x9F)\n");
   if(!rr_rdid(fd, buf, &bsize)) {
      perror("rr_rdid error");
      close(fd);
      return -1; 
   }
  
   printf("RX data:\n");
   for (int i = 0; i < bsize; i++) {
      printf("0x%02X ", buf[i]);
   }
   printf("\n\n");

   // RDUID
   printf("RDUID command (0x83)\n");
   if(!rr_rduid(fd, buf, &bsize)) {
      perror("rr_rduid error");
      close(fd);
      return -1; 
   }
  
   printf("RX data:\n");
   for (int i = 0; i < bsize; i++) {
      printf("0x%02X ", buf[i]);
   }
   printf("\n\n");

   // RDSR
   printf("RDSR command (0x05)\n");
   if(!rr_rdsr(fd, &sr)) {
      perror("rr_rdsr error");
      close(fd);
      return -1; 
   }
   printf("RX data:\n");
   printf("0x%02X\n", sr);

   // WRSR
   sr_value = 0xE0;
   printf("WRSR command (0x01) value 0x%02X\n", sr_value);
   if(!rr_wrsr(fd, sr_value)) {
      perror("rr_wrsr error");
      close(fd);
      return -1; 
   }

   // RDSR
   printf("RDSR command (0x05)\n");
   if(!rr_rdsr(fd, &sr)) {
      perror("rr_rdsr error");
      close(fd);
      return -1; 
   }
   printf("RX data:\n");
   printf("0x%02X\n", sr);

   // WRSR
   sr_value = 0x00;
   printf("WRSR command (0x01) value 0x%02X\n", sr_value);
   if(!rr_wrsr(fd, sr_value)) {
      perror("rr_wrsr error");
      close(fd);
      return -1; 
   }

   // RDSR
   printf("RDSR command (0x05)\n");
   if(!rr_rdsr(fd, &sr)) {
      perror("rr_rdsr error");
      close(fd);
      return -1; 
   }
   printf("RX data:\n");
   printf("0x%02X\n", sr);

   printf("\n\n");
   
   // READ
   printf("READ memory address 0x0\n");
   m_address = 0x0;
   if(!rr_read(fd, m_address, &m_value)) {
      perror("rr_read error");
      close(fd);
      return -1;
   }
   printf("RX data:\n");
   printf("0x%02X\n", m_value); 
   
   // WRITE
   printf("WRITE memory address 0x0 value 0x48\n");
   m_address = 0x0;
   m_value = 0x48;
   if(!rr_write(fd, m_address, m_value)) {
      perror("rr_write error");
      close(fd);
      return -1;
   }

   // READ
   printf("READ memory address 0x0\n");
   m_address = 0x0;
   if(!rr_read(fd, m_address, &m_value)) {
      perror("rr_read error");
      close(fd);
      return -1;
   }
   printf("RX data:\n");
   printf("0x%02X\n", m_value); 

   printf("\n\n");

   // READ with auto-increment
   printf("READ memory with auto-increment\n");
   if(!rr_read_buffer(fd, m_address, memory, sizeof(memory))) {
      perror("rr_read_buffer error");
      close(fd);
      return -1;
   }

   for(int i=0;i<sizeof(memory);i++) {
      if(i>0 && i%8 == 0)
         printf("\n");
      printf("0x%02X ", memory[i]);
   }

   printf("\n\n");

   // WRITE
   printf("WRITE memory with auto-increment\n");
   uint8_t wbuf[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
   if(!rr_write_buffer(fd, m_address, wbuf, sizeof(wbuf))) {
      perror("rr_write_buffer error");
      close(fd);
      return -1;
   }

   printf("\n\n");

   // READ with auto-increment
   printf("READ memory with auto-increment\n");
   if(!rr_read_buffer(fd, m_address, memory, sizeof(memory))) {
      perror("rr_read_buffer error");
      close(fd);
      return -1;
   }

   for(int i=0;i<sizeof(memory);i++) {
      if(i>0 && i%8 == 0)
         printf("\n");
      printf("0x%02X ", memory[i]);
   }

   printf("\n\n");

   // RDSR
   printf("RDSR command (0x05)\n");
   if(!rr_rdsr(fd, &sr)) {
      perror("rr_rdsr error");
      close(fd);
      return -1; 
   }
   printf("RX data:\n");
   printf("0x%02X\n", sr);

   printf("\n\n");

	int size = RERAM_SIZE;
	uint8_t *array_ff = (uint8_t *) malloc(size);

	for(int i=0;i<size;i++) {
		//array_ff[i] = i%0x100;
		array_ff[i] = BYTE;
		//printf("array_ff[%d]: 0x%02X\n", i, array_ff[i]);
	}

	// FILL memory
	printf("FILL memory with 0x00-0xFF starting at offset %d\n", RERAM_OFFSET);
	//printf("FILL memory with %d starting at offset %d\n", BYTE, RERAM_OFFSET);
   if(!rr_write_buffer(fd, RERAM_OFFSET, array_ff, size)) {
      perror("rr_write_buffer error");
      close(fd);
      return -1;
   }

	uint8_t *reram = (uint8_t *) malloc(RERAM_SIZE);
	memset(reram, 0, RERAM_SIZE);

   // RDSR
   printf("RDSR command (0x05)\n");
   if(!rr_rdsr(fd, &sr)) {
      perror("rr_rdsr error");
      close(fd);
      return -1; 
   }
   printf("RX data:\n");
   printf("0x%02X\n", sr);

   printf("\n\n");

   // READ with auto-increment
   printf("READ memory with auto-increment\n");
   if(!rr_read_buffer(fd, 0, reram, RERAM_SIZE)) {
      perror("rr_read_buffer error");
      close(fd);
      return -1;
   }

   for(int i=0;i<RERAM_SIZE;i++) {
      if(i>0 && i%8 == 0)
         printf("\n");
      printf("0x%02X ", reram[i]);
   }

   printf("\n\n");

   // RDSR
   printf("RDSR command (0x05)\n");
   if(!rr_rdsr(fd, &sr)) {
      perror("rr_rdsr error");
      close(fd);
      return -1; 
   }
   printf("RX data:\n");
   printf("0x%02X\n", sr);

   printf("\n\n");

   close(fd);
   return 0;
}
