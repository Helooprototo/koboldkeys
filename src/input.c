#include "window.h"
#include <fcntl.h>
#include <gtk/gtk.h>
#include <linux/input.h>
#include <poll.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>
#define DOWN 1
#define UP 0
#define REPEAT 2
#define X 0
#define Y 1

void press_button(struct ButtonConfig *button) {
  atomic_fetch_add((_Atomic int *)&button->runtime.clicked_by, 1);
}
void unpress_button(struct ButtonConfig *button) {
  atomic_fetch_sub((_Atomic int *)&button->runtime.clicked_by, 1);
}
int is_button_pressed(struct ButtonConfig *button) {
  return (atomic_load((_Atomic int *)&button->runtime.clicked_by) > 1);
}

void handle_button_press(struct ButtonConfig *button, struct input_event ev) {
  if (ev.value == DOWN) {
    struct ButtonClickUpdate *upd = malloc(sizeof(struct ButtonClickUpdate));
    if (upd == NULL) {
      perror("Malloc failure");
      exit(1);
    }
    upd->button = button->runtime.widget;
    upd->set = TRUE;
    upd->flag = GTK_STATE_FLAG_CHECKED;
    g_idle_add_full(G_PRIORITY_HIGH_IDLE, button_click_update, upd, NULL);
    press_button(button);
  } else if (ev.value == UP) {
    if (!is_button_pressed(button)) {
      struct ButtonClickUpdate *upd = malloc(sizeof(struct ButtonClickUpdate));
      if (upd == NULL) {
        perror("Malloc failure");
        exit(1);
      }
      upd->button = button->runtime.widget;
      upd->set = FALSE;
      upd->flag = GTK_STATE_FLAG_CHECKED;
      g_idle_add_full(G_PRIORITY_HIGH_IDLE, button_click_update, upd, NULL);
    }
    unpress_button(button);
  }
}
int is_thread_running(struct ThreadConfig thread){
  return atomic_load((_Atomic int *)&thread.is_running);
}
void *keyboard_loop(void *args) {
  struct KeyboardThreadConfig *config = (struct KeyboardThreadConfig *)args;

  struct xkb_state *state = config->state;
  printf("Opening input device: %s \n", config->thread.event);
  int input_device = open(config->thread.event, O_RDONLY);
  int flags = fcntl(input_device, F_GETFL, 0);
  fcntl(input_device, flags | O_NONBLOCK);
  if (input_device == -1) {
    perror("error opening input device");
    return (void *)1;
  }
  struct input_event ev;
  int prev_caps_level = 0;
  struct pollfd fds;
  fds.fd = input_device;
  fds.events = POLLIN;
  while (is_thread_running(config->thread)) {
    int ret = poll(&fds, 1, 10);
    if (ret > 0 && (fds.revents & POLLIN)) {
      // Only accept non-repeat Key inputs
      if (read(input_device, &ev, sizeof(ev)) != sizeof(ev)) {
        perror("Read error");
        break;
      };
      if (ev.type == EV_KEY && ev.value != REPEAT) {
        xkb_keycode_t keycode = ev.code + 8;
        xkb_state_update_key(state, keycode,
                             ev.value == UP ? XKB_KEY_UP : XKB_KEY_DOWN);
        // Kinda ugly hack to get the current caps state
        int level = xkb_state_key_get_level(state, KEY_A + 8, 0);
        xkb_keysym_t keysym = xkb_state_key_get_one_sym(state, keycode);
        char key_name[64];
        xkb_keysym_get_name(keysym, key_name, sizeof(key_name));
        for (int i = 0; i < config->size; i++) {
          //  Switching the labels of tha buttons
          if (level == TRUE && level != prev_caps_level) {
            struct ButtonLabelUpdate *upd =
                malloc(sizeof(struct ButtonLabelUpdate));
            if (upd == NULL) {
              perror("Malloc failure");
              exit(1);
            }
            upd->button = config->buttons[i]->conf.runtime.widget;
            upd->name = config->buttons[i]->case_label;
            g_idle_add(button_label_update, upd);
          } else if (level != prev_caps_level) {
            struct ButtonLabelUpdate *upd =
                malloc(sizeof(struct ButtonLabelUpdate));
            if (upd == NULL) {
              perror("Malloc failure");
              exit(1);
            }
            upd->button = config->buttons[i]->conf.runtime.widget;
            upd->name = config->buttons[i]->label;
            g_idle_add(button_label_update, upd);
          }
          // Color tha buttons if sym is pressed
          for (int sym_i = 0; sym_i < config->buttons[i]->sym_count; sym_i++) {
            char *sym = config->buttons[i]->syms[sym_i];
            if (strcasecmp(key_name, sym) == 0) {
              handle_button_press(&config->buttons[i]->conf, ev);
            }
          }
        }
        prev_caps_level = level;
        printf("Pressed Sym: %s\n", key_name);
      }
    }
  }
  free(config->thread.event);
  free(config);
  return (void *)0;
}

