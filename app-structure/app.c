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

static struct {
	struct wl_display *wl_display;
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct wl_compositor *wl_compositor;
	struct wl_seat *wl_seat;
	struct xdg_wm_base *xdg_wm_base;
} globals;

static struct {
	struct wl_surface *wl_surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
} surface;

static struct {
	void (*on_close)();
	void (*on_key)(uint32_t key);
	void (*on_draw)(uint32_t *pixels, int width, int height);

	int width;
	int height;
} app;

static struct {
	struct wl_buffer *wl_buffer;

	uint32_t *pixels;
	int size;

	int width;
	int height;

	int busy;
} buffer;

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

	else if (strcmp(interface, wl_seat_interface.name) == 0) {
		globals.wl_seat =
				wl_registry_bind(registry, name, &wl_seat_interface, 8);
	}

	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		globals.xdg_wm_base =
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
	// Make sure compositor is releasing the one buffer we're holding
	assert(buffer.wl_buffer == wl_buffer);

	buffer.busy = 0;
}

static const struct wl_buffer_listener wl_buffer_listener = {
	.release = wl_buffer_release,
};

static void buffer_init(int width, int height)
{
	static int fd;

	if (buffer.wl_buffer)
		wl_buffer_destroy(buffer.wl_buffer);
	if (buffer.pixels)
		munmap(buffer.pixels, buffer.size);

	int stride = width * sizeof(uint32_t);
	int size = stride * height;

	if (fd == 0)
		fd = memfd_create("buffer-pool", 0);
	ftruncate(fd, size);

	struct wl_shm_pool *wl_shm_pool = wl_shm_create_pool(globals.wl_shm, fd, size);

	buffer.wl_buffer = wl_shm_pool_create_buffer(wl_shm_pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
	buffer.pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	buffer.width = width;
	buffer.height = height;
	buffer.size = size;

	wl_buffer_add_listener(buffer.wl_buffer, &wl_buffer_listener, NULL);
	wl_shm_pool_destroy(wl_shm_pool);
}

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
		uint32_t serial)
{
	if (buffer.busy) return;

	xdg_surface_ack_configure(xdg_surface, serial);

	if (buffer.width != app.width || buffer.height != app.height)
		buffer_init(app.width, app.height);

	if (app.on_draw)
		app.on_draw(buffer.pixels, buffer.width, buffer.height);

	wl_surface_attach(surface.wl_surface, buffer.wl_buffer, 0, 0);
	wl_surface_commit(surface.wl_surface);

	buffer.busy = 1; // compositor now has it
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
	if (app.on_close)
		app.on_close();
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

static void frame(void *data, struct wl_callback *wl_callback, uint32_t time)
{

	if (buffer.width != app.width || buffer.height != app.height)
		buffer_init(app.width, app.height);

	if (app.on_draw)
		app.on_draw(buffer.pixels, buffer.width, buffer.height);

	wl_surface_attach(surface.wl_surface, buffer.wl_buffer, 0, 0);
	wl_surface_damage_buffer(surface.wl_surface, 0, 0, buffer.width,
			buffer.height);
	wl_surface_commit(surface.wl_surface);

	buffer.busy = 1;
}

static const struct wl_callback_listener frame_listener = {
	.done = frame,
};

void app_init(int width, int height,
		void (*on_close)(),
		void (*on_key)(uint32_t key),
		void (*on_draw)(uint32_t *pixels, int width, int height))
{
	app.width = width;
	app.height = height;
	app.on_close = on_close;
	app.on_key = on_key;
	app.on_draw = on_draw;

	globals.wl_display = wl_display_connect(NULL);
	globals.wl_registry = wl_display_get_registry(globals.wl_display);
	wl_registry_add_listener(globals.wl_registry, &registry_listener, NULL);
	wl_display_roundtrip(globals.wl_display);

	assert(globals.wl_shm && globals.wl_compositor && globals.wl_seat && globals.xdg_wm_base);

	// Set up surface
	surface.wl_surface = wl_compositor_create_surface(globals.wl_compositor);

	surface.xdg_surface = xdg_wm_base_get_xdg_surface(globals.xdg_wm_base, surface.wl_surface);
	xdg_surface_add_listener(surface.xdg_surface, &xdg_surface_listener, NULL);

	surface.xdg_toplevel = xdg_surface_get_toplevel(surface.xdg_surface);
	xdg_toplevel_add_listener(surface.xdg_toplevel, &xdg_toplevel_listener, NULL);
	xdg_toplevel_set_title(surface.xdg_toplevel, "SHM buffer sample");
	xdg_toplevel_set_app_id(surface.xdg_toplevel, "learnwayland");

	wl_surface_commit(surface.wl_surface);

	// Set up input
	struct wl_keyboard *wl_keyboard = wl_seat_get_keyboard(globals.wl_seat);
	wl_keyboard_add_listener(wl_keyboard, &wl_keyboard_listener, NULL);
}

int app_run()
{
	return wl_display_dispatch(globals.wl_display) != -1;
}

void app_redraw()
{
	struct wl_callback *frame_callback = wl_surface_frame(surface.wl_surface);
	wl_callback_add_listener(frame_callback, &frame_listener, NULL);
	wl_surface_commit(surface.wl_surface);
}
