#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>

#include "../protocols/xdg-shell-client-protocol.h"

#include "display.h"

static void wl_registry_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	struct display *d = data;

	if (strcmp(interface, wl_shm_interface.name) == 0) {
		d->wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}

	else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		d->wl_compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	}

	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		d->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
	}
}

static void wl_registry_global_remove(void *data, struct wl_registry *wl_registry,
		uint32_t name)
{
}

static const struct wl_registry_listener wl_registry_listener = {
	.global = wl_registry_global,
	.global_remove = wl_registry_global_remove,
};

struct display *create_display()
{
	struct display *display;

	display = calloc(1, sizeof(*display));

	display->wl_display = wl_display_connect(NULL);
	display->wl_registry = wl_display_get_registry(display->wl_display);
	wl_registry_add_listener(display->wl_registry, &wl_registry_listener, display);
	wl_display_roundtrip(display->wl_display);

	return display;
}

void destroy_display(struct display* display)
{
	if (display->xdg_wm_base)
		xdg_wm_base_destroy(display->xdg_wm_base);

	if (display->wl_shm)
		wl_shm_destroy(display->wl_shm);

	if (display->wl_compositor)
		wl_compositor_destroy(display->wl_compositor);

	wl_registry_destroy(display->wl_registry);
	wl_display_flush(display->wl_display);
	wl_display_disconnect(display->wl_display);

	free(display);
}
