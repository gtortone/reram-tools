#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <stdint.h>
#include <map>

#define ZERO_TO_ONE  0x01
#define ONE_TO_ZERO  0x10

void prettyPrint(std::vector<uint8_t> data, const unsigned int offset=0);
std::map<uint8_t, uint8_t> bitcheck(uint8_t pattern, uint8_t value);

void enableRawMode(void);
void disableRawMode(void);
char getch_nonblock(void);

#endif
