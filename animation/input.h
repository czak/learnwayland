#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

struct input {
	struct display *display;

	struct wl_keyboard *wl_keyboard;

	void (*on_key)(uint32_t key);
};

struct input* create_input(struct display *display, void (*on_key)(uint32_t key));
void destroy_input(struct input *input);

#endif
