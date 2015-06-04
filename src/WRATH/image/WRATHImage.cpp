/*! 
 * \file WRATHImage.cpp
 * \brief file WRATHImage.cpp
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
#include <map>
#include <set>
#include <vector>
#include "WRATHImage.hpp"
#include "WRATHUtil.hpp"
#include "WRATHGPUConfig.hpp"
#include "WRATHStaticInit.hpp"

namespace
{
  class GLPixelStore;

  class global_consumption_stat:
    public WRATHReferenceCountedObjectT<global_consumption_stat>
  {
  public:
    WRATHImage::TextureAllocatorHandle::texture_consumption_data_type m_stats;
  };

  class consumption_stats:
    public WRATHReferenceCountedObjectT<consumption_stats>
  {
  public:

    global_consumption_stat::handle m_global;
    WRATHImage::TextureAllocatorHandle::texture_consumption_data_type m_local;

    explicit 
    consumption_stats(global_consumption_stat::handle h):
      m_global(h)
    {}

    void
    note_new_atlas(const ivec2 &psize)
    {
      int d(psize.x()*psize.y());
      m_global->m_stats.m_number_texels+=d;
      m_local.m_number_texels+=d;
    }

    void
    note_delete_atlas(const ivec2 &psize)
    {
      int d(psize.x()*psize.y());
      m_global->m_stats.m_number_texels-=d;
      m_local.m_number_texels-=d;
    }

    void
    note_new_rect(const ivec2 &psize)
    {
      int d(psize.x()*psize.y());
      m_global->m_stats.m_number_texels_used+=d;
      m_local.m_number_texels_used+=d;
    }

    void
    note_delete_rect(const ivec2 &psize)
    {
      int d(psize.x()*psize.y());
      m_global->m_stats.m_number_texels_used-=d;
      m_local.m_number_texels_used-=d;
    }
  };

  class pixel_store_set:public std::set<GLPixelStore*>
  {
  public:
    explicit
    pixel_store_set(global_consumption_stat::handle h)
    {
      m_stats=WRATHNew consumption_stats(h);
    }

    consumption_stats::handle m_stats;
  };

  class LocalAtlasType:public WRATHAtlas
  {
  public:
    typedef handle_t<LocalAtlasType> handle;

    LocalAtlasType(const ivec2 &psize, 
                   WRATHPixelStore *pix,
                   consumption_stats::handle h):
      WRATHAtlas(psize, pix),
      m_h(h)
    {
      m_h->note_new_atlas(psize);
    }

    ~LocalAtlasType(void)
    {
      m_h->note_delete_atlas(size());
    }

    virtual
    const rectangle_handle*
    add_rectangle(const ivec2 &dimension)
    {
      const rectangle_handle *R;
      R=WRATHAtlas::add_rectangle(dimension);

      if(R!=NULL)
        {
          m_h->note_new_rect(dimension);
        }
      return R;
    }

  protected:
    virtual
    enum return_code
    remove_rectangle_implement(const rectangle_handle *im)
    {
      ivec2 sz(im->size());
      enum return_code R;

      R=WRATHAtlas::remove_rectangle_implement(im);
      if(routine_success==R)
        {
          m_h->note_delete_rect(sz);
        }
      return R;
    }

  private:
    consumption_stats::handle m_h;
  };


  #if defined(WRATH_GL_VERSION)
    typedef std::map<WRATHImage::ImageFormatArray, 
                     pixel_store_set, 
                     WRATHImage::ImageFormatArrayComparer> map_type;

    typedef std::map<WRATHImage::ImageFormatArray, 
                     std::vector<std::vector<uint8_t> >, 
                     WRATHImage::ImageFormatArrayComparer> clear_map_type;
  #else
    typedef std::map<WRATHImage::ImageFormatArray, 
                     pixel_store_set> map_type;

    typedef std::map<WRATHImage::ImageFormatArray, 
                     std::vector<std::vector<uint8_t> > > clear_map_type;
  #endif


  /*
    A word about dtor'ing of objects.

    1) a GLPixelStore has a _WEAK_ pointer to the WRATHAtlas 
       that uses it
    2) but a GLPixelStore has a _handle_ (and thus a reference) 
       to the TextureAllocator that created it
    3) each rectangle allocated by a WRATHAtlas has a 
       handle (and thus a reference) to the creating WRATHAtlas.
      
    When an image is created the following happens:
     a rectangle is created from a WRATHAtlas, that WRATHAtlas
     has as it's WRATHPixelStore a GLPixelStore, which in turn
     has a pointer (but not a handle) back to the WRATHAtlas
     and the GLPixelStore also has a _HANDLE_ to the 
     TextureAllocator.

    Whan the image is deleted, the rectangle of it
    is also deleted. That deletion potentially
    triggers the deletion a WRATHAtlas which
    in turn deletes a GLPixelStore which then
    in turn potentially deleted a TextureAllocator.
    
   */


  class TextureAllocator:
    public WRATHReferenceCountedObjectT<TextureAllocator>
  {
  public:

    TextureAllocator(bool memeset_zero_texture_data, ivec2 dim,
                     GLenum texture_wrap_s, GLenum texture_wrap_t):
      m_texture_atlas_dimension(dim),
      m_memeset_zero_texture_data(memeset_zero_texture_data),
      m_texture_wrap_s(texture_wrap_s),
      m_texture_wrap_t(texture_wrap_t)
    {
      m_total_stats=WRATHNew global_consumption_stat();
    }

    ~TextureAllocator()
    {}

    const WRATHAtlas::rectangle_handle*
    allocate(const WRATHImage::ImageFormatArray &fmt, 
             const ivec2 &psize);

    enum return_code
    allocate_multiple_images_on_same_page(const WRATHImage::ImageFormatArray &fmt,
                                          const_c_array<ivec2> in_sizes,
                                          std::list<const WRATHAtlas::rectangle_handle*> &out_rects);
      
    GLPixelStore*
    generate_new_atlas(const WRATHImage::ImageFormatArray &fmt);


    WRATHMutex m_mutex;
    map_type m_map;
    vecN<uint32_t, 2> m_texture_atlas_dimension;
    bool m_memeset_zero_texture_data;
    GLenum m_texture_wrap_s, m_texture_wrap_t;
    clear_map_type m_clear_bits;
    global_consumption_stat::handle m_total_stats;
  };


  class TexSubImageCommand
  {
  public:
    TexSubImageCommand(void):
      m_LOD(-1),
      m_place(-1, -1),
      m_size(-1, -1),
      m_pixel_data_format(GL_INVALID_ENUM),
      m_pixel_type(GL_INVALID_ENUM),
      m_alignment(-1),
      m_update_mips(false),
      m_clear_region(false)
    {}

    std::vector<uint8_t> m_pixels;
    int m_LOD;
    ivec2 m_place, m_size;
    GLenum m_pixel_data_format;
    GLenum m_pixel_type;
    unsigned int m_alignment;
    bool m_update_mips;

    /*
      special command to indicate that
      not to load pixels, but to clear
      at [m_place, m_place+m_size]
     */
    bool m_clear_region;

    /*
      if non-empty specifies the pixel value bits
      for the clear color.
     */
    std::vector<uint8_t> m_clear_pixel_value;
  };


  class GLPixelStore:public WRATHPixelStore
  {
  public:
    GLPixelStore(const WRATHImage::ImageFormatArray &fmt, 
                 const ivec2 &psize,
                 GLenum texture_wrap_mode_s,
                 GLenum texture_wrap_mode_t);

    GLPixelStore(const WRATHImage::ImageFormatArray &fmt, 
                 const ivec2 &psize,
                 TextureAllocator::handle h,
                 GLenum texture_wrap_mode_s,
                 GLenum texture_wrap_mode_t,
                 consumption_stats::handle ch);

    GLPixelStore(const WRATHImage::ImageFormat &fmt,
                 const ivec2 &psize,
                 GLuint tex);

    ~GLPixelStore();

    void
    add_clear_command(const ivec2 &bl, const ivec2 &sz, 
                      const WRATHImage::ImageFormatArray &fmt,
                      const_c_array<std::vector<uint8_t> > clear_bits);

    void
    add_clear_command(const ivec2 &bl, const ivec2 &sz, 
                      const_c_array<std::vector<uint8_t> > clear_bits)
    {
      add_clear_command(bl, sz, m_format, clear_bits);
    }

    void
    add_clear_command(const ivec2 &bl, const ivec2 &sz)
    {
      add_clear_command(bl, sz, const_c_array<std::vector<uint8_t> >());
    }

    void
    bind_texture(int layer);

    void
    create_gl_texture(int layer);

    WRATHUniformData::uniform_setter_base::handle
    texture_size(const std::string &pname);

    //note: is a pointer not a handle
    //because m_atlas "owns" this.
    WRATHAtlasBase *m_atlas;
    WRATHImage::ImageFormatArray m_format;
    std::vector<GLuint> m_texture;
    ivec2 m_size;
    std::vector<bool> m_mipmaps_dirty, m_has_mipmaps;
    bool m_own_texture;
    std::vector<WRATHTextureChoice::texture_base::handle> m_texture_binder;
    GLenum m_texture_wrap_mode_s, m_texture_wrap_mode_t;

    std::map<std::string, WRATHUniformData::uniform_setter_base::handle> m_uniform_texture_size;

    TextureAllocator::handle m_h;

    WRATHMutex m_mutex;
    std::vector<std::list<TexSubImageCommand> > m_deffered_uploads;
  };

  class GLPixelStoreTextureBinder:public WRATHTextureChoice::texture_base
  {
  public:
    GLPixelStore *m_pixel_store;
    int m_layer;

    GLPixelStoreTextureBinder(GLPixelStore *ptr, int layer):
      WRATHTextureChoice::texture_base(),
      m_pixel_store(ptr),
      m_layer(layer)
    {}

    void
    bind_texture(GLenum)
    {
      if(m_pixel_store!=NULL)
        {
          m_pixel_store->bind_texture(m_layer);
        }
      else
        {
          glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    WRATHUniformData::uniform_setter_base::handle
    texture_size(const std::string &pname)
    {
      return m_pixel_store->texture_size(pname);
    }
  };

  class GLPixelStoreTextureSizeUniform:
    public WRATHUniformData::uniform_by_name_base
  {
  public:
    GLPixelStoreTextureSizeUniform(const std::string &pname, ivec2 psize):
      uniform_by_name_base(pname+"Size"),
      m_v(psize.x(), psize.y())
    {}

    virtual
    void
    set_uniform_value(GLint location)
    {
      WRATHglUniform(location, m_v);
    }

  private:
    vec2 m_v;
  };

}  

////////////////////////////
// GLPixelStore methods
GLPixelStore::
GLPixelStore(const WRATHImage::ImageFormatArray &fmt, 
             const ivec2 &psize,
             GLenum texture_wrap_mode_s,
             GLenum texture_wrap_mode_t):
  m_atlas(NULL),
  m_format(fmt),
  m_texture(fmt.size(), 0),
  m_size(psize),
  m_mipmaps_dirty(fmt.size(), false),
  m_has_mipmaps(fmt.size()),
  m_own_texture(true),
  m_texture_binder(fmt.size()),
  m_texture_wrap_mode_s(texture_wrap_mode_s),
  m_texture_wrap_mode_t(texture_wrap_mode_t),
  m_deffered_uploads(fmt.size())
{
  for(unsigned int i=0, endi=m_format.size(); i<endi; ++i)
    {
      m_texture_binder[i]=WRATHNew GLPixelStoreTextureBinder(this, i);
      m_has_mipmaps[i]=m_format.format(i).requires_mipmaps();
    }
  m_atlas=WRATHNew WRATHAtlas(m_size, this);
}

GLPixelStore::
GLPixelStore(const WRATHImage::ImageFormatArray &fmt, 
             const ivec2 &psize,
             TextureAllocator::handle h,
             GLenum texture_wrap_mode_s,
             GLenum texture_wrap_mode_t,
             consumption_stats::handle ch):
  m_atlas(NULL),
  m_format(fmt),
  m_texture(fmt.size(), 0),
  m_size(psize),
  m_mipmaps_dirty(fmt.size(), false),
  m_has_mipmaps(fmt.size()),
  m_own_texture(true),
  m_texture_binder(fmt.size()),
  m_texture_wrap_mode_s(texture_wrap_mode_s),
  m_texture_wrap_mode_t(texture_wrap_mode_t),
  m_h(h),
  m_deffered_uploads(fmt.size())
{
  for(unsigned int i=0, endi=m_format.size(); i<endi; ++i)
    {
      m_texture_binder[i]=WRATHNew GLPixelStoreTextureBinder(this, i);
      m_has_mipmaps[i]=m_format.format(i).requires_mipmaps();
    }
  m_atlas=WRATHNew LocalAtlasType(m_size, this, ch);  
}


GLPixelStore::
GLPixelStore(const WRATHImage::ImageFormat &fmt,
             const ivec2 &psize, GLuint tex):
  m_atlas(NULL),
  m_format(fmt),
  m_texture(1, tex),
  m_size(psize),
  m_mipmaps_dirty(1, false),
  m_has_mipmaps(1, false),
  m_own_texture(false),
  m_deffered_uploads(1)
{
  WRATHassert(m_texture[0]!=0);
  m_texture_binder.resize(1);
  m_texture_binder[0]=WRATHNew WRATHTextureChoice::texture(tex);
  m_atlas=WRATHNew WRATHAtlas(m_size, this);
}

WRATHUniformData::uniform_setter_base::handle
GLPixelStore::
texture_size(const std::string &pname) 
{
  std::map<std::string, WRATHUniformData::uniform_setter_base::handle>::iterator iter;
     
  WRATHAutoLockMutex(m_mutex);
  iter=m_uniform_texture_size.find(pname);
  if(iter!=m_uniform_texture_size.end())
    {
      return iter->second;
    }

  WRATHUniformData::uniform_setter_base::handle R;
  R=WRATHNew GLPixelStoreTextureSizeUniform(pname, m_size);
  m_uniform_texture_size[pname]=R;

  return R;
}


GLPixelStore::
~GLPixelStore()
{
  if(m_own_texture)
    {
      if(m_h.valid())
        {
          map_type::iterator iter;
          
          WRATHLockMutex(m_h->m_mutex);
          
          iter=m_h->m_map.find(m_format);
          WRATHassert(iter!=m_h->m_map.end());
          
          iter->second.erase(this);
          
          WRATHUnlockMutex(m_h->m_mutex);
        }
      
      /*
        TODO: deleting the texture should be
        forced to be done in the rendering thread,
        via a TripleBufferEnabler object...
      */
      for(unsigned int i=0, endi=m_texture.size(); i<endi; ++i)
        {
          if(m_texture[i]!=0)
            {
              glDeleteTextures(1, &m_texture[i]);
            }

          WRATHReferenceCountedObject::handle_t<GLPixelStoreTextureBinder> pp;
          pp=m_texture_binder[i].dynamic_cast_handle<GLPixelStoreTextureBinder>();
          if(pp.valid())
            {
              pp->m_pixel_store=NULL;
            }
        }
    }
  //NOTE that we do NOT delete m_atlas
  //this is because m_atlas owns "this".
}

void
GLPixelStore::
add_clear_command(const ivec2 &bl, const ivec2 &sz,
                  const WRATHImage::ImageFormatArray &fmt,
                  const_c_array<std::vector<uint8_t> > clear_bits)
{
  

  WRATHLockMutex(m_mutex);

  for(unsigned int layer=0, endlayer=std::min(fmt.size(), m_format.size()); layer<endlayer; ++layer)
    {
      const WRATHImage::PixelImageFormat &px(fmt[layer].m_pixel_format);
      m_deffered_uploads[layer].push_back(TexSubImageCommand());
      m_deffered_uploads[layer].back().m_place=bl;
      m_deffered_uploads[layer].back().m_size=sz;
      m_deffered_uploads[layer].back().m_LOD=0;
      m_deffered_uploads[layer].back().m_clear_region=true;
      m_deffered_uploads[layer].back().m_pixel_data_format=px.m_pixel_data_format;
      m_deffered_uploads[layer].back().m_pixel_type=px.m_pixel_type;
      if(layer<clear_bits.size())
        {
          m_deffered_uploads[layer].back().m_clear_pixel_value=clear_bits[layer]; 
        }
      WRATHassert(m_deffered_uploads[layer].back().m_size.x()>0);
      WRATHassert(m_deffered_uploads[layer].back().m_size.y()>0);
      

      if(fmt[layer].requires_mipmaps())
        {
          ivec2 mip_sz(sz/2);
          ivec2 mip_bl(bl/2);

          for(int LOD=1; mip_sz.x()>0 and mip_sz.y()>0; ++LOD, mip_sz/=2, mip_bl/=2)
            {
              m_deffered_uploads[layer].push_back(TexSubImageCommand());
              m_deffered_uploads[layer].back().m_place=mip_bl;
              m_deffered_uploads[layer].back().m_size.x()=std::max(1, mip_sz.x());
              m_deffered_uploads[layer].back().m_size.y()=std::max(1, mip_sz.y());
              m_deffered_uploads[layer].back().m_LOD=LOD;
              m_deffered_uploads[layer].back().m_clear_region=true;
              m_deffered_uploads[layer].back().m_pixel_data_format=px.m_pixel_data_format;
              m_deffered_uploads[layer].back().m_pixel_type=px.m_pixel_type;
              if(layer<clear_bits.size())
                {
                  m_deffered_uploads[layer].back().m_clear_pixel_value=clear_bits[layer]; 
                }
            }
        }
    }
  WRATHUnlockMutex(m_mutex);
}

void
GLPixelStore::
bind_texture(int layer)
{
  
  std::vector<uint8_t> zero_bytes;
  std::list<TexSubImageCommand> cmds;

  WRATHLockMutex(m_mutex);
  if(m_texture[layer]==0)
    {
      create_gl_texture(layer);
    }
  std::swap(m_deffered_uploads[layer], cmds);
  WRATHUnlockMutex(m_mutex);
  
  
  WRATHassert(m_texture[layer]!=0);
  glBindTexture(GL_TEXTURE_2D, m_texture[layer]);
  for(std::list<TexSubImageCommand>::iterator 
        iter=cmds.begin(), end=cmds.end();
      iter!=end; ++iter)
    {
      const TexSubImageCommand &value(*iter);

      if(value.m_clear_region)
        {
          /*
            issue glTexSubImage2D but data is all zeros.
          */
          WRATHImage::PixelImageFormat fmt;
          
          fmt
            .pixel_data_format(value.m_pixel_data_format)
            .pixel_type(value.m_pixel_type);

          unsigned int bpp(fmt.bytes_per_pixel());
          
          zero_bytes.resize(value.m_size.x()*value.m_size.y()*bpp);
          if(value.m_clear_pixel_value.empty())
            {
              std::fill(zero_bytes.begin(), zero_bytes.end(), 0);
            }
          else
            {
              for(int p=0, endp=zero_bytes.size(); p<endp; )
                {
                  unsigned int endc, c;

                  for(c=0, endc=value.m_clear_pixel_value.size();
                      c<endc and p<endp and c<bpp; ++c, ++p)
                    {
                      zero_bytes[p]=value.m_clear_pixel_value[c];
                    }
                  for(;c<bpp and p<endp; ++c, ++p)
                    {
                      zero_bytes[p]=0;
                    }
                }
            }

          /*
            if bpp is a power of 2, then we can
            use it's size as the alignment size
           */
          unsigned int alignment;

          if(WRATHUtil::is_power_of_2(bpp))
            {
              /*
                out of paranoia we do not use 8
                even if we could...
               */
              alignment=std::min(4u, bpp);
            }
          else
            {
              alignment=1;
            }

          glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
          glTexSubImage2D(GL_TEXTURE_2D,
                          value.m_LOD,
                          value.m_place.x(), value.m_place.y(),
                          value.m_size.x(), value.m_size.y(),
                          value.m_pixel_data_format,
                          value.m_pixel_type,
                          &zero_bytes[0]);
        }
      else
        {
          glPixelStorei(GL_UNPACK_ALIGNMENT, value.m_alignment);
          glTexSubImage2D(GL_TEXTURE_2D,
                          value.m_LOD,
                          value.m_place.x(), value.m_place.y(),
                          value.m_size.x(), value.m_size.y(),
                          value.m_pixel_data_format,
                          value.m_pixel_type,
                          &value.m_pixels[0]);
          
          m_mipmaps_dirty[layer]=value.m_update_mips and m_has_mipmaps[layer];
        }
    }

  if(m_mipmaps_dirty[layer])
    {
      m_mipmaps_dirty[layer]=false;
      glGenerateMipmap(GL_TEXTURE_2D);
    }
}


void
GLPixelStore::
create_gl_texture(int layer)
{
  WRATHassert(m_texture[layer]==0);
  
  glGenTextures(1, &m_texture[layer]);
  WRATHassert(m_texture[layer]!=0);
  
  //allocate the texture data:
  glBindTexture(GL_TEXTURE_2D, m_texture[layer]);
  
  const void *init_pixels(NULL);
  
#ifdef WRATHDEBUG
  std::vector<uint8_t> init_image(m_size.x()*m_size.y()*m_format[layer].m_pixel_format.bytes_per_pixel(), 0x77);
  init_pixels=&init_image[0];
#endif
  
  glTexImage2D(GL_TEXTURE_2D, 
               0, //mipmap
               m_format[layer].m_internal_format,
               m_size.x(), m_size.y(), 0,
               m_format[layer].m_pixel_format.m_pixel_data_format, 
               m_format[layer].m_pixel_format.m_pixel_type, 
               init_pixels);

  if(m_has_mipmaps[layer])
    {
      /*
        explictiely allocate mipmaps:
      */
      for(int m=1, w=m_size.x()/2, h=m_size.y()/2;
          w>=1 or h>=1; w/=2, h/=2, ++m)
        {
          glTexImage2D(GL_TEXTURE_2D, 
                       m, //mipmap
                       m_format[layer].m_internal_format,
                       std::max(1, w), 
                       std::max(1, h), 
                       0,
                       m_format[layer].m_pixel_format.m_pixel_data_format, 
                       m_format[layer].m_pixel_format.m_pixel_type, 
                       init_pixels);
        }
    }
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                  m_format[layer].m_minification_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
                  m_format[layer].m_magnification_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_texture_wrap_mode_s);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_texture_wrap_mode_t);


  if(m_format[layer].m_max_mip_level>=0 
     and WRATHGPUConfig::gl_max_texture_level()!=GL_INVALID_ENUM)
    {
      glTexParameteri(GL_TEXTURE_2D, 
                      WRATHGPUConfig::gl_max_texture_level(), 
                      m_format[layer].m_max_mip_level);
    }
}


