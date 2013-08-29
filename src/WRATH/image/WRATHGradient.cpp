/*! 
 * \file WRATHGradient.cpp
 * \brief file WRATHGradient.cpp
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
#include "WRATHGradient.hpp"
#include "WRATHStaticInit.hpp"
#include <math.h>

#define NUMBER_GRADIENTS_PER_TEXTURE 128

/*
  Basic idea:

   1) A RawGradientData represents the data of a WRATHGradient.
      It stores the color stops to it's resolution. When it is
      flushed colors between the stops are calculated.
      A RawGradientData is reference counted object and a
      WRATHGradient object stores such a reference. Additionally,
      a RawGradientData stores a reference to which GradientTexture
      it is a part.

   1) A GradientTexture represents a set of RawGradientData of
      all the same resolution storted on a single GL texture.
      It stores a _pointer_ to each RawGradientData. A GradientTexture
      is a texture binder, thus reference counted. 

   2) a WRATHGradient stores a reference to it's RawGradientData, and
      for conveniance a reference to the GradientTexture of the 
      RawGradientData. When the WRATHGradient goes out of scope the
      reference to the RawGradientData does as well, which triggers
      the deletion of the RawGradientData object.

   3) Getting what GradientTexture and from where within what 
      GradientTexture is handled by _the_ GradientTextureAllocator.
      When a GradientTexture is created it adds itself to the
      list of available GradientTexture objects. When a RawGradientData
      is requested (by creating a WRATHGradient) it asks the 
      GradientTextureAllocator for it. This in turn checks it's
      if there is a GradientTexture of the correct parameters with
      a spot avalable, and if so allocates a spot from such an available
      GradientTexture.
  
   4) When a GradientTexture allocates a spot, it checks if it has additional
      spots available, and if not removes itself from the list of available
      GradientTexture objects from the GradientTextureAllocator

   5) When a RawGradientData is deleted, it frees the spot it has from
      the GradientTexture which in turn triggers the GradientTexture to mark
      itself as available to the GradientTextureAllocator. When a GradientTexture
      it marks itself as unavailable.
 */


namespace
{

  class RawGradientData;
  class GradientTextureAllocator;

  
  float
  compute_texture_coordinate(int py)
  {
    float y(static_cast<float>(py) + 0.5f);
    
    return y/static_cast<float>(NUMBER_GRADIENTS_PER_TEXTURE);
  }

  /*
    A gradient texture holds it's pixel data
    client side as well and presents an interface
    to mark line dirty/clean...
   */
  class GradientTexture:public WRATHTextureChoice::texture_base
  {
  public:
    typedef handle_t<GradientTexture> handle;

    GradientTexture(uint32_t log2_resolution, enum WRATHGradient::repeat_type_t r);
    ~GradientTexture();

    void
    bind_texture(GLenum);

    void
    mark_dirty(int);

    WRATHReferenceCountedObject::handle
    allocate(void);

    void
    deregister(RawGradientData*);

    enum WRATHGradient::repeat_type_t
    repeat_mode(void) const
    {
      return m_r;
    }

  private:

    void
    flush(void);

    /*
      ctor params
     */
    uint32_t m_log2_resolution;
    enum WRATHGradient::repeat_type_t m_r;

    /*
      GL .. stuff.
     */
    GLuint m_texture;
    ivec2 m_resolution;

    WRATHMutex m_mutex;
    unsigned int m_current_y;
    std::vector<int> m_free_ys;

    /*
      NOTE: we store a raw _pointer_ to the
      gradients. This is so that they can
      go out of scope, the deletion is triggered
      by a WRATHGradient going out of scope.
     */
    vecN<RawGradientData*, NUMBER_GRADIENTS_PER_TEXTURE> m_grads;

    /*
      list of those indices of m_grads
      that are dirty
     */
    std::set<int> m_dirty_grads;
  };


  class RawGradientData:public WRATHReferenceCountedObjectT<RawGradientData>
  {
  public:

    RawGradientData(int x_size, GradientTexture *pparent, int py);
    ~RawGradientData();

    float
    texture_coordinate_y(void) const
    {
      return m_y_normalized;
    }

