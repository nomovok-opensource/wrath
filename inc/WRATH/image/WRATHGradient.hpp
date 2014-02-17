/*! 
 * \file WRATHGradient.hpp
 * \brief file WRATHGradient.hpp
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




#ifndef WRATH_HEADER_GRADIENT_HPP_
#define WRATH_HEADER_GRADIENT_HPP_


#include "WRATHConfig.hpp"
#include "WRATHImage.hpp"
#include "WRATHUniformData.hpp"
#include "WRATHAttributePacker.hpp"

/*! \addtogroup Imaging
 * @{
 */


/*!\class WRATHGradient
  A WRATHGradient represents a sequence a
  color stop values which are used to 
  compute interpolate color between
  those stops. A WRATHGradient is implemented
  internally as a portion of a GL texture.

  WRATHGradient is a resource managed object,
  i.e. that class has a WRATHResourceManager,
  see \ref WRATH_RESOURCE_MANAGER_DECLARE.
  A WRATHGradient can be modified and created 
  from threads outside of the GL context, 
  however, it must only be deleted from 
  within the GL context. In particular the 
  resource manager may only be cleared from 
  within the GL context.
 */
class WRATHGradient:boost::noncopyable
{
public:
  /// @cond
  WRATH_RESOURCE_MANAGER_DECLARE(WRATHGradient, std::string);
  /// @endcond

  /*!\enum repeat_type_t
    Enumeration type specifying how a WRATHGradient
    repeats
   */ 
  enum repeat_type_t
    {
      /*!
        Gradient saturates, i.e. outside of
        range [0.0, 1.0] take the closest value.
       */
      Clamp=0,

      /*!
        Gradient repeats
       */
      Repeat=1,

      /*!
        Gradient mirror repeats
       */
      MirrorRepeat=2
    };

  /*!\class parameters
    Class to specify the parameters of a 
    \ref WRATHGradient object.
   */
  class parameters
  {
  public:
    /*!\var m_repeat_type
      Specifies the repeat type of a WRATHGradient
     */
    enum repeat_type_t m_repeat_type;
    
    /*!\var m_log2_resolution
      Specifies the _log2_ of the resolution
      of a WRATHGradient
     */
    int m_log2_resolution;

    /*!\fn parameters(enum repeat_type_t, float)
      Ctor.
      \param tp repeat type for WRATHGradient
      \param delta_t determines resolution of WRATHGraident
                     by specifying the delta in color stop
                     positions of the center of texels of
                     a WRATHGradient
     */
    parameters(enum repeat_type_t tp, float delta_t);

    /*!\fn parameters(enum repeat_type_t)
      Ctor.
      \param tp repeat type for WRATHGradient
     */
    parameters(enum repeat_type_t tp):
      m_repeat_type(tp),
      m_log2_resolution(5)
    {}
  };


  /*!\class color
    Conveniance class to specify a color in
    an easy overloadable fashion.
   */
  class color
  {
  public:
    /*!\var m_value
      Color value for each color channel,
      0.0=no color, 1.0=full, i.e.
      vec4(1,1,1,1)=white. Encoding
      is:
      - .x() --> red
      - .y() --> green
      - .z() --> blue
      - .w() --> alpha
     */
    vec4 m_value;

    /*!\fn color(const vec4 &)
      Ctor from a vec4 
      \param v value from which to set \ref m_value
     */
    color(const vec4 &v):
      m_value(v)
    {}

    /*!\fn color(const vec3&, float)
      Ctor from a vec3 and a float
      \param v value from which to set rgb values of \ref m_value
      \param alpha value from which to set alpha value of \ref m_value
     */
    color(const vec3 &v, float alpha=1.0f):
      m_value(v.x(), v.y(), v.z(), alpha)
    {}

    /*!\fn color(float, float, float, float)
      Ctor from a 4 floats
      \param r value from which to set red value of \ref m_value
      \param g value from which to set green value of \ref m_value
      \param b value from which to set blue value of \ref m_value
      \param a value from which to set alpha value of \ref m_value
     */
    color(float r, float g, float b, float a=1.0f):
      m_value(r,g,b,a)
    {}