/////////////////////////////////////////////
//TextureAllocator methods
GLPixelStore*
TextureAllocator::
generate_new_atlas(const WRATHImage::ImageFormatArray &fmt)
{
  GLPixelStore *pix(NULL);
      
  map_type::iterator iter;
  iter=m_map.find(fmt);

  if(iter==m_map.end())
    {
      std::pair<map_type::iterator, bool> R;
      map_type::value_type V(fmt, pixel_store_set(m_total_stats));

      R=m_map.insert(V);
      WRATHassert(R.second);
      iter=R.first;
    }

  //create a new pixel store and WRATHAtlas:
  pix=WRATHNew GLPixelStore(fmt, 
                            ivec2(m_texture_atlas_dimension.x(), 
                                  m_texture_atlas_dimension.y()),
                            this,
                            m_texture_wrap_s,
                            m_texture_wrap_t,
                            iter->second.m_stats);
  
  //insert pixelstore into useable set of pixel stores.
  iter->second.insert(pix);
  
  if(m_memeset_zero_texture_data)
    {
      clear_map_type::iterator iter;
      
      iter=m_clear_bits.find(fmt);
      if(iter==m_clear_bits.end())
        {
          pix->add_clear_command(ivec2(0,0), 
                                 ivec2(m_texture_atlas_dimension.x(),
                                       m_texture_atlas_dimension.y()));
        }
      else
        {
          pix->add_clear_command(ivec2(0,0), 
                                 ivec2(m_texture_atlas_dimension.x(),
                                       m_texture_atlas_dimension.y()),
                                 iter->second);
        }
    }
  return pix;
}



