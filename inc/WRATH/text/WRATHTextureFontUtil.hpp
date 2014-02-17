/*! 
 * \file WRATHTextureFontUtil.hpp
 * \brief file WRATHTextureFontUtil.hpp
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




#ifndef WRATH_HEADER_TEXTURE_FONT_UTIL_HPP_
#define WRATH_HEADER_TEXTURE_FONT_UTIL_HPP_

#include "WRATHConfig.hpp"
#include <boost/signals2.hpp>
#include <boost/bind.hpp>
#include <vector>
#include <map>
#include <boost/multi_array.hpp>
#include "vectorGL.hpp"
#include "type_tag.hpp"
#include "WRATHTextureChoice.hpp"
#include "WRATHImage.hpp"

/*! \addtogroup Text
 * @{
 */

/*!\namespace WRATHTextureFontUtil
  Namespace containing common utility classes 
  for texture fonts.
 */
namespace WRATHTextureFontUtil
{
  /*!\fn GLint effective_texture_creation_size(int, bool)
    Given a "wished" for texture dimension R and
    a flag weather or not to allow only power of 
    2 textures, return a dimension value that
    is acceptable for creating textures that is
    no more than R and if required, also a power 
    of 2.
   */
  GLint
  effective_texture_creation_size(int R, bool force_pow2);

  /*!\class TexturePageTracker
    Many font types may realize each glyph
    as a WRATHImage, these font types then
    need to track the texture binder(s) 
    (\ref WRATHImage::texture_binder()) 
    and the atlas size. A TexturePageTracker
    will build an std::map<> keyed by
    WRATHTextureChoice::texture_base::handle
    with values as integers. It also implements
    on behalf of a WRATHTextureFont derived
    class the methods:
    - \ref WRATHTextureFont::texture_binder()
    - \ref WRATHTextureFont::number_texture_pages()
    - \ref WRATHTextureFont::texture_page_data_size() via size of \ref custom_data()
    - \ref WRATHTextureFont::texture_page_data() via elements of \ref custom_data()
   */
  class TexturePageTracker:boost::noncopyable
  {
  public:

    /*!\typedef signal_t
      Signal type for signal fired whenever a new
      page is created; signal is fired while
      the access mutex is locked, passing the
      argments: texture page, texture_size(),
      texture_binders() and custom_data()
    */
    typedef boost::signals2::signal<void (int, ivec2, 
                                          const std::vector<WRATHTextureChoice::texture_base::handle>&,
                                          std::vector<float>&)> signal_t;

    /*!\typedef connect_t
      Conveniance typedef for the connection type.
    */
    typedef boost::signals2::connection connect_t;

    ~TexturePageTracker();

    /*!\fn connect_t connect
      Connect to the signal fired whenever a new texture
      page is created.
      \param subscriber functor to call
      \param gp_order Calling order of slots 
                      can be specified by setting gp_order,
                      slots connected with a lower gp_order 
                      are guaranteed to be called before slots 
                      connected with a higher (for a fixed 
                      signal_type-signal_time pair). Slots
                      in the same gp_order have no guarantee
                      of which is called first.
     */
    connect_t
    connect(const signal_t::slot_type &subscriber, int gp_order=0)
    {
      return m_signal.connect(gp_order, subscriber);
    }

    /*!\fn int get_page_number(ivec2, const_c_array<WRATHTextureChoice::texture_base::handle>)
      Returns the page number for a specified (array of texture handlers)-key.
      If the key does not yet exist in the system, it is added with that the
      value of texture_size specifies the \ref texture_size() for the 
      returned page.
      \param texture_size the value to use for texture size
                          if the key R does not already exist
      \param R array of handles to WRATHTextureChoice::texture_base objects
     */
    int
    get_page_number(ivec2 texture_size,
                    const_c_array<WRATHTextureChoice::texture_base::handle> R);

    /*!\fn int get_page_number(WRATHImage*, const_c_array<WRATHImage*>)
      Conveniance function, equivalent to
      \code
      get_page_number(mainImage->atlas_size(), R)
      \endcode
      where the array R is the contents of mainImage->texture_binders()
      concacted with the texture_binders() of the elements of the 
      array additional_images.
      \param mainImage WRATHImage from which to extract the atlast size
      \param additional_images array of additional image with which to 
                               concact WRATHImage::texture_binders() or
                               mainImage
     */
    int
    get_page_number(WRATHImage *mainImage,
                    const_c_array<WRATHImage*> additional_images);

    /*!\fn int get_page_number(WRATHImage*)
      Provided as a (much used) conveniance, equivalent to:
      \code
      get_page_number(pImage->atlas_size(), pImage->texture_binders());
      \endcode
      \param pImage pointer to WRATHImage from which to get the page number.
     */
    int
    get_page_number(WRATHImage *pImage);
    