    /*!\fn color(vecN<GLubyte,4>)
      Ctor from a vecN<GLubyte,4>. All bits up
      indicates 1.0, all bits down indicate 0.0, etc.
      \param v value from which to set \ref m_value
     */
    color(vecN<GLubyte,4> v):
      m_value( vec4(v)/255.0f)
    {}

    /*!\fn color(vecN<GLubyte,3>, GLubyte)
      Ctor from a vecN<GLubyte,3> and GLubyte. All bits up
      indicates 1.0, all bits down indicate 0.0, etc.
      \param v value from which to set rgb values of \ref m_value
      \param a value from which to set alpha value of \ref m_value
     */
    color(vecN<GLubyte,3> v, GLubyte a=255):
      m_value(vec4(vecN<GLubyte,4>(v.x(), v.y(), v.z(), a))/255.0f)
    {}
  };


  /*!\class GradientYCoordinate
    Derives from WRATHStateBasedPackingData, to
    hold the y-texture coordinate of a WRATHGradient
    object.
   */
  class GradientYCoordinate:public WRATHStateBasedPackingData
  {
  public:
    /*!\typedef handle
      Conveniant local typedef
     */
    typedef handle_t<GradientYCoordinate> handle;

    /*!\fn GradientYCoordinate
      Ctor
      \param v value holding the y-texture 
               coordinate of a \ref WRATHGradient.
     */
    explicit
    GradientYCoordinate(float v):
      m_texture_coordinate_y(v)
    {}

    /*!\fn float texture_coordinate_y
      Returns the y-coordinate of a \ref WRATHGradient.
     */
    float
    texture_coordinate_y(void) const
    {
      return m_texture_coordinate_y;
    }

  private:
    float m_texture_coordinate_y;
  };
  
  /*!\fn WRATHGradient(const std::string&, const parameters&)
    Ctor. Register the WRATHGradient to the WRATHGradient
    resource manager.
    \param presource_name resource name of the WRATHGradient
    \param pp specifies repeat type and resolution of WRATHGradient
   */
  explicit
  WRATHGradient(const std::string &presource_name, 
                const parameters &pp=Repeat);

  /*!\fn WRATHGradient(const parameters &pp)
    Ctor. Does NOT register the WRATHGradient 
    to the WRATHGradient resource manager.
    \param pp specifies repeat type and resolution of WRATHGradient
   */
  explicit
  WRATHGradient(const parameters &pp=Repeat);

  virtual
  ~WRATHGradient();

  /*!\fn boost::signals2::connection connect_dtor
    The dtor of a WRATHImage emit's a signal, use this function
    to connect to that signal. The signal is emiited just before
    the WRATHImage is removed from the resource manager which
    in turn is before the underlying GL resources are marked
    as free.
   */
  boost::signals2::connection 
  connect_dtor(const boost::signals2::signal<void () >::slot_type &slot)
  {
    return m_dtor_signal.connect(slot);
  }

  /*!\fn const std::string& resource_name
    Returns the resource name of the WRATHGradient.
    If the WRATHGradient was not registered to the
    resource manager at construction, returns an
    empty string.
   */
  const std::string&
  resource_name(void) const
  {
    return m_resource_name;
  }

  /*!\fn const WRATHTextureChoice::texture_base::handle& texture_binder
    Returns the texture binder of the WRATHGradient.
    The texture of a WRATHGradient may hold multiple
    gradients. The texture value to use in GLSL 
    of the WRATHGradient is given by:
    \code
    texture2D(GradientSampler, vec2(t, y))
    \endcode
    where t is the interpolate along the gradient
    and y is given by \ref texture_coordinate_y().
   */
  const WRATHTextureChoice::texture_base::handle&
  texture_binder(void) const
  {
    return m_binder;
  }

  /*!\fn float texture_coordinate_y
    Returns the y-texture coordinate for use
    with this WRATHGradient in GLSL
   */
  float
  texture_coordinate_y(void) const;

