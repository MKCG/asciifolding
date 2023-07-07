build_tape:
	wget https://github.com/apache/lucene/blob/main/lucene/analysis/common/src/java/org/apache/lucene/analysis/miscellaneous/ASCIIFoldingFilter.java \
	&& python gen_tape.py > asciifolding_tape.h \
	&& rm ASCIIFoldingFilter.java

compile_branchless: build_tape
	gcc -march=native -O3 -o asciifolding asciifolding.c

compile_branchy: build_tape
	gcc -march=native -O3 -o asciifolding asciifolding.c -DASCIIFOLDING_IS_BRANCHY -DASCIIFOLDING_USE_SIMD

run_branchless: compile_branchless
	./asciifolding

run_branchy: compile_branchy
	./asciifolding
