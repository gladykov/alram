CFLAGS += `pkg-config --cflags libnotify`
LDFLAGS += `pkg-config --libs libnotify`

main: main.c
	gcc main.c -o main $(CFLAGS) $(LDFLAGS)