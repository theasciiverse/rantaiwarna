#
# Copyright (C) 2014 Guido Berhoerster <guido+rantaiwarna@berhoerster.name>
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

PACKAGE =	rantaiwarna
VERSION =	1
DISTNAME :=	$(PACKAGE)-$(VERSION)

COMPILE.c =	$(CC) $(CFLAGS) $(XCFLAGS) $(CPPFLAGS) $(XCPPFLAGS) $(TARGET_ARCH) -c
# gcc, clang, icc
MAKEDEPEND.c =	$(CC) -MM $(CFLAGS) $(XCFLAGS) $(CPPFLAGS) $(XCPPFLAGS)
# Sun/Solaris Studio
#MAKEDEPEND.c =	$(CC) -xM1 $(CFLAGS) $(XCFLAGS) $(CPPFLAGS) $(XCPPFLAGS)
# X makedepend
#MAKEDEPEND.c =	makedepend -f- -Y -- $(CFLAGS) $(XCFLAGS) $(CPPFLAGS) $(XCPPFLAGS) --
LINK.c =	$(CC) $(CFLAGS) $(XCFLAGS) $(CPPFLAGS) $(XCPPFLAGS) $(LDFLAGS) $(XLDFLAGS) $(TARGET_ARCH)
LINK.o =	$(CC) $(LDFLAGS) $(XLDFLAGS) $(TARGET_ARCH)
INSTALL :=	install
INSTALL.exec :=	$(INSTALL) -D -m 0755
INSTALL.data :=	$(INSTALL) -D -m 0644
PAX :=		pax
GZIP :=		gzip
SED :=		sed
XSLTPROC :=	xsltproc
DOCBOOK5_MANPAGES_STYLESHEET =	http://docbook.sourceforge.net/release/xsl-ns/current/manpages/docbook.xsl

define generate-manpage-rule =
%.$(1): %.$(1).xml
	$$(XSLTPROC) \
	    --xinclude \
	    --stringparam package $$(PACKAGE) \
	    --stringparam version $$(VERSION)\
	    docbook-update-source-data.xsl $$< | \
	    $$(XSLTPROC) \
	    --xinclude \
	    $$(DOCBOOK5_MANPAGES_FLAGS) \
	    --output $$@ \
	    $$(DOCBOOK5_MANPAGES_STYLESHEET) \
	    -
endef

DESTDIR ?=
prefix ?=	/usr/local
bindir ?=	$(prefix)/bin
datadir ?=	$(prefix)/share
mandir ?=	$(datadir)/man

OBJS =		rantaiwarna.o board.o highscore.o util.o
MANPAGES =	$(PACKAGE).6
DOCBOOK5_MANPAGES_FLAGS =	--stringparam man.authors.section.enabled 0 \
				--stringparam man.copyright.section.enabled 0

.DEFAULT_TARGET = all

.PHONY: all clean clobber dist install

all: $(PACKAGE) $(MANPAGES)

$(PACKAGE): XCPPFLAGS :=
# Illumos/Solaris
#$(PACKAGE): XCPPFLAGS := -I/usr/xpg4/include
$(PACKAGE): XCFLAGS :=
$(PACKAGE): LDLIBS :=	-lcurses -lm
# Linux, FreeBSD, OpenBSD
#$(PACKAGE): LDLIBS :=	-lncursesw -ltinfo -lm
# Illumos/Solaris, UnixWare, NetBSD
#$(PACKAGE): LDLIBS :=	-lcurses -lm
$(PACKAGE): XLDFLAGS :=
# Illumos/Solaris
#$(PACKAGE): XLDFLAGS := -L/usr/xpg4/lib -R/usr/xpg4/lib
$(PACKAGE): $(OBJS)
	$(LINK.o) $^ $(LDLIBS) -o $@

$(foreach section,1 2 3 4 5 6 7 8 9,$(eval $(call generate-manpage-rule,$(section))))

%.o: %.c
	$(MAKEDEPEND.c) $< | $(SED) -f deps.sed >$*.d
	$(COMPILE.c) -o $@ $<

install:
	$(INSTALL.exec) $(PACKAGE) "$(DESTDIR)$(bindir)/$(PACKAGE)"
	for manpage in $(MANPAGES); do \
	    $(INSTALL.data) $${manpage} \
	        "$(DESTDIR)$(mandir)/man$${manpage##*.}/$${manpage##*/}"; \
	done

clean:
	rm -f $(PACKAGE) $(OBJS) $(MANPAGES)

clobber: clean
	rm -f $(patsubst %.o,%.d,$(OBJS))

dist: clobber
	$(PAX) -w -x ustar -s ',.*/\..*,,' -s ',./[^/]*\.tar\.gz,,' \
	    -s ',\./,$(DISTNAME)/,' . | $(GZIP) > $(DISTNAME).tar.gz

-include $(patsubst %.o,%.d,$(OBJS))
