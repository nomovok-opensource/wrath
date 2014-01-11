/*! 
 * \file WRATHTextureFontFreeType_Analytic.cpp
 * \brief file WRATHTextureFontFreeType_Analytic.cpp
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
#include <sstream>
#include <limits>
#include <boost/algorithm/string/predicate.hpp>
#include <fstream>
#include <iomanip>
#include <boost/multi_array.hpp>
#include <sys/time.h>
#include "WRATHTextureFontFreeType_Analytic.hpp"
#include "WRATHFreeTypeSupport.hpp"
#include "WRATHUtil.hpp"
#include "WRATHStaticInit.hpp"
#include "ostream_utility.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H




using namespace WRATHFreeTypeSupport;

namespace
{
  class CubicsToQuadraticsEmitter:
    public WRATHFreeTypeSupport::ContourEmitterBase
  {
  public:
    CubicsToQuadraticsEmitter(const FT_Outline &outline, int pscale_factor):
      m_worker(outline, pscale_factor)
    {
      
    }

    virtual
    void
    produce_contours(geometry_data data)
    {
      consumer_state S(data, this);
      m_worker.produce_contours(data);
    }

  private:

    class consumer_state
    {
    public:
      consumer_state(geometry_data d, 
                     CubicsToQuadraticsEmitter *master):
        m_data(d),
        m_master(master)
      {
        signal_emit_curve::slot_type C(boost::bind(&CubicsToQuadraticsEmitter::consumer_state::consume_curve,
                                                   this, _1));
        signal_end_contour::slot_type O(boost::bind(&CubicsToQuadraticsEmitter::consumer_state::consume_contour,
                                                    this));
        
        m_consume_curves=m_master->m_worker.connect_emit_curve(C);
        m_consume_contours=m_master->m_worker.connect_emit_end_contour(O);
      }

      ~consumer_state()
      {
        m_consume_curves.disconnect();
        m_consume_contours.disconnect();
      }
      
    private:
      void
      consume_contour(void)
      {
        m_master->emit_end_contour();
      }
      
      void
      consume_curve(WRATHFreeTypeSupport::BezierCurve *curve)
      {
        if(curve->degree()==3) 
          {
            vecN<WRATHFreeTypeSupport::BezierCurve*, 4> quads_4;
            
            curve->approximate_cubic(m_data, quads_4);
            for(int i=0; i<4; ++i)
              {
                m_master->emit_curve(quads_4[i]);
              }
            
            WRATHDelete(curve);
          }
        else
          {
            m_master->emit_curve(curve);
          }
      }
      
      geometry_data m_data;
      CubicsToQuadraticsEmitter *m_master;
      boost::signals2::connection m_consume_curves;
      boost::signals2::connection m_consume_contours;    
    };

    WRATHFreeTypeSupport::ContourEmitterFromFT_Outline m_worker;
  };
  

  unsigned int
  compute_num_levels_needed(const ivec2 &glyph_size,
                            int mipmap_levels)
  {
    if(mipmap_levels==0)
      {
        return 1;
      }

    unsigned int return_value;
    for(return_value=0; 
        (glyph_size.x()>>return_value)>0 and (glyph_size.y()>>return_value)>0;
        ++return_value)
      {}


    return std::max(1u, return_value);
  }
   

  template<unsigned int N>
  vecN<uint8_t, N>
  pack_from_minus_one_plus_one(const vecN<float,N> &v)
  {
    vecN<uint8_t, N> R;
    for(unsigned int i=0;i<N;++i)
      {
        int int_value;

        //NOTE that we pack to [0,254] this is so that
        //the value 0 is represented!
        int_value= static_cast<int>( 254.0f*0.5f*(v[i] + 1.0f) );
        int_value=std::max(0, int_value);
        int_value=std::min(254, int_value);
        R[i]=static_cast<uint8_t>(int_value);
      }
    return R;
  }

  template<unsigned int N>
  vecN<float,N>
  unpack_from_minus_one_plus_one(vecN<uint8_t, N> v)
  {
    vecN<float,N> return_value(v);

    return_value/=(254.0f*0.5f);
    return_value+=-1.0f;
    return return_value;
  }

  void
  calculate_line_segment_data(const vec2 &p0,
                              const vec2 &p1,
                              vec2 &n, float &o)
  {
    vec2 v;
    v=vec2( p1.y()-p0.y(), p0.x()-p1.x());
    n= v/std::max( std::abs(v.x()), std::abs(v.y()));
    o=dot(p0,n);
  }

  GLenum
  teximage_internal_format(enum WRATHTextureFontFreeType_Analytic::texture_mode_type fmt)
  {
    WRATHassert(fmt<3);
    /*
      GLES2 just plane sucks. The internal format
      of a texture is NOT determined by the 3rd
      argument of glTexImage2D, rather it is determined
      by that and the pixel type.
    */
    #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
    {
      const GLenum values[3]=
        {
          /*[local_pixel_coordinates]=*/ GL_RGBA,
          /*[global_pixel_coordinates_16bit]=*/ GL_LUMINANCE_ALPHA,
          /*[global_pixel_coordinates_32bit]=*/ GL_LUMINANCE_ALPHA
        };
      return values[fmt];
    }
    #else
    {
      const GLenum values[3]=
        {
          /*[local_pixel_coordinates]=*/ GL_RGBA8,
          /*[global_pixel_coordinates_16bit]=*/ GL_RG16F,
          /*[global_pixel_coordinates_32bit]=*/ GL_RG32F
        };
      return values[fmt];
    }
    #endif
    
  }
  
  GLenum
  teximage_external_format(enum WRATHTextureFontFreeType_Analytic::texture_mode_type fmt)
  {
    WRATHassert(fmt<3);

    #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
    {
      const GLenum values[3]=
        {
          /*[local_pixel_coordinates]=*/ GL_RGBA,
          /*[global_pixel_coordinates_16bit]=*/ GL_LUMINANCE_ALPHA,
          /*[global_pixel_coordinates_32bit]=*/ GL_LUMINANCE_ALPHA
        };
      return values[fmt];
    }
    #else
    {
      const GLenum values[3]=
        {
          /*[local_pixel_coordinates]=*/ GL_RGBA,
          /*[global_pixel_coordinates_16bit]=*/ GL_RG,
          /*[global_pixel_coordinates_32bit]=*/ GL_RG
        };
      return values[fmt];
    }
    #endif
  }
  
  
  GLenum
  teximage_pixel_type(enum WRATHTextureFontFreeType_Analytic::texture_mode_type fmt)
  {
    WRATHassert(fmt<3);
    #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
    {
      const GLenum values[3]=
        {
          /*[local_pixel_coordinates]=*/ GL_UNSIGNED_BYTE,
          /*[global_pixel_coordinates_16bit]=*/ GL_HALF_FLOAT_OES,
          /*[global_pixel_coordinates_32bit]=*/ GL_FLOAT
        };
      return values[fmt];
    }
    #else
    {
      const GLenum values[3]=
        {
          /*[local_pixel_coordinates]=*/ GL_UNSIGNED_BYTE,
          /*[global_pixel_coordinates_16bit]=*/ GL_HALF_FLOAT_ARB,
          GL_FLOAT
        };
      return values[fmt];
    }
    #endif
  }
  
  
       
  class common_analytic_texture_data:boost::noncopyable
  {
  public:
    common_analytic_texture_data(void);
    ~common_analytic_texture_data();


    const WRATHTextureFont::GlyphGLSL*
    glyph_glsl(enum WRATHTextureFontFreeType_Analytic::texture_mode_type,
                    unsigned int);

    WRATHMutex m_mutex;
    WRATHImage::TextureAllocatorHandle m_allocator;
    enum WRATHTextureFontFreeType_Analytic::texture_mode_type m_creation_mode;
    bool m_generate_sub_quads;
    GLint m_texture_creation_size;
    unsigned int m_mipmap_level;

  private:
    
    std::vector<WRATHTextureFont::GlyphGLSL*> m_relative_glyph_glsl;
    WRATHTextureFont::GlyphGLSL m_glyph_glsl;
  };

  common_analytic_texture_data&
  common_data(void)
  {
    WRATHStaticInit();
    static common_analytic_texture_data R;
    return R;
  }

  class local_glyph_data:public WRATHTextureFont::glyph_data_type
  {
  public:
    explicit
    local_glyph_data(WRATHImage *pImage):
      m_image(pImage)
    {}

    ~local_glyph_data()
    {
      WRATHDelete(m_image);
    }

    WRATHImage *m_image;
  };

}

