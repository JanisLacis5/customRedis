#include "hyperloglog.h"

#include <string.h>

static bool verify_hll(std::string &hll) {
    return hll.size() == (HLL_DENSE_SIZE_BYTES + HLL_HEADER_SIZE_BYTES) && hll.substr(0, 4) == "HYLL";
}

static HLLHeader* new_hdr(uint8_t encoding) {
    HLLHeader *hdr = new HLLHeader();
    hdr->magic[0] = 'H';
    hdr->magic[1] = 'Y';
    hdr->magic[2] = 'L';
    hdr->magic[3] = 'L';
    hdr->enc = encoding;
    return hdr;
}

static void hll_init(std::string &hll) {
    // Create the header
    // TODO: use sparse type as well
    HLLHeader *hdr = new_hdr(HLL_DENSE);
    hll.assign(HLL_HEADER_SIZE_BYTES + HLL_DENSE_SIZE_BYTES, '\0');
    memcpy(&hll[0], hdr->magic, 4);
    memcpy(&hll[4], &hdr->enc, 1);
    memcpy(&hll[5], hdr->unused, 3);
    memcpy(&hll[8], hdr->card, 8);
    hll[15] |= 128; // Set the msb to tell that the cached value is not usable
    delete hdr;
}


uint8_t hll_add(std::string *hll, std::string &val) {

}

uint64_t hll_count(std::string *hll) {

}

void hll_merge(std::string *hll1, std::string *hll2) {

}