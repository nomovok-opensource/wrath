/*! 
 * \file WRATHFontShaderSpecifier.hpp
 * \brief file WRATHFontShaderSpecifier.hpp
 * 
 * Copyright 2013 by Nomovok Ltd.
 * 
 * Contact: info@nomovok.com
 * 
 * This Source Code Form is subject to the
 * terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with
 * this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 * 
 * \author Kevin Rogovin <kevin.rogovin@nomovok.com>
 * 
 */




#ifndef __WRATH_FONT_SHADER_SPECIFIER_HPP__
#define __WRATH_FONT_SHADER_SPECIFIER_HPP__

#include "WRATHConfig.hpp"
#include "WRATHShaderSpecifier.hpp"
#include "WRATHTextureFont.hpp"
#include "WRATHTextDataStream.hpp"
#include "WRATHTextDataStreamManipulator.hpp"
#include "WRATHTextAttributePacker.hpp"
#include "WRATHBrush.hpp"

/*! \addtogroup Text
 * @{
 */

/*!\class WRATHFontShaderSpecifier
  A WRATHFontShaderSpecifier purpose is an
  analogue of WRATHShaderSpecifier, but specific
  for texture font rendering. An instance of
  WRATHFontShaderSpecifier produces (and holds) 
  WRATHTextureFontDrawer objects which are
  manufactured as combination from a user provided
  vertex shader, _generic_ fragment shader and
  the WRATHTextureFont::GlyphGLSL data
  of a font type. Changing, querying, etc
  what WRATHFontShaderSpecifier is applied
  in a \ref WRATHTextDataStream is done
  by the manipulator created with
  \ref WRATHText::font_shader.

  When rendering a glyph so that the
  fragment position within the glyph is linear
  (i.e. \ref is true), the user defined
  vertex shader source code must call

  \code
  wrath_font_prepare_glyph_vs(in vec2 glyph_position,
                              in vec2 glyph_bottom_left,
                              in vec2 glyph_size)
  \endcode

  and use
  \code
  is_covered()
  \endcode
  or
  \code
  compute_coverage()
  \endcode
  in the fragment shader. For the nonlinear case,
  the vertex function is
  \code
  wrath_font_prepare_glyph_vs(in vec2 glyph_bottom_left,
                              in vec2 glyph_size)
  \endcode

  and the fragment shader functions are
  \code
  is_covered(in vec2 glyph_position)
  \endcode
  or
  \code
  compute_coverage(in vec2 glyph_position)
  \endcode

  
 */
class WRATHFontShaderSpecifier
{
public:
  /*!\typedef ResourceKey
    Resource key type for WRATHFontShaderSpecifier 
    resource manager.
   */
  typedef std::string ResourceKey; 

  /// @cond
  WRATH_RESOURCE_MANAGER_DECLARE(WRATHFontShaderSpecifier, ResourceKey);
  /// @endcond

  /*!\fn WRATHFontShaderSpecifier(const ResourceKey &,
                                  const WRATHGLShader::shader_source&,
                                  const WRATHGLShader::shader_source &,
                                  const WRATHGLProgramInitializerArray &,
                                  const WRATHGLProgramOnBindActionArray &)
    Ctor.
    The vertex shader of a WRATHFontShaderSpecifier needs to obey the following
    conventions:
    - It must produce 3 varyings: tex_coord, relative_coord, GlyphNormalizedCoordinate(vec2).
      In addition, for font types that make use, GlyphIndex(float). GlyphIndex
      is used by \ref WRATHTextureFontFreeType_DetailedCoverage 
      and \ref WRATHTextureFontFreeType_CurveAnalytic.
    - If the macro MIX_FONT_SHADER is defined both tex_coord and relative_coord are to
      be vec4's with .xy holding the data for the font at native resolution
      and .zw holding the data for the font at minified resolution. If 
      MIX_FONT_SHADER is not defined, each must be a vec2.
    - tex_coord is to hold the _texture_ coordinate for the vertex of the glyph.
    - relative_coord is to hold the position of the vertex relative to the glyph,
      i.e. the bottom left corner vertex has value (0,0) and the top right corner
      has value (width, height) where width is the width of the glyph unscaled 
      (i.e. how many texel across the glyph occupies in the texture) and height 
      is the height of the glyph unscaled.

    The fragment shader of a WRATHFontShaderSpecifier needs to obey the following 
    conventions:
    - It must use either (or both) the functions is_covered() and/or
      compute_coverage() to determine how to draw a fragment. The function
      is_covered() returns 0.0 if the fragment has coverage less that 50%
      from the glyph and 1.0 otherwise. The function compute_coverage() returns
      a value in the range [0.0, 1.0] indicating the coverage of the glyph
      on the fragment.
    - The macros WRATH_IS_OPAQUE_PASS, WRATH_IS_TRANSLUCENT_PASS, WRATH_IS_PURE_TRANSLUCENT_PASS are
      defined for different passes of drawing text. The macro WRATH_IS_OPAQUE_PASS is
      defined if and only if the drawing pass is to draw the entirely opaque
      portions of solid text. The macro WRATH_IS_TRANSLUCENT_PASS is defined if and only
      if the drawing pass is to draw the AA portions of solid text. The macro
      WRATH_IS_PURE_TRANSLUCENT_PASS is defined if and only if the drawer is drawing
      text that is viewed as transparent. In addition, when either of WRATH_IS_OPAQUE_PASS
      and WRATH_IS_TRANSLUCENT_PASS are defined, the macro WRATH_TRANSLUCENT_THRESHOLD should
      be used in the opaque pass (i.e. WRATH_IS_OPAQUE_PASS is defined) to discard those 
      fragments whose coverage is less than WRATH_TRANSLUCENT_THRESHOLD and for the 
      translucent pass (i.e. WRATH_IS_TRANSLUCENT_PASS is defined) set gl_FragColor.a=0.0 (alpha) 
      for when the coverage matches or exceeds WRATH_TRANSLUCENT_THRESHOLD.

    A default font vertex shader is provided by the static method \ref
    default_vertex_shader().

    Two default fragment shaders are provided \ref default_non_aa_fragment_shader()
    and \ref default_aa_fragment_shader().

    In addition two WRATHFontShaderSpecifier are ready using the default
    vertex and fragment shaders, \ref default_non_aa() and \ref default_aa().


    \param pname resource name of the WRATHFontShaderSpecifier
    \param vs initial value to give the vertex shader source (see \ref vertex_shader_source())
    \param fs initial value to give the fragment shader source (see \ref fragment_shader_source())
    \param initers initial value to give to the initializers (see \ref initializers())
    \param on_bind_actions initial value to give the bind actions (see \ bind_actions())
   */
  explicit
  WRATHFontShaderSpecifier(const ResourceKey &pname,
                           const WRATHGLShader::shader_source &vs=WRATHGLShader::shader_source(),
                           const WRATHGLShader::shader_source &fs=WRATHGLShader::shader_source(),
                           const WRATHGLProgramInitializerArray &initers=WRATHGLProgramInitializerArray(),
                           const WRATHGLProgramOnBindActionArray &on_bind_actions=WRATHGLProgramOnBindActionArray());

