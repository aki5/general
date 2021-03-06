
LIBEVHTP_LIBS=\
	lib/libevhtp.a\

LIBEVHTP_HFILES=\
	include/evhtp/evhtp.h\
	include/evhtp/htparse.h\
	include/evhtp/evhtp-config.h\
	include/evhtp/evthr.h\
	include/evhtp/onigposix.h\

libevhtp/build/Makefile: $(LIBEVENT_HFILES) $(LIBEVENT_LIBS)
	cd libevhtp/build && CMAKE_INCLUDE_PATH=$(CURDIR)/include CMAKE_LIBRARY_PATH=$(CURDIR)/lib cmake -DLIBEVENT_INCLUDE_DIR=$(CURDIR)/include -DCMAKE_INSTALL_PREFIX=$(CURDIR) ..

$(LIBEVHTP_LIBS) $(LIBEVHTP_HFILES): libevhtp/build/Makefile $(LIBC_ALL)
	make -C libevhtp/build install
	touch -c $(LIBEVHTP_LIBS) $(LIBEVHTP_HFILES)
