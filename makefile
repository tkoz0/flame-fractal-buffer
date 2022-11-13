CC=gcc
CPPC=g++
IDIR=.
CFLAGS=-I$(IDIR)
LIBS=-lm

FLAME_HEADERS = flame/renderer.h flame/types.h flame/variations.h
UTILS_HEADERS = utils/jrand.h utils/json_small.hpp
HEADERS = $(FLAME_HEADERS) $(UTILS_HEADERS)

ffbuf.out: main_buf.o parser.o renderer.o variations.o jrand.o json_small.o
	$(CPPC) -o ffbuf.out parser.o jrand.o variations.o renderer.o json_small.o main_buf.o $(LIBS)

#libjsonsmall.so: json_small.o
#	$(CC) -shared -s -o libjsonsmall.so json_small.o \
#	-fvisibility=hidden -fvisibility-inlines-hidden -fno-rtti -s

main_buf.o: main_buf.cpp
	$(CPPC) -g -Wall -Werror -O3 -c -o main_buf.o main_buf.cpp

parser.o: parser.cpp parser.hpp
	$(CPPC) -g -Wall -Werror -O3 -c -o parser.o parser.cpp

renderer.o: flame/renderer.c $(HEADERS)
	$(CC) -g -Wall -Werror -O3 -c -o renderer.o flame/renderer.c

variations.o: flame/variations.c $(HEADERS)
	$(CC) -g -Wall -Werror -O3 -c -o variations.o flame/variations.c

jrand.o: utils/jrand.c $(HEADERS)
	$(CC) -g -Wall -Werror -O3 -c -o jrand.o utils/jrand.c

json_small.o: utils/json_small.cpp $(HEADERS) nlohmann/json.hpp
	$(CPPC) -g -Wall -Werror -O3 -c -o json_small.o utils/json_small.cpp

nlohmann/json.hpp:
	wget -o nlohmann/json.hpp \
	https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp

.PHONY: clean

clean:
	rm jrand.o variations.o renderer.o json_small.o parser.o main_buf.o ffbuf.out
