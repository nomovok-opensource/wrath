#make sure we are using bash
SHELL=/bin/bash
#docs 
docs: examples
	doxygen Doxyfile

#generate examples related documentation
examples:
	./generate_example_docs.sh ../demos/examples/ documentation/WRATH/ screenshots/

clean:
	-rm -r doxy/html
	-rm doxy/*.tmp

.PHONY: examples