//////////////////////////////////////////
// common_analytic_texture_data methods
common_analytic_texture_data::
common_analytic_texture_data(void):
  m_creation_mode(WRATHTextureFontFreeType_Analytic::local_pixel_coordinates),
  m_generate_sub_quads(false),
  m_texture_creation_size(1024),
  m_mipmap_level(0)
{
  m_allocator=WRATHImage::create_texture_allocator(true, 1024,
                                                   GL_CLAMP_TO_EDGE,
                                                   GL_CLAMP_TO_EDGE);
  /*
    specify the clear values for the 3 types of formats
    analytic fonts use.
  */
  WRATHImage::ImageFormatArray fmt;
  vecN< std::vector<uint8_t>, 4> values;
  vecN<uint8_t, 4> v;
  vec2 offsets(10000.0f, 10000.0f);
  
  values[0].resize(4);
  v=pack_from_minus_one_plus_one( vec4(0.0f, 0.0f, 0.0f, 0.0f));
  std::copy(v.begin(), v.end(), values[0].begin());
  
  values[1].resize(4);
  values[1][0]=255;
  values[1][1]=255;
  values[1][2]=0;
  values[1][3]=0;
  
  values[2].resize(4);
  WRATHUtil::convert_to_halfp_from_float(values[2], offsets);
  
  values[3].resize(8);
  memcpy(&values[3][0], &offsets[0], 8);
  
  fmt.format(0, WRATHImage::ImageFormat()
             .pixel_data_format(GL_RGBA)
             .pixel_type(GL_UNSIGNED_BYTE)
             .internal_format(GL_RGBA)
             .magnification_filter(GL_NEAREST)
             .minification_filter(GL_NEAREST)
             .automatic_mipmap_generation(false));
  
  for(int i=0;i<3; ++i)
    {
      enum WRATHTextureFontFreeType_Analytic::texture_mode_type tp;
      tp=static_cast<enum WRATHTextureFontFreeType_Analytic::texture_mode_type>(i);
      
      fmt.format(1, WRATHImage::ImageFormat()
                 .pixel_data_format(teximage_external_format(tp))
                 .pixel_type(teximage_pixel_type(tp))
                 .internal_format(teximage_internal_format(tp))
                 .magnification_filter(GL_NEAREST)
                 .minification_filter(GL_NEAREST)
                 .automatic_mipmap_generation(false));
      
      
      m_allocator.set_clear_bits(fmt, 
                                 vecN<std::vector<uint8_t>, 2>(values[0], 
                                                               values[1+i]));
    }

  /*
    only GLES2 requires the LA lookup.
   */
  #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
  {  
    for(unsigned int i=0; i<WRATHTextureFont::GlyphGLSL::num_linearity_types; ++i)
      {
        m_glyph_glsl.m_fragment_processor[i].add_macro("WRATH_FONT_USE_LA_LOOKUP");
      }
  }
  #endif


  

  m_glyph_glsl.m_vertex_processor[WRATHTextureFont::GlyphGLSL::linear_glyph_position]
    .add_source("font_analytic_linear.vert.wrath-shader.glsl",
                WRATHGLShader::from_resource);

  m_glyph_glsl.m_fragment_processor[WRATHTextureFont::GlyphGLSL::linear_glyph_position]
    .add_source("font_analytic_base.frag.wrath-shader.glsl", WRATHGLShader::from_resource)
    .add_source("font_analytic_linear.frag.wrath-shader.glsl", WRATHGLShader::from_resource);

  m_glyph_glsl.m_vertex_processor[WRATHTextureFont::GlyphGLSL::nonlinear_glyph_position]
    .add_source("font_analytic_nonlinear.vert.wrath-shader.glsl",
                WRATHGLShader::from_resource);

  m_glyph_glsl.m_fragment_processor[WRATHTextureFont::GlyphGLSL::nonlinear_glyph_position]
    .add_source("font_analytic_base.frag.wrath-shader.glsl", WRATHGLShader::from_resource)
    .add_source("font_analytic_nonlinear.frag.wrath-shader.glsl", WRATHGLShader::from_resource);

  
  #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
  {  
    for(unsigned int i=0; i<WRATHTextureFont::GlyphGLSL::num_linearity_types; ++i)
      {
        m_glyph_glsl.m_fragment_processor[i].remove_macro("WRATH_FONT_USE_LA_LOOKUP");
      }
  }
  #endif

  m_glyph_glsl.m_sampler_names.push_back("wrath_AnalyticNormalTexture");
  m_glyph_glsl.m_sampler_names.push_back("wrath_AnalyticPositionTexture");
  m_glyph_glsl.m_global_names.push_back("wrath_analytic_font_compute_distance");
  m_glyph_glsl.m_global_names.push_back("wrath_AnalyticTexCoord_Position");
  m_glyph_glsl.m_global_names.push_back("wrath_AnalyticBottomLeft");
}

