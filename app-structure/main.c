#include "app.h"
#include <stdio.h>

static int offset = 0;

static void on_timer()
{
	offset += 100;
	app_redraw();
}

static void on_key(uint32_t key)
{
	if (key == 1) app_stop();
	else if (key < 10) {
		int n = key - 1;
		fprintf(stderr, "Setting timer every %d seconds.\n", n);
		app_set_timer(n, on_timer);
	}
}

static void on_draw(uint32_t *pixels, int width, int height)
{
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint8_t r = (x + offset) ^ y;
			uint8_t g = (x + offset) ^ y;
			uint8_t b = (x + offset) ^ y;
			uint8_t a = 0x7f;
			pixels[y * width + x] = (a << 24) + (r << 16) + (g << 8) + b;
		}
	}
}

int main(int argc, char *argv[])
{
	app_init(256, 256, "App demo", "learnwayland", on_key, on_draw);

	app_set_timer(3, on_timer);

	app_run();

	return 0;
}
