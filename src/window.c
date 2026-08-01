#include "input.h"
#include "structs.h"
#include <poll.h>
#include <sys/inotify.h>
#include <xkbcommon/xkbcommon.h>
#ifdef LAYER_SHELL
#include <gtk-layer-shell/gtk-layer-shell.h>
#endif
#include <gtk/gtk.h>
#include <linux/input.h>
#include <pthread.h>
#include <stdatomic.h>
static gboolean quit(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  struct Config *conf = (struct Config *)user_data;
  struct InputConfig *in = &conf->input;
  pthread_mutex_lock(&in->input_thread.mut);
  pthread_cond_signal(&in->input_thread.quit_cond);
  pthread_mutex_unlock(&in->input_thread.mut);
  fflush(stdout);
  pthread_join(in->input_thread.thread, NULL);
  xkb_state_unref(in->kbd.input.state);
  for (int i = 0; i < in->kbd.dev.device_count; i++) {
    free(in->kbd.dev.devices[i]);
  }
  if (in->kbd.dev.device_count > 0) {
    free(in->kbd.dev.devices);
  }
  for (int i = 0; i < in->mouse.dev.device_count; i++) {
    free(in->mouse.dev.devices[i]);
  }
  if (in->mouse.dev.device_count > 0) {
    free(in->mouse.dev.devices);
  }
  g_object_unref(conf->window.watcher);
  free(conf->base_path);
  free(conf);
  return FALSE;
}

static void css_watcher(GFileMonitor *monitor, GFile *file, GFile *other_file,
                        GFileMonitorEvent event_type, gpointer user_data) {
  GtkCssProvider *css_provider = (GtkCssProvider *)user_data;
  gchar *path = g_file_get_path(file);
  if (event_type == G_FILE_MONITOR_EVENT_CHANGED) {
    gtk_css_provider_load_from_path(css_provider, path, NULL);
  }
}

gboolean mouse_move_update(void *data) {
  struct MouseMoveUpdate *update = (struct MouseMoveUpdate *)data;
  GtkWidget *fixed = gtk_widget_get_parent(update->mouse_widget);
  gtk_fixed_move(GTK_FIXED(fixed), update->mouse_widget, update->x, update->y);
  g_free(update);
  return G_SOURCE_REMOVE;
}

gboolean button_label_update(void *data) {
  struct ButtonLabelUpdate *update = (struct ButtonLabelUpdate *)data;
  gtk_button_set_label(GTK_BUTTON(update->button), update->name);
  g_free(update);
  return G_SOURCE_REMOVE;
}

gboolean button_click_update(void *data) {
  struct ButtonClickUpdate *update = (struct ButtonClickUpdate *)data;
  if (update->set) {
    gtk_widget_set_state_flags(update->button, update->flag, FALSE);
  } else {
    gtk_widget_unset_state_flags(update->button, update->flag);
  }
  g_free(update);
  return G_SOURCE_REMOVE;
}

static void activate(GtkApplication *app, gpointer user_data) {
  struct Config *conf = (struct Config *)user_data;
  struct InputConfig *in = &conf->input;
  int kbd_size = in->kbd.input.size;
  int mouse_size = in->mouse.input.size;
  GtkWidget *window;
  for (int i = 0; i < mouse_size; i++) {
    in->mouse.input.buttons[i]->button = gtk_button_new();
    gtk_widget_set_name(in->mouse.input.buttons[i]->button,
                        in->mouse.input.buttons[i]->name);
  }
  for (int i = 0; i < kbd_size; i++) {
    in->kbd.input.buttons[i]->button = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(in->kbd.input.buttons[i]->button),
                         in->kbd.input.buttons[i]->label);
    gtk_widget_set_size_request(in->kbd.input.buttons[i]->button,
                                in->kbd.input.buttons[i]->coords.width,
                                in->kbd.input.buttons[i]->coords.height);
    gtk_widget_set_name(in->kbd.input.buttons[i]->button,
                        in->kbd.input.buttons[i]->name);
  }
  GtkWidget *grid;
  GtkWidget *box;
  GtkWidget *fixed;
  fixed = gtk_fixed_new();
  grid = gtk_grid_new();
  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_set_spacing(GTK_BOX(box), conf->window.mouse_padding);
  window = gtk_application_window_new(app);
