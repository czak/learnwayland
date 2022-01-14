#include <stdlib.h>
#include <wayland-client.h>

#include "../protocols/xdg-shell-client-protocol.h"

#include "display.h"
#include "window.h"
#include "buffer.h"

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
		uint32_t serial)
{
	struct window *window = data;

	xdg_surface_ack_configure(xdg_surface, serial);

	window->on_draw(window->buffer->data);

	wl_surface_attach(window->wl_surface, window->buffer->wl_buffer, 0, 0);
	wl_surface_commit(window->wl_surface);
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

struct window *create_window(struct display *display, int width, int height, void (*on_draw)(uint32_t *))
{
	struct window *window;

	window = calloc(1, sizeof(*window));
	window->display = display;
	window->width = width;
	window->height = height;
	window->on_draw = on_draw;

	window->wl_surface = wl_compositor_create_surface(display->wl_compositor);
	window->xdg_surface = xdg_wm_base_get_xdg_surface(display->xdg_wm_base,
			window->wl_surface);
	window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);

	// not resizable
	xdg_toplevel_set_min_size(window->xdg_toplevel, width, height);
	xdg_toplevel_set_max_size(window->xdg_toplevel, width, height);

	window->buffer = create_buffer(display, width, height);

	xdg_surface_add_listener(window->xdg_surface, &xdg_surface_listener, window);
	// TODO: handle close
	// xdg_toplevel_add_listener(window->xdg_toplevel, &xdg_toplevel_listener, window);
	
	// TODO: is this necessary? does this trigger first configure?
	wl_surface_commit(window->wl_surface);

	return window;
}

void destroy_window(struct window *window)
{
	if (window->buffer)
		destroy_buffer(window->buffer);

	if (window->xdg_toplevel)
		xdg_toplevel_destroy(window->xdg_toplevel);
	if (window->xdg_surface)
		xdg_surface_destroy(window->xdg_surface);
	if (window->wl_surface)
		wl_surface_destroy(window->wl_surface);

	free(window);
}
