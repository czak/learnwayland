#include <stdio.h>
#include <stdint.h>
#include <wayland-client.h>

#include "display.h"
#include "window.h"
#include "input.h"

const uint32_t KEY_ESC = 1;

const int WIDTH = 512;
const int HEIGHT = 512;

static int running = 1;

static void on_close()
{
	running = 0;
}

static void on_draw(uint32_t *pixels, uint32_t time)
{
	for (int y=0; y<HEIGHT; y++) {
		for (int x=0; x<WIDTH; x++) {
			uint32_t d1 = time / 10;
			uint32_t d2 = time / 5;
			uint8_t r = (x + d2) ^ y;
			uint8_t g = (x + d1) ^ (y + d2);
			uint8_t b = x  ^ (y + d1);
			pixels[y * WIDTH + x] = (r << 16) + (g << 8) + b;
		}
	}
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

  while (running && wl_display_dispatch(display->wl_display) != -1) {
	  // main loop
  }

  destroy_input(input);
  destroy_window(window);
  destroy_display(display);

  return 0;
}
