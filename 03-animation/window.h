#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>

struct window {
	struct display *display;

	// Window objects
	struct wl_surface *wl_surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;

	// Input objects
	struct wl_keyboard *wl_keyboard;

	int width;
	int height;

	struct buffer *buffer;

	void (*on_draw)(uint32_t *);
	void (*on_close)();
};

struct window *create_window(struct display *display, int width, int height, void (*on_draw)(uint32_t *), void (*on_close)());
void destroy_window(struct window *window);

#endif
