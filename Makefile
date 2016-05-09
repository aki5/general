
CFLAGS=-Iinclude -O2 -fomit-frame-pointer
LDFLAGS=-Llib

all: general

include libssl.mk
include libevent.mk
include libevhtp.mk

general: main.o $(LIBEVHTP_LIBS) $(LIBEVENT_LIBS) $(LIBSSL_LIBS)
	$(CC) $(LDFLAGS) -o $@ main.o -levhtp -levent -levent_openssl -lssl -lcrypto

clean:
	rm -rf general *.o

distclean: clean
	rm -rf lib bin include share ssl
	rm -rf libevhtp/build
	cd libevhtp && git checkout build/placeholder
	make -C openssl clean && rm openssl/Makefile
	make -C libevent distclean

main.o: $(LIBEVENT_HFILES) $(LIBSSL_HFILES)
