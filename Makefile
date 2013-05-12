# $Id: Makefile,v 1.6 2008/05/08 22:08:31 carregal Exp $

T= rings
CONFIG= ./config

include $(CONFIG)

SRCS= src/rings.c
OBJS= src/rings.o

all: src/rings.so

src/rings.so: $(OBJS)
	export MACOSX_DEPLOYMENT_TARGET="10.3"; $(CC) $(CFLAGS) $(LIB_OPTION) -o src/rings.so $(OBJS)

install:
	mkdir -p $(DESTDIR)$(LUA_LIBDIR)
	cp src/rings.so $(DESTDIR)$(LUA_LIBDIR)
	mkdir -p $(DESTDIR)$(LUA_DIR)
	cp src/stable.lua $(DESTDIR)$(LUA_DIR)

clean:
	rm -f src/rings.so $(OBJS)
