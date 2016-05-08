
CFLAGS=-Iinclude -O2 -fomit-frame-pointer
LDFLAGS=-Llib

all: generalstore

include libevent.mk

generalstore: main.o $(LIBEVENT_LIBS)
	$(CC) $(LDFLAGS) -o $@ main.o -levent

clean:
	rm -rf generalstore *.o

distclean: clean
	rm -rf lib bin include
	make -C libevent distclean


main.o: $(LIBEVENT_HFILES)
