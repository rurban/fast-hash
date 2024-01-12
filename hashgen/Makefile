ULIB_INC = dep/ulib/include
ULIB_LIB = dep/ulib/lib

DEFS = -DUNDEBUG
CXXFLAGS = $(DEFS) -I $(ULIB_INC) -O3 -W -Wall -march=native
LDFLAGS  = -L $(ULIB_LIB) -lulib -lm -lrt -lpthread

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $< -o $@

.PHONY: dep all clean

all: dep avalanche hashgen magic

dep/ulib/lib/libulib.a:
	make -C dep/ulib release
dep: ../fasthash.c ../fasthash1.c dep/ulib/lib/libulib.a

avalanche: avalanche.o aval_main.o xxhash.c
	$(CXX) $(CXXFLAGS) avalanche.o aval_main.o xxhash.c ../fasthash.c ../fasthash1.c -o avalanche $(LDFLAGS)

hashgen: hashgen.o xxhash.c
	$(CXX) $(CXXFLAGS) avalanche.o hashgen.o ../fasthash.c ../fasthash1.c xxhash.c -o hashgen $(LDFLAGS)

magic: magic.o
	$(CXX) $(CXXFLAGS) magic.o -o magic $(LDFLAGS)

clean:
	rm -rf *.o
	rm -rf *~
	rm -rf avalanche
	rm -rf hashgen
	rm -rf magic
	make -C dep/ulib clean