  /*!\fn WRATHFontShaderSpecifier(const WRATHGLShader::shader_source&,
                                  const WRATHGLShader::shader_source&,
                                  const WRATHGLProgramInitializerArray&,
                                  const WRATHGLProgramOnBindActionArray&)
    Ctor to create a WRATHFontShaderSpecifier which is not on
    the resource manager of WRATHFontShaderSpecifier.
    \param vs initial value to give the vertex shader source (see \ref vertex_shader_source())
    \param fs initial value to give the fragment shader source (see \ref fragment_shader_source())
    \param initers initial value to give to the initializers (see \ref initializers())
    \param on_bind_actions initial value to give the bind actions (see \ bind_actions())
   */
  explicit
  WRATHFontShaderSpecifier(const WRATHGLShader::shader_source &vs=WRATHGLShader::shader_source(),
                           const WRATHGLShader::shader_source &fs=WRATHGLShader::shader_source(),
                           const WRATHGLProgramInitializerArray &initers=WRATHGLProgramInitializerArray(),
                           const WRATHGLProgramOnBindActionArray &on_bind_actions=WRATHGLProgramOnBindActionArray());

  ~WRATHFontShaderSpecifier();

  /*!\fn const WRATHGLShader::shader_source& default_vertex_shader
    Returns a default vertex shader that works with
    \ref WRATHDefaultTextAttributePacker for vertex
    shading of font drawing.
   */
  static
  const WRATHGLShader::shader_source&
  default_vertex_shader(void);
  
  /*!\fn const WRATHGLShader::shader_source& default_aa_fragment_shader
    Returns a default fragment shader for drawing
    anti-aliased text.
   */
  static
  const WRATHGLShader::shader_source&
  default_aa_fragment_shader(void);
  
  /*!\fn const WRATHGLShader::shader_source& default_non_aa_fragment_shader
    Returns a default fragment shader for drawing
    NON-anti-aliased text.
   */
  static
  const WRATHGLShader::shader_source&
  default_non_aa_fragment_shader(void);

  /*!\fn const WRATHFontShaderSpecifier& default_aa
    Returns the pre-built WRATHFontShaderSpecifier
    for drawing AA-text. 
   */
  static
  const WRATHFontShaderSpecifier&
  default_aa(void);

  /*!\fn const WRATHFontShaderSpecifier& default_brush_item_aa
    Returns a pre-built WRATHFontShaderSpecifier
    for drawing AA-text with a brush applied on
    item coordinates.
    \param brush \ref WRATHShaderBrush specifing shader code 
                 to apply gradient, color and/or texture
   */
  static
  const WRATHFontShaderSpecifier&
  default_brush_item_aa(const WRATHShaderBrush &brush);

  /*!\fn const WRATHFontShaderSpecifier& default_brush_letter_aa
    Returns a pre-built WRATHFontShaderSpecifier
    for drawing AA-text with a brush applied on
    letter coordinates. 
    \param brush \ref WRATHShaderBrush specifing shader code 
                 to apply gradient, color and/or texture
   */
  static
  const WRATHFontShaderSpecifier&
  default_brush_letter_aa(const WRATHShaderBrush &brush);

  /*!\fn const WRATHFontShaderSpecifier& default_non_aa
    Returns the pre-built WRATHFontShaderSpecifier
    for drawing non-AA text. 
   */
  static
  const WRATHFontShaderSpecifier&
  default_non_aa(void);

