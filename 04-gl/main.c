#include <stdio.h>
#include <stdint.h>
#include <wayland-client.h>
#include <EGL/egl.h>
#include <GL/gl.h>

#include "display.h"
#include "window.h"
#include "input.h"

const uint32_t KEY_ESC = 1;

const int WIDTH = 512;
const int HEIGHT = 512;

static int running = 1;

static int start_time = 0;
static int end_time = 0;
static int frames = 0;

static void on_close()
{
	running = 0;
}

static void on_draw(uint32_t time)
{
	float f = (time % 2000) / 3000.f;

	glClearColor(f, f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Count frames
	if (!start_time) start_time = time;
	end_time = time;
	++frames;

}

static void on_key(uint32_t key)
{
	if (key == KEY_ESC) {
		running = 0;
	}
}


int main(int argc, char **argv)
{
	struct display *display;
	struct window *window;
	struct input *input;

	display = create_display();
	window = create_window(display, WIDTH, HEIGHT, on_draw, on_close);
	input = create_input(display, on_key);

	int ret;
	while (running && ret != -1) {
		ret = wl_display_dispatch(display->wl_display);
	}

	float duration = (end_time - start_time) / 1000.0f;
	fprintf(stderr, "%d frames in %f s = %f FPS", frames, duration, frames / duration);

	destroy_input(input);
	destroy_window(window);
	destroy_display(display);

	return 0;
}