#ifdef LAYER_SHELL
  if (conf->window.layer_shell) {
    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_layer(GTK_WINDOW(window), conf->window.layer);
    gtk_layer_set_anchor(GTK_WINDOW(window), conf->window.edge, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), conf->window.edge2, TRUE);
    gtk_layer_set_margin(GTK_WINDOW(window), conf->window.edge2,
                         conf->window.layer_margin);
  }
#endif
  gtk_window_set_title(GTK_WINDOW(window), "KoboldKeys");
  // gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
  gtk_widget_set_app_paintable(window, conf->window.paintable);
  gtk_window_set_decorated(GTK_WINDOW(window), FALSE);

  gtk_container_add(GTK_CONTAINER(window), box);

  if (in->kbd.dev.device_count > 0) {
    gtk_container_add(GTK_CONTAINER(box), grid);
    for (int i = 0; i < kbd_size; i++) {
      GtkStyleContext *cntx =
          gtk_widget_get_style_context(in->kbd.input.buttons[i]->button);
      gtk_style_context_add_class(cntx, "keyboardbutton");
      gtk_grid_attach(GTK_GRID(grid), in->kbd.input.buttons[i]->button,
                      in->kbd.input.buttons[i]->coords.x,
                      in->kbd.input.buttons[i]->coords.y,
                      in->kbd.input.buttons[i]->coords.width,
                      in->kbd.input.buttons[i]->coords.height);
    }
  }
  if (in->mouse.dev.device_count > 0) {
    gtk_container_add(GTK_CONTAINER(box), fixed);
    for (int i = 0; i < mouse_size; i++) {
      gtk_widget_set_size_request(in->mouse.input.buttons[i]->button,
                                  in->mouse.input.buttons[i]->coords.width,
                                  in->mouse.input.buttons[i]->coords.height);
      GtkStyleContext *cntx =
          gtk_widget_get_style_context(in->mouse.input.buttons[i]->button);
      gtk_style_context_add_class(cntx, "mousebutton");
      gtk_fixed_put(GTK_FIXED(fixed), in->mouse.input.buttons[i]->button,
                    in->mouse.input.buttons[i]->coords.x,
                    in->mouse.input.buttons[i]->coords.y);
    }
    if (in->mouse.input.show_cursor) {
      struct MouseCursorConfig *cursor = &in->mouse.input.movement_widget;
      struct MouseCursorConfig *area = &in->mouse.input.movement_area;
      cursor->widget = gtk_button_new();
      area->widget = gtk_fixed_new();
      gtk_widget_set_name(cursor->widget, "cursor");
      gtk_widget_set_size_request(cursor->widget, cursor->coords.width,
                                  cursor->coords.height);
      gtk_widget_set_size_request(area->widget, area->coords.width,
                                  area->coords.height);
      GtkStyleContext *cntx = gtk_widget_get_style_context(cursor->widget);
      gtk_style_context_add_class(cntx, "mousebutton");

      gtk_fixed_put(GTK_FIXED(fixed), in->mouse.input.movement_area.widget,
                    in->mouse.input.movement_area.coords.x,
                    in->mouse.input.movement_area.coords.y);
      gtk_fixed_put(GTK_FIXED(in->mouse.input.movement_area.widget),
                    cursor->widget, in->mouse.input.movement_area.coords.x,
                    in->mouse.input.movement_area.coords.y);
    }
  }
  char *path =
      malloc(strlen(conf->base_path) + strlen("style.css") * sizeof(char)+1);
  strcpy(path, conf->base_path);
  strcat(path, "style.css");
  GtkCssProvider *css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(css_provider, path, NULL);
  GFile *css = g_file_new_for_path(path);
  conf->window.watcher = g_file_monitor(css, G_FILE_MONITOR_NONE, NULL, NULL);
  gtk_style_context_add_provider_for_screen(gtk_widget_get_screen(window),
                                            GTK_STYLE_PROVIDER(css_provider),
                                            GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_signal_connect(conf->window.watcher, "changed", G_CALLBACK(css_watcher), css_provider);
  g_object_unref(css);
  free(path);
  pthread_create(&in->input_thread.thread, NULL, input_loop, in);
  g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(quit), conf);
  gtk_widget_show_all(window);
}

void create_overlay(struct Config *b) {
  GtkApplication *app;
  int status;

  app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), b);
  status = g_application_run(G_APPLICATION(app), 0, NULL);
  g_object_unref(app);
}