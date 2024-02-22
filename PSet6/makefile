ftest:
	gcc -I. -o ftest.exe ftest.c fifo.c fifo.h spinlock.c spinlock.h tas.S

spintest:
	gcc -I. -o spintest.exe spintest.c spinlock.c spinlock.h tas.S

clean:
	rm -f *.exe *.o *.stackdump *~

backup:
	test -d backups || mkdir backups
	cp *.cpp backups
	cp *.h backups
	cp Makefile backups