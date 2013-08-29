# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

dir := $(d)/shaders
include $(dir)/Rules.mk

DEMOS += wavy-clip2

wavy-clip2_SOURCES := $(call filelist, wavy_clip2.cpp rect_attribute_packer.cpp) $(COMMON_DEMO_SOURCES)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
