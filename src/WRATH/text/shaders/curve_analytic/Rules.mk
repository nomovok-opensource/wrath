# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header



SHADERS += $(call filelist, font_curve_analytic_base.frag.wrath-shader.glsl font_curve_analytic_linear.frag.wrath-shader.glsl font_curve_analytic_linear.vert.wrath-shader.glsl font_curve_analytic_nonlinear.frag.wrath-shader.glsl font_curve_analytic_nonlinear.vert.wrath-shader.glsl)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
