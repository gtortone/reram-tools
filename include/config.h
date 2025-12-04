#include <string>
#include "yaml-cpp/yaml.h"
#include "yaml-cpp/node/parse.h"
#include "yaml-cpp/node/node.h"

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

