#ifndef DISPLAY_H
#define DISPLAY_Y

struct display {
	struct wl_display *wl_display;
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct wl_compositor *wl_compositor;
	struct wl_seat *wl_seat;
	struct xdg_wm_base *xdg_wm_base;

	// EGL
	void *egl_display;
	void *egl_config;
	void *egl_context;
};

struct display *create_display();
void destroy_display(struct display *display);

#endif