  /*!\fn const WRATHFontShaderSpecifier& default_brush_item_non_aa
    Returns a pre-built WRATHFontShaderSpecifier
    for drawing non-AA text with a brush applied on
    item coordinates.
    \param brush \ref WRATHShaderBrush specifing shader code 
                 to apply gradient, color and/or texture
   */
  static
  const WRATHFontShaderSpecifier&
  default_brush_item_non_aa(const WRATHShaderBrush &brush);

  /*!\fn const WRATHFontShaderSpecifier& default_brush_letter_non_aa
    Returns a pre-built WRATHFontShaderSpecifier
    for drawing non-AA text with a brush applied on
    letter coordinates. 
    \param brush \ref WRATHShaderBrush specifing shader code 
                 to apply gradient, color and/or texture
   */
  static
  const WRATHFontShaderSpecifier&
  default_brush_letter_non_aa(const WRATHShaderBrush &brush);

  /*!\fn const ResourceKey& resource_name
    returns the resource name of this WRATHFontShaderSpecifier.
   */
  const ResourceKey&
  resource_name(void) const
  {
    return m_resource_name;
  }

  /*!\fn WRATHGLProgramOnBindActionArray& append_bind_actions
    Returns a reference to the WRATHGLProgramOnBindActionArray
    object of this WRATHShaderSpecifier. Modify the returned
    object to specify actions to be executed on the _each_ 
    time a GLSL program created with this WRATHShaderSpecifier 
    is bound. It is an error to add (or remove) bind actions
    after the first call \ref fetch_texture_font_drawer(). 
    Do NOT use append_bind_actions()
    to specify texture units for additional textures. Use 
    add_sampler() and remove_sampler() to add and remove
    additional textures.
   */
  WRATHGLProgramOnBindActionArray&
  append_bind_actions(void)
  {
    WRATHassert(m_modifiable);
    return m_bind_actions;
  }

  /*!\fn const WRATHGLProgramOnBindActionArray& bind_actions
    Returns a const reference of the bind actions of this
    WRATHShaderSpecifier. 
   */
  const WRATHGLProgramOnBindActionArray&
  bind_actions(void) const
  {
    return m_bind_actions;
  }

  /*!\fn bool linear_glyph_position(void) const
    Returns true if the font shader code is to 
    compute the glyph positional values in
    the vertex shader. Default value is true
   */
  bool 
  linear_glyph_position(void) const
  {
    return m_linear_glyph_position;
  }

  /*!\fn void linear_glyph_position(bool) 
    Sets true if the font shader code is to 
    compute the glyph positional values in
    the vertex shader. Default value is true
    \param b value with which to assign
   */
  void
  linear_glyph_position(bool b)
  {
    WRATHassert(m_modifiable);
    m_linear_glyph_position=b;
  }

  /*!\fn WRATHGLProgramInitializerArray& append_initializers
    Returns a reference to the WRATHGLProgramInitializerArray
    object of this WRATHShaderSpecifier. Modify the returned
    object to specify actions (typically setting of uniforms)
    to be executed on the _first_ time a GLSL program created
    with this WRATHShaderSpecifier is bound. It is an error to 
    add (or remove) initializers after the first call \ref 
    fetch_texture_font_drawer(). Do NOT use append_initializers()
    to specify texture units for additional textures. Use 
    add_sampler() and remove_sampler() to add and remove
    additional textures.
   */
  WRATHGLProgramInitializerArray&
  append_initializers(void)
  {
    WRATHassert(m_modifiable);
    return m_initializers;
  }

  /*!\fn const WRATHGLProgramInitializerArray& initializers
    Returns a const reference of the initializes of this
    WRATHShaderSpecifier. 
   */
  const WRATHGLProgramInitializerArray&
  initializers(void) const
  {
    return m_initializers;
  }

  /*!\fn WRATHGLShader::shader_source& append_shader_source
    Returns a reference for the shader source
    code object for the named shader type.
    Modify the returned object to specify the
    shader source code for the named shader type.
    It is an error to add (or remove) source code 
    after the first call \ref fetch_texture_font_drawer().
    \param v shader type as a GLenum, for example 
             GL_VERTEX_SHADER for a vertex shader
             and GL_FRAGMENT_SHADER for a fragment
             shader
   */
  WRATHGLShader::shader_source&
  append_shader_source(GLenum v)
  {
    WRATHassert(m_modifiable);
    return m_shader_source_code[v];
  }

  /*!\fn std::map<GLenum, WRATHGLShader::shader_source>& append_all_shader_sources
    Returns a reference of all the
    shader source code object of this
    WRATHFontShaderSpecifier as an std::map
    with values WRATHGLShader::shader_source
    obects keyed by shader type.
   */
  std::map<GLenum, WRATHGLShader::shader_source>&
  append_all_shader_sources(void) 
  {
    WRATHassert(m_modifiable);
    return m_shader_source_code;
  }

  /*!\fn void add_shader_source_code
    Add the shader source code from a WRATHBaseSource
    object. 
    \param src source code to add
    \param prec precision qaulifier to use on the added source code
    \param suffix suffix to which to append to all function, macros, etc
                  added to the code of src
   */
  void
  add_shader_source_code(const WRATHBaseSource *src,
                         enum WRATHBaseSource::precision_t prec,
                         const std::string &suffix="");

