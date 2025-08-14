#ifndef HYPERLOGLOG_H
#define HYPERLOGLOG_H
#include <stdint.h>
#include <string>

const int32_t HLL_SIZE_BITS = 16 * 8 + 6 * 16384;

uint8_t pf_add(std::string &hll, std::string &val);
uint64_t pf_count(std::string &hll);
void pf_merge(std::string &hll1, std::string &hll2);


#endif