const WRATHAtlas::rectangle_handle*
TextureAllocator::
allocate(const WRATHImage::ImageFormatArray &fmt, 
         const ivec2 &sz)
{
  const WRATHAtlas::rectangle_handle *return_value(NULL);

  WRATHLockMutex(m_mutex);

  map_type::iterator iter;
  iter=m_map.find(fmt);
  
  if(iter!=m_map.end())
    {
      //TODO: this is not the best way to go about this, walking
      //each atlas available, better to make a list of what atlases
      //are avialable keyed by sizes...
      for(std::set<GLPixelStore*>::iterator siter=iter->second.begin(),
            send=iter->second.end(); siter!=send and return_value==NULL; ++siter)
        {
          return_value=(*siter)->m_atlas->add_rectangle(sz);
        }
    }
  
  if(return_value==NULL)
    {
      GLPixelStore *pix(NULL);

      /*
        TODO: if too many atlases are already
        made then we should obey our memory 
        consumption and issue a fail which the
        caller then needs to handle.
       */
      pix=generate_new_atlas(fmt);
      return_value=pix->m_atlas->add_rectangle(sz);
      WRATHassert(return_value!=NULL);
    }

  

  WRATHUnlockMutex(m_mutex);  
  return return_value;
}

enum return_code
TextureAllocator::
allocate_multiple_images_on_same_page(const WRATHImage::ImageFormatArray &fmt,
                                      const_c_array<ivec2> in_sizes,
                                      std::list<const WRATHAtlas::rectangle_handle*> &out_rects)
{
  WRATHAutoLockMutex(m_mutex);
  enum return_code return_value(routine_fail);

  /*
    TODO:
    first check if it is even possible to allocate all
    those images...
   */

  /*
    is possible, now we go ahead and take the first
    texture atlas that handles the job, if none do,
    make a new atlas.
   */
  map_type::iterator iter;
  iter=m_map.find(fmt);
  
  if(iter!=m_map.end())
    {
      //TODO: this is not the best way to go about this, walking
      //each atlas available, better to make a list of what atlases
      //are avialable keyed by sizes...
      for(std::set<GLPixelStore*>::iterator siter=iter->second.begin(),
            send=iter->second.end(); siter!=send and return_value==routine_fail; ++siter)
        {
          return_value=(*siter)->m_atlas->add_rectangles(in_sizes, out_rects);
        }
    }

  if(return_value==routine_fail)
    {
      GLPixelStore *pix(NULL);

      pix=generate_new_atlas(fmt);
      return_value=pix->m_atlas->add_rectangles(in_sizes, out_rects);
    }
  return return_value;
}


