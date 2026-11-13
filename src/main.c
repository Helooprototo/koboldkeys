#include "socket.h"
#include <fcntl.h>
#include <linux/input.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "config.h"
#include <pthread.h>
#include <poll.h>
#include <xkbcommon/xkbcommon.h>
#include "protocol.h"
#include <errno.h>

#define MAX_CLIENTS 256
#define MAX_THREADS 256


int is_running = 1;

struct thread_data{
  const char* device;
  int* clients;
  int* client_count;
  int running;
  struct xkb_state* state;
};

struct thread{
    pthread_t thread;
    struct thread_data data;
};

struct overrides{
  char* config_path;
  struct socket_overrides socket_overrides;
};

int parse_args(struct overrides* out,int argc, char** argv){
  out->config_path = NULL;
  out->socket_overrides.path = NULL;
  out->socket_overrides.unlink_custom_path = 0;
  for(int i=0;i<argc;i++){
    if((strcmp(argv[i],"-c")==0 || strcmp(argv[i],"--config")==0) && i<argc+1){
      out->config_path = strdup(argv[i+1]);
    }else if((strcmp(argv[i],"-s")== 0 || strcmp(argv[i],"--socket")==0) && i<argc+1){
      out->socket_overrides.path = strdup(argv[i+1]);
    }else if((strcmp(argv[i],"--unlink-custom-socket-path")==0) && i<argc+1){
      out->socket_overrides.unlink_custom_path=1;
    }else if(strcmp(argv[i],"-h")== 0 || strcmp(argv[i],"--help")==0){
      printf("USAGE: koboldkeysd [args]\n");
      printf("\n");
      printf("\n");
      printf("ARGS:\n");
      printf("-s --socket [path] | Override the path where koboldkeysd creates its socket, will also have to be set for any used frontend\n");
      printf("-c --config [path] | Override the path to koboldkeysd conf.ini, default $XDG_CONFIG_HOME/koboldkeys/conf.ini or ~/.config/koboldkeys/conf.ini\n");
      printf("--unlink-custom-socket-path | Unlink an existing file at the given path of the socket, by default koboldkeysd only unlinks a file at the default socket path\n");
      return -1;
    }
  }
  return 0;
}

void free_overrides(struct overrides* overrides){
  free(overrides->socket_overrides.path);
  free(overrides->config_path);
}

char* keycode_to_keysym(int code,struct xkb_state* state){
  xkb_keycode_t keycode = code+8;
char key_name[64];
  xkb_keysym_t keysym = xkb_state_key_get_one_sym(state,keycode);
  xkb_keysym_get_name(keysym,key_name,sizeof(key_name));
  return strdup(key_name);
}

void* read_loop(void* c){
  struct thread_data* data = (struct thread_data*)c;
  
  const char* device = data->device;
  printf("Thread initialized with device: %s\n",device);
  int input_device = open(device,O_RDONLY);
  struct input_event ev;
  struct pollfd pfds;
  pfds.fd = input_device;
  pfds.events = POLLIN;
  while(data->running){
    int ready = poll(&pfds,1,1);
    if(ready <= 0){
      continue;
    }
      read(input_device,&ev,sizeof(ev));
      struct timeval time;
      gettimeofday(&time,NULL);
      printf("Sending input events to sockets at time: %lu\n",time.tv_usec);
      fflush(stdout);
      struct koboldkeysd_message msg;
      msg.event = ev;
      xkb_keysym_t ks = xkb_state_key_get_one_sym(data->state,ev.code+8);
      xkb_keysym_get_name(ks,msg.xkb_keysym,64);
      for(int i=0;i<*data->client_count;i++){
        int sent = send(data->clients[i],&msg,sizeof(msg),MSG_NOSIGNAL);
        if(sent < 0){
          perror("Error sending");
        }
      }
    }
  return 0;
}

void sigint(int sig) {
  is_running = 0;
  return;
}


int main(int argc, char** argv) {
  struct input_event ev;
  struct configuration config;
  struct overrides overrides;
  printf("Hello, world!\n");
  signal(SIGINT, sigint);
  if(parse_args(&overrides,argc,argv)<0){
    return 0;
  }
  if(get_config(&config,overrides.config_path)){
    printf("Error fetching config\n");
    return 1;
  }
  int len;
  int sock_fd;
  struct sockaddr_un sock;
  if(create_socket(&sock, &sock_fd,overrides.socket_overrides)<0){
    perror("Error creating socket");
    return 1;
  }
  if(listen(sock_fd,2)<0){
    perror("Error listening");
    return 1;
  }
  socklen_t addrlen = sizeof(sock);
  struct thread threads[MAX_THREADS-1];
  int clients[MAX_CLIENTS-1];
  int client_count = 0;
  printf("Using %d input devices\n",config.event_count);
  struct xkb_context* ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  struct xkb_rule_names names = {NULL,NULL,config.layout,NULL,NULL};
  struct xkb_keymap* keymap = xkb_keymap_new_from_names2(ctx,&names,XKB_KEYMAP_FORMAT_TEXT_V2, XKB_KEYMAP_COMPILE_NO_FLAGS);
  struct xkb_state* state = xkb_state_new(keymap);
  for(int i=0;i<config.event_count;i++){
    threads[i].data.device = config.events[i];
    threads[i].data.clients = &clients[0];
    threads[i].data.client_count = &client_count;
    threads[i].data.running = 1;
    threads[i].data.state = state;
    pthread_create(&threads[i].thread,NULL,read_loop,(void*)&threads[i].data);
  }
  while (is_running==1) {
 struct pollfd pfds;
        for(int i=0;i<client_count;i++){
         pfds.fd = clients[i];
         pfds.events = POLLIN;
         int incoming = poll(&pfds,1,1);
         if(incoming > 0){
            char msg[256];
            ssize_t n = recv(clients[i],&msg,sizeof(msg),0);
            if(n > 0){
              printf("Recieved message: %s with bytes: %lu \n",msg,n);
              fflush(stdout);
            }
           }
         }
        if(client_count < MAX_CLIENTS-1){
          clients[client_count] = accept(sock_fd,(struct sockaddr*)&sock,&addrlen);
          if(clients[client_count] != -1){
            client_count++;
        }else{
          sleep(1); // Dont stress out the cpu if we dont have a pending connection
        }
     }
   }
   printf("Joining threads\n");
   for(int i=0;i<config.event_count;i++){
     threads[i].data.running = 0;
     pthread_join(threads[i].thread,NULL);
     free((void*)config.events[i]);
   }
  printf("Cleaning\n");
  unlink(sock.sun_path);
  free(config.events);
  free_overrides(&overrides);
  return 0;
}
