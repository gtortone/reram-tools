#include <iostream>
#include <vector>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <chrono>
#include "argparse.h"
#include "utils.h"
#include "config.h"
#include "mb85as12mt.h"

int main(int argc, const char **argv) {

   std::string toolname(std::filesystem::path(argv[0]).filename().u8string());
   std::string s = toolname + " [options]";
   const char *spiconf = "0.0";
   int freq = 1;  // 1 MHz
   const char *conf_file = NULL;
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
   const char *input_file = NULL;

   std::chrono::steady_clock::time_point begin, end;

   static const char *const usage[] = {
      s.c_str(),
      NULL
   };

   std::vector<struct argparse_option> options = {
      OPT_HELP(),
      OPT_STRING('c', "config", &conf_file, "YAML config file", NULL, 0, 0),
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

      options.push_back(OPT_INTEGER('v', "value", &fill_byte, "byte to fill whole memory (default: 0)", NULL, 0, 0));
      options.push_back(OPT_BOOLEAN('y', "yes", &fill_proceed, "do not ask confirmation", NULL, 0, 0));
      options.push_back(OPT_END());

   } else if (toolname == "rrread") {

      options.push_back(OPT_INTEGER('a', "addr", &read_addr, "address to read (default: 0)", NULL, 0, 0));
      options.push_back(OPT_INTEGER('n', "nb", &read_nbytes, "number of bytes to read (default: 1)", NULL, 0, 0));

   } else if (toolname == "rrwrite") {

      options.push_back(OPT_INTEGER('a', "addr", &write_addr, "address to write (default: 0)", NULL, 0, 0));
      options.push_back(OPT_INTEGER('v', "value", &write_byte, "byte to write (default: 0)", NULL, 0, 0));
      options.push_back(OPT_STRING('f', "file", &input_file, "file to write", NULL, 0, 0));
      options.push_back(OPT_BOOLEAN('y', "yes", &write_proceed, "do not ask confirmation", NULL, 0, 0));
   }

   struct argparse argparse;
   argparse_init(&argparse, options.data(), usage, 0);
   argparse_describe(&argparse, "Options", "");
   argparse_parse(&argparse, argc, argv);

   std::map<std::string, DUT> dut;
   std::map<std::string, MB85AS12MT *> rrmap;

   if(conf_file != NULL) {

      YAML::Node config = YAML::LoadFile(conf_file);

      for (YAML::const_iterator it = config.begin(); it != config.end(); ++it) {

         std::string name;
         try {
            name = (*it)["name"].as<std::string>();
         } catch (YAML::Exception &e) {
            std::cout << "E: " << e.what() << std::endl;
            continue;
         }

         try {
            dut[name] = it->as<DUT>();
         } catch (YAML::Exception &e) {
            // skip bad DUT
            std::cout << "E: " << e.what() << std::endl;
            continue;
         }
      }

      for (auto &p : dut) {
         // get SPI bus parameters
         std::stringstream ss(p.second.bus);
         std::string tmp;
         // bus
         std::getline((ss), tmp, '.');
         int bus = std::stoul(tmp, nullptr, 10);
         // cs
         std::getline((ss), tmp, '.');
         int cs = std::stoul(tmp, nullptr, 10);
         int freq = p.second.speed?p.second.speed:1;

         if(p.second.enable == 1) {
            rrmap[p.first] = new MB85AS12MT(bus, cs, freq * 1000000);
         }
      }

      if (rrmap.size() == 0) {
         printf("E: no valid/enabled DUT found in config file\n");
         return -1;
      }      

      printf("> configuration file: %s\n", conf_file);
      
   } else {

      printf("> command line parameters\n");      

      // get SPI bus parameters
      std::vector<unsigned int> bus;
      std::stringstream ss(spiconf);
      std::string tmp;
      while(std::getline((ss), tmp, '.')) {
         int value = std::stoul(tmp, nullptr, 10);
         bus.push_back(value);
      }

      if(bus.size() < 2) {
         throw std::runtime_error("E: SPI bus config error: " + std::string(spiconf));
      }

      rrmap[spiconf] = new MB85AS12MT(bus[0], bus[1], freq * 1000000);
   }

   for (auto &p : rrmap) {

      MB85AS12MT &rr = *(p.second);

      printf("> SPI bus: %d, CS: %d, SCK: %d MHz\n", rr.getBus(), rr.getCS(), rr.getSpeed() / 1000000); 

      if(toolname == "rrdump") {

         if (dump_full) {
            dump_start = 0; dump_end = rr.size-1;
         }
         printf("> dump start: 0x%X (%d), dump end: 0x%X (%d)\n\n", 
            dump_start, dump_start, dump_end, dump_end);
         
         begin = std::chrono::steady_clock::now();
         std::vector<uint8_t> data = rr.readBuffer(dump_start, dump_end-dump_start+1);		
         end = std::chrono::steady_clock::now();

         prettyPrint(data, dump_start);

         printf("\n\ndump execution time: %lld ms\n\n", 
            std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

      } else if(toolname == "rrinfo") {

         rr.printInfo();
         printf("\n");

      } else if(toolname == "rrfill") {

         printf("> fill byte: 0x%X (%d)\n", fill_byte, fill_byte);
         if (!fill_proceed) {
            printf("\nAre you sure (y/n): ");
            char c = std::tolower(getchar());
            while (getchar() != '\n');
            if (c == 'n')
               continue;
         }
         
         printf("\nOperation in progress...\n");

         std::vector<uint8_t> data(rr.size, fill_byte);

         begin = std::chrono::steady_clock::now();
         try {
            rr.writeBuffer(0, data);			
         } catch(...) {
            printf("E: rrfill abort\n");
            continue;
         }
         end = std::chrono::steady_clock::now();

         printf("\nfill execution time: %lld ms\n\n", 
            std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

      } else if(toolname == "rrread") {

         printf("> read addr: 0x%X (%d), nbytes: %d\n\n", read_addr, read_addr, read_nbytes);			

         begin = std::chrono::steady_clock::now();
         std::vector data = rr.readBuffer(read_addr, read_nbytes);
         end = std::chrono::steady_clock::now();

         prettyPrint(data, read_addr);

         printf("\n\nread execution time: %lld ms\n\n", 
            std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

      } else if(toolname == "rrwrite") {

         if (input_file != NULL) {

            std::ifstream f(input_file, std::ios::in | std::ios::binary | std::ios::ate);

            if(!f.is_open()) {
               printf("E: file %s open error\n", input_file);
               return(-1);
            }

            std::streamsize fsize = f.tellg();

            char *memblock = new char[rr.size];
            f.seekg(0, std::ios::beg);
            f.read(memblock, rr.size);

            printf("> write file: %s, number of bytes: %d, file size: %d\n\n", input_file, f.gcount(), fsize);

            if (!write_proceed) {
               printf("\nAre you sure (y/n): ");
               char c = std::tolower(getchar());
               while (getchar() != '\n');
               if (c == 'n')
                  continue;
            }

            begin = std::chrono::steady_clock::now();
            try {
               rr.writeBuffer(0, (uint8_t *) memblock, f.gcount());
            } catch(...) {
               printf("E: rrwrite abort\n");
               continue;
            }
            end = std::chrono::steady_clock::now(); 

            f.close();
            
         } else {

            printf("> write addr: 0x%X (%d), byte: 0x%X (%d)\n\n", 
               write_addr, write_addr, write_byte, write_byte);
            if (!write_proceed) {
               printf("\nAre you sure (y/n): ");
               char c = std::tolower(getchar());
               while (getchar() != '\n');
               if (c == 'n')
                  continue;
            }
            begin = std::chrono::steady_clock::now();
            try {
               rr.write(write_addr, write_byte);
            } catch(...) {
               printf("E: rrwrite abort\n");
               continue;
            }
            end = std::chrono::steady_clock::now();
         }

         printf("\nwrite execution time: %lld ms\n\n", 
            std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

      } else {
         
         printf("E: tool '%s' not found\n", toolname.c_str());
         return(-1);
      }
   }

   return(0);
}
