PROG=astream
DESTDIR=
PREFIX=/usr
BINDIR=$(PREFIX)/bin

astream : astream.c astream.h astream_log.o
	cc -g -Wall -o astream astream.c astream_log.o

astream_log.o : astream_log.c astream_log.h
	cc -g -Wall -c astream_log.c

install:
	install -d $(DESTDIR)$(BINDIR)
	install -p -m 0755 $(PROG) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(PROG)
clean:
	rm -f astream astream.o astream_log.o
