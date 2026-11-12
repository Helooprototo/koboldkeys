#include <linux/input.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
struct koboldkeysd_message{
  struct input_event event;
  char xkb_keysym[64];
};


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
  struct koboldkeysd_message msg;
  int len = 0;
  printf("Listening\n");
  while ((len = recv(fd, &msg, sizeof(msg), 0)) > 0) {
    struct timeval time;
    gettimeofday(&time, NULL);
    printf("Recieved %d bytes from socket with sym %s and code %d at time: %lu\n", len,msg.xkb_keysym,msg.event.code,time.tv_usec);
    memset(&msg, 0, sizeof(msg));
  }

  close(fd);
  return 0;
}
