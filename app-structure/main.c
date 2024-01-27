#include "app.h"

static int running = 1;
static int offset = 0;

static void on_key(uint32_t key)
{
	if (key == 1) running = 0;
	else {
		offset += key;
		app_redraw();
	}
}

static void on_close()
{
	running = 0;
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
	app_init(256, 256, on_close, on_key, on_draw);

	while (running) {
		app_run();
	}

	return 0;
}
