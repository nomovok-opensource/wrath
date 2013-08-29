# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

dir := $(d)/translate
include $(dir)/Rules.mk

dir := $(d)/rotate_translate
include $(dir)/Rules.mk

INCLUDES += -I$(d)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