common_analytic_texture_data::
~common_analytic_texture_data()
{
  for(int i=0, endi=m_relative_glyph_glsl.size(); i<endi; ++i)
    {
      WRATHDelete(m_relative_glyph_glsl[i]);
    }
}

const WRATHTextureFont::GlyphGLSL*
common_analytic_texture_data::
glyph_glsl(enum WRATHTextureFontFreeType_Analytic::texture_mode_type pmode,
           unsigned int pmipmap_levels)
{
  WRATHAutoLockMutex(m_mutex);
  if(pmode!=WRATHTextureFontFreeType_Analytic::local_pixel_coordinates)
    {
      return &m_glyph_glsl;
    }

  if(pmipmap_levels<m_relative_glyph_glsl.size())
    {
      return m_relative_glyph_glsl[pmipmap_levels];
    }

  unsigned int old_sz(m_relative_glyph_glsl.size());

  m_relative_glyph_glsl.resize(pmipmap_levels+1);
  for(;old_sz<=pmipmap_levels; ++old_sz)
    {
      m_relative_glyph_glsl[old_sz]=WRATHNew WRATHTextureFont::GlyphGLSL();

      m_relative_glyph_glsl[old_sz]->m_sampler_names=m_glyph_glsl.m_sampler_names;
      m_relative_glyph_glsl[old_sz]->m_global_names=m_glyph_glsl.m_global_names;

      std::ostringstream ostr;
      ostr << (1<<old_sz) << ".0";

      for(unsigned int i=0; i<WRATHTextureFont::GlyphGLSL::num_linearity_types; ++i)
        {
          m_relative_glyph_glsl[old_sz]->m_fragment_processor[i]
            .add_macro("WRATH_FONT_ANALYTIC_PIXEL_RELATIVE_COORDINATES")
            .add_macro("WRATH_FONT_ANALYTIC_MAX_GLYPH_NORMALIZED_SIZE", ostr.str())
            .absorb(m_glyph_glsl.m_fragment_processor[i])
            .remove_macro("WRATH_FONT_ANALYTIC_MAX_GLYPH_NORMALIZED_SIZE")
            .remove_macro("WRATH_FONT_ANALYTIC_PIXEL_RELATIVE_COORDINATES");
          
          m_relative_glyph_glsl[old_sz]->m_vertex_processor[i]
            .absorb(m_glyph_glsl.m_vertex_processor[i]);
        }
    }

  return m_relative_glyph_glsl[pmipmap_levels];

}

  
///////////////////////////////////////////////
// WRATHTextureFontFreeType_Analytic methods
WRATHTextureFontFreeType_Analytic::
WRATHTextureFontFreeType_Analytic(WRATHFreeTypeSupport::LockableFace::handle pface, 
                                  const WRATHTextureFontKey &presource_name):
  WRATHTextureFontFreeTypeT<WRATHTextureFontFreeType_Analytic>(pface, presource_name),
  m_generate_sub_quads(generate_sub_quads()),
  m_mipmap_level(mipmap_level()),
  m_texture_mode(creation_texture_mode()),
  m_bytes_per_pixel(4, m_texture_mode==global_pixel_coordinates_32bit?8:4)
{
  ctor_init();
}

