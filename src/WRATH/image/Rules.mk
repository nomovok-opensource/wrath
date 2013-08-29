# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

dir := $(d)/shaders
include $(dir)/Rules.mk

LIB_SOURCES += $(call filelist, WRATHDefaultRectAttributePacker.cpp WRATHImage.cpp WRATHGradientSourceBase.cpp WRATHGradientSource.cpp WRATHGradient.cpp WRATHColorValueSource.cpp WRATHLinearGradientValue.cpp WRATHRadialGradientValue.cpp WRATHRepeatGradientValue.cpp WRATHGradientValueBase.cpp WRATHTextureCoordinateSourceBase.cpp WRATHTextureCoordinateSource.cpp WRATHTextureCoordinate.cpp WRATHTextureCoordinateDynamic.cpp WRATHBrush.cpp WRATHShaderBrushSourceHoard.cpp WRATHDefaultRectShader.cpp)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
