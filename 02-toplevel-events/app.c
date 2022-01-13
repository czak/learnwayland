#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include "../common/shm.h"
#include "../protocols/xdg-shell-client-protocol.h"

#include "app.h"

// --- no-op callback ---
static void noop() {}


// --- wl_registry callbacks ---

static void registry_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	struct app_state *app = data;

	if (strcmp(interface, wl_shm_interface.name) == 0) {
		app->wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}

	else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		app->wl_compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	}

	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		app->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
	}
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = noop,
};


// --- xdg_surface callbacks ---

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
		uint32_t serial)
{
	struct app_state *app = data;

	xdg_surface_ack_configure(xdg_surface, serial);
	
	wl_surface_attach(app->wl_surface, app->buffer.wl_buffer, 0, 0);
	wl_surface_commit(app->wl_surface);
}


static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};


// --- xdg_toplevel callbacks ---

static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	struct app_state *app = data;

	app->running = 0;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = noop,
	.close = xdg_toplevel_close,
};


// --- PRIVATE ---

void buffer_init(struct app_state *app, int width, int height)
{
	const int stride = width * 4;
	const int size = stride * height;

	int fd = allocate_shm_file(size);

	struct wl_shm_pool *pool = wl_shm_create_pool(app->wl_shm, fd,  size);
	
	app->buffer.wl_buffer = wl_shm_pool_create_buffer(pool, 0,
			width, height, stride, WL_SHM_FORMAT_XRGB8888);
	app->buffer.data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	wl_shm_pool_destroy(pool);
	close(fd);
}


// --- PUBLIC ---

void app_init(struct app_state *app)
{
	// Connect & bind globals
	app->wl_display = wl_display_connect(NULL);
	app->wl_registry = wl_display_get_registry(app->wl_display);
	wl_registry_add_listener(app->wl_registry, &registry_listener, app);
	wl_display_roundtrip(app->wl_display);

	buffer_init(app, 256, 256);

	// Create surface and give it xdg_toplevel role
	app->wl_surface = wl_compositor_create_surface(app->wl_compositor);
	app->xdg_surface = xdg_wm_base_get_xdg_surface(app->xdg_wm_base, app->wl_surface);
	app->xdg_toplevel = xdg_surface_get_toplevel(app->xdg_surface);

	// Bind XDG surface listeners
	xdg_surface_add_listener(app->xdg_surface, &xdg_surface_listener, app);
	xdg_toplevel_add_listener(app->xdg_toplevel, &xdg_toplevel_listener, app);
	wl_surface_commit(app->wl_surface);

	app->running = 1;
}

int app_run(struct app_state *app)
{
	return app->running && wl_display_dispatch(app->wl_display) != -1;
}
