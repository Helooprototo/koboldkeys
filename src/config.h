#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "structs.h"
void *map_bool(int *out, void *data);
void *map_layer(int *out, void *layer);
void *map_edge(int *out, void *edge, int def);
char *get_config_path();
struct Config *config(int argc, char **argv);
#endif
