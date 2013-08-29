/*! 
 * \file WRATHBasicTextItem.hpp
 * \brief file WRATHBasicTextItem.hpp
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




#ifndef __WRATH_BASIC_TEXT_ITEM_HPP__
#define __WRATH_BASIC_TEXT_ITEM_HPP__

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
#include "WRATHTextItemTypes.hpp"

/*! \addtogroup Items
 * @{
 */

/*!\class WRATHBasicTextItem
  A WRATHBasicTextItem represents drawing
  an item of text for which the font
  and drawer of the text is the same.
  
  The interface for a WRATHBasicTextItem is
  very low level: the WRATHTextureFontDrawer 
  is specified at the ctor. For most
  use cases, a developer should use
  WRATHTextItem as it specifies how
  text is drawn with a WRATHFontShaderSpecifier
  and allows for that and the font to change
  mid-stream.  
 */
class WRATHBasicTextItem:public boost::noncopyable
{
public:

  /*!\typedef draw_order
    Conveniance typedef to WRATHTextItemTypes::TextDrawOrder
   */
  typedef WRATHTextItemTypes::TextDrawOrder draw_order;

  /*!\typedef ExtraDrawState
    Conveniance typedef to WRATHTextItemTypes::TextExtraDrawState 
   */
  typedef WRATHTextItemTypes::TextExtraDrawState ExtraDrawState;
  
  /*!\class draw_method
    A draw_method determines how text is drawn
    by specifying a WRATHTextureFontDrawer and
    a WRATHTextAttributePacker. If no 
    WRATHTextAttributePacker is specified,
    the draw_method automatically init's the value
    WRATHDefaultTextAttributePacker::fetch().
   */
  class draw_method
  {
  public:

    /*!\fn draw_method(WRATHTextureFontDrawer*)
      Ctor, sets m_program_drawer as specified
      and \ref m_attribute_packer as WRATHDefaultTextAttributePacker::fetch().
      \param p WRATHTextureFontDrawer to use to draw the font
     */
    draw_method(WRATHTextureFontDrawer *p=NULL):
      m_program_drawer(p),
      m_attribute_packer(WRATHDefaultTextAttributePacker::fetch())
    {}

    /*!\fn draw_method(WRATHTextureFontDrawer*, const WRATHTextAttributePacker*)
      Ctor, setting both m_program_drawer and m_attribute_packer
      \param p WRATHTextureFontDrawer to use to draw the font
      \param q WRATHTextAttributePacker to pack the attributes
     */
    draw_method(WRATHTextureFontDrawer *p,
                const WRATHTextAttributePacker *q):
      m_program_drawer(p),
      m_attribute_packer(q)
    {}

    /*!\fn draw_method(const WRATHTextAttributePacker*, WRATHTextureFontDrawer*)
      Ctor, setting both m_program_drawer and m_attribute_packer
      \param q WRATHTextAttributePacker to pack the attributes
      \param p WRATHTextureFontDrawer to use to draw the font
     */
    draw_method(const WRATHTextAttributePacker *q,
                WRATHTextureFontDrawer *p):
      m_program_drawer(p),
      m_attribute_packer(q)
    {}

    /*!\var m_program_drawer
      WRATHTextureFontDrawer used to draw the font.
     */
    WRATHTextureFontDrawer *m_program_drawer;

    /*!\var m_attribute_packer
      WRATHTextAttributePacker used to pack attribute
      data consumed by \ref m_program_drawer.
     */
    const WRATHTextAttributePacker *m_attribute_packer;
    
    /*!\fn bool operator<(const draw_method&) const
      Comparison operator, sort first by
      m_program_drawer then by m_attribute_packer.
      \param rhs object with which to compare
     */
    bool
    operator<(const draw_method &rhs) const;
    
    /*!\fn bool operator!=(const draw_method&) const
      Inquality operator.
      \param rhs object with which to compare
     */
    bool
    operator!=(const draw_method &rhs) const
    {
      return m_program_drawer!=rhs.m_program_drawer
        or m_attribute_packer!=rhs.m_attribute_packer;
    }
    
    /*!\fn bool operator==(const draw_method&) const
      Equality operator.
      \param rhs object with which to compare
     */
    bool
    operator==(const draw_method &rhs) const
    {
      return m_program_drawer==rhs.m_program_drawer
        and m_attribute_packer==rhs.m_attribute_packer;
    }
  };


  /*!\fn WRATHBasicTextItem
    Ctor. About texturing: WRATHTextureFont's export what
    texture(s) to use via the method texture_binder(),
    the drawn items of a  WRATHBasicTextItem builds their 
    drawing by first setting GL_TEXTUREi to the
    font's texture_binder()[i], and then applying
    (and thus possibly replacing) the textures as 
    specified in extra_state. If text is viewed as 
    opaque, then non-antialiased portions are drawn 
    in an  opaue pass and aliased portions are drawn 
    in a transparent pass.
    \param pdrawer that does the drawing and attribute
                   packing of the text
    \param subkey SubKey used to create draw groups for text items
    \param pcontainer WRATHCanvas that holds the text
    \param pfont font to draw the texture with
    \param opacity_type opacity of text drawn by WRATHBasicTextItem
    \param extra_state extra GL state for drawing the WRATHBasicTextItem
    \param pdraw_order draw order specification of opaque and
                       transluscent portions of text.
   */
  WRATHBasicTextItem(draw_method pdrawer,
                     const WRATHCanvas::SubKeyBase &subkey,
                     WRATHCanvas *pcontainer,
                     WRATHTextureFont *pfont,
                     enum WRATHTextItemTypes::text_opacity_t opacity_type,
                     const draw_order &pdraw_order=draw_order(),
                     const ExtraDrawState &extra_state=ExtraDrawState());

 
  virtual
  ~WRATHBasicTextItem();

