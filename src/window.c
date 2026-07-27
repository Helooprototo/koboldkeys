#include "input.h"
#include "structs.h"
#ifdef LAYER_SHELL
#include <gtk-layer-shell/gtk-layer-shell.h>
#endif
#include <gtk/gtk.h>
#include <linux/input.h>
#include <pthread.h>

static gboolean quit(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  struct InputConfig *conf = (struct InputConfig *)user_data;

  pthread_mutex_lock(&conf->mut);
  conf->should_quit = 1;
  pthread_cond_signal(&conf->quit_cond);
  pthread_mutex_unlock(&conf->mut);
  for(int i=0;i<conf->kbd.dev.device_count;i++){
    free(conf->kbd.dev.devices[i]);
  }
  if(conf->kbd.dev.device_count > 0){
    free(conf->kbd.dev.devices);
  }
  for(int i=0;i<conf->mouse.dev.device_count;i++){
    free(conf->mouse.dev.devices[i]);
  }
  if(conf->mouse.dev.device_count > 0){
    free(conf->mouse.dev.devices);
  }
  return FALSE; // let GTK proceed to destroy the window
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
  size_t malloc_size =
      sizeof(struct InputConfig) +
      conf->input.kbd.input.size * sizeof(struct ButtonConfig *);
  struct InputConfig *in = malloc(malloc_size);
  if (in == NULL) {
    perror("Malloc failure");
    exit(1);
  }
  memcpy(in, &conf->input, malloc_size);
  int size = in->kbd.input.size;
  GtkWidget *window;

  for (int i = 0; i < size; i++) {
    in->kbd.input.buttons[i]->button = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(in->kbd.input.buttons[i]->button),
                         in->kbd.input.buttons[i]->label);
    gtk_widget_set_name(in->kbd.input.buttons[i]->button, "unclicked");
  }
  GtkWidget *grid;
  GtkWidget *box;
  grid = gtk_grid_new();
  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  window = gtk_application_window_new(app);
#ifdef LAYER_SHELL
  if (conf->window.layer_shell) {
    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_layer(GTK_WINDOW(window), conf->window.layer);
    gtk_layer_set_anchor(GTK_WINDOW(window), conf->window.edge, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), conf->window.edge2, TRUE);
  }
#endif
  gtk_window_set_title(GTK_WINDOW(window), "KoboldKeys");
  // gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

  GtkCssProvider *css = gtk_css_provider_new();

  char *path = malloc(strlen(conf->base_path) + strlen("style.css") + 1);
  if (path == NULL) {
    perror("Malloc failure");
  }
  strcpy(path, conf->base_path);
  strcat(path, "style.css");
  gtk_css_provider_load_from_file(css, g_file_new_for_path(path), NULL);
  free(path);
  gtk_style_context_add_provider_for_screen(gtk_widget_get_screen(window),
                                            GTK_STYLE_PROVIDER(css),
                                            GTK_STYLE_PROVIDER_PRIORITY_USER);
  gtk_widget_set_app_paintable(window, conf->window.paintable);
  gtk_window_set_decorated(GTK_WINDOW(window), FALSE);

  gtk_container_add(GTK_CONTAINER(window), box);
  gtk_container_add(GTK_CONTAINER(box), grid);
  g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(quit), in);
  // GtkWidget* quit = gtk_button_new();
  // g_signal_connect(GTK_BUTTON(quit),"clicked",G_CALLBACK(destroy),app);
  // gtk_container_add(GTK_CONTAINER(box),quit);
  for (int i = 0; i < size; i++) {
    gtk_grid_attach(GTK_GRID(grid), in->kbd.input.buttons[i]->button,
                    in->kbd.input.buttons[i]->coords.x,
                    in->kbd.input.buttons[i]->coords.y,
                    in->kbd.input.buttons[i]->coords.width,
                    in->kbd.input.buttons[i]->coords.height);
  }
  pthread_t input_thread;
  pthread_create(&input_thread, NULL, input_loop, in);
  free(conf->base_path);
  free(conf);
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