  /*!\fn WRATHGLShader::shader_source& append_pre_shader_source
    Returns a reference for the pre-shader source
    code object for the named shader type.
    Modify the returned object to specify the
    shader source code for the named shader type.
    It is an error to add (or remove) source code 
    after the first call \ref fetch_texture_font_drawer().
    \param v shader type as a GLenum, for example 
             GL_VERTEX_SHADER for a vertex shader
             and GL_FRAGMENT_SHADER for a fragment
             shader
   */
  WRATHGLShader::shader_source&
  append_pre_shader_source(GLenum v)
  {
    WRATHassert(m_modifiable);
    return m_pre_shader_source_code[v];
  }

  /*!\fn std::map<GLenum, WRATHGLShader::shader_source>& append_all_pre_shader_sources
    Returns a reference of all the
    pre-shader source code object of this
    WRATHFontShaderSpecifier as an std::map
    with values WRATHGLShader::shader_source
    obects keyed by shader type.
   */
  std::map<GLenum, WRATHGLShader::shader_source>&
  append_all_pre_shader_sources(void) 
  {
    WRATHassert(m_modifiable);
    return m_pre_shader_source_code;
  }

  /*!\fn void add_pre_shader_source_code
    Add the shader source code from a WRATHBaseSource
    object to pre-shader code. 
    \param src source code to add
    \param prec precision qaulifier to use on the added source code
    \param suffix suffix to which to append to all function, macros, etc
                  added to the code of src
   */
  void
  add_pre_shader_source_code(const WRATHBaseSource *src,
                             enum WRATHBaseSource::precision_t prec,
                             const std::string &suffix="");

  /*!\fn const WRATHGLShader::shader_source& shader_source
    Returns a const refernce to the shader
    source code for the named shader type.
    If the named shader type does not exist,
    returns a const reference to a shader
    source code object that is empty.
    \param v shader type as a GLenum, for example 
             GL_VERTEX_SHADER for a vertex shader
             and GL_FRAGMENT_SHADER for a fragment
             shader
   */
  const WRATHGLShader::shader_source&
  shader_source(GLenum v) const
  {
    std::map<GLenum, WRATHGLShader::shader_source>::const_iterator iter;

    iter=m_shader_source_code.find(v);
    return (iter!=m_shader_source_code.end())?
      iter->second:
      m_empty_source;
  }

  /*!\fn const WRATHGLShader::shader_source& pre_shader_source
    Returns a const refernce to the pre-shader
    source code for the named shader type.
    If the named shader type does not exist,
    returns a const reference to a shader
    source code object that is empty.
    \param v shader type as a GLenum, for example 
             GL_VERTEX_SHADER for a vertex shader
             and GL_FRAGMENT_SHADER for a fragment
             shader
   */
  const WRATHGLShader::shader_source&
  pre_shader_source(GLenum v) const
  {
    std::map<GLenum, WRATHGLShader::shader_source>::const_iterator iter;

    iter=m_pre_shader_source_code.find(v);
    return (iter!=m_pre_shader_source_code.end())?
      iter->second:
      m_empty_source;
  }

  /*!\fn WRATHFontShaderSpecifier& add_sampler
    A WRATHFontShaderSpecifier may also have 
    use samplers to perform it's custom font
    shading. These sampler's are assigned
    locations _AFTER_ the locations of
    the font being drawn. For example, if the
    WRATHFontShaderSpecifier is drawing a
    font F which uses 2 texture units, then
    GL_TEXTURE0 and GL_TEXTURE1 are used by F
    and GL_TEXTUREn, where n=2+S is reserved
    for the sampler at of id S of the 
    WRATHFontShaderSpecifier.

    It is an error to add (or remove) a sampler 
    after the first call \ref fetch_texture_font_drawer().

    \param S sample integer ID
    \param glsl_uniform_name name of sampler appearing in GLSL code
   */
  WRATHFontShaderSpecifier&
  add_sampler(unsigned int S, const std::string &glsl_uniform_name)
  {
    m_additional_textures[S]=glsl_uniform_name;
    return *this;
  }

  /*!\fn WRATHFontShaderSpecifier& remove_sampler
    Remove a sampler added by add_sampler
    
    It is an error to add (or remove) a sampler 
    after the first call \ref fetch_texture_font_drawer().

    \param S sampler integer ID to remove
   */
  WRATHFontShaderSpecifier&
  remove_sampler(unsigned int S)
  {
    WRATHassert(m_modifiable);
    m_additional_textures.erase(S);
    return *this;
  }

  /*!\fn const std::map<unsigned int, std::string>& additional_samplers
    Returns an std::map, keyed by sampler integer
    ID with names as GLSL uniforms of samplers 
    add via \ref add_sampler().
   */
  const std::map<unsigned int, std::string>&
  additional_samplers(void) const
  {
    return m_additional_textures;
  }

  /*!\fn bool has_additional_sampler
    Provided as a conveninage, equivalent to
    \code
    additional_samplers().find(T)!=additional_samplers().end()
    \endcode
   */
  bool
  has_additional_sampler(unsigned int T) const
  {
    return m_additional_textures.find(T)!=m_additional_textures.end();
  }

