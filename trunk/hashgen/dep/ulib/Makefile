all:
	make -C src/
	make -C lib/
	make -C test/

.PHONY: all clean remove_bak release

release:
	make DEBUG=-DUNDEBUG -C src/
	make -C lib/ release
	make DEBUG=-DUNDEBUG -C test/

clean:
	make -C src/ clean
	make -C test/ clean
	make -C lib/ clean

remove_bak:
	find . -name "*~" | xargs rm
