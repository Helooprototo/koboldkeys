#include <sys/un.h>

struct socket_overrides{
  char* path;
  int unlink_custom_path;
};

int create_socket(struct sockaddr_un* sock, int* fd,struct socket_overrides overrides);
