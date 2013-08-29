# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

LIB_SOURCES += $(call filelist, WRATHBaseItem.cpp WRATHShapeItem.cpp WRATHTextItem.cpp WRATHRectItem.cpp WRATHBasicTextItem.cpp)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
