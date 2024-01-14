#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include "protocols/xdg-decoration-unstable-v1.h"
#include "protocols/xdg-shell.h"
#include "protocols/single-pixel-buffer-v1.h"
#include "protocols/viewporter.h"

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
	struct wl_buffer *wl_buffer;
	struct wp_viewport *wp_viewport;

	// App state
	bool configured;
	bool running;
};

static void noop() {}

static void registry_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	struct app_state *app = data;

	// clang-format off
	if (strcmp(interface, wl_shm_interface.name) == 0) {
		app->wl_shm =
				wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}

	else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		app->wl_compositor =
				wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	}

	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		app->xdg_wm_base =
				wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
	}

	else if (strcmp(interface, wp_viewporter_interface.name) == 0) {
		app->wp_viewporter =
				wl_registry_bind(registry, name, &wp_viewporter_interface, 1);
	}

	else if (strcmp(interface, wp_single_pixel_buffer_manager_v1_interface.name) == 0) {
		app->wp_single_pixel_buffer_manager_v1 =
				wl_registry_bind(registry, name, &wp_single_pixel_buffer_manager_v1_interface, 1);
	}

	else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
		app->zxdg_decoration_manager_v1 =
				wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1);
	}
	// clang-format on
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = noop,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
		uint32_t serial)
{
	struct app_state *app = data;

	xdg_surface_ack_configure(xdg_surface, serial);
	if (app->configured) {
		wl_surface_commit(app->wl_surface);
	} else {
		app->configured = true;
	}
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	struct app_state *app = data;

	app->running = false;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = noop,
	.close = xdg_toplevel_close,
	.configure_bounds = noop,
	.wm_capabilities = noop,
};

void app_init(struct app_state *app) {
	app->wl_display = wl_display_connect(NULL);
	app->wl_registry = wl_display_get_registry(app->wl_display);
	wl_registry_add_listener(app->wl_registry, &registry_listener, app);
	wl_display_roundtrip(app->wl_display);

	assert(app->wl_shm &&
			app->wl_compositor &&
			app->xdg_wm_base &&
			app->wp_viewporter &&
			app->wp_single_pixel_buffer_manager_v1 &&
			app->zxdg_decoration_manager_v1);

	// Set up surface
	app->wl_surface = wl_compositor_create_surface(app->wl_compositor);

	app->xdg_surface = xdg_wm_base_get_xdg_surface(app->xdg_wm_base, app->wl_surface);
	xdg_surface_add_listener(app->xdg_surface, &xdg_surface_listener, app);

	app->xdg_toplevel = xdg_surface_get_toplevel(app->xdg_surface);
	xdg_toplevel_add_listener(app->xdg_toplevel, &xdg_toplevel_listener, app);
	xdg_toplevel_set_title(app->xdg_toplevel, "Decoration sample");
	xdg_toplevel_set_min_size(app->xdg_toplevel, 256, 256);
	xdg_toplevel_set_max_size(app->xdg_toplevel, 256, 256);

	app->wp_viewport = wp_viewporter_get_viewport(app->wp_viewporter, app->wl_surface);
	wp_viewport_set_destination(app->wp_viewport, 256, 256);

	app->zxdg_toplevel_decoration_v1 =
			zxdg_decoration_manager_v1_get_toplevel_decoration(
					app->zxdg_decoration_manager_v1, app->xdg_toplevel);
	zxdg_toplevel_decoration_v1_set_mode(app->zxdg_toplevel_decoration_v1,
			ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);

	wl_surface_commit(app->wl_surface);
}

int main(int argc, char *argv[])
{
	struct app_state app = {
		.configured = false,
		.running = true,
	};

	app_init(&app);

	// Run loop until surface configured
	while (wl_display_dispatch(app.wl_display) != -1 && !app.configured) {
	}

	// Attach buffer
	uint32_t shade = UINT32_MAX * 0.3;
	app.wl_buffer = wp_single_pixel_buffer_manager_v1_create_u32_rgba_buffer(
			app.wp_single_pixel_buffer_manager_v1, shade, shade, shade,
			UINT32_MAX);
	wl_surface_attach(app.wl_surface, app.wl_buffer, 0, 0);
	wl_surface_commit(app.wl_surface);

	// Main loop
	while (wl_display_dispatch(app.wl_display) != -1 && app.running) {
	}

	return 0;
}
