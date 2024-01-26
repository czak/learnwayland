#pragma once

struct app_state {
	// Wayland globals
	struct wl_display *wl_display;
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct wl_compositor *wl_compositor;
	struct xdg_wm_base *xdg_wm_base;

	// Surface & roles
	struct wl_surface *wl_surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;

	// App state
	int running;

	int width;
	int height;
};


void app_init(struct app_state *app);
int app_run(struct app_state *app);
