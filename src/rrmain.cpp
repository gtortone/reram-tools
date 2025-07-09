#include <iostream>
#include <vector>
#include <filesystem>
#include <sstream>
#include <chrono>
#include "argparse.h"
#include "mb85as12mt.h"

int main(int argc, const char **argv) {

	std::string toolname(std::filesystem::path(argv[0]).filename().u8string());
   std::string s = toolname + " [options]";
   const char *spiconf = "0.0";
   int freq = 1;  // 1 MHz
	// rdump
	unsigned int dump_start = 0;
	unsigned int dump_end = 255;
	bool dump_full = false;

   static const char *const usage[] = {
      s.c_str(),
      NULL
   };

   std::vector<struct argparse_option> options = {
      OPT_HELP(),
      OPT_STRING('b', "spi", &spiconf, "SPI bus number and chip select (default: 0.0 - /dev/spidev0.0)", NULL, 0, 0),
      OPT_INTEGER('s', "freq", &freq, "SPI clock frequency in MHz (default: 1 - 1 MHz)"),
   };

	// add options for different tools
	if (toolname == "rrdump") {
		options.push_back(OPT_INTEGER('s', "start", &dump_start, "start address to dump (default: 0)"));
		options.push_back(OPT_INTEGER('e', "end", &dump_end, "end address to dump (default: 256)"));
		options.push_back(OPT_BOOLEAN('f', "full", &dump_full, "full dump"));
      options.push_back(OPT_END());
	}

   struct argparse argparse;
   argparse_init(&argparse, options.data(), usage, 0);
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

   MB85AS12MT *rr;

   rr = new MB85AS12MT(bus[0], bus[1], freq * 1000000);

	printf("> SPI bus: %d, CS: %d, SCK: %d MHz\n", bus[0], bus[1], freq);

	if(toolname == "rrdump") {
		if (dump_full) {
			dump_start = 0;
			dump_end = rr->size-1;
		}
		printf("> dump start: 0x%X (%d), dump end: 0x%X (%d)\n\n", 
			dump_start, dump_start, dump_end, dump_end);
		
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		std::vector<uint8_t> data = rr->readBuffer(dump_start, dump_end-dump_start+1);		
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

		for(unsigned int i=0; i<data.size(); i++) {
			if(i%16 == 0) {
				printf("%06X-%06X: ",
					i+dump_start, (i+15)<data.size()-1?i+dump_start+15:data.size()-1);
				printf("%02X ", data[i]);
			} else {
				printf("%02X ", data[i]);
				if((i+1) % 16 == 0 && i+1 < data.size())
					printf("\n");
				else if((i+1) % 8 == 0 && i+1 < data.size())
					printf(". ");
			}
		}	

		printf("\n\ndump execution time: %lld ms\n\n", 
			std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
	}

	return(0);
}
