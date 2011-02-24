# ej

VERSION = 0.1
CFLAGS += -c -Wall -O2 -I/usr/local/include `pkg-config --cflags gtk+-2.0` -DVERSION=\"${VERSION}\"
LDFLAGS += -L/usr/local/lib -ldjvulibre `pkg-config --libs gtk+-2.0` 

OBJS = ej.o

ej: $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) $<

clean:
	rm -f $(OBJS)
	rm -f ej

