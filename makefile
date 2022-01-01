DBG_CFLAGS=-g3 -Wall -Wextra -pg
OPT_CFLAGS=-Wall -Wextra -O2
PROFILE_CFLAGS=-pg -fprofile-arcs -Wall -Wextra -O2 -g
MEM_CFLAGS=-Wall -Wextra -O0 -g3
OUTDIR=./build/

hmap: src/hmap.h src/hmap_test.c src/test_helpers.h
	$(CC) $(DBG_CFLAGS) src/hmap_test.c -o $(OUTDIR)/hmap_test

hash_test: src/hash_test.c src/ahash.h
	$(CC) $(OPT_CFLAGS) src/hash_test.c -o $(OUTDIR)/hash_test
	$(OUTDIR)/hash_test

dynarr: src/dynarr_test.c src/test_helpers.h src/dynarr.h 
	$(CC) $(DBG_CFLAGS) src/dynarr_test.c -o $(OUTDIR)/dynarr_test

dynarr_test: dynarr
	$(OUTDIR)/dynarr_test

outdir:
	mkdir -p $(OUTDIR)

clean:
	rm -rf $(OUTDIR)

hmap_test: hmap
	$(OUTDIR)/hmap_test | tee hmap_test.log

mem_test: src/mem.h src/mem_test.c
	$(CC) $(MEM_CFLAGS) src/mem_test.c -o $(OUTDIR)/mem_test
	$(OUTDIR)/mem_test | tee mem_test.txt

hmap_cmp: src/hmap.h src/hmap_cmp.c src/test_helpers.h
	$(CC) $(PROFILE_CFLAGS) stb/stb_ds.h src/hmap_cmp.c -o $(OUTDIR)/hmap_cmp
	git rev-parse --short HEAD > hmap_cmp.txt
	cat /proc/cpuinfo | grep name | uniq >> hmap_cmp.txt
	$(OUTDIR)/hmap_cmp >> hmap_cmp.txt
	cat hmap_cmp.txt
	gprof -l $(OUTDIR)/hmap_cmp gmon.out > hmap_cmp_analysis.txt

hmap_bench: src/hmap.h src/hmap_bench.c src/test_helpers.h
	$(CC) $(PROFILE_CFLAGS) src/hmap_bench.c -o $(OUTDIR)/hmap_bench
	git rev-parse --short HEAD > hmap_bench.txt
	cat /proc/cpuinfo | grep name | uniq >> hmap_bench.txt
	$(OUTDIR)/hmap_bench >> hmap_bench.txt
	cat hmap_bench.txt
	gprof -l $(OUTDIR)/hmap_bench gmon.out > hmap_analysis.txt

tests: dynarr_test hmap_test  hash_test
