#include "app.h"

int main(int argc, char *argv[])
{
	struct app_state app = {
		.width = 256,
		.height = 256,
	};

	app_init(&app);

	while (app_run(&app)) {}

	return 0;
}
