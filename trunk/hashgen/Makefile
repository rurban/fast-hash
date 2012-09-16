ULIB_INC = dep/ulib/include
ULIB_LIB = dep/ulib/lib

CXXFLAGS = -DUNDEBUG -I $(ULIB_INC) -O3 -W -Wall -pipe
#CXXFLAGS = -I $(ULIB_INC) -O3 -W -Wall -pipe
LDFLAGS  = -L $(ULIB_LIB) -lulib -lm -lrt -lpthread

.cpp.o:
	$(CXX) -c $(CXXFLAGS)  $< -o $@

.PHONY: dep all clean

all: dep avalanche hashgen

dep: ../fasthash.o
	make -C dep/ulib release

avalanche: avalanche.o aval_main.o xxhash.o
	$(CXX) avalanche.o ../fasthash.o aval_main.o xxhash.o -o avalanche $(LDFLAGS)

hashgen: hashgen.o xxhash.o
	$(CXX) avalanche.o hashgen.o ../fasthash.o xxhash.o -o hashgen $(LDFLAGS)

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
