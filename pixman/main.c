#include <assert.h>
#include <pixman.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <unistd.h>
#include <wayland-client.h>

#include "../protocols/single-pixel-buffer-v1.h"
#include "../protocols/viewporter.h"
#include "../protocols/xdg-decoration-unstable-v1.h"
#include "../protocols/xdg-shell.h"

struct buffer {
	struct wl_buffer *wl_buffer;
	uint32_t *pixels;
	pixman_image_t *pixman_image;
	int size;
};

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
	bool running;

	int width;
	int height;
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

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer)
{
	struct buffer *buffer = data;

	wl_buffer_destroy(buffer->wl_buffer);
	pixman_image_unref(buffer->pixman_image);
	munmap(buffer->pixels, buffer->size);
	free(buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
	.release = wl_buffer_release,
};

struct buffer *create_buffer(struct app_state *app, int width, int height)
{
	int size = width * height * 4;
	int stride = width * 4;

	int fd = memfd_create("buffer-pool", 0);
	ftruncate(fd, size);

	struct wl_shm_pool *wl_shm_pool = wl_shm_create_pool(app->wl_shm, fd, size);

	struct buffer *buffer = calloc(1, sizeof(*buffer));
	buffer->size = size;
	buffer->pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	buffer->pixman_image = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8,
			width, height, buffer->pixels, stride);
	buffer->wl_buffer = wl_shm_pool_create_buffer(wl_shm_pool, 0, width, height,
			stride, WL_SHM_FORMAT_ARGB8888);
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

	// Fill xor pattern by hand
	for (int y = 0; y < app->height; y++) {
		for (int x = 0; x < app->width; x++) {
			uint8_t r = x ^ y;
			uint8_t g = x ^ y;
			uint8_t b = x ^ y;
			uint8_t a = 0x7f;
			buffer->pixels[y * app->width + x] = (a << 24) + (r << 16) +
												 (g << 8) + b;
		}
	}

	// Left half solid fill
	pixman_image_t *fill_img = pixman_image_create_solid_fill(
			&((pixman_color_t){
				.red = random() & 0x7fff,
				.green = random() & 0x7fff,
				.blue = random() & 0x7fff,
				.alpha = 0xffff,
			}));
	pixman_image_composite(PIXMAN_OP_COLOR_BURN, fill_img, NULL, buffer->pixman_image,
			0, 0, 0, 0, 0, 0, app->width / 2, app->height);
	pixman_image_unref(fill_img);

	// Right half gradient	
	pixman_point_fixed_t p1 = { 0, 0 };
	pixman_point_fixed_t p2 = { pixman_int_to_fixed(app->width / 2), pixman_int_to_fixed(app->height) };
	pixman_gradient_stop_t stops[] = {
		{ .x = pixman_int_to_fixed(0), .color = { 0xffff, 0x0000, 0xffff, 0xffff } },
		{ .x = pixman_int_to_fixed(1), .color = { 0x0000, 0xffff, 0xffff, 0xffff } },
	};
	pixman_image_t *gradient_img = pixman_image_create_linear_gradient(&p1, &p2, stops, 2);
	pixman_image_composite(PIXMAN_OP_SATURATE, gradient_img, NULL, buffer->pixman_image,
			0, 0, 0, 0, app->width / 2, 0, app->width / 2, app->height);
	pixman_image_unref(gradient_img);

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

void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	struct app_state *app = data;

	app->running = false;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = xdg_toplevel_configure,
	.close = xdg_toplevel_close,
	.configure_bounds = noop,
	.wm_capabilities = noop,
};

void app_init(struct app_state *app)
{
	app->wl_display = wl_display_connect(NULL);
	app->wl_registry = wl_display_get_registry(app->wl_display);
	wl_registry_add_listener(app->wl_registry, &registry_listener, app);
	wl_display_roundtrip(app->wl_display);

	assert(app->wl_shm && app->wl_compositor && app->xdg_wm_base &&
			app->wp_viewporter && app->wp_single_pixel_buffer_manager_v1 &&
			app->zxdg_decoration_manager_v1);

	// Set up surface
	app->wl_surface = wl_compositor_create_surface(app->wl_compositor);

	app->xdg_surface = xdg_wm_base_get_xdg_surface(app->xdg_wm_base,
			app->wl_surface);
	xdg_surface_add_listener(app->xdg_surface, &xdg_surface_listener, app);

	app->xdg_toplevel = xdg_surface_get_toplevel(app->xdg_surface);
	xdg_toplevel_add_listener(app->xdg_toplevel, &xdg_toplevel_listener, app);
	xdg_toplevel_set_title(app->xdg_toplevel, "SHM buffer sample");
	xdg_toplevel_set_app_id(app->xdg_toplevel, "learnwayland");

	app->wp_viewport = wp_viewporter_get_viewport(app->wp_viewporter,
			app->wl_surface);

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
		.running = true,
		.width = 256,
		.height = 256,
	};

	app_init(&app);

	// Main loop
	while (wl_display_dispatch(app.wl_display) != -1 && app.running) {
	}

	return 0;
}