    const WRATHStateBasedPackingData::handle&
    texture_coordinate_y_state_based_packing_data(void) const
    {
      return m_texture_coordinate_y_state_based_packing_data;
    }

    int
    texel(float t);

    int 
    set_color(float t, const vec4 &pcolor)
    {
      WRATHAutoLockMutex(m_mutex);
      mark_dirty();
      
      int I;

      I=texel(t);
      m_stops[I]=pcolor;

      return I;
    }

    void
    remove_color(int I)
    {
      WRATHAutoLockMutex(m_mutex);

      if(m_stops.find(I)!=m_stops.end())
        {
          m_stops.erase(I);
          mark_dirty();
        }
    }

    const std::vector<uint8_t>&
    color_bits(void)
    {
      flush();
      return m_raw_color_bits;
    }

    int
    y(void) const
    {
      return m_y;
    }

    WRATHTextureChoice::texture_base::handle
    binder(void);

    enum WRATHGradient::repeat_type_t
    repeat_mode(void) const
    {
      return m_repeat_mode;
    }

  private:

    void
    mark_dirty(void);

    void
    flush(void);

    GradientTexture::handle m_parent;
    int m_y;
    int m_resolution;
    float m_conversion_factor;
    float m_y_normalized;
    WRATHStateBasedPackingData::handle m_texture_coordinate_y_state_based_packing_data;
    enum WRATHGradient::repeat_type_t m_repeat_mode;

    std::map<int, vec4> m_stops;

    WRATHMutex m_mutex;
    std::vector<vec4> m_interpolate_color_value_float;
    std::vector<uint8_t> m_raw_color_bits;
    bool m_user_alive;
  };


  class GradientTextureAllocator:boost::noncopyable
  {
  public:
    void
    put_on_free_list(uint32_t log2_resolution, 
                     enum WRATHGradient::repeat_type_t r,
                     GradientTexture *q)
    {
      WRATHAutoLockMutex(m_mutex);
      m_have_free[log2_resolution][r].insert(q);
    }

    void
    remove_from_free_list(uint32_t log2_resolution, 
                          enum WRATHGradient::repeat_type_t r,
                          GradientTexture *q)
    {
      WRATHAutoLockMutex(m_mutex);
      m_have_free[log2_resolution][r].erase(q);
    }

    WRATHReferenceCountedObject::handle
    allocate(uint32_t log2_resolution, enum WRATHGradient::repeat_type_t r)
    {
      GradientTexture *q;

      WRATHAutoLockMutex(m_mutex);
      if(m_have_free[log2_resolution][r].empty())
        {
          q=WRATHNew GradientTexture(log2_resolution, r);
          m_have_free[log2_resolution][r].insert(q);
        }
      else
        {
          q=*m_have_free[log2_resolution][r].begin();
        }

      return q->allocate();
    }

  private:
    typedef std::set<GradientTexture*> free_texture_list;

    /*
      m_have_free[Resolution][repeat_mode] gives a list
      of GradientTexture objects that can be used to hold
      gradient texture data for the specified resolution
      for the specified repeat mode.
     */
    WRATHMutex m_mutex;
    vecN< vecN<free_texture_list, 3>, 8> m_have_free;
  };

  GradientTextureAllocator&
  gradient_allocator(void)
  {
    WRATHStaticInit();
    static GradientTextureAllocator R;
    return R;
  }
  
}


////////////////////////////////////////////////
//GradientTexture methods
GradientTexture::
GradientTexture(uint32_t log2_resolution, enum WRATHGradient::repeat_type_t r):
  m_log2_resolution(log2_resolution),
  m_r(r),
  m_texture(0),
  m_resolution(1<<log2_resolution, NUMBER_GRADIENTS_PER_TEXTURE),
  m_current_y(0),
  m_grads(NULL)
{
}

GradientTexture::
~GradientTexture()
{
  WRATHassert(m_current_y==m_free_ys.size());

  /*
    We have an.. issue here. we need to delete
    the texture if it is non-zero. The right thing
    is that the texture is deleted via queuing up
    a command to delete it.. we have the same issue
    in WRATHImage too. On a side note, a WRATHGradient
    object holds a _reference_ to a texture binder
    which in turn points to a GradientTexture, thus
    we are guaranteed that in the call statck to
    ~GradientTexture() is a deletion of a WRATHGradient
    object.. just as in WRATHImage..
   */
  if(m_texture!=0)
    {
      glDeleteTextures(1, &m_texture);
    }
  gradient_allocator().remove_from_free_list(m_log2_resolution, m_r, this);

}

