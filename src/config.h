#include <ini.h>

struct configuration{
  const char** events;
  int event_count;
  char* layout;
};


int get_config(struct configuration* out,char* override);
