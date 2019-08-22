CC = gcc
CFLAGS = -g -pedantic -Wall -O3
OUTFILES = rwn.a objectstoreserver.out objectstorelib.a clientfortest.out
HEADFILES = objectstorelib.h
OBJFILES = objectstorelib.o
THRD = -lpthread

.PHONY: test clean 

all: $(OUTFILES)

rwn.a: rwn.c
	@echo "creo libreria rwn"
	$(CC) $(CFLAGS) $< -c -o $(OBJFILES)
	ar rvs $@ $(OBJFILES)

objectstoreserver.out: objectstoreserver.c
	$(CC) $(CFLAGS) $< -o $@ $(THRD) -L . rwn.a

objectstorelib.a: objectstorelib.c
	@echo "creo libreria objst"
	$(CC) $(CFLAGS) $< -c -o $(OBJFILES)
	ar rvs $@ $(OBJFILES)

clientfortest.out: clientfortest.c
	$(CC) $(CFLAGS) $< -o $@ -L . objectstorelib.a rwn.a

test:
	@echo "eseguo"
	gnome-terminal -- valgrind --leak-check=full ./objectstoreserver.out
	(i=1; while [ "$$i" -le 50 ]; do (valgrind --leak-check=full --log-file="vagrindcl$$i.log" ./clientfortest.out "client$$i" 3 &) 1>>"testout$$i.log" ; i=$$((i + 1)); done)

clean:
	@echo "cleaning generated files"
	-rm -rf $(OUTFILES) $(OBJFILES) *.txt *.sock ./data *.log
