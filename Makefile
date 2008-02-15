# $Id: Makefile,v 1.5 2008/02/15 20:52:18 carregal Exp $

T= rings
V= 1.2.0
CONFIG= ./config

include $(CONFIG)

SRCS= src/rings.c
OBJS= src/rings.o

all: src/rings.so

src/rings.so: $(OBJS)
	export MACOSX_DEPLOYMENT_TARGET="10.3"; $(CC) $(CFLAGS) $(LIB_OPTION) -o src/rings.so $(OBJS)

install:
	mkdir -p $(LUA_LIBDIR)
	cp src/rings.so $(LUA_LIBDIR)
	mkdir -p $(LUA_DIR)
	cp src/stable.lua $(LUA_DIR)

clean:
	rm -f src/rings.so $(OBJS)
