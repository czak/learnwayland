#include <stdint.h>
#include <stdio.h>
#include <wayland-client.h>

#include "display.h"
#include "window.h"

static void draw(uint32_t *pixels)
{
	for (int y=0; y<256; y++) {
		for (int x=0; x<256; x++) {
			uint8_t n = x ^ y;
			pixels[y * 256 + x] = (n << 16) + (n << 8) + n;
		}
	}
}

int main(int argc, char **argv)
{
  struct display *display;
  struct window *window;

  display = create_display();
  window = create_window(display, 256, 256, draw);

  while (wl_display_dispatch(display->wl_display) != -1) {
    // main loop
  }

  destroy_window(window);
  destroy_display(display);

  return 0;
}
