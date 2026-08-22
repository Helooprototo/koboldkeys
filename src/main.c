
#include "config.h"
#include "window.h"
#include <stdio.h>

char *get_config_path() {
  char *path;
  if (getenv("XDG_CONFIG_HOME") == 0) {
    char *home = getenv("HOME");
    const char *def = "/.config/koboldkeys/";
    path = (char *)malloc(strlen(home) + strlen(def) + 1);
    if (path == NULL) {
      perror("Malloc failure");
      exit(1);
    }
    strcpy(path, home);
    strcat(path, def);
    return path;
  } else {
    char *xdg_config;
    xdg_config = getenv("XDG_CONFIG_HOME");
    path = (char *)malloc(strlen(xdg_config) + strlen("/koboldkeys/") + 1);
    if (path == NULL) {
      perror("Malloc failure");
      exit(1);
    }
    strcpy(path, xdg_config);
    strcat(path, "/koboldkeys/");
    return path;
  }
}

int main(int argc, char **argv) {
  printf("Hello, koboldkeys!\n");
  char* conf_path = NULL;
   for(int i=0;i<argc;i++){
    if(strcmp(argv[i],"-c")==0 || strcmp(argv[i],"--config")==0){
      fflush(stdout);
      if(i+1 < argc){
        if(argv[i+1][strlen(argv[i+1])-1]!='/'){
        conf_path = (char*)malloc(strlen(argv[i+1])+2);
        strcpy(conf_path,argv[i+1]);
        strcat(conf_path,"/");
        }else{
          conf_path = strdup(argv[i+1]);
        }
      }
    }else if(strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0){
      printf("Usage: koboldkeys [OPTIONS]\n\n");
      printf("OPTIONS: \n");
      printf("\t-c, --config \t Specify a configuration folder to use, instead of the default\n");
      printf("\t-h, --help \t Print this message and exit");
      exit(0);
    }
  }
  if(conf_path == NULL){
    conf_path = get_config_path();
  }
  struct Config *conf = config(conf_path);
  create_overlay(conf);

  return 0;
}