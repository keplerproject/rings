# $Id: Makefile,v 1.6 2008/05/08 22:08:31 carregal Exp $

T= rings
CONFIG= ./config

include $(CONFIG)

DESTDIR := /
LDIR := $(DESTDIR)/$(LUA_DIR)/wsapi
CDIR := $(DESTDIR)/$(LUA_LIBDIR)
BDIR := $(DESTDIR)/$(BIN_DIR)

SRCS= src/rings.c
OBJS= src/rings.o

all: src/rings.so

src/rings.so: $(OBJS)
	export MACOSX_DEPLOYMENT_TARGET="10.3"; $(CC) $(CFLAGS) $(LIB_OPTION) -o src/rings.so $(OBJS)

install:
	@mkdir -p $(CDIR)
	@cp src/rings.so $(CDIR)
	@mkdir -p $(LDIR)
	@cp src/stable.lua $(LDIR)
	@echo "Installing of Rings library is done!"

clean:
	@rm -f src/rings.so $(OBJS)
	@echo "Cleaning is done!"
