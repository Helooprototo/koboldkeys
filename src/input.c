#include "key.h"
#include "window.h"
#include <fcntl.h>
#include <gtk/gtk.h>
#include <linux/input.h>
#include <stdio.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

#define DOWN 1
#define UP 0
#define REPEAT 2

void *keyboard_loop(void *args) {
  struct KeyboardConfig *config = (struct KeyboardConfig *)args;

  struct xkb_context *kbd_ctx;
  kbd_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!kbd_ctx) {
    perror("Could not create keyboard context");
  }
  struct xkb_keymap *keymap;
  struct xkb_rule_names names = {NULL, NULL, .layout = config->xkb.layout,
                                 .variant = config->xkb.variant,
                                 config->xkb.options};
  keymap = xkb_keymap_new_from_names2(
      kbd_ctx, &names, XKB_KEYMAP_FORMAT_TEXT_V2, XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!keymap) {
    perror("Could not create keymap");
  }
  struct xkb_state *state;
  state = xkb_state_new(keymap);
  if (!state) {
    perror("Could not create state");
  }
  printf("Opening input device: %s \n", config->event);
  int input_device = open(config->event, O_RDONLY);
  if (input_device == -1) {
    perror("error opening input device");
    return (void *)1;
  }
  struct input_event ev;
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
          char *button_sym = strdup(config->buttons[i].sym);
          const char *token = strtok(button_sym, "|");
          // Switching the labels of tha buttons
          if (level == TRUE) {
            struct ButtonLabelUpdate *upd =
                malloc(sizeof(struct ButtonLabelUpdate));
            upd->button = config->buttons[i].button;
            upd->name = config->buttons[i].case_label;
            config->buttons[i].caps_state = TRUE;
            g_idle_add(button_label_update, upd);
          } else {
            struct ButtonLabelUpdate *upd =
                malloc(sizeof(struct ButtonLabelUpdate));
            upd->button = config->buttons[i].button;
            upd->name = config->buttons[i].label;
            config->buttons[i].caps_state = TRUE;
            g_idle_add(button_label_update, upd);
          }
          // Color tha buttons if sym is pressed
          while (token) {
            if (ev.value == DOWN && strcasecmp(key_name, token) == 0) {
              struct ButtonClickUpdate *upd =
                  malloc(sizeof(struct ButtonClickUpdate));
              upd->button = config->buttons[i].button;
              upd->set = TRUE;
              upd->flag = GTK_STATE_FLAG_CHECKED;
              // Not sure if this is the right way to do things but we handle
              // the setting of the state flag IN this thread instead of queuing
              // Otherwise theres a 1-2 millisecond delay between the physical
              // key and the state update, this cuts that down to ~20-30
              // microseconds g_idle_add(button_click_update, upd);

              if (upd->set) {
                gtk_widget_set_state_flags(upd->button, upd->flag, FALSE);
              } else {
                gtk_widget_unset_state_flags(upd->button, upd->flag);
              }
              g_free(upd);
              config->buttons[i].clicked_by += 1;
            } else if (ev.value == UP && strcasecmp(key_name, token) == 0) {
              if (config->buttons[i].clicked_by <= 1) {
                struct ButtonClickUpdate *upd =
                    malloc(sizeof(struct ButtonClickUpdate));
                upd->button = config->buttons[i].button;
                upd->set = FALSE;
                upd->flag = GTK_STATE_FLAG_CHECKED;
                // g_idle_add(button_click_update, upd);
                if (upd->set) {
                  gtk_widget_set_state_flags(upd->button, upd->flag, FALSE);
                } else {
                  gtk_widget_unset_state_flags(upd->button, upd->flag);
                }
                g_free(upd);
              }
              config->buttons[i].clicked_by -= 1;
            }
            token = strtok(NULL, "|");
          }
          free(button_sym);
        }
        printf("Sym: %s\n", key_name);
      }
    }
  }
  return (void *)0;
}

void *mouse_loop(void *args) {
  struct MouseConfig *conf = (struct MouseConfig *)args;
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
  if (strcmp(conf->kbd.event, "") != 0) {
    pthread_t kbd;
    pthread_create(&kbd, NULL, keyboard_loop, &conf->kbd);
  }
  if (strcmp(conf->mouse.event, "") != 0) {
    pthread_t mouse;
    pthread_create(&mouse, NULL, mouse_loop, &conf->mouse);
  }
  return (void *)0;
}
