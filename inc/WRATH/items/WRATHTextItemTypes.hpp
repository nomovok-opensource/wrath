/*! 
 * \file WRATHTextItemTypes.hpp
 * \brief file WRATHTextItemTypes.hpp
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




#ifndef WRATH_HEADER_TEXT_ITEM_TYPES_HPP_
#define WRATH_HEADER_TEXT_ITEM_TYPES_HPP_


/*! \addtogroup Items
 * @{
 */


#include "WRATHConfig.hpp"
#include <string>
#include <vector>
#include <boost/utility.hpp>
#include "c_array.hpp"
#include "WRATHTextureFontDrawer.hpp"
#include "WRATHTextureFont.hpp"
#include "WRATHCanvas.hpp"
#include "WRATHTextData.hpp"
#include "WRATHFormattedTextStream.hpp"
#include "WRATHTextDataStream.hpp"
#include "WRATHDefaultTextAttributePacker.hpp"
#include "WRATHFontShaderSpecifier.hpp"


/*!\namespace WRATHTextItemTypes
  Namespace to encapsulate types used for costruction
  of \ref WRATHTextItem objects.
 */
namespace WRATHTextItemTypes
{
  /*!\class TextDrawerPacker 
    Class to specify the default drawer
    and packing to be used for a WRATHTextItem,
    i.e. the font shader specified by a 
    TextDrawerPacker is used for the text
    in a WRATHTextDataStream before encountering
    a WRATHText::set_font_shader() (see \ref WRATHText::font_shader) 
    and the packer specified by a TextDrawerPacker
    is used before encountering a WRATHText::set_packer()
    (see \ref WRATHText::font_packer)
   */
  class TextDrawerPacker 
  {
  public:
    
    /*!\fn TextDrawerPacker(const WRATHFontShaderSpecifier*,
                            const WRATHTextAttributePacker*)
      Ctor. 
      \param shader value to which to set \ref m_shader_specifier
      \param packer value to which to set \ref m_attribute_packer
     */
    TextDrawerPacker(const WRATHFontShaderSpecifier *shader=&WRATHFontShaderSpecifier::default_aa(),
                     const WRATHTextAttributePacker *packer=WRATHDefaultTextAttributePacker::fetch()):
      m_shader_specifier(shader),
      m_attribute_packer(packer)
    {}

    /*!\fn TextDrawerPacker(const WRATHTextAttributePacker*)
      Ctor. Textizes \ref m_shader_specifier as 
      \ref WRATHFontShaderSpecifier::default_aa()
      \param packer value to which to set \ref m_attribute_packer
     */
    explicit
    TextDrawerPacker(const WRATHTextAttributePacker *packer):
      m_shader_specifier(&WRATHFontShaderSpecifier::default_aa()),
      m_attribute_packer(packer)
    {}

    
    /*!\var m_shader_specifier
      Specifies how the a \ref WRATHTextItem is
      drawn in GLSL.
     */
    const WRATHFontShaderSpecifier *m_shader_specifier;

    /*!\var m_attribute_packer
      Specifies and creates the attribute data
      which \ref m_shader_specifier processes to draw
      the \ref WRATHTextItem.
     */
    const WRATHTextAttributePacker *m_attribute_packer;
  };

  
  /*!\enum text_opacity_t
    Enumeration to describe if a WRATHTextItem
    is to be constructed as opaque or transparent
   */
  enum text_opacity_t
    {
      /*!
        WRATHTextItem is drawn as transparent
       */
      text_transparent,

      /*!
        WRATHTextItem is drawn as opaque.
       */
      text_opaque,

      /*!
        WRATHTextItem is opaque AND is 
        not anti-aliased, thus text
        is drawn in one (opaque) pass
       */
      text_opaque_non_aa,
    };

  
  /*!\class TextDrawOrder
    Used to specify the drawing order of glyphs.
    Specifies the WRATHTextureFont::DrawTypeSpecifier
    object to use to determine the WRATHDrawType
    value to use for each drawing pass of text
    and specifies the WRATHDrawOrder object to
    force drawing order of a text item within
    a WRATHDrawType.
   */
  class TextDrawOrder
  {
  public:

