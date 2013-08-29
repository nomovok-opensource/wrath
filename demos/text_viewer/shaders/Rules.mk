# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

text-viewer_SHADERS :=$(call filelist, distance_image.frag.glsl simple_const_color.frag.glsl font_animated.frag.glsl simple.frag.glsl simple_ui_shape.vert.glsl simple_ui_font.vert.glsl simple_ui.vert.glsl simple_ui_line.vert.glsl simple_ui_shape.frag.glsl)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
