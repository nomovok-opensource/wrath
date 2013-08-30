/*! 
 * \file WRATHItemDrawerFactory.hpp
 * \brief file WRATHItemDrawerFactory.hpp
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




#ifndef __WRATH_DRAW_GROUP_DRAWER_FACTORY_HPP__
#define __WRATH_DRAW_GROUP_DRAWER_FACTORY_HPP__

#include "WRATHConfig.hpp"

class WRATHItemDrawer;
class WRATHShaderSpecifier;
class WRATHAttributePacker;

/*! \addtogroup Group
 * @{
 */

/*!\class WRATHItemDrawerFactory
  WRATHItemDrawerFactory provides an interface 
  to generate WRATHItemDrawer objects.
  Derived class objects should be stateless,
  light weight copyable objects. That a 
  WRATHItemDrawerFactory object is stateless
  is _critical_ for correct use. If two 
  WRATHItemDrawerFactory derived objects
  have identical type information (as returned
  by C++'s typeid operator), then it is undefined
  behavior which of those two are used to
  generate a WRATHItemDrawer object in
  \ref WRATHShaderSpecifier::fetch_drawer().
 */
class WRATHItemDrawerFactory
{
public:
  virtual
  ~WRATHItemDrawerFactory()
  {}

  /*!\fn WRATHItemDrawer* generate_drawer
    To be implemented by a derived class to create a 
    WRATHItemDrawer using the user provided shader 
    source code within a WRATHShaderSpecifier object and 
    a WRATHAttributePacker object to determine the attribute 
    bindings. 
   
    For generating the GLSL code, the factory needs to produce
    a main() routine which does as follows:
    - first call any initialization code (for example for node fetching)
    - then call the user provided shader code function, shader_main().
    
    In addition the factory must add macros to <B>each</B> shader stage 
    so that every shader stage "knows" what shader stages support
    node value fetcthing. For each shader stage, S (for example
    <B>GL_VERTEX_SHADER</B>), the macro
    \code
    WRATH_XXX_ITEM_VALUE_FETCH_OK
    \endcode
    where XXX=\ref WRATHGLShader::gl_shader_type_label() passed
    the GL enumeration value S.
    The macro set is added to EVERY shader stage of the \ref
    WRATHMultiGLProgram of the \ref WRATHItemDrawer.
    For example if fetching per-node values is possible
    in the fragment shader, then all shader stages
    will have the macro <B>WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK</B>
    defined in them.

    For those shader stages that support node value fetching,
    the addded shader source code must provide 
    - a macro fetch_node_value(X) which "returns" the node value named X
    
    
    The added shader source code must provide in the vertex shader:
    - vec4 compute_gl_position(in vec3) returns the value to use for gl_Position, 
                                        passing as input an (x,y,z) coordinate where
                                        (x,y) is a coordinate before the transformation
                                        of the node (i.e. item local coordinates) and
                                        z is z-coordinate for perspective transformations.
    - vec2 compute_clipped_normalized_coordinate(vec2 in_normalized,
                                                 vec2 in_quad_top_left,
                                                 vec2 in_quad_size)
      computes the normalized coordinate to use to accomplish clipping for
      a quad that is parallel to the item's local coordinate system.
      The value in_normalized has each coordinate as 0 or 1, with (0,0)
      representing the bottom left and (1,1) representing the top right.
   
    - vec4 compute_gl_position_and_apply_clipping(in vec3) returns the same value as
      compute_gl_position(vec3), but also sets values to perform _per_ item clipping,
      such clipping is dertermined by the Node of an item. A WRATHCanvas
      implementation may also perform additional clipping that is per WRATHCanvas
      as well.
    
   The added shader source code must provide in the fragment shader:
    - void discard_if_clipped(void) is to perform discard if the fragment
      is clipped. The function may be empty (and thus not actually perform
      any discarding if for example clipping is accomplished by hardware 
      clipping planes and/or if the Node type does not support clipping).
    - float discard_via_alpha(void) is to return 0.0 if the fragment is
      clipped and 1.0 if the fragment is not. Use cases for using this
      function is for blended fragments to avoid discard. 

    Note that for those transformation systems that do not clip on a per item
    basis or use hardware clipping planes, then the function discard_if_clipped()
    is empty and the function discard_via_alpha() always returns 1.0. For the case
    where the functins are not this (i.e. clipping is done in the fragment shader),
    the symbol CLIPPING_USES_DISCARD must be defined as well. In addition,
    the function discard_if_clipped() should be a no-op and CLIPPING_USES_DISCARD
    not defined when the macro WRATH_COVER_DRAW is defined, see also \ref 
    WRATHBaseItem::selector_color_draw_cover() and \ref 
    WRATHBaseItem::selector_non_color_draw_cover()

    \param shader_specifier WRATHShaderSpecifier specifying user provided share source code
    \param attribute_packer WRATHAttributePacker specifying explicit attribute data
    \param sub_drawer_id a value to allow for a derived class to generate different
                         WRATHItemDrawer objects based upon additional criteria
                         specified by an integer
   */
  virtual
  WRATHItemDrawer*
  generate_drawer(const WRATHShaderSpecifier *shader_specifier,
                  const WRATHAttributePacker *attribute_packer,
                  int sub_drawer_id) const=0;

  /*!\fn WRATHItemDrawerFactory* copy
    To be implemented by a derived class to create
    a copy of the WRATHItemDrawerFactory.    
   */
  virtual
  WRATHItemDrawerFactory*
  copy(void) const=0;

};

/*! @} */

#endif
