#include "app.h"

static int running = 1;

static void on_key(uint32_t key)
{
	if (key == 1) running = 0;
}

static void on_close()
{
	running = 0;
}

int main(int argc, char *argv[])
{
	app_init(256, 256, on_close, on_key);

	while (running && app_run()) {}

	return 0;
}
