# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

cells_SHADERS :=$(call filelist, simple_ui_shape.frag.glsl simple_ui_shape_translate_layer.vert.glsl)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
