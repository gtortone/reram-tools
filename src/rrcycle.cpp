#include <iostream>
#include <ctime>
#include <filesystem>
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
   unsigned int polluter = 0; 
   
   static const char *const usage[] = {
      s.c_str(),
      NULL
   };

   std::vector<struct argparse_option> options = {
      OPT_HELP(),
      OPT_STRING('c', "config", &conf_file, "YAML config file", NULL, 0, 0),
      OPT_BOOLEAN('p', "polluter", &polluter, "enable polluter on read data", NULL, 0, 0),
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
      printf("time=%ld,event=prog.start,name=%s,bus=%s,speed=%d,chipid=0x%X\n", 
         std::time(nullptr), p.first.c_str(), dut[p.first].bus.c_str(), dut[p.first].speed, dut[p.first].chipid);
   }

   // start main loop

   std::vector<uint8_t> pattern = { 0xAA, 0x55, 0x00, 0xFF };
   std::chrono::steady_clock::time_point begin, end;

   while(true) {

      for (auto &p : rr) {    // for each configured DUT

         MB85AS12MT &m = *(p.second);

         for(auto b : pattern) {    // for each pattern

            // fill

            std::vector<uint8_t> data(m.size, b);

            printf("time=%ld,event=fill.start,name=%s,pattern=0x%X\n",
               std::time(nullptr), p.first.c_str(), b);

            begin = std::chrono::steady_clock::now();
            try {
               m.writeBuffer(0, data);
            } catch (const std::exception &e){
               printf("time=%ld,event=fill.error,name=%s,pattern=0x%X,error=%s\n",
                  std::time(nullptr), p.first.c_str(), b, 
                  e.what());
               continue;
            }
            end = std::chrono::steady_clock::now();

            printf("time=%ld,event=fill.end,name=%s,pattern=0x%X,elapsed_ms=%lld\n",
               std::time(nullptr), p.first.c_str(), b, 
               std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

            // read 

            printf("time=%ld,event=read.start,name=%s\n",
               std::time(nullptr), p.first.c_str());

            data.clear();

            begin = std::chrono::steady_clock::now();
            try {
               data = m.readBuffer(0, m.size); 
            } catch (const std::exception &e){
               printf("time=%ld,event=read.error,name=%s,error=%s\n",
                  std::time(nullptr), p.first.c_str(), 
                  e.what());
               continue;
            }
            end = std::chrono::steady_clock::now();

            printf("time=%ld,event=read.end,name=%s,elapsed_ms=%lld\n",
               std::time(nullptr), p.first.c_str(), 
               std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

            // polluter

            if(polluter) {
               srand(time(0));
               if(rand() % 2 == 1) {
                  unsigned int num = (rand() % 10) + 1;     // number of changes [1,10]
                  printf("time=%ld,event=polluter.start,name=%s,num_changes=%d\n",
                     std::time(nullptr), p.first.c_str(), num);

                  for(unsigned int i=0; i<num; i++) {
                     unsigned int pos = rand() % m.size;
                     unsigned int value = rand() % 17;
                     data[pos] = value;
                     printf("time=%ld,event=polluter.exec,name=%s,pos=%d,value=0x%X\n",
                        std::time(nullptr), p.first.c_str(), pos, value);
                  }

                  printf("time=%ld,event=polluter.end,name=%s,num_changes=%d\n",
                     std::time(nullptr), p.first.c_str(), num);
               }
            }

            // compare

            unsigned int nmismatch = 0;

            printf("time=%ld,event=compare.start,name=%s,pattern=0x%X\n",
               std::time(nullptr), p.first.c_str(), b);

            begin = std::chrono::steady_clock::now();
            for(unsigned int i=0; i<data.size(); i++) {
               if(data[i] != b) {
                  nmismatch++;
                  printf("time=%ld,event=compare.mismatch,name=%s,pattern=0x%X,pos=%d,value=0x%X\n",
                     std::time(nullptr), p.first.c_str(), b, i, data[i]);
               }
            }
            end = std::chrono::steady_clock::now();

            printf("time=%ld,event=compare.end,name=%s,pattern=0x%X,num_mismatch=%d,elapsed_ms=%lld\n",
               std::time(nullptr), p.first.c_str(), b, nmismatch,
               std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
         }
      }
   }
      
   return 0;
}
