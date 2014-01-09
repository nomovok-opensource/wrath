/*! 
 * \file WRATHShaderBrushSourceHoard.hpp
 * \brief file WRATHShaderBrushSourceHoard.hpp
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



/*! \addtogroup Imaging
 * @{
 */

#ifndef __WRATH_SHADER_HOARD_HPP__
#define __WRATH_SHADER_HOARD_HPP__

#include "WRATHConfig.hpp"
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include "WRATHBrush.hpp"
#include "WRATHShaderSpecifier.hpp"
#include "WRATHFontShaderSpecifier.hpp"
#include "WRATHMutex.hpp"
#include "WRATHItemDrawState.hpp"

/*!\class WRATHShaderBrushSourceHoard
  Conveniance class to add macros and source
  code to a set of shader sources based off
  of the values in a \ref WRATHShaderBrush.
 */
class WRATHShaderBrushSourceHoard:boost::noncopyable
{
public:

  /*!\enum brush_mapping_t
    Enumeration dictating how fetch()
    and fetch_font_shader() construct
    the shaders
   */
  enum brush_mapping_t
    {
      /*!
        Indicating to use linear brush mapping computed
        from the vertex shader
        - <B>void wrath_shader_brush_prepare(in vec2 p)</B> 
          in the vertex shader taking as input the brush 
          coordinate to feed to image and gradient
        - <B>vec4 wrath_shader_brush_color()</B> in the fragment
          shader to compute the color of the brush at the fragment
          position. If the brush dictates that the fragment should
          not be drawn, issues a discard. Also defines 
          <B>vec4 wrath_shader_brush_color(out float)</B> which 
          returns the color of the brush and the out argument is 
          0.0 if the brush dictates that the fragment should
          not be drawn and 1.0 if the brush dictates that the
          fragment should be drawn.
       */
      linear_brush_mapping,

      /*!
        Indicates to use non-lienar brush mapping computed
        from the fragment shader
        - <B>void wrath_shader_brush_prepare()</B> in the
          vertex shader to perform any pre-compute stages of 
          the gradient and image 
        - <B>vec4 wrath_shader_brush_color(in vec2 p)</B> in the fragment
          shader taking as input the brush coordinate to feed 
          to image and gradient. If the brush dictates that the 
          fragment should not be drawn, issues a discard. Also defines 
          <B>vec4 wrath_shader_brush_color(in vec2 p, out float)</B> which 
          returns the color of the brush and the out argument is 
          0.0 if the brush dictates that the fragment should
          not be drawn and 1.0 if the brush dictates that the
          fragment should be drawn.
       */
      nonlinear_brush_mapping,

      /*!
        Indicates to not provide the brush pre-computation
        and computation functions
       */
      no_brush_function
      
    };

  /*!\class ModifyShaderSpecifierBase
    Class that specifies if and how to modify
    a shader after it is assembled.
   */
  class ModifyShaderSpecifierBase:
    public WRATHReferenceCountedObjectT<ModifyShaderSpecifierBase>
  {
  public:
    virtual
    ~ModifyShaderSpecifierBase(void)
    {}

    /*!\fn void modify_shader(WRATHShaderSpecifier&,
                              const WRATHShaderBrush&, 
                              enum WRATHBaseSource::precision_t,
                              enum brush_mapping_t) const
      To be implemented by a derived class to
      modify a shader before being returned
      by \ref WRATHShaderBrushSourceHoard::fetch().
      \param shader WRATHShaderSpecifier to be returned by fetch
      \param brush WRATHShaderBrush fed to WRATHShaderBrushSourceHoard::fetch()
                   that generated shader
      \param prec precision qualifier fed to WRATHShaderBrushSourceHoard::fetch()
                  that generated shader
      \param brush_mapping enumeration fed to WRATHShaderBrushSourceHoard::fetch()
                           that generated shader
    */  
    virtual
    void
    modify_shader(WRATHShaderSpecifier &shader,
                  const WRATHShaderBrush &brush, 
                  enum WRATHBaseSource::precision_t prec,
                  enum brush_mapping_t brush_mapping) const=0;

    /*!\fn void modify_shader(WRATHFontShaderSpecifier&,
                              const WRATHShaderBrush&, 
                              enum WRATHBaseSource::precision_t,
                              enum brush_mapping_t) const
      To be implemented by a derived class to
      modify a shader before being returned
      by \ref WRATHShaderBrushSourceHoard::fetch_font_shader().
      \param shader WRATHShaderSpecifier to be returned by 
                    WRATHShaderBrushSourceHoard::fetch_font_shader()
      \param brush WRATHShaderBrush fed to WRATHShaderBrushSourceHoard::fetch_font_shader()
                   that generated shader
      \param prec precision qualifier fed to WRATHShaderBrushSourceHoard::fetch()
                  that generated shader
      \param brush_mapping enumeration fed to WRATHShaderBrushSourceHoard::fetch()
                           that generated shader
    */  
    virtual
    void
    modify_shader(WRATHFontShaderSpecifier &shader,
                  const WRATHShaderBrush &brush, 
                  enum WRATHBaseSource::precision_t prec,
                  enum brush_mapping_t brush_mapping) const=0;
  };

