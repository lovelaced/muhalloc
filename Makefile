mem: mem.c mem.h
	gcc -c -g -Wall -m32 -fpic mem.c -O
	gcc -shared -g -Wall -m32 -o libmem.so mem.o -O

clean:
	rm -rf mem.o libmem.so
