
LIBSSL_LIBS=\
	lib/libssl.a\
	lib/libcrypto.a\

LIBSSL_HFILES=\
	include/openssl/aes.h\
	include/openssl/asn1.h\
	include/openssl/asn1_mac.h\
	include/openssl/asn1t.h\
	include/openssl/bio.h\
	include/openssl/bn.h\
	include/openssl/buffer.h\
	include/openssl/cmac.h\
	include/openssl/cms.h\
	include/openssl/comp.h\
	include/openssl/conf_api.h\
	include/openssl/conf.h\
	include/openssl/crypto.h\
	include/openssl/des.h\
	include/openssl/des_old.h\
	include/openssl/dh.h\
	include/openssl/dsa.h\
	include/openssl/dso.h\
	include/openssl/dtls1.h\
	include/openssl/ebcdic.h\
	include/openssl/ecdh.h\
	include/openssl/ecdsa.h\
	include/openssl/ec.h\
	include/openssl/engine.h\
	include/openssl/e_os2.h\
	include/openssl/err.h\
	include/openssl/evp.h\
	include/openssl/hmac.h\
	include/openssl/krb5_asn.h\
	include/openssl/kssl.h\
	include/openssl/lhash.h\
	include/openssl/md5.h\
	include/openssl/mdc2.h\
	include/openssl/modes.h\
	include/openssl/objects.h\
	include/openssl/obj_mac.h\
	include/openssl/ocsp.h\
	include/openssl/opensslconf.h\
	include/openssl/opensslv.h\
	include/openssl/ossl_typ.h\
	include/openssl/pem2.h\
	include/openssl/pem.h\
	include/openssl/pkcs12.h\
	include/openssl/pkcs7.h\
	include/openssl/pqueue.h\
	include/openssl/rand.h\
	include/openssl/ripemd.h\
	include/openssl/rsa.h\
	include/openssl/safestack.h\
	include/openssl/sha.h\
	include/openssl/srp.h\
	include/openssl/srtp.h\
	include/openssl/ssl23.h\
	include/openssl/ssl2.h\
	include/openssl/ssl3.h\
	include/openssl/ssl.h\
	include/openssl/stack.h\
	include/openssl/symhacks.h\
	include/openssl/tls1.h\
	include/openssl/ts.h\
	include/openssl/txt_db.h\
	include/openssl/ui_compat.h\
	include/openssl/ui.h\
	include/openssl/whrlpool.h\
	include/openssl/x509.h\
	include/openssl/x509v3.h\
	include/openssl/x509_vfy.h\


ifeq ($(UNAME),Darwin)

openssl/Makefile: openssl/Configure
	cd openssl && CC=$(CC) ./Configure no-shared no-dso no-ssl2 --prefix=$(CURDIR) darwin64-x86_64-cc

else

openssl/Makefile: openssl/Configure
	cd openssl && CC=$(CC) CFLAGS='-O2 -ffunction-sections -fdata-sections -fomit-frame-pointer' \
		./config\
		no-shared\
		no-dso\
		no-ssl2\
		no-ssl3\
		no-asm\
		no-hw\
		no-krb5\
		no-idea\
		no-camellia\
		no-seed\
		no-bf\
		no-cast\
		no-rc2\
		no-rc4\
		no-rc5\
		no-md2\
		no-md4\
		--prefix=$(CURDIR)

#		no-md5\
#		no-ripemd\
#		no-mdc2\
#		no-aes\
#		no-des\
#		no-sha\
#		no-rsa\
#		no-dsa\
#		no-dh\
#		no-ec\
#		no-ecdsa\
#		no-ecdh\
#		no-err\

endif

bin/openssl $(LIBSSL_LIBS) $(LIBSSL_HFILES): $(LIBC_ALL) $(shell find openssl -type f -name '*.[ch]') openssl/Makefile
	make -C openssl install

