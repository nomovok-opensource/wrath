# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

# This dir must be first, others don't matter
dir := $(d)/common
include $(dir)/Rules.mk

dir := $(d)/text_viewer
include $(dir)/Rules.mk

dir := $(d)/image_demo
include $(dir)/Rules.mk

dir := $(d)/cells
include $(dir)/Rules.mk

dir := $(d)/examples
include $(dir)/Rules.mk

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
