#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>

struct window {
	struct display *display;

	// Window objects
	struct wl_surface *wl_surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;

	struct wl_surface *drawing_wl_surface;
	struct wl_subsurface *drawing_wl_subsurface;

	// Input objects
	struct wl_keyboard *wl_keyboard;

	int width;
	int height;

	struct buffer *buffers[2];
	int current_buffer_index;

	void (*on_draw)(uint32_t *pixels, uint32_t time);
	void (*on_close)();

	int configured;
};

struct window *create_window(struct display *display, int width, int height, void (*on_draw)(uint32_t *pixels, uint32_t time), void (*on_close)());
void destroy_window(struct window *window);

#endif
