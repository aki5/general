
LIBEVHTP_LIBS=\
	lib/libevhtp.a\

libevhtp/build/Makefile: $(LIBEVENT_LIBS)
	cd libevhtp/build && CMAKE_LIBRARY_PATH=$(CURDIR)/lib CMAKE_INCLUDE_PATH=$(CURDIR)/include cmake ..

libevhtp/build/libevhtp.a: libevhtp/build/Makefile
	make -C libevhtp/build

$(LIBEVHTP_LIBS): libevhtp/build/libevhtp.a
	cp libevhtp/build/libevhtp.a lib/libevhtp.a

