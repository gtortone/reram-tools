#include <iostream>
#include <ctime>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <map>
#include "yaml-cpp/yaml.h"
#include "yaml-cpp/node/parse.h"
#include "yaml-cpp/node/node.h"
#include "argparse.h"
#include "utils.h"
#include "mb85as12mt.h"

struct DUT {
   std::string name;
   bool enable;
   std::string bus;
   int speed;
   int chipid;

   DUT() : name(""), enable(true), bus(""), speed(0), chipid(0) {}

   void print(void) {
      printf("enable: %d\n", enable);
      printf("bus: %s\n", bus.c_str());
      printf("speed: %d\n", speed);
      printf("chipid: 0x%X\n", chipid);
   }
};

namespace YAML {
   template<>
   struct convert<DUT> {
      static bool decode(const YAML::Node& node, DUT& obj) {
         if (!node.IsMap()) return false;
         try {
            obj.bus = node["bus"].as<std::string>();
         } catch (YAML::Exception &e) {
            printf("E: bus attribute not found or bad format\n");
            return false;
         }

         try {
            obj.enable = node["enable"].as<bool>();
            obj.speed = node["speed"].as<int>();
            obj.chipid = node["chipid"].as<int>();
         } catch (YAML::Exception &e) {}
         return true;
       }
   };
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

   while(true) {

      if (idx == pattern.size())
         idx = 0;

      uint8_t b = pattern[idx++];

      for (auto &p : rr) {    // for each configured DUT

         MB85AS12MT &m = *(p.second);
         std::vector<uint8_t> data(m.size);

         if(do_fill) {

            // fill

            std::fill(data.begin(), data.end(), b);

            printf("%ld FILL_START %s 0x%X\n",
               std::time(nullptr), p.first.c_str(), b);

            begin = std::chrono::steady_clock::now();
            try {
               m.writeBuffer(0, data);
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

         if(injector) {
            std::srand(unsigned(std::time(nullptr)));
            if(std::rand() % 2 == 1) {
               unsigned int num = (rand() % 10) + 1;     // number of changes [1:10]
               printf("%ld INJECT_START %s %d\n",
                  std::time(nullptr), p.first.c_str(), num);

               // generate and sort a random list of addresses
               std::vector<uint8_t> pos(num);
               std::generate(pos.begin(), pos.end(), [&m](){ return std::rand() % m.size; } );
               std::sort(pos.begin(), pos.end());

               for(unsigned int i=0; i<num; i++) {
                  unsigned int value;
                  // force value not equal to pattern
                  do {
                     value  = rand() % 0x100;      // value [0x00:0xFF]
                  } while (value == b);
                  try {
                     m.write(pos[i], value);
                  } catch (const std::exception &e){
                     printf("%ld INJECT_ERROR %s 0x%X %s\n",
                        std::time(nullptr), p.first.c_str(), value,
                        e.what());
                     continue;
                  }
                  std::map<uint8_t, uint8_t> m = bitcheck(b, value);
                  printf("%ld INJECT %s 0x%08X %d ",
                     std::time(nullptr), p.first.c_str(), pos[i], m.size());
                  for(auto el : m)
                     printf("%d:%s ", el.first, (el.second == ZERO_TO_ONE)?"0->1":"1->0");
                  printf("\n");
               }

               printf("%ld INJECT_END %s %d\n",
                  std::time(nullptr), p.first.c_str(), num);
            }
         }

         // read 

         printf("%ld READ_START %s\n",
            std::time(nullptr), p.first.c_str());

         data.clear();

         begin = std::chrono::steady_clock::now();
         try {
            data = m.readBuffer(0, m.size); 
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

         unsigned int nmismatch = 0;

         begin = std::chrono::steady_clock::now();

         
         if (input_file != NULL) {
   
            // compare source: file

            printf("%ld CHECK_START %s %s\n",
               std::time(nullptr), p.first.c_str(), input_file);

            unsigned int length = (data.size() < memblock.size()) ? data.size() : memblock.size();

            for(unsigned int i=0; i<length; i++) {
               if(data[i] != memblock[i]) {
                  nmismatch++;
                  std::map<uint8_t, uint8_t> m = bitcheck(b, data[i]);
                  printf("%ld FLIP %s 0x%08X %d ",
                     std::time(nullptr), p.first.c_str(), i, m.size());
                  for(auto el : m) {
                     printf("%d:%s ", el.first, (el.second == ZERO_TO_ONE)?"0->1":"1->0");
                  } 
                  printf("\n");
               }
            }

            end = std::chrono::steady_clock::now();

            printf("%ld CHECK_COMPLETE %s %s %d %lld\n",
               std::time(nullptr), p.first.c_str(), input_file, nmismatch,
               std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
            
         } else {

            // compare source: byte 

            printf("%ld CHECK_START %s 0x%X\n",
               std::time(nullptr), p.first.c_str(), b);

            for(unsigned int i=0; i<data.size(); i++) {
               if(data[i] != b) {
                  nmismatch++;
                  std::map<uint8_t, uint8_t> m = bitcheck(b, data[i]);
                  printf("%ld FLIP %s 0x%08X %d ",
                     std::time(nullptr), p.first.c_str(), i, m.size());
                  for(auto el : m) {
                     printf("%d:%s ", el.first, (el.second == ZERO_TO_ONE)?"0->1":"1->0");
                  } 
                  printf("\n");
               }
            }

            end = std::chrono::steady_clock::now();

            printf("%ld CHECK_COMPLETE %s 0x%X %d %lld\n",
               std::time(nullptr), p.first.c_str(), b, nmismatch,
               std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
         }
      }
   }
      
   return 0;
}
