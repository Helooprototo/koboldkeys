#include "structs.h"
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <iostream>
#include <malloc.h>
#include <sys/stat.h>
#include <toml++/toml.hpp>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

#define MAX_SYM_LENGTH 256 // should be more than enough
xkb_state *create_xkb(toml::node_view<toml::node> data) {
  const char *layout = data["layout"].value_or("");
  const char *variant = data["variant"].value_or("");
  const char *options = data["options"].value_or("");
  struct xkb_context *kbd_ctx;
  kbd_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!kbd_ctx) {
    perror("Could not create keyboard context");
  }
  struct xkb_rule_names names = {NULL, NULL, layout, variant, options};
  struct xkb_keymap *keymap;
  keymap = xkb_keymap_new_from_names2(
      kbd_ctx, &names, XKB_KEYMAP_FORMAT_TEXT_V2, XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!keymap) {
    perror("Could not create keymap");
  }
  return xkb_state_new(keymap);
}
char *check_device(toml::node_view<toml::node> data) {
  struct stat st;
  st.st_dev = 0;
  if (data.is_string()) {
    stat(data.value_or(""), &st);
    if (st.st_dev != 7) {
      perror("File is probably not an input device!");
      std::cout << "Ignoring device: " << data.value_or("") << std::endl;
      return (char *)NULL;
    } else {
      return strdup(data.value_or(""));
    }
  } else {
    return strdup("");
  }
}
void map_devices(DeviceConfig *config, toml::node_view<toml::node> data) {
  config->device_count = 0;
  if (data.is_array()) {
    int device_index = 0;
    config->devices = (char **)malloc(1 * sizeof(char *));
    if (config->devices == NULL) {
      perror("Malloc failure");
      exit(1);
    }
    for (auto &&device : *data.as_array()) {
      char *dev = check_device(toml::node_view<toml::node>(device));
      if (dev != NULL) {
        config->devices[device_index] = strdup(dev);
        device_index += 1;
        config->device_count = device_index;
        config->devices = (char **)realloc(config->devices,
                                           (device_index + 1) * sizeof(char *));
        if (config->devices == NULL) {
          perror("Malloc failure");
          exit(1);
        }
        free(dev);
      }
    }
  } else if (data.is_string()) {
    char *dev = check_device(data);
    if (dev != NULL) {
      config->device_count = 1;
      config->devices = (char **)malloc(1 * sizeof(char *));
      config->devices[0] = strdup(data.value_or(""));
    } else {
      config->device_count = 0;
    }
    free(dev);
  } else {
    config->device_count = 0;
  }
}
int map_bool(toml::node_view<toml::node> data) {
  if (data.is_string()) {
    const char *str = data.value_or("true");
    return (strcasecmp(str, "true") == 0) ? 1 : 0;
  } else if (data.is_integer()) {
    return data.value_or(1);
  } else if (data.is_boolean()) {
    return data.value<bool>().value_or(1);
  } else {
    return 1;
  }
}
int map_edge(toml::node_view<toml::node> edge, int def) {
  if (edge.is_string()) {
    const char *str = edge.value_or("");
    if (strcasecmp(str, "LEFT") == 0) {
      return GTK_LAYER_SHELL_EDGE_LEFT;
    } else if (strcasecmp(str, "RIGHT") == 0) {
      return GTK_LAYER_SHELL_EDGE_RIGHT;
    } else if (strcasecmp(str, "TOP") == 0) {
      return GTK_LAYER_SHELL_EDGE_TOP;
    } else if (strcasecmp(str, "BOTTOM") == 0) {
      return GTK_LAYER_SHELL_EDGE_BOTTOM;
    } else {
      return def;
    }
  } else if (edge.is_integer()) {
    return edge.value_or(def);
  } else {
    return def;
  }
}

