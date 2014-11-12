# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

NGL_FILTER := $(call filelist, filter)
NGL_EXTRACTOR := $(call filelist, extractor)

NGL_EXTRACTOR_LDFLAGS := 
NGL_LL := flex

$(NGL_FILTER): $(call filelist, filter.cpp)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(NGL_EXTRACTOR): $(call filelist, gl_flex.o HeaderCreator.o)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(NGL_EXTRACTOR_LDFLAGS)

$(call filelist, gl_flex.cpp): $(call filelist, gl_flex.fl.cpp)
	$(NGL_LL) -o $@ $^


EXTRA_CLEAN += $(call filelist, extractor filter gl_flex.cpp lex.yy.c)
EXTRA_CLEAN += $(call filelist, *.o *.exe)

.SECONDARY: $(NGL_FILTER) $(NGL_EXTRACTOR)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