/////////////////////////////////////////
//WRATHImage::TextureAllocatorHandle methods
WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
WRATHImage::TextureAllocatorHandle::
texture_consumption(const ImageFormatArray &fmt)
{
  TextureAllocator::handle h;
  h=m_handle.dynamic_cast_handle<TextureAllocator>();
  if(h.valid())
    {
      WRATHAutoLockMutex(h->m_mutex);

      map_type::iterator iter;
      
      iter=h->m_map.find(fmt);
      if(iter!=h->m_map.end())
        {
          return iter->second.m_stats->m_local;
        }
    }
  return texture_consumption_data_type();
}

WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
WRATHImage::TextureAllocatorHandle::
texture_consumption(void)
{
  TextureAllocator::handle h;
  
  h=m_handle.dynamic_cast_handle<TextureAllocator>();
  if(h.valid())
    {
      WRATHAutoLockMutex(h->m_mutex);
      return h->m_total_stats->m_stats;
    }
  return texture_consumption_data_type();
}

void
WRATHImage::TextureAllocatorHandle::
texture_atlas_dimension(uint32_t vx, uint32_t vy) const
{
  TextureAllocator::handle h;
  h=m_handle.dynamic_cast_handle<TextureAllocator>();
  if(h.valid())
    {
      WRATHLockMutex(h->m_mutex);
      h->m_texture_atlas_dimension.x()=vx;
      h->m_texture_atlas_dimension.y()=vy;
      WRATHUnlockMutex(h->m_mutex);
    }
}

vecN<uint32_t, 2>
WRATHImage::TextureAllocatorHandle::
texture_atlas_dimension(void) const
{
  vecN<uint32_t,2> R(0, 0);

  TextureAllocator::handle h;
  h=m_handle.dynamic_cast_handle<TextureAllocator>();
  if(h.valid())
    {
      WRATHLockMutex(h->m_mutex);
      R=h->m_texture_atlas_dimension;
      WRATHUnlockMutex(h->m_mutex);
    }
  return R;
}

void
WRATHImage::TextureAllocatorHandle::
set_clear_bits(const ImageFormatArray &fmt,
               const_c_array<std::vector<uint8_t> > bits) const
{
  TextureAllocator::handle h;
  h=m_handle.dynamic_cast_handle<TextureAllocator>();
  if(h.valid())
    {
      WRATHLockMutex(h->m_mutex);
      h->m_clear_bits[fmt]=std::vector<std::vector<uint8_t> >(bits.begin(), bits.end());
      WRATHUnlockMutex(h->m_mutex);
    }
}


