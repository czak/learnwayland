BIN = sample
OBJ = main.o \
	  display.o \
	  window.o \
	  buffer.o \
	  ../common/shm.o \
	  ../protocols/xdg-shell-protocol.o
LIB = -lwayland-client -lrt

$(BIN): $(OBJ)
	$(CC) -o $@ $^ $(LIB)

.PHONY: clean
clean:
	rm -f $(BIN) $(OBJ)
