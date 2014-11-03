# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

dir := $(d)/clip
include $(dir)/Rules.mk

dir := $(d)/clip2
include $(dir)/Rules.mk

dir := $(d)/augmented_node
include $(dir)/Rules.mk

dir := $(d)/augmented_node2
include $(dir)/Rules.mk

dir := $(d)/hello_widget_generator
include $(dir)/Rules.mk

dir := $(d)/hello_widget_generator2
include $(dir)/Rules.mk

dir := $(d)/hello_wrathlayer
include $(dir)/Rules.mk

dir := $(d)/shape
include $(dir)/Rules.mk

dir := $(d)/text
include $(dir)/Rules.mk

dir := $(d)/item_example
include $(dir)/Rules.mk

dir := $(d)/item_example2
include $(dir)/Rules.mk

dir := $(d)/brush_example
include $(dir)/Rules.mk

dir := $(d)/rect
include $(dir)/Rules.mk

dir := $(d)/rect2
include $(dir)/Rules.mk

dir := $(d)/rect3
include $(dir)/Rules.mk

dir := $(d)/wavy_clip2
include $(dir)/Rules.mk

dir := $(d)/wavy-text
include $(dir)/Rules.mk

dir := $(d)/counter
include $(dir)/Rules.mk



# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
