/*! 
 * \file WRATHRawDrawData.cpp
 * \brief file WRATHRawDrawData.cpp
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


#include "WRATHConfig.hpp"
#include "WRATHRawDrawData.hpp"

namespace
{
  
  void
  init_attributes(void)
  {
    for(unsigned int i=0;i!=WRATHDrawCallSpec::attribute_count; ++i)
      {
        glDisableVertexAttribArray(i);
      }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  unsigned int
  simulate_MultiDrawElements(GLenum primitive_type, 
                             const std::vector<WRATHDrawCommand::index_range> &draw_ranges,
                             GLenum index_type,
                             WRATHBufferObject *indx_source,
                             std::vector<uint8_t> &temp_bytes)
  {

    /*
      TODO: build an array of indices and use that to draw.
    */
    WRATHunused(temp_bytes);

    if(indx_source->has_buffer_object_on_bind())
      {
        indx_source->bind(GL_ELEMENT_ARRAY_BUFFER);
      }
    else
      {
        WRATHLockMutex(indx_source->mutex());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      }

    for(unsigned int i=0, lasti=draw_ranges.size(); i<lasti; ++i)
      {
        glDrawElements(primitive_type,
                       draw_ranges[i].m_count,
                       index_type,
                       indx_source->offset_pointer(draw_ranges[i].m_location));
      }

    
    if(!indx_source->has_buffer_object_on_bind())
      {
        WRATHUnlockMutex(indx_source->mutex());
      }

    return draw_ranges.size();
  }


  unsigned int
  local_MultiDrawElements(GLenum primitive_type, 
                          const std::vector<WRATHDrawCommand::index_range> &draw_ranges,
                          GLenum index_type,
                          WRATHBufferObject *indx_source,
                          std::vector<uint8_t> &temp_bytes)
  {
    if(indx_source->has_buffer_object_on_bind())
      {
        indx_source->bind(GL_ELEMENT_ARRAY_BUFFER);
      }
    else
      {
        WRATHLockMutex(indx_source->mutex());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      }

    size_t bytes_needed;

    bytes_needed=draw_ranges.size()*sizeof(GLsizei)
      + draw_ranges.size()*sizeof(const GLvoid *);

    temp_bytes.resize(std::max(bytes_needed, temp_bytes.size()));

    c_array<uint8_t> bytes_ptr(temp_bytes);
    c_array<const GLvoid*> indices;
    c_array<GLsizei> counts;
    int start_counts;

    start_counts=draw_ranges.size()*sizeof(const GLvoid*);
    indices=bytes_ptr.sub_array(0, start_counts).reinterpret_pointer<const GLvoid*>();
    counts=bytes_ptr.sub_array(start_counts).reinterpret_pointer<GLsizei>();

    for(unsigned int i=0, lasti=draw_ranges.size(); i<lasti; ++i)
      {
        counts[i]=draw_ranges[i].m_count;
        indices[i]=indx_source->offset_pointer(draw_ranges[i].m_location);
      }

    #ifdef WRATH_GL_VERSION
    {
      glMultiDrawElements(primitive_type, counts.c_ptr(), 
                          index_type, indices.c_ptr(), 
                          draw_ranges.size());
    }
    #else
    {
      glMultiDrawElementsEXT(primitive_type, counts.c_ptr(), 
                             index_type, indices.c_ptr(), 
                             draw_ranges.size());
    }
    #endif

    
    if(!indx_source->has_buffer_object_on_bind())
      {
        WRATHUnlockMutex(indx_source->mutex());
      }
    
    return 1;
  }

  class MultiDrawElementsChooser
  {
  public:
    typedef unsigned int (*function_type)(GLenum primitive_type, 
                                          const std::vector<WRATHDrawCommand::index_range> &draw_ranges,
                                          GLenum index_type,
                                          WRATHBufferObject *indx_source,
                                          std::vector<uint8_t> &temp_bytes);

    MultiDrawElementsChooser(void)
    {
      #ifdef WRATH_GL_VERSION
      {
        m_function=local_MultiDrawElements;
      }
      #else
      {
        if(ngl_functionExists(glMultiDrawElementsEXT))
          {
            m_function=local_MultiDrawElements;
          }
        else
          {
            m_function=simulate_MultiDrawElements;
          }
      }
      #endif
    }

    function_type m_function;
  };

  
}

