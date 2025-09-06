#include <cstdlib>
#include "server.h"
#include "tblock.h"

TransBlock* tb_init() {
    TransBlock *tb = (TransBlock*)malloc(sizeof(TransBlock));

    tb->arg = (dstr**)calloc(TB_ARG_INIT_SIZE, sizeof(dstr*));
    tb->argcap = TB_ARG_INIT_SIZE;
    tb->argcnt = 0;

    tb->idxv = (int*)calloc(4, sizeof(int));
    tb->idxc = 0;

    tb->flags = 0;
    return tb;
}

uint8_t tb_insert(TransBlock *tb, std::vector<dstr*> &cmd) {
    size_t cmdsize = cmd.size();

    if (cmdsize + tb->argcnt > tb->argcap) {
        tb->arg = (dstr**)realloc(tb->arg, tb->argcap * 2);
        if (!tb->arg) {
            return 1;
        }
        tb->argcap *= 2; 
    }

    for (dstr *c: cmd) {
        int idx = tb->argcnt;
        tb->arg[idx] = c;
        tb->argcnt++;
    }

    return 0;
}
