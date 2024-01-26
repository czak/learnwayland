#pragma once

struct app_state {
	// Wayland globals
	struct wl_display *wl_display;
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct wl_compositor *wl_compositor;
	struct xdg_wm_base *xdg_wm_base;
	struct wp_viewporter *wp_viewporter;
	struct wp_single_pixel_buffer_manager_v1 *wp_single_pixel_buffer_manager_v1;
	struct zxdg_decoration_manager_v1 *zxdg_decoration_manager_v1;

	// Surface & roles
	struct wl_surface *wl_surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	struct zxdg_toplevel_decoration_v1 *zxdg_toplevel_decoration_v1;

	// Surface content & transform
	struct wp_viewport *wp_viewport;

	// App state
	int running;

	int width;
	int height;
};


void app_init(struct app_state *app);
int app_run(struct app_state *app);
