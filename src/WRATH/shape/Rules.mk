# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

dir := $(d)/shaders
include $(dir)/Rules.mk

LIB_SOURCES += $(call filelist, WRATHDynamicStrokeAttributePacker.cpp WRATHShapeDistanceFieldGPU.cpp WRATHShapePreStroker.cpp WRATHDefaultFillShapeAttributePacker.cpp WRATHGenericStrokeAttributePacker.cpp WRATHShapeTriangulator.cpp WRATHShapeDistanceFieldGPUutil.cpp WRATHShapeSimpleTessellator.cpp WRATHDefaultStrokeAttributePacker.cpp WRATHTessGLU.cpp WRATHDefaultShapeShader.cpp)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
