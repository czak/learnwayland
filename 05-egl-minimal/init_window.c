#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include <wayland-client.h>
#include <wayland-egl.h> // Wayland EGL MUST be included before EGL headers
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "../protocols/xdg-shell-client-protocol.h"

#if defined(DEBUG)
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif

struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct wl_egl_window *egl_window;

struct xdg_wm_base *XDGWMBase;
struct xdg_surface *XDGSurface;
struct xdg_toplevel *XDGToplevel;


struct _escontext
{
	/// Native System informations
	EGLNativeDisplayType native_display;
	EGLNativeWindowType native_window;
	uint16_t window_width, window_height;
	/// EGL display
	EGLDisplay  display;
	/// EGL context
	EGLContext  context;
	/// EGL surface
	EGLSurface  surface;
};

struct _escontext ESContext = {
	.native_display = NULL,
	.window_width = 0,
	.window_height = 0,
	.native_window  = 0,
	.display = NULL,
	.context = NULL,
	.surface = NULL
};

#define TRUE 1
#define FALSE 0

#define WINDOW_WIDTH 256
#define WINDOW_HEIGHT 256

bool program_alive;

static void noop() {}

static void xdg_toplevel_handle_close(void *data,
		struct xdg_toplevel *xdg_toplevel) {
	// window closed, be sure that this event gets processed
	program_alive = false;
}

struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = noop,
	.close = xdg_toplevel_handle_close,
};


static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
		uint32_t serial) {
	xdg_surface_ack_configure(xdg_surface, serial);
}

const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};


void CreateNativeWindow(char *title, int width, int height) {
	struct wl_egl_window *egl_window = 
		wl_egl_window_create(surface, width, height);

	if (egl_window == EGL_NO_SURFACE) {
		LOG("No window !?\n");
		exit(1);
	}
	else LOG("Window created !\n");
	ESContext.window_width = width;
	ESContext.window_height = height;
	ESContext.native_window = egl_window;
}

EGLBoolean CreateEGLContext ()
{
	EGLint numConfigs;
	EGLint majorVersion;
	EGLint minorVersion;
	EGLContext context;
	EGLSurface surface;
	EGLConfig config;
	EGLint fbAttribs[] =
	{
		EGL_RED_SIZE,        8,
		EGL_GREEN_SIZE,      8,
		EGL_BLUE_SIZE,       8,
		EGL_NONE
	};
	EGLDisplay display = eglGetDisplay( ESContext.native_display );
	if ( display == EGL_NO_DISPLAY )
	{
		LOG("No EGL Display...\n");
		return EGL_FALSE;
	}

	// Initialize EGL
	if ( !eglInitialize(display, &majorVersion, &minorVersion) )
	{
		LOG("No Initialisation...\n");
		return EGL_FALSE;
	}

	// Choose config
	if ( (eglChooseConfig(display, fbAttribs, &config, 1, &numConfigs) != EGL_TRUE) || (numConfigs != 1))
	{
		LOG("No configuration...\n");
		return EGL_FALSE;
	}

	// Create a surface
	surface = eglCreateWindowSurface(display, config, ESContext.native_window, NULL);
	if ( surface == EGL_NO_SURFACE )
	{
		LOG("No surface...\n");
		return EGL_FALSE;
	}

	// Create a GL context
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL );
	if ( context == EGL_NO_CONTEXT )
	{
		LOG("No context...\n");
		return EGL_FALSE;
	}

	// Make the context current
	if ( !eglMakeCurrent(display, surface, surface, context) )
	{
		LOG("Could not make the current window current !\n");
		return EGL_FALSE;
	}

	ESContext.display = display;
	ESContext.surface = surface;
	ESContext.context = context;
	return EGL_TRUE;
}

void draw() {
	glClearColor(0.5, 0.3, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
}

static void global_registry_handler
(void *data, struct wl_registry *registry, uint32_t id,
 const char *interface, uint32_t version) {
	LOG("Got a registry event for %s id %d\n", interface, id);
	if (strcmp(interface, "wl_compositor") == 0)
		compositor = 
			wl_registry_bind(registry, id, &wl_compositor_interface, 1);
	else if(strcmp(interface, xdg_wm_base_interface.name) == 0) {
		XDGWMBase = wl_registry_bind(registry, id,
				&xdg_wm_base_interface, 1);
	} 
}

const struct wl_registry_listener wl_registry_listener = {
	.global = global_registry_handler,
	.global_remove = noop,
};

static void
get_server_references() {
	struct wl_display * display = wl_display_connect(NULL);
	struct wl_registry *wl_registry = wl_display_get_registry(display);
	wl_registry_add_listener(wl_registry, &wl_registry_listener, NULL);
	wl_display_roundtrip(display);

	// If at this point, global_registry_handler didn't set the 
	// compositor, nor the shell, bailout !
	if (compositor == NULL || XDGWMBase == NULL) {
		LOG("No compositor !? No XDG !! There's NOTHING in here !\n");
		exit(1);
	}
	else {
		LOG("Okay, we got a compositor and a shell... That's something !\n");
		ESContext.native_display = display;
	}
}

void destroy_window() {
	eglDestroySurface(ESContext.display, ESContext.surface);
	wl_egl_window_destroy(ESContext.native_window);
	xdg_toplevel_destroy(XDGToplevel);
	xdg_surface_destroy(XDGSurface);
	wl_surface_destroy(surface);
	eglDestroyContext(ESContext.display, ESContext.context);
}

int main() {
	get_server_references();

	surface = wl_compositor_create_surface(compositor);
	if (surface == NULL) {
		LOG("No Compositor surface ! Yay....\n");
		exit(1);
	}
	else LOG("Got a compositor surface !\n");

	XDGSurface = xdg_wm_base_get_xdg_surface(XDGWMBase, surface);

	xdg_surface_add_listener(XDGSurface, &xdg_surface_listener, NULL);

	XDGToplevel = xdg_surface_get_toplevel(XDGSurface);
	xdg_toplevel_set_title(XDGToplevel, "Wayland EGL example");
	xdg_toplevel_add_listener(XDGToplevel, &xdg_toplevel_listener, NULL);
	xdg_toplevel_set_min_size(XDGToplevel, WINDOW_WIDTH, WINDOW_HEIGHT);
	xdg_toplevel_set_max_size(XDGToplevel, WINDOW_WIDTH, WINDOW_HEIGHT);

	wl_surface_commit(surface);

	CreateNativeWindow("Test", WINDOW_WIDTH, WINDOW_HEIGHT);
	CreateEGLContext();

	program_alive = true;

	while (program_alive) {
		wl_display_dispatch_pending(ESContext.native_display);
		draw();
		eglSwapBuffers(ESContext.display, ESContext.surface);
	}

	destroy_window();
	wl_display_disconnect(ESContext.native_display);
	LOG("Display disconnected !\n");

	exit(0);
}

