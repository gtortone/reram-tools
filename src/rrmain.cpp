#include <iostream>
#include <vector>
#include <filesystem>
#include <sstream>
#include <chrono>
#include "argparse.h"
#include "utils.h"
#include "mb85as12mt.h"

int main(int argc, const char **argv) {

	std::string toolname(std::filesystem::path(argv[0]).filename().u8string());
   std::string s = toolname + " [options]";
   const char *spiconf = "0.0";
   int freq = 1;  // 1 MHz
	// rdump
	unsigned int dump_start = 0;
	unsigned int dump_end = 255;
	unsigned int dump_full = 0;
	// rrfill
	unsigned int fill_byte = 0x00;
	unsigned int fill_proceed = 0;
	// rrread
	unsigned int read_addr = 0x00;
	unsigned int read_nbytes = 1;
   // rrwrite
   unsigned int write_addr = 0x00;
   unsigned int write_byte = 0x00;
   unsigned int write_proceed = 0;

   static const char *const usage[] = {
      s.c_str(),
      NULL
   };

   std::vector<struct argparse_option> options = {
      OPT_HELP(),
      OPT_STRING('b', "spi", &spiconf, "SPI bus number and chip select (default: 0.0 - /dev/spidev0.0)", NULL, 0, 0),
      OPT_INTEGER('s', "freq", &freq, "SPI clock frequency in MHz (default: 1 - 1 MHz)", NULL, 0, 0),
   };

   if (toolname == "rrmain") {
      
      if(argc > 1)
         toolname = std::string(argv[1]);
      else {
         printf("syntax: %s <toolname>\n", toolname.c_str());
         printf("I: supported tools: \n");
         printf("\trrdump\n");
         printf("\trrinfo\n");
         printf("\trrfill\n");
         printf("\trrread\n");
         printf("\trrwrite\n");
         printf("\n");
         return 0;
      }
   }

	// add options for different tools
	if (toolname == "rrdump") {

		options.push_back(OPT_INTEGER(0, "start", &dump_start, "start address to dump (default: 0)", NULL, 0, 0));
		options.push_back(OPT_INTEGER(0, "end", &dump_end, "end address to dump (default: 256)", NULL, 0, 0));
		options.push_back(OPT_BOOLEAN('f', "full", &dump_full, "full dump", NULL, 0, 0));
      options.push_back(OPT_END());

	} else if (toolname == "rrfill") {

		options.push_back(OPT_INTEGER('c', "byte", &fill_byte, "byte to fill whole memory (default: 0)", NULL, 0, 0));
		options.push_back(OPT_BOOLEAN('y', "yes", &fill_proceed, "do not ask confirmation", NULL, 0, 0));
      options.push_back(OPT_END());

	} else if (toolname == "rrread") {

		options.push_back(OPT_INTEGER('a', "addr", &read_addr, "address to read (default: 0)", NULL, 0, 0));
		options.push_back(OPT_INTEGER('n', "nb", &read_nbytes, "number of bytes to read (default: 1)", NULL, 0, 0));

	} else if (toolname == "rrwrite") {
		options.push_back(OPT_INTEGER('a', "addr", &write_addr, "address to write (default: 0)", NULL, 0, 0));
		options.push_back(OPT_INTEGER('c', "byte", &write_byte, "byte to write (default: 0)", NULL, 0, 0));
		options.push_back(OPT_BOOLEAN('y', "yes", &write_proceed, "do not ask confirmation", NULL, 0, 0));
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

   printf("> SPI bus: %d, CS: %d, SCK: %d MHz\n", bus[0], bus[1], freq); if(toolname == "rrdump") {
   if (dump_full) {
      dump_start = 0; dump_end = rr->size-1;
   }
   printf("> dump start: 0x%X (%d), dump end: 0x%X (%d)\n\n", 
      dump_start, dump_start, dump_end, dump_end);
   
   std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
   std::vector<uint8_t> data = rr->readBuffer(dump_start, dump_end-dump_start+1);		
   std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

   prettyPrint(data, dump_start);

   printf("\n\ndump execution time: %lld ms\n\n", 
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

   } else if(toolname == "rrinfo") {

      rr->printInfo();

   } else if(toolname == "rrfill") {

      // ask confirmation

      printf("> fill byte: 0x%X (%d)\n", fill_byte, fill_byte);
      if (!fill_proceed) {
         printf("\nAre you sure (y/n): ");
         if (std::tolower(getchar()) == 'n')
            return 0;
      }
      
      printf("\nOperation in progress...\n");

      std::vector<uint8_t> data(rr->size, fill_byte);

      std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
      rr->writeBuffer(0, data);			
      std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

      printf("\nfill execution time: %lld ms\n\n", 
         std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

   } else if(toolname == "rrread") {

      printf("> read addr: 0x%X (%d), nbytes: %d\n\n", read_addr, read_addr, read_nbytes);			

      std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
      std::vector data = rr->readBuffer(read_addr, read_nbytes);
      std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

      prettyPrint(data, read_addr);

      printf("\n\nread execution time: %lld ms\n\n", 
         std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

   } else if(toolname == "rrwrite") {

      printf("> write addr: 0x%X (%d), byte: 0x%X (%d)\n\n", 
         write_addr, write_addr, write_byte, write_byte);
      if (!write_proceed) {
         printf("\nAre you sure (y/n): ");
         if (std::tolower(getchar()) == 'n')
            return 0;
      }

      std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
      rr->write(write_addr, write_byte);
      std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

      printf("\nwrite execution time: %lld ms\n\n", 
         std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

   } else {
      
      printf("E: program '%s' not found\n", toolname.c_str());
   }

   return(0);
}
