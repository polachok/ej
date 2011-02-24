# ej

VERSION = 0.1
LOCALBASE ?= /usr/local
CFLAGS += -c -Wall -O2 -I${LOCALBASE}/include `pkg-config --cflags gtk+-2.0` -DVERSION=\"${VERSION}\"
LDFLAGS += -L${LOCALBASE}/lib -ldjvulibre `pkg-config --libs gtk+-2.0` 

OBJS = ej.o

ej: $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) $<

clean:
	@rm -f $(OBJS)
	@rm -f ej

dist: clean
	@mkdir -p ej-${VERSION}
	@cp -R README LICENSE Makefile ej.c ej.1 ej-${VERSION}
	@tar -cf ej-${VERSION}.tar ej-${VERSION}
	@gzip ej-${VERSION}.tar
	@rm -rf ej-${VERSION}

