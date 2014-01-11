/*! 
 * \file WRATHTextAttributePacker.hpp
 * \brief file WRATHTextAttributePacker.hpp
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




#ifndef __WRATH_TEXT_ATTRIBUTE_PACKER_HPP__
#define __WRATH_TEXT_ATTRIBUTE_PACKER_HPP__

#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include "c_array.hpp"
#include "WRATHTextData.hpp"
#include "WRATHFormattedTextStream.hpp"
#include "WRATHTextDataStream.hpp"
#include "WRATHResourceManager.hpp"
#include "WRATHCanvas.hpp"
#include "WRATHBBox.hpp"
#include "WRATHTextureFontDrawer.hpp"
#include "WRATHAttributePacker.hpp"

/*! \addtogroup Text
 * @{
 */

/*!\class WRATHTextAttributePacker
  A WRATHTextAttributePacker specifies 
  how attributes are packed for font 
  drawing. Derived classes of
  WRATHTextAttributePacker are used for
  packing of attribute data by
  WRATHTextItem. Changing, querying, etc
  what WRATHFontShaderSpecifier is applied
  in a \ref WRATHTextDataStream is done
  by the manipulator created with
  \ref WRATHText::font_packer.
 */
class WRATHTextAttributePacker:boost::noncopyable
{
public:
  
  /*!\typedef BBox  
    Bounding box type
   */
  typedef WRATHBBox<2,float> BBox;

  /*!\class allocation_requirement_type
    An allocation_requirement holds the number
    of attributes and indices required to display
    a block of text.
   */
  class allocation_requirement_type
  {
  public:
    allocation_requirement_type(void):
      m_number_attributes(0),
      m_number_indices(0)
    {}

    /*!\var m_number_attributes
      Number of attributes required, the attributes
      do NOT need to be allocated in one block.
     */
    int m_number_attributes;

    /*!\var m_number_indices
      Number of indices required, the indices
      do need to be allocated in one block.
     */
    int m_number_indices;

    /*!\fn empty
      Returns true if \ref m_number_indices is zero.
     */
    bool
    empty(void) const
    {
      return m_number_indices==0;
    }
  };

  /*!\class allocation_allotment_type
    A allocation_allotment_type is the return type
    for allocation_allotment() which conveys
    what ranges from an array of ranges of
    a text stream can fit using no more than
    a given number of attributes.
   */
  class allocation_allotment_type
  {
  public:
    /*\!fn
      Ctor providing conveniant default values.
     */
    allocation_allotment_type(void):
      m_room_for_all(true),
      m_number_attributes(0),
      m_handled_end(0),
      m_sub_end(0)
    {}
    
    /*!\var m_room_for_all
      If true, there was sufficient room in attribute
      data to store all character ranges. 
      Initialized as _true_.
     */
    bool m_room_for_all;

    /*!\var m_number_attributes
      Number of attributes that would be used
      storing the indicated character ranges.
     */
    int m_number_attributes;

    /*!\var m_handled_end
      Index to one past the last range that
      can be completely handled. Initialized as 0.
     */
    int m_handled_end;

    /*!\var m_sub_end
      If the entire array of ranges cannot be handled
      this stores one past the last index of the
      subrange indexed by \ref m_handled_end that 
      is handled, i.e. handled elements are:

      - all of Rinput[0], Rinput[1], ... , Rinput[m_handled_end-1]
      - if m_handled_end<input.size(), also handles 
        [ Rinput[m_handled_end].m_begin, m_sub_end )

      where Rinput as in the second argument to 
      allocation_allotment().

      Initialized as 0.
     */
    int m_sub_end;
  };

  /*!\typedef ResourceKey
    Resource key type for WRATHAttributePacker 
    resource manager.
   */
  typedef std::string ResourceKey;

  /// @cond
  WRATH_RESOURCE_MANAGER_DECLARE(WRATHTextAttributePacker, ResourceKey);
  /// @endcond
  
