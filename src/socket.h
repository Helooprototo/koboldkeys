#include <sys/un.h>

int create_socket(struct sockaddr_un* sock, int* fd);

int get_socket_path(char buff[108]);
