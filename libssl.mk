
openssl/Makefile: openssl/config
	cd openssl && ./config no-shared no-dso --prefix=$(CURDIR)

#openssl/Makefile: openssl/Configure
#	cd openssl && ./Configure no-shared no-dso --prefix=$(CURDIR) darwin64-x86_64-cc

lib/libssl.a: $(shell find openssl -type f -name '*.[ch]') openssl/Makefile
	make -C openssl install

