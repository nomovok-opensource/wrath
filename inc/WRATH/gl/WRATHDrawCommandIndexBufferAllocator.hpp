/*! 
 * \file WRATHDrawCommandIndexBufferAllocator.hpp
 * \brief file WRATHDrawCommandIndexBufferAllocator.hpp
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




#ifndef WRATH_HEADER_DRAW_COMMAND_INDEX_BUFFER_ALLOCATOR_HPP_
#define WRATH_HEADER_DRAW_COMMAND_INDEX_BUFFER_ALLOCATOR_HPP_

#include "WRATHConfig.hpp"
#include "WRATHNew.hpp"
#include "WRATHBufferAllocator.hpp"
#include "WRATHItemDrawer.hpp"
#include "WRATHRawDrawData.hpp"
#include "WRATHDrawCommand.hpp"


/*! \addtogroup Kernel
 * @{
 */

/*!\class WRATHDrawCommandIndexBufferAllocator
  A WRATHDrawCommandIndexBufferAllocator draws the range of
  indices as stored by a WRATHBufferAlloctor.
 */
class WRATHDrawCommandIndexBufferAllocator:public WRATHDrawCommand
{
public:

  /*!\class params
    Parameters class to initialize a \ref WRATHDrawCommandIndexBufferAllocator
   */
  class params
  {
  public:
    /*!\fn params
      Default ctor initializing this params
      as a set of invalid values to trigger
      WRATHassert and/or GL errors if it is used
      in a glDrawElements call.
     */
    params(void):
      m_index_buffer(NULL),
      m_primitive_type(GL_INVALID_ENUM),
      m_index_type(GL_INVALID_ENUM),
      m_index_type_size(-1)
    {}

    /*!\fn params(WRATHBufferAllocator*, GLenum, type_tag<T>)
      Construct a params. The typename T
      is used to define both index type
      as opengl_trait<T>::type and
      the index size as sizeof(T).
      \param idx_buffer WRATHBufferObject to source from
      \param pprimtive_type GL primitive type to draw
    */
    template<typename T>
    params(WRATHBufferAllocator *idx_buffer,
           GLenum pprimtive_type,
           type_tag<T>):
      m_index_buffer(idx_buffer),
      m_primitive_type(pprimtive_type),
      m_index_type(opengl_trait<T>::type),
      m_index_type_size(sizeof(T))
    {}

    /*!\fn params(WRATHBufferAllocator*, GLenum, GLenum, GLsizei)
      Ctor.
      \param idx_buffer WRATHBufferObject to source from
      \param pprimtive_type GL primitive type to draw
      \param pindex_type GL enumeration for index type
      \param pindex_type_size size in bytes of index type
     */
    params(WRATHBufferAllocator *idx_buffer,
           GLenum pprimtive_type,
           GLenum pindex_type,
           GLsizei pindex_type_size):
      m_index_buffer(idx_buffer),
      m_primitive_type(pprimtive_type),
      m_index_type(pindex_type),
      m_index_type_size(pindex_type_size)
    {}

    /*!\fn params& index_type(void) const
      Returns the GL enumeration for the index type.
     */
    GLenum
    index_type(void) const
    {
      return m_index_type;
    }

    /*!\fn params& index_type_size
      Returns the size in bytes of the index type.
     */
    GLsizei
    index_type_size(void) const
    {
      return m_index_type_size;
    }

    /*!\fn params& index_type(type_tag<T>)
      Set the index type with a type_tag.
     */
    template<typename T>
    params&
    index_type(type_tag<T>)
    {
      m_index_type=opengl_trait<T>::type;
      m_index_type_size=sizeof(T);
      return *this;
    }

    /*!\fn params& index_type(GLenum, GLsizei)
      Set the index type directly by 
      providing the GL enumeration of
      the index type and the size in
      bytes of the index type.
      \param tp GL enumeration of the index type
      \param sz size in bytes of the index type.
     */
    params&
    index_type(GLenum tp, GLsizei sz)
    {
      m_index_type=tp;
      m_index_type_size=sz;
      return *this;
    }

    /*!\var m_index_buffer
      WRATHBufferAllocator which holds the index data
     */
    WRATHBufferAllocator *m_index_buffer;

    /*!\var m_primitive_type
      Primitive type fed to glDrawElements
     */
    GLenum m_primitive_type;

  private:
    GLenum m_index_type;
    GLsizei m_index_type_size;
  };

  /*!\fn WRATHDrawCommandIndexBufferAllocator
    Ctor.
    \param tr handle to WRATHTripleBufferEnabler to which to sync
    \param pparams parameters used to create the 
                   WRATHDrawCommandIndexBufferAllocator
   */
  explicit
  WRATHDrawCommandIndexBufferAllocator(const WRATHTripleBufferEnabler::handle &tr,
                                       const params &pparams):
    WRATHDrawCommand(tr),
    m_params(pparams)
  {
    WRATHassert(m_params.m_index_buffer!=NULL);
    m_buffer_object=m_params.m_index_buffer->buffer_object();
  }

  virtual
  GLenum
  index_type(void)
  {
    return m_params.index_type();
  }

  virtual
  GLenum
  primitive_type(void)
  {
    return m_params.m_primitive_type;
  }

  virtual
  void
  append_draw_elements(std::vector<index_range> &output);
  
  virtual
  bool
  draw_elements_empty(void)
  {
    range_type<int> R(m_params.m_index_buffer->allocated_range());
    return (R.m_end<=R.m_begin);
  }

  /*!\fn const params& parameters
    Returns the parameters of this 
    WRATHDrawCommandIndexBufferAllocator
   */
  const params&
  parameters(void) const
  {
    return m_params;
  }

private:
  params m_params;
};
/*! @} */


#endif
