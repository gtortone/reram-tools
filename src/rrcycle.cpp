#include <iostream>
#include <ctime>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <map>
#include <thread>
#include <atomic>
#include "yaml-cpp/yaml.h"
#include "yaml-cpp/node/parse.h"
#include "yaml-cpp/node/node.h"
#include "argparse.h"
#include "utils.h"
#include "config.h"
#include "mb85as12mt.h"

#define MAX_FLIP_COUNT     4096

std::atomic<bool> running(true);

void keyListener() {
   enableRawMode();

   while (running) {
      if (getch_nonblock() == 'q') {
         printf("\n\n QUIT command received... \n\n");
         running = false;
         break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
   }

   disableRawMode();
}

int main(int argc, const char **argv) {

   std::string progname(std::filesystem::path(argv[0]).filename().u8string());
   std::string s = progname + " [options]";
   std::map<std::string, DUT> dut;
   std::map<std::string, MB85AS12MT *> rr;
   // rrcycle
   const char *conf_file = NULL;
   const char *input_file = NULL;
   unsigned int injector = 0; 
   int basevalue = -1;
   std::ifstream f;
   std::vector<char> memblock;
   bool do_fill = true;
   std::chrono::steady_clock::time_point run_begin, run_end;
   bool alarm = false;
   
   static const char *const usage[] = {
      s.c_str(),
      NULL
   };

   std::vector<struct argparse_option> options = {
      OPT_HELP(),
      OPT_STRING('c', "config", &conf_file, "YAML config file", NULL, 0, 0),
      OPT_BOOLEAN('i', "injector", &injector, "enable fault injector", NULL, 0, 0),
      OPT_INTEGER(0, "verify-value", &basevalue, "verify memory with value", NULL, 0, 0),
      OPT_STRING(0, "verify-file", &input_file, "verify memory with file", NULL, 0, 0),
      OPT_END()
   };

   struct argparse argparse;
   argparse_init(&argparse, options.data(), usage, 0);
   argparse_describe(&argparse, "Options", "");
   argparse_parse(&argparse, argc, argv);

   if (conf_file == NULL) {
      argparse_usage(&argparse);
      return -1;
   }

   if(input_file != NULL) {
      f.open(input_file, std::ios::in | std::ios::binary | std::ios::ate);

      if(!f.is_open()) {
         printf("E: file %s open error\n", input_file);
         return(-1);
      } 

      std::streamsize size = f.tellg();
      f.seekg(0, std::ios::beg);

      memblock.resize(size);
      f.read(memblock.data(), size);

      do_fill = false;
   }

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
         rr[p.first] = new MB85AS12MT(bus, cs, freq * 1000000); 
      }
   }

   if (rr.size() == 0) {
      printf("E: no valid/enabled DUT found in config file\n");
      return -1;
   }

   for (auto &p : rr) {
      printf("%ld PROG_START %s %s %d 0x%X\n", 
         std::time(nullptr), p.first.c_str(), dut[p.first].bus.c_str(), dut[p.first].speed, dut[p.first].chipid);
   }

   // start main loop

   std::vector<uint8_t> pattern;
   if(basevalue == -1) {
      pattern = { 0xAA, 0x55, 0x00, 0xFF };
   } else {
      pattern = { (uint8_t) basevalue };
      do_fill = false;
   }

   std::chrono::steady_clock::time_point begin, end;
   unsigned int idx = 0;

   run_begin = std::chrono::steady_clock::now();

   std::thread listener(keyListener);

   while(running && !alarm) {

      if (idx == pattern.size())
         idx = 0;

      uint8_t b = pattern[idx++];

      auto t = std::time(nullptr);
      printf("%ld DATETIME %s", std::time(nullptr), std::ctime(&t));

      for (auto &p : rr) {    // for each configured DUT

         if(!running)
            break;

         MB85AS12MT &m = *(p.second);
         std::vector<uint8_t> rrdata(m.size);      // data read from ReRAM
         unsigned int rrlen;                       // data length of payload stored in ReRAM

         if (input_file != NULL)
            rrlen = (rrdata.size() < memblock.size()) ? rrdata.size() : memblock.size();
         else
            rrlen = m.size;

         if(do_fill) {

            // fill

            std::fill(rrdata.begin(), rrdata.end(), b);

            printf("%ld FILL_START %s 0x%X\n",
               std::time(nullptr), p.first.c_str(), b);

            begin = std::chrono::steady_clock::now();
            try {
               m.writeBuffer(0, rrdata);
            } catch (const std::exception &e){
               printf("%ld FILL_ERROR %s 0x%X %s\n",
                  std::time(nullptr), p.first.c_str(), b, 
                  e.what());
               continue;
            }
            end = std::chrono::steady_clock::now();

            printf("%ld FILL_END %s 0x%X %lld\n",
               std::time(nullptr), p.first.c_str(), b, 
               std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
         }

         // fault injector 

         std::vector<uint32_t> pos;
         std::vector<uint8_t> orig_values;

         if(injector) {
            std::srand(unsigned(std::time(nullptr)));
            if(std::rand() % 2 == 1) {
               unsigned int num = (rand() % 10) + 1;     // number of changes [1:10]
               printf("%ld INJECT_START %s %d\n",
                  std::time(nullptr), p.first.c_str(), num);

               // reset address array
               pos.clear();
               pos.resize(num);
               orig_values.clear();

               // generate and sort a random list of addresses
               std::generate(pos.begin(), pos.end(), [&rrlen](){ return std::rand() % rrlen; } );
               std::sort(pos.begin(), pos.end());
               // remove duplicates
               pos.erase(std::unique(pos.begin(), pos.end()), pos.end());

               for(unsigned int i=0; i<pos.size(); i++) {

                  // read value from ReRAM
                  uint8_t rr_value = m.read(pos[i]);
                  orig_values.push_back(rr_value);

                  // generate value to inject
                  unsigned int value;
                  value = rr_value ^ ((rand() % 0xFE) + 1);    // mask [0x01:0xFE]

                  // write value to ReRAM
                  try {
                     m.write(pos[i], value);
                  } catch (const std::exception &e){
                     printf("%ld INJECT_ERROR %s 0x%X %s\n",
                        std::time(nullptr), p.first.c_str(), value,
                        e.what());
                     continue;
                  }

                  // print flip data
                  std::map<uint8_t, uint8_t> mflip;
                  if (input_file != NULL)
                     mflip = bitcheck(memblock[pos[i]], value);
                  else
                     mflip = bitcheck(b, value);

                  printf("%ld INJECT %s 0x%08X %d ",
                     std::time(nullptr), p.first.c_str(), pos[i], mflip.size());
                  for(auto el : mflip)
                     printf("%d:%s ", el.first, (el.second == ZERO_TO_ONE)?"0->1":"1->0");
                  printf("\n");
               }

               printf("%ld INJECT_END %s %d\n",
                  std::time(nullptr), p.first.c_str(), pos.size());
            }
         }

         // read 

         printf("%ld READ_START %s\n",
            std::time(nullptr), p.first.c_str());

         rrdata.clear();

         begin = std::chrono::steady_clock::now();
         try {
            rrdata = m.readBuffer(0, m.size); 
         } catch (const std::exception &e){
            printf("%ld READ_ERROR %s %s\n",
               std::time(nullptr), p.first.c_str(), 
               e.what());
            continue;
         }
         end = std::chrono::steady_clock::now();

         printf("%ld READ_END %s %lld\n",
            std::time(nullptr), p.first.c_str(), 
            std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

         // compare

         unsigned int nmismatch_loc = 0;
         unsigned int nmismatch_bit = 0;

         begin = std::chrono::steady_clock::now();
         
         if (input_file != NULL) {
   
            // compare source: file

            printf("%ld CHECK_START %s %s\n",
               std::time(nullptr), p.first.c_str(), input_file);

            for(unsigned int i=0; i<rrlen; i++) {
               if(rrdata[i] != memblock[i]) {
                  nmismatch_loc++;
                  std::map<uint8_t, uint8_t> m = bitcheck(memblock[i], rrdata[i]);
                  printf("%ld FLIP %s 0x%08X %d ",
                     std::time(nullptr), p.first.c_str(), i, m.size());
                  for(auto el : m) {
                     printf("%d:%s ", el.first, (el.second == ZERO_TO_ONE)?"0->1":"1->0");
                     nmismatch_bit++;
                  } 
                  if(nmismatch_loc > MAX_FLIP_COUNT) {
                     const char *dump_filename = (p.first + ".dump").c_str();
                     printf("%ld DUMP_FILE %s\n", std::time(nullptr), dump_filename);
                     std::remove(dump_filename);
                     std::ofstream outfile(dump_filename, std::ios::binary);
                     outfile.write((const char *) rrdata.data(), rrdata.size());
                     outfile.close();
                     alarm = true;
                     continue;
                  }
                  printf("\n");
               }
            }

            end = std::chrono::steady_clock::now();

            printf("%ld CHECK_COMPLETE %s %d %d %s %lld\n",
               std::time(nullptr), p.first.c_str(), nmismatch_loc, nmismatch_bit, input_file,
               std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
            
         } else {

            // compare source: byte 

            printf("%ld CHECK_START %s 0x%X\n",
               std::time(nullptr), p.first.c_str(), b);

            for(unsigned int i=0; i<rrdata.size(); i++) {
               if(rrdata[i] != b) {
                  nmismatch_loc++;
                  std::map<uint8_t, uint8_t> m = bitcheck(b, rrdata[i]);
                  printf("%ld FLIP %s 0x%08X %d ",
                     std::time(nullptr), p.first.c_str(), i, m.size());
                  for(auto el : m) {
                     printf("%d:%s ", el.first, (el.second == ZERO_TO_ONE)?"0->1":"1->0");
                     nmismatch_bit++;
                  } 
                  if(nmismatch_loc > MAX_FLIP_COUNT) {
                     const char *dump_filename = (p.first + ".dump").c_str();
                     printf("%ld DUMP_FILE %s\n", std::time(nullptr), dump_filename);
                     std::remove(dump_filename);
                     std::ofstream outfile(dump_filename, std::ios::binary);
                     outfile.write((const char *) rrdata.data(), rrdata.size());
                     outfile.close();
                     alarm = true;
                     continue;
                  }
                  printf("\n");
               }
            }

            end = std::chrono::steady_clock::now();

            printf("%ld CHECK_COMPLETE %s %d %d 0x%X %lld\n",
               std::time(nullptr), p.first.c_str(), nmismatch_loc, nmismatch_bit, b,
               std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
         }

         // restore ReRAM original values
         if(injector) {
            for(unsigned int i=0; i<pos.size(); i++) {
               m.write(pos[i], orig_values[i]);
            }
         }
      }
   }

   run_end = std::chrono::steady_clock::now();
   printf("\nRun duration: %lld seconds\n", std::chrono::duration_cast<std::chrono::seconds>(run_end - run_begin).count());  

   listener.join();
   printf("\nBye!");
      
   return 0;
}
