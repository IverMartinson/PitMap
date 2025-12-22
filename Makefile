COMPILER=gcc
FLAGS_ALL=-g -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-sequence-point -O0
FLAGS_EXAMPLE=-Lbuilds/ -lpitmap -Wl,-rpath=builds/
FLAGS_LIB=-lm -shared -fPIC

main.bin: pitmap.so
	$(COMPILER) $(FLAGS_ALL) src/launch_program/main.c -o builds/main.bin $(FLAGS_EXAMPLE) 

pitmap.so:
	$(COMPILER) $(FLAGS_ALL) src/library/main.c -o builds/libpitmap.so $(FLAGS_LIB) 

clean:
	rm build/*
