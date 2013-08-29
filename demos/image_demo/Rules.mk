# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

DEMOS += image-demo

image-demo_SOURCES := $(call filelist, image_demo.cpp NodePacker.cpp) $(COMMON_DEMO_SOURCES)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
