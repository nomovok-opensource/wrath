LL=flex
CXX?=g++
CFLAGS=-Wall 
LINKER=-lfl 

all:  extractor filter

extractor: gl_flex.o HeaderCreator.o gl_flex.o
	$(CXX) $(CFLAGS) -o extractor gl_flex.o HeaderCreator.o $(LINKER)

filter: filter.cpp
	$(CXX) $(CFLAGS) -o filter filter.cpp

gl_flex.cpp: gl_flex.fl.cpp 
	$(LL) -o gl_flex.cpp gl_flex.fl.cpp 

gl_flex.o: gl_flex.cpp HeaderCreator.hpp
	$(CXX) $(CFLAGS) -c gl_flex.cpp

HeaderCreator.o: HeaderCreator.hpp HeaderCreator.cpp
	$(CXX) $(CFLAGS) -c HeaderCreator.cpp

clean:
	-rm -f *.o extractor *.exe filter gl_flex.cpp *~ lex.yy.c 

