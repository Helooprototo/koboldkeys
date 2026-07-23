#include "config.h"
#include "input.h"
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <gtk/gtk.h>
#include <linux/input.h>
#include <pthread.h>

void destroy(GtkWidget *widget, gpointer data) {
  g_application_quit(G_APPLICATION(data));
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
  size_t malloc_size = sizeof(struct InputConfig)+conf->input.kbd.size*sizeof(struct ButtonConfig);
  struct InputConfig* in = malloc(malloc_size);
  memcpy(in,&conf->input,malloc_size);
  int size = in->kbd.size;
  GtkWidget *window;

  for (int i = 0; i < size; i++) {
    in->kbd.buttons[i].button = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(in->kbd.buttons[i].button),
                         in->kbd.buttons[i].label);
    gtk_widget_set_name(in->kbd.buttons[i].button, "unclicked");
  }
  GtkWidget *grid;
  GtkWidget *box;
  grid = gtk_grid_new();
  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  window = gtk_application_window_new(app);
  if (conf->window.layer_shell) {
    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_layer(GTK_WINDOW(window), conf->window.layer);
    gtk_layer_set_anchor(GTK_WINDOW(window), conf->window.edge, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), conf->window.edge2, TRUE);
  }
  gtk_window_set_title(GTK_WINDOW(window), "KoboldKeys");
  // gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

  GtkCssProvider *css = gtk_css_provider_new();

  char *xdg_config = get_config_path();
  char *path = malloc(strlen(xdg_config) + strlen("style.css") + 1);
  strcpy(path, xdg_config);
  free(xdg_config);
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
  // GtkWidget* quit = gtk_button_new();
  // g_signal_connect(GTK_BUTTON(quit),"clicked",G_CALLBACK(destroy),app);
  // gtk_container_add(GTK_CONTAINER(box),quit);
  for (int i = 0; i < size; i++) {
    gtk_grid_attach(GTK_GRID(grid), in->kbd.buttons[i].button,
                    in->kbd.buttons[i].coords.x, in->kbd.buttons[i].coords.y,
                    in->kbd.buttons[i].coords.width,
                    in->kbd.buttons[i].coords.height);
  }
  pthread_t input_thread;
  pthread_create(&input_thread, NULL, input_loop, in);
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