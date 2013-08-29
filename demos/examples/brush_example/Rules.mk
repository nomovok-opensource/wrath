# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header


dir := $(d)/shaders
include $(dir)/Rules.mk

DEMOS += brush-example

brush-example_SOURCES := $(call filelist, brush_example.cpp item.cpp item_packer.cpp) $(COMMON_DEMO_SOURCES)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
