# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

LIB_SOURCES += $(call filelist, WRATHAttributeStore.cpp WRATHCanvasHandle.cpp WRATHItemDrawer.cpp WRATHAttributePacker.cpp WRATHCanvas.cpp WRATHItemGroup.cpp WRATHShaderSpecifier.cpp WRATHTwoPassDrawer.cpp WRATHBaseSource.cpp WRATHItemDrawState.cpp WRATHIndexGroupAllocator.cpp) 

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
