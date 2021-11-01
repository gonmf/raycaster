all:
	$(CC) main.c -o raycaster -Wall -lm -lcsfml-graphics -lcsfml-window
clean:
	$(RM) raycaster
