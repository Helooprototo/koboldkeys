#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include <gtk/gtk.h>
#include <linux/input.h>
struct ButtonCoordinates {
  int x;
  int y;
  int width;
  int height;
};
struct ButtonConfig {
  const char *label;
  const char *case_label;
  int clicked_by; // atomic
  struct ButtonCoordinates coords;
  GtkWidget *button;
  size_t sym_count;
  char **syms;
};
struct ButtonClickUpdate {
  int set;
  GtkWidget *button;
  int flag;
};
struct ButtonLabelUpdate {
  const char *name;
  GtkWidget *button;
};
struct MouseMoveUpdate {
  int x;
  int y;
  GtkWidget *fixed;
  GtkWidget *mouse_widget;
};
struct XkbConfig {
  const char *layout;
  const char *variant;
  const char *options;
};
struct WindowConfig {
  int layer;
  int edge;
  int edge2;
  int layer_shell;
  int paintable;
  int mouse_padding;
};
struct DeviceConfig {
  size_t device_count;
  char **devices;
};
struct MouseInputConfig {
  const char *event;
  GtkWidget *mouse_widget;
  GtkWidget *fixed;
};
struct MouseConfig {
  struct DeviceConfig dev;
  struct MouseInputConfig input;
};
struct KeyboardInputConfig {
  size_t size;
  char *event;
  struct xkb_state *state;
  struct ButtonConfig *buttons[];
};
struct KeyboardConfig {
  struct DeviceConfig dev;
  struct KeyboardInputConfig input;
};
struct InputConfig {
  pthread_cond_t quit_cond;
  pthread_mutex_t mut;
  struct MouseConfig mouse;
  struct KeyboardConfig kbd;
};
struct Config {
  char *base_path;
  struct WindowConfig window;
  struct InputConfig input;
};

#endif