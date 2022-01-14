#ifndef WINDOW_H
#define WINDOW_H

struct window {
	struct display *display;

	struct wl_surface *wl_surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;

	int width;
	int height;

	struct buffer *buffer;
};

struct window *create_window(struct display *display, int width, int height);
void destroy_window(struct window *window);

#endif