  /*!\fn const std::map<GLenum, WRATHGLShader::shader_source>& all_shader_sources
    Returns a const reference of all the
    shader source code object of this
    WRATHShaderSpecifier as an std::map
    with values WRATHGLShader::shader_source
    obects keyed by shader type.
   */
  const std::map<GLenum, WRATHGLShader::shader_source>&
  all_shader_sources(void) const
  {
    return m_shader_source_code;
  }

  /*!\fn const std::map<GLenum, WRATHGLShader::shader_source>& all_pre_shader_sources
    Returns a const reference of all the
    pre-shader source code object of this
    WRATHShaderSpecifier as an std::map
    with values WRATHGLShader::shader_source
    obects keyed by shader type.
   */
  const std::map<GLenum, WRATHGLShader::shader_source>&
  all_pre_shader_sources(void) const
  {
    return m_pre_shader_source_code;
  }

  /*!\fn WRATHGLShader::shader_source& append_vertex_shader_source
    Provided as a conveniance, equivalent
    to calling shader_source(GLenum) passing
    GL_VERTEX_SHADER as the argument
   */
  WRATHGLShader::shader_source&
  append_vertex_shader_source(void)
  {
    return append_shader_source(GL_VERTEX_SHADER);
  }

  /*!\fn const WRATHGLShader::shader_source& vertex_shader_source
    Provided as a conveniance, equivalent
    to calling shader_source(GLenum) const
    passing GL_VERTEX_SHADER as the argument
   */
  const WRATHGLShader::shader_source&
  vertex_shader_source(void) const
  {
    return shader_source(GL_VERTEX_SHADER);
  }

  /*!\fn WRATHGLShader::shader_source& append_pre_vertex_shader_source
    Provided as a conveniance, equivalent
    to calling append_pre_shader_source(GLenum) 
    passing GL_VERTEX_SHADER as the argument
   */
  WRATHGLShader::shader_source&
  append_pre_vertex_shader_source(void)
  {
    return append_pre_shader_source(GL_VERTEX_SHADER);
  }

  /*!\fn const WRATHGLShader::shader_source& vertex_pre_shader_source
    Provided as a conveniance, equivalent
    to calling pre_shader_source(GLenum) const
    passing GL_VERTEX_SHADER as the argument
   */
  const WRATHGLShader::shader_source&
  vertex_pre_shader_source(void) const
  {
    return pre_shader_source(GL_VERTEX_SHADER);
  }

  /*!\fn WRATHGLShader::shader_source& append_fragment_shader_source
    Provided as a conveniance, equivalent
    to calling append_shader_source(GLenum) passing
    GL_FRAGMENT_SHADER as the argument
   */
  WRATHGLShader::shader_source&
  append_fragment_shader_source(void)
  {
    return append_shader_source(GL_FRAGMENT_SHADER);
  }

  /*!\fn const WRATHGLShader::shader_source& fragment_shader_source
    Provided as a conveniance, equivalent
    to calling shader_source(GLenum) const
    passing GL_FRAGMENT_SHADER as the argument
   */
  const WRATHGLShader::shader_source&
  fragment_shader_source(void) const
  {
    return shader_source(GL_FRAGMENT_SHADER);
  }

  /*!\fn WRATHGLShader::shader_source& append_pre_fragment_shader_source
    Provided as a conveniance, equivalent
    to calling append_pre_shader_source(GLenum)
    passing GL_FRAGMENT_SHADER as the argument
   */
  WRATHGLShader::shader_source&
  append_pre_fragment_shader_source(void)
  {
    return append_pre_shader_source(GL_FRAGMENT_SHADER);
  }

  /*!\fn const WRATHGLShader::shader_source& fragment_pre_shader_source
    Provided as a conveniance, equivalent
    to calling pre_shader_source(GLenum) const
    passing GL_FRAGMENT_SHADER as the argument
   */
  const WRATHGLShader::shader_source&
  fragment_pre_shader_source(void) const
  {
    return pre_shader_source(GL_FRAGMENT_SHADER);
  }

  /*!\fn float font_discard_thresh(void) const
    Returns the threshhold for this WRATHFontShaderSpecifier
    used to consider that a fragment is entirely covered
    by the glyph. The default value is 0.9.
   */
  float 
  font_discard_thresh(void) const
  {
    return m_font_discard_thresh;
  }

  /*!\fn void font_discard_thresh(float)
    Sets the threshhold for this WRATHFontShaderSpecifier
    used to consider that a fragment is entirely covered
    by the glyph. The default value is 0.9. 
    It is an error to set this value after the first
    call to \ref fetch_texture_font_drawer().
   */
  void
  font_discard_thresh(float v)
  {
    WRATHassert(m_modifiable);
    m_font_discard_thresh=v;
  }

