
generalstore: main.o
	$(CC) -o $@ main.o -levent

clean:
	rm -f *.o
