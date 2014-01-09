# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header



SHADERS += $(call filelist, font_detailed_base.frag.wrath-shader.glsl font_detailed_nonlinear.frag.wrath-shader.glsl font_detailed_linear.frag.wrath-shader.glsl font_detailed_nonlinear.vert.wrath-shader.glsl font_detailed_linear.vert.wrath-shader.glsl)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