  /*!\fn WRATHTextureFontDrawer* fetch_texture_font_drawer(const WRATHTextureFont::GlyphGLSL*,
                                                           const WRATHItemDrawerFactory&,
                                                           const WRATHTextAttributePacker*,
                                                           int) const
    Fetch, and if necessary first create, a WRATHTextureFontDrawer
    object using the named WRATHTextureFont::GlyphGLSL object
    to implement is_covered() and compute_coverage(). The WRATHTextureFontDrawer
    are stored in the creating WRATHFontShaderSpecifier object.
    Hence, one should save one's created WRATHFontShaderSpecifier.
    Recall that  A WRATHTextureFontDrawer
    has _3_ WRATHItemDrawer objects:
    - WRATHTextureFontDrawer::opaque_pass_drawer() drawer 
      for opaque pass for drawing solid text
    - WRATHTextureFontDrawer::translucent_pass_drawer() drawer 
      for translucent pass for drawing the AA portions of solid text
    - WRATHTextureFontDrawer::translucent_only_drawer() drawer
      for text that is purely transparent

    Each of these drawers has an additional macro added
    to both their vertex and fragment shaders:
    - WRATH_IS_OPAQUE_PASS for the opaque pass of solid text
    - WRATH_IS_TRANSLUCENT_PASS for the translucent pass for AA-portions of solid text
    - WRATH_IS_PURE_TRANSLUCENT_PASS for text the drawer to draw purely transparent text
    \param fs_source WRATHTextureFont::GlyphGLSL object, typically
                     taken from a WRATHTextureFont derived object
    \param factory WRATHItemDrawerFactory to specify per item
                   and per draw group collection transformation source
                   code
    \param attribute_packer WRATHTextAttributePacker to be used to specify
                            the binding point on user defined attributes
    \param sub_drawer_id SubDrawerId passed to the WRATHItemDrawerFactory 
                         factor object.
   */
  WRATHTextureFontDrawer*
  fetch_texture_font_drawer(const WRATHTextureFont::GlyphGLSL *fs_source,
                            const WRATHItemDrawerFactory &factory,
                            const WRATHTextAttributePacker *attribute_packer,
                            int sub_drawer_id) const;

  /*!\fn WRATHTextureFontDrawer* fetch_texture_font_drawer(WRATHTextureFont*,
                                                           const WRATHItemDrawerFactory&,
                                                           const WRATHTextAttributePacker*,
                                                           int) const
    Provided as a conveniance, equivalent to:
    \code
    fetch_texture_font_drawer(font->glyph_glsl(), factory, attribute_packer, sub_drawer_id); 
    \endcode
    \param font WRATHTextureFont derived object from which to take the
                WRATHTextureFont::GlyphGLSL object
    \param factory WRATHItemDrawerFactory to specify per item
                   and per draw group collection transformation source
                   code
    \param attribute_packer WRATHTextAttributePacker to be used to specify
                            the binding point on user defined attributes
    \param sub_drawer_id SubDrawerId passed to the WRATHItemDrawerFactory 
                         factor object.
   */
  WRATHTextureFontDrawer*
  fetch_texture_font_drawer(WRATHTextureFont *font,
                            const WRATHItemDrawerFactory &factory,
                            const WRATHTextAttributePacker *attribute_packer,
                            int sub_drawer_id) const
  {
    return fetch_texture_font_drawer(font->glyph_glsl(),
                                     factory, attribute_packer, sub_drawer_id);
  }

private:

 
  typedef WRATHTextureFont::GlyphGLSL GlyphGLSL;
  typedef std::map<const GlyphGLSL*, WRATHShaderSpecifier*> map_type;
  
  ResourceKey m_resource_name;
  bool m_remove_from_manager;

  std::map<GLenum, WRATHGLShader::shader_source> m_shader_source_code;
  std::map<GLenum, WRATHGLShader::shader_source> m_pre_shader_source_code;

  std::map<unsigned int, std::string> m_additional_textures;

  WRATHGLProgramInitializerArray m_initializers;
  WRATHGLProgramOnBindActionArray m_bind_actions;
  mutable bool m_modifiable;
  
  float m_font_discard_thresh;
  WRATHGLShader::shader_source m_empty_source;

  bool m_linear_glyph_position;

  mutable WRATHMutex m_mutex;
  mutable map_type m_actual_creators;
  
};



namespace WRATHText
{
  
  /*!\class WRATHText::font_shader
    See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY.
    Determines the WRATHFontShaderSpecifier used.
     - streams not-initialized with any value 
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(font_shader, const WRATHFontShaderSpecifier*)
    
  /*!\fn font_shader::set_type set_font_shader_brush_letter_aa(const WRATHShaderBrush&)
    Conveniance function to set the brush using WRATHFontShaderSpecifier::default_brush_letter_aa(),
    equivalent to
    \code
    set_font_shader(&WRATHFontShaderSpecifier::default_brush_letter_aa(brush));
    \endcode
    \param brush WRATHShaderBrush to specify the WRATHFontShaderSpecifier
   */
  inline
  font_shader::set_type
  set_font_shader_brush_letter_aa(const WRATHShaderBrush &brush)
  {
    return set_font_shader(&WRATHFontShaderSpecifier::default_brush_letter_aa(brush));
  }
  
