
LIBJSON5_OFILES=\
	libjson5/json.o\

LIBJSON5_HFILES=\
	include/json.h\

LIBJSON5_LIBS=\
	lib/libjson5.a\

$(LIBJSON5_HFILES): libjson5/json.h
	cp libjson5/json.h $(LIBJSON5_HFILES)

$(LIBJSON5_OFILES): $(LIBJSON5_HFILES)

$(LIBJSON5_LIBS): $(LIBJSON5_OFILES)
	$(AR) r $@ $(LIBJSON5_OFILES)
