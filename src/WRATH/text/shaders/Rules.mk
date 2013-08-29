# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

SHADERS += $(call filelist, font_analytic_base.frag.wrath-shader.glsl font_curve_analytic_base.frag.wrath-shader.glsl font_generic_aa.frag.wrath-shader.glsl font_common_base.frag.wrath-shader.glsl font_common_base.vert.wrath-shader.glsl font_detailed_base.frag.wrath-shader.glsl font_generic.frag.wrath-shader.glsl simple_ui_font.vert.wrath-shader.glsl font_coverage_base.frag.wrath-shader.glsl font_distance_base.frag.wrath-shader.glsl font_mix_base.frag.wrath-shader.glsl)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
