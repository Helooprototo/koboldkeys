#include "window.h"
#include <fcntl.h>
#include <gtk/gtk.h>
#include <linux/input.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>
#define DOWN 1
#define UP 0
#define REPEAT 2

void *keyboard_loop(void *args) {
  struct KeyboardInputConfig *config = (struct KeyboardInputConfig *)args;

  struct xkb_state *state = config->state;
  printf("Opening input device: %s \n", config->event);
  int input_device = open(config->event, O_RDONLY);
  if (input_device == -1) {
    perror("error opening input device");
    return (void *)1;
  }
  struct input_event ev;
  int prev_caps_level = 0;
  while (true) {
    if (read(input_device, &ev, sizeof(ev)) != sizeof(ev)) {
      perror("Failed to read event");
      break;
    } else {

      // Only accept non-repeat Key inputs
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
            upd->button = config->buttons[i]->button;
            upd->name = config->buttons[i]->case_label;
            g_idle_add(button_label_update, upd);
          } else if (level != prev_caps_level) {
            struct ButtonLabelUpdate *upd =
                malloc(sizeof(struct ButtonLabelUpdate));
            if (upd == NULL) {
              perror("Malloc failure");
              exit(1);
            }
            upd->button = config->buttons[i]->button;
            upd->name = config->buttons[i]->label;
            g_idle_add(button_label_update, upd);
          }
          // Color tha buttons if sym is pressed
          for (int sym_i = 0; sym_i < config->buttons[i]->sym_count; sym_i++) {
            char *sym = config->buttons[i]->syms[sym_i];
            if (ev.value == DOWN && strcasecmp(key_name, sym) == 0) {
              struct ButtonClickUpdate *upd =
                  malloc(sizeof(struct ButtonClickUpdate));
              if (upd == NULL) {
                perror("Malloc failure");
                exit(1);
              }
              upd->button = config->buttons[i]->button;
              upd->set = TRUE;
              upd->flag = GTK_STATE_FLAG_CHECKED;
              g_idle_add_full(G_PRIORITY_HIGH_IDLE, button_click_update, upd,
                              NULL);
              atomic_fetch_add((_Atomic int *)&config->buttons[i]->clicked_by,
                               1);
            } else if (ev.value == UP && strcasecmp(key_name, sym) == 0) {
              if (config->buttons[i]->clicked_by <= 1) {
                struct ButtonClickUpdate *upd =
                    malloc(sizeof(struct ButtonClickUpdate));
                if (upd == NULL) {
                  perror("Malloc failure");
                  exit(1);
                }
                upd->button = config->buttons[i]->button;
                upd->set = FALSE;
                upd->flag = GTK_STATE_FLAG_CHECKED;
                g_idle_add_full(G_PRIORITY_HIGH_IDLE, button_click_update, upd,
                                NULL);
              }
              atomic_fetch_sub((_Atomic int *)&config->buttons[i]->clicked_by,
                               1);
            }
          }
        }
        prev_caps_level = level;
        printf("Pressed Sym: %s\n", key_name);
      }
    }
  }
  return (void *)0;
}

void *mouse_loop(void *args) {
  struct MouseInputConfig *conf = (struct MouseInputConfig *)args;
  struct input_event ev;
  int mouse = open(conf->event, O_RDONLY);
  while (TRUE) {
    if (read(mouse, &ev, sizeof(ev)) != sizeof(ev)) {
      perror("Failed to read event");
    }
    if (ev.type == EV_REL) {
      printf("Mouse event: %i %i\n", ev.code, ev.value);
      if (ev.code == 0) {

        gtk_fixed_move(GTK_FIXED(conf->fixed), conf->mouse_widget,
                       1 + ev.value * 10, 100);
      } else if (ev.code == 1) {
        gtk_fixed_move(GTK_FIXED(conf->fixed), conf->mouse_widget, 1,
                       100 + ev.value * 10);
      }
    }
  }
  return (void *)0;
}

void *input_loop(void *args) {
  struct InputConfig *conf = (struct InputConfig *)args;
  pthread_t *kbd_threads =
      malloc(conf->kbd.dev.device_count * sizeof(pthread_t));
  pthread_t *mouse_threads =
      malloc(conf->mouse.dev.device_count * sizeof(pthread_t));
  if (conf->kbd.dev.device_count > 0) {
    for (int i = 0; i < conf->kbd.dev.device_count; i++) {
      size_t malloc_size = sizeof(struct KeyboardInputConfig) +
                           conf->kbd.input.size * sizeof(struct ButtonConfig *);
      struct KeyboardInputConfig *kbd_conf = malloc(malloc_size);
      memcpy(kbd_conf, &conf->kbd.input, malloc_size);
      kbd_conf->event = strdup(conf->kbd.dev.devices[i]);
      pthread_create(&kbd_threads[i], NULL, keyboard_loop, kbd_conf);
    }
  }
  if (conf->mouse.dev.device_count > 0) {
    for (int i = 0; i < conf->mouse.dev.device_count; i++) {
      size_t malloc_size = sizeof(struct MouseInputConfig);
      struct MouseInputConfig *mouse_conf = malloc(malloc_size);
      memcpy(mouse_conf, &conf->mouse.input, malloc_size);
      mouse_conf->event = strdup(conf->mouse.dev.devices[i]);
      pthread_create(&mouse_threads[i], NULL, mouse_loop, mouse_conf);
    }
  }
  pthread_mutex_lock(&conf->mut);
  while (!conf->should_quit) {
    pthread_cond_wait(&conf->quit_cond, &conf->mut);
  }
  pthread_mutex_unlock(&conf->mut);
  for (int i; i < conf->kbd.dev.device_count; i++) {
    pthread_cancel(kbd_threads[i]);
  }
  for (int i; i < conf->mouse.dev.device_count; i++) {
    pthread_cancel(mouse_threads[i]);
  }
  free(kbd_threads);
  free(mouse_threads);
  xkb_state_unref(conf->kbd.input.state);
  free(conf);
  return (void *)0;
}
