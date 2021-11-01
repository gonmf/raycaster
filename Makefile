CFLAGS := -std=c99 -O2 -Wall -Wextra -Wformat=2 -pedantic-errors \
	-Wfatal-errors -Wundef -Wno-unused-result -fno-stack-protector \
	-march=native

LDFLAGS += -lm -lcsfml-graphics -lcsfml-window

all:
	$(CC) main.c -o raycaster $(CFLAGS) $(LDFLAGS)
clean:
	$(RM) raycaster
