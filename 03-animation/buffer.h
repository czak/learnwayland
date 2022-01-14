#ifndef BUFFER_H
#define BUFFER_H

struct buffer {
	struct wl_buffer *wl_buffer;
	void *data;
};

struct buffer *create_buffer(struct display *display, int width, int height);
void destroy_buffer(struct buffer *buffer);

#endif
