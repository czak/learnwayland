BIN = sample
OBJ = init_window.o \
			../protocols/xdg-shell-protocol.o
LIB = -lwayland-client -lwayland-egl -lrt -lEGL -lGLESv2

$(BIN): $(OBJ)
	$(CC) -o $@ $^ $(LIB)

.PHONY: clean
clean:
	rm -f $(BIN) $(OBJ)
