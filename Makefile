all: build-dbg

build-dbg:
	mkdir -p build-dbg
	(cd build-dbg && cmake .. && make)

test: build-dbg
	build-dbg/tests/test-sensord

clean:
	rm -rf build-dbg

.PHONY: build-dbg all test clean
