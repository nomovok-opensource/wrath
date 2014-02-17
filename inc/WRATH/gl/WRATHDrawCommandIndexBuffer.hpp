/*! 
 * \file WRATHDrawCommandIndexBuffer.hpp
 * \brief file WRATHDrawCommandIndexBuffer.hpp
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




#ifndef WRATH_HEADER_DRAW_COMMAND_INDEX_BUFFER_HPP_
#define WRATH_HEADER_DRAW_COMMAND_INDEX_BUFFER_HPP_

#include "WRATHConfig.hpp"
#include "WRATHNew.hpp"
#include "WRATHBufferObject.hpp"
#include "WRATHDrawCommand.hpp"

/*! \addtogroup Kernel
 * @{
 */

/*!\class WRATHDrawCommandIndexBuffer
  A WRATHDrawCommandIndexBuffer indicates to send
  a range of indices stored in a \ref WRATHBufferObject
  to GL for drawing. 
 */
class WRATHDrawCommandIndexBuffer:public WRATHDrawCommand
{
public:

  /*!\fn WRATHDrawCommandIndexBuffer
    Construct a WRATHDrawCommandIndexBuffer.
    \tparam T type to determine index type.
    \param tr handle to WRATHTripleBufferEnabler to which to sync
    \param idx_buffer WRATHBufferObject to source from
    \param pprimtive_type GL primitive type to draw
    \param prange Location of indices into idx_buffer in _bytes_
   */
  template<typename T>
  WRATHDrawCommandIndexBuffer(const WRATHTripleBufferEnabler::handle &tr,
                              WRATHBufferObject *idx_buffer,
                              index_range prange,
                              GLenum pprimtive_type=GL_TRIANGLES,
                              type_tag<T>):
    WRATHDrawCommand(tr, idx_buffer),
    m_primitive_type(pprimtive_type),
    m_index_type(opengl_trait<T>::type),
    m_range(prange)    
  {}

  
  virtual
  GLenum
  index_type(void)
  {
    return m_index_type;
  }

  virtual
  GLenum
  primitive_type(void)
  {
    return m_primitive_type;
  }

  virtual
  void
  append_draw_elements(std::vector<index_range> &output)
  {
    output.push_back(m_range);
  }

  /*!\var m_primitive_type
    primitive type to feed to GL
  */
  GLenum m_primitive_type;
  
  /*!\var m_index_type
    index type to feed to GL
  */
  GLenum m_index_type;
  
  /*!\var m_range
    Specifies range into the buffer object
    holding the indices to send to glDrawElements.
  */
  index_range m_range;
};
/*! @} */




#endif
