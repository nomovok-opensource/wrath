# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

QT_LIB_SOURCES += $(call filelist, WRATHQTFontSupport.cpp  WRATHQTImageSupport.cpp ngl_backend_qt.cpp)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
