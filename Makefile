
CFLAGS=-Iinclude -O2 -fomit-frame-pointer
LDFLAGS=-Llib

generalstore: main.o
	$(CC) $(LDFLAGS) -o $@ main.o -levent

clean:
	rm -rf generalstore *.o

distclean: clean
	rm -rf lib bin include
	make -C libevent distclean

include libevent.mk
