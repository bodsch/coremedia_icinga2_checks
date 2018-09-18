
.PHONY: build redox clean

default: build

redox:
	cd 3rdParty/redox && mkdir -p build && cd build && cmake .. && make

build:	redox
	mkdir -p build && cd build && cmake .. && make

clean:
	cd 3rdParty/redox/build && make clean && cd .. && rm -rf build
	cd build && make clean && cd .. && rm -rf build
