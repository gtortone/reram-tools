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
   const char *conf_file = NULL;
   std::map<std::string, DUT> dut;
   std::map<std::string, MB85AS12MT *> rr;
   
   static const char *const usage[] = {
      s.c_str(),
      NULL
   };

   std::vector<struct argparse_option> options = {
      OPT_HELP(),
      OPT_STRING('c', "config", &conf_file, "YAML config file", NULL, 0, 0),
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
      printf("time=%ld,event=START,name=%s,bus=%s,speed=%d,chipid=0x%X\n", 
         std::time(nullptr), p.first.c_str(), dut[p.first].bus.c_str(), dut[p.first].speed, dut[p.first].chipid);
   }

   // start main loop

      
   return 0;
}
