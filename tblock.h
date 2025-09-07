#ifndef TBLOCK_H
#define TBLOCK_H
#include "server.h"

#define TB_ARG_INIT_SIZE 64

void tb_init(TransBlock *tb);
uint8_t tb_insert(TransBlock *tb, std::vector<dstr*> &cmd);

#endif
