GCC_OPTIONS=-std=c99 -march=native -O3

build_tape:
	wget https://github.com/apache/lucene/blob/main/lucene/analysis/common/src/java/org/apache/lucene/analysis/miscellaneous/ASCIIFoldingFilter.java \
	&& python gen_tape.py > asciifolding_tape.h \
	&& rm ASCIIFoldingFilter.java

compile_branchless:
	gcc ${GCC_OPTIONS} -o asciifolding asciifolding.c

compile_branchy:
	gcc ${GCC_OPTIONS} -o asciifolding asciifolding.c -DASCIIFOLDING_IS_BRANCHY

compile_branchy_simd:
	gcc ${GCC_OPTIONS} -o asciifolding asciifolding.c -DASCIIFOLDING_IS_BRANCHY -DASCIIFOLDING_USE_SIMD

run_branchless: compile_branchless
	./asciifolding

run_branchy: compile_branchy
	./asciifolding

run_branchy_simd: compile_branchy_simd
	./asciifolding