  /*!\fn font_shader::set_type set_font_shader_brush_letter_non_aa(const WRATHShaderBrush&)
    Conveniance function to set the brush using WRATHFontShaderSpecifier::default_brush_letter_non_aa(),
    equivalent to
    \code
    set_font_shader(&WRATHFontShaderSpecifier::default_brush_letter_non_aa(brush));
    \endcode
    \param brush WRATHShaderBrush to specify the WRATHFontShaderSpecifier
   */
  inline
  font_shader::set_type
  set_font_shader_brush_letter_non_aa(const WRATHShaderBrush &brush)
  {
    return set_font_shader(&WRATHFontShaderSpecifier::default_brush_letter_non_aa(brush));
  }

  /*!\fn font_shader::set_type set_font_shader_brush_item_aa(const WRATHShaderBrush&)
    Conveniance function to set the brush using WRATHFontShaderSpecifier::default_brush_item_aa(),
    equivalent to
    \code
    set_font_shader(&WRATHFontShaderSpecifier::default_brush_item_aa(brush));
    \endcode
    \param brush WRATHShaderBrush to specify the WRATHFontShaderSpecifier
   */
  inline
  font_shader::set_type
  set_font_shader_brush_item_aa(const WRATHShaderBrush &brush)
  {
    return set_font_shader(&WRATHFontShaderSpecifier::default_brush_item_aa(brush));
  }
  
  /*!\fn font_shader::set_type set_font_shader_brush_item_non_aa(const WRATHShaderBrush&)
    Conveniance function to set the brush using WRATHFontShaderSpecifier::default_brush_item_non_aa(),
    equivalent to
    \code
    set_font_shader(&WRATHFontShaderSpecifier::default_brush_item_non_aa(brush));
    \endcode
    \param brush WRATHShaderBrush to specify the WRATHFontShaderSpecifier
   */
  inline
  font_shader::set_type
  set_font_shader_brush_item_non_aa(const WRATHShaderBrush &brush)
  {
    return set_font_shader(&WRATHFontShaderSpecifier::default_brush_item_non_aa(brush));
  }

  /*!\var number_additional_textures_supported
    Number of additional texture samplers that are supported
    via the stream manipulator class \ref additional_texture.
   */
  const int number_additional_textures_supported=8;

  /*!\fn int stream_id_additional_texture(int)
    Provided as a conveniance, returns the stream ID
    for the named additional texture to apply to text
    drawing
    \param S additional texture ID must be that 0 <= S < \ref number_additional_textures_supported
   */
  inline
  int
  stream_id_additional_texture(int S)
  {
    WRATHassert(S>=0 and S<number_additional_textures_supported);
    return -1-S;
  }

  /*!\typedef additional_texture
    Conveniance typedef to 
    to specify an additional texture to
    use in addition to the textures from
    a WRATHTextureFont.
   */
  typedef WRATHTextureChoice::texture_base::handle additional_texture;
  
  /*!\fn set_state_type<additional_texture> set_additional_sampler(int, const additional_texture&)
    "Manipulator" to set an additional sampler value
    \param S which additional sampler (must be 0<=S<number_additional_textures_supported)
             S corresponts to the sampler ID of \ref WRATHFontShaderSpecifier::add_sampler()
             and \ref WRATHFontShaderSpecifier::remove_sampler()
    \param pvalue value
   */
  inline
  set_state_type<additional_texture>
  set_additional_sampler(int S, const additional_texture &pvalue)
  {
    WRATHassert(S>=0 and S<number_additional_textures_supported);
    return set_state_type<additional_texture>(pvalue, stream_id_additional_texture(S));
  }
  
  /*!\fn push_state_type<additional_texture> push_additional_sampler(int, const additional_texture&)
    "Manipulator" to push an additional sampler value
    \param S which additional sampler (must be 0<=S<number_additional_textures_supported).
             S corresponts to the sampler ID of \ref WRATHFontShaderSpecifier::add_sampler()
             and \ref WRATHFontShaderSpecifier::remove_sampler()
    \param pvalue value
   */
  inline
  push_state_type<additional_texture>
  push_additional_sampler(int S, const additional_texture &pvalue)
  {
    WRATHassert(S>=0 and S<number_additional_textures_supported);
    return push_state_type<additional_texture>(pvalue, stream_id_additional_texture(S));
  }

  /*!\fn pop_state_type<additional_texture> pop_additional_sampler(int)
    "Manipulator" to push an additional sampler value
    \param S which additional sampler (must be 0<=S<number_additional_textures_supported).
             S corresponts to the sampler ID of \ref WRATHFontShaderSpecifier::add_sampler()
             and \ref WRATHFontShaderSpecifier::remove_sampler()
   */
  inline
  pop_state_type<additional_texture>
  pop_additional_sampler(int S)
  {
    WRATHassert(S>=0 and S<number_additional_textures_supported);
    return pop_state_type<additional_texture>(stream_id_additional_texture(S));
  }

  /*!\fn get_state_type<additional_texture> get_additional_sampler(int, additional_texture&)
    "Manipulator" to push an additional sampler value
    \param S which additional sampler (must be 0<=S<number_additional_textures_supported).
             S corresponts to the sampler ID of \ref WRATHFontShaderSpecifier::add_sampler()
             and \ref WRATHFontShaderSpecifier::remove_sampler()
    \param ptarget location to place value
   */
  inline
  get_state_type<additional_texture>
  get_additional_sampler(int S, additional_texture &ptarget)
  {
    WRATHassert(S>=0 and S<number_additional_textures_supported);
    return get_state_type<additional_texture>(ptarget, stream_id_additional_texture(S));
  }

