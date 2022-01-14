#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>

struct window {
	struct display *display;

	struct wl_surface *wl_surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;

	int width;
	int height;

	struct buffer *buffer;

	void (*on_draw)(uint32_t *);
};

struct window *create_window(struct display *display, int width, int height, void (*on_draw)(uint32_t *));
void destroy_window(struct window *window);

#endif
