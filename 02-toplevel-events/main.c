#include "app.h"

void draw(uint32_t *data, int offset)
{
	for (int y = 0; y < 256; ++y) {
		for (int x = 0; x < 256; ++x) {
			uint8_t n = (x + offset) ^ y;
			data[y * 256 + x] = (n << 16) + (n << 8) + n;
		}
	}
}

int main(int argc, char *argv[])
{
	struct app_state app = {0};
	int frame = 0;

	app_init(&app);

	while (app_run(&app)) {
		draw(app.buffer.data, ++frame);	
	}

	return 0;
}
