#include <iostream> 
#include <vector>
#include <filesystem>
#include <sstream>
#include "argparse.h"
#include "mb85as12mt.h"
#include <unistd.h>		// sleep

int main(int argc, const char **argv) {

	std::string s = std::filesystem::path(argv[0]).filename().u8string() + " [options]";
	const char *spiconf = "0.0";
	int freq = 1000000;	// 1 MHz
	int start = 0;
	int end = 256;
	//
	std::vector<uint8_t> data;

	static const char *const usage[] = {
   	s.c_str(),
		NULL
   };

	struct argparse_option options[] = {
		OPT_HELP(),
		OPT_STRING('b', "spi", &spiconf, "SPI bus number and chip select (default: 0.0 - /dev/spidev0.0)", NULL, 0, 0),
		OPT_INTEGER('s', "freq", &freq, "SPI clock frequency in Hz (default: 1000000 - 1 MHz)"),
		OPT_INTEGER(0, "start", &start, "start address to dump (default: 0)"),
		OPT_INTEGER(0, "end", &end, "end address to dump (default: 256)"),
		OPT_END()
	};

	struct argparse argparse;
   argparse_init(&argparse, options, usage, 0);
   argparse_describe(&argparse, "Options", "");
   argparse_parse(&argparse, argc, argv);

	// get SPI bus parameters
	std::stringstream ss(spiconf);
	std::string tmp;
   std::vector<unsigned int> bus;
   while(std::getline((ss), tmp, '.')) {
      int value = std::stoul(tmp, nullptr, 10);
      bus.push_back(value);
   }

	if(bus.size() < 2) {
      throw std::runtime_error("E: SPI bus config error: " + std::string(spiconf));
   }

	printf("start: %d, end: %d\n", start, end);

	MB85AS12MT *rr;

	rr = new MB85AS12MT(bus[0], bus[1], freq);

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

	printf("-- fetch address 0-15 inside 'data' buffer\n");
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

	printf("-- fetch address 0-31 inside 'data' buffer\n");
	data = rr->readBuffer(0, 32);

	for(unsigned int i=0;i<data.size();i++)
		printf("data[%d]: 0x%02X\n", i, data[i]);
	printf("\n\n");

	printf("-- dump address 0-%d\n", rr->size-1);
	//rr->dump(0, 1023);
	rr->dump(0,rr->size-1);

	return 0;
}
