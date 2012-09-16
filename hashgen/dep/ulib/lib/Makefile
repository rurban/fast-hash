TARGET	= libulib.a

$(TARGET):
	for lib in $(wildcard *.a); do \
		ar x $${lib}; \
	done;
	ar csr $(TARGET) *.o
	rm -rf *.o

.PHONY: debug release clean

debug: $(TARGET)

release:
	for lib in $(wildcard *.a); do \
		ar x $${lib}; \
	done;
	ar csr $(TARGET) *.o
	strip --strip-debug --strip-unneeded $(TARGET)
	rm -rf *.o

clean:
	rm -rf *.a
