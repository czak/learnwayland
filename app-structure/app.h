#pragma once

#include <stdint.h>

struct app_state {
	// Wayland globals
	struct wl_display *wl_display;
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct wl_compositor *wl_compositor;
	struct xdg_wm_base *xdg_wm_base;

	// Wayland objects
	struct wl_surface *wl_surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;

	// Backing buffer
	struct {
		struct wl_buffer *wl_buffer;
		uint32_t *data;
	} buffer;

	// App state
	int running;

	// Callbacks
	void (*on_close)(struct app_state *app);
};

void app_init(struct app_state *app);
int app_run(struct app_state *app);
