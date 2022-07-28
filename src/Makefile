PROG=astream
DESTDIR=
PREFIX=/usr
BINDIR=$(PREFIX)/local

obj := astream.o astream.h astream_log.c astream_log.h
astream : ${obj}
	cc -o astream ${obj}

install:
	install -d $(DESTDIR)$(BINDIR)
	install -p -m 0755 $(PROG) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(PROG)
clean:
	rm -f astream.o astream_log.o