ULIB_INC = dep/ulib/include
ULIB_LIB = dep/ulib/lib

DEFS = -DUNDEBUG
CXXFLAGS = -g $(DEFS) -I $(ULIB_INC) -O3 -W -Wall -march=native
LDFLAGS  = -L $(ULIB_LIB) -lulib -lm -lrt -lpthread

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $< -o $@

.PHONY: dep all clean clang-format

all: dep avalanche hashgen magic

dep/ulib/lib/libulib.a:
	make -C dep/ulib release
dep: ../fasthash.c dep/ulib/lib/libulib.a

avalanche: avalanche.o aval_main.o xxhash.c
	$(CXX) $(CXXFLAGS) avalanche.o ../fasthash.c aval_main.o xxhash.c -o avalanche $(LDFLAGS)

hashgen: hashgen.o xxhash.c
	$(CXX) $(CXXFLAGS) avalanche.o hashgen.o ../fasthash.c xxhash.c -o hashgen $(LDFLAGS)

magic: magic.o
	$(CXX) $(CXXFLAGS) magic.o -o magic $(LDFLAGS)

clang-format:
	clang-format -i *.cpp *.c *.h
clean:
	rm -rf *.o
	rm -rf *~
	rm -rf avalanche
	rm -rf hashgen
	rm -rf magic
	make -C dep/ulib clean
