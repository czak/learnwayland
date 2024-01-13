#include <stdio.h>
#include <wayland-client.h>

static void wl_registry_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	fprintf(stderr, "%d: %s (%d)\n", name, interface, version);
}

static void wl_registry_global_remove(void *data,
		struct wl_registry *wl_registry, uint32_t name) {}

static const struct wl_registry_listener wl_registry_listener = {
		.global = wl_registry_global,
		.global_remove = wl_registry_global_remove,
};

int main(void) {
	struct wl_display *wl_display = wl_display_connect(NULL);
	struct wl_registry *wl_registry = wl_display_get_registry(wl_display);
	wl_registry_add_listener(wl_registry, &wl_registry_listener, NULL);
	wl_display_roundtrip(wl_display);

	return 0;
}