enum return_code
WRATHImage::TextureAllocatorHandle::
allocate_multiple_images_on_same_page(const ImageFormatArray &fmt,
                                      const_c_array<std::pair<ivec2, BoundarySize> > in_sizes,
                                      std::vector<WRATHImage*> &new_images)
{
  std::vector<ivec2> sz(in_sizes.size());

  for(unsigned int i=0, endi=in_sizes.size(); i<endi; ++i)
    {
      ivec2 extra(in_sizes[i].second.m_minX + in_sizes[i].second.m_maxX,
                  in_sizes[i].second.m_maxY + in_sizes[i].second.m_minY);

      sz[i]=in_sizes[i].first+extra;
    }

  if(routine_success==allocate_multiple_images_on_same_page(fmt, sz, new_images))
    {
      for(unsigned int i=0, endi=in_sizes.size(); i<endi; ++i)
        {
          new_images[i]->m_boundary_size=in_sizes[i].second;
          new_images[i]->compute_texture_coordinates();
        }
      return routine_success;
    }

  return routine_fail;
}

enum return_code
WRATHImage::TextureAllocatorHandle::
allocate_multiple_images_on_same_page(const ImageFormatArray &fmt,
                                      const_c_array<ivec2> in_sizes,
                                      const BoundarySize &bd,
                                      std::vector<WRATHImage*> &new_images)
{
  std::vector<ivec2> sz(in_sizes.size());

  for(unsigned int i=0, endi=in_sizes.size(); i<endi; ++i)
    {
      ivec2 extra(bd.m_minX + bd.m_maxX,  bd.m_maxY + bd.m_minY);

      sz[i]=in_sizes[i]+extra;
    }

  if(routine_success==allocate_multiple_images_on_same_page(fmt, sz, new_images))
    {
      for(unsigned int i=0, endi=in_sizes.size(); i<endi; ++i)
        {
          new_images[i]->m_boundary_size=bd;
          new_images[i]->compute_texture_coordinates();
        }
      return routine_success;
    }

  return routine_fail;
}





enum return_code
WRATHImage::TextureAllocatorHandle::
allocate_multiple_images_on_same_page(const ImageFormatArray &fmt,
                                      const_c_array<ivec2> in_sizes,
                                      std::vector<WRATHImage*> &out_images)
{
  TextureAllocator::handle h;

  h=m_handle.dynamic_cast_handle<TextureAllocator>();
  out_images.clear();

  if(h.valid())
    {
      enum return_code R;
      std::list<const WRATHAtlas::rectangle_handle*> rects;

      R=h->allocate_multiple_images_on_same_page(fmt, in_sizes, rects);
      if(R==routine_success)
        {
          out_images.reserve(in_sizes.size());
          
          for(std::list<const WRATHAtlas::rectangle_handle*>::iterator
                iter=rects.begin(), end=rects.end(); iter!=end; ++iter)
            {
              out_images.push_back(WRATHNew WRATHImage(*iter, BoundarySize()));
            }
        }
      return R;
    }
  else
    {
      return routine_fail;
    }
}


//////////////////////////////////////
//WRATHImage::PixelImageFormat methods
int
WRATHImage::PixelImageFormat::
bytes_per_pixel(void) const
{
  int bytes_per_channel(0);

  switch(m_pixel_data_format)
    {
#if defined(UNSIGNED_INT_24_8_OES)
    case UNSIGNED_INT_24_8_OES:
      return 4;
#elif defined(GL_UNSIGNED_INT_24_8)
    case GL_UNSIGNED_INT_24_8:
      return 4;
#endif
    }
  
  /*
    ick, gross, this is how GL implementors feel.
    the values m_pixel_data_format and m_pixel_type
    determine the number of bytes per pixel.
    
    TODO: support OpenGL (desktop) enumerations (shudders).
  */
  switch(m_pixel_type)
    {
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
#if defined(GL_UNSIGNED_SHORT_5_6_5_REV)
    case GL_UNSIGNED_SHORT_5_6_5_REV:
#endif
#if defined(GL_UNSIGNED_SHORT_4_4_4_4_REV)
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
#endif
#if defined(GL_UNSIGNED_SHORT_1_5_5_5_REV)
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
#endif
        return 2;


#if defined(GL_UNSIGNED_BYTE_3_3_2)
    case GL_UNSIGNED_BYTE_3_3_2:
      return 1;
#endif
#if defined(GL_UNSIGNED_BYTE_2_3_3_REV)
    case GL_UNSIGNED_BYTE_2_3_3_REV:
      return 1;
#endif

#if defined(GL_UNSIGNED_INT_8_8_8_8)
    case GL_UNSIGNED_INT_8_8_8_8:
      return 4;
#endif
#if defined(GL_UNSIGNED_INT_8_8_8_8_REV)
    case GL_UNSIGNED_INT_8_8_8_8_REV:
      return 4;
#endif
#if defined(GL_UNSIGNED_INT_10_10_10_2)
    case GL_UNSIGNED_INT_10_10_10_2:
      return 4;
#endif
#if defined(GL_UNSIGNED_INT_2_10_10_10_REV)
    case GL_UNSIGNED_INT_2_10_10_10_REV:
      return 4;
#endif
#if defined(GL_UNSIGNED_INT_24_8)
    case GL_UNSIGNED_INT_24_8:
      return 4;
#endif
#if defined(GL_UNSIGNED_INT_10F_11F_11F_REV)
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
      return 4;
#endif
#if defined(GL_UNSIGNED_INT_5_9_9_9_REV)
    case GL_UNSIGNED_INT_5_9_9_9_REV:
      return 4;
#endif
#if defined(GL_FLOAT_32_UNSIGNED_INT_24_8_REV)
    case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
      return 64;
#endif

        
    case GL_UNSIGNED_BYTE:
    case GL_BYTE:
      bytes_per_channel=1;
      break;
      
      //THIS is CRAZY: the values of GL_HALF_FLOAT_OES and
      //GL_HALF_FLOAT are NOT the same:
      // GL_HALF_FLOAT_OES 0x8D61 (GLES2/glext.2)
      // GL_HALF_FLOAT 0x140B (GL3/gl3.h and GL_HALF_FLOAT_ARB, GL_HALF_FLOAT_NV in GL/glext.h)
#if defined(GL_HALF_FLOAT_OES)
    case GL_HALF_FLOAT_OES:
#endif
#if defined(GL_HALF_FLOAT)
    case GL_HALF_FLOAT:
#endif
    case GL_UNSIGNED_SHORT:
    case GL_SHORT:
      bytes_per_channel=2;
      break;
      
    case GL_FLOAT:
    case GL_UNSIGNED_INT:
    case GL_INT:
#if defined(GL_FIXED)
    case GL_FIXED:
#endif
      bytes_per_channel=4;
      break;
      
    default:
      WRATHwarning("Unknown pixel type: 0x" << std::hex << m_pixel_type);
      bytes_per_channel=1;
    }
  
  switch(m_pixel_data_format)
    {
    case GL_RGBA:
#if defined(GL_RGBA_INTEGER)
    case GL_RGBA_INTEGER:
#elif defined(GL_RGBA_INTEGER_EXT)
    case GL_RGBA_INTEGER_EXT:
#endif

#if defined(GL_BGRA_INTEGER)
    case GL_BGRA_INTEGER:
#elif defined(GL_BGRA_INTEGER_EXT)
    case GL_BGRA_INTEGER_EXT:
#endif
      return 4*bytes_per_channel;
      
    case GL_RGB:
#if defined(GL_RGB_INTEGER)
    case GL_RGB_INTEGER:
#elif defined(GL_RGB_INTEGER_EXT)
    case GL_RGB_INTEGER_EXT:
#endif
      return 3*bytes_per_channel;
      
#if defined(GL_LUMINANCE_ALPHA)
    case GL_LUMINANCE_ALPHA:
#endif
#if defined(GL_RG_INTEGER)
    case GL_RG_INTEGER:
#elif defined(GL_RG_INTEGER_EXT)
    case GL_RG_INTEGER_EXT:
#endif

#if defined(GL_RG)
    case GL_RG:
#endif
      return 2*bytes_per_channel;

#if defined(GL_ALPHA)     
    case GL_ALPHA:
#endif
#if defined(GL_LUMINANCE)    
    case GL_LUMINANCE:
#endif
#if defined(GL_RED)
    case GL_RED:
#endif
#if defined(GL_RED_INTEGER)
    case GL_RED_INTEGER:
#elif defined(GL_RED_INTEGER_EXT)
    case GL_RED_INTEGER_EXT:
#endif
      return bytes_per_channel;
      
    default:
      WRATHwarning("Unknown pixel type: 0x" << std::hex 
                   << m_pixel_data_format);
      return bytes_per_channel;
    }
  
}



