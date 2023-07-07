compile_branchless:
	gcc -march=native -O3 -o asciifolding asciifolding.c

compile_branchy:
	gcc -march=native -O3 -o asciifolding asciifolding.c -DASCIIFOLDING_IS_BRANCHY -DASCIIFOLDING_USE_SIMD

run_branchless: compile_branchless
	./asciifolding

run_branchy: compile_branchy
	./asciifolding
