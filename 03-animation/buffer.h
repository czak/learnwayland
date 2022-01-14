#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

struct buffer {
	struct wl_buffer *wl_buffer;
	void *data;
	size_t size;
	int busy;
};

struct buffer *create_buffer(struct display *display, int width, int height);
void destroy_buffer(struct buffer *buffer);

#endif