  /*!\fn WRATHTextAttributePacker(const ResourceKey&)
    Ctor. Specifies the resource name of the attribute packer
    and the names of each attribute as an STL range.
    The number of attributes is then std::distance(begin,end)
    and the attribute of index I has value begin+I.
    \param pname resource name to identify the WRATHTextAttributePacker
   */
  explicit
  WRATHTextAttributePacker(const ResourceKey &pname);
  
  virtual
  ~WRATHTextAttributePacker();

  /*!\fn const ResourceKey& resource_name(void)
    returns the resource name of this WRATHAttributePacker.
   */
  const ResourceKey&
  resource_name(void) const
  {
    return m_resource_name;
  }

  /*!\fn attribute_names
    To be implemented by a derived class to
    return the names of attrivutes packed
    by the WRATHTextAttributePacker
    \param out_names array to which to resize and write the
                     attribute names
    \param number_custom_data_to_use indicates the size of \ref
                                     WRATHTextureFont::GlyphGLSL::m_custom_data_use
                                     which are to also be packed into attributes
   */
  virtual
  void
  attribute_names(std::vector<std::string> &out_names,
                  int number_custom_data_to_use) const=0;

  /*!\fn fetch_attribute_packer
    Returns the WRATHAttributePacker of this WRATHTextAttributePacker
    \param number_custom_data_to_use indicates the size of \ref
                                     WRATHTextureFont::GlyphGLSL::m_custom_data_use
                                     which are to also be packed into attributes
   */
  const WRATHAttributePacker*
  fetch_attribute_packer(int number_custom_data_to_use) const;

  /*!\fn generate_custom_data_glsl
    To be implemented by a derived class to generate the
    GLSL code that implements
    \code
    void wrath_font_shader_custom_data_func(out wrath_font_custom_data_t)
    \endcode
    where 
    \code
    struct wrath_font_custom_data_t
    {
      float highp values[N];
    }
    \endcode
    is previously defined and N=number_custom_data_to_use
    \param number_custom_data_to_use indicates the size of \ref
                                     WRATHTextureFont::GlyphGLSL::m_custom_data_use
                                     which are to also be packed into attributes
   */
  virtual
  void
  generate_custom_data_glsl(WRATHGLShader::shader_source &out_src,
                            int number_custom_data_to_use) const=0;

  /*!\fn allocation_requirement_type allocation_requirement
    To be implemented by a derived class to indicate
    how many attributes and indices are required to
    display a set of blocks of text.
    \param R array of ranges of glyphs of pdata to display
    \param font font to use, only those glyphs of that font
                and the specified texture page are considered
    \param texture_page texture page to use, only those glyphs 
                        of that texture page and the specified
                        font are considered.
    \param pdata formatted text stream from which to get the character data
    \param state_stream state change stream of pdata 
   */
  virtual
  allocation_requirement_type
  allocation_requirement(const_c_array<range_type<int> > R,
                         WRATHTextureFont *font,
                         int texture_page,
                         const WRATHFormattedTextStream &pdata,
                         const WRATHStateStream &state_stream) const=0;

  /*!\fn allocation_allotment_type allocation_allotment
    To be implemented by a derived class to provide
    the largest sub-array of sub-ranges that can
    be packed using no more than a given number of attributes. 
    
    \param attributes_allowed maximum number attributes allowed
    \param Rinput input of ranges to investigate
    \param pdata formatted text stream to get the characters from
    \param state_stream state change stream of pdata  
   */
  virtual
  allocation_allotment_type
  allocation_allotment(int attributes_allowed,
                       const_c_array<range_type<int> > Rinput,
                       const WRATHFormattedTextStream &pdata,
                       const WRATHStateStream &state_stream) const=0;


