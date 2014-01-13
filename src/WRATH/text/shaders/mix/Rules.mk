# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header



SHADERS += $(call filelist, font_mix_page_data_func.wrath-shader.glsl font_mix_linear.frag.wrath-shader.glsl font_mix_linear.vert.wrath-shader.glsl font_mix_nonlinear.frag.wrath-shader.glsl font_mix_nonlinear.vert.wrath-shader.glsl)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
