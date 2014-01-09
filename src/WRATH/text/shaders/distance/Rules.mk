# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header



SHADERS += $(call filelist, font_distance_linear.frag.wrath-shader.glsl font_distance_nonlinear.frag.wrath-shader.glsl font_distance_linear.vert.wrath-shader.glsl font_distance_nonlinear.vert.wrath-shader.glsl)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
