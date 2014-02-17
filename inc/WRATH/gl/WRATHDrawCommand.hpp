/*! 
 * \file WRATHDrawCommand.hpp
 * \brief file WRATHDrawCommand.hpp
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




#ifndef WRATH_HEADER_DRAW_COMMAND_HPP_
#define WRATH_HEADER_DRAW_COMMAND_HPP_

#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include "WRATHNew.hpp"
#include "WRATHBufferObject.hpp"

/*! \addtogroup Kernel
 * @{
 */

/*!\class WRATHDrawCommand
  A WRATHDrawCommand represents drawing a range
  of indices which are stored in a \ref WRATHBufferObject.
  WRATHDrawCommand provides an _interface_ for
  specifying what to append.
 */
class WRATHDrawCommand:
  public WRATHTripleBufferEnabler::PhasedDeletedObject
{
public:

  /*!\class index_range
    Conveniance class to specify a range
    of indices.
   */
  class index_range
  {
  public:
    /*!\fn index_range(void)
      Ctor. Initialize both \ref
      m_location and \ref m_count
      as 0.
     */
    index_range(void):
      m_location(0),
      m_count(0)
    {}

    /*!\fn index_range(int, int)
      Ctor.
      \param loc value to which to initialize \ref m_location
      \param cnt value to which to initialize \ref m_count
     */
    index_range(int loc, int cnt):
      m_location(loc),
      m_count(cnt)
    {}

    /*!\var m_location
      Starting offset into a WRATHBufferObject
      of the 1st index to send to GL. Value
      is in _bytes_.
     */
    int m_location;

    /*!\var m_count
      Number of indices to send to GL. Value
      is in number of indices, not bytes.
     */
    int m_count;
  };
  
  /*!\fn WRATHDrawCommand 
    Ctor.
    \param tr WRATHTripleBufferEnabler to which to sync
    \param bo value to which to set the buffer object
              of the created WRATHDrawCommand, i.e
              value to which to initialize \ref m_buffer_object.
   */
  explicit
  WRATHDrawCommand(const WRATHTripleBufferEnabler::handle &tr,
                   WRATHBufferObject *bo=NULL):
    WRATHTripleBufferEnabler::PhasedDeletedObject(tr),
    m_buffer_object(bo)
  {}

  virtual
  ~WRATHDrawCommand()
  {}

  /*!\fn WRATHBufferObject* buffer_object
    Called by \ref WRATHRawDrawData to determine
    which buffer object to have bound
    to GL_ELEMENT_ARRAY_BUFFER, NULL
    return value indicates to have 0
    bound to GL_ELEMENT_ARRAY_BUFFER.
   */
  WRATHBufferObject*
  buffer_object(void)
  {
    return m_buffer_object;
  }

  /*!\fn GLenum index_type
    To be implemented by a derived class
    to return the index type, i.e. one
    of GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT
    or GL_UNSIGNED_INT.
   */
  virtual
  GLenum
  index_type(void)=0;

  /*!\fn GLenum primitive_type
    To be implemented by a derived class to
    return the primitive type passed
    to a glDraw* command (for example,
    GL_TRIANGLES, GL_LINES, etc).
   */
  virtual
  GLenum
  primitive_type(void)=0;

  /*!\fn void append_draw_elements
    To be implemented by a dervied class 
    to append the ranges into
    the WRATHBufferObject returned
    by buffer_object() to draw.
   */
  virtual
  void
  append_draw_elements(std::vector<index_range> &output)=0;

  /*!\fn bool draw_elements_empty
    May be reimplemented by a derived class.
    If draw_elements_empty() returns true, 
    then the WRATHRawDrawDataElement using this
    WRATHDrawCommand will be skipped, default
    value is to return false.
   */
  virtual
  bool
  draw_elements_empty(void)
  {
    return false;
  }
  
protected:

  /*!\var m_buffer_object
    The underlying WRATHBufferObject of
    this WRATHDrawCommand, it must be
    set by a derived class in it's ctor
    or set in the ctor of WRATHDrawCommand.

    The object is NOT viewed owned by the
    WRATHDrawCommand.
   */
  WRATHBufferObject* m_buffer_object;
};
/*! @} */

#endif
