# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

LIB_SOURCES += $(call filelist, $(NGL).cpp)
INCLUDES += -I$(d) -I$(GL_INCLUDEPATH)


GL_RAW_HEADER_FILES=GL/gl.h GL/glext.h
GLES2_RAW_HEADER_FILES=GLES2/gl2platform.h GLES2/gl2.h GLES2/gl2ext.h 
GLES3_RAW_HEADER_FILES=GLES3/gl3platform.h GLES3/gl3.h GLES3/gl3ext.h GLES2/gl2ext.h 

GL_HEADER_FILES=$(GL_RAW_HEADER_FILES:%.h=$(GL_INCLUDEPATH)/%.h)
GLES2_HEADER_FILES=$(GLES2_RAW_HEADER_FILES:%.h=$(GL_INCLUDEPATH)/%.h)
GLES3_HEADER_FILES=$(GLES3_RAW_HEADER_FILES:%.h=$(GL_INCLUDEPATH)/%.h)

NGL_DIR := $(d)

$(call filelist, ngl_gl.cpp ngl_gl.hpp): $(NGL_FILTER) $(NGL_EXTRACTOR)
	$(NGL_FILTER) $(GL_HEADER_FILES) \
	| $(NGL_EXTRACTOR) name=ngl_gl path=$(GL_INCLUDEPATH) outputpath=$(NGL_DIR) $(GL_RAW_HEADER_FILES)

$(call filelist, ngl_gles2.cpp ngl_gles2.hpp): $(NGL_FILTER) $(NGL_EXTRACTOR)
	$(NGL_FILTER) $(GLES2_HEADER_FILES) \
	| $(NGL_EXTRACTOR) name=ngl_gles2 path=$(GL_INCLUDEPATH) outputpath=$(NGL_DIR) $(GLES2_RAW_HEADER_FILES)

$(call filelist, ngl_gles3.cpp ngl_gles3.hpp): $(NGL_FILTER) $(NGL_EXTRACTOR)
	$(NGL_FILTER) $(GLES3_HEADER_FILES) \
	| $(NGL_EXTRACTOR) name=ngl_gles3 path=$(GL_INCLUDEPATH) outputpath=$(NGL_DIR) $(GLES3_RAW_HEADER_FILES)

EXTRA_CLEAN += $(call filelist, ngl_gldetailed_output.txt ngl_gles2detailed_output.txt ngl_gles3detailed_output.txt)
EXTRA_CLEAN += $(call filelist, ngl_glfunction_list_output.txt ngl_gles2function_list_output.txt ngl_gles3function_list_output.txt)
EXTRA_CLEAN += $(call filelist, ngl_gl.cpp ngl_gl.hpp ngl_gles2.cpp ngl_gles2.hpp ngl_gles3.cpp ngl_gles3.hpp)

.SECONDARY: $(NGL).cpp $(NGL).hpp

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
