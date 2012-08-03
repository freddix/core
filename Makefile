VERSION		= 1.0

CC		= gcc
OPT_FLAGS	= -O2 -g
CFLAGS		= $(OPT_FLAGS) -Wall
INSTALL		= install
SBINDIR		= /usr/sbin
ETCDIR		= /etc

ETC_FILES       = filesystems fstab group host.conf hosts motd \
		  passwd profile protocols resolv.conf securetty \
		  services shells

DOCS		= README.asciidoc

BIN_FILES	= tools/delpasswd tools/joinpasswd tools/postshell

SOURCES		= tools/delpasswd.c tools/joinpasswd.c tools/postshell.c

CLEAN		= $(BIN_FILES) *.o *.tar.xz *~

all: $(BIN_FILES)

delpasswd: tools/delpasswd.o
joinpasswd: tools/joinpasswd.o
postshell: tools/postshell.o

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLEAN)
	cd etc; rm -f $(CLEAN)

install:
	$(INSTALL) -d $(DESTDIR)/$(SBINDIR)
	$(INSTALL) -d $(DESTDIR)/$(ETCDIR)
	$(INSTALL) $(BIN_FILES) $(DESTDIR)/$(SBINDIR)
	cd etc; $(INSTALL) $(ETC_FILES) $(DESTDIR)/$(ETCDIR)
	ln -sf /proc/self/mounts $(DESTDIR)/$(ETCDIR)/mtab

dist: clean
	$(INSTALL) -d core-$(VERSION)/etc
	$(INSTALL) -d core-$(VERSION)/tools
	$(INSTALL) $(DOCS) Makefile core-$(VERSION)
	$(INSTALL) $(SOURCES) core-$(VERSION)/tools

	for file in $(ETC_FILES); do \
		$(INSTALL) -m644 etc/$$file core-$(VERSION)/etc; \
	done

	tar -cJf core-$(VERSION).tar.xz core-$(VERSION)

	rm -rf core-$(VERSION)

changelog:
	./changelog.sh

