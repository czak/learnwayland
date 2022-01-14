#include <stdint.h>
#include <wayland-client.h>

#include "display.h"
#include "window.h"
#include "input.h"

const uint32_t KEY_ESC = 1;

static int running = 1;

static void on_close()
{
	running = 0;
}

static void on_draw(uint32_t *pixels)
{
	for (int y=0; y<256; y++) {
		for (int x=0; x<256; x++) {
			uint8_t n = x ^ y;
			pixels[y * 256 + x] = (n << 16) + (n << 8) + n;
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
  window = create_window(display, 256, 256, on_draw, on_close);
  input = create_input(display, on_key);

  while (running && wl_display_dispatch(display->wl_display) != -1) {
    // main loop
  }

  destroy_input(input);
  destroy_window(window);
  destroy_display(display);

  return 0;
}