struct ButtonCoordinates map_coords(toml::node_view<toml::node> data) {
  struct ButtonCoordinates x;
  x.height = data["height"].value_or(1);
  x.width = data["width"].value_or(1);
  x.x = data["x"].value_or(0);
  x.y = data["y"].value_or(0)*-1;
  return x;
}
int map_layer(toml::node_view<toml::node> layer) {
  if (layer.is_string()) {
    const char *str = layer.value_or("overlay");
    if (strcasecmp(str, "BACKGROUND") == 0) {
      return GTK_LAYER_SHELL_LAYER_BACKGROUND;
    } else if (strcasecmp(str, "BOTTOM") == 0) {
      return GTK_LAYER_SHELL_LAYER_BOTTOM;
    } else if (strcasecmp(str, "TOP") == 0) {
      return GTK_LAYER_SHELL_LAYER_TOP;
    } else if (strcasecmp(str, "OVERLAY") == 0) {
      return GTK_LAYER_SHELL_LAYER_OVERLAY;
    } else {
      return GTK_LAYER_SHELL_LAYER_OVERLAY;
    }
  } else if (layer.is_integer()) {
    return layer.value_or(GTK_LAYER_SHELL_LAYER_OVERLAY);
  } else {
    return GTK_LAYER_SHELL_LAYER_OVERLAY;
  }
}
extern "C" char *get_config_path(int argc, char **argv);
char *get_config_path(int argc, char **argv) {
  char *path;
  int opt;
  while ((opt = getopt(argc, argv, "c:")) != -1) {
    switch (opt) {
    case 'c':
      std::cout << "config path set through opt: " << optarg << std::endl;
      if (optarg[strlen(optarg) - 1] != '/') {
        // Insert trailing slash if not gi
        path = (char *)malloc(strlen(optarg) + 2);
        strcpy(path, optarg);
        strcat(path, "/");
      } else {
        path = strdup(optarg);
      }
      return path;
      break;
    }
  }
  if (getenv("XDG_CONFIG_HOME") == 0) {
    char *home = getenv("HOME");
    const char *def = "/.config/koboldkeys/";
    path = (char *)malloc(strlen(home) + strlen(def) + 1);
    if (path == NULL) {
      perror("Malloc failure");
      exit(1);
    }
    strcpy(path, home);
    strcat(path, def);
    return path;
  } else {
    char *xdg_config;
    xdg_config = getenv("XDG_CONFIG_HOME");
    path = (char *)malloc(strlen(xdg_config) + strlen("/koboldkeys/") + 1);
    if (path == NULL) {
      perror("Malloc failure");
      exit(1);
    }
    strcpy(path, xdg_config);
    strcat(path, "/koboldkeys/");
    return path;
  }
}
extern "C" struct Config *config(int argc, char **argv);
struct Config *config(int argc, char **argv) {
  struct Config *config;
  char *xdg_config = get_config_path(argc, argv);
  char *path = (char *)malloc(strlen(xdg_config) + strlen("conf.toml") + 1);
  if (path == NULL) {
    perror("Malloc failure");
    exit(1);
  }
  strcpy(path, xdg_config);

