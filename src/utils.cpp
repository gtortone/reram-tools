#include <cstdio>
#include "utils.h"

void prettyPrint(std::vector<uint8_t> data, const unsigned int offset) {

   for(unsigned int i=0; i<data.size(); i++) {
      if(i%16 == 0) {
         printf("%06X-%06X: ",
            i+offset, (i+15)<data.size()-1?i+offset+15:data.size()-1+offset);
         printf("%02X ", data[i]);
      } else {
         printf("%02X ", data[i]);
         if((i+1) % 16 == 0 && i+1 < data.size())
            printf("\n");
         else if((i+1) % 8 == 0 && i+1 < data.size())
            printf(". ");
      }
   }
}
