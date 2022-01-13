TARGETS = \
	xdg-shell-client-protocol.h \
	xdg-shell-protocol.c

all: $(TARGETS)

xdg-shell-client-protocol.h:
	wayland-scanner client-header \
		< /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
		> xdg-shell-client-protocol.h

xdg-shell-protocol.c:
	wayland-scanner private-code \
		< /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
		> xdg-shell-protocol.c

.PHONY: clean
clean:
	rm -f $(TARGETS)
