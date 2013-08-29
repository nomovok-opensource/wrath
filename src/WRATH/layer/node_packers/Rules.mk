# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header


LIB_SOURCES += $(call filelist, WRATHLayerNodeValuePackerUniformArrays.cpp WRATHLayerNodeValuePackerTexture.cpp WRATHLayerNodeValuePackerHybrid.cpp)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
