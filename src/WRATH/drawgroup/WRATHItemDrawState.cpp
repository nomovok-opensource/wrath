/*! 
 * \file WRATHItemDrawState.cpp
 * \brief file WRATHItemDrawState.cpp
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
#include "WRATHItemDrawState.hpp"
#include "WRATHStaticInit.hpp"
#include <boost/signals2.hpp>
#include <boost/bind.hpp>
#include <boost/utility.hpp>

namespace
{
  /*
    We want to produce a collection object from a set
    of elements, but that collection object is reference counted
    as well. If we store a handle to that collection object
    in the Hoard, then the collection object would never get
    deleted. So we:
    - store just a pointer
    - use a derived class that emits a signal to delete the collection object
   */

  typedef boost::signals2::signal<void ()> dtor_signal_t;

  template<typename T>
  class SignalDtor:public T
  {
  public:
    
    ~SignalDtor()
    {
      m_sig();
    }

    dtor_signal_t m_sig;
  };

  template<typename Key, typename T>
  class FetchHoard:boost::noncopyable
  {
  public:
    typedef typename T::const_handle return_type;

    FetchHoard(void)
    {}

    ~FetchHoard(void)
    {
      #ifdef WRATHDEBUG
      {
	if(!m_map.empty())
	  {
	    WRATHwarning("WARNING: FetchHoard, T=" << typeid(T).name()
			 << " non-empty");
	  }
      }
      #endif
    }

    return_type
    fetch(const Key &k)
    {
      iterator iter;

      WRATHAutoLockMutex(m_mutex);

      iter=m_map.find(k);
      if(iter==m_map.end())
	{
	  SignalDtor<T> *new_value;
	  new_value=WRATHNew SignalDtor<T>();
	  new_value->add(k.begin(), k.end());

	  iter=m_map.insert(value_type(k, new_value)).first;
	  new_value->m_sig.connect(boost::bind(&FetchHoard::auto_remove,
					       this,
					       iter));
	}

      return return_type(iter->second);
    }


  private:
    typedef std::map<Key, T*> map_type;
    typedef typename map_type::value_type value_type;
    typedef typename map_type::iterator iterator;

    void
    auto_remove(iterator iter)
    {
      WRATHAutoLockMutex(m_mutex);
      m_map.erase(iter);
    }

    WRATHMutex m_mutex;
    std::map<Key, T*> m_map; 
  };

  class Hoard:boost::noncopyable
  {
  public:
    typedef std::set<WRATHGLStateChange::state_change::handle> gl_state_key;
    typedef std::map<GLenum, WRATHTextureChoice::texture_base::handle> texture_key;
    typedef std::set<WRATHUniformData::uniform_setter_base::handle> uniform_key;

    FetchHoard<uniform_key, WRATHUniformData> m_uniform;
    FetchHoard<texture_key, WRATHTextureChoice> m_texture;
    FetchHoard<gl_state_key, WRATHGLStateChange> m_gl_state_change;

    static
    Hoard&
    get(void) 
    {
      WRATHStaticInit();
      static Hoard R;
      return R;
    }
  private:
    Hoard(void)
    {}
  };
}


////////////////////////////////////////////
// WRATHItemDrawState methods
bool
WRATHItemDrawState::
compare_GL_state_vector(const WRATHItemDrawState &obj) const
{

  if(m_drawer!=obj.m_drawer)
    {
      return m_drawer<obj.m_drawer;
    }

  if(m_buffer_object_hint!=obj.m_buffer_object_hint)
    {
      return m_buffer_object_hint<obj.m_buffer_object_hint;
    }

  if(m_primitive_type!=obj.m_primitive_type)
    {
      return m_primitive_type<obj.m_primitive_type;
    }

  if(m_draw_type!=obj.m_draw_type)
    {
      return m_draw_type<obj.m_draw_type;
    }

  if(m_textures!=obj.m_textures)
    {
      return m_textures<obj.m_textures;
    }

  if(m_gl_state_change!=obj.m_gl_state_change)
    {
      return m_gl_state_change<obj.m_gl_state_change;
    }

  return m_uniforms<obj.m_uniforms;
}

bool
WRATHItemDrawState::
operator<(const WRATHItemDrawState &obj) const
{
  if(m_force_draw_order!=obj.m_force_draw_order)
    {
      return m_force_draw_order<obj.m_force_draw_order;
    }

  return compare_GL_state_vector(obj);
}

