all: p1 p2 

p1: src/png.c
	gcc -Wall -Wno-unused -Wno-uninitialized -ansi -lGL -lGLU -lglut `libpng12-config --cflags --libs` -lpng -c src/png.c -o png
p2: src/car_world.c
	gcc -Wall -Wno-unused -Wno-uninitialized -I/usr/include/   -c -o car_world.o src/car_world.c
	gcc -Wall -Wno-unused -I/usr/include/ -o car_world -L/usr/X11R6/lib  car_world.o -lX11 -lXi -l glut -lGL -lGLU -lm `libpng12-config --cflags --libs` -lpng -pthread
	rm car_world.o

