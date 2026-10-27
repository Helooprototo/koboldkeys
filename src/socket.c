
#include <linux/limits.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <unistd.h>

struct socket_overrides{
  char* path;
  int unlink_custom_path;
};


int get_socket_path(char out[108],char* override){
    if(override == NULL){
    char* runtime_dir = getenv("XDG_RUNTIME_DIR");
    if(runtime_dir == NULL){
        perror("no XDG_RUNTIME_DIR found");
        return -1;
    }
    strncpy(out,runtime_dir,108);
    strcat(out,"/koboldkeysd.sock");
    }else{
        strncpy(out,override,108);
    }
    return 0;
}

int create_socket(struct sockaddr_un* sock, int* sock_fd,struct socket_overrides overrides){
    char socket_dir[108];
    if(get_socket_path(socket_dir,overrides.path) < 0){
        perror("Error fetching socket path");
        return -1;
    };
    printf("Creating socket at: %s\n",socket_dir);
    *sock_fd = socket(AF_UNIX,SOCK_STREAM|SOCK_NONBLOCK,0);
    if(*sock_fd < 0){
        perror("Error creating socket");
        return -1;
    }
    memset(sock,0,sizeof(*sock));
    sock->sun_family = AF_UNIX;
    strncpy(sock->sun_path,socket_dir,108);
    if(access(sock->sun_path,F_OK)==0 && (overrides.path == NULL || overrides.unlink_custom_path)){ // Only delete existing socket if it isnt a custom path, to avoid deleting files the user wants to keep, UNLESS specifically allowed with the cmdline flag
        printf("Socket at %s already exists, unlinking...\n",sock->sun_path);
        unlink(sock->sun_path);
    }
    if(bind(*sock_fd,(struct sockaddr*)sock,sizeof(*sock))<0){
        perror("Error binding to socket");
        return -1;
    }

    return 0;
}
