#include <iostream>
#include <vector>
#include "mb85as12mt.h"

int main(void) {

	MB85AS12MT *rr;

	rr = new MB85AS12MT(0, 0, 1000000);

	printf("-- print ReRAM info\n");
	rr->printInfo();
	printf("\n\n");

	printf("-- write 0xE0 to SR, read SR, write 0x00 to SR\n");
	printf("Status register: 0x%02X\n", rr->readStatusRegister());
	rr->writeStatusRegister(0xE0);
	printf("Status register: 0x%02X\n", rr->readStatusRegister());
	rr->writeStatusRegister(0x00);
	printf("Status register: 0x%02X\n", rr->readStatusRegister());
	printf("\n\n");

	printf("-- write 0x00 to address 0-15\n");
	for(int i=0;i<16;i++)
		rr->write(i, 0x00);
	printf("\n\n");
	
	printf("-- read address 0-15\n");
	for(int i=0;i<16;i++)
		printf("address[%d]: 0x%02X\n", i, rr->read(i));
	printf("\n\n");

	printf("-- write 0x42 to address 0,2,4\n");
	rr->write(0, 0x42);
	rr->write(2, 0x42);
	rr->write(4, 0x42);
	printf("\n\n");

	printf("-- read address 0-15 with single read\n");
	for(int i=0;i<16;i++)
		printf("address[%d] 0x%02X\n", i, rr->read(i));
	printf("\n\n");

	std::vector<uint8_t> data;

	printf("-- read address 0-15 inside 'data' buffer\n");
	data = rr->readBuffer(0, 16);

	for(unsigned int i=0;i<data.size();i++)
		printf("data[%d]: 0x%02X\n", i, data[i]);
	printf("\n\n");

	printf("-- fill local 'data' buffer with 0x88\n");
	for(unsigned int i=0;i<data.size();i++)
		data[i] = 0x88;
	printf("\n\n");

	printf("-- write 'data' buffer at offset 4\n");
	rr->writeBuffer(4, data);
	printf("\n\n");

	printf("-- read address 0-31 inside 'data' buffer\n");
	data = rr->readBuffer(0, 32);

	for(unsigned int i=0;i<data.size();i++)
		printf("data[%d]: 0x%02X\n", i, data[i]);
	printf("\n\n");

	return 0;
}
