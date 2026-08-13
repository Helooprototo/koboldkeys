#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include <gtk/gtk.h>
#include <linux/input.h>

struct ButtonRuntimeData {
  GtkWidget *widget;
  int clicked_by; // atomic
};
struct ButtonStaticData {
  char *name;
  char *css_class;
  struct ButtonCoordinates *coords;
};
struct ButtonCoordinates {
  int x;
  int y;
  int width;
  int height;
  int z;
};
struct ButtonConfig {
  struct ButtonRuntimeData runtime;
  struct ButtonStaticData st;
};
struct KeyboardButtonConfig {
  char *label;
  char *case_label;

  struct ButtonConfig conf;
  size_t sym_count;
  char **syms;
};
struct MouseButtonConfig {
  int key;
  struct ButtonConfig conf;
};
struct MouseCursorConfig {
  int should_show;
  struct ButtonCoordinates *coords;
  GtkWidget *widget;
};
struct ButtonClickUpdate {
  int set;
  GtkWidget *button;
  int flag;
};
struct ButtonScrollUpdate {
  int axis;
  GtkWidget *button;
};
struct ButtonScrollClearUpdate {
  unsigned int *g_source;
  GtkWidget *button;
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

struct InputLoopThread {
  pthread_t thread;
  pthread_cond_t quit_cond;
  pthread_mutex_t mut;
};
struct WindowConfig {
  GFileMonitor *watcher;
  GtkCssProvider *css_provider;
  int layer;
  int edge;
  int edge2;
  int layer_shell;
  int paintable;
  int mouse_padding;
  int layer_margin;
};
struct ThreadConfig {
  char *event;
  int is_running; // atomic
};
struct ThreadContainer{
  pthread_t thread;
  struct ThreadConfig* conf;
};
struct DeviceConfig {
  size_t device_count;
  char **devices;
};
struct MouseWheelConfig {
  int axis;
  struct ButtonConfig conf;
  unsigned int g_source;
};

struct MouseThreadConfig {
  int wheel_clear_timeout;
  struct MouseCursorConfig movement_widget;
  struct ThreadConfig thread;
  size_t wheel_size;
  struct MouseWheelConfig **wheels;
  size_t size;
  struct MouseButtonConfig **buttons;
};
struct MouseConfig {
  struct DeviceConfig dev;
  struct MouseThreadConfig input;
};
struct KeyboardThreadConfig {
  size_t size;
  struct ThreadConfig thread;
  struct xkb_state *state;
  struct KeyboardButtonConfig **buttons;
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