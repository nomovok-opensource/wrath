# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

DEMOS_INCLUDES += -I$(d)

COMMON_DEMO_SOURCES := $(call filelist, generic_command_line.cpp)
COMMON_SDL_DEMO_SOURCES := $(call filelist, sdl_demo.cpp)
COMMON_QT_DEMO_SOURCES := $(call filelist, qt_demo.cpp)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
