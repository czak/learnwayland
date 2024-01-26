#pragma once

#include <stdint.h>

void app_init(int width, int height,
		void (*on_close)(),
		void (*on_key)(uint32_t key),
		void (*on_draw)(uint32_t *pixels, int width, int height));

int app_run();