///////////////////////////////////////////////////
// WRATHImage::ImageFormat methods
#define EPIC_LAZY(field) do { if(field!=obj.field) return field<obj.field; } while(0)

bool
WRATHImage::ImageFormat::
platform_compare(const ImageFormat &obj) const
{
  EPIC_LAZY(m_internal_format);
  EPIC_LAZY(m_magnification_filter);
  EPIC_LAZY(m_minification_filter);
  EPIC_LAZY(m_automatic_mipmap_generation);

  return false;
}

bool
WRATHImage::ImageFormat::
operator<(const WRATHImage::ImageFormat &obj) const
{
  EPIC_LAZY(m_internal_format);
  EPIC_LAZY(m_pixel_format.m_pixel_data_format);
  EPIC_LAZY(m_pixel_format.m_pixel_type);
  EPIC_LAZY(m_magnification_filter);
  EPIC_LAZY(m_minification_filter);
  EPIC_LAZY(m_automatic_mipmap_generation);
  EPIC_LAZY(m_max_mip_level);
  return false;
}

bool
WRATHImage::ImageFormat::
operator==(const WRATHImage::ImageFormat &obj) const
{
  return m_internal_format==obj.m_internal_format
    and m_pixel_format.m_pixel_data_format==obj.m_pixel_format.m_pixel_data_format
    and m_pixel_format.m_pixel_type==obj.m_pixel_format.m_pixel_type
    and m_magnification_filter==obj.m_magnification_filter
    and m_minification_filter==obj.m_minification_filter
    and m_automatic_mipmap_generation==obj.m_automatic_mipmap_generation
    and m_max_mip_level==obj.m_max_mip_level;
}
bool
WRATHImage::ImageFormat::
platform_equality(const WRATHImage::ImageFormat &obj) const
{
  return m_internal_format==obj.m_internal_format
#if !defined(WRATH_GL_VERSION)
    and m_pixel_format.m_pixel_data_format==obj.m_pixel_format.m_pixel_data_format
    and m_pixel_format.m_pixel_type==obj.m_pixel_format.m_pixel_type
#endif

#if defined(WRATH_GL_TEXTURE_MAX_LEVEL)
    and m_max_mip_level==obj.m_max_mip_level
#endif

    and m_magnification_filter==obj.m_magnification_filter
    and m_minification_filter==obj.m_minification_filter
    and m_automatic_mipmap_generation==obj.m_automatic_mipmap_generation;
}

/////////////////////////////////////////////
// WRATHImage::ImageFormatArray methods
bool
WRATHImage::ImageFormatArray::
platform_compare(const ImageFormatArray &rhs) const
{  
  unsigned int min_size(std::min(size(), rhs.size()));
  
  for(unsigned int i=0; i<min_size; ++i)
    {
      if(!format(i).platform_equality(rhs.format(i)))
        {
          return format(i).platform_compare(rhs.format(i));
        }
    }
  return size()<rhs.size();
}

bool
WRATHImage::ImageFormatArray::
platform_equality(const ImageFormatArray &rhs) const
{
  bool R(size()==rhs.size());
  for(unsigned int i=0, endi=size(); i<endi and R; ++i)
    {
      R=(format(i)==rhs.format(i));
    }
  return R;
}

////////////////////////////////
//WRATHImage methods
WRATH_RESOURCE_MANAGER_IMPLEMENT(WRATHImage, WRATHImage::WRATHImageID);

WRATHImage::
WRATHImage(const WRATHAtlas::rectangle_handle *rect,
           const BoundarySize &bd):
  m_boundary_size(bd),
  m_location(rect)
{
  compute_texture_coordinates();
}

WRATHImage::
WRATHImage(const WRATHImage::WRATHImageID &pname, 
           const ivec2 &sz, const ImageFormatArray &fmt,
           enum WRATHImage::UniquePixelStoreTag,
           GLenum texture_wrap_mode_s,
           GLenum texture_wrap_mode_t):
  m_boundary_size(),
  m_location(NULL),
  m_name(pname),
  m_on_manager(true)
{
  resource_manager().add_resource(pname, this);
  init(sz, fmt, texture_wrap_mode_s, texture_wrap_mode_t);
}

WRATHImage::
WRATHImage(const ivec2 &sz, const ImageFormatArray &fmt,
           enum WRATHImage::UniquePixelStoreTag,
           GLenum texture_wrap_mode_s,
           GLenum texture_wrap_mode_t):
  m_boundary_size(),
  m_location(NULL),
  m_on_manager(false)
{
  init(sz, fmt, texture_wrap_mode_s, texture_wrap_mode_t);
}


void
WRATHImage::
init(const ivec2 &sz, const ImageFormatArray &fmt,
     GLenum texture_wrap_mode_s,
     GLenum texture_wrap_mode_t)
{
  if(image_size_valid(sz))
    {
      GLPixelStore *pix;

      pix=WRATHNew GLPixelStore(fmt, sz,
                                texture_wrap_mode_s,
                                texture_wrap_mode_t);

      m_location=pix->m_atlas->add_rectangle(sz);
      WRATHassert(m_location!=NULL);

      WRATHassert(m_location->minX_minY()==ivec2(0,0));
      WRATHassert(m_location->size()==sz);

      compute_texture_coordinates();
    }
}

WRATHImage::
WRATHImage(const WRATHImage::WRATHImageID &pname, 
           const WRATHImage::ImageFormat &im,
           GLuint tex_name, const ivec2 &bl, const ivec2 &sz):
  m_boundary_size(),
  m_name(pname),
  m_on_manager(true)
{
  resource_manager().add_resource(pname, this);
  init(im, tex_name, bl, sz);
}

WRATHImage::
WRATHImage(const WRATHImage::ImageFormat &im,
           GLuint tex_name, const ivec2 &bl, const ivec2 &sz):
  m_boundary_size(),
  m_on_manager(false)
{
  init(im, tex_name, bl, sz);
}



void
WRATHImage::
init(const WRATHImage::ImageFormat &im,
     GLuint tex_name, const ivec2 &bl, const ivec2 &sz)
{
  GLPixelStore *pix;

  pix=WRATHNew GLPixelStore(im, sz+bl, tex_name);
  m_location=pix->m_atlas->add_rectangle(sz);
  compute_texture_coordinates();
}


WRATHImage::
WRATHImage(const WRATHImage::WRATHImageID &pname, 
           const ivec2 &sz, const ImageFormatArray &fmt,
           const BoundarySize &pboundary_size,
           const TextureAllocatorHandle &tex_allocator):
  m_boundary_size(pboundary_size),
  m_location(NULL),
  m_name(pname),
  m_on_manager(true)
{
  resource_manager().add_resource(pname, this);
  init(sz, fmt, tex_allocator);
}

