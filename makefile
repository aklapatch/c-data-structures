CFLAGS=-ggdb -Og -Wall -Wextra
OUTDIR=./build/

tests: dynarr hmap

hmap: src/hmap.h src/hmap_test.c src/test_helpers.h
	$(CC) $(CFLAGS) src/hmap_test.c -o $(OUTDIR)/hmap_test

hash_test: src/hash_test.c src/ahash.h
	$(CC) $(CFLAGS) src/hash_test.c -o $(OUTDIR)/hash_test
	$(OUTDIR)/hash_test

dynarr: src/dynarr_test.c src/test_helpers.h src/dynarr.h 
	$(CC) $(CFLAGS) src/dynarr_test.c -o $(OUTDIR)/dynarr_test

dynarr_test: dynarr
	$(OUTDIR)/dynarr_test

j_dist: src/jump_dist_test.c
	$(CC) $(CFLAGS) src/jump_dist_test.c -o $(OUTDIR)/jump_dist_test
	$(OUTDIR)/jump_dist_test


outdir:
	mkdir -p $(OUTDIR)

clean:
	rm -rf $(OUTDIR)

hmap_test: hmap
	$(OUTDIR)/hmap_test

test: tests
	$(OUTDIR)/dynarr_test
	echo "Starting hash_test"
	echo "---------------------------------------------------------------------"
	$(OUTDIR)hash_test
