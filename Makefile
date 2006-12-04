# $Id: Makefile,v 1.3 2006/12/04 16:01:12 mascarenhas Exp $

T= rings
V= 1.1.0
CONFIG= ./config

include $(CONFIG)

SRCS= src/rings.c
OBJS= src/rings.o

src/$(LIBNAME) : $(OBJS)
	export MACOSX_DEPLOYMENT_TARGET="10.3"; $(CC) $(CFLAGS) $(LIB_OPTION) -o src/$(LIBNAME) $(OBJS)

install:
	mkdir -p $(LUA_LIBDIR)
	cp src/$(LIBNAME) $(LUA_LIBDIR)
	cd $(LUA_LIBDIR); ln -f -s $(LIBNAME) $T.so
	mkdir -p $(LUA_DIR)
	cp src/stable.lua $(LUA_DIR)

clean:
	rm -f src/$(LIBNAME) $(OBJS)
