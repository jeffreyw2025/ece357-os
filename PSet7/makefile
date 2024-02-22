readwrite: assemble
	ld -m elf_x86_64 readwrite.o

assemble:
	as -o readwrite.o readwrite.S --64

clean:
	rm -f *.exe *.o *.stackdump *~