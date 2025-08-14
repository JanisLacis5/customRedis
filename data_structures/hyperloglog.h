#ifndef HYPERLOGLOG_H
#define HYPERLOGLOG_H
#include <stdint.h>
#include <string>

struct HyperLogLog {

};

uint8_t pf_add(HyperLogLog *hll, std::string &val);
uint64_t pf_count(HyperLogLog *hll);
void pf_merge(HyperLogLog *hll1, HyperLogLog *hll2);


#endif
