# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header


dir := $(d)/shaders
include $(dir)/Rules.mk

DEMOS += wavy-text

wavy-text_SOURCES := $(call filelist, wavy-text.cpp) $(COMMON_DEMO_SOURCES)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