///////////////////////////////////
// global methods
std::ostream&
operator<<(std::ostream &ostr, const WRATHDrawOrder::print_t &obj)
{
  if(obj.m_h.valid())
    {
      obj.m_h->print_stats(ostr);
    }
  else
    {
      ostr << "NULL";
    }
  return ostr;
}

////////////////////////////////////
//WRATHDrawCallSpec methods
bool
WRATHDrawCallSpec::
valid(void) const
{
  bool return_value(m_program!=NULL and  m_draw_command!=NULL);

  for(unsigned int i=0; i<attribute_count and return_value; ++i)
    {
      return_value=(!m_attribute_format_location[i].valid()
                    or m_data_source[i]!=NULL);
    }
  return return_value;
}


/////////////////////////////////////////////////
// WRATHRawDrawData::DrawState methods
void
WRATHRawDrawData::DrawState::
texture(const WRATHTextureChoice::const_handle &hnd)
{
  WRATHassert(m_active);
  if(m_tex!=hnd)
    {
      if(hnd.valid())
        {
          int ct;

          flush_draws();
          ct=hnd->bind_textures(m_tex);
          m_draw_information_ptr->m_texture_choice_count+=ct;
        }
      else //since m_tex!=hnd and hnd is not valid, m_tex must be valid
        {
          m_tex->unbind_textures();
        }
      m_tex=hnd;
    }
}


void
WRATHRawDrawData::DrawState::
gl_state_change(const WRATHGLStateChange::const_handle &hnd)
{
  WRATHassert(m_active);
  if(m_gl_state_source!=hnd and hnd.valid())
    {
      int ct;

      flush_draws();

      ct=hnd->set_state(m_gl_state_source, m_current_glsl);
      m_gl_state_source=hnd;
      m_draw_information_ptr->m_gl_state_change_count+=ct;
    }
}

void
WRATHRawDrawData::DrawState::
uniform(const WRATHUniformData::const_handle &hnd)
{
  WRATHassert(m_active);
  if(m_uniform!=hnd)
    {
      flush_draws();
      if(hnd.valid())
        {
          make_program_active();
          hnd->execute_gl_commands(m_current_glsl);
        }
      m_uniform=hnd;
    }
}

void
WRATHRawDrawData::DrawState::
selector(WRATHMultiGLProgram::Selector s)
{
  WRATHassert(m_active);
  if(s!=m_selector)
    {
      flush_draws();
      m_selector=s;
      m_uniform=NULL;
      m_current_glsl=NULL;
    }
}

void
WRATHRawDrawData::DrawState::
program(WRATHMultiGLProgram *pr)
{
  /*
    Note that changes of GLSL program resets m_uniform to NULL,
    thus the uniform gets called whenever the GLSL program changes
  */
  WRATHassert(m_active);
  if(pr!=m_prog)
    {
      flush_draws();
      m_prog=pr;
      m_uniform=NULL;
      m_current_glsl=NULL;
    }
}

void
WRATHRawDrawData::DrawState::
make_program_active(void)
{
  WRATHassert(m_active);
  if(m_current_glsl==NULL and m_prog!=NULL)
    {
      m_current_glsl=m_prog->fetch_program(m_selector);
      m_current_glsl->use_program();
      ++m_draw_information_ptr->m_program_count;
    }
}

