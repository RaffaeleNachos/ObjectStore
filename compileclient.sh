gcc -g objectstorelib.c -c -o objectstorelib.o ; ar rvs objectstorelib.a objectstorelib.o ; gcc -g -Wall typicalclient.c -o typicalclient.out -L . objectstorelib.a
