/*! 
 * \file WRATHItemTypes.hpp
 * \brief file WRATHItemTypes.hpp
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


/*! \addtogroup Items
 * @{
 */


#ifndef __WRATH_ITEM_TYPES_HPP__
#define __WRATH_ITEM_TYPES_HPP__

#include "WRATHConfig.hpp"
#include "WRATHBaseItem.hpp"
#include "WRATHCanvas.hpp"
#include "WRATHShaderSpecifier.hpp"

/*!\namespace WRATHItemTypes
  Namespace holding common base types
  to define how to draw various item
  types.
 */
namespace WRATHItemTypes
{
  /*!\class DrawerPass
    Defines a set of values dictating
    how to draw one pass of drawing 
    an item.
   */
  class DrawerPass
  {
  public:
    /*!\fn DrawerPass
      \param sh value to which to initialize \ref m_shader
      \param pdraw_type value to which to initialize \ref m_draw_type
     */ 
    DrawerPass(const WRATHShaderSpecifier *sh=NULL,
               WRATHDrawType pdraw_type=WRATHDrawType::opaque_pass()):
      m_shader(sh),
      m_draw_type(pdraw_type)
    {}

    /*!\fn void set_item_draw_state_value(WRATHItemDrawState&,
                                          const WRATHItemDrawerFactory&, int,
                                          GLenum , const WRATHAttributePacker*,
                                          GLenum) const
      Augments a WRATHItemDrawState to the values
      of this \ref DrawerPass, equivalent to
      \code
      draw_state
        .drawer(m_shader->fetch_drawer(factory, packer, subdrawer_id))
        .primitive_type(primitive_type)
        .absorb(m_draw_state)
        .force_draw_order(m_force_draw_order)
        .draw_type(m_draw_type)
        .buffer_object_hint(buffer_object_hint);
      \endcode

      \param draw_state WRATHItemDrawState to modify
      \param factory \ref WRATHItemDrawerFactory used to generate the 
                     \ref WRATHItemDrawer from \ref m_shader
      \param subdrawer_id SubDrawerID passed to \ref WRATHItemDrawerFactory
                          to generate the \ref WRATHItemDrawer from \ref m_shader
      \param packer \ref WRATHAttributePacker (which specifies the names of attributes
                    of the vertex shader) passed to \ref WRATHItemDrawerFactory
                    to generate the \ref WRATHItemDrawer from \ref m_shader
      \param primitive_type GL enumeration for the primitive type, for example
                            GL_TRIANGLES
      \param buffer_object_hint buffer object storage for the indices                             
     */
    void
    set_item_draw_state_value(WRATHItemDrawState &draw_state,
                              const WRATHItemDrawerFactory &factory, int subdrawer_id,
                              GLenum primitive_type, const WRATHAttributePacker *packer,
                              GLenum buffer_object_hint) const
    {
      draw_state
        .drawer(m_shader->fetch_drawer(factory, packer, subdrawer_id))
        .primitive_type(primitive_type)
        .absorb(m_draw_state)
        .force_draw_order(m_force_draw_order)
        .draw_type(m_draw_type)
        .buffer_object_hint(buffer_object_hint);
    }


    /*!\fn void set_item_draw_state_value(WRATHItemDrawState&,
                                          const WRATHItemDrawerFactory&, int,
                                          GLenum , const WRATHAttributePacker*) const
      Augments a WRATHItemDrawState to the values
      of this \ref DrawerPass, equivalent to
      \code
      draw_state
        .drawer(m_shader->fetch_drawer(factory, packer, subdrawer_id))
        .primitive_type(primitive_type)
        .absorb(m_draw_state)
        .force_draw_order(m_force_draw_order)
        .draw_type(m_draw_type)
        .buffer_object_hint(buffer_object_hint);
      \endcode

      \param draw_state WRATHItemDrawState to modify
      \param factory \ref WRATHItemDrawerFactory used to generate the 
                     \ref WRATHItemDrawer from \ref m_shader
      \param subdrawer_id SubDrawerID passed to \ref WRATHItemDrawerFactory
                          to generate the \ref WRATHItemDrawer from \ref m_shader
      \param packer \ref WRATHAttributePacker (which specifies the names of attributes
                    of the vertex shader) passed to \ref WRATHItemDrawerFactory
                    to generate the \ref WRATHItemDrawer from \ref m_shader
      \param primitive_type GL enumeration for the primitive type, for example
                            GL_TRIANGLES                         
     */
    void
    set_item_draw_state_value(WRATHItemDrawState &draw_state,
                              const WRATHItemDrawerFactory &factory, int subdrawer_id,
                              GLenum primitive_type, const WRATHAttributePacker *packer) const
    {
      draw_state
        .drawer(m_shader->fetch_drawer(factory, packer, subdrawer_id))
        .primitive_type(primitive_type)
        .absorb(m_draw_state)
        .force_draw_order(m_force_draw_order)
        .draw_type(m_draw_type);
    }


    /*!\var m_shader  
      Specifies how the the draw pass is drawn in GLSL.
     */
    const WRATHShaderSpecifier *m_shader;

    /*!\var m_draw_state  
      WRATHSubItemDrawState for the drawn item.
      Additional textures, uniforms, GL state can
      be added and specified with \ref m_draw_state.
     */
    WRATHSubItemDrawState m_draw_state; 
 
    /*!\var m_force_draw_order
      Static draw order, diffrent draw order
      values break batching as in 
      WRATHItemDrawState::m_force_draw_order.
      Default value is an invalid handle.
     */
    WRATHDrawOrder::handle m_force_draw_order;

