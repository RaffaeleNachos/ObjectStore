gcc -g objectstorelib.c -c -o objectstorelib.o ; ar rvs objectstorelib.a objectstorelib.o ; gcc -g -Wall clientfortest.c -o clientfortest.out -L . objectstorelib.a