void
WRATHRawDrawData::DrawState::
set_attribute_sources(const vecN<WRATHBufferObject*, attribute_count> &p_attr_source,
                      const attribute_array_params &p_attr_fmt)
{
  WRATHassert(m_active);
  if(m_init_attributes)
    {
      init_attributes();
      m_init_attributes=false;
    }

  for(int i=0; i<WRATHDrawCallSpec::attribute_count; ++i)
    {
      if(m_attr_source[i]!=p_attr_source[i])
        {
          flush_draws();
          break;
        }
    }

  for(int i=0; i<WRATHDrawCallSpec::attribute_count; ++i)
    {
      if(m_attr_source[i]!=p_attr_source[i])
        {               
          if(m_attr_source[i]!=NULL 
             and !m_attr_source[i]->has_buffer_object_on_bind()
             and m_locked_bos.find(m_attr_source[i])!=m_locked_bos.end())
            {
              m_locked_bos.erase(m_attr_source[i]);
              WRATHUnlockMutex(m_attr_source[i]->mutex());
            }
          
          if(p_attr_source[i]!=NULL 
             and p_attr_fmt[i].valid())
            {
              if(!m_attr_format[i].valid() or m_attr_source[i]==NULL)
                {
                  glEnableVertexAttribArray(i);
                }
              
              m_attr_source[i]=p_attr_source[i];  
              m_attr_format[i]=p_attr_fmt[i];
              
              if(m_currently_bound!=m_attr_source[i])
                {
                  m_attr_source[i]->bind(GL_ARRAY_BUFFER);
                  
                  m_currently_bound=m_attr_source[i];
                  ++m_draw_information_ptr->m_buffer_object_bind_count;
                }
              
              if(!m_attr_source[i]->has_buffer_object_on_bind()  
                 and m_locked_bos.find(m_attr_source[i])==m_locked_bos.end())
                {
                  //locking is only necessary if the attribute values
                  //are not backed by a buffer object
                  WRATHLockMutex(m_attr_source[i]->mutex());
                  m_locked_bos.insert(m_attr_source[i]);
                }
              
              glVertexAttribPointer(i, //index
                                    m_attr_format[i].m_count, 
                                    m_attr_format[i].m_type, 
                                    m_attr_format[i].m_normalized, 
                                    m_attr_format[i].m_stride, 
                                    m_attr_source[i]->offset_pointer(m_attr_format[i].m_offset));
              
              
              ++m_draw_information_ptr->m_attribute_change_count;
            }
          else
            {
              /*
                new data source is NULL or the format indicates
                it is not active attribute.
              */
              if(m_attr_format[i].valid())
                {
                  glDisableVertexAttribArray(i);
                }
              m_attr_format[i]=p_attr_fmt[i];
              m_attr_source[i]=p_attr_source[i];  
            }
        }
      else if(m_attr_format[i]!=p_attr_fmt[i])
        {
          /*
            attribute change, but buffer object is the same.
          */
          if(m_attr_format[i].valid() and !p_attr_fmt[i].valid())
            {
              glDisableVertexAttribArray(i);
              m_attr_format[i]=p_attr_fmt[i];
            }
          else if(p_attr_fmt[i].valid())
            {
              if(!m_attr_format[i].valid())
                {
                  glEnableVertexAttribArray(i);
                }
              
              if(m_attr_source[i]->has_buffer_object_on_bind()
                 and m_currently_bound!=m_attr_source[i])
                {
                  glBindBuffer(GL_ARRAY_BUFFER, m_attr_source[i]->name());
                  m_currently_bound=m_attr_source[i];
                  
                  ++m_draw_information_ptr->m_buffer_object_bind_count;
                }
              
              m_attr_format[i]=p_attr_fmt[i];
              glVertexAttribPointer(i, //index
                                    m_attr_format[i].m_count, 
                                    m_attr_format[i].m_type, 
                                    m_attr_format[i].m_normalized, 
                                    m_attr_format[i].m_stride, 
                                    m_attr_source[i]->offset_pointer(m_attr_format[i].m_offset));
              
            }
        }
    }
}


