#include <wayland-client.h>

#include "display.h"
#include "window.h"

int main(int argc, char **argv)
{
  struct display *display;
  struct window *window;

  display = create_display();
  window = create_window(display, 256, 256);

  while (wl_display_dispatch(display->wl_display) != -1) {
    // main loop
  }

  destroy_window(window);
  destroy_display(display);

  return 0;
}
