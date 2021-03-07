all:
	$(CC) -ggdb src/test_main.c -o test

run: all
	./test

valgrind: all
	valgrind test
