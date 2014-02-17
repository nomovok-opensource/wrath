/*! 
 * \file WRATHTextItem.hpp
 * \brief file WRATHTextItem.hpp
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




#ifndef WRATH_HEADER_TEXT_ITEM_HPP_
#define WRATH_HEADER_TEXT_ITEM_HPP_

#include "WRATHConfig.hpp"
#include "WRATHBaseItem.hpp"
#include "WRATHBasicTextItem.hpp"
#include "WRATHFontShaderSpecifier.hpp"

/*! \addtogroup Items
 * @{
 */

/*!\class WRATHTextItem
  A WRATHTextItem represents an item of text
  (interally it uses a collection of \ref
  WRATHBasicTextItem objects). A WRATHTextItem
  specifies how to draw text with a \ref
  WRATHFontShaderSpecifier object and allows
  for the font (and font type!) to change 
  mid-stream.
 */
class WRATHTextItem:public WRATHBaseItem
{
public:

  /*!\typedef draw_order
    Conveniance typedef taken from WRATHText
   */
  typedef WRATHTextItemTypes::TextDrawOrder draw_order;
  
  /*!\typedef ExtraDrawState
    Conveniance typedef taken from WRATHTextItemTypes.
    Used to append additional GL-state vector on a
    WRATHTextItem. However, the texture binder field
    (\ref WRATHSubItemDrawState::m_textures)
    are _ignored_. To set additional texture units use
    \ref WRATHText::set_additional_sampler within a 
    text stream added with \ref add_text(). Additionally,
    since a fixed WRATHTextItem object might have different
    fonts and different programs of portions of text,
    the uniform objects (\ref WRATHSubItemDrawState::m_uniforms)
    should be so that they are not tied to a particular
    WRATHGLProgram. In particular the 
    \ref WRATHUniformData::uniform_by_name_base 
    derived objects are fine.
   */
  typedef WRATHTextItemTypes::TextExtraDrawState ExtraDrawState;

  /*!\typedef Drawer
    Conveniance typedef taken from WRATHTextItemTypes.
   */
  typedef WRATHTextItemTypes::TextDrawerPacker Drawer;

  /*!\fn WRATHTextItem
    Ctor. 
    \param fact WRATHItemDrawerFactory responsible for fetching/creating
                the WRATHItemDrawer(s) used by the item
    \param psubdrawer_id SubDrawer Id passed to \ref WRATHItemDrawerFactory::generate_drawer()
    \param pcontainer WRATHCanvas where created item is placed, Canvas does NOT own the item.
    \param subkey SubKey used by pcanvas, typically holds "what" transformation/clipping node to use
    \param item_opacity dictates the opacity of the text drawn by the created WRATHTextItem
    \param pdrawer specifies intial value for drawer and packer, i.e. is the drawer/packer
                   used for the portions of text in a text stream which do not have drawer/packer set.
    \param pdraw_order draw order specification of opaque and transluscent portions of text.
    \param extra_state extra GL state passed to the WRATHTextItem of this
                       WRATHTextItem
   */
  WRATHTextItem(const WRATHItemDrawerFactory &fact,
                int psubdrawer_id,
                WRATHCanvas *pcontainer,
                const WRATHCanvas::SubKeyBase &subkey,
                enum WRATHTextItemTypes::text_opacity_t item_opacity,
                const Drawer &pdrawer=Drawer(),
                const draw_order &pdraw_order=draw_order(),
                const ExtraDrawState &extra_state=ExtraDrawState());
   
  virtual
  ~WRATHTextItem();

  virtual
  WRATHCanvas*
  canvas_base(void) const
  {
    return m_group;
  }

  virtual
  void
  canvas_base(WRATHCanvas *c);
    
  /*!\fn void add_text(const WRATHTextDataStream&)
    Add text to the WRATHTextItem.
    \param ptext formatted text stream.
   */
  void
  add_text(const WRATHTextDataStream &ptext)
  {
    add_text(ptext.formatted_text(),
             ptext.state_stream());
  }
 
  /*!\fn void add_text(const WRATHFormattedTextStream&, const WRATHStateStream&)
    Add text to the WRATHTextItem.
    \param ptext formatted text stream.
    \param state_stream change state stream which indicates
                        state changes of ptext (such as font changes)
   */
  void
  add_text(const WRATHFormattedTextStream &ptext,
           const WRATHStateStream &state_stream)
  {
    range_type<int> R(0, ptext.data_stream().size());
    add_text(R, ptext, state_stream);
  }

  /*!\fn void add_text(range_type<int> R, const WRATHFormattedTextStream&, const WRATHStateStream&)
    Add text to the WRATHTextItem.
    \param R range of formatted characters of the text stream to add
    \param ptext formatted text stream.
    \param state_stream change state stream which indicates
                        state changes of ptext (such as font changes)
   */
  void
  add_text(range_type<int> R,
           const WRATHFormattedTextStream &ptext,
           const WRATHStateStream &state_stream);

  /*!\fn void clear
    Clears the WRATHTextItem, i.e. set it
    to draw no text.
   */
  void
  clear(void);
    
  /*!\fn const WRATHTextAttributePacker::BBox& bounding_box
    Returns the bouning box of this
    WRATHTextItem content (i.e. formatted
    attribute data).
   */
  const WRATHTextAttributePacker::BBox&
  bounding_box(void) const
  {
    return m_box;
  }

private:
  
  typedef vecN<WRATHText::additional_texture, 
               WRATHText::number_additional_textures_supported> texture_array;
  
  
  typedef boost::tuple<WRATHBasicTextItem::draw_method, 
                       WRATHTextureFont*,
                       texture_array,
                       const WRATHFontShaderSpecifier*> text_item_key;


  WRATHBasicTextItem*
  get_empty_text_item(text_item_key k);
       
  void
  add_text_implement(c_array<range_type<int> > Rarray,
                     const WRATHFormattedTextStream &ptext,
                     const WRATHStateStream &state_stream,
                     WRATHBasicTextItem::draw_method pdrawer,
                     WRATHTextureFont *fnt, 
                     const texture_array &texes,
                     const WRATHFontShaderSpecifier *spec);
    
  
  WRATHCanvas::SubKeyBase *m_subkey;
  ExtraDrawState m_extra_state;
  WRATHCanvas *m_group;
  Drawer m_default_drawer;
  draw_order m_draw_order;
  enum WRATHTextItemTypes::text_opacity_t m_text_opacity;
  WRATHItemDrawerFactory *m_factory;
  int m_sub_drawer_id;

  WRATHTextAttributePacker::BBox m_box;
  std::list<WRATHBasicTextItem*> m_all_items;
  std::map<text_item_key, std::list<WRATHBasicTextItem*> > m_cleared_items;
  std::map<text_item_key, std::list<WRATHBasicTextItem*> > m_uncleared_items;

  
};

/*! @} */



#endif
