compile:
	gcc -O3 -o asciifolding asciifolding.c

compile_branchy:
	gcc -O3 -o asciifolding asciifolding.c -DASCIIFOLDING_IS_BRANCHY

run: compile
	./asciifolding

run_branchy: compile_branchy
	./asciifolding
