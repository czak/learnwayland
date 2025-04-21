#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-egl.h>

#include <EGL/egl.h>
#include <GL/gl.h>

#include "xdg-shell.h"

static struct wl_compositor *compositor = NULL;
static struct xdg_wm_base *xdg_wm_base = NULL;

static int configured = 0;

// https://wayland.app/protocols/wayland#wl_registry:event:global
static void registry_global(void *data, struct wl_registry *registry, uint32_t name,
		const char *interface, uint32_t version)
{
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	}

	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
	}
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
	xdg_surface_ack_configure(xdg_surface, serial);

	configured = 1;
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

int main()
{
	struct wl_display *display = wl_display_connect(NULL);
	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);

	EGLConfig egl_config;
	EGLint attributes[] = {EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_NONE};
	int n;

	struct wl_surface *wl_surface = wl_compositor_create_surface(compositor);
	struct xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, wl_surface);
	xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
	struct xdg_toplevel *xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);

	xdg_toplevel_set_min_size(xdg_toplevel, 512, 512);
	xdg_toplevel_set_max_size(xdg_toplevel, 512, 512);

	EGLDisplay egl_display = eglGetDisplay(display);
	eglInitialize(egl_display, NULL, NULL);
	eglBindAPI(EGL_OPENGL_API);
	eglChooseConfig(egl_display, attributes, &egl_config, 1, &n);
	EGLContext egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, NULL);

	struct wl_egl_window *egl_window = wl_egl_window_create(wl_surface, 512, 512);
	EGLSurface egl_surface = eglCreateWindowSurface(egl_display, egl_config, egl_window, NULL);
	int ret = eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
	assert(ret == EGL_TRUE);

	wl_surface_commit(wl_surface);
	while (!configured)
		wl_display_dispatch(display);

	int frame = 0;
	while (1) {
		wl_display_dispatch_pending(display);

		float f = (++frame % 200) / 300.f;

		glClearColor(f, f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		eglSwapBuffers(egl_display, egl_surface);
	}
}
