# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

SHADERS += $(call filelist, image.vert.wrath-shader.glsl image.frag.wrath-shader.glsl empty_pre_compute_shader_code_highp.wrath-shader.glsl empty_pre_compute_shader_code_noprec.wrath-shader.glsl empty_pre_compute_shader_code_mediump.wrath-shader.glsl linear-gradient-values.compute.wrath-shader.glsl linear-gradient-values.pre-compute.wrath-shader.glsl radial-gradient-values.compute.wrath-shader.glsl radial-gradient-values.pre_compute.wrath-shader.glsl repeat-gradient.pre-compute.wrath-shader.glsl repeat-gradient.wrath-shader.glsl empty_pre_compute_tex_shader_code_highp.wrath-shader.glsl empty_pre_compute_tex_shader_code_mediump.wrath-shader.glsl empty_pre_compute_tex_shader_code_noprec.wrath-shader.glsl image-repeat-mode-functions.wrath-shader.glsl image-value-normalized-coordinate.compute.wrath-shader.glsl image-value-normalized-coordinate.pre-compute.wrath-shader.glsl image-value-normalized-coordinate-dynamic.compute.wrath-shader.glsl image-value-normalized-coordinate-dynamic.pre-compute.wrath-shader.glsl wrath-brush.vert.wrath-shader.glsl wrath-brush.frag.wrath-shader.glsl)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
