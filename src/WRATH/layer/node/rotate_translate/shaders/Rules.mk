# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

SHADERS += $(call filelist, transformation_layer_rotate_translate.frag.wrath-shader.glsl transformation_layer_rotate_translate.vert.wrath-shader.glsl)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