  /*!\fn WRATHUniformData::uniform_by_name_base::handle texture_coordinate_y_uniform
    Returns a uniform suitable for different GLSL programs
    (since it is a \ref WRATHUniformData::uniform_by_name_base)
    which is a float whose value is \ref texture_coordinate_y().
    \param uniform_name name of the uniform
   */
  WRATHUniformData::uniform_by_name_base::handle
  texture_coordinate_y_uniform(const std::string &uniform_name="gradient_y_coordinate") const;

  /*!\fn const WRATHStateBasedPackingData::handle& texture_coordinate_y_state_based_packing_data
    Returns a handle whose underlying object is 
    a GradientYCoordinate whose GradientYCoordinate::texture_coordinate_y()
    method returns the same value as this objects
    texture_coordinate_y() method does.
   */
  const WRATHStateBasedPackingData::handle&
  texture_coordinate_y_state_based_packing_data(void) const;

  /*!\fn enum repeat_type_t repeat_mode
    Returns the repeat mode of the WRATHGradient.
   */
  enum repeat_type_t
  repeat_mode(void) const;

  /*!\fn int texel
    A WRATHGradient is implemented as a texture.
    If two color values land in the same texel
    of the texture, then the last one set
    is what is used. This function returns
    the texel coordinate for a given interpolate.
    In particular, if texel(t0) and texel(t1)
    are the same then the affect and return value of
    set_color(t0, c) and set_color(t1, c) are
    identical.

    For \ref Clamp and \ref MirrorRepeat modes, 
    positions 0.0 and 1.0 are distinct.
    For \ref Clamp mode for positions at t<0.0, 
    are the same as position at t=0.0 and
    positions for t>1.0 are the same as 
    position at t=1.0.
    For \ref MirrorRepeat modes, the position
    t is the same as the position 1-abs(1-2*fract(t/2))

    For \ref Repeat mode, position 0.0 
    and 1.0 are _the_ same, i.e. they are 
    mapped to the same texel. Additionally
    for any position t, t is the same as
    fract(t).

    \param t interpolate of which to fetch the texel 
   */
  int
  texel(float t) const;

  /*!\fn bool same_spot
    Provided as a conveniance, equivalent to
    \code   
    texel(t0)==texel(t1);
    \endcode
    \param t0 interpolate to which to compare t1 against
    \param t1 interpolate to which to compare t0 against
   */
  bool
  same_spot(float t0, float t1) const
  {
    return texel(t0)==texel(t1);
  }

  /*!\fn int set_color
    Set the color value of the WRATHGradient
    at an interpolate. Note that the resolution
    of the WRATHGradient is limited. Returns
    the "texel" within the WRATHGradient that
    was affected in the call.

    For \ref Clamp and \ref MirrorRepeat modes, 
    positions 0.0 and 1.0 are distinct.
    For \ref Clamp mode for positions at t<0.0, 
    are the same as position at t=0.0 and
    positions for t>1.0 are the same as 
    position at t=1.0.
    For \ref MirrorRepeat modes, the position
    t is the same as the position 1-abs(1-2*fract(t/2))

    For \ref Repeat mode, position 0.0 
    and 1.0 are _the_ same, i.e. they are 
    mapped to the same texel. Additionally
    for any position t, t is the same as
    fract(t).

    \param t interpolate value to which to assign a new color
    \param pcolor color value
   */
  int
  set_color(float t, const color &pcolor);  

  /*!\fn void remove_color
    Remove a color value from the WRATHGradient.
    \param texel value as returned by \ref set_color()
   */
  void
  remove_color(int texel);

private:
  void
  construct(const parameters &pp);

  bool m_registered;
  std::string m_resource_name;
  WRATHReferenceCountedObject::handle m_data_handle;
  WRATHTextureChoice::texture_base::handle m_binder;

  mutable std::map<std::string, WRATHUniformData::uniform_by_name_base::handle> m_uniforms;
  boost::signals2::signal<void () > m_dtor_signal;
};

/*! @} */


#endif