  /*!\fn WRATHCanvas* canvas(void) const
    Returns the WRATHCanvas on which the item resides
   */
  WRATHCanvas*
  canvas(void) const
  {
    return m_group_collection;
  }

  /*!\fn void canvas(WRATHCanvas*)
    Sets the WRATHCanvas on which the item resides
   */
  void
  canvas(WRATHCanvas *c);

  /*!\fn void set_text(const_c_array<range_type<int> >,
                       const WRATHFormattedTextStream&,
                       const WRATHStateStream&)
    Main API point for setting the text of 
    a WRATHBasicTextItem. Will only use those
    glyphs which use font().
    \param R array of range of characters to display of ptext
    \param ptext text formatted text stream to display.
    \param state_stream change state stream which indicates
                        state changes of ptext (such as font changes).
   */
  void
  set_text(const_c_array<range_type<int> > R,
           const WRATHFormattedTextStream &ptext, 
           const WRATHStateStream &state_stream);

  /*!\fn void set_text(range_type<int> R,
                       const WRATHFormattedTextStream&,
                       const WRATHStateStream&)
    API point for setting the text of 
    a WRATHBasicTextItem. Will only use those
    glyphs which use font().
    \param R range of characters to display of ptext
    \param ptext text formatted text stream to display.
    \param state_stream change state stream which indicates
                        state changes of ptext (such as font changes).
   */
  void
  set_text(range_type<int> R,
           const WRATHFormattedTextStream &ptext, 
           const WRATHStateStream &state_stream)
  {
    const_c_array<range_type<int> > arrayR(&R, 1);
    set_text(arrayR, ptext, state_stream);
  }


  /*!\fn void set_text(range_type<int>, const WRATHTextDataStream&)
    Set the text displayed by this WRATHBasicTextItem.
    Will only use those glyphs which use font().
    \param R range of characters to display of ptext
    \param ptext text stream to display.
   */
  void
  set_text(range_type<int> R,
           const WRATHTextDataStream &ptext)
  {    
    set_text(R, 
             ptext.formatted_text(), 
             ptext.state_stream());
  }

  /*!\fn void set_text(const WRATHTextDataStream&)
    Set the text displayed by this WRATHBasicTextItem.
    Will only use those glyphs which use font().
    \param ptext text stream to display.
   */
  void
  set_text(const WRATHTextDataStream &ptext)
  {    
    range_type<int> R(0, ptext.raw_text().character_data().size());
    set_text(R, 
             ptext.formatted_text(), 
             ptext.state_stream());
  }

  /*!\fn void clear 
    Clears all text of this WRATHBasicTextItem, making it empty.
   */
  void
  clear(void);

  /*!\fn WRATHTextureFont* font
    Returns the WRATHTextureFont that this WRATHBasicTextItem
    uses.
   */
  WRATHTextureFont*
  font(void) const
  {
    return m_font;
  }

  /*!\fn const WRATHTextAttributePacker::BBox& bounding_box
    Returns the bouning box of this
    WRATHBasicTextItem content (i.e. formatted
    attribute data).
   */
  const WRATHTextAttributePacker::BBox&
  bounding_box(void) const
  {
    return m_box;
  }

private:

  class per_page_type
  {
  public:
    per_page_type(int page, WRATHBasicTextItem *parent);

    ~per_page_type();

    void
    clear(void);

    void
    set_text(const_c_array<range_type<int> > R,
             const WRATHFormattedTextStream &ptext, 
             const WRATHStateStream &state_stream,
             WRATHTextAttributePacker::BBox *out_bounds_box);
    
    void
    canvas(WRATHCanvas *c);

  private:

    void
    allocate_room_if_needed(void);

    void
    change_attribute_store(void);
    
    WRATHBasicTextItem *m_parent;
    std::set<WRATHItemDrawState> m_key;
    WRATHAttributeStoreKey m_attribute_key;
    int m_texture_page;
    WRATHAttributeStore::handle m_attribute_store;
 
    WRATHCanvas::DataHandle m_item_group;
    std::vector<range_type<int> > m_attribute_location;
    WRATHTextAttributePacker::allocation_requirement_type m_required, m_allocated;
    WRATHIndexGroupAllocator::index_group<GLushort> m_index_data_location;
  };


  void
  preallocate_subitems(unsigned int number_pages);

  void
  init(enum WRATHTextItemTypes::text_opacity_t opacity_type);

  void
  generate_key(std::set<WRATHItemDrawState> &out_key,
               WRATHAttributeStoreKey &attribute_key,
               int page);
  

  WRATHCanvas::SubKeyBase *m_subkey;
  ExtraDrawState m_extra_state;
  WRATHCanvas *m_group_collection;
  WRATHTextureFont *m_font;
  WRATHTextureFontDrawer *m_drawer;
  const WRATHTextAttributePacker *m_packer;
  draw_order m_draw_order;

  WRATHTextAttributePacker::BBox m_box;  
  
  std::vector<enum WRATHTextureFontDrawer::drawing_pass_type> m_passes;
  std::vector<per_page_type*> m_items;
};
/*! @} */


#endif
