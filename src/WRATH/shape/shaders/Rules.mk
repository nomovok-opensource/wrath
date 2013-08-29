# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

SHADERS += $(call filelist, shape.frag.wrath-shader.glsl shape.vert.wrath-shader.glsl distance_field_draw_distance_points.frag.wrath-shader.glsl distance_field_draw_distance_points.vert.wrath-shader.glsl distance_field_draw_distance_rects.frag.wrath-shader.glsl distance_field_draw_distance_rects.vert.wrath-shader.glsl distance_field_simple_shader.frag.wrath-shader.glsl distance_field_simple_shader.vert.wrath-shader.glsl)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
