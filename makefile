CFLAGS=-ggdb -Os -Wall -Wextra
OUTDIR=./build/

tests: src/test_main.c src/dynarr.h src/alloc_fail_tests.c src/hmap.h src/hash_test.c outdir
	$(CC) $(CFLAGS) src/test_main.c -o $(OUTDIR)/test
	$(CC) $(CFLAGS) src/alloc_fail_tests.c -o $(OUTDIR)/alloc_fail_tests
	$(CC) $(CFLAGS) src/hash_test.c -o $(OUTDIR)/hash_test

outdir:
	mkdir -p $(OUTDIR)

clean:
	rm -rf $(OUTDIR)

test: tests
	$(OUTDIR)/test
	echo "Starting allocation failure tests!"
	echo "---------------------------------------------------------------------"
	$(OUTDIR)/alloc_fail_tests
	echo "Starting hash_test"
	echo "---------------------------------------------------------------------"
	$(OUTDIR)hash_test
