#include <linux/input.h>
#include <xkbcommon/xkbcommon.h>

struct koboldkeysd_message{
  struct input_event event;
  char xkb_keysym[64];
};
