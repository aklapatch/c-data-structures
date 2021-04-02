CFLAGS=-ggdb -Os -Wall -Wextra
tests: src/test_main.c src/dynarr.h src/alloc_fail_tests.c
	$(CC) $(CFLAGS) src/test_main.c -o test
	$(CC) $(CFLAGS) src/alloc_fail_tests.c -o alloc_fail_tests

run: tests
	./test
	echo "Starting allocation failure tests!"
	echo "---------------------------------------------------------------------"
	./alloc_fail_tests

valgrind: tests
	valgrind test