void
WRATHTextureFontFreeType_Analytic::
ctor_init(void)
{
  WRATHFreeTypeSupport::LockableFace::handle F;

  F=ttf_face();
  WRATHassert(F.valid());
  WRATHassert(F->face()!=NULL);
  WRATHassert((F->face()->face_flags&FT_FACE_FLAG_SCALABLE)!=0); 

  /*
    set pixel size and transform for lifetime of font.
   */
  FT_Set_Pixel_Sizes(F->face(), pixel_size(), pixel_size());
  FT_Set_Transform(F->face(), NULL, NULL);
  
  /*
    initialized m_format:
   */
  m_format
    .format(0, WRATHImage::ImageFormat()
            .pixel_data_format(GL_RGBA)
            .pixel_type(GL_UNSIGNED_BYTE)
            .internal_format(GL_RGBA)
            .magnification_filter(GL_NEAREST)
            .minification_filter(GL_NEAREST)
            .automatic_mipmap_generation(false))
    .format(1, 
            WRATHImage::ImageFormat()
            .pixel_data_format(teximage_external_format(m_texture_mode))
            .pixel_type(teximage_pixel_type(m_texture_mode))
            .internal_format(teximage_internal_format(m_texture_mode))
            .magnification_filter(GL_NEAREST)
            .minification_filter(GL_NEAREST)
            .automatic_mipmap_generation(false));

  if(m_mipmap_level>0)
    {
      m_format[0]
        .minification_filter(GL_NEAREST_MIPMAP_NEAREST)
        .max_mip_level(m_mipmap_level);

      m_format[1]
        .minification_filter(GL_NEAREST_MIPMAP_NEAREST)
        .max_mip_level(m_mipmap_level);
    }

  m_pow2_mipmap_level=static_cast<float>(1 << m_mipmap_level);
  
  std::string file_extension(WRATHUtil::filename_extension(simple_name()));
  m_is_ttf=boost::iequals(file_extension, "ttf");
}

WRATHTextureFontFreeType_Analytic::
~WRATHTextureFontFreeType_Analytic()
{

#if defined(WRATH_FONT_GENERATION_STATS)  
  /*
    I want to know how long it took to 
    generate the glyphs on average
   */
  std::cout << "[Analytic]" << simple_name() << " "
            << glyph_data_stats()
            << " spread across " 
            << m_page_tracker.number_texture_pages()
            << " pages\n";
#endif
  
}

WRATHImage*
WRATHTextureFontFreeType_Analytic::
allocate_glyph(std::vector< vecN<std::vector<uint8_t>, number_textures_per_page> > &analytic_pixel_data,
               const ivec2 &glyph_size)
{
  WRATHImage *pImage;

  pImage=WRATHNew WRATHImage(glyph_size, m_format,
                             WRATHImage::BoundarySize(),
                             common_data().m_allocator);

  for(unsigned int LOD=0; LOD<analytic_pixel_data.size(); ++LOD)
    {
      for(int layer=0; layer<number_textures_per_page; ++layer)
        {
          pImage->respecify_sub_image(layer,
                                      LOD, //LOD
                                      m_format[layer].m_pixel_format,
                                      analytic_pixel_data[LOD][layer],
                                      ivec2(0,0),
                                      ivec2(glyph_size.x()>>LOD, glyph_size.y()>>LOD),
                                      4); //alignment
        }
    }



  return pImage;
}