  /*!\class set_font_brush_implement
    Conveniance implementation class with operator<< overload that uses
    set_additional_sampler() and set_font_shader() using the values 
    within a \ref WRATHBrush. The brush will occupy additional
    sampler 0 and 1 if it has both an image and gradient,
    sampler 0 only if it has only one of them and
    no additional sampler if it has neither.
    \tparam F pointer to function that takes as argument a const reference
              to a \ref WRATHShaderBrush and returns a const reference to 
              a WRATHFontShaderSpecifier.
   */
  template<const WRATHFontShaderSpecifier& (*F)(const WRATHShaderBrush&)>
  class set_font_brush_implement
  {
  public:
    /*!\fn set_font_brush_implement
      Ctor 
      \param brush value to which to initialize \ref m_brush
     */
    explicit
    set_font_brush_implement(const WRATHBrush &brush):
      m_brush(brush)
    {
      m_brush.make_consistent();      
    }

    /*!\var m_brush
      WRATHBrush to apply to text
     */
    WRATHBrush m_brush;
  };

  /*!\typedef set_font_brush_item_aa
    Conveniance typedef using \ref set_font_brush_implement
    to draw text with anti-aliasing with item coordinates
    fed to the brush.
   */
  typedef set_font_brush_implement<&WRATHFontShaderSpecifier::default_brush_item_aa> set_font_brush_item_aa;
  
  /*!\typedef set_font_brush_letter_aa
    Conveniance typedef using \ref set_font_brush_implement
    to draw text with anti-aliasing with glyph coordinates
    fed to the brush.
   */
  typedef set_font_brush_implement<&WRATHFontShaderSpecifier::default_brush_letter_aa> set_font_brush_letter_aa;
  
  /*!\typedef set_font_brush_item_non_aa
    Conveniance typedef using \ref set_font_brush_implement
    to draw text without anti-aliasing with item coordinates
    fed to the brush.
   */
  typedef set_font_brush_implement<&WRATHFontShaderSpecifier::default_brush_item_non_aa> set_font_brush_item_non_aa;
  
  /*!\typedef set_font_brush_letter_non_aa
    Conveniance typedef using \ref set_font_brush_implement
    to draw text without anti-aliasing with glyph coordinates
    fed to the brush.
   */
  typedef set_font_brush_implement<&WRATHFontShaderSpecifier::default_brush_letter_non_aa> set_font_brush_letter_non_aa; 
}


/*!\fn WRATHStateStream& operator<<(WRATHStateStream &, 
                                    const WRATHText::set_font_brush_implement<F>&)
  Operator overload to set font shader and samplers for brush shading. 
  The brush will occupy additional sampler 0 and 1 if it has both 
  an image and gradient, sampler 0 only if it has only one of them 
  and no additional sampler if it has neither.\
  \param target WRATHStateStream to affect
  \param obj specifies shader and brush
 */ 
template<const WRATHFontShaderSpecifier& (*F)(const WRATHShaderBrush&)>
WRATHStateStream&                                                     
operator<<(WRATHStateStream &target, const WRATHText::set_font_brush_implement<F> &obj) 
{                                                                     
   int grad_unit(0);    
   target << WRATHText::set_font_shader(&F(obj.m_brush));
   if(obj.m_brush.m_image!=NULL)                                       
     {                       
       WRATHText::additional_texture v(obj.m_brush.m_image->texture_binder(0), true);
       target << WRATHText::set_additional_sampler(0, v); 
       ++grad_unit;                                                
     }                                                             
   if(obj.m_brush.m_gradient!=NULL)                                    
     {                                                             
       target << WRATHText::set_additional_sampler(grad_unit, obj.m_brush.m_gradient->texture_binder()); 
     }                                                  
   return target;                                                      
} 

/*!\fn WRATHTextDataStream::stream_type<T>& operator<<(WRATHTextDataStream::stream_type<T>, 
                                                       const WRATHText::set_font_brush_implement<F>&)
  Operator overload to set font shader and samplers for brush shading. 
  The brush will occupy additional sampler 0 and 1 if it has both 
  an image and gradient, sampler 0 only if it has only one of them 
  and no additional sampler if it has neither.\
  \param target stream to affect
  \param obj specifies shader and brush
 */
template<typename T, const WRATHFontShaderSpecifier& (*F)(const WRATHShaderBrush&)>
WRATHTextDataStream::stream_type<T> 
operator<<(WRATHTextDataStream::stream_type<T> target, 
           const WRATHText::set_font_brush_implement<F> &obj) 
{ 
  int grad_unit(0);    
  target << WRATHText::set_font_shader(&F(obj.m_brush));
  if(obj.m_brush.m_image!=NULL)                                       
    {                                                             
      WRATHText::additional_texture v(obj.m_brush.m_image->texture_binder(0));
      target << WRATHText::set_additional_sampler(0, v); 
      ++grad_unit;                                                
    }                                                             
  if(obj.m_brush.m_gradient!=NULL)                                    
    {                                                             
      target << WRATHText::set_additional_sampler(grad_unit, obj.m_brush.m_gradient->texture_binder()); 
    }                                                  
  return target;     
}  





/*! @} */





#endif