WRATHImage::
WRATHImage(const ivec2 &sz, const ImageFormatArray &fmt,
           const BoundarySize &pboundary_size,
           const TextureAllocatorHandle &tex_allocator):
  m_boundary_size(pboundary_size),
  m_location(NULL),
  m_on_manager(false)
{
  init(sz, fmt, tex_allocator);
}




void
WRATHImage::
init(const ivec2 &in_sz, const ImageFormatArray &fmt,
     const TextureAllocatorHandle &ptex_allocator)
{
  
  TextureAllocator::handle tex_allocator;
  tex_allocator=ptex_allocator.m_handle.dynamic_cast_handle<TextureAllocator>();

  WRATHassert(ptex_allocator.valid());
  WRATHassert(tex_allocator.valid());

  ivec2 sz(in_sz.x()+m_boundary_size.m_minX+m_boundary_size.m_maxX,
           in_sz.y()+m_boundary_size.m_minY+m_boundary_size.m_maxY);

  if(ptex_allocator.valid() and ptex_allocator.image_size_valid(sz))
    {
      m_location=tex_allocator->allocate(fmt, sz);
      if(m_location!=NULL)
        {
          compute_texture_coordinates();
        }
    }
}

WRATHImage::
~WRATHImage()
{
  m_dtor_signal();

  if(m_on_manager)
    {
      resource_manager().remove_resource(this);
    }

  if(m_location!=NULL)
    {
      WRATHAtlasBase::delete_rectangle(m_location);
    }
}


void
WRATHImage::
register_image(const WRATHImageID &pid)
{
  if(m_on_manager)
    {
      resource_manager().remove_resource(this);
    }

  m_on_manager=true;
  m_name=pid;
  resource_manager().add_resource(m_name, this);
}


const WRATHImage::ImageFormatArray&
WRATHImage::
image_format(void) const
{
  WRATHassert(valid());
  if(valid())
    {      
      GLPixelStore *pixel_store;
      pixel_store=dynamic_cast<GLPixelStore*>(m_location->atlas()->pixelstore());
      
      WRATHassert(pixel_store!=NULL);
      return pixel_store->m_format;
    }
  else
    {
      WRATHStaticInit();
      static ImageFormatArray fmt;
      return fmt;
    }
}


const_c_array<GLuint>
WRATHImage::
texture_atlas_glnames(void) const
{
  WRATHassert(valid());
  if(valid())
    {      
      GLPixelStore *pixel_store;
      pixel_store=dynamic_cast<GLPixelStore*>(m_location->atlas()->pixelstore());
      
      WRATHassert(pixel_store!=NULL);
      return pixel_store->m_texture;
    }
  else
    {
      return const_c_array<GLuint>();
    }
}

GLuint
WRATHImage::
texture_atlas_glname(unsigned int layer) const
{
  const_c_array<GLuint> R;
  R=texture_atlas_glnames();

  return (R.size()>layer)?
    R[layer]:
    0;
}


ivec2
WRATHImage::
atlas_size(void) const
{
  WRATHassert(valid());
  if(valid())
    {
      GLPixelStore *pixel_store;
      pixel_store=dynamic_cast<GLPixelStore*>(m_location->atlas()->pixelstore());
      
      WRATHassert(pixel_store!=NULL);
      return pixel_store->m_size;
    }
  else
    {
      return ivec2(0,0);
    }
}

void
WRATHImage::
clear(void)
{
  clear_implement(minX_minY_boundary(), size_including_boundary());
}

void
WRATHImage::
clear(ivec2 min_corner, ivec2 psize)
{
  if(!valid())
    {
      return;
    }

  ivec2 delta;

  /*
    if the min corner goes beyond the range of the WRATHImage,
    we need to adjust it and the passed size.
   */
  delta.x()=std::min(min_corner.x()+boundary_size().m_minX, 0);
  delta.y()=std::min(min_corner.y()+boundary_size().m_minY, 0);
  psize+=delta;
  min_corner-=delta;


  psize.x()=std::min(size_including_boundary().x(), psize.x());
  psize.y()=std::min(size_including_boundary().y(), psize.y());
  min_corner+=minX_minY_boundary();

  if(psize.x()<=0 or psize.y()<=0)
    {
      return;
    }

  clear_implement(min_corner, psize);
}

void
WRATHImage::
clear_sub_image(const ImageFormatArray &fmt,
                const_c_array<std::vector<uint8_t> > bits,
                ivec2 min_corner, 
                ivec2 psize)
{
  if(!valid())
    {
      return;
    }

  ivec2 delta;

  /*
    if the min corner goes beyond the range of the WRATHImage,
    we need to adjust it and the passed size.
   */
  delta.x()=std::min(min_corner.x()+boundary_size().m_minX, 0);
  delta.y()=std::min(min_corner.y()+boundary_size().m_minY, 0);
  psize+=delta;
  min_corner-=delta;


  psize.x()=std::min(size_including_boundary().x(), psize.x());
  psize.y()=std::min(size_including_boundary().y(), psize.y());
  min_corner+=minX_minY_boundary();

  if(psize.x()<=0 or psize.y()<=0)
    {
      return;
    }

  GLPixelStore *pixel_store;

  pixel_store=dynamic_cast<GLPixelStore*>(m_location->atlas()->pixelstore());
  WRATHassert(pixel_store!=NULL);
  
  pixel_store->add_clear_command(min_corner, psize, 
                                 fmt, bits);

}

void
WRATHImage::
clear_implement(ivec2 min_corner, ivec2 psize)
{
  GLPixelStore *pixel_store;

  pixel_store=dynamic_cast<GLPixelStore*>(m_location->atlas()->pixelstore());
  WRATHassert(pixel_store!=NULL);
  if(pixel_store->m_h.valid())
    {
      WRATHAutoLockMutex(pixel_store->m_h->m_mutex);

      clear_map_type::iterator iter;
      iter=pixel_store->m_h->m_clear_bits.find(pixel_store->m_format);
      if(iter!=pixel_store->m_h->m_clear_bits.end())
        {
          pixel_store->add_clear_command(min_corner, psize, iter->second);
          return;
        }
    }
  pixel_store->add_clear_command(min_corner, psize);
}


