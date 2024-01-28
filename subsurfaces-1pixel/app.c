#include <assert.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <wayland-client.h>

#include "../protocols/single-pixel-buffer-v1.h"
#include "../protocols/viewporter.h"
#include "../protocols/xdg-shell.h"

#include "app.h"
#include "log.h"

static struct {
	struct wl_display *wl_display;
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct wl_compositor *wl_compositor;
	struct wl_subcompositor *wl_subcompositor;
	struct wl_seat *wl_seat;
	struct xdg_wm_base *xdg_wm_base;
	struct wp_single_pixel_buffer_manager_v1 *wp_single_pixel_buffer_manager_v1;
	struct wp_viewporter *wp_viewporter;
} globals;

static struct {
	struct wl_surface *wl_surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	struct wp_viewport *wp_viewport;

	struct wl_buffer *fill_buffer;
} bg;

static struct {
	struct wl_surface *wl_surface;
	struct wl_subsurface *wl_subsurface;
	struct wp_viewport *wp_viewport;

	struct wl_buffer *fill_buffer;
} fg;

static struct {
	void (*on_key)(uint32_t key);
	void (*on_draw)(uint32_t *pixels, int width, int height);

	int width;
	int height;

	int running;
} app;

static void noop() {}

static void registry_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	// clang-format off
	if (strcmp(interface, wl_shm_interface.name) == 0) {
		globals.wl_shm =
				wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}

	else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		globals.wl_compositor =
				wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	}

	else if (strcmp(interface, wl_subcompositor_interface.name) == 0) {
		globals.wl_subcompositor =
				wl_registry_bind(registry, name, &wl_subcompositor_interface, 1);
	}

	else if (strcmp(interface, wl_seat_interface.name) == 0) {
		globals.wl_seat =
				wl_registry_bind(registry, name, &wl_seat_interface, 7);
	}

	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		globals.xdg_wm_base =
				wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);

	}

	else if (strcmp(interface, wp_single_pixel_buffer_manager_v1_interface.name) == 0) {
		globals.wp_single_pixel_buffer_manager_v1 =
				wl_registry_bind(registry, name, &wp_single_pixel_buffer_manager_v1_interface, 1);
	}

	else if (strcmp(interface, wp_viewporter_interface.name) == 0) {
		globals.wp_viewporter =
				wl_registry_bind(registry, name, &wp_viewporter_interface, 1);
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
	xdg_surface_ack_configure(xdg_surface, serial);

	wl_surface_attach(bg.wl_surface, bg.fill_buffer, 0, 0);
	wp_viewport_set_destination(bg.wp_viewport, app.width, app.height);
	wl_surface_commit(bg.wl_surface);

	wl_surface_attach(fg.wl_surface, fg.fill_buffer, 0, 0);
	wl_subsurface_set_position(fg.wl_subsurface, MAX(0, (app.width - 256) / 2), MAX(0, (app.height - 256) / 2));
	wp_viewport_set_destination(fg.wp_viewport, MIN(256, app.width), MIN(256, app.height));
	wl_surface_commit(fg.wl_surface);

}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

static void xdg_toplevel_configure(void *data,
		struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height,
		struct wl_array *states)
{
	if (width > 0) app.width = width;
	if (height > 0) app.height = height;
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	app_stop();
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = xdg_toplevel_configure,
	.close = xdg_toplevel_close,
	.configure_bounds = noop,
	.wm_capabilities = noop,
};

static void wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	if (state != WL_KEYBOARD_KEY_STATE_PRESSED) return;

	if (app.on_key)
		app.on_key(key);
}

static const struct wl_keyboard_listener wl_keyboard_listener = {
	.keymap = noop,
	.enter = noop,
	.leave = noop,
	.key = wl_keyboard_key,
	.modifiers = noop,
	.repeat_info = noop,
};

void app_init(int width, int height,
		const char *title,
		const char *app_id,
		void (*on_key)(uint32_t key),
		void (*on_draw)(uint32_t *pixels, int width, int height))
{
	app.width = width;
	app.height = height;
	app.on_key = on_key;
	app.on_draw = on_draw;
	app.running = 1;

	globals.wl_display = wl_display_connect(NULL);
	globals.wl_registry = wl_display_get_registry(globals.wl_display);
	wl_registry_add_listener(globals.wl_registry, &registry_listener, NULL);
	wl_display_roundtrip(globals.wl_display);

	assert(globals.wl_shm && globals.wl_compositor &&
			globals.wl_subcompositor && globals.wl_seat &&
			globals.xdg_wm_base && globals.wp_single_pixel_buffer_manager_v1 &&
			globals.wp_viewporter);

	// Set up main surface
	bg.wl_surface = wl_compositor_create_surface(globals.wl_compositor);
	bg.xdg_surface = xdg_wm_base_get_xdg_surface(globals.xdg_wm_base, bg.wl_surface);
	bg.xdg_toplevel = xdg_surface_get_toplevel(bg.xdg_surface);
	bg.wp_viewport = wp_viewporter_get_viewport(globals.wp_viewporter, bg.wl_surface);
	xdg_surface_add_listener(bg.xdg_surface, &xdg_surface_listener, NULL);
	xdg_toplevel_add_listener(bg.xdg_toplevel, &xdg_toplevel_listener, NULL);
	xdg_toplevel_set_title(bg.xdg_toplevel, title);
	xdg_toplevel_set_app_id(bg.xdg_toplevel, app_id);
	wl_surface_commit(bg.wl_surface);

	bg.fill_buffer = wp_single_pixel_buffer_manager_v1_create_u32_rgba_buffer(
			globals.wp_single_pixel_buffer_manager_v1, 0, 0, UINT32_MAX / 4,
			UINT32_MAX / 3);

	// Set up subsurface
	fg.wl_surface = wl_compositor_create_surface(globals.wl_compositor);
	fg.wl_subsurface = wl_subcompositor_get_subsurface(globals.wl_subcompositor, fg.wl_surface, bg.wl_surface);
	fg.wp_viewport = wp_viewporter_get_viewport(globals.wp_viewporter, fg.wl_surface);
	wl_subsurface_set_position(fg.wl_subsurface, 10, 10);
	wl_surface_commit(fg.wl_surface);

	fg.fill_buffer = wp_single_pixel_buffer_manager_v1_create_u32_rgba_buffer(
			globals.wp_single_pixel_buffer_manager_v1, UINT32_MAX / 6, 0, 0,
			UINT32_MAX / 3);

	// Set up input
	struct wl_keyboard *wl_keyboard = wl_seat_get_keyboard(globals.wl_seat);
	wl_keyboard_add_listener(wl_keyboard, &wl_keyboard_listener, NULL);
}

void app_run()
{
	enum {
		WAYLAND,
	};

	struct pollfd pollfds[] = {
		[WAYLAND] = {
			.fd = wl_display_get_fd(globals.wl_display),
			.events = POLLIN,
		},
	};

	while (app.running) {
		wl_display_flush(globals.wl_display);

		poll(pollfds, sizeof(pollfds) / sizeof(pollfds[0]), -1);

		if (pollfds[WAYLAND].revents & POLLIN)
			wl_display_dispatch(globals.wl_display);
	}
}

void app_stop()
{
	app.running = 0;
}
