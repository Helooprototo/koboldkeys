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
  const char *sym;
  const char *label;
  const char *case_label;
  int clicked_by;
  int caps_state;
  struct ButtonCoordinates coords;
  GtkWidget *button;
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
};
struct MouseConfig {
  GtkWidget* mouse_widget;
  GtkWidget* fixed;
  const char *event;
};
struct KeyboardConfig {
  size_t size;
  const char *event;
  struct XkbConfig xkb;
  struct ButtonConfig buttons[];
};
struct InputConfig {
  struct MouseConfig mouse;
  struct KeyboardConfig kbd;
};
struct Config {
  struct WindowConfig window;
  struct InputConfig input;
};

#endif