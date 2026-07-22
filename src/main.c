
#include "window.h"
#include "config.h"
#include <stdio.h>

int main(){
    printf("Hello, world!\n");
    struct Config* conf = config();
    create_overlay(conf);

    return 0;
}