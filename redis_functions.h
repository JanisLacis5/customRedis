#ifndef REDIS_FUNCTIONS_H
#define REDIS_FUNCTIONS_H

#include "hashmap.h"
#include "server.h"

void do_get(HMap *hmap, std::string &key, Conn *conn);
void do_set(HMap *hmap, Conn *conn, std::string &key, std::string &value);
void do_del(HMap *hmap, Conn *conn, std::string &key);
void do_keys(HMap *hmap, Conn *conn);

#endif
