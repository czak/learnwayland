#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include "../common/shm.h"

#include "display.h"
#include "buffer.h"

struct buffer *create_buffer(struct display *display, int width, int height)
{
	const int stride = width * 4;
	const int size = stride * height;

	struct buffer *buffer = calloc(1, sizeof(*buffer));

	int fd = allocate_shm_file(size);

	struct wl_shm_pool *pool = wl_shm_create_pool(display->wl_shm, fd,  size);

	buffer->wl_buffer = wl_shm_pool_create_buffer(pool, 0,
			width, height, stride, WL_SHM_FORMAT_XRGB8888);
	buffer->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	wl_shm_pool_destroy(pool);
	close(fd);

	return buffer;
}

void destroy_buffer(struct buffer *buffer)
{
	if (buffer->wl_buffer)
		wl_buffer_destroy(buffer->wl_buffer);

	free(buffer);
}
