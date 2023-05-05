all:
	gcc fuse.c -o fuse -D_FILE_OFFSET_BITS=64 -static -pthread -lfuse -ldl
	gcc -o exp exp.c -lcap
	gcc -o gc getshell.c

clean:
	rm -rf exp gc fuse