    /*!\fn TextDrawOrder(const WRATHDrawOrder::handle&,
                         const WRATHTextureFontDrawer::DrawTypeSpecifier::const_handle&,
                         int)
      Ctor. 
      \param value draw order for both transparency and opaque pass
      \param spec handle to WRATHTextureFont::DrawTypeSpecifier object
      \param pitem_pass the "item" pass for the text item, value is passed
                        to the WRATHTextureFont::DrawTypeSpecifier object 
                        to help determine the WRATHDrawType value
                        to use for the text item.
    */
    TextDrawOrder(const WRATHDrawOrder::handle& value=WRATHDrawOrder::handle(), 
                  const WRATHTextureFontDrawer::DrawTypeSpecifier::const_handle &spec
                  =WRATHTextureFontDrawer::default_pass_specifier(),
                  int pitem_pass=0):
      m_pass_specifier(spec),
      m_item_pass(pitem_pass),
      m_values(value, value)
    {}

    /*!\fn TextDrawOrder(const WRATHDrawOrder::handle&,
                         const WRATHDrawOrder::handle&,
                         const WRATHTextureFontDrawer::DrawTypeSpecifier::const_handle&,
                         int)
      Ctor. 
      \param opaque_value opaque draw order
      \param transparency_value transparent draw order
      \param spec handle to WRATHTextureFont::DrawTypeSpecifier object
      \param pitem_pass the "item" pass for the text item, value is passed
                        to the WRATHTextureFont::DrawTypeSpecifier object 
                        to help determine the WRATHDrawType value
                        to use for the text item.
     */
    TextDrawOrder(const WRATHDrawOrder::handle& transparency_value, 
                  const WRATHDrawOrder::handle& opaque_value,
                  const WRATHTextureFontDrawer::DrawTypeSpecifier::const_handle &spec
                  =WRATHTextureFontDrawer::default_pass_specifier(),
                  int pitem_pass=0):
      m_pass_specifier(spec),
      m_item_pass(pitem_pass),
      m_values(opaque_value, transparency_value)
    {}

    /*!\fn TextDrawOrder(int,
                         const WRATHTextureFontDrawer::DrawTypeSpecifier::const_handle&)
      Ctor. 
      \param spec handle to WRATHTextureFontDrawer::DrawTypeSpecifier object
      \param pitem_pass the "item" pass for the text item, value is passed
                        to the WRATHTextureFont::DrawTypeSpecifier object 
                        to help determine the WRATHDrawType value
                        to use for the text item.
     */
    explicit
    TextDrawOrder(int pitem_pass,
                  const WRATHTextureFontDrawer::DrawTypeSpecifier::const_handle &spec
                  =WRATHTextureFontDrawer::default_pass_specifier()):
      m_pass_specifier(spec),
      m_item_pass(pitem_pass),
      m_values(WRATHDrawOrder::handle(),
               WRATHDrawOrder::handle())
    {}

    /*!\var m_pass_specifier
      Handle to \ref WRATHTextureFontDrawer::DrawTypeSpecifier
      (i.e. a \ref WRATHTwoPassDrawer::DrawTypeSpecifier) 
      to specify the WRATHDrawType values to use for each drawing
      pass of text.
     */
    WRATHTextureFontDrawer::DrawTypeSpecifier::const_handle m_pass_specifier;

    /*!\var m_item_pass
      The "item" pass for the text item, value is passed
      to \ref WRATHTextureFontDrawer::DrawTypeSpecifier::draw_type()
      to help determine the WRATHDrawType value
      to use for the text item.
     */
    int m_item_pass;

    /*!\fn TextDrawOrder& pass_specifier
      Set \ref m_pass_specifier
      \param v value to which to set \ref m_pass_specifier
     */
    TextDrawOrder&
    pass_specifier(const WRATHTextureFontDrawer::DrawTypeSpecifier::const_handle &v)
    {
      m_pass_specifier=v;
      return *this;
    }

    /*!\fn TextDrawOrder& item_pass
      Set \ref m_item_pass
      \param v value to which to set \ref m_item_pass
     */
    TextDrawOrder&
    item_pass(int v)
    {
      m_item_pass=v;
      return *this;
    }

    /*!\fn TextDrawOrder& opaque_draw_order(const WRATHDrawOrder::handle&)
      Set the opaque draw order.
      \param v value to set the opaque draw order to.
     */
    TextDrawOrder&
    opaque_draw_order(const WRATHDrawOrder::handle& v) 
    {
      m_values[0]=v;
      return *this;
    }
    
