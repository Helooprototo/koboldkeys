#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "structs.h"
void *map_bool(int *out, void *data);
void *map_layer(int *out, void *layer);
void *map_edge(int *out, void *edge, int def);
struct Config *config(char* xdg_config);
#endif
