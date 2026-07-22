#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "structs.h"
gboolean button_label_update(void* data);
gboolean button_click_update(void* data);
void* create_overlay(struct Config* arg);
#endif
