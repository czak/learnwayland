TARGETS = \
	xdg-shell-client-protocol.h \
	xdg-shell-protocol.c \
	xdg-decoration-client-protocol.h \
	xdg-decoration-protocol.c

all: $(TARGETS)

xdg-shell-client-protocol.h:
	wayland-scanner client-header \
		< /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
		> xdg-shell-client-protocol.h

xdg-shell-protocol.c:
	wayland-scanner private-code \
		< /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
		> xdg-shell-protocol.c

xdg-decoration-client-protocol.h:
	wayland-scanner client-header \
		< /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml \
		> xdg-decoration-client-protocol.h

xdg-decoration-protocol.c:
	wayland-scanner private-code \
		< /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml \
		> xdg-decoration-protocol.c

.PHONY: clean
clean:
	rm -f $(TARGETS)
