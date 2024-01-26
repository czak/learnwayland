#include <assert.h>
#include <stdlib.h>
#include <wayland-client.h>

#include "../protocols/xdg-shell.h"

#include "display.h"
#include "window.h"
#include "buffer.h"

static void frame(void *data, struct wl_callback *wl_callback, uint32_t time);

static const struct wl_callback_listener frame_listener = {
	.done = frame,
};

static void noop()
{
}

static void frame(void *data, struct wl_callback *wl_callback, uint32_t time)
{
	struct window *window = data;
	struct buffer *buffer = window->buffers[window->current_buffer_index];

	assert(!buffer->busy);

	if (window->on_draw)
		window->on_draw(buffer->data, time);

	// Request next frame
	struct wl_callback *frame_callback = wl_surface_frame(window->wl_surface);
	wl_callback_add_listener(frame_callback, &frame_listener, window);

	wl_surface_attach(window->wl_surface, buffer->wl_buffer, 0, 0);
	wl_surface_damage_buffer(window->wl_surface, 0, 0, window->width, window->height);
	wl_surface_commit(window->wl_surface);

	buffer->busy = 1;

	// Switch to the other buffer
	window->current_buffer_index = 1 - window->current_buffer_index;
}

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
		uint32_t serial)
{
	struct window *window = data;

	xdg_surface_ack_configure(xdg_surface, serial);

	if (!window->configured) {
		frame(window, NULL, 0);
		window->configured = 1;
	}
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	struct window *window = data;

	if (window->on_close)
		window->on_close();
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = noop,
	.close = xdg_toplevel_close,
};

struct window *create_window(struct display *display, int width, int height, void (*on_draw)(uint32_t *pixels, uint32_t time), void (*on_close)())
{
	struct window *window;

	window = calloc(1, sizeof(*window));
	window->display = display;
	window->width = width;
	window->height = height;
	window->on_draw = on_draw;
	window->on_close = on_close;
	window->configured = 0;

	window->buffers[0] = create_buffer(display, width, height);
	window->buffers[1] = create_buffer(display, width, height);

	window->wl_surface = wl_compositor_create_surface(display->wl_compositor);
	window->xdg_surface = xdg_wm_base_get_xdg_surface(display->xdg_wm_base,
			window->wl_surface);
	xdg_surface_add_listener(window->xdg_surface, &xdg_surface_listener, window);
	window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
	xdg_toplevel_add_listener(window->xdg_toplevel, &xdg_toplevel_listener, window);

	// After creating a role-specific object and setting it up,
	// the client must perform an initial commit without any buffer attached
	// -- https://wayland.app/protocols/xdg-shell#xdg_surface
	wl_surface_commit(window->wl_surface);

	// not resizable
	xdg_toplevel_set_min_size(window->xdg_toplevel, width, height);
	xdg_toplevel_set_max_size(window->xdg_toplevel, width, height);

	return window;
}

void destroy_window(struct window *window)
{
	if (window->buffers[0])
		destroy_buffer(window->buffers[0]);
	if (window->buffers[1])
		destroy_buffer(window->buffers[1]);

	if (window->xdg_toplevel)
		xdg_toplevel_destroy(window->xdg_toplevel);
	if (window->xdg_surface)
		xdg_surface_destroy(window->xdg_surface);
	if (window->wl_surface)
		wl_surface_destroy(window->wl_surface);

	free(window);
}