  /*!\fn WRATHShaderBrushSourceHoard(const std::map<GLenum, WRATHGLShader::shader_source>&,
                                     uint32_t, uint32_t, 
                                     const ModifyShaderSpecifierBase::const_handle&);
    \param src actual shader source code
    \param custom_bit_mask bit mask to filter \ref WRATHShaderBrush::m_custom_bits
    \param bit_mask bit mask to filter \ref WRATHShaderBrush::m_bits
    \param modifier handle to ModifyShaderSpecifierBase to perform additional
                    modification to shaders returned by fetch() and fetch_font_shader().
   */
  WRATHShaderBrushSourceHoard(const std::map<GLenum, WRATHGLShader::shader_source> &src,
                              uint32_t custom_bit_mask=0,
                              uint32_t bit_mask=~0,
                              const ModifyShaderSpecifierBase::const_handle &modifier
                              =ModifyShaderSpecifierBase::const_handle());

  /*!\fn WRATHShaderBrushSourceHoard(const WRATHGLShader::shader_source&,
                                     const WRATHGLShader::shader_source&,
                                     uint32_t, uint32_t, 
                                     const ModifyShaderSpecifierBase::const_handle&);
    \param vertex_shader shader source code for vertex shader
    \param fragment_shader shader source code for fragment shader
    \param custom_bit_mask bit mask to filter \ref WRATHShaderBrush::m_custom_bits
    \param bit_mask bit mask to filter \ref WRATHShaderBrush::m_bits
    \param modifier handle to ModifyShaderSpecifierBase to perform additional
                    modification to shaders returned by fetch() and fetch_font_shader().
   */
  WRATHShaderBrushSourceHoard(const WRATHGLShader::shader_source &vertex_shader,
                              const WRATHGLShader::shader_source &fragment_shader,
                              uint32_t custom_bit_mask=0,
                              uint32_t bit_mask=~0,
                              const ModifyShaderSpecifierBase::const_handle &modifier
                              =ModifyShaderSpecifierBase::const_handle());

  ~WRATHShaderBrushSourceHoard(void);
    
  /*!\fn void add_state
    Add the necessary state of this
    WRATHBrush to a WRATHSubItemDrawState:
    - bound textures for image and gradient
    - uniforms for texture sizes.
    This should NOT be used for text drawing,
    to augment text drawing state with the
    state of a brush, one needs to place
    the brush within the WRATHTextDataStream,
    see \ref WRATHText::set_font_brush_implement,
    \ref WRATHText::set_font_brush_item_aa,
    \ref WRATHText::set_font_brush_letter_aa,
    \ref WRATHText::set_font_brush_item_non_aa,
    and \ref WRATHText::set_font_brush_letter_non_aa
   */
  void
  add_state(const WRATHBrush &brush, WRATHSubItemDrawState &subkey) const;