void
WRATHRawDrawData::DrawState::
index_buffer(WRATHDrawCommand *draw_command)
{
  WRATHBufferObject *indx_source;
  GLenum primitive_type, index_type;

  primitive_type=draw_command->primitive_type();
  index_type=draw_command->index_type();  
  indx_source=draw_command->buffer_object();

  if(primitive_type!=m_primitive_type
     or index_type!=m_index_type
     or indx_source!=m_indx_source)
    {
      flush_draws();
    }

  m_primitive_type=primitive_type;
  m_index_type=index_type;
  m_indx_source=indx_source;
}



void
WRATHRawDrawData::DrawState::
flush_draws(void)
{
  WRATHassert(m_active);
  make_program_active();
  if(m_current_glsl!=NULL and m_indx_source!=NULL and !m_draw_ranges.empty())
    {
      static MultiDrawElementsChooser draw_elements;
      unsigned int cnt;
      
      ++m_draw_information_ptr->m_buffer_object_bind_count; //call always forces a bind
      cnt=draw_elements.m_function(m_primitive_type, 
                                   m_draw_ranges, 
                                   m_index_type, m_indx_source,
                                   m_temp_bytes);

      m_draw_information_ptr->m_draw_count+=cnt;

    }
  m_draw_ranges.clear();
}


void
WRATHRawDrawData::DrawState::
queue_drawing(WRATHDrawCommand *draw_command)
{
  WRATHassert(m_active);
  index_buffer(draw_command);
  draw_command->append_draw_elements(m_draw_ranges);
}


bool
WRATHRawDrawData::DrawState::
valid_program_active(void)
{
  WRATHassert(m_active);
  make_program_active();
  return m_current_glsl!=NULL and m_current_glsl->link_success();
}


void
WRATHRawDrawData::DrawState::
draw_begin(draw_information &out_stats, 
           WRATHMultiGLProgram::Selector pselector)
{  
  WRATHassert(!m_active);
  *this=DrawState(pselector, &out_stats);
  m_active=true;
}

void
WRATHRawDrawData::DrawState::
draw_end(void)
{
  WRATHassert(m_active);

  flush_draws();
  for(std::set<WRATHBufferObject*>::iterator iter=m_locked_bos.begin(),
        end=m_locked_bos.end(); iter!=end; ++iter)
    {
      WRATHBufferObject *ptr(*iter);
      WRATHUnlockMutex(ptr->mutex());
    }

  if(m_tex.valid())
    {
      m_tex->unbind_textures();
    }

  m_active=false;
}