//////////////////////////////////////////////
//WRATHCompiledItemDrawState methods
bool
WRATHCompiledItemDrawState::
operator<(const WRATHCompiledItemDrawState &obj) const
{
  
  if(m_drawer!=obj.m_drawer)
    {
      return m_drawer<obj.m_drawer;
    }

  if(m_buffer_object_hint!=obj.m_buffer_object_hint)
    {
      return m_buffer_object_hint<obj.m_buffer_object_hint;
    }

  if(m_primitive_type!=obj.m_primitive_type)
    {
      return m_primitive_type<obj.m_primitive_type;
    }

  if(m_draw_type!=obj.m_draw_type)
    {
      return m_draw_type<obj.m_draw_type;
    }

  if(m_textures!=obj.m_textures)
    {
      return m_textures<obj.m_textures;
    }

  if(m_gl_state_change!=obj.m_gl_state_change)
    {
      return m_gl_state_change<obj.m_gl_state_change;
    }

  return m_uniforms<obj.m_uniforms;
}

WRATHUniformData::const_handle
WRATHCompiledItemDrawState::
fetch_compiled_uniform(const std::set<WRATHUniformData::uniform_setter_base::handle> &p)
{
  return Hoard::get().m_uniform.fetch(p);
}


WRATHGLStateChange::const_handle
WRATHCompiledItemDrawState::
fetch_compiled_state_change(const std::set<WRATHGLStateChange::state_change::handle> &p)
{
  return Hoard::get().m_gl_state_change.fetch(p);
}

WRATHTextureChoice::const_handle
WRATHCompiledItemDrawState::
fetch_compiled_texture(const std::map<GLenum, WRATHTextureChoice::texture_base::handle> &p)
{
  return Hoard::get().m_texture.fetch(p);
}


///////////////////////////////////////////////
//WRATHCompiledItemDrawStateCollection methods
WRATHCompiledItemDrawStateCollection::
WRATHCompiledItemDrawStateCollection(const std::set<WRATHItemDrawState> &p):
  m_draw_states(p.size()),
  m_force_draw_orders(p.size())
{
  std::set<WRATHItemDrawState>::iterator iter, end;
  unsigned int i;

  for(i=0, iter=p.begin(), end=p.end(); iter!=end; ++i, ++iter)
    {
      m_draw_states[i]=*iter;
      m_force_draw_orders[i]=iter->m_force_draw_order;

      WRATHassert(iter->m_buffer_object_hint==p.begin()->m_buffer_object_hint);
      WRATHassert(iter->m_primitive_type==p.begin()->m_primitive_type);
    }

}

WRATHCompiledItemDrawStateCollection::
WRATHCompiledItemDrawStateCollection(const WRATHItemDrawState &p):
  m_draw_states(1, p),
  m_force_draw_orders(1, p.m_force_draw_order)
{}

WRATHCompiledItemDrawStateCollection::
WRATHCompiledItemDrawStateCollection(const_c_array<WRATHCompiledItemDrawState> pdraw_states,
                                     const_c_array<WRATHDrawOrder::const_handle> pforce_draw_orders):
  m_draw_states(pdraw_states.begin(), pdraw_states.end()),
  m_force_draw_orders(pforce_draw_orders.begin(), pforce_draw_orders.end())
{
  m_force_draw_orders.resize(m_draw_states.size(), WRATHDrawOrder::const_handle());
  #ifdef WRATHDEBUG
  {
    for(unsigned int i=1, endi=m_draw_states.size(); i<endi; ++i)
      {
        WRATHassert(m_draw_states[i].m_buffer_object_hint==m_draw_states[0].m_buffer_object_hint);
        WRATHassert(m_draw_states[i].m_primitive_type==m_draw_states[0].m_primitive_type);
      }
  }
  #endif
}


bool
WRATHCompiledItemDrawStateCollection::
operator<(const WRATHCompiledItemDrawStateCollection &rhs) const
{
  /*
    to make the comparison faster we first sort by size of array
   */
  unsigned int ls, rs;

  ls=m_force_draw_orders.size();
  rs=rhs.m_force_draw_orders.size();

  return ls!=rs?
    ls<rs:
    (m_force_draw_orders < rhs.m_force_draw_orders
     or (m_force_draw_orders==rhs.m_force_draw_orders and m_draw_states<rhs.m_draw_states));
}
