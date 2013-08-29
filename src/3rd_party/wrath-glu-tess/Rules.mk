# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

LIB_SOURCES += $(call filelist, dict.cpp geom.cpp memalloc.cpp mesh.cpp normal.cpp priorityq.cpp render.cpp sweep.cpp tess.cpp tessmono.cpp)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