void
WRATHTextureFontFreeType_Analytic::
generate_LOD_bitmap(const WRATHFreeTypeSupport::OutlineData &outline_data,
                    const ivec2 &glyph_size,
                    boost::multi_array<int, 2> &covered,
                    const boost::multi_array<WRATHFreeTypeSupport::analytic_return_type, 2> &analytic_data)
{
  /*
   */
  WRATHassert(m_mipmap_level>0);
    
  unsigned int LOD(m_mipmap_level+1);
  
  std::fill(covered.data(), 
            covered.data() + covered.num_elements(), 
            0);
  
  /*
    at m_mipmap_level+1 we will make it so that 
    the texel is either up or down based simply
    off how many texels are up or down at the 
    level 0..
  */
  for(int x=0; x<glyph_size.x(); ++x)
    {
      for(int y=0;y<glyph_size.y(); ++y)
        {
          if(x<outline_data.bitmap_size().x()
             and y<outline_data.bitmap_size().y())
            {
              if(analytic_data[x][y].m_parity_count[0]&1)
                {
                  ++covered[x>>LOD][y>>LOD];
                }
              else
                {
                  --covered[x>>LOD][y>>LOD];
                }
            }
        }
    }
}




WRATHTextureFont::glyph_data_type*
WRATHTextureFontFreeType_Analytic::
generate_character(WRATHTextureFont::glyph_index_type G)
{
  ivec2 pos, bitmap_sz, bitmap_offset, glyph_size;
  character_code_type C;
  ivec2 iadvance;
  
  WRATHassert(G.valid());
  C=character_code(G);

  //lock ttf_face mutex when we manipulate ttf_face:
  WRATHLockMutex(ttf_face()->mutex());
  
 
  FT_Load_Glyph(ttf_face()->face(), G.value(), FT_LOAD_DEFAULT);
  FT_Render_Glyph(ttf_face()->face()->glyph, FT_RENDER_MODE_NORMAL);

  bitmap_sz=ivec2(ttf_face()->face()->glyph->bitmap.width,
                  ttf_face()->face()->glyph->bitmap.rows);

  if(bitmap_sz.x()>0 and bitmap_sz.y()>0)
    {
      int padding(2<<m_mipmap_level);

      //is a slack of 2 pixels really needed?
      //test on N900 seem to indicate so,
      //but in truth no slack should be needed.
      glyph_size=bitmap_sz+ivec2(padding, padding);
    }
  else
    {
      glyph_size=bitmap_sz;
    }
      
  bitmap_offset=ivec2(ttf_face()->face()->glyph->bitmap_left,
                      ttf_face()->face()->glyph->bitmap_top 
                      - ttf_face()->face()->glyph->bitmap.rows);


  iadvance=ivec2(ttf_face()->face()->glyph->advance.x,
                 ttf_face()->face()->glyph->advance.y);

  
  std::vector<WRATHFreeTypeSupport::point_type> pts;
  geometry_data dbg(NULL, pts);


  /*
  int scale_factor(4);
  CubicsToQuadraticsEmitter emitter(ttf_face()->face()->glyph->outline, scale_factor);

  OutlineData outline_data(&emitter, scale_factor,
                           bitmap_sz, bitmap_offset, dbg);
  */
  
  OutlineData outline_data(ttf_face()->face()->glyph->outline, 
                           bitmap_sz, bitmap_offset, dbg);
  


  boost::multi_array<analytic_return_type, 2> analytic_data(boost::extents[bitmap_sz.x()][bitmap_sz.y()]);
  WRATHTextureFontUtil::SubQuadProducer *sub_primitive_maker(NULL);
  

  WRATHUnlockMutex(ttf_face()->mutex());
  //no longer need ttf_face.
  
  
  if(m_generate_sub_quads)
    {
      /*
        allow sub-tesselation of upto 25 rects.
      */
      int quad_size( std::max(bitmap_sz.x(),bitmap_sz.y())/8);
      
      sub_primitive_maker=WRATHNew WRATHTextureFontUtil::SubQuadProducer(bitmap_sz, quad_size);
    }

  /*
    now generate the analytic data pixels:
   */
  unsigned int num_levels_total;

  num_levels_total=compute_num_levels_needed(glyph_size,
                                             m_mipmap_level);

  std::vector< vecN<std::vector<uint8_t>, number_textures_per_page> > packed_analytic_pixel_data(num_levels_total);
  std::vector< vecN<c_array<uint8_t>, number_textures_per_page> > analytic_pixel_data(num_levels_total);
  std::vector<OutlineData::curve_segment> ncts(2);
  std::vector<bool> reverse_component;

  for(unsigned int LOD=0; LOD<num_levels_total; ++LOD)
    {
      int product_size((glyph_size.x()>>LOD)*(glyph_size.y()>>LOD));

      /* allocate pixel data*/
      for(int i=0;i<number_textures_per_page;++i)
        {
          packed_analytic_pixel_data[LOD][i].resize(m_bytes_per_pixel[i]*product_size);
          analytic_pixel_data[LOD][i]=packed_analytic_pixel_data[LOD][i];
        }
    }
      
  /* compute intersecions*/
  outline_data.compute_analytic_values(analytic_data, reverse_component);

  boost::multi_array<int, 2> covered;
  boost::multi_array<bool, 2> no_intersection_texel_is_full_table(boost::extents[glyph_size.x()][glyph_size.y()]);

  if(m_mipmap_level>0 and glyph_size.x()>0 and glyph_size.y()>0)
    {
      boost::multi_array<int, 2>::extent_gen extents;
      int xsz(glyph_size.x()>>(1+m_mipmap_level));
      int ysz(glyph_size.y()>>(1+m_mipmap_level));

      covered.resize(extents[1+xsz][1+ysz]);
      generate_LOD_bitmap(outline_data, glyph_size,
                          covered, analytic_data);
    }

  /* 
     pack intersecion data into pixel data,
     note that analytic_pixel_data is the
     same memory as packed_analytic_pixel_data,
     only that it views the memory as an
     array of vecN<uint8_t,4>.
   */
  for(int y=0;y<glyph_size.y();++y)
    {
      /*
        we do NOT rely on m_parity_count
        to determine if a texel without
        intersections should be full or empty
        because that intersection account
        is correct only if the horizontal
        (or vertical) line used intersects
        the outline transversally, i.e.
        not tangentially to a curve and not
        through a vertex. This only happens
        when the glyph consists of quadratics
        only though.
       */
      bool no_intersection_texel_is_full;

      no_intersection_texel_is_full=false;

      for(int x=0;x<glyph_size.x();++x)
        {
          unsigned int curve_count, curves_used(0);
          float far_away_offset;
          
          /*
            save the value for the mipmap levels to use.
           */
          no_intersection_texel_is_full_table[x][y]=no_intersection_texel_is_full;

          if(x<bitmap_sz.x() and y<bitmap_sz.y() 
             and no_intersection_texel_is_full)
            {
              far_away_offset=-1.0f;
              if(sub_primitive_maker!=NULL)
                {
                  sub_primitive_maker->mark_texel(x,y);
                }
            }
          else
            {
              far_away_offset=1.0f;
            }
          
          if(x<bitmap_sz.x() and y<bitmap_sz.y() 
             and !analytic_data[x][y].m_empty)
            {
              
              curve_count=
                outline_data.compute_localized_affectors(analytic_data[x][y], 
                                                         ivec2(x,y), ncts);

              //now store the first N curves in the texture data:
              for(unsigned int i=0; i<curve_count; ++i)
                {
                  if(reverse_component[ncts[i].m_curve->contourID()])
                    {
                      std::reverse(ncts[i].m_control_points.begin(),
                                   ncts[i].m_control_points.end());
                    }

                  //TODO: if a curve is too degnerate substitute far_away_line.
                  ++curves_used;
                }
            }     
                  
          int L;
          L=x + y*glyph_size.x();

          if(curves_used>0)
            {
              if(sub_primitive_maker!=NULL)
                {
                  sub_primitive_maker->mark_texel(x,y);
                }
            }
          pack_lines(ivec2(x,y), L, ncts, curves_used, far_away_offset,
                     analytic_pixel_data[0], no_intersection_texel_is_full);

          
        } //of for(x=...)
    } //of for(y=...)

  if(m_mipmap_level>0 and glyph_size.x()>0 and glyph_size.y()>0)
    {
      int L;

      for(unsigned int LOD=1; LOD<=m_mipmap_level and LOD<num_levels_total; ++LOD)
        {
          int end_xlod, end_ylod;

          end_xlod=glyph_size.x()>>LOD;
          end_ylod=glyph_size.y()>>LOD;

          for(int ylod=0; ylod<end_ylod; ++ylod)
            {
              bool no_intersection_texel_is_full;

              no_intersection_texel_is_full=false;
              for(int xlod=0; xlod<end_xlod; ++xlod)
                {
                  int x(xlod<<LOD), y(ylod<<LOD);
                  unsigned int curve_count, curves_used(0);
                  float far_away_offset;
                  
                  if(x<bitmap_sz.x() and y<bitmap_sz.y() 
                     and no_intersection_texel_is_full_table[x][y])
                    {
                      far_away_offset=-1.0f;
                    }
                  else
                    {
                      far_away_offset=1.0f;
                    }

                  curve_count=
                    outline_data.compute_localized_affectors_LOD(LOD,
                                                                 analytic_data,
                                                                 ivec2(xlod, ylod),
                                                                 ncts);

                  for(unsigned int i=0; i<curve_count; ++i)
                    {
                      if(reverse_component[ncts[i].m_curve->contourID()])
                        {
                          std::reverse(ncts[i].m_control_points.begin(),
                                       ncts[i].m_control_points.end());
                        }
                      //TODO: if a curve is too degnerate substitute far_away_line.
                      ++curves_used;
                    }

              
                  L=xlod + ylod*end_xlod;
                  pack_lines(ivec2(x, y), 
                             L, ncts, curves_used, far_away_offset,
                             analytic_pixel_data[LOD], no_intersection_texel_is_full);
                }
            }
        }
      
      for(unsigned int LOD=m_mipmap_level+1; LOD<num_levels_total; ++LOD)
        {
          unsigned int LOD_delta(LOD-m_mipmap_level-1);
          int end_xlod, end_ylod;
          float far_away_offset;

          end_xlod=glyph_size.x()>>LOD;
          end_ylod=glyph_size.y()>>LOD;
          
          
          for(int ylod=0;ylod<end_ylod; ++ylod)
            {
              bool no_intersection_texel_is_full;

              no_intersection_texel_is_full=false;
              for(int xlod=0;xlod<end_xlod; ++xlod)
                {
                  int x(xlod<<LOD_delta), y(ylod<<LOD_delta);
                  
                  if(covered[x][y]>=0)
                    {
                      far_away_offset=-1.0f;
                    }
                  else
                    {
                      far_away_offset=1.0f;
                    }

                  L=xlod + ylod*end_xlod;
                  pack_lines(ivec2(x, y), 
                             L, ncts, 0, far_away_offset,
                             analytic_pixel_data[LOD], no_intersection_texel_is_full);
                }
            }
        }
    }


  
  WRATHImage *glyph_image;
  glyph_data_type *return_value;

  glyph_image=allocate_glyph(packed_analytic_pixel_data, 
                             glyph_size);

  return_value=WRATHNew local_glyph_data(glyph_image);
  glyph_data_type &glyph(*return_value);

  glyph
    .font(this)
    .iadvance(iadvance)
    .texture_page(m_page_tracker.get_page_number(glyph_image))
    .texel_values(glyph_image->minX_minY(), bitmap_sz)
    .origin(bitmap_offset)
    .bounding_box_size(bitmap_sz)
    .character_code(C)
    .glyph_index(G);

  /*
    create sub-primitiveing:
   */  
  if(sub_primitive_maker!=NULL)
    {
      const std::vector<uint16_t> &source_indices(sub_primitive_maker->primitive_indices());
      const std::vector<ivec2>& source_attributes(sub_primitive_maker->primitives_attributes());
  
      glyph.sub_primitive_indices().resize(source_indices.size());
      std::copy(source_indices.begin(), source_indices.end(), 
                glyph.sub_primitive_indices().begin());
            
      glyph.sub_primitive_attributes().resize(source_attributes.size());
      for(int a=0, end_a=source_attributes.size(); a!=end_a; ++a)
        {
          glyph.sub_primitive_attributes()[a].set(glyph, source_attributes[a]);
        }
      WRATHDelete(sub_primitive_maker);
    }

  return return_value;
}


