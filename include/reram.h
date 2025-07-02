#include <stdint.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

bool rr_rdid(int fd, uint8_t *buf, int *bufsize);
bool rr_rduid(int fd, uint8_t *buf, int *bufsize);

bool rr_wren(int fd);
bool rr_wrdi(int fd);

bool rr_rdsr(int fd, uint8_t *sr);
bool rr_wrsr(int fd, uint8_t value);

bool rr_read(int fd, uint32_t address, uint8_t *value);
bool rr_write(int fd, uint32_t address, uint8_t value);

bool rr_read_buffer(int fd, uint32_t address, uint8_t *buf, int bufsize);
bool rr_write_buffer(int fd, uint32_t address, uint8_t *buf, int bufsize);
