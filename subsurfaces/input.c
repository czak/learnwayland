#include <stdlib.h>
#include <wayland-client.h>

#include "display.h"
#include "input.h"

static void noop()
{
}

static void
wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	struct input *input = data;
	input->on_key(key);
}

static const struct wl_keyboard_listener wl_keyboard_listener = {
	.keymap = noop,
	.enter = noop,
	.leave = noop,
	.key = wl_keyboard_key,
	.modifiers = noop,
	.repeat_info = noop,
};

struct input *create_input(struct display *display, void (*on_key)(uint32_t))
{
	struct input *input;

	input = calloc(1, sizeof(*input));
	input->display = display;
	input->on_key = on_key;

	input->wl_keyboard = wl_seat_get_keyboard(display->wl_seat);
	wl_keyboard_add_listener(input->wl_keyboard, &wl_keyboard_listener, input);

	return input;
}

void destroy_input(struct input *input)
{
	if (input->wl_keyboard)
		wl_keyboard_destroy(input->wl_keyboard);

	free(input);
}