  /*!\fn void set_attribute_data(const_c_array<range_type<int> >,
                            WRATHTextureFont*,
                            int,
                            WRATHAbstractDataSink&,
                            const std::vector<range_type<int> >&,
                            WRATHAbstractDataSink&,
                            const WRATHFormattedTextStream&,
                            const WRATHStateStream&,
                            BBox*) const
    Pack attribute data. Only those glyphs
    within the specified range using
    the named texture page and font are to
    be observed. The caller will have
    allocated attribute and index data
    for those glyphs within the specfied
    range using the specified font and texture
    page. The assumption is that the total number
    of attributes allocated is alteast that which
    is returned by allocation_requirement() and
    the total number of indices needed is also.
    \param R array of ranges of glyphs of pdata to set attribute data
    \param font font to use, only those glyphs of that font
                and the specified texture page are to be packed.
    \param texture_page texture page to use, only those glyphs 
                        of that texture page and the specified
                        font are to be packed.
    \param attribute_store reference to sink to which to write attribute data
    \param attr_location location within the WRATHAttributeStore::handle
                         of the attribute data to write to
    \param index_group reference to sink to which to write index data 
    \param pdata formatted text stream to get the characters from
    \param state_stream state change stream of pdata  
    \param out_bounds_box if non-NULL, or's the packed letters bounding boxes
                          with *out_bounds_box
   */
  void
  set_attribute_data(const_c_array<range_type<int> > R,
                     WRATHTextureFont *font,
                     int texture_page,
                     WRATHAbstractDataSink &attribute_store,
                     const std::vector<range_type<int> > &attr_location,
                     WRATHAbstractDataSink &index_group,
                     const WRATHFormattedTextStream &pdata,
                     const WRATHStateStream &state_stream,
                     BBox *out_bounds_box) const
  {
    WRATHassert(&attribute_store!=NULL);
    WRATHassert(&index_group!=NULL);
    set_attribute_data_implement(R, font, texture_page, 
                                 attribute_store, attr_location,
                                 index_group,
                                 pdata, state_stream, out_bounds_box);
  }   


  /*!\fn void set_attribute_data(const_c_array<range_type<int> >,
                            WRATHTextureFont*,
                            int,
                            WRATHCanvas::DataHandle,
                            const std::vector<range_type<int> > &,
                            WRATHIndexGroupAllocator::index_group<GLushort>,
                            const WRATHFormattedTextStream&,
                            const WRATHStateStream&,
                            BBox*) const
    Packs attribute data. Only those glyphs
    within the specified range using
    the named texture page and font are to
    be observed. The caller will have
    allocated attribute and index data
    for those glyphs within the specfied
    range using the specified font and texture
    page. The assumption is that the total number
    of attributes allocated is alteast that which
    is returned by allocation_requirement() and
    the total number of indices needed is also.
    Provided as a conveniance, equivalent to
    \code
    WRATHassert(item_group.valid());
    WRATHassert(index_group.valid());
    WRATHAttributeStore::DataSink attribute_sink(item_group.data_sink());
    WRATHIndexGroupAllocator::DataSink idx_sink(index_group.data_sink());
    set_attribute_data(R, font, texture_page, 
                       attribute_sink, attr_location, 
                       idx_sink, pdata, state_stream, 
                       out_bounds_box);
    \endcode

    \param R array of ranges of glyphs of pdata to set attribute data
    \param font font to use, only those glyphs of that font
                and the specified texture page are to be packed.
    \param texture_page texture page to use, only those glyphs 
                        of that texture page and the specified
                        font are to be packed.
    \param item_group WRATHCanvas::DataHandle that holds the index 
                      data and a WRATHAttributeStore::handle to the attribute data.
    \param attr_location location within the WRATHAttributeStore::handle
                         of the attribute data to write to
    \param index_group handle to index group to where to pack indices.
                       Indices beyond those that are needed are to be set as 0.
    \param pdata formatted text stream to get the characters from
    \param state_stream state change stream of pdata  
    \param out_bounds_box if non-NULL, or's the packed letters bounding boxes
                          with *out_bounds_box
   */
  void
  set_attribute_data(const_c_array<range_type<int> > R,
                     WRATHTextureFont *font,
                     int texture_page,
                     WRATHCanvas::DataHandle item_group,
                     const std::vector<range_type<int> > &attr_location,
                     WRATHIndexGroupAllocator::index_group<GLushort> index_group,
                     const WRATHFormattedTextStream &pdata,
                     const WRATHStateStream &state_stream,
                     BBox *out_bounds_box) const
  {
    WRATHassert(item_group.valid());
    WRATHassert(index_group.valid());
    WRATHAttributeStore::DataSink attribute_sink(item_group.attribute_store()->data_sink());
    WRATHIndexGroupAllocator::DataSink idx_sink(index_group.data_sink());
    set_attribute_data(R, font, texture_page, 
                       attribute_sink, attr_location, 
                       idx_sink, pdata, state_stream, 
                       out_bounds_box);
  }  

