#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <unistd.h>
#include <wayland-client.h>

#include "../protocols/xdg-shell.h"

#include "app.h"

struct buffer {
	struct wl_buffer *wl_buffer;
	uint32_t *pixels;
	int size;
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

	else if (strcmp(interface, wl_seat_interface.name) == 0) {
		app->wl_seat =
				wl_registry_bind(registry, name, &wl_seat_interface, 8);
	}

	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		app->xdg_wm_base =
				wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
	}
	// clang-format on
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = noop,
};

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer)
{
	struct buffer *buffer = data;

	wl_buffer_destroy(buffer->wl_buffer);
	munmap(buffer->pixels, buffer->size);
	free(buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
	.release = wl_buffer_release,
};

static struct buffer *create_buffer(struct app_state *app, int width, int height)
{
	int size = width * height * 4;
	int stride = width * 4;

	int fd = memfd_create("buffer-pool", 0);
	ftruncate(fd, size);

	struct wl_shm_pool *wl_shm_pool = wl_shm_create_pool(app->wl_shm, fd, size);

	struct buffer *buffer = calloc(1, sizeof(*buffer));
	buffer->size = size;
	buffer->pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	buffer->wl_buffer = wl_shm_pool_create_buffer(wl_shm_pool, 0, width,
					height, stride, WL_SHM_FORMAT_ARGB8888);
	wl_buffer_add_listener(buffer->wl_buffer, &wl_buffer_listener, buffer);

	wl_shm_pool_destroy(wl_shm_pool);
	close(fd);

	return buffer;
}

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
		uint32_t serial)
{
	struct app_state *app = data;

	xdg_surface_ack_configure(xdg_surface, serial);

	struct buffer *buffer = create_buffer(app, app->width, app->height);
	for (int y = 0; y < app->height; y++) {
		for (int x = 0; x < app->width; x++) {
			uint8_t r = x ^ y;
			uint8_t g = x ^ y;
			uint8_t b = x ^ y;
			uint8_t a = 0x7f;
			buffer->pixels[y * app->width + x] = (a << 24) + (r << 16) + (g << 8) + b; 
		}
	}

	wl_surface_attach(app->wl_surface, buffer->wl_buffer, 0, 0);
	wl_surface_commit(app->wl_surface);
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

static void xdg_toplevel_configure(void *data,
		struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height,
		struct wl_array *states)
{
	struct app_state *app = data;

	if (width > 0) app->width = width;
	if (height > 0) app->height = height;
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	struct app_state *app = data;

	if (app->on_close)
		app->on_close(app);
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
	struct app_state *app = data;

	if (app->on_key)
		app->on_key(app, key);
}

static const struct wl_keyboard_listener wl_keyboard_listener = {
	.keymap = noop,
	.enter = noop,
	.leave = noop,
	.key = wl_keyboard_key,
	.modifiers = noop,
	.repeat_info = noop,
};

void app_init(struct app_state *app)
{
	app->wl_display = wl_display_connect(NULL);
	app->wl_registry = wl_display_get_registry(app->wl_display);
	wl_registry_add_listener(app->wl_registry, &registry_listener, app);
	wl_display_roundtrip(app->wl_display);

	assert(app->wl_shm &&
			app->wl_compositor &&
			app->wl_seat &&
			app->xdg_wm_base);

	// Set up surface
	app->wl_surface = wl_compositor_create_surface(app->wl_compositor);

	app->xdg_surface = xdg_wm_base_get_xdg_surface(app->xdg_wm_base, app->wl_surface);
	xdg_surface_add_listener(app->xdg_surface, &xdg_surface_listener, app);

	app->xdg_toplevel = xdg_surface_get_toplevel(app->xdg_surface);
	xdg_toplevel_add_listener(app->xdg_toplevel, &xdg_toplevel_listener, app);
	xdg_toplevel_set_title(app->xdg_toplevel, "SHM buffer sample");
	xdg_toplevel_set_app_id(app->xdg_toplevel, "learnwayland");

	wl_surface_commit(app->wl_surface);

	// Set up input
	struct wl_keyboard *wl_keyboard = wl_seat_get_keyboard(app->wl_seat);
	wl_keyboard_add_listener(wl_keyboard, &wl_keyboard_listener, app);

	app->running = 1;
}

int app_run(struct app_state *app)
{
	return app->running == 1 && wl_display_dispatch(app->wl_display) != -1;
}
