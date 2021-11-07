CFLAGS := -std=c99 -Wall -Wextra -Wformat=2 -pedantic-errors \
	-Wfatal-errors -Wundef -Wno-unused-result -fno-stack-protector \
	-march=native -Wno-deprecated

LDFLAGS += -lm -lcsfml-graphics -lcsfml-window

all:
	$(CC) src/*.c -Iinc -o raycaster $(CFLAGS) -O2 $(LDFLAGS)
debug:
	$(CC) src/*.c -Iinc -o raycaster $(CFLAGS) -O0 -g $(LDFLAGS)
# valgrind ./raycaster --track-origins=yes
clean:
	$(RM) raycaster *.ppm
