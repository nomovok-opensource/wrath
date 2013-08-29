/*! 
 * \file WRATHAtlasBase.hpp
 * \brief file WRATHAtlasBase.hpp
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


#ifndef __WRATH_ATLAS_BASE_HPP__
#define __WRATH_ATLAS_BASE_HPP__

#include "WRATHConfig.hpp"
#include <stdint.h>
#include <boost/utility.hpp>
#include "WRATHgl.hpp"
#include "c_array.hpp"
#include "vecN.hpp"
#include "vectorGL.hpp"
#include "WRATHNew.hpp"
#include "WRATHReferenceCountedObject.hpp"



/*! \addtogroup Utility
 * @{
 */

/*!\class WRATHPixelStore
  A WRATHPixelStore represents where image
  data is stored. The class is an empty
  class with no methods, but the expectation
  is that derived classes will provide an API 
  to upload image data.
 */
class WRATHPixelStore:public boost::noncopyable
{
public:
  WRATHPixelStore(void)
  {}

  virtual
  ~WRATHPixelStore()
  {}

private:
};


/*!\class WRATHAtlasBase
  A WRATHAtlasBase provides an interface for
  allocating rectangles within a rectange.
  Allocated rectangles size and position are static.
 */
class WRATHAtlasBase:
  public WRATHReferenceCountedObjectT<WRATHAtlasBase>
{
public:
  /*!\class rectangle_handle
    An rectangle_handle gives the location (i.e size and
    position) of a rectangle within a WRATHAtlasBase. 
    The location of a rectangle does not change for the 
    lifetime of the rectangle after returned by
    add_rectangle().
   */
  class rectangle_handle:public boost::noncopyable
  {
  public:
    /*!\fn const ivec2& minX_minY
      Returns the minX_minY of the rectangle.
     */
    const ivec2&
    minX_minY(void) const
    {
      return m_minX_minY;
    }

    /*!\fn const ivec2& size
      Returns the size of the rectangle.
     */
    const ivec2&
    size(void) const
    {
      return m_size;
    }

    /*!\fn const handle& atlas
      Returns the owning WRATHAtlasBase of this
      rectangle_handle.
     */
    const handle&
    atlas(void) const
    {
      return m_atlas;
    }

    /*!\fn rectangle_handle
      Ctor used by implementation of WRATHAtlasBase
      to create rectangle_handle objects.
     */
    rectangle_handle(const handle &p, const ivec2 &psize):
      m_atlas(p), 
      m_minX_minY(0, 0),
      m_size(psize)
    {}

    virtual
    ~rectangle_handle()
    {}

  private:
    friend class WRATHAtlasBase;

    handle m_atlas;
    ivec2 m_minX_minY, m_size;
  };

  /*!\fn WRATHAtlasBase
    Constructs a WRATHAtlasBase, passing a 
    pixel store. The created WRATHAtlasBase 
    OWNS the passed WRATHPixelStore and will delete it
    when the WRATHAtlasBase goes out of scope.
    \param ppixelstore pixel store for the WRATHAtlasBase 
   */
  explicit
  WRATHAtlasBase(WRATHPixelStore *ppixelstore):
    m_pixelstore(ppixelstore)
  {}

  virtual
  ~WRATHAtlasBase();
  
  /*!\fn WRATHPixelStore* pixelstore
    Returns the WRATHPixelStore of the WRATHAtlas,
    do NOT delete the returned pointer as the WRATHAtlasBase
    will do so when it goes out of scope.
   */
  WRATHPixelStore*
  pixelstore(void) const
  {
    return m_pixelstore;
  }

  /*!\fn const rectangle_handle* add_rectangle
    To be implemented by a derived class to
    allocate a rectangle, returning a pointer 
    to the rectangle. Returns NULL on failure. 
    The rectangle is owned by this WRATHAtlasBase.
    An implementation may not change the location
    (or size) of a rectangle once it has been
    returned by add_rectangle() or add_rectangles().
    \param dimension width and height of the rectangle
   */
  virtual
  const rectangle_handle*
  add_rectangle(const ivec2 &dimension)=0;

  /*!\fn enum return_code add_rectangles
    To be optinally reimplemented by a derived class to
    allocate a group of rectangles, the rectangles
    are owned by the allocating WRATHAtlasBase
    object. Returns routine_fail if unable to allocate
    _ALL_ the rectangles requested and will not allocate
    unless it can allocate all. Default implementation is
    to allocate a rectangle for each element in dimensions.
    If an allocation fails, then rectangles made are deleted.
    An implementation may not change the location
    (or size) of a rectangle once it has been
    returned by add_rectangle() or add_rectangles().
    \param dimensions array of dimensions to allocate
    \param out_rects contonainer to which to allocate,
                     on success the created rectangles
                     are _appended_ to out_rects, in 
                     the order of allocation requested
                     in the argument dimensions.
   */
  virtual
  enum return_code
  add_rectangles(const_c_array<ivec2> &dimensions,
                 std::list<const rectangle_handle*> &out_rects);

  /*!\fn void clear
    To be implemented by a dervied class to clear the
    WRATHAtlasBase. After clear(), all 
    rectangle_handle objects returned by add_rectangle()
    and add_rectangles() are deleted, and as such the
    pointers are then wild-invalid.
   */
  virtual
  void
  clear(void)=0;

  /*!\fn enum return_code delete_rectangle
    Delete a rectangle, and in doing so remove it
    from the owning WRATHAtlasBase, and thus allowing
    subsequent rectangles added to use the room of the
    removed rectangle. Removing a rectangle deallocates
    it's backing data structure.
    \param im pointer to a rectangle_handle, 
              as returned by add_rectangle, to remove
   */
  static
  enum return_code
  delete_rectangle(const rectangle_handle *im);

protected:
  
  /*!\fn enum return_code remove_rectangle_implement
    To be implemented by a derivied class
    to do the work of delete_rectangle().
    \param im rectangle_handle rectangle to remove.
   */
  virtual
  enum return_code
  remove_rectangle_implement(const rectangle_handle *im)=0;

  /*!\fn void move_rectangle
    Move a rectangle by an amount, i.e
    change m_minX_minY.
    \param rect rectangle to move.
    \param moveby amount by which to increment rect->m_minX_minY
   */
  static
  void
  move_rectangle(rectangle_handle *rect, const ivec2 &moveby)
  {
    WRATHassert(rect!=NULL);
    rect->m_minX_minY+=moveby;
  }

  /*!\fn void set_minX_minY
    Set the bottom left corner of a rectangle.
    \param rect rectangle to move.
    \param bl new value for rect's m_minX_minY
   */
  static
  void
  set_minX_minY(rectangle_handle *rect, const ivec2 &bl)
  {
    WRATHassert(rect!=NULL);
    rect->m_minX_minY=bl;
  }

private:  
  WRATHPixelStore *m_pixelstore;
};
/*! @} */


#endif
