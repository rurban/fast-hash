WORKROOT	= ../..

INCPATH		= $(WORKROOT)/include/ulib
LIBPATH		= $(WORKROOT)/lib

TARGET		= $(LIBPATH)/libubase.a

CC		?= gcc
CFLAGS		?= -g3 -O2 -Wall -W -Werror -pipe -c -fno-strict-aliasing
DEBUG		?=

DS_OBJS		= tree.o treeutils.o bitmap.o chainhash.o
SEARCH_OBJS	= fbsearch.o
CRYPT_OBJS	= aes.o sha1sum.o md5sum.o sha256sum.o rc4.o
SORT_OBJS	= listsort.o
MISC_OBJS	= strutils.o gcd.o bn.o hash.o hexdump.o
COMMON_OBJS	= version.o

OBJS		= $(DS_OBJS) $(SEARCH_OBJS) $(CRYPT_OBJS) \
		  $(SORT_OBJS) $(MISC_OBJS) $(COMMON_OBJS)

.c.o:
	$(CC) $(CFLAGS) $(DEBUG) $< -o $@

.PHONY: all clean $(INCPATH)

all: $(TARGET)

$(TARGET): $(OBJS) $(INCPATH)
	mkdir -p $(LIBPATH)
	ar csr $(TARGET) $(OBJS)

$(INCPATH):
	mkdir -p $(INCPATH)
	cp *.h $(INCPATH)/

clean:
	rm -rf $(OBJS)
	rm -rf $(TARGET)
	rm -rf $(INCPATH)/*.h
	rm -rf $(INCPATH)