void
WRATHImage::
respecify_sub_image(int layer, int LOD,
                    const WRATHImage::PixelImageFormat &fmt,
                    std::vector<uint8_t> &raw_pixels, 
                    ivec2 min_corner, 
                    ivec2 psize, 
                    int alignment)
{
  

  WRATHassert(valid());
  if(!valid() or psize.x()<=0 or psize.y()<=0)
    {
      return;
    }

  const ImageFormat &im_fmt(image_format()[layer]);


  WRATHassert(LOD>=0);
  WRATHassert(LOD==0 or !im_fmt.m_automatic_mipmap_generation);

  BoundarySize bdlod(boundary_size(), LOD);
  ivec2 bllod(minX_minY(LOD));
  ivec2 szlod(size(LOD));

  WRATHassert(min_corner.x()+bdlod.m_minX>=0);
  WRATHassert(min_corner.y()+bdlod.m_minY>=0);  

  /*
    We allow a slack of 1 pixel to the maxX and above
    the image boundary LOD>0.
   */
  WRATHassert(min_corner.x()+psize.x()<=szlod.x()+bdlod.m_maxX + (LOD>0?1:0) );
  WRATHassert(min_corner.y()+psize.y()<=szlod.y()+bdlod.m_maxY + (LOD>0?1:0) );

  GLPixelStore *pixel_store;
  pixel_store=dynamic_cast<GLPixelStore*>(m_location->atlas()->pixelstore());

  WRATHassert(pixel_store!=NULL);
  WRATHassert(LOD==0 or pixel_store->m_has_mipmaps[layer]);


  WRATHassert(min_corner.x()+psize.x()+bllod.x() <= 1+(pixel_store->m_size.x()>>LOD) );
  WRATHassert(min_corner.y()+psize.y()+bllod.y() <= 1+(pixel_store->m_size.y()>>LOD) );

  psize.x()=std::min(psize.x(), 
                     (pixel_store->m_size.x()>>LOD)
                     - min_corner.x() - bllod.x());

  psize.y()=std::min(psize.y(), 
                     (pixel_store->m_size.y()>>LOD)
                     - min_corner.y() - bllod.y());

  
  if(psize.x()<=0 or psize.y()<=0)
    {
      return;
    }

  int bpp(fmt.bytes_per_pixel());

  WRATHassert(static_cast<unsigned int>(psize.x()*psize.y()*bpp)<=raw_pixels.size());

  WRATHLockMutex(pixel_store->m_mutex);
  
  pixel_store->m_deffered_uploads[layer].push_back(TexSubImageCommand());

  TexSubImageCommand &cmd(pixel_store->m_deffered_uploads[layer].back());  
  cmd.m_LOD=LOD;
  cmd.m_place=min_corner+bllod;
  cmd.m_size=psize;
  cmd.m_pixel_data_format=fmt.m_pixel_data_format;
  cmd.m_pixel_type=fmt.m_pixel_type;
  cmd.m_update_mips=im_fmt.m_automatic_mipmap_generation 
    and pixel_store->m_has_mipmaps[layer];
  

  if(alignment<=bpp)
    {
      if(WRATHUtil::is_power_of_2(bpp))
        {
          /*
            out of paranoid we do not use 8..
           */
          cmd.m_alignment=std::min(4, bpp);
        }
      else
        {
          cmd.m_alignment=1;
        }
    }
  else
    {
      WRATHassert(alignment==1 or alignment==2 or alignment==4 or alignment==8);
      cmd.m_alignment=alignment;
    }
  


  std::swap(raw_pixels, cmd.m_pixels);

  WRATHUnlockMutex(pixel_store->m_mutex);
    

}







const_c_array<WRATHTextureChoice::texture_base::handle>
WRATHImage::
texture_binders(void) const
{
  WRATHassert(valid());
  if(valid())
    {
      GLPixelStore *pixel_store;
      pixel_store=dynamic_cast<GLPixelStore*>(m_location->atlas()->pixelstore());
      
      WRATHassert(pixel_store!=NULL);
      return pixel_store->m_texture_binder;
    }
  else
    {
      return const_c_array<WRATHTextureChoice::texture_base::handle>();
    }
}

WRATHTextureChoice::texture_base::handle
WRATHImage::
texture_binder(unsigned int layer) const
{
  const_c_array<WRATHTextureChoice::texture_base::handle> R;
  R=texture_binders();

  return (R.size()>layer)?
    R[layer]:
    WRATHTextureChoice::texture_base::handle();
 
}


void
WRATHImage::
compute_texture_coordinates(void)
{
  m_minX_minY_texture_coordinate[0]=compute_minX_minY_texture_coordinate(true);
  m_minX_minY_texture_coordinate[1]=compute_minX_minY_texture_coordinate(false);

  m_maxX_maxY_texture_coordinate[0]=compute_maxX_maxY_texture_coordinate(true);
  m_maxX_maxY_texture_coordinate[1]=compute_maxX_maxY_texture_coordinate(false);

  m_size_texture_coordinate=m_maxX_maxY_texture_coordinate-m_minX_minY_texture_coordinate;
}

vec2
WRATHImage::
compute_minX_minY_texture_coordinate(bool add_central_offset) 
{
  WRATHassert(valid());
  if(valid())
    {
      vec2 raw(minX_minY().x(), minX_minY().y());
      GLPixelStore *pixel_store;
      pixel_store=dynamic_cast<GLPixelStore*>(m_location->atlas()->pixelstore());
      
      WRATHassert(pixel_store!=NULL);
      if(add_central_offset)
        {
          raw+=vec2(0.5f, 0.5f);
        }

      return raw
        / vec2( pixel_store->m_size.x(), 
                pixel_store->m_size.y());
    }
  else
    {
      return vec2(0,0);
    }
}

vec2
WRATHImage::
compute_maxX_maxY_texture_coordinate(bool add_central_offset) 
{
  WRATHassert(valid());
  if(valid())
    {
      vec2 raw(minX_minY().x()+size().x(), minX_minY().y()+size().y());
      GLPixelStore *pixel_store;
      pixel_store=dynamic_cast<GLPixelStore*>(m_location->atlas()->pixelstore());
      
      WRATHassert(pixel_store!=NULL);
      
      if(add_central_offset)
        {
          raw-=vec2(0.5f, 0.5f);
        }
      
      return raw
        / vec2( pixel_store->m_size.x(), 
                pixel_store->m_size.y());
    }
  else
    {
      return vec2(0,0);
    }
}

void
WRATHImage::
bind_texture_to_fbo(GLenum attachment, unsigned int layer) const
{
  /*
    TODO: add support for GL_TEXTURE_2D_ARRAY
   */
  WRATHassert(valid());
  if(valid())
    {
      glFramebufferTexture2D(GL_FRAMEBUFFER, attachment,
                             GL_TEXTURE_2D, texture_atlas_glname(layer), 0);
    }
}

void
WRATHImage::
copy_from_framebuffer(ivec2 location, 
                      ivec2 screen_location,
                      ivec2 width_height,
                      unsigned int layer)
{
  /*
    adjust location so that it is within 
    the WRATHImage:
   */
  ivec2 out_of_bounds;

  out_of_bounds.x()=std::min(0, boundary_size().m_minX+location.x());
  out_of_bounds.y()=std::min(0, boundary_size().m_minY+location.y());

  location-=out_of_bounds;
  width_height-=out_of_bounds;

  ivec2 dims, image_dims(size_including_boundary()-location);
  ivec2 tex_loc(minX_minY_boundary()+location);


  dims.x()=std::min(width_height.x(), image_dims.x());
  dims.y()=std::min(width_height.y(), image_dims.y());

  texture_binder(layer)->bind_texture(WRATHglGet<GLint>(GL_ACTIVE_TEXTURE));
  glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                      tex_loc.x(), tex_loc.y(),
                      screen_location.x(), screen_location.y(),
                      dims.x(), dims.y());

}


const WRATHImage::TextureAllocatorHandle&
WRATHImage::
default_texture_allocator(void)
{
  WRATHStaticInit();
  static TextureAllocator::handle R(WRATHNew TextureAllocator(false, 
                                                              ivec2(2048, 2048),
                                                              GL_CLAMP_TO_EDGE,
                                                              GL_CLAMP_TO_EDGE));
  static TextureAllocatorHandle ret(R);
  return ret;
}

WRATHImage::TextureAllocatorHandle
WRATHImage::
create_texture_allocator(bool memeset_zero_texture_data,
                         ivec2 dim,
                         GLenum texture_wrap_mode_s,
                         GLenum texture_wrap_mode_t)
{
  TextureAllocator::handle R;

  R=WRATHNew TextureAllocator(memeset_zero_texture_data, dim,
                              texture_wrap_mode_s, texture_wrap_mode_t);

  return TextureAllocatorHandle(R);
}
