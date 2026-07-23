#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "structs.h"
void* map_layer(int* out,void* layer);
void* map_edge(int* out,void* edge1);
char* get_config_path();
struct Config* config();
#endif
