all:
	gcc test.c -o test -lssl -lcrypto

clean:
	rm -f test