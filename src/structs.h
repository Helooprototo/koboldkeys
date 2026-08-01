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
  char *label;
  char *case_label;
  char *name;
  int clicked_by; // atomic
  struct ButtonCoordinates coords;
  GtkWidget *button;
  size_t sym_count;
  char **syms;
};
struct MouseButtonConfig {
  int key;
  int clicked_by; // atomic
  struct ButtonCoordinates coords;
  char *name;
  GtkWidget *button;
};
struct MouseCursorConfig {
  struct ButtonCoordinates coords;
  GtkWidget *widget;
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
  GtkWidget *mouse_widget;
};
struct XkbConfig {
  const char *layout;
  const char *variant;
  const char *options;
};

struct InputLoopThread{
  pthread_t thread;
  pthread_cond_t quit_cond;
  pthread_mutex_t mut;
};
struct WindowConfig {
  GFileMonitor* watcher;
  int layer;
  int edge;
  int edge2;
  int layer_shell;
  int paintable;
  int mouse_padding;
  int layer_margin;
};
struct KeyboardInputThreadContainer {
  pthread_t thread;
  struct KeyboardThreadConfig *thread_conf;
};
struct MouseInputThreadContainer {
  pthread_t thread;
  struct MouseThreadConfig *thread_conf;
};
struct DeviceConfig {
  size_t device_count;
  char **devices;
};
struct MouseThreadConfig {
  char *event;
  int is_running; // Atomic
  int show_cursor;
  size_t size;
  struct MouseCursorConfig movement_area;
  struct MouseCursorConfig movement_widget;
  struct MouseButtonConfig **buttons;
};
struct MouseConfig {
  struct DeviceConfig dev;
  struct MouseThreadConfig input;
};
struct KeyboardThreadConfig {
  size_t size;
  char *event;
  int is_running; // atomic
  struct xkb_state *state;
  struct ButtonConfig *buttons[];
};
struct KeyboardConfig {
  struct DeviceConfig dev;
  struct KeyboardThreadConfig input;
};
struct InputConfig {
  struct InputLoopThread input_thread;
  struct MouseConfig mouse;
  struct KeyboardConfig kbd;
};
struct Config {
  char *base_path;
  struct WindowConfig window;
  struct InputConfig input;
};

#endif