  /*!\fn void compute_bounding_box
    May be implemented by a derived class to OR the
    bounding box enclosing a stream of text.
    Default implementation increments through the
    specified range and enlarges out_bounds to
    contain each of the 4 corners as reported
    by WRATHFormattedText::position(int, float).
    \param R range of characters of pdata to walk
    \param pdata character data stream to walk
    \param state_stream state stream accompanying pdata
    \param out_bounds BBox to expand
   */
  virtual
  void
  compute_bounding_box(range_type<int> R,
                       const WRATHFormattedTextStream &pdata,
                       const WRATHStateStream &state_stream,
                       BBox &out_bounds) const;
    
  /*!\fn void attribute_key
    To be implemented by a derived class to
    fetch the attribute key.
    \param attrib_key WRATHAttributeStoreKey to which to set
    \param number_custom_data_to_use indicates the size of \ref
                                     WRATHTextureFont::GlyphGLSL::m_custom_data_use
                                     which are to also be packed into attributes
   */
  virtual
  void
  attribute_key(WRATHAttributeStoreKey &attrib_key,
                int number_custom_data_to_use) const=0;

  /*!\fn unsigned int number_of_characters(range_type<int>, 
                                           const WRATHFormattedTextStream&,
                                           WRATHTextureFont*, int)
    Returns the number of characters within
    a specified range of a WRATHFormattedTextStream
    that use a specified font and texture page.
    \param R range to examine within pdata
    \param pdata formatted character stream
    \param font to look for
    \param texture_page texture page of font to look for.
   */
  static
  unsigned int
  number_of_characters(range_type<int> R, 
                       const WRATHFormattedTextStream &pdata,
                       WRATHTextureFont *font, 
                       int texture_page);

  /*!\fn unsigned int number_of_characters(iterator, iterator, 
                                           const WRATHFormattedTextStream&,
                                           WRATHTextureFont*, int)
    Returns the number of characters within
    a set of ranges of  a WRATHFormattedTextStream
    that use a specified font and texture page.
    \param begin iterator to first range to examine
    \param end iterator one past the last range to examine
    \param pdata formatted character stream
    \param font to look for
    \param texture_page texture page of font to look for.    
   */
  template<typename iterator>
  static
  unsigned int
  number_of_characters(iterator begin, iterator end, 
                       const WRATHFormattedTextStream &pdata,
                       WRATHTextureFont *font, 
                       int texture_page)
  {
    unsigned int return_value(0);
    for(;begin!=end;++begin)
      {
        return_value+=number_of_characters(*begin, pdata, font, texture_page);
      }
    return return_value;
  }

  /*!\fn unsigned int number_of_characters(const_c_array<range_type<int> >, 
                                           const WRATHFormattedTextStream&,
                                           WRATHTextureFont*, int)
    Provided as a conveniance, equivalent to
    \code
    number_of_characters(R.begin(), R.end(), pdata, font, texture_page);
    \endcode
    \param R array of ranges
    \param pdata formatted character stream
    \param font to look for
    \param texture_page texture page of font to look for.        
   */
  static
  unsigned int
  number_of_characters(const_c_array<range_type<int> > R, 
                       const WRATHFormattedTextStream &pdata,
                       WRATHTextureFont *font, 
                       int texture_page)
  {
    return number_of_characters(R.begin(), R.end(),
                                pdata, font, texture_page);
  }

