#include "structs.h"
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <iostream>
#include <malloc.h>
#include <sys/stat.h>
#include <toml++/toml.hpp>
#include <unistd.h>

#define MAX_SYM_LENGTH 256 // should be more than enough

void map_bool(int *out, toml::node_view<toml::node> data) {
  if (data.is_string()) {
    char *str = strdup(data.value_or("true"));
    *out = (strcasecmp(str, "true") == 0) ? 1 : 0;
    free(str);
  } else if (data.is_integer()) {
    *out = data.value_or(1);
  } else if (data.is_boolean()) {
    *out = data.value<bool>().value_or(1);
  } else {
    *out = 1;
  }
}
void map_edge(int *out, toml::node_view<toml::node> edge) {
  if (edge.is_string()) {
    char *str = strdup(edge.value_or("bottom"));
    if (strcasecmp(str, "LEFT") == 0) {
      *out = GTK_LAYER_SHELL_EDGE_LEFT;
    } else if (strcasecmp(str, "RIGHT") == 0) {
      *out = GTK_LAYER_SHELL_EDGE_RIGHT;
    } else if (strcasecmp(str, "TOP") == 0) {
      *out = GTK_LAYER_SHELL_EDGE_TOP;
    } else if (strcasecmp(str, "BOTTOM") == 0) {
      *out = GTK_LAYER_SHELL_EDGE_BOTTOM;
    }
    free(str);
  } else if (edge.is_integer()) {
    *out = edge.value_or(GTK_LAYER_SHELL_EDGE_BOTTOM);
  } else {
    *out = GTK_LAYER_SHELL_EDGE_BOTTOM;
  }
}
void map_layer(int *out, toml::node_view<toml::node> layer) {
  if (layer.is_string()) {
    char *str = strdup(layer.value_or("overlay"));
    if (strcasecmp(str, "BACKGROUND") == 0) {
      *out = GTK_LAYER_SHELL_LAYER_BACKGROUND;
    } else if (strcasecmp(str, "BOTTOM") == 0) {
      *out = GTK_LAYER_SHELL_LAYER_BOTTOM;
    } else if (strcasecmp(str, "TOP") == 0) {
      *out = GTK_LAYER_SHELL_LAYER_TOP;
    } else if (strcasecmp(str, "OVERLAY") == 0) {
      *out = GTK_LAYER_SHELL_LAYER_OVERLAY;
    }
  } else if (layer.is_integer()) {
    *out = layer.value_or(GTK_LAYER_SHELL_LAYER_OVERLAY);
  } else {
    *out = GTK_LAYER_SHELL_LAYER_OVERLAY;
  }
}
extern "C" char *get_config_path();
char *get_config_path() {
  char *path;
  if (getenv("XDG_CONFIG_HOME") == 0) {
    std::cout.flush();
    char *home = getenv("HOME");
    char *def = strdup("/.config/koboldkeys/");
    path = (char *)malloc(strlen(home) + strlen(def) + 1);
    strcpy(path, home);
    strcat(path, def);
    free(def);
  } else {
    char *xdg_config;
    xdg_config = strdup(getenv("XDG_CONFIG_HOME"));
    path = (char *)malloc(strlen(xdg_config) + strlen("/koboldkeys/") + 1);
    strcpy(path, xdg_config);
    free(xdg_config);
    strcat(path, "/koboldkeys/");
  }
  return path;
}
extern "C" struct Config *config();
struct Config *config() {
  struct Config *config;
  char *xdg_config = get_config_path();
  char *path = (char *)malloc(strlen(xdg_config) + strlen("conf.toml") + 1);
  strcpy(path, xdg_config);
  free(xdg_config);
  strcat(path, "conf.toml");
  std::cout << "Using config path: " << path << std::endl;
  struct stat st;
  if (stat(path, &st) == -1) {
    printf("Please create your config file\n");
    exit(1);
  }
  auto toml = toml::parse_file(path);
  free(path);
  if (!toml["button"].is_table() || !toml["input"].is_table()) {
    perror("Could not find the necessary config structure");
    exit(1);
  }
  size_t size = toml["button"].as_table()->size();
  config = (Config *)malloc(sizeof(struct Config) +
                            size * sizeof(struct ButtonConfig));
  config->input.kbd.size = size;
  int index = 0;

  config->input.mouse.event = strdup(toml["input"]["mouse"].value_or(""));
  config->input.kbd.event = strdup(toml["input"]["keyboard"].value_or(""));
  config->input.kbd.xkb.layout = strdup(toml["xkb"]["layout"].value_or(""));
  config->input.kbd.xkb.variant = strdup(toml["xkb"]["variant"].value_or(""));
  config->input.kbd.xkb.options = strdup(toml["xkb"]["options"].value_or(""));
  map_edge(&config->window.edge, toml["window"]["edge"]);
  map_edge(&config->window.edge2, toml["window"]["edge2"]);
  map_layer(&config->window.layer, toml["window"]["layer"]);
  map_bool(&config->window.layer_shell, toml["window"]["is-layer-shell"]);
  map_bool(&config->window.paintable, toml["window"]["transparent"]);
  toml["button"].as_table()->for_each([config, &index,
                                       size](auto &key, toml::table &value) {
    if (value["sym"].is_array()) {
      size_t sym_count = value["sym"].as_array()->size();
      config->input.kbd.buttons[index].sym_count = sym_count;
      config->input.kbd.buttons[index].syms = (char **)malloc(sym_count * sizeof(char *));
      size_t sym_i = 0;
      for (auto &&sym : *value["sym"].as_array()) {
          config->input.kbd.buttons[index].syms[sym_i] = strdup(sym.value_or(""));
          sym_i+=1;
        
      };
    } else if(value["sym"].is_string()){
      config->input.kbd.buttons[index].sym_count = 1;
      config->input.kbd.buttons[index].syms = (char **)malloc(1 * sizeof(char *));
      config->input.kbd.buttons[index].syms[0] = strdup(value["sym"].value_or(""));
    }

    config->input.kbd.buttons[index].label =
        strdup(value["label"].value_or(value["sym"].value_or("")));
    config->input.kbd.buttons[index].case_label = strdup(
        value["case-label"].value_or(config->input.kbd.buttons[index].label));
    config->input.kbd.buttons[index].clicked_by = 0;
    config->input.kbd.buttons[index].caps_state = FALSE;
    config->input.kbd.buttons[index].coords.x = value["x"].value_or(0);
    config->input.kbd.buttons[index].coords.y = value["y"].value_or(0) * -1;
    config->input.kbd.buttons[index].coords.width = value["width"].value_or(1);
    config->input.kbd.buttons[index].coords.height =
        value["height"].value_or(1);
    index += 1;
  });
  return config;
}