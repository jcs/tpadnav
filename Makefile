#
# Copyright 2023 joshua stein <jcs@jcs.org>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

PREFIX?=	/usr/local
X11BASE?=	/usr/X11R6

PKGLIBS=	x11 xtst xi

CC?=		cc
CFLAGS+=	-O2 -Wall -Wunused \
		-Wunused -Wmissing-prototypes -Wstrict-prototypes \
		-Wpointer-sign -Wshadow \
		`pkg-config --cflags ${PKGLIBS}`
LDFLAGS+=	`pkg-config --libs ${PKGLIBS}`

#CFLAGS+=	-g

BINDIR=		$(PREFIX)/bin

SRC=		tpadnav.c

OBJ=		${SRC:.c=.o}

BIN=		tpadnav

all: $(BIN)

$(OBJ):		Makefile

$(BIN): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

install: all
	mkdir -p $(BINDIR) $(MANDIR)
	install -s $(BIN) $(BINDIR)

clean:
	rm -f $(BIN) $(OBJ)

.PHONY: all install clean