  /*!\fn int highest_texture_page(range_type<int>, const WRATHFormattedTextStream&,
                                 WRATHTextureFont*)
    Returns the highest texture page used within
    a specified range of a WRATHFormattedTextStream
    that use a specified font. If no glyph is within
    the specified range, returns -1.
    \param R range to examine within pdata
    \param pdata formatted character stream
    \param font to look for
   */
  static
  int
  highest_texture_page(range_type<int> R, 
                       const WRATHFormattedTextStream &pdata,
                       WRATHTextureFont *font);

  /*!\fn int highest_texture_page(iterator, iterator, const WRATHFormattedTextStream&,
                                  WRATHTextureFont*)
    Returns the highest texture page used within
    a set of ranges of a WRATHFormattedTextStream
    that use a specified font. If no glyph is within
    the specified ranges, returns -1.
    \param begin iterator to first range to examine
    \param end iterator one past the last range to examine    
    \param pdata formatted character stream
    \param font to look for
   */  
  template<typename iterator>
  static
  int
  highest_texture_page(iterator begin, iterator end, 
                       const WRATHFormattedTextStream &pdata,
                       WRATHTextureFont *font)
  {
    int return_value(-1);
    for(;begin!=end;++begin)
      {
        return_value=std::max(return_value, 
                              highest_texture_page(*begin, pdata, font));
      }
    return return_value;
  }

  /*!\fn int highest_texture_page(const_c_array<range_type<int> >,
                                  const WRATHFormattedTextStream&,
                                  WRATHTextureFont*)
    Provided as a conveniance, equivalent to
    \code
    highest_texture_page(R.begin(), R.end(), pdata, font);
    \endcode
    \param R array of ranges
    \param pdata formatted character stream
    \param font to look for      
   */
  static
  int
  highest_texture_page(const_c_array<range_type<int> > R, 
                       const WRATHFormattedTextStream &pdata,
                       WRATHTextureFont *font)
  {
    return highest_texture_page(R.begin(), R.end(), pdata, font);
  }

protected:
   /*!\fn set_attribute_data_implement
    To be implemented by a derived class to
    pack attribute data. Only those glyphs
    within the specified range using
    the named texture page and font are to
    be observed. The caller will have
    allocated attribute and index data
    for those glyphs within the specfied
    range using the specified font and texture
    page. The assumption is that the total number
    of attributes allocated is alteast that which
    is returned by allocation_requirement() and
    the total number of indices needed is also.

    \param R array of ranges of glyphs of pdata to set attribute data
    \param font font to use, only those glyphs of that font
                and the specified texture page are to be packed.
    \param texture_page texture page to use, only those glyphs 
                        of that texture page and the specified
                        font are to be packed.
    \param attribute_store reference to sink to which to write attribute data
    \param attr_location location within the WRATHAttributeStore::handle
                         of the attribute data to write to
    \param index_group reference to sink to which to write index data 
    \param pdata formatted text stream to get the characters from
    \param state_stream state change stream of pdata  
    \param out_bounds_box if non-NULL, or's the packed letters bounding boxes
                          with *out_bounds_box
   */
  virtual
  void
  set_attribute_data_implement(const_c_array<range_type<int> > R,
                               WRATHTextureFont *font,
                               int texture_page,
                               WRATHAbstractDataSink &attribute_store,
                               const std::vector<range_type<int> > &attr_location,
                               WRATHAbstractDataSink &index_group,
                               const WRATHFormattedTextStream &pdata,
                               const WRATHStateStream &state_stream,
                               BBox *out_bounds_box) const=0;   

private:
  ResourceKey m_resource_name;
  mutable WRATHMutex m_mutex;
  mutable std::map<int, const WRATHAttributePacker*> m_packers;
};

namespace WRATHText
{

  /*!\class WRATHText::font_packer
    See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY.
    Determines the WRATHTextAttributePacker used.
     - streams not-initialized with any value 
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(font_packer, const WRATHTextAttributePacker*)

}



/*! @} */

#endif