  /*!\fn const WRATHShaderSpecifier& fetch
    Fetches the WRATHShaderSpecifier with the functions provided
    by a WRATHShaderBrush added. Additionally, can provide functions
    necessary to compute the brush color, see the enumeration
    \ref brush_mapping_t for those functions.
   
    The bits that are up in the WRATHShaderBrush add macros
    - AA_HINT is added if WRATHShaderBrush::anti_alias() is true
    - IMAGE_ALPHA_TEST is added if WRATHShaderBrush::image_alpha_test() is true
    - GRADIENT_ALPHA_TEST is added if WRATHShaderBrush::gradient_alpha_test() is true
    - CONST_COLOR_ALPHA_TEST is added if WRATHShaderBrush::color_alpha_test() is true
    - FINAL_ALPHA_TEST is added if WRATHShaderBrush::final_color_alpha_test() is true
    - GRADIENT_INTERPOLATE_RANGE_ENFORCE_POSITIVE is added if WRATHShaderBrush::gradient_interpolate_enforce_positive() is true
    - GRADIENT_INTERPOLATE_RANGE_ENFORCE_LESS_THAN_ONE is added if WRATHShaderBrush::gradient_interpolate_enforce_greater_than_one() is true
    - GRADIENT_INTERPOLATE_ENFORCE_BLEND is added if WRATHShaderBrush::gradient_interpolate_enforce_by_blend() is true
    - FLIP_IMAGE_Y is added if WRATHShaderBrush::flip_image_y() is true
    - PREMULTIPLY_ALPHA is added if WRATHShaderBrush::premultiply_alpha() is true

    The shader brush color computation is affected as follows:
    - if GRADIENT_INTERPOLATE_RANGE_ENFORCE_POSITIVE, then the gradient interpolate
      is not in range whenever it is negative
    - if GRADIENT_INTERPOLATE_RANGE_ENFORCE_LESS_THAN_ONE, then the gradient interpolate
      is not in range whenever it is greater than one
    - if GRADIENT_INTERPOLATE_ENFORCE_BLEND, then the alpha value is 0 if the gradient
      interpolate is not in range
    - if GRADIENT_INTERPOLATE_ENFORCE_BLEND is false and if the gradient interpolate
      is not within range, then the brush would discard is issued
    - if IMAGE_ALPHA_TEST, then the brush would discard is issued for when image alpha is less than 0.5
    - if GRADIENT_ALPHA_TEST, then the brush would discard is issued for when gradient alpha is less than 0.5
    - if CONST_COLOR_ALPHA_TEST, then the brush would discard is issued if the const color alpha is less than 0.5
    - if FINAL_ALPHA_TEST, then the brush would discrad is issued if the final brush color is less than 0.5
    - if the brush may issue a discard then WRATH_BRUSH_ISSUES_DISCARD is defined

    The color computation function <B>vec4 wrath_shader_brush_color(out float)</B>
    (for linear) and <B>vec4 wrath_shader_brush_color(in vec2 p, out float)</B>
    (for non-linear) do NOT issue a discard -ever- and return through the out
    if the brush would issue a discard, where as <B>vec4 wrath_shader_brush_color()</B>
    (for linear) and <B>vec4 wrath_shader_brush_color(in vec2 p)</B>
    (for non-linear) do issue a discard.

    The brush color function NEVER pre-multiplies the alpha color, even when
    PREMULTIPLY_ALPHA is true. The purpose of PREMULTIPLY_ALPHA is for a shader
    that uses the brush to pre-multiply the color itself.

    If an image is present the macro WRATH_BRUSH_IMAGE_PRESENT is added.
    If a gradient is present the macro WRATH_BRUSH_GRADIENT_PRESENT is added.
    Adds the macro WRATH_LINEAR_BRUSH_PRESENT if brush_mapping is linear_brush_mapping,
    adds the macro NONWRATH_LINEAR_BRUSH_PRESENT if brush_mapping is nonlinear_brush_mapping

    Adds the macro WRATH_BRUSH_IMAGE_PRESENT if an image is in the brush 
    (see \ref WRATHShaderBrush::m_texture_coordinate_source).

    Adds the macro WRATH_BRUSH_GRADIENT_PRESENT if a gradient is in the brush
    (see \ref WRATHShaderBrush::m_gradient_source).

    Adds the macro WRATH_BRUSH_COLOR_PRESENT if a color value is in the brush
    (see \ref WRATHShaderBrush::m_color_value_source)

    Notes: if the brush has both an image and gradient
    then the image is placed on texture unit 0 and
    the gradient on unit 1; if only one is non-NULL,
    the it uses texture unit 0. The sampler for the texture
    has the name <B>wrath_brush_imageTexture</B> and the
    sampler for the gradient has the name
    <B>wrath_brush_gradientTexture</B>. If an image
    is used, the size of the image texture is
    stored in the uniform <B>wrath_brush_imageTextureSize</B>

    \param brush WRATHShaderBrush specifying the brush: shaders, image and gradient values
    \param prec precision in which to apply the brush
    \param brush_mapping enumeration indicating if and how the brush 
                         compute and pre-compute functions appear in the
                         vertex and fragment shaders
   */
  const WRATHShaderSpecifier&
  fetch(const WRATHShaderBrush &brush, enum WRATHBaseSource::precision_t prec,
        enum brush_mapping_t brush_mapping=linear_brush_mapping) const;  

  /*!\fn const WRATHFontShaderSpecifier& fetch_font_shader
    Analgoue of fetch() for font shading returning a WRATHFontShaderSpecifier.
    However, image and gradient textures are added first image then gradient
    via WRATHFontShaderSpecifier::add_sampler().

    \param brush WRATHShaderBrush specifying the brush: shaders, image and gradient values
    \param prec precision in which to apply the brush
    \param brush_mapping enumeration indicating if and how the brush 
                         compute and pre-compute functions appear in the
                         vertex and fragment shaders
   */
  const WRATHFontShaderSpecifier&
  fetch_font_shader(const WRATHShaderBrush &brush, enum WRATHBaseSource::precision_t prec,
                    enum brush_mapping_t brush_mapping=linear_brush_mapping) const;

protected:
  /*!\fn void add_custom_macros
    Add macros to the shader source based off
    the value of \ref WRATHShaderBrush::m_custom_bits.
    \param dest shader source to which to add macros
    \param custom_bits value of WRATHShaderBrush::m_custom_bits
   */
  virtual
  void
  add_custom_macros(WRATHGLShader::shader_source &dest, uint32_t custom_bits) const;

private:
  typedef boost::tuple<WRATHShaderBrush, 
                       enum WRATHBaseSource::precision_t, 
                       enum brush_mapping_t> key_type;
  typedef std::map<key_type, WRATHShaderSpecifier*> map_type;
  typedef std::map<key_type, WRATHFontShaderSpecifier*> font_map_type;
  
  std::map<GLenum, WRATHGLShader::shader_source> m_src;

  uint32_t m_custom_bit_mask, m_bit_mask;
  ModifyShaderSpecifierBase::const_handle m_modifier;

  mutable WRATHMutex m_mutex;
  mutable map_type m_shaders;
  mutable WRATHMutex m_font_mutex;
  mutable font_map_type m_font_shaders;
  
};


/*! @} */

#endif
