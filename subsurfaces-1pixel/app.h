#pragma once

#include <stdint.h>

void app_init(int width, int height,
		const char *title,
		const char *app_id,
		void (*on_key)(uint32_t key),
		void (*on_draw)(uint32_t *pixels, int width, int height));

void app_run();

void app_stop();