  strcat(path, "conf.toml");
  std::cout << "Using config path: " << path << std::endl;
  struct stat st;
  if (stat(path, &st) == -1) {
    perror("Please create your config file\n");
    exit(1);
  }
  auto toml = toml::parse_file(path);
  free(path);
  if (!toml["button"].is_table() || !toml["input"].is_table() ||
      !toml["mousebutton"].is_table()) {
    perror("Could not find the necessary config structure");
    exit(1);
  }
  size_t size = toml["button"].as_table()->size();
  size_t mouse_size = toml["mousebutton"].as_table()->size();
  config = (Config *)malloc(sizeof(struct Config) +
                            size * sizeof(struct ButtonConfig *));
  if (config == NULL) {
    perror("Malloc failure");
    exit(1);
  }
  config->base_path = strdup(xdg_config);
  free(xdg_config);
  config->input.kbd.input.size = size;
  map_devices(&config->input.mouse.dev, toml["input"]["mouse"]);
  map_devices(&config->input.kbd.dev, toml["input"]["keyboard"]);
  config->input.kbd.input.state = create_xkb(toml["xkb"]);
  config->window.edge =
      map_edge(toml["window"]["anchors"][0], GTK_LAYER_SHELL_EDGE_BOTTOM);
  config->window.edge2 =
      map_edge(toml["window"]["anchors"][1], GTK_LAYER_SHELL_EDGE_LEFT);
  config->window.layer = map_layer(toml["window"]["layer"]);
  config->window.layer_shell = map_bool(toml["window"]["is-layer-shell"]);
  config->window.paintable = map_bool(toml["window"]["transparent"]);
  config->input.mouse.input.buttons = (struct MouseButtonConfig **)malloc(
      mouse_size * sizeof(struct MouseButtonConfig *));
  config->input.mouse.input.size = mouse_size;
  config->input.mouse.input.show_cursor =
      map_bool(toml["window"]["show-cursor"]);
  config->input.mouse.input.movement_area.coords =
      map_coords(toml["mouse"]["area"]);
  config->input.mouse.input.movement_widget.coords =
      map_coords(toml["mouse"]["cursor"]);
  int btn_index = 0;
  toml["mousebutton"].as_table()->for_each([&btn_index, config](
                                               auto &key, toml::table &value) {
    config->input.mouse.input.buttons[btn_index] =
        (struct MouseButtonConfig *)malloc(sizeof(struct MouseButtonConfig));
    struct MouseButtonConfig *button =
        config->input.mouse.input.buttons[btn_index];
    button->coords = map_coords(toml::node_view<toml::node>(value));
    button->key = value["code"].value_or(0);
    button->clicked_by = 0;
    button->name = strdup(std::string(key).c_str());
    btn_index += 1;
  });
  btn_index = 0;
  toml["button"].as_table()->for_each([config, &btn_index,
                                       size](auto &key, toml::table &value) {
    config->input.kbd.input.buttons[btn_index] =
        (struct ButtonConfig *)malloc(sizeof(struct ButtonConfig));
    struct ButtonConfig *button = config->input.kbd.input.buttons[btn_index];
    if (value["sym"].is_array()) {
      size_t sym_count = value["sym"].as_array()->size();
      button->sym_count = sym_count;
      button->syms = (char **)malloc(sym_count * sizeof(char *));
      if (button->syms == NULL) {
        perror("Malloc failure");
        exit(1);
      }
      size_t sym_i = 0;
      for (auto &&sym : *value["sym"].as_array()) {
        button->syms[sym_i] = strdup(sym.value_or(""));
        sym_i += 1;
      };
    } else if (value["sym"].is_string()) {
      button->sym_count = 1;
      button->syms = (char **)malloc(1 * sizeof(char *));
      if (button->syms == NULL) {
        perror("Malloc failure");
        exit(1);
      }
      button->syms[0] = strdup(value["sym"].value_or(""));
    } else {
      std::cerr << "Button " << key.str() << " missing a symbolic! "
                << std::endl;
      button->sym_count = 0;
    }
    button->name = strdup(std::string(key).c_str());

    button->label = strdup(value["label"].value_or(value["sym"].value_or("")));
    button->case_label = strdup(value["case-label"].value_or(button->label));
    button->clicked_by = 0;
    button->coords = map_coords(toml::node_view<toml::node>(value));
    btn_index += 1;
  });
  config->window.mouse_padding = toml["window"]["mouse-padding"].value_or(0);
  config->window.layer_margin = toml["window"]["layer-margin"].value_or(0);
  config->input.input_thread.quit_cond = PTHREAD_COND_INITIALIZER;
  config->input.input_thread.mut = PTHREAD_MUTEX_INITIALIZER;
  return config;
}