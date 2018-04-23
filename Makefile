all:
	gcc source.c -o bin
san:
	gcc -fsanitize=address source.c -o bin
