# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

dir := $(d)/shaders
include $(dir)/Rules.mk

LIB_SOURCES += $(call filelist, WRATHColumnFormatter.cpp WRATHDefaultTextAttributePacker.cpp WRATHFontConfig.cpp WRATHFontShaderSpecifier.cpp WRATHFormattedTextStream.cpp WRATHFreeTypeSupport.cpp WRATHGenericTextAttributePacker.cpp WRATHTextAttributePacker.cpp WRATHTextDataStream.cpp WRATHTextureFont.cpp WRATHTextureFontDrawer.cpp WRATHTextureFontFreeType_Analytic.cpp WRATHTextureFontFreeType_Distance.cpp WRATHTextureFontFreeType_Coverage.cpp WRATHTextureFontFreeType_CurveAnalytic.cpp WRATHTextureFontFreeType_DetailedCoverage.cpp WRATHTextureFontFreeType_Mix.cpp WRATHTextureFontUtil.cpp WRATHTextDataStreamManipulator.cpp WRATHFontDatabase.cpp WRATHFontFetch.cpp)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
