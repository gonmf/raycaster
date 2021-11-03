CFLAGS := -std=c99 -O2 -Wall -Wextra -Wformat=2 -pedantic-errors \
	-Wfatal-errors -Wundef -Wno-unused-result -fno-stack-protector \
	-march=native -Wno-deprecated

LDFLAGS += -lm -lcsfml-graphics -lcsfml-window

all:
	$(CC) *.c -o raycaster $(CFLAGS) $(LDFLAGS)
clean:
	$(RM) raycaster *.ppm