/////////////////////////////////////
//WRATHRawDrawData::sorter  methods
bool
WRATHRawDrawData::sorter::
operator()(const WRATHRawDrawDataElement *plhs,
           const WRATHRawDrawDataElement *prhs) 
{
  /*
    make NULL come last, thus NULL<obj is always false
   */
  if(plhs==NULL or plhs==prhs)
    {
      return false;
    }

  /*
    and obj<NULL is always true.
   */
  if(prhs==NULL)
    {
      return true;
    }

  WRATHassert(plhs!=NULL);
  WRATHassert(prhs!=NULL);

  const WRATHDrawCallSpec &lhs(plhs->draw_spec());
  const WRATHDrawCallSpec &rhs(prhs->draw_spec());

  if(lhs.m_force_draw_order!=rhs.m_force_draw_order
     and m_comparer.valid())
    {
      enum WRATHDrawOrderComparer::draw_sort_order_type v;

      v=m_comparer->compare_objects(lhs.m_force_draw_order, 
                                    rhs.m_force_draw_order);

      if(v!=WRATHDrawOrderComparer::equal_draw_sort_order)
        {
          return v==WRATHDrawOrderComparer::less_draw_sort_order;
        }
    }

  if(lhs.m_program!=rhs.m_program)
    {
      return lhs.m_program<rhs.m_program;
    }

  if(lhs.m_bind_textures!=rhs.m_bind_textures)
    {
      return lhs.m_bind_textures<rhs.m_bind_textures;
    }

  if(lhs.m_gl_state_change!=rhs.m_gl_state_change)
    {
      return lhs.m_gl_state_change<rhs.m_gl_state_change;
    }

  if(lhs.m_data_source!=rhs.m_data_source)
    {
      return lhs.m_data_source<rhs.m_data_source;
    }

  if(lhs.m_attribute_format_location!=rhs.m_attribute_format_location)
    {
      return lhs.m_attribute_format_location<rhs.m_attribute_format_location;
    }

  if(lhs.m_uniform_data!=rhs.m_uniform_data)
    {
      return lhs.m_uniform_data<rhs.m_uniform_data;
    }

  if(lhs.m_draw_command!=rhs.m_draw_command)
    {
      if(lhs.m_draw_command!=NULL
         and rhs.m_draw_command!=NULL
         and lhs.m_draw_command->buffer_object()!=rhs.m_draw_command->buffer_object())
        {
          return lhs.m_draw_command->buffer_object()<rhs.m_draw_command->buffer_object();
        }
      else
        {
          return lhs.m_draw_command<rhs.m_draw_command;
        }
    }

  return false;
}


/////////////////////////////////
// WRATHRawDrawData methods
WRATHRawDrawData::
WRATHRawDrawData(const WRATHTripleBufferEnabler::handle &ptriple_buffer_enabler,
                 const WRATHDrawOrderComparer::const_handle &h):
  WRATHTripleBufferEnabler::PhasedDeletedObject(ptriple_buffer_enabler),
  m_sorter(h),
  m_list_dirty(false)
{
  
  m_connections[0]=connect(WRATHTripleBufferEnabler::on_complete_simulation_frame,
                           WRATHTripleBufferEnabler::pre_update_no_lock,
                           boost::bind(&WRATHRawDrawData::check_sort_elements, this));
  
  m_connections[1]=connect(WRATHTripleBufferEnabler::on_complete_simulation_frame,
                           WRATHTripleBufferEnabler::post_update_no_lock,
                           boost::bind(&WRATHRawDrawData::post_copy_elements, this));
}

WRATHRawDrawData::
~WRATHRawDrawData()
{
}

void
WRATHRawDrawData::
phase_simulation_deletion(void)
{
  int w(current_simulation_ID());
  for(std::vector<WRATHRawDrawDataElement*>::iterator 
        iter=m_buffers[w].begin(), 
        end=m_buffers[w].end(); 
      iter!=end; ++iter)
    {
      WRATHRawDrawDataElement *obj(*iter);
      if(obj!=NULL)
        {
          obj->m_location_in_raw_draw_data=-1;
          obj->m_raw_draw_data=NULL;
          obj->m_draw_order_dirty.disconnect();
        }
    }
}

void
WRATHRawDrawData::
on_place_on_deletion_list(void)
{
  m_connections[0].disconnect();
  m_connections[1].disconnect();
}


void
WRATHRawDrawData::
draw(DrawState &draw_state)
{
  WRATHassert(draw_state.draw_active());

  for(std::vector<WRATHRawDrawDataElement*>::iterator 
        iter=m_buffers[present_ID()].begin(), 
        end=m_buffers[present_ID()].end();
      iter!=end; ++iter)
    {
      WRATHassert(NULL!=*iter);

      const WRATHRawDrawDataElement &current_element(**iter);
      const WRATHDrawCallSpec &current(current_element.draw_spec());

      if(!current.valid()
         or current.m_draw_command->draw_elements_empty())
        {
          continue;
        }
      
      draw_state.program(current.m_program);
      if(!draw_state.valid_program_active())
        {
          continue;
        }
      
      draw_state.texture(current.m_bind_textures);
      draw_state.gl_state_change(current.m_gl_state_change);
      draw_state.uniform(current.m_uniform_data);
      draw_state.set_attribute_sources(current.m_data_source,
                                       current.m_attribute_format_location);
      draw_state.queue_drawing(current.m_draw_command);
    }
}



