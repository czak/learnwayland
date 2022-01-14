#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include "../common/shm.h"

#include "display.h"
#include "buffer.h"

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer)
{
	struct buffer *buffer = data;
	buffer->busy = 0;
}

static const struct wl_buffer_listener wl_buffer_listener = {
	.release = wl_buffer_release,
};

struct buffer *create_buffer(struct display *display, int width, int height)
{
	const int stride = width * 4;
	const int size = stride * height;

	struct buffer *buffer = calloc(1, sizeof(*buffer));
	buffer->busy = 0;

	int fd = allocate_shm_file(size);
	struct wl_shm_pool *pool = wl_shm_create_pool(display->wl_shm, fd,  size);

	buffer->wl_buffer = wl_shm_pool_create_buffer(pool, 0,
			width, height, stride, WL_SHM_FORMAT_XRGB8888);
	buffer->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	buffer->size = size;

	wl_buffer_add_listener(buffer->wl_buffer, &wl_buffer_listener, buffer);

	wl_shm_pool_destroy(pool);
	close(fd);

	return buffer;
}

void destroy_buffer(struct buffer *buffer)
{
	if (buffer->wl_buffer)
		wl_buffer_destroy(buffer->wl_buffer);

	if (buffer->data)
		munmap(buffer->data, buffer->size);

	free(buffer);
}