    /*!\var m_draw_type
      Drawing type for the pass.
     */
    WRATHDrawType m_draw_type; 
  };

  /*!\class Drawer
    A Drawer represents how to draw an item
    in multiple passes.
    \tparam AttributePacker attribute packer type for the item
    \tparam Pass type derived from DrawerPass specifying how to draw a pass
   */
  template<typename AttributePacker, typename Pass=DrawerPass>
  class Drawer
  {
  public:
    /*!\fn Drawer(void)
      Ctor. Initialize \ref m_packer as NULL,
      \ref m_draw_passes as an array of length 0.
     */
    Drawer(void):
      m_packer(NULL),
      m_buffer_object_hint(GL_STATIC_DRAW)
    {}

    /*!\fn Drawer(const WRATHShaderSpecifier*,const AttributePacker*, WRATHDrawType)
      Initialize Drawer to have one pass drawn
      with the specified shader as an item
      of the specified \ref WRATHDrawType
      \param sh value to which to set \ref m_draw_passes[0].m_shader
      \param pk attribute packer to generate attribute data
      \param ppass WRATHDrawType specifying at which pass to draw
     */
    Drawer(const WRATHShaderSpecifier *sh,
           const AttributePacker *pk,
           WRATHDrawType ppass=WRATHDrawType::opaque_pass()):
      m_packer(pk),
      m_draw_passes(1, Pass(sh,ppass) ),
      m_buffer_object_hint(GL_STATIC_DRAW)
    {}

    /*!\fn void set_item_draw_state_value(WRATHItemDrawState&, int, 
                                          const WRATHItemDrawerFactory &, int,
                                          GLenum) const
      Sets a \ref WRATHItemDrawState from the values of an 
      element in \ref m_draw_passes. Equivalent to
      \code
      m_draw_passes[pass].set_item_draw_state_value(draw_state, factory, subdrawer_id,
                                                    primitive_type, m_packer, m_buffer_object_hint);
      \endcode
      See also \ref DrawerPass::set_item_draw_state_value().
      \param draw_state WRATHItemDrawState to modify
      \param pass index into \ref m_draw_passes from which to set the values
      \param factory \ref WRATHItemDrawerFactory used to generate the 
                     \ref WRATHItemDrawer from the shaders of each pass
      \param subdrawer_id SubDrawerID passed to \ref WRATHItemDrawerFactory
                          to generate the \ref WRATHItemDrawer from the shaders of each pass
      \param primitive_type GL enumeration for the primitive type, for example
                            GL_TRIANGLES
       
     */
    void
    set_item_draw_state_value(WRATHItemDrawState &draw_state,
                              int pass,
                              const WRATHItemDrawerFactory &factory, int subdrawer_id,
                              GLenum primitive_type) const
    {
      m_draw_passes[pass].set_item_draw_state_value(draw_state, 
                                                    factory, subdrawer_id,
                                                    primitive_type, m_packer, 
                                                    m_buffer_object_hint);
    }

    /*!\fn void set_item_draw_state_value(std::set<WRATHItemDrawState>&, 
                                          const WRATHItemDrawerFactory &, int,
                                          GLenum) const
      Sets a \ref WRATHItemDrawState from the values of an 
      element in \ref m_draw_passes. Equivalent to
      \code
      for(unsigned int i=0, endi=m_draw_passes.size(); i<endi; ++i)
        {
          WRATHItemDrawState draw_state;
          set_item_draw_state_value(draw_state, i, factory, subdrawer_id, primitive_type);
          multi_pass_draw_state.insert(draw_state);
        }
      \endcode
      See also \ref DrawerPass::set_item_draw_state_value().
      \param multi_pass_draw_state WRATHItemDrawState to modify
      \param factory \ref WRATHItemDrawerFactory used to generate the 
                     \ref WRATHItemDrawer from  the shaders of each pass
      \param subdrawer_id SubDrawerID passed to \ref WRATHItemDrawerFactory
                          to generate the \ref WRATHItemDrawer from 
                           the shaders of each pass
      \param primitive_type GL enumeration for the primitive type, for example
                            GL_TRIANGLES
     */
    void
    set_item_draw_state_value(std::set<WRATHItemDrawState> &multi_pass_draw_state,
                              const WRATHItemDrawerFactory &factory, int subdrawer_id,
                              GLenum primitive_type) const
    {
      for(unsigned int i=0, endi=m_draw_passes.size(); i<endi; ++i)
        {
          WRATHItemDrawState draw_state;
          set_item_draw_state_value(draw_state, i, factory, subdrawer_id, primitive_type);
          multi_pass_draw_state.insert(draw_state);
        }
    }

    /*!\var m_packer
      Specifies and creates the attribute data for the item.
     */
    const AttributePacker *m_packer;
    
    /*!\var m_immutable_packing_data
      Holds a handle to immutable data that affects attribute 
      packing, this handle is passed to the attribute packer.
     */
    WRATHStateBasedPackingData::handle m_immutable_packing_data;

    /*!\var m_draw_passes   
      Each element of the array \ref
      m_draw_passes specifies how
      one draw pass of the item
      is drawn (shader and GL-state vector)
     */
    std::vector<Pass> m_draw_passes;

    /*!\var m_buffer_object_hint   
      Buffer object hint as in \ref
      WRATHItemDrawState::m_buffer_object_hint.
      Specifies the storage for index
      data of the item.  Default value 
      is GL_STATIC_DRAW.
     */
    GLenum m_buffer_object_hint;
  };
}

/*! @} */

#endif
