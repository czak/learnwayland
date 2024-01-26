#include "app.h"

static void on_key(struct app_state *app, uint32_t key)
{
	if (key == 1)
		app->running = 0;
}

static void on_close(struct app_state *app)
{
	app->running = 0;
}

int main(int argc, char *argv[])
{
	struct app_state app = {
		.width = 256,
		.height = 256,
		.on_close = on_close,
		.on_key = on_key,
	};

	app_init(&app);

	while (app_run(&app)) {}

	return 0;
}
