CC = gcc
CFLAGS = -g -Wall
OUTFILES = objectstoreserver.out objectstorelib.a clientfortest.out
HEADFILES = objectstorelib.h
OBJFILES = objectstorelib.o
THRD = -lpthread

.PHONY: test clean 

all: $(OUTFILES)

objectstoreserver.out: objectstoreserver.c
	$(CC) $(CFLAGS) $< -o $@ $(THRD)

objectstorelib.a: objectstorelib.c
	@echo "creo libreria"
	$(CC) $(CFLAGS) $< -c -o $(OBJFILES)
	ar rvs $@ $(OBJFILES)

clientfortest.out: clientfortest.c
	$(CC) $(CFLAGS) $< -o $@ -L . objectstorelib.a

test:
	@echo "eseguo"
	i=1; while [ "$$i" -le 50 ]; do \
    	./clientfortest.out "client$$i" 1; \
		i=$$((i + 1));\
  	done

clean:
	@echo "cleaning generated files"
	-rm -rf $(OUTFILES) $(OBJFILES) *.txt *.sock ./data