    /*!\fn const_c_array<WRATHTextureChoice::texture_base::handle> texture_binder
      Returns the texture binders for a given page number
      as returned by get_page_number().
      \param pg page number from which to fetch the binders
     */
    const_c_array<WRATHTextureChoice::texture_base::handle>
    texture_binder(int pg) const;

    /*!\fn const ivec2& texture_size
      Returns the main texture size (essentially the atlas size) for
      a given page number as returned by get_page_number(WRATHImage*) and/or
      get_page_number(ivec2, const_c_array<WRATHTextureChoice::texture_base::handle>).
      \param pg page number from which to fetch the texture size      
     */
    const ivec2&
    texture_size(int pg) const;

    /*!\fn const std::vector<float>& custom_data(int) const
      Returns a const-reference to the custom data
      associated to a texture page.
      \param pg page number from which to fetch the custom data
     */
    const std::vector<float>&
    custom_data(int pg) const;

    /*!\fn std::vector<vec4>& custom_data(int) 
      Returns a reference to the custom data
      associated to a texture page.
      \param pg page number from which to fetch the custom data
     */
    std::vector<float>&
    custom_data(int pg);

    /*!\fn int number_texture_pages
      Returns the number of texture pages.
     */
    int
    number_texture_pages(void) const;

  private:
    typedef std::vector<WRATHTextureChoice::texture_base::handle> binder_array;
    typedef std::map<binder_array, int> map_type;

    class page_type:public std::pair<ivec2, binder_array>
    {
    public:
      page_type(const ivec2 &v, binder_array &r)
      {
        first=v;
        std::swap(second, r);
      }

      const ivec2&
      texture_size(void) const
      {
        return first;
      }

      const binder_array&
      binders(void) const
      {
        return second;
      }

      std::vector<float> m_custom_data;
    };

    int
    get_page_number_implement(ivec2 ptexture_size, binder_array &raw_key);
    
    mutable WRATHMutex m_mutex;
    map_type m_map;
    std::vector<page_type*> m_pages;
    signal_t m_signal;
  };

  /*!\class SubQuadProducer
    A SubQuadProducer's purpose it to create
    a list of sub-quads which cover those
    texels that are non-empty but do cover
    a smaller area than the original quad.
   */
  class SubQuadProducer:boost::noncopyable
  {
  public:
    
    /*!\fn SubQuadProducer
      Ctor.
      \param pfull_quad_resolution resolution of quad
      \param pmin_subquad_size minimum size of sub-quads
     */
    SubQuadProducer(const ivec2 &pfull_quad_resolution,
                    int pmin_subquad_size);

    ~SubQuadProducer()
    {}

    /*!\fn void mark_texel(int, int)
      Mark the texel at a point as "having"
      data, i.e. the texel must be covered
      by the sub-quads.
      \param x x-coordinate of texel
      \param y y-coordinate of texel
     */
    void
    mark_texel(int x, int y);

    /*!\fn void mark_texel(const ivec2&)
      Provided as a conveniance, equivalent to:
      \code
      mark_texel(pt.x(), pt.y())
      \endcode
      \param pt coordinate to mark
     */
    void
    mark_texel(const ivec2 &pt)
    {
      mark_texel(pt.x(), pt.y());
    }
    
    /*!\fn const std::vector<ivec2>& primitives_attributes
      Returns the vertex data of the primitives. Each
      value is a corner from a texel.
     */
    const std::vector<ivec2>&
    primitives_attributes(void) const
    {
      flush();
      return m_attributes;
    }

    /*!\fn const std::vector<uint16_t>& primitive_indices
      Returns the indices of the quads as
      triangle indices, i.e. suitable for
      GL_TRIANGLES of glDrawElements.      
     */
    const std::vector<uint16_t>&
    primitive_indices(void) const
    {
      flush();
      return m_indices;
    }

  private:

    void
    flush(void) const;

    ivec2
    fullres_coordinate(const ivec2 &in_lowres_coordinate) const
    {
      ivec2 R;
      
      /*
        the last tile in each dimension will likely
        overrun past m_full_resolution (if for example
        m_min_quad_size does not evenly divide
        m_full_resolution). However, the trick out of
        this is to just saturate to m_full_resolution,
        since those points are the on the right or top 
        edge of a tile
       */
      R=in_lowres_coordinate*m_min_quad_size;
      return ivec2( std::min(m_full_resolution.x(), R.x()),
                    std::min(m_full_resolution.y(), R.y()));
    }

    ivec2
    lowres_coordinate(const ivec2 &in_fullres_coordinate) const
    {
      return in_fullres_coordinate/m_min_quad_size;
    }

    int m_min_quad_size;
    ivec2 m_full_resolution;
    ivec2 m_lowres_resolution;
    boost::multi_array<bool, 2> m_tile_covered;
    std::list<ivec2> m_list_of_covered_tiles;
    
    mutable bool m_ready;
    mutable std::vector<ivec2> m_attributes;
    mutable std::vector<uint16_t> m_indices;
  };
}
/*! @} */


#endif
