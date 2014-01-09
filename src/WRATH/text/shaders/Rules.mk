# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header


dir := $(d)/analytic
include $(dir)/Rules.mk

dir := $(d)/coverage
include $(dir)/Rules.mk

dir := $(d)/detailed_coverage
include $(dir)/Rules.mk

dir := $(d)/distance
include $(dir)/Rules.mk

dir := $(d)/common
include $(dir)/Rules.mk

dir := $(d)/curve_analytic
include $(dir)/Rules.mk

dir := $(d)/mix
include $(dir)/Rules.mk

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
