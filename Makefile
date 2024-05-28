CFLAGS += `pkg-config --cflags gio-2.0`
LDFLAGS += `pkg-config --libs gio-2.0`

main: main.c
	gcc main.c -o alram $(CFLAGS) $(LDFLAGS)