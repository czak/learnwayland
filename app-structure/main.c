#include "app.h"

void draw(uint32_t *data)
{
	for (int y = 0; y < 256; ++y) {
		for (int x = 0; x < 256; ++x) {
			uint8_t n = x ^ y;
			data[y * 256 + x] = (n << 16) + (n << 8) + n;
		}
	}
}

void on_close(struct app_state *app) {
	app->running = 0;
}

int main(int argc, char *argv[])
{
	struct app_state app = {
		.on_close = on_close,
	};

	app_init(&app);

	while (app_run(&app)) {
		draw(app.buffer.data);
	}

	return 0;
}
