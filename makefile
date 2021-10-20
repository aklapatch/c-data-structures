CFLAGS=-ggdb -Og -Wall -Wextra
OUTDIR=./build/

tests: dynarr hmap

hmap: src/hmap.h src/hmap_test.c src/test_helpers.h
	$(CC) $(CFLAGS) src/hmap_test.c -o $(OUTDIR)/hmap_test

hash_test: src/hash_test.c src/ahash.h
	$(CC) $(CFLAGS) src/hash_test.c -o $(OUTDIR)/hash_test

dynarr: src/test_main.c src/test_helpers.h src/dynarr.h src/alloc_fail_tests.c
	$(CC) $(CFLAGS) src/test_main.c -o $(OUTDIR)/test
	$(CC) $(CFLAGS) src/alloc_fail_tests.c -o $(OUTDIR)/alloc_fail_tests


outdir:
	mkdir -p $(OUTDIR)

clean:
	rm -rf $(OUTDIR)

hmap_test: hmap
	$(OUTDIR)/hmap_test

test: tests
	$(OUTDIR)/test
	echo "Starting allocation failure tests!"
	echo "---------------------------------------------------------------------"
	$(OUTDIR)/alloc_fail_tests
	echo "Starting hash_test"
	echo "---------------------------------------------------------------------"
	$(OUTDIR)hash_test
