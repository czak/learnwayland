#include <assert.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <unistd.h>
#include <wayland-client.h>

#include "wlr-layer-shell-unstable-v1.h"
#include "log.h"

static struct {
	struct wl_display *wl_display;
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct wl_compositor *wl_compositor;
	struct wl_seat *wl_seat;
	struct zwlr_layer_shell_v1 *zwlr_layer_shell_v1;
} globals;

static struct {
	struct wl_surface *wl_surface;
	struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1;
} surface;

static struct {
	void (*on_key)(uint32_t key);
	void (*on_draw)(uint32_t *pixels, int width, int height);

	int width;
	int height;

	int running;
} app;

static struct buffer {
	int fd;
	struct wl_buffer *wl_buffer;

	uint32_t *pixels;
	int size;

	int width;
	int height;

	int busy;
} buffers[2];

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

	else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		globals.zwlr_layer_shell_v1 =
				wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 4);
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

	buffer->busy = 0;
}

static const struct wl_buffer_listener wl_buffer_listener = {
	.release = wl_buffer_release,
};

static struct buffer *get_buffer(int width, int height)
{
	struct buffer *buffer = &buffers[0];
	if (buffer->busy) buffer = &buffers[1];
	if (buffer->busy) return NULL;

	// Reuse existing buffer if compatible
	if (buffer->width == width && buffer->height == height)
		return buffer;

	if (buffer->wl_buffer) wl_buffer_destroy(buffer->wl_buffer);
	if (buffer->pixels) munmap(buffer->pixels, buffer->size);

	int stride = width * sizeof(uint32_t);
	int size = stride * height;

	if (buffer->fd == 0)
		buffer->fd = memfd_create("buffer-pool", 0);
	ftruncate(buffer->fd, size);

	struct wl_shm_pool *wl_shm_pool = wl_shm_create_pool(globals.wl_shm, buffer->fd, size);

	buffer->wl_buffer = wl_shm_pool_create_buffer(wl_shm_pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
	buffer->pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, buffer->fd, 0);
	buffer->width = width;
	buffer->height = height;
	buffer->size = size;

	wl_buffer_add_listener(buffer->wl_buffer, &wl_buffer_listener, buffer);
	wl_shm_pool_destroy(wl_shm_pool);

	return buffer;
}

static void frame(void *data, struct wl_callback *wl_callback, uint32_t time)
{
	struct buffer *buffer = get_buffer(app.width, app.height);

	if (!buffer) {
		LOG("All buffers busy");

		return;
	}

	if (app.on_draw)
		app.on_draw(buffer->pixels, buffer->width, buffer->height);

	wl_surface_attach(surface.wl_surface, buffer->wl_buffer, 0, 0);
	wl_surface_damage_buffer(surface.wl_surface, 0, 0, buffer->width,
			buffer->height);
	wl_surface_commit(surface.wl_surface);

	buffer->busy = 1;
}

static void zwlr_layer_surface_v1_configure(void *data,
		struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1, uint32_t serial,
		uint32_t width, uint32_t height)
{
	zwlr_layer_surface_v1_ack_configure(zwlr_layer_surface_v1, serial);

	if (width > 0) app.width = width;
	if (height > 0) app.height = height;

	frame(NULL, NULL, 0);
}

static void zwlr_layer_surface_v1_closed(void *data,
		struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1)
{
	app.running = 0;
}

static const struct zwlr_layer_surface_v1_listener zwlr_layer_surface_v1_listener = {
	.configure = zwlr_layer_surface_v1_configure,
	.closed = zwlr_layer_surface_v1_closed,

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

	assert(globals.wl_shm && globals.wl_compositor && globals.wl_seat &&
			globals.zwlr_layer_shell_v1);

	// Set up surface
	surface.wl_surface = wl_compositor_create_surface(globals.wl_compositor);
	surface.zwlr_layer_surface_v1 = zwlr_layer_shell_v1_get_layer_surface(
			globals.zwlr_layer_shell_v1, surface.wl_surface, NULL,
			ZWLR_LAYER_SHELL_V1_LAYER_TOP, "learnwayland");
	assert(surface.wl_surface && surface.zwlr_layer_surface_v1);
	zwlr_layer_surface_v1_add_listener(surface.zwlr_layer_surface_v1,
			&zwlr_layer_surface_v1_listener, NULL);
	zwlr_layer_surface_v1_set_size(surface.zwlr_layer_surface_v1, 400, 400);
	zwlr_layer_surface_v1_set_anchor(surface.zwlr_layer_surface_v1, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP);
	// zwlr_layer_surface_v1_set_exclusive_zone(surface.zwlr_layer_surface_v1, 100);
	zwlr_layer_surface_v1_set_keyboard_interactivity(surface.zwlr_layer_surface_v1, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND);
	zwlr_layer_surface_v1_set_margin(surface.zwlr_layer_surface_v1, 200, 0, 0, 0);
	wl_surface_commit(surface.wl_surface);

	// Set up input
	struct wl_keyboard *wl_keyboard = wl_seat_get_keyboard(globals.wl_seat);
	wl_keyboard_add_listener(wl_keyboard, &wl_keyboard_listener, NULL);
}

void app_run()
{
	enum {
		WAYLAND,
		TIMER,
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

static void on_key(uint32_t key)
{
	if (key == 1) app.running = 0;
}

static void on_draw(uint32_t *pixels, int width, int height)
{
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint8_t r = ((uint8_t) x ^ (uint8_t) y) * 0.2;
			uint8_t g = ((uint8_t) x ^ (uint8_t) y) * 0.2;
			uint8_t b = ((uint8_t) x ^ (uint8_t) y) * 0.2;
			uint8_t a = 0x4f;
			pixels[y * width + x] = (a << 24) + (r << 16) + (g << 8) + b;
		}
	}
}

int main(int argc, char *argv[])
{
	app_init(256, 256, "WLR layers", "learnwayland", on_key, on_draw);

	app_run();

	return 0;
}