void
WRATHTextureFontFreeType_Analytic::
pack_lines(ivec2 pt, int L, 
           const std::vector<WRATHFreeTypeSupport::OutlineData::curve_segment> &curves,
           int curve_count, float far_away_offset,
           vecN<c_array<uint8_t>, number_textures_per_page> analytic_data,
           bool &no_intersection_texel_is_full)
{
  vecN<vec2,2> n_vector;
  vecN<float,2> offset;
  bool use_and(false);
  bool bad_dot(false);
  
  far_away_offset*=m_pow2_mipmap_level;

  if(curve_count==0)
    {
      n_vector[0]=vec2(0.0f, 0.0f);
      offset[0]=far_away_offset;
    }


  for(int i=0, end_i=std::min(2, curve_count); i<end_i; ++i)
    { 
      calculate_line_segment_data(curves[i].m_control_points.front().m_texel_normalized_coordinate,
                                  curves[i].m_control_points.back().m_texel_normalized_coordinate,
                                  n_vector[i], offset[i]);
    }
  
  /*
    if there is only 1 curve, we make
    the 2nd curve the same as the first.
   */
  for(int i=curve_count; i<2; ++i)
    {
      n_vector[i]=n_vector[0];
      offset[i]=offset[0];
    }
        
  if(curve_count>=2)
    {
      vec2 p0, p1, p, p0a, p0b, p1a, p1b;
      float dot0, dot1;
      
      p0a=curves[0].m_control_points.front().m_texel_normalized_coordinate;
      p0b=curves[0].m_control_points.back().m_texel_normalized_coordinate;
      p0=0.5f*(p0a+p0b);
        
      p1a=curves[1].m_control_points.front().m_texel_normalized_coordinate;
      p1b=curves[1].m_control_points.back().m_texel_normalized_coordinate;
      p1=0.5f*(p1a+p1b);
      
      p=0.5f*(p0+p1);
      dot0=dot( n_vector[0], p-p0);
      dot1=dot( n_vector[1], p-p1);
      
      bad_dot=( (dot0>0.0f) xor (dot1>0.0f) );
      WRATHunused(bad_dot);

      //if p is on the "inside" of both sides
      //then the logical operation is AND
      //also, it should be that either p
      //is on inside of both sides or neither!
      if(dot0>0.0f or dot1>0.0f)
        {
          use_and=true;
        }
    }
  
  /*
    update no_intersection_texel_is_full
    if there are any curves, take the
    middle of the right edge as the test
    point.
   */
  if(curve_count>=1)
    {
      float dot0, dot1;
      vec2 q(1.0f, 0.5f);

      dot0=dot( n_vector[0], q) - offset[0];
      dot1=dot( n_vector[1], q) - offset[1];
      
      if(use_and)
        {
          no_intersection_texel_is_full= dot0>0.0f and dot1>0.0f;
        }
      else
        {
          no_intersection_texel_is_full= dot0>0.0f or dot1>0.0f;
        }
    }

  //now pack into the texture data
  //packing is:
  //  [0].xy = normal[0].xy
  //  [0].zw = normal[1].zw
  //  [1].x = offset[0]
  //  [1].y = offset[1]
  //  [1].zw=texel coordinate
  
  //we are going to implicitly store which logical operation
  //in the ordering of the lines as folows:
  //If the "restricted" cross product of n_vector[0] X n_vector[1]
  //is negative then we use AND:
  float cross_value;
  cross_value=n_vector[0].x()*n_vector[1].y() - n_vector[0].y()*n_vector[1].x();
  
  if((cross_value<0.0) xor (use_and))
    {
      std::swap(n_vector[0], n_vector[1]);
      std::swap(offset[0], offset[1]);
    }
  
  vecN<uint8_t, 4> packed_normals;
    
  packed_normals=pack_from_minus_one_plus_one( vec4(n_vector[0].x(),
                                                    n_vector[0].y(),
                                                    n_vector[1].x(),
                                                    n_vector[1].y()) );
  WRATHassert(m_bytes_per_pixel[0]==4);
  for(int i=0;i<4;++i)
    {
      analytic_data[0][ m_bytes_per_pixel[0]*L+i ]=packed_normals[i];
    }

  switch(m_texture_mode)
    {
    default:
    case local_pixel_coordinates:
      {
        vecN<uint8_t,2> ioffset;
        ioffset=pack_from_minus_one_plus_one(offset/(2.0f*static_cast<float>(1<<m_mipmap_level)));

        analytic_data[1][4*L+0]=ioffset[0];
        analytic_data[1][4*L+1]=ioffset[1];
        analytic_data[1][4*L+2]=pt.x();
        analytic_data[1][4*L+3]=pt.y();
      }
      break;

    case global_pixel_coordinates_16bit:
    case global_pixel_coordinates_32bit:
      {
        vec2 fpt(pt.x(), pt.y());
        /*
          we need to increment offsets[]
          normally we would just increment
          offset[i] by dot(n_vector[i], fpt),
          but we need to keep in mind that
          we store the normal in 8 bits,
          so we will get the normal back
          from the 8-bit encoding and
          do the computation from that value.
         */
        for(int i=0;i<2;++i)
          {
            vec2 n( packed_normals[2*i], packed_normals[2*i+1]);

            n/=(254.0f*0.5f);
            n+=vec2(-1.0f, -1.0f);
            offset[i]+=dot(n, fpt);
          }

        if(m_texture_mode==global_pixel_coordinates_16bit)
          {
            WRATHassert(m_bytes_per_pixel[1]==4);
            vecN<uint8_t, 4> as_fp16;

            WRATHUtil::convert_to_halfp_from_float(as_fp16, offset);
            for(int i=0; i<4; ++i)
              {
                analytic_data[1][4*L+i]=as_fp16[i];
              }
          }
        else
          {
            WRATHassert(m_bytes_per_pixel[1]==8);
            memcpy(analytic_data[1].c_ptr()+8*L, offset.c_ptr(), 8);
          }
      }
      break;

      

    }
  
}




