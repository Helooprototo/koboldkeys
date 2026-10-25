
#include <linux/limits.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <unistd.h>

int get_socket_path(char buff[108]){
    char* runtime_dir = getenv("XDG_RUNTIME_DIR");
    if(runtime_dir == NULL){
        perror("no XDG_RUNTIME_DIR found");
        return -1;
    }
    strncpy(buff,runtime_dir,108);
    strcat(buff,"/koboldkeysd.sock");
    return 0;
}

int create_socket(struct sockaddr_un* sock, int* sock_fd){
    char socket_dir[108];
    if(get_socket_path(socket_dir) < 0){
        perror("Error fetching socket path");
        return -1;
    };
    printf("Creating socket at: %s\n",socket_dir);
    *sock_fd = socket(AF_UNIX,SOCK_STREAM|SOCK_NONBLOCK,0);
    if(*sock_fd < 0){
        perror("Error creating socket");
        return 1;
    }
    memset(sock,0,sizeof(*sock));
    sock->sun_family = AF_UNIX;
    strncpy(sock->sun_path,socket_dir,108);
    if(bind(*sock_fd,(struct sockaddr*)sock,sizeof(*sock))<0){
        perror("Error binding to socket");
        return 1;
    }

    return 0;
}
