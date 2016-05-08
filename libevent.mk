
LIBEVENT_LIBS=\
	lib/libevent.a\
	lib/libevent_core.a\
	lib/libevent_extra.a\
	lib/libevent_pthreads.a\

LIBEVENT_HFILES=\
	include/evdns.h\
	include/event.h\
	include/event2/buffer.h\
	include/event2/buffer_compat.h\
	include/event2/bufferevent.h\
	include/event2/bufferevent_compat.h\
	include/event2/bufferevent_ssl.h\
	include/event2/bufferevent_struct.h\
	include/event2/dns.h\
	include/event2/dns_compat.h\
	include/event2/dns_struct.h\
	include/event2/event-config.h\
	include/event2/event.h\
	include/event2/event_compat.h\
	include/event2/event_struct.h\
	include/event2/http.h\
	include/event2/http_compat.h\
	include/event2/http_struct.h\
	include/event2/keyvalq_struct.h\
	include/event2/listener.h\
	include/event2/rpc.h\
	include/event2/rpc_compat.h\
	include/event2/rpc_struct.h\
	include/event2/tag.h\
	include/event2/tag_compat.h\
	include/event2/thread.h\
	include/event2/util.h\
	include/event2/visibility.h\
	include/evhttp.h\
	include/evrpc.h\
	include/evutil.h\

libevent/configure: libevent/autogen.sh
	cd libevent && ./autogen.sh

libevent/Makefile: libevent/configure
	cd libevent && ./configure --prefix=$(CURDIR) --disable-shared --disable-openssl

$(LIBEVENT_LIBS) $(LIBEVENT_HFILES): $(shell find libevent -type f -name '*.[ch]') libevent/Makefile
	make -C libevent install

