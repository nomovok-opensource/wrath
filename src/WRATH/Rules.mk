# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

dir := $(d)/util
include $(dir)/Rules.mk

dir := $(d)/drawgroup
include $(dir)/Rules.mk

dir := $(d)/gl
include $(dir)/Rules.mk

dir := $(d)/image
include $(dir)/Rules.mk

dir := $(d)/shape
include $(dir)/Rules.mk

dir := $(d)/text
include $(dir)/Rules.mk

dir := $(d)/items
include $(dir)/Rules.mk

dir := $(d)/layer
include $(dir)/Rules.mk

dir := $(d)/Qt
include $(dir)/Rules.mk

dir := $(d)/SDL
include $(dir)/Rules.mk

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
