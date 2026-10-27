#include "ini.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


struct configuration{
  const char** events;
  int event_count;
};

static int config_handler(void* user, const char* section, const char* name, const char* value){
  struct configuration* pconfig = (struct configuration*)user;
  #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
  if(MATCH("config","events")){
    char* str = strdup(value);
    char *ch;
    int event_count = 0;
    ch = strtok(str,",");
    pconfig->events = malloc(sizeof(char*)*2);
    int cur_malloced = 2;
    while (ch != NULL){
      // Trim starting and ending spaces
      if(ch[0] == ' '){
        ch++;
      }if(ch[strlen(ch)]== ' '){
        ch[strlen(ch)-1] = '\0';
      } 
      printf("Got device: %s\n",ch);
      if(event_count > cur_malloced-1){
        pconfig->events = realloc(pconfig->events,cur_malloced*2*sizeof(char*));
        cur_malloced*=2;
      }
      if(cur_malloced >= 256){
        break; //Exit early if too many devices
      }
      pconfig->events[event_count] = strdup(ch);
      event_count++;
      
      ch = strtok(NULL,",");
    }
    if(event_count > 0){
      pconfig->events = realloc(pconfig->events,(sizeof(char*)*event_count)); //shrink back to required size
    }
    free(str);
    pconfig->event_count=event_count;
  }
  return 0;
}

char* get_default_config_path(){
     char* config_dir = getenv("XDG_CONFIG_HOME");
     if(config_dir == NULL){
        char* home = getenv("HOME");
        char* conf_home = malloc(strlen(home)+strlen("/.config/koboldkeys/conf.ini")+1);
        strcpy(conf_home,home);
        strcat(conf_home,"/.config/koboldkeys/conf.ini");
        config_dir = strdup(conf_home);
        free(conf_home);
     }else{
        char* kobold_conf = malloc(strlen(config_dir)+strlen("/koboldkeys/conf.ini")+1);
        strcpy(kobold_conf,config_dir);
        strcat(kobold_conf,"/koboldkeys/conf.ini");
        config_dir = strdup(kobold_conf);
        free(kobold_conf);
    }
    return config_dir;
}
char* get_config_path( char* override){
    char* out;
     if(override == NULL){
      out = get_default_config_path();
    }else{
      out = strdup(override);
    }
    return out;
}
int get_config(struct configuration* out,char* override){
    
    char* config = get_config_path(override);
    if(access(config,F_OK)!=0){
      printf("No configuration file found at %s! Please create one\n",config);
      free(config);
      return -1;
    }
    if(ini_parse(config,config_handler,out)<0){
    perror("Cant load config");
    return -1;
  }
  free(config);
  return 0;
}
