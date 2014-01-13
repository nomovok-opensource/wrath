# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header



SHADERS += $(call filelist, simple_ui_font.vert.wrath-shader.glsl font_generic_aa.frag.wrath-shader.glsl font_generic.frag.wrath-shader.glsl font_shader_texture_page_data.wrath-shader.glsl font_shader_wrath_prepare_glyph_vs.vert.wrath-shader.glsl)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
