# $Id: Makefile,v 1.1.1.1 2005/12/29 13:09:17 tomas Exp $

T= rings
V= 1.0.0
CONFIG= ./config

include $(CONFIG)

SRCS= src/rings.c
OBJS= $(COMPAT_O) src/rings.o
COMPAT_O= $(COMPAT_DIR)/compat-5.1.o


src/$(LIBNAME) : $(OBJS)
	export MACOSX_DEPLOYMENT_TARGET="10.3"; $(CC) $(CFLAGS) $(LIB_OPTION) -o src/$(LIBNAME) src/rings.o $(COMPAT_O)

$(COMPAT_O): $(COMPAT_DIR)/compat-5.1.c
	$(CC) -c $(CFLAGS) -o $@ $(COMPAT_DIR)/compat-5.1.c

install:
	mkdir -p $(LUA_LIBDIR)
	cp src/$(LIBNAME) $(LUA_LIBDIR)
	cd $(LUA_LIBDIR); ln -f -s $(LIBNAME) $T.so

clean:
	rm -f src/$(LIBNAME) $(OBJS)
