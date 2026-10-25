#include <linux/input.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main() {
  printf("Hello, client!\n");
  int fd;
  struct sockaddr_un server;
    strcpy(server.sun_path, "/run/user/1000/koboldkeysd.sock");
  server.sun_family = AF_UNIX;

  fd = socket(AF_UNIX, SOCK_STREAM, 0);

  int conn = connect(fd, (struct sockaddr *)&server, sizeof(server));
  if (conn < 0) {
    perror("Error connecting");
    return -1;
  }
  struct input_event ev;
  int len = 0;
  printf("Listening\n");
  while ((len = recv(fd, &ev, sizeof(ev), 0)) > 0) {
    struct timeval time;
    gettimeofday(&time, NULL);
    printf("Recieved %d bytes from socket at time: %lu\n", len, time.tv_usec);
    memset(&ev, 0, sizeof(ev));
  }

  close(fd);
  return 0;
}