const_c_array<WRATHTextureChoice::texture_base::handle>
WRATHTextureFontFreeType_Analytic::
texture_binder(int pg)
{
  return m_page_tracker.texture_binder(pg);
}

ivec2
WRATHTextureFontFreeType_Analytic::
texture_size(int pg)
{
  return m_page_tracker.main_texture_size(pg);
}

int
WRATHTextureFontFreeType_Analytic::
number_texture_pages(void)
{
  return m_page_tracker.number_texture_pages();
}

const WRATHTextureFont::GlyphGLSL*
WRATHTextureFontFreeType_Analytic::
glyph_glsl(void)
{
  return common_data().glyph_glsl(m_texture_mode, m_mipmap_level);
}

GLint
WRATHTextureFontFreeType_Analytic::
texture_creation_size(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  return common_data().m_texture_creation_size;
}

void
WRATHTextureFontFreeType_Analytic::
texture_creation_size(GLint v)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  common_data().m_texture_creation_size=v;
  common_data().m_allocator.texture_atlas_dimension(v);
}

void
WRATHTextureFontFreeType_Analytic::
creation_texture_mode(enum WRATHTextureFontFreeType_Analytic::texture_mode_type v)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  common_data().m_creation_mode=v;
}

enum WRATHTextureFontFreeType_Analytic::texture_mode_type
WRATHTextureFontFreeType_Analytic::
creation_texture_mode(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  return common_data().m_creation_mode;
}

bool
WRATHTextureFontFreeType_Analytic::
generate_sub_quads(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  return common_data().m_generate_sub_quads;
}

void
WRATHTextureFontFreeType_Analytic::
generate_sub_quads(bool v)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  common_data().m_generate_sub_quads=v;
}

void
WRATHTextureFontFreeType_Analytic::
mipmap_level(unsigned int N)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  common_data().m_mipmap_level=N;
}

unsigned int
WRATHTextureFontFreeType_Analytic::
mipmap_level(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  return common_data().m_mipmap_level;
}


WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
WRATHTextureFontFreeType_Analytic::
texture_consumption(void)
{
  return common_data().m_allocator.texture_consumption();
}




