#include <cstdio>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

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

std::map<uint8_t, uint8_t> bitcheck(uint8_t pattern, uint8_t value) {

   std::map<uint8_t, uint8_t> m;
   uint8_t diff = pattern ^ value;

   for(int i=0; i<8; i++) {
      if(diff & 1<<i) {
         if(pattern & 1<<i)
            m[i] = ONE_TO_ZERO;
         else
            m[i] = ZERO_TO_ONE; 
      }
   }

   return m;
}

void enableRawMode(void) {
    termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

void disableRawMode(void) {
    termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= ICANON | ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    fcntl(STDIN_FILENO, F_SETFL, 0);
}


char getch_nonblock(void) {
    char c = 0;
    read(STDIN_FILENO, &c, 1);
    return c;
}


