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
  pthread_mutex_lock(&in->mut);
  pthread_cond_signal(&in->quit_cond);
  pthread_mutex_unlock(&in->mut);
  fflush(stdout);
  atomic_store((_Atomic int *)&conf->window.css_watcher_thread.is_running,
               FALSE);
  pthread_join(conf->window.css_watcher_thread.thread, NULL);
  pthread_join(in->input_thread, NULL);
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
  free(conf->base_path);
  free(conf);
  return FALSE;
}

void *css_watcher(void *data) {
  struct CssWatcherThread *conf = (struct CssWatcherThread *)data;
  int event_size = sizeof(struct inotify_event);
  int buffer_len = 1024 * (event_size + 16);
  char buf[buffer_len];
  int wd;

  char *path = malloc(strlen(conf->dir_path) + strlen("style.css") + 1);
  if (path == NULL) {
    perror("Malloc failure");
  }
  strcpy(path, conf->dir_path);
  strcat(path, "style.css");
  gtk_css_provider_load_from_file(conf->css, g_file_new_for_path(path), NULL);

  int inotify_fd = inotify_init();
  if (inotify_fd < 0) {
    perror("Inotify init failed");
  }

  wd = inotify_add_watch(inotify_fd, conf->dir_path,
                         IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE);
  if (wd < 0) {
    perror("Inotify watch failed");
  }

  while (atomic_load((_Atomic int *)&conf->is_running)) {
    struct pollfd fds;
    fds.fd = inotify_fd;
    fds.events = POLLIN;
    int ret = poll(&fds, 1, 10);
    if (ret > 0 && (fds.revents & POLLIN)) {
      ssize_t bytes_read = read(inotify_fd, buf, buffer_len);
      if (bytes_read <= 0) {
        perror("watcher read failure");
        continue;
      }

      for (char *ptr = buf; ptr < buf + bytes_read;) {
        struct inotify_event *event = (struct inotify_event *)ptr;

        if (event->len > 0 && strcmp(event->name, conf->filename) == 0) {
          if (event->mask & (IN_CLOSE_WRITE | IN_MOVED_TO)) {
            printf("style.css changed, reloading\n");
            gtk_css_provider_load_from_file(conf->css,
                                            g_file_new_for_path(path), NULL);
          }
        }

        ptr += event_size + event->len;
      }
    }
  }
  free(path);
  free(conf->dir_path);
  return 0;
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

  struct CssWatcherThread *css_watcher_data = &conf->window.css_watcher_thread;
  css_watcher_data->filename = "style.css";

  css_watcher_data->css = gtk_css_provider_new();
  css_watcher_data->dir_path = strdup(conf->base_path);
  pthread_create(&css_watcher_data->thread, NULL, css_watcher,
                 css_watcher_data);
  gtk_style_context_add_provider_for_screen(
      gtk_widget_get_screen(window), GTK_STYLE_PROVIDER(css_watcher_data->css),
      GTK_STYLE_PROVIDER_PRIORITY_USER);
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
  pthread_create(&in->input_thread, NULL, input_loop, in);
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