
UNAME := $(shell uname)
CFLAGS=-Iinclude -Iinclude/evhtp -O2 -fomit-frame-pointer
LDFLAGS=-Llib -Wl,--gc-sections

ifeq ($(UNAME),Darwin)
LIBS=-levhtp -levent -levent_openssl -lssl -lcrypto -lpthread
else
CC=$(CURDIR)/bin/musl-gcc
LIBS=-levhtp -levent -levent_openssl -lssl -lcrypto -lpthread -lrt
endif

all: general

include libc.mk
include libssl.mk
include libevent.mk
include libevhtp.mk
include libpaxos.mk

general: main.o servedns.o base64.o city.o $(LIBC_LIBS) $(LIBEVHTP_LIBS) $(LIBEVENT_LIBS) $(LIBSSL_LIBS)
	$(CC) $(LDFLAGS) -o $@ main.o servedns.o base64.o $(LIBS)

certs:
	mkdir certs

certs/localhost.key: bin/openssl certs
	bin/openssl genpkey -algorithm RSA -out certs/localhost.key -pkeyopt rsa_keygen_bits:2048

certs/localhost.csr: certs/localhost.key bin/openssl certs
	bin/openssl req -subj '/C=US/ST=CA/L=San Francisco/CN=localhost' -new -key certs/localhost.key -out certs/localhost.csr

certs/localhost.cert: certs/localhost.key certs/localhost.csr bin/openssl certs
	bin/openssl req -x509 -days 365 -key certs/localhost.key -in certs/localhost.csr -out certs/localhost.cert

certs/localhost.pem: certs/localhost.cert certs/localhost.key certs
	cat certs/localhost.key certs/localhost.cert > certs/localhost.pem

test: general certs/localhost.pem
	(./general -c certs/localhost.pem & (sleep 1 && curl --insecure https://localhost:5443/ ; echo ; kill $$!))

clean:
	rm -rf general *.o

distclean: clean
	rm -rf lib bin include share ssl
	rm -rf libevhtp/build
	cd libevhtp && git checkout build/placeholder
	make -C openssl clean && rm openssl/Makefile || exit 0
	make -C libevent distclean || exit 0
	make -C musl distclean || exit 0

main.o servedns.o base64.o city.o: $(LIBC_ALL) $(LIBEVHTP_HFILES) $(LIBEVENT_HFILES) $(LIBSSL_HFILES)