void *mouse_loop(void *args) {
  struct MouseThreadConfig *config = (struct MouseThreadConfig *)args;
  struct input_event ev;
  printf("Opening input device: %s\n",config->thread.event);
  int mouse = open(config->thread.event, O_RDONLY);
  struct pollfd fds;
  fds.fd = mouse;
  fds.events = POLLIN;
  while (is_thread_running(config->thread)) {
    int ret = poll(&fds, 1, 10);
    if (ret > 0 && (fds.revents & POLLIN)) {
      if (read(mouse, &ev, sizeof(ev)) != sizeof(ev)) {
        perror("Read error");
        break;
      };
      if (ev.type == EV_REL && config->movement_widget.should_show &&
          (ev.code != REL_WHEEL && ev.code != REL_WHEEL_HI_RES)) {
        struct MouseMoveUpdate *upd = malloc(sizeof(struct MouseMoveUpdate));
        upd->mouse_widget = config->movement_widget.widget;
        upd->x = 0;
        upd->y = 0;
        if (ev.code == X) {
          upd->x = ev.value;
        } else if (ev.code == Y) {
          upd->y = ev.value;
        }
        upd->x += config->movement_widget.coords->x;
        upd->y += config->movement_widget.coords->y;
        g_idle_add_full(G_PRIORITY_HIGH_IDLE, mouse_move_update, upd, NULL);
      } else if (ev.type == EV_KEY) {

        for (int i = 0; i < config->size; i++) {
          if (config->buttons[i]->key == ev.code) {
            handle_button_press(&config->buttons[i]->conf, ev);
          }
        }
        printf("Pressed mouse key: %i\n", ev.code);
      } else if (ev.type == EV_REL && ev.code == REL_WHEEL) {
        for (int i = 0; i < config->wheel_size; i++) {
          printf("%i",config->wheels[i]->axis);
          if (config->wheels[i]->axis == ev.value ||
              config->wheels[i]->axis == SCROLLBOTH) {
            struct ButtonScrollUpdate *upd =
                malloc(sizeof(struct ButtonScrollUpdate));
            if (upd == NULL) {
              perror("Malloc failure");
              exit(1);
            }
            upd->button = config->wheels[i]->conf.runtime.widget;
            upd->axis = ev.value;

            if (config->wheels[i]->g_source != 0) {
              g_source_remove(config->wheels[i]->g_source);
            }
            struct ButtonScrollClearUpdate *clear =
                malloc(sizeof(struct ButtonScrollClearUpdate));
            clear->button = config->wheels[i]->conf.runtime.widget;
            clear->g_source = &config->wheels[i]->g_source;
            config->wheels[i]->g_source = g_timeout_add_full(
                G_PRIORITY_HIGH_IDLE, config->wheel_clear_timeout,
                button_scroll_clear, clear, NULL);

            g_idle_add_full(G_PRIORITY_HIGH_IDLE, button_scroll_update, upd,
                            NULL);
          }
        }
      }
    }
  }

  free(config->thread.event);
  free(config);
  return (void *)0;
}
void destroy_button_static_data(struct ButtonConfig *button) {
  free(button->st.name);
  free(button->st.coords);
  free(button->st.css_class);
}
void destroy_keyboard_button(struct KeyboardButtonConfig *button) {
  for (int x = 0; x < button->sym_count; x++) {
    free(button->syms[x]);
  }
  destroy_button_static_data(&button->conf);
  free(button->case_label);
  free(button->label);
  free(button->syms);
  free(button);
}
void *input_loop(void *args) {
  struct InputConfig *conf = (struct InputConfig *)args;
  struct ThreadContainer *threads = malloc((conf->kbd.dev.device_count+conf->mouse.dev.device_count)*sizeof(struct ThreadContainer));
  int thread_count = 0;
  if (conf->kbd.dev.device_count > 0) {
    for (int i = 0; i < conf->kbd.dev.device_count; i++) {
      size_t malloc_size = sizeof(struct KeyboardThreadConfig);
      struct KeyboardThreadConfig *kbd_conf = malloc(malloc_size);
      threads[thread_count].conf = &kbd_conf->thread;
      memcpy(kbd_conf, &conf->kbd.thread_conf, malloc_size);
      kbd_conf->thread.event = strdup(conf->kbd.dev.devices[i]);
      atomic_store((_Atomic int *)&kbd_conf->thread.is_running, 1);
      pthread_create(&threads[thread_count].thread, NULL, keyboard_loop, kbd_conf);
      thread_count+=1;
    }
  }
  if (conf->mouse.dev.device_count > 0) {
    for (int i = 0; i < conf->mouse.dev.device_count; i++) {
      size_t malloc_size = sizeof(struct MouseThreadConfig);
      struct MouseThreadConfig *mouse_conf = malloc(malloc_size);
      memcpy(mouse_conf, &conf->mouse.thread_conf, malloc_size);
      threads[thread_count].conf = &mouse_conf->thread;
      mouse_conf->thread.event = strdup(conf->mouse.dev.devices[i]);
      atomic_store((_Atomic int *)&mouse_conf->thread.is_running, 1);
      pthread_create(&threads[thread_count].thread, NULL, mouse_loop, mouse_conf);
      thread_count+=1;
    }
  }
  pthread_mutex_lock(&conf->input_thread.mut);
  pthread_cond_wait(&conf->input_thread.quit_cond, &conf->input_thread.mut);
  pthread_mutex_unlock(&conf->input_thread.mut);
  for (int i = 0; i < thread_count; i++) {
    atomic_store((_Atomic int *)&threads[i].conf->is_running, 0);
    pthread_join(threads[i].thread, NULL);
  }
  for (int i = 0; i < conf->kbd.thread_conf.size; i++) {
    destroy_keyboard_button(conf->kbd.thread_conf.buttons[i]);
  }
  free(conf->kbd.thread_conf.buttons);
  for (int i = 0; i < conf->mouse.thread_conf.wheel_size; i++) {
    destroy_button_static_data(&conf->mouse.thread_conf.wheels[i]->conf);
    free(conf->mouse.thread_conf.wheels[i]);
  }
  free(conf->mouse.thread_conf.wheels);
  for (int i = 0; i < conf->mouse.thread_conf.size; i++) {
    destroy_button_static_data(&conf->mouse.thread_conf.buttons[i]->conf);
    free(conf->mouse.thread_conf.buttons[i]);
  }
  if (conf->mouse.thread_conf.movement_widget.should_show) {
    free(conf->mouse.thread_conf.movement_widget.coords);
  }
  free(conf->mouse.thread_conf.buttons);
  free(threads);

  return (void *)0;
}