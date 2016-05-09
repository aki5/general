
CFLAGS=-Iinclude -Iinclude/evhtp -O2 -fomit-frame-pointer
LDFLAGS=-Llib

all: general

include libssl.mk
include libevent.mk
include libevhtp.mk

general: main.o $(LIBEVHTP_LIBS) $(LIBEVENT_LIBS) $(LIBSSL_LIBS)
	$(CC) $(LDFLAGS) -o $@ main.o -levhtp -levent -levent_openssl -lssl -lcrypto -lpthread

clean:
	rm -rf general *.o

distclean: clean
	rm -rf lib bin include share ssl
	rm -rf libevhtp/build
	cd libevhtp && git checkout build/placeholder
	make -C openssl clean && rm openssl/Makefile
	make -C libevent distclean

main.o: $(LIBEVHTP_HFILES) $(LIBEVENT_HFILES) $(LIBSSL_HFILES)
