VERSION		= 1.0

CC		= gcc
OPT_FLAGS	= -O2 -g
CFLAGS		= $(OPT_FLAGS) -Wall
INSTALL		= install
SBINDIR		= /usr/sbin
ETCDIR		= /etc

ETC_FILES       = filesystems fstab group host.conf hosts motd \
		  passwd profile resolv.conf securetty shells

DOCS		= README.asciidoc

BIN_FILES	= tools/delpasswd tools/joinpasswd tools/postshell

SOURCES		= tools/delpasswd.c tools/joinpasswd.c tools/postshell.c

CLEAN		= $(BIN_FILES) *.o *.tar.xz *~

all: $(BIN_FILES)

delpasswd:	tools/delpasswd.o
joinpasswd:	tools/joinpasswd.o
postshell:	tools/postshell.o

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLEAN)
	cd etc; rm -f $(CLEAN)

install:
	$(INSTALL) -d $(DESTDIR)$(SBINDIR)
	$(INSTALL) -d $(DESTDIR)$(ETCDIR)
	$(INSTALL) $(BIN_FILES) $(DESTDIR)$(SBINDIR)
	cd etc; $(INSTALL) $(ETC_FILES) $(DESTDIR)$(ETCDIR)
	ln -sf /proc/self/mounts $(DESTDIR)$(ETCDIR)/mtab

dist: clean
	git archive --format=tar --prefix=core-$(VERSION)/ v$(VERSION) | xz -c > core-$(VERSION).tar.xz

