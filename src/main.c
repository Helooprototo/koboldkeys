
#include "config.h"
#include "window.h"
#include <stdio.h>

int main(int argc, char **argv) {
  printf("Hello, koboldkeys!\n");
  struct Config *conf = config(argc, argv);
  create_overlay(conf);

  return 0;
}