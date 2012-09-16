# root of ulib directory
ROOT	= ../../..

# path of ulib include output
INCPATH = $(ROOT)/include/ulib

# path of ulib lib output
LIBPATH	= $(ROOT)/lib

# output library
TARGET	= $(LIBPATH)/librng.a

CC	?= gcc
CFLAGS	?= -g3 -O2 -Wall -W -Werror -pipe -c

#
# define object files below
#
OBJS	= zipf_rng.o

.c.o:
	$(CC) -I $(INCPATH) $(CFLAGS) $< -o $@

#
# build targets
#

all: $(TARGET)

.PHONY: all clean $(INCPATH)

$(TARGET): $(OBJS) $(INCPATH)
	mkdir -p $(LIBPATH)
	ar csr $(TARGET) $(OBJS)

$(INCPATH):
	mkdir -p $(INCPATH)
	cp *.h $(INCPATH)/

clean:
	rm -rf $(OBJS)
	rm -rf $(TARGET)