    /*!\fn TextDrawOrder& transparency_draw_order(const WRATHDrawOrder::handle&)
      Set the transparency draw order.
      \param v value to set the transparency draw order to.
     */
    TextDrawOrder&
    transparency_draw_order(const WRATHDrawOrder::handle& v) 
    {
      m_values[1]=v;
      return *this;
    }
        
    /*!\fn TextDrawOrder& draw_orders(const WRATHDrawOrder::handle&)
      Set both draw orders.
      \param v value to set the draw orders to.
     */
    TextDrawOrder&
    draw_orders(const WRATHDrawOrder::handle& v)
    {
      m_values[0]=v;
      m_values[1]=v;
      return *this;
    }

    /*!\fn const WRATHDrawOrder::handle& opaque_draw_order(void) const   
      Get the opaque draw order.
     */
    const WRATHDrawOrder::handle&
    opaque_draw_order(void) const
    {
      return m_values[0];
    }

    /*!\fn const WRATHDrawOrder::handle& transparency_draw_order(void) const
      Get the transparency draw order.
     */
    const WRATHDrawOrder::handle&
    transparency_draw_order(void) const
    {
      return m_values[1];
    }    

    /*!\fn const WRATHDrawOrder::handle& named_draw_order
      Returns opaque_draw_order() if the passed enumeration
      is an opaque pass, otherwise returns transparency_draw_order().
      \param tp enumeration stating which draw pass as a 
                WRATHTextureFontDrawer::drawing_pass_type
     */
    const WRATHDrawOrder::handle&
    named_draw_order(enum WRATHTextureFontDrawer::drawing_pass_type tp) const
    {
      return (tp==WRATHTextureFontDrawer::opaque_draw_pass)?
        opaque_draw_order():
        transparency_draw_order();
    }

  private:
    vecN<WRATHDrawOrder::handle, 2> m_values;
  };
  
  /*!\class TextExtraDrawState
    An TextExtraDrawState is just a wrapper over
    three WRATHSubItemDrawState's: state common
    applied to both passes, a set of state applied
    only to the opaque pass and a set of state
    only applied to the translucent pass.
   */
  class TextExtraDrawState
  {
  public:

    /*!\fn TextExtraDrawState(void)
      Ctor initializing the TextExtraDrawState
      as empty.
     */
    TextExtraDrawState(void)
    {}

    /*!\fn TextExtraDrawState(const WRATHSubItemDrawState&)
      Ctor, intializes the extra state common
      to both passes as the passed WRATHSubItemDrawState.

      \param pcommon_state value with which to
                           initialize m_common_pass_state
     */
    TextExtraDrawState(const WRATHSubItemDrawState &pcommon_state):
      m_common_pass_state(pcommon_state)
    {}

    /*!\var m_named_pass_state
      Holds the extra state for each pass
      that is not common to both.
      - m_named_pass_state[0] holds state for opaque pass
      - m_named_pass_state[1] holds state for transparent pass
     */
    vecN<WRATHSubItemDrawState, 2> m_named_pass_state;

    /*!\var m_common_pass_state
      Holds the extra state that is
      common to both passes.
     */
    WRATHSubItemDrawState m_common_pass_state;

    /*!\fn WRATHSubItemDrawState& opaque_pass_state
      Returns the extra state of
      which is only for the opaque 
      pass.
     */
    WRATHSubItemDrawState&
    opaque_pass_state(void)
    {
      return m_named_pass_state[0];
    }

    /*!\fn WRATHSubItemDrawState& translucent_pass_state
      Returns the extra state of
      which is only for the 
      translucent pass.
     */
    WRATHSubItemDrawState&
    translucent_pass_state(void)
    {
      return m_named_pass_state[1];
    }

    /*!\fn WRATHSubItemDrawState& named_state
      Returns the extra state of which is only for the named 
      pass with the name given as a WRATHTextureFontDrawer::drawing_pass_type,
      WRATHTextureFontDrawer::opaque_draw_pass returns the
      opaque pass and both WRATHTextureFontDrawer::pure_transluscent
      and WRATHTextureFontDrawer::transluscent_draw_pass
      returns the transluscent extra state.
      \param tp named pass to of whose extra state to return 
     */
    WRATHSubItemDrawState&
    named_state(enum WRATHTextureFontDrawer::drawing_pass_type tp)
    {
      return m_named_pass_state[tp<2?tp:1];
    }
  };
 
}


/*! @} */
#endif
