#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include "../common/shm.h"
#include "../protocols/xdg-shell-client-protocol.h"

struct app_state {
	struct wl_surface *surface;
	struct wl_buffer *buffer;
};

// globals
static struct wl_shm *shm = NULL;
static struct wl_compositor *compositor = NULL;
static struct xdg_wm_base *xdg_wm_base = NULL;

// --- drawing ---

static struct wl_buffer *draw_frame()
{
	const int width = 256, height = 256;
	int stride = width * 4;
	int size = stride * height;

	int fd = allocate_shm_file(size);
	uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0,
			width, height, stride, WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(pool);
	close(fd);

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint8_t n = x ^ y;
			data[y * width + x] = (n << 16) + (n << 8) + n;
		}
	}

	munmap(data, size);

	return buffer;
}


// --- wl_registry callbacks ---

// https://wayland.app/protocols/wayland#wl_registry:event:global
static void registry_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	if (strcmp(interface, wl_shm_interface.name) == 0) {
		shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}

	else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	}

	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
	}
}

// https://wayland.app/protocols/wayland#wl_registry:event:global_remove
static void registry_global_remove(void *data, struct wl_registry *registry,
		uint32_t name)
{
	// do nothing 
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove,
};


// --- xdg_surface callbacks ---

// https://wayland.app/protocols/xdg-shell#xdg_surface:event:configure
static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
		uint32_t serial)
{
	xdg_surface_ack_configure(xdg_surface, serial);
	
	struct app_state *state = data;
	wl_surface_attach(state->surface, state->buffer, 0, 0);
	wl_surface_commit(state->surface);
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};


// ===== MAIN =====

int main(int argc, char *argv[])
{
	// Initialize and bind globals
	struct wl_display *display = wl_display_connect(NULL);
	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);

	// Init state
	struct app_state state = {
		.surface = wl_compositor_create_surface(compositor),
		.buffer = draw_frame(),
	};

	// Give the xdg_toplevel role to the surface
	struct xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, state.surface);
	xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, &state);
	xdg_surface_get_toplevel(xdg_surface);
	wl_surface_commit(state.surface);

	while (wl_display_dispatch(display) != -1) {
	}

	return 0;
}
