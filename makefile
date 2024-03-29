# Makefile
# Raffaele Apetino - Matricola 549220 (r.apetino@studenti.unipi.it)

SHELL := /bin/bash
CC = gcc
CFLAGS = -g -pedantic -Wall -O3 -std=c99 -D_POSIX_C_SOURCE=200809L
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
	declare -a pids && \
	i=1; while [ "$$i" -le 50 ]; do \
	echo "Lancio client$$i TEST 1" >>"testout.log"; \
	./clientfortest.out "client$$i" 1 1>>"testout.log" & pids[$$i]=$$!; \
	i=$$((i + 1)); \
	done && \
	i=1; while [ "$$i" -le 50 ]; do \
	echo $$i $${pids[$$i]}; \
	wait $${pids[$$i]}; \
	i=$$((i + 1)); \
	done
	@echo "done"
	@echo "TEST 1 TERMINATO"
	(i=1; while [ "$$i" -le 30 ]; do \
	echo "Lancio client$$i TEST 2" >>"testout.log"; \
	(./clientfortest.out "client$$i" 2 &) 1>>"testout.log" ; \
	i=$$((i + 1)); \
	done)
	(i=31; while [ "$$i" -le 50 ]; do \
	echo "Lancio client$$i TEST 3" >>"testout.log"; \
	(./clientfortest.out "client$$i" 3 &) 1>>"testout.log" ; \
	i=$$((i + 1)); \
	done)
	@echo "TEST 2 e 3 TERMINATI"
	@echo "Mando in esecuzione lo script"
	bash ./testsum.sh

clean:
	@echo "cleaning generated files"
	-rm -rf $(OUTFILES) $(OBJFILES) *.txt *.sock ./data *.log

