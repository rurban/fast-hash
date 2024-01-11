ULIB_INC = dep/ulib/include
ULIB_LIB = dep/ulib/lib

CXXFLAGS = -DUNDEBUG -I $(ULIB_INC) -O3 -W -Wall -march=native
#CXXFLAGS = -I $(ULIB_INC) -g -O3 -W -Wall -march=native
LDFLAGS  = -L $(ULIB_LIB) -lulib -lm -lrt -lpthread

.cpp.o:
	$(CXX) -c $(CXXFLAGS)  $< -o $@

.PHONY: dep all clean

all: dep avalanche hashgen

dep/ulib/lib/libulib.a:
	make -C dep/ulib release
dep: ../fasthash.o dep/ulib/lib/libulib.a

avalanche: avalanche.o aval_main.o xxhash.o
	$(CXX) avalanche.o ../fasthash.o aval_main.o xxhash.c -o avalanche $(LDFLAGS)

hashgen: hashgen.o xxhash.o
	$(CXX) avalanche.o hashgen.o ../fasthash.o xxhash.c -o hashgen $(LDFLAGS)

magic: magic.o
	$(CXX) magic.o -o magic $(LDFLAGS)

clean:
	rm -rf *.o
	rm -rf *~
	rm -rf ../fasthash.o
	rm -rf avalanche
	rm -rf hashgen
	rm -rf magic
	make -C dep/ulib clean