void
GradientTexture::
mark_dirty(int y)
{
  WRATHAutoLockMutex(m_mutex);
  m_dirty_grads.insert(y);
}

void
GradientTexture::
deregister(RawGradientData *q)
{
  WRATHAutoLockMutex(m_mutex);

  unsigned int y;

  y=q->y();
  WRATHassert(m_grads[y]==q);
  WRATHassert(m_current_y>0);

  if(y==m_current_y-1)
    {
      --m_current_y;
    }
  else
    {
      m_free_ys.push_back(y);
    }
  m_grads[y]=NULL;

  WRATHassert(m_current_y>=m_free_ys.size());
  gradient_allocator().put_on_free_list(m_log2_resolution, m_r, this);
  
}

void
GradientTexture::
bind_texture(GLenum)
{
  if(m_texture==0)
    {
      const GLenum repeat_mode[]=
        {
          /*[Clamp]=*/ GL_CLAMP_TO_EDGE,
          /*[Repeat]=*/ GL_REPEAT,
          /*[MirrorRepeat]=*/ GL_MIRRORED_REPEAT,
        };

      glGenTextures(1, &m_texture);
      glBindTexture(GL_TEXTURE_2D, m_texture);
      glTexImage2D(GL_TEXTURE_2D, 
                   0, //mipmap
                   GL_RGBA,
                   m_resolution.x(), m_resolution.y(), 0, //size
                   GL_RGBA,
                   GL_UNSIGNED_BYTE,
                   NULL);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat_mode[m_r]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
  else
    {
      glBindTexture(GL_TEXTURE_2D, m_texture);
    }
  flush();
}

void
GradientTexture::
flush(void)
{
  std::set<int> dirty_grads;
  vecN<RawGradientData::handle, NUMBER_GRADIENTS_PER_TEXTURE> temp_handles;
 

  /*
    this is.. amusing. Basic ideas are:
    1) get the list of dirty gradients
    2) save handles to the gradients to make
       sure they don't get reference-counted
       deleted midway.
   */
  WRATHLockMutex(m_mutex);
  std::swap(dirty_grads, m_dirty_grads);
  std::copy(m_grads.begin(), m_grads.end(), temp_handles.begin());
  WRATHUnlockMutex(m_mutex);
  

  for(std::set<int>::iterator iter=dirty_grads.begin(),
        end=dirty_grads.end(); iter!=end; ++iter)
    {
      int y(*iter);

      if(temp_handles[y].valid())
        {
          const_c_array<uint8_t> color_bits(temp_handles[y]->color_bits());
          glTexSubImage2D(GL_TEXTURE_2D,
                          0, //LOD
                          0, y, // coordinate of rect
                          m_resolution.x(), 1, //size of rect
                          GL_RGBA,
                          GL_UNSIGNED_BYTE,
                          color_bits.c_ptr());
        }
    }
}


WRATHReferenceCountedObject::handle
GradientTexture::
allocate(void)
{
  int y;

  WRATHAutoLockMutex(m_mutex);

  if(m_free_ys.empty())
    {
      WRATHassert(m_current_y<NUMBER_GRADIENTS_PER_TEXTURE);

      y=m_current_y;
      ++m_current_y;
    }
  else
    {
      y=m_free_ys.back();
      m_free_ys.pop_back();
    }

  if(NUMBER_GRADIENTS_PER_TEXTURE==m_current_y and m_free_ys.empty())
    {
      gradient_allocator().remove_from_free_list(m_log2_resolution, m_r, this);
    }

  RawGradientData *ptr;

  ptr=WRATHNew RawGradientData(m_resolution.x(), this, y);
  m_grads[y]=ptr;

  return ptr;

}


/////////////////////////////////////////////////
// RawGradientData methods
RawGradientData::
RawGradientData(int x_size, GradientTexture *pparent, int py):
  m_parent(pparent),
  m_y(py),
  m_resolution(x_size),
  m_conversion_factor(static_cast<float>(x_size)),
  m_y_normalized(compute_texture_coordinate(py)),
  m_repeat_mode(pparent->repeat_mode()),
  m_interpolate_color_value_float(x_size, vec4(1.0f, 1.0f, 1.0f, 1.0f)),
  m_raw_color_bits(4*x_size, 255u),
  m_user_alive(true)
{
  m_texture_coordinate_y_state_based_packing_data=WRATHNew WRATHGradient::GradientYCoordinate(m_y_normalized);
}

RawGradientData::
~RawGradientData()
{
  m_parent->deregister(this);
}


WRATHTextureChoice::texture_base::handle
RawGradientData::
binder(void)
{
  return m_parent;
}


void
RawGradientData::
mark_dirty(void)
{
  m_parent->mark_dirty(m_y);
}

int
RawGradientData::
texel(float t)
{
  /*
    the _center_ of the texel at I is the color
    stop at (I+0.5)/m_resolution. Thus, 
    I = floor(t*m_resolution - 0.5).
   */
  int I;
  I=floorf(m_conversion_factor*t - 0.5f);

  if(m_repeat_mode==WRATHGradient::Repeat)
    {
      I=I%m_resolution;
    }
  else if(m_repeat_mode==WRATHGradient::MirrorRepeat)
    {
      I=I%(2*m_resolution);
      I=(I<m_resolution)?
        I:
        2*m_resolution-1-I;           
    }
  else
    {
      I=std::max(0, std::min(I, m_resolution-1));
    }
  return I;
}

void
RawGradientData::
flush(void)
{
  WRATHAutoLockMutex(m_mutex);
  if(!m_stops.empty())
    {
      int lastIndex = 0;
      vec4 lastColor;
      
      switch(m_repeat_mode)
        {
        case WRATHGradient::Clamp:
        case WRATHGradient::MirrorRepeat:
          lastColor=m_stops.begin()->second;
          lastIndex=0;
          break;
        case WRATHGradient::Repeat:
          lastColor=m_stops.rbegin()->second;
          lastIndex=-(m_resolution-1-m_stops.rbegin()->first);
          break;
        }
      
      
      for(std::map<int, vec4>::iterator iter=m_stops.begin(),
            end=m_stops.end(); iter!=end; ++iter)
        {
          vec4 nextColor(iter->second);
          float delta_t(iter->first-lastIndex);
          
          if(iter->first!=lastIndex)
            {
              delta_t=1.0f/delta_t;
            }
          
          for(int I=std::max(0,lastIndex), endI=iter->first; I<endI; ++I)
            {
              float t;
              
              t=static_cast<float>(I-lastIndex)*delta_t;
              m_interpolate_color_value_float[I] = (1.0f-t)*lastColor + t*nextColor;
            }
          lastColor=nextColor;
          lastIndex=iter->first;
        }
      
      
      
      switch(m_repeat_mode)
        {
        case WRATHGradient::Clamp:
        case WRATHGradient::MirrorRepeat:
          for(int I=lastIndex; I<m_resolution; ++I)
            {
              m_interpolate_color_value_float[I]=lastColor;
            }
          break;
          
        case WRATHGradient::Repeat:
          {
            vec4 nextColor;
            int delta_tI;
            float delta_t;
            
            nextColor=m_stops.begin()->second;
            delta_tI=m_resolution-1-lastIndex+m_stops.begin()->first;
            delta_t=(delta_tI!=0)?
              1.0f/static_cast<float>(delta_tI):
              0.0f;
            
            for(int I=lastIndex; I<m_resolution; ++I)
              {
                float t;
                
                t=static_cast<float>(I-lastIndex)*delta_t;
                m_interpolate_color_value_float[I] = (1.0f-t)*lastColor + t*nextColor;
              }
          }
          break;
        }

      /*
        dump that into m_image, note that raw_bytes is
        twice as long, so that we set both "lines"
        within m_image
      */
      c_array<uint8_t> raw_bytes_ptr(m_raw_color_bits);
      c_array< vecN<uint8_t,4> > color_ptr;
      
      color_ptr=raw_bytes_ptr.reinterpret_pointer<vecN<uint8_t,4> >();
      for(int i=0;i<m_resolution;++i)
        {
          for(int c=0;c<4;++c)
            {
              int32_t pre_clamp, clamped;
              
              pre_clamp=static_cast<int32_t>(255.0f*m_interpolate_color_value_float[i][c]);
              clamped=std::max(0, std::min(255, pre_clamp));
              
              color_ptr[i][c]= static_cast<uint8_t>(clamped); 
            }
        }
    }
  else
    {
      std::fill(m_raw_color_bits.begin(), m_raw_color_bits.end(), 0xFF);
    }
}



///////////////////////////////////////////////
//WRATHGradient::parameters methods
WRATHGradient::parameters::
parameters(enum repeat_type_t tp, float delta_t):
  m_repeat_type(tp),
  m_log2_resolution(0)
{
  delta_t=std::max(1.0f/1024.0f, delta_t);
  
  int res;
  res=1.0f/delta_t;
  while(res>>=1 and m_log2_resolution<8)
    {
      ++m_log2_resolution;
    }
}


/////////////////////////////////////////
// WRATHGradient methods
WRATH_RESOURCE_MANAGER_IMPLEMENT(WRATHGradient, std::string);

WRATHGradient::
WRATHGradient(const std::string &presource_name, 
              const parameters &pp):
  m_registered(true),
  m_resource_name(presource_name)
{
  resource_manager().add_resource(m_resource_name, this);
  construct(pp);
}

WRATHGradient::
WRATHGradient(const parameters &pp):
  m_registered(false)
{
  construct(pp);
}

WRATHGradient::
~WRATHGradient()
{
  m_dtor_signal();
  if(m_registered)
    {
      resource_manager().remove_resource(this);
    }
}

void
WRATHGradient::
construct(const parameters &pp)
{
  m_data_handle=gradient_allocator().allocate(std::max(0, std::min(8, pp.m_log2_resolution)), 
                                              pp.m_repeat_type);

  m_binder=m_data_handle.static_cast_handle<RawGradientData>()->binder();
}

const WRATHStateBasedPackingData::handle&
WRATHGradient::
texture_coordinate_y_state_based_packing_data(void) const
{
  WRATHassert(m_data_handle.dynamic_cast_handle<RawGradientData>().valid());
  return m_data_handle.static_cast_handle<RawGradientData>()->texture_coordinate_y_state_based_packing_data();
}

float
WRATHGradient::
texture_coordinate_y(void) const
{
  WRATHassert(m_data_handle.dynamic_cast_handle<RawGradientData>().valid());
  return m_data_handle.static_cast_handle<RawGradientData>()->texture_coordinate_y();
}

int
WRATHGradient::
texel(float t) const
{
  WRATHassert(m_data_handle.dynamic_cast_handle<RawGradientData>().valid());
  return m_data_handle.static_cast_handle<RawGradientData>()->texel(t);
}

int
WRATHGradient::
set_color(float t, const color &pcolor)
{
  WRATHassert(m_data_handle.dynamic_cast_handle<RawGradientData>().valid());
  return m_data_handle.static_cast_handle<RawGradientData>()->set_color(t, pcolor.m_value);
}

void
WRATHGradient::
remove_color(int texel)
{
  WRATHassert(m_data_handle.dynamic_cast_handle<RawGradientData>().valid());
  m_data_handle.static_cast_handle<RawGradientData>()->remove_color(texel);
}

enum WRATHGradient::repeat_type_t
WRATHGradient::
repeat_mode(void) const
{
  WRATHassert(m_data_handle.dynamic_cast_handle<RawGradientData>().valid());
  return m_data_handle.static_cast_handle<RawGradientData>()->repeat_mode();
}


WRATHUniformData::uniform_by_name_base::handle
WRATHGradient::
texture_coordinate_y_uniform(const std::string &puniform_name) const
{
  std::map<std::string, WRATHUniformData::uniform_by_name_base::handle>::iterator iter;

  iter=m_uniforms.find(puniform_name);
  if(iter!=m_uniforms.end())
    {
      return iter->second;
    }

  WRATHUniformData::uniform_by_name_base::handle R;
  R=WRATHNew WRATHUniformData::uniform_by_name<float>(puniform_name, texture_coordinate_y());

  m_uniforms[puniform_name]=R;
  return R;

}