void
WRATHRawDrawData::
draw(draw_information &out_stats, WRATHMultiGLProgram::Selector selector)
{
  DrawState draw_state(selector, &out_stats);

  draw_state.draw_begin();
  draw(draw_state);
  draw_state.draw_end();
}

const WRATHDrawOrderComparer::const_handle&
WRATHRawDrawData::
draw_order_sorter(void) const
{
  return m_sorter.m_comparer;
}

void
WRATHRawDrawData::
draw_order_sorter(const WRATHDrawOrderComparer::const_handle &v)
{
  if(v!=m_sorter.m_comparer)
    {
      mark_list_dirty();
      m_sorter.m_comparer=v;    
    }
}

void
WRATHRawDrawData::
add_element(WRATHRawDrawDataElement *b)
{
  int w(current_simulation_ID());

  WRATHassert(b->m_location_in_raw_draw_data==-1);
  WRATHassert(b->m_raw_draw_data==NULL);

  b->m_raw_draw_data=this;
  b->m_location_in_raw_draw_data=m_buffers[w].size();
  m_buffers[w].push_back(b);  

  if(b->draw_spec().m_force_draw_order.valid())
    {
      b->m_draw_order_dirty
        =b->draw_spec().m_force_draw_order->m_signal.connect(boost::bind(&WRATHRawDrawData::mark_list_dirty,
                                                                         this));
    }
  mark_list_dirty();
}

void
WRATHRawDrawData::
mark_list_dirty(void)
{
  m_list_dirty=true;  
}

void
WRATHRawDrawData::
remove_element(WRATHRawDrawDataElement *b)
{
  if(b!=NULL and b->m_raw_draw_data!=NULL)
    {
      b->m_raw_draw_data->remove_element_implement(b);
    }
}

void
WRATHRawDrawData::
remove_element_implement(WRATHRawDrawDataElement *b)
{
  int w(current_simulation_ID());

  WRATHassert(b->m_location_in_raw_draw_data>=0);
  WRATHassert(b->m_raw_draw_data==this);
  WRATHassert(b->m_location_in_raw_draw_data<static_cast<int>(m_buffers[w].size()));
  WRATHassert(m_buffers[w][b->m_location_in_raw_draw_data]==b);

  m_buffers[w][b->m_location_in_raw_draw_data]=NULL;
  b->m_location_in_raw_draw_data=-1;
  b->m_raw_draw_data=NULL;
  b->m_draw_order_dirty.disconnect();
  mark_list_dirty();
}


void
WRATHRawDrawData::
check_sort_elements(void)
{
  int w(current_simulation_ID());

  if(m_list_dirty)
    {
      unsigned int actual_size, end;
      std::sort(m_buffers[w].begin(), m_buffers[w].end(), m_sorter);

      for(actual_size=0, end=m_buffers[w].size(); 
          actual_size<end and m_buffers[w][actual_size]!=NULL;
          ++actual_size)
        {
          m_buffers[w][actual_size]->m_location_in_raw_draw_data=actual_size;
        }

      for(unsigned int i=actual_size; i<end; ++i)
        {
          WRATHassert(m_buffers[w][i]==NULL);
        }
      m_buffers[w].resize(actual_size);
      m_list_dirty=false;
    }
}


void
WRATHRawDrawData::
post_copy_elements(void)
{  
  int from(last_simulation_ID());
  int to(current_simulation_ID());

  m_buffers[to]=m_buffers[from];
}


bool
WRATHRawDrawData::
render_empty(void)
{
  return m_buffers[present_ID()].empty();
}
