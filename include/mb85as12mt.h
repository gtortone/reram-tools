
#ifndef MB85AS12MT_H
#define MB85AS12MT_H

#include <stdint.h>
#include <vector>

class MB85AS12MT {

public:

	typedef union UID {
		struct {
			uint8_t manufacturer_id:8;
			uint8_t continuation_code:8;
			uint16_t product_id:16;
			char lot_id[6];		// add '\0'
			uint8_t wafer_id:8;
			uint16_t chip_id:16;
		};
	} UID;

	typedef union DeviceId {
		struct {
			uint16_t product_id:16;
			uint8_t continuation_code:8;
			uint8_t manufacturer_id:8;
		};
		uint32_t value:32;
	} DeviceId;

	const uint32_t size = 1572864;

	MB85AS12MT(const int bus, const int cs, const int speed);
	~MB85AS12MT(void);

	MB85AS12MT::DeviceId getDeviceId(void);
	MB85AS12MT::UID getUID(void);
	uint64_t getUniqueId(void);
	void printInfo(void);

	void writeEnable(void);
	void writeDisable(void);
	bool writeInProgress(void);

	uint8_t readStatusRegister(void);
	void writeStatusRegister(const uint8_t value);

	uint8_t read(const uint32_t address);
	void write(const uint32_t address, const uint8_t value);

	void readBuffer(uint32_t offset, uint8_t *buf, uint32_t bufsize);
	void writeBuffer(uint32_t offset, uint8_t *buf, uint32_t bufsize);

	std::vector<uint8_t> readBuffer(uint32_t offset, uint32_t nb);
	void writeBuffer(uint32_t offset, std::vector<uint8_t> buffer);

	void dump(uint32_t start, uint32_t end);

private:
	int fd;
	uint32_t spidev_bufsize;
};

#endif
