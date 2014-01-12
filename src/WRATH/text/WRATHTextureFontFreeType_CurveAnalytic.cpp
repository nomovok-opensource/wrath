/*! 
 * \file WRATHTextureFontFreeType_CurveAnalytic.cpp
 * \brief file WRATHTextureFontFreeType_CurveAnalytic.cpp
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
#include "WRATHTextureFontFreeType_CurveAnalytic.hpp"
#include "WRATHPolynomial.hpp"
#include "WRATHStaticInit.hpp"
#include <map>


/*
  Overview of CurveAnalytic font rendering:

  Let c(t) = ( x(t), y(t) ) = c0 + t*c1 + t*t*c2

  be a quadratic or linear (if c2=0) curve.
  If x(t)=c0_x + c1_x*t, then the descision if a point (x,y) 
  is on the correct side of a curve is just:

  (c0_y + c1_y *t+ c1_y*t*t - y)*c1_x > 0

  where t=(x-c0_x)/c1_x. 

  Replacing (x,y) with (x-c_x, y-c_y) gives:

  (c1_y*t + c1_y*t*t - y)*c1_x > 0

  where t=x/c1_x. 

  There is a rotation Q, so that c2_x=0,
  this rotation is esseintially given in 
  complex arithmatic as Q(z) = z*(i*c2)/||c2||.
  Then:

  Qc(t) = ( m0*t, m1*t + S*t*t)  

  where S=||c2||

  given a point p=(p_x, p_y) we produce
  (x,y)=Q(p_x-c0_x, p_y-c0_y)

  and then the curve c views the point within the
  glyph if and only if:

  ( m1*t + S*t*t - y)*m0 > 0

  where t=x/m0.

  The remaining issue are handling when t<0 or t>1,
  i.e. outside the curves range and handling at the 
  corners where curve's meet. See the shader source
  code for those details.

  Due to various issues with different GLES2 implementations,
  a number of work arounds are supported:

  - Use 2xLA16F in place of RGBA16F (goverened by the bit-flag: two_channel
  
  Additionally a number of options:

  - Make S always 1 (thus the mapping Q is a rotation and scaling).
  This saves us from storing another floating point value. Controlled
  by the bit flag: with_scaling

  - Store curves as curve-corner pairs or separetely, conrolled by the
  bit flag: separate_curve. 

  Storing curve-corner pairs means that each texel of the "curve texture" 
  stores a pair of curvers, alpha and beta. Alpha and beta are parameterized 
  to that they both come out of the point of the corner. We parameterize alpha
  as "backwards" to it's usual orientation. As such the shader needs to adjust
  the equation ( m1*t + S*t*t - y)*m0 > 0 to ( m1*t + S*t*t - y)*m0 < 0
  for alpha.
  
  Storing seperately means each curve is given it's own texel. The
  advantage being that then a texel is much less memory. However,
  the shader needs magick within to handle that the curves to not
  "eminate" from the same point, i.e for one curve the corner is
  at t=0 and for the other it is at t=1. Additionally, curves
  need to store the "next curve" so that the shader can fetch it.


  The texture data is as follows:
  - texture 0: is a 1-channel (LUMINANCE or RED) unfiltered texture. 
    The value of the texture, I, with the the
    normalized_localized_glyph_code, G, forms
    a texture coordinate (I,G) which is used in 
    the remaining textures.

  followed by the following data 
  (see common_data_type::build_sampler_names_and_format and 
  AnalyticDataPacket::pack) for how the values are packed:

  For each curve:
    - m0 (in CurveExtractor::m0)
    - m1 (in CurveExtractor::m1)
    - Q (in CurveExtractor::m_q)
    - if curve is quadratic (in CurveExtractor::m_c)
    - S (in AnalyticData::m_quad_coeffA and/or AnalyticData::m_quad_coeffB
    
  Each curve corner also has additional data
    - AnalyticData::m_rule combine rule (AND or OR) determined by tangents of curves 
    at the corner
    - AnalyticData::m_tangled additional "complication" for handling when
    atleast one of the curves is a quadratic. Essentially represents
    if the a curve or it's axis of it's shadow (for a line it is the
    line, for a quadratic is it the unit segement starting at the 
    corner _perpindicular to the axis of the parabola) is between
    the other curve and it's axis.
    
 */

#if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
  #define HALF_FLOAT_INTERNAL_FORMAT_4CHANNEL GL_RGBA
  #define HALF_FLOAT_INTERNAL_FORMAT_2CHANNEL GL_LUMINANCE_ALPHA
  #define HALF_FLOAT_INTERNAL_FORMAT_1CHANNEL GL_LUMINANCE
  #define HALF_FLOAT_PIXEL_TYPE GL_HALF_FLOAT_OES
  #define PIXEL_TYPE_2CHANNEL GL_LUMINANCE_ALPHA
  #define PIXEL_TYPE_1CHANNEL GL_LUMINANCE
#else
  #define HALF_FLOAT_INTERNAL_FORMAT_4CHANNEL GL_RGBA16F
  #define HALF_FLOAT_INTERNAL_FORMAT_2CHANNEL GL_RG16F
  #define HALF_FLOAT_INTERNAL_FORMAT_1CHANNEL GL_R16F
  #define HALF_FLOAT_PIXEL_TYPE GL_HALF_FLOAT
  #define PIXEL_TYPE_2CHANNEL GL_RG
  #define PIXEL_TYPE_1CHANNEL GL_RED
#endif


namespace
{
  enum rule_type
    {
      or_rule,
      and_rule
    };


  enum completely_texel_t
    {
      completely_empty_texel=0,
      completely_full_texel=255,
    };

  enum 
    {
      with_scaling=1,
      two_channel=2,
      separate_curve=4
    };

  class MakeEvenFilter:public WRATHFreeTypeSupport::geometry_data_filter
  {
  public:
    virtual
    ivec2
    apply_filter(const ivec2 &in_pt, enum WRATHFreeTypeSupport::point_type::point_classification cl) const
    {
      if(cl==WRATHFreeTypeSupport::point_type::on_curve)
        {
          return ivec2( in_pt.x() + (in_pt.x()&1),
                        in_pt.y() + (in_pt.y()&1));
        }
      else
        {
          return in_pt;
        }
    }
  };
 

  template<typename T>
  vecN<T,2>
  apply_J(const vecN<T,2> &v)
  {
    return vecN<T,2>(v.y(), -v.x());
  }

  class TaggedOutlineData;

  class TranslateControlPointFilter
  {
  public:
    explicit
    TranslateControlPointFilter(const ivec2 &v):
      m_v(v)
    {}

    TranslateControlPointFilter(const_c_array<ivec2> pts,
                                enum WRATHUtil::reverse_control_points_t t):
      m_v( (t==WRATHUtil::reverse_control_points)?pts.back():pts.front())
    {}

    ivec2
    operator()(const ivec2 &v) const
    {
      return v-m_v;
    }
    ivec2 m_v;
  };

  vec2
  get_point(const ivec2 &texel_bl, const ivec2 &texel_tr,
            int side,
            const WRATHFreeTypeSupport::simple_line *L)
  {
    enum WRATHFreeTypeSupport::boundary_type v(static_cast<enum WRATHFreeTypeSupport::boundary_type>(side));
    vec2 R;
    int fixed_coord;

    fixed_coord=WRATHUtil::fixed_coordinate(WRATHFreeTypeSupport::side_type(v));

    R[fixed_coord]=(WRATHFreeTypeSupport::is_min_side_type(v))?
      texel_bl[fixed_coord]:
      texel_tr[fixed_coord];

    R[1-fixed_coord]=L->m_value;

    return R;
  }

  float
  compute_area(vec2 a, vec2 b, vec2 c)
  {
    b-=a;
    c-=a;

    return 0.5f*std::abs( b.x()*c.y() - b.y()*c.x());
  }


  vecN<vec2, 2>
  get_corner_points(const ivec2 &texel_bl, const ivec2 &texel_tr,
                    int side0, int side1, const vec2 &if_not_found)
  {
    vecN<vec2, 2> R;
    enum WRATHFreeTypeSupport::boundary_type v0(static_cast<enum WRATHFreeTypeSupport::boundary_type>(side0));
    enum WRATHFreeTypeSupport::boundary_type v1(static_cast<enum WRATHFreeTypeSupport::boundary_type>(side1));

    if(v0==WRATHFreeTypeSupport::opposite_boundary(v1))
      {
        if(WRATHFreeTypeSupport::side_type(v0)==WRATHUtil::x_fixed)
          {
            R[0]=vec2(texel_bl.x(), texel_bl.y());
            R[1]=vec2(texel_tr.x(), texel_bl.y());
          }
        else
          {
            R[0]=vec2(texel_bl.x(), texel_bl.y());
            R[1]=vec2(texel_bl.x(), texel_tr.y());
          }
      }
    else
      {
        R[1]=if_not_found;
        
        if(side_type(v0)!=WRATHUtil::x_fixed)
          {
            std::swap(v0, v1);
          }
        
        /*
          v0 is either left or right
          and v1 is either below or above.
        */
        WRATHassert(v0==WRATHFreeTypeSupport::left_boundary
                    or v0==WRATHFreeTypeSupport::right_boundary);
        WRATHassert(v1==WRATHFreeTypeSupport::below_boundary
                    or v1==WRATHFreeTypeSupport::above_boundary);
        
        R[0].x()= (v0==WRATHFreeTypeSupport::left_boundary)?
          texel_bl.x():
          texel_tr.x();
        
        R[0].y()= (v0==WRATHFreeTypeSupport::below_boundary)?
          texel_bl.y():
          texel_tr.y();
      }
    return R;
  }

  class CurveExtractor
  {
  public:
    CurveExtractor(bool make_rotation_unitary,
                   const WRATHFreeTypeSupport::BezierCurve *c,
                   const TaggedOutlineData &outline_data,
                   bool reverse_curve);

    float m_0, m_1, m_quad_coeff;
    vec2 m_q;
    bool m_c;

    vecN<int64_t, 2> m_derivative;
    vecN<int64_t, 2> m_ray;
    vecN<int64_t, 2> m_accelleration;

    bool
    tangled(const vecN<int64_t, 2> &v)
    {
      WRATHassert(m_c);
      
      vecN<int64_t, 2> r, d;
      vecN<int64_t, 2> vv( -v.y(), v.x());

      r.x()=dot(v, m_ray);
      r.y()=dot(vv, m_ray);

      d.x()=dot(v, m_derivative);
      d.y()=dot(vv, m_derivative);

      return (r.x()>0) 
        and (d.x()>0)
        and ( (r.y()>0) xor (d.y()>0));
    }
  };
 
  /*
    data extracted from a BezierCurve that
    is to be packed into textures suitable
    for the shader.
   */
  class AnalyticData
  {
  public:
    explicit
    AnalyticData(bool make_rotation_unitary,
                 const TaggedOutlineData &raw_outline_data,
                 const WRATHFreeTypeSupport::BezierCurve *c);

    explicit
    AnalyticData(enum completely_texel_t);


    vec2
    a0_b0(void) const { return vec2(m_a0_b0_a1_b1.x(), m_a0_b0_a1_b1.y()); }

    vec2
    a1_b1(void) const { return vec2(m_a0_b0_a1_b1.z(), m_a0_b0_a1_b1.w()); }

    vec2
    a0_a1(void) const { return vec2(m_a0_b0_a1_b1.x(), m_a0_b0_a1_b1.z()); }
    
    vec4
    p2_scale_ab(void) const
    {
      return vec4(m_p2.x(), m_p2.y(),
                  m_quad_coeffA, m_quad_coeffB);
    }

    vec4
    a0_a1_p2(void) const 
    { 
      return vec4(m_a0_b0_a1_b1.x(), 
                  m_a0_b0_a1_b1.z(),
                  m_p2.x(),
                  m_p2.y()); 
    }

    vec2
    qa(void) const { return vec2(m_qa_qb.x(), m_qa_qb.y()); }

    vec2
    qb(void) const { return vec2(m_qa_qb.z(), m_qa_qb.w()); }


    vec2 m_p2;
    vec4 m_a0_b0_a1_b1; //m0, m1 of alpha and beta, a0<-->A's m0, b0<-->B's m0, etc 
    vec4 m_qa_qb; //rotation of A and B, qa <--> A's Q, qb<--> B's Q
    float m_quad_coeffA, m_quad_coeffB; //scale factor for alpha and beta (the S)
    bool m_cA, m_cB; // boolean flags indicating if A or B is quadratic
    enum rule_type m_rule; //combine rule
    bool m_tangled; //tangle tule
    bool m_tangential_curves; //if curves are tangential

    int m_id_curveA, m_id_curveB; //Id's of each curve
  };

  /*
    AnalyticData packed into the textures..
   */
  class AnalyticDataPacket
  {
  public:

    /*
      .first = which layer
      .second = raw bytes of layer
     */
    typedef std::pair<int, std::vector<uint8_t> > LayerBytes;
    
    AnalyticDataPacket(uint32_t flags,
                       const TaggedOutlineData &raw_outline_data,
                       int curve_offset);

    
    AnalyticDataPacket(uint32_t flags,
                       enum completely_texel_t v);
         

    std::list<LayerBytes> m_layers;    

    void
    relieve_layers(std::vector<std::vector<uint8_t> > &bytes);

  private:
    
    template<typename T>
    c_array<T>
    add_layer(int N)
    {
      int layer;

      layer=(m_layers.empty())?
        0:
        m_layers.back().first+1;

      m_layers.push_back(LayerBytes());

      m_layers.back().first=layer;
      m_layers.back().second.resize( sizeof(T)*N);

      c_array<uint8_t> raw(m_layers.back().second);

      return raw.reinterpret_pointer<T>();
    }


    void
    pack(uint32_t flags,
         const std::vector<AnalyticData> &numbers,
         int curve_offset); 
  };
  


  /*
    class to indicate how much
    room is availble on a horizontal
    line. Allocation is simple: 
    just increment a value.

    The 0'th and 255'th entries are 
    ALWAYS pre-allocated. The 0'th
    entry is to represent a texel
    that is always off and the 255'th
    entry is to represent a texel
    that is always on.
  */
  class horizontal_line
  {
  public:
    explicit
    horizontal_line(void):
      m_consumed(1)
    {}

    int
    allocate(int curve_count)
    {
      WRATHassert(can_allocate(curve_count));

      int ret(m_consumed);      
      m_consumed+=curve_count;
      return ret;
      
    }
    
    bool
    can_allocate(int curve_count)
    {
      return curve_count+m_consumed<=255;
    }
    
    int
    max_allocate_allowed(void)
    {
      return 255-m_consumed;
    }
    
  private:
    int m_consumed;
  };

  /*
    class to hold one WRATHImage which holds outline data
    for many glyphs.
   */
  class GeometryDataImage:boost::noncopyable
  {
  public:
    GeometryDataImage(const WRATHImage::ImageFormatArray &fmt, 
                      uint32_t flags,
                      const_c_array<std::vector<uint8_t> > always_on_bits,
                      const_c_array<std::vector<uint8_t> > always_off_bits);
    ~GeometryDataImage();

    WRATHImage*
    image(void) { return m_image; }

    ivec2
    allocate(int num_pts);

    void
    note_image_dtor(void)
    {
      m_image=NULL;
    }

    int
    max_allocate_allowed(void)
    {
      if(m_image==NULL)
        return 0;

      if(m_current_y<=255)
        return 254;
      else if(!m_finder.empty())
        return m_finder.rbegin()->first;
      else
        return 0;
    }

    void
    set_values(ivec2 xy,
               const TaggedOutlineData &raw_outline_data);

  private:

    
    void
    update_finder(int lineID)
    {
      int room_left(m_lines[lineID].max_allocate_allowed());
      if(room_left>0)
        {
          m_finder.insert( std::multimap<int, int>::value_type(room_left, lineID));
        }
    }
    uint32_t m_flags;
    WRATHImage *m_image;
    
    int m_current_y;
    std::multimap<int, int> m_finder;
    vecN<horizontal_line, 256> m_lines;
  };


  class GeometryDataImageSet:boost::noncopyable
  {
  public:
    typedef std::pair<ivec2, GeometryDataImage*> allocation_location;

    explicit
    GeometryDataImageSet(uint32_t flags,
                         const WRATHImage::ImageFormatArray &fmt);

    ~GeometryDataImageSet();

    allocation_location
    allocate_and_fill(const TaggedOutlineData &pdata);

  private:

    allocation_location
    allocate(int cnt);

    void
    update_finder(GeometryDataImage *pImage)
    {
      int room_left(pImage->max_allocate_allowed());
      if(room_left>0)
        {
          m_finder.insert( std::multimap<int, GeometryDataImage*>::value_type(room_left, pImage));
        }
    }

    WRATHMutex m_mutex;
    bool m_separate_curves;
    uint32_t m_flags;
    WRATHImage::ImageFormatArray m_fmt;

    std::vector<std::vector<uint8_t> > m_always_on_bits;
    std::vector<std::vector<uint8_t> > m_always_off_bits;
    

    std::vector<GeometryDataImage*> m_pool;
    std::multimap<int, GeometryDataImage*> m_finder;
  };

  std::string
  generate_unique_name(GeometryDataImage *p)
  {
    std::ostringstream ostr;
    static int count(0);

    ostr << "CurveAnalyticImage#" << ++count << ":" << p;
    return ostr.str();
  }

  class LocalImage:public WRATHImage
  {
  public:
    LocalImage(GeometryDataImage *owner,
               const WRATHImage::ImageFormatArray &fmt):
      WRATHImage(generate_unique_name(owner),
                 ivec2(256, 256),
                 fmt,
                 WRATHImage::UniquePixelStore),
      m_owner(owner)
    {}

    ~LocalImage()
    {
      if(m_owner!=NULL)
        {
          m_owner->note_image_dtor();
        }
    }

  private:
    GeometryDataImage *m_owner;
  };

  class IndexTextureData
  {
  public:
    IndexTextureData(TaggedOutlineData &outline_data,
                     ivec2 bitmap_size);

    WRATHImage*
    allocate_index_texture_and_fill(const GeometryDataImageSet::allocation_location &geometry_loc);

  private:
    typedef const WRATHFreeTypeSupport::simple_line* curve_cache_value_entry;
    typedef std::map<int, std::vector<curve_cache_value_entry> > curve_cache_value;
    typedef const WRATHFreeTypeSupport::BezierCurve *curve_cache_key;
    typedef std::map<curve_cache_key, curve_cache_value> curve_cache;

    uint8_t
    select_index(int x, int y,  
                 const GeometryDataImageSet::allocation_location &geometry_loc);

    
    bool
    intersection_should_be_used(int side, 
                                const WRATHFreeTypeSupport::simple_line *intersection);

    
    enum return_code
    sub_select_index(uint8_t &pixel_value,
                     const curve_cache &curves,
                     int x, int y,  
                     const GeometryDataImageSet::allocation_location &geometry_loc,
                     int winding_value);

    
    uint8_t
    sub_select_index_hard_case(curve_cache &curves,
                               int x, int y,
                               const ivec2 &texel_bl, 
                               const ivec2 &texel_tr,
                               const GeometryDataImageSet::allocation_location &geometry_loc);
    
    void
    remove_edge_huggers(curve_cache &curves,
                        const ivec2 &texel_bl, 
                        const ivec2 &texel_tr);

    static
    bool
    curve_hugs_edge(const WRATHFreeTypeSupport::BezierCurve *curve,
                    const ivec2 &texel_bl, const ivec2 &texel_tr,
                    int thresshold);

    
    std::pair<float, const WRATHFreeTypeSupport::BezierCurve*>
    compute_feature_importance(curve_cache &cache,
                               curve_cache::iterator iter,
                               const ivec2 &texel_bl, const ivec2 &texel_tr,
                               float texel_area);
    
    
    uint8_t
    hunt_neighbor_curves(const GeometryDataImageSet::allocation_location &geometry_loc,
                         const std::vector<uint8_t> &input,
                         int x, int y, ivec2 sz);

    
    void
    hunt_neighbor_curves_helper(std::set<int> &pset,
                                int x, int y,
                                int dx, int dy,
                                ivec2 sz,
                                const std::vector<uint8_t> &input);

    ivec2 m_bitmap_sz;
    const TaggedOutlineData &m_outline_data;
    std::vector<uint8_t> m_index_pixels;
    std::vector<bool> m_reverse_components;
    boost::multi_array<WRATHFreeTypeSupport::analytic_return_type, 2> m_intersection_data;
    boost::multi_array<int, 2> m_winding_values;
  };

  class common_data_type
  {
  public:

    common_data_type(void);
    ~common_data_type();

    WRATHImage*
    allocate_index_texture_and_fill(ivec2 sz,
                                    const TaggedOutlineData &outline_data,
                                    const GeometryDataImageSet::allocation_location &geometry_loc);

    WRATHImage*
    allocate_all_filled_index_texture(ivec2 sz);

    const WRATHImage::ImageFormatArray&
    index_fmt(void) 
    {
      return m_index_fmt;
    }

    WRATHImage*
    allocate_index_image(const ivec2 &sz, const WRATHImage::BoundarySize &bd_size)
    {
      return WRATHNew WRATHImage(sz,
                                 m_index_fmt,
                                 bd_size,
                                 m_allocator);
    }

    GeometryDataImageSet&
    get_geometry_data_set(int flags)
    {
      return *m_all_data[flags];
    }

    void
    texture_creation_size(GLint v)
    {
      WRATHAutoLockMutex(m_mutex);
      
      m_texture_creation_size=v;
      v=WRATHTextureFontUtil::effective_texture_creation_size(v, m_force_power2_texture);
      m_allocator.texture_atlas_dimension(v);
    }

    int
    texture_creation_size(void)
    {
      WRATHAutoLockMutex(m_mutex);
      return m_texture_creation_size;
    }

    int
    effective_texture_creation_size(void)
    {
      WRATHAutoLockMutex(m_mutex);
      return WRATHTextureFontUtil::effective_texture_creation_size(m_texture_creation_size,
                                                                   m_force_power2_texture);
    }

    void
    include_scaling_data(bool b)
    {
      WRATHAutoLockMutex(m_mutex);
      m_include_scaling_data=b;
    }

    bool
    include_scaling_data(void)
    {
      WRATHAutoLockMutex(m_mutex);
      return m_include_scaling_data;
    }
    
    void
    two_channel_texture_work_around(bool v)
    {
      WRATHAutoLockMutex(m_mutex);
      m_two_channel_texture_work_around=v;
    }

    bool
    two_channel_texture_work_around(void)
    {
      WRATHAutoLockMutex(m_mutex);
      return m_two_channel_texture_work_around;
    }
    
    void
    store_separate_curves(bool v)
    {
      WRATHAutoLockMutex(m_mutex);
      m_store_separate_curves=v;
    }

    bool
    store_separate_curves(void)
    {
      WRATHAutoLockMutex(m_mutex);
      return m_store_separate_curves;
    }

    uint32_t
    current_flags(void)
    {
      uint32_t r(0);

      if(two_channel_texture_work_around())
        {
          r|=two_channel;
        }

      if(include_scaling_data())
        {
          r|=with_scaling;
        }

      if(store_separate_curves())
        {
          r|=separate_curve;
        }

      return r;
    }


    void
    force_power2_texture(bool b)
    {
      WRATHAutoLockMutex(m_mutex);
      if(b!=m_force_power2_texture)
        {
          int v;
          
          v=WRATHTextureFontUtil::effective_texture_creation_size(m_texture_creation_size, b);
          m_force_power2_texture=b;
          m_allocator.texture_atlas_dimension(v);
        }
    }

    bool
    force_power2_texture(void)
    {
      WRATHAutoLockMutex(m_mutex);
      return m_force_power2_texture;
    }

    void
    curvature_collapse(float v)
    {
      WRATHAutoLockMutex(m_mutex);
      m_curvature_collapse=v;
    }
    
    float 
    curvature_collapse(void)
    {
      WRATHAutoLockMutex(m_mutex);
      return m_curvature_collapse;
    }

    WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
    texture_consumption_index(void)
    {
      return m_allocator.texture_consumption(m_index_fmt);
    }

    WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
    texture_consumption_curve(void)
    {
      WRATHAutoLockMutex(m_curve_consumption_counter_mutex);
      return m_curve_consumption;
    }

    void
    note_curve_texture_utilization(int num_pts)
    {
      WRATHAutoLockMutex(m_curve_consumption_counter_mutex);
      m_curve_consumption.m_number_texels_used+=num_pts;
    }

    void
    note_new_curve_texture(void)
    {
      WRATHAutoLockMutex(m_curve_consumption_counter_mutex);
      m_curve_consumption.m_number_texels+=256*256;
      m_curve_consumption.m_number_texels_used+=256*2;
    }

    const WRATHTextureFont::GlyphGLSL*
    glyph_glsl(uint32_t flags);
    
    WRATHImage::TextureAllocatorHandle m_allocator;

  private:


    static
    void
    build_sampler_names_and_format(int I,
                                   std::vector<std::string> &sampler_names,
                                   WRATHImage::ImageFormatArray &fmt);

    static
    void
    append_custom(int &layer, const std::string &pname,
                  std::vector<std::string> &sampler_names,
                  WRATHImage::ImageFormatArray &fmt,
                  const WRATHImage::ImageFormat &v);
    

    static
    void
    append_rgba16f(int &layer, const std::string &pname,
                   std::vector<std::string> &sampler_names,
                   WRATHImage::ImageFormatArray &fmt,
                   bool as_2_textures);

    static
    void
    append_la16f(int &layer, const std::string &pname,
                 std::vector<std::string> &sampler_names,
                 WRATHImage::ImageFormatArray &fmt);
    

    WRATHImage::ImageFormatArray m_index_fmt;

    WRATHMutex m_mutex;
    bool m_force_power2_texture;
    int m_texture_creation_size;
    bool m_include_scaling_data;
    bool m_two_channel_texture_work_around;
    bool m_store_separate_curves;
    float m_curvature_collapse;

    WRATHMutex m_curve_consumption_counter_mutex;
    WRATHImage::TextureAllocatorHandle::texture_consumption_data_type m_curve_consumption;
    vecN<GeometryDataImageSet*, 8> m_all_data;
    vecN<WRATHTextureFont::GlyphGLSL, 8> m_glyph_glsl;
  };

  common_data_type&
  common_data(void)
  {
    WRATHStaticInit();
    static common_data_type R;
    return R;
  }


  class local_glyph_data_type:public WRATHTextureFont::glyph_data_type
  {
  public:
    explicit
    local_glyph_data_type(WRATHImage *index_image,
                          GeometryDataImageSet::allocation_location loc,
                          int number_curves):
      m_index_image(index_image),
      m_loc(loc),
      m_number_curves(number_curves)
    {
      float t, n;
      /*
        The custom_float value is the y-texture coordinate
        of where the curve data sits, the texture size
        is 256 in height. We want to give the -normalized-
        texure coordinate. Now for something interesting:
        
        The normalization is from [0,256] to [0,1],
        and we want the "center" texel, so it is
        given by:

        (texel + 0.5)/256.0
      */
      t=static_cast<float>(loc.first.y());
      n=(0.5f + t)/256.0f;
      m_custom_float_data.push_back(n);
    }
    
    ~local_glyph_data_type()
    {
      WRATHDelete(m_index_image);
    }
    
    WRATHImage *m_index_image;
    GeometryDataImageSet::allocation_location m_loc;
    int m_number_curves;
  };

  class CollapsingContourEmitter:
    public WRATHFreeTypeSupport::ContourEmitterBase,
    public WRATHFreeTypeSupport::CoordinateConverter
  {
  public:

    CollapsingContourEmitter(float curvature_collapse,
                             const FT_Outline &outline,
                             const WRATHFreeTypeSupport::CoordinateConverter &conv,
                             int ch):
      WRATHFreeTypeSupport::CoordinateConverter(conv),
      m_real_worker(outline, conv.scale_factor()),
      m_glyph_code(ch),
      m_curvature_collapse(curvature_collapse)
    {}

    virtual
    void
    produce_contours(WRATHFreeTypeSupport::geometry_data data)
    {
      consumer_state S(this, data);
      m_real_worker.produce_contours(data);
    }

    
    int 
    glyph_code(void) const
    {
      return m_glyph_code;
    }

  private:
    
    class consumer_state
    {
    public:
      consumer_state(CollapsingContourEmitter *master,
                     WRATHFreeTypeSupport::geometry_data data);

      ~consumer_state()
      {
        m_consume_curves.disconnect();
        m_consume_contours.disconnect();
      }

    private:

      void
      consume_curve(WRATHFreeTypeSupport::BezierCurve *curve);
      
      void
      consume_contour(void);
      
      CollapsingContourEmitter *m_master;
      std::vector< std::pair<WRATHFreeTypeSupport::BezierCurve*, bool> > m_curves;
      std::vector< std::pair<WRATHFreeTypeSupport::BezierCurve*, int> > m_curves_to_emit;

      WRATHFreeTypeSupport::geometry_data m_data;
      boost::signals2::connection m_consume_curves;
      boost::signals2::connection m_consume_contours;
    };

    static
    float
    compute_curvature(const WRATHFreeTypeSupport::BezierCurve *curve);

    WRATHFreeTypeSupport::ContourEmitterFromFT_Outline m_real_worker;  
    int m_glyph_code;
    float m_curvature_collapse;
  };

  
  class TaggedOutlineData:public WRATHFreeTypeSupport::OutlineData
  {
  public:
    TaggedOutlineData(CollapsingContourEmitter *collapsing_contour_emitter,
                      const WRATHFreeTypeSupport::geometry_data gmt);
    
    int 
    glyph_code(void) const
    {
      return m_glyph_code;
    }

  private:
    int m_glyph_code;
  };

}

/////////////////////////////////////
// TaggedOutlineData methods
TaggedOutlineData::
TaggedOutlineData(CollapsingContourEmitter *collapsing_contour_emitter,
                  const WRATHFreeTypeSupport::geometry_data gmt):
  WRATHFreeTypeSupport::OutlineData(collapsing_contour_emitter,
                                    *collapsing_contour_emitter,
                                    gmt),
  m_glyph_code(collapsing_contour_emitter->glyph_code())
{}


//////////////////////////////////////
// CollapsingContourEmitter methods
CollapsingContourEmitter::consumer_state::
consumer_state(CollapsingContourEmitter *master,
               WRATHFreeTypeSupport::geometry_data data):
  m_master(master),
  m_data(data)
{
  signal_emit_curve::slot_type C(boost::bind(&CollapsingContourEmitter::consumer_state::consume_curve,
                                             this, _1));
  signal_end_contour::slot_type O(boost::bind(&CollapsingContourEmitter::consumer_state::consume_contour,
                                              this));

  m_consume_curves=m_master->m_real_worker.connect_emit_curve(C);
  m_consume_contours=m_master->m_real_worker.connect_emit_end_contour(O);
}



void
CollapsingContourEmitter::consumer_state::
consume_curve(WRATHFreeTypeSupport::BezierCurve *curve)
{
  /*
    Step 1: detect if the start and end position
    of curve are within the same texel:
   */
  ivec2 p0, p1;
  ivec2 tp0, tp1;
  std::pair<WRATHFreeTypeSupport::BezierCurve*, bool> v(curve, false);

  p0=curve->pt0();
  p1=curve->pt1();

  tp0=m_master->texel(p0);
  tp1=m_master->texel(p1);

  
  v.second=(tp0==tp1);

  if(curve->degree()==3)
    {
      vecN<WRATHFreeTypeSupport::BezierCurve*, 4> quads_4;
      vecN<WRATHFreeTypeSupport::BezierCurve*, 4> quads_2;
      vecN<WRATHFreeTypeSupport::BezierCurve*, 1> quads_1;
      c_array<WRATHFreeTypeSupport::BezierCurve*> quads;
      enum return_code R;
      bool split_as_4, split_as_2;
      int L1dist;

      /*
        "small" cubics, i.e. those whose end points
        are 2 or fewer texels apart are broken into
        1 or 2 quads rather than 4.
       */
      L1dist=(tp0-tp1).L1norm();
      split_as_4= (L1dist>6);
      split_as_2= (L1dist>3);

      if(split_as_4)
        {
          R=curve->approximate_cubic(m_data, quads_4);
          quads=quads_4;
          
          WRATHassert(R==routine_success);
        }
      else if(split_as_2)
        {
          R=curve->approximate_cubic(m_data, quads_2);
          quads=quads_2;
          
          WRATHassert(R==routine_success);
        }
      else 
        {
          R=curve->approximate_cubic(m_data, quads_1);
          quads=quads_1;
          
          WRATHassert(R==routine_success);
        }

      for(unsigned int i=0; i<quads.size(); ++i)
        {
          std::pair<WRATHFreeTypeSupport::BezierCurve*, bool> w(quads[i], false);
          ivec2 wp0, wp1;
          ivec2 wtp0, wtp1;
          
          wp0=quads[i]->pt0();
          wp1=quads[i]->pt1();
          wtp0=m_master->texel(wp0);
          wtp1=m_master->texel(wp1);
          w.second=(wtp0==wtp1);

          m_curves.push_back(w);
        }
      WRATHDelete(curve);
    }
  else
    {
      m_curves.push_back(v);
    }
}



void
CollapsingContourEmitter::consumer_state::
consume_contour(void)
{
  
  for(unsigned int i=0; i<m_curves.size(); ++i)
    {
      if(!m_curves[i].second)
        {
          m_curves_to_emit.push_back( std::make_pair(m_curves[i].first, i));
        }
    }

  if(m_curves_to_emit.empty())
    {
      /*
        all curves within the same texel, 
        thus we wil ignore the entire contour!
       */
      for(unsigned int i=0; i<m_curves.size(); ++i)
        {
          WRATHDelete(m_curves[i].first);
        }
      m_curves.clear();
      return;
    }

  for(unsigned int C=0, endC=m_curves_to_emit.size(); C+1<endC; ++C)
    {
      /*
        loop through the curves that are to be destroyed...
       */
      ivec2 pt(m_curves_to_emit[C].first->pt1());
      unsigned int number_skipped(0);

      for(int k=m_curves_to_emit[C].second+1; k<m_curves_to_emit[C+1].second; ++k, ++number_skipped)
        {
          WRATHassert(m_curves[k].first!=NULL);
          pt+=m_curves[k].first->pt1();

          WRATHDelete(m_curves[k].first);
          m_curves[k].first=NULL;
        }

      if(number_skipped>0)
        {
          pt/=(1+number_skipped);
          std::vector<GLushort> indices;
          GLushort new_pt_index;

          new_pt_index=m_data.push_back(pt, FT_CURVE_TAG_ON);  

          indices=m_curves_to_emit[C].first->control_point_indices();
          indices.back()=new_pt_index;   
          *m_curves_to_emit[C].first=WRATHFreeTypeSupport::BezierCurve(m_data, indices);
          
          indices=m_curves_to_emit[C+1].first->control_point_indices();
          indices.front()=new_pt_index;   
          *m_curves_to_emit[C+1].first=WRATHFreeTypeSupport::BezierCurve(m_data, indices);
        }
    }

  if(!m_curves_to_emit.empty())
    {
      int number_skipped(0);
      ivec2 pt(m_curves_to_emit.back().first->pt1());
      
      for(int k=m_curves_to_emit.back().second+1, end_k=m_curves.size(); k<end_k; ++k, ++number_skipped)
        {
          WRATHassert(m_curves[k].first!=NULL);
          pt+=m_curves[k].first->pt1();

          WRATHDelete(m_curves[k].first);
          m_curves[k].first=NULL;
        }
      
      for(int k=0; k<m_curves_to_emit.front().second; ++k, ++number_skipped)
        {
          WRATHassert(m_curves[k].first!=NULL);
          pt+=m_curves[k].first->pt1();

          WRATHDelete(m_curves[k].first);
          m_curves[k].first=NULL;
        }
      
      if(number_skipped>0)
        {
          pt/=(1+number_skipped);
          std::vector<GLushort> indices;
          GLushort new_pt_index;
          
          new_pt_index=m_data.push_back(pt, FT_CURVE_TAG_ON);  
          
          indices=m_curves_to_emit.back().first->control_point_indices();
          indices.back()=new_pt_index;   
          *m_curves_to_emit.back().first=WRATHFreeTypeSupport::BezierCurve(m_data, indices);
          
          indices=m_curves_to_emit.front().first->control_point_indices();
          indices.front()=new_pt_index;   
          *m_curves_to_emit.front().first=WRATHFreeTypeSupport::BezierCurve(m_data, indices);
        }
    }

  for(unsigned int C=0, endC=m_curves_to_emit.size(); C<endC; ++C)
    {
      WRATHFreeTypeSupport::BezierCurve *ptr(m_curves_to_emit[C].first);

      if(ptr->degree()==2 and m_master->m_curvature_collapse>0.0f)
        {
          float curvature;

          curvature=compute_curvature(ptr);
          if(curvature < m_master->m_curvature_collapse)
            {
              std::vector<GLushort> indices(2);
              
              indices[0]=ptr->control_point_indices().front();
              indices[1]=ptr->control_point_indices().back();
              
              *ptr=WRATHFreeTypeSupport::BezierCurve(m_data, indices);
            }
        }

      m_master->emit_curve(ptr);
    }

  m_curves.clear();
  m_curves_to_emit.clear();
  m_master->emit_end_contour();
}

//////////////////////////////////////////
// CollapsingContourEmitter methods
float
CollapsingContourEmitter::
compute_curvature(const WRATHFreeTypeSupport::BezierCurve *ptr)
{
  if(ptr->degree()!=2)
    {
      return 0.0f;
    }


  /*
    Curvature = integral_{t=0}^{t=1} K(t) || p_t(t) || dt
    
    p(t) = a0 + a1*t + a2*t*t

    K(t)= || p_t X p_tt || / || p_t ||^ 3

    then

    Curvature = integral_{t=0}^{t=1} ||a1 X a2||/( ||a1||^2 + 2t<a1,a2> + t^2 ||a2||^2 )

    Notes:
       Integral ( 1/(a+bx+cxx) ) dx = 2 atan( (b+2cx)/d ) / d
       where d=sqrt(4ac-b*b)
    
    and
       integral_{x=0}^{x=1} dx = 2/d * ( atan( (b+2c)/d ) - atan(b/d) )
                               = 2/d * atan( ( (b+2c)/d - b/d)/(1 + (b+2c)*b/(d*d) ) ) 
                               = 2/d * atan( 2cd/( dd + bb + 2cb)) 
                               = 2/d * atan( 2cd/( 4ac - bb + bb + 2cb))
                               = 2/d * atan( d/(2a + b) )

   */

  const std::vector<int> &src_x(ptr->curve().x());
  const std::vector<int> &src_y(ptr->curve().y());
  vec2 a1(src_x[1], src_y[1]), a2(src_x[2], src_y[2]);
  float a, b, c, R, desc, tt;
  
  R=std::abs(a1.x()*a2.y() - a1.y()*a2.x());
  a=dot(a1, a1);
  b=2.0f*dot(a1, a2);
  c=dot(a2, a2);

  const float epsilon(0.000001f), epsilon2(epsilon*epsilon);

  desc=std::sqrt(std::max(epsilon2, 4.0f*a*c - b*b));
  tt= desc/std::max(epsilon, std::abs(2.0f*a + b));
  return 2.0*R*atanf(tt) / desc;
    
}


/////////////////////////////////////
// common_data_type methods
common_data_type::
common_data_type(void):
  m_force_power2_texture(false),
  m_texture_creation_size(1024),
  m_include_scaling_data(false),
  m_two_channel_texture_work_around(false),
  m_store_separate_curves(false),
  m_curvature_collapse(0.05f)
{
  
  m_allocator=WRATHImage::create_texture_allocator(true, m_texture_creation_size,
                                                   GL_CLAMP_TO_EDGE,
                                                   GL_CLAMP_TO_EDGE);
  
  m_index_fmt
    .format(0, WRATHImage::ImageFormat()
            .internal_format(PIXEL_TYPE_1CHANNEL)
            .pixel_data_format(PIXEL_TYPE_1CHANNEL)
            .pixel_type(GL_UNSIGNED_BYTE)
            .magnification_filter(GL_NEAREST)
            .minification_filter(GL_NEAREST)
            .max_mip_level(0)
            .automatic_mipmap_generation(false));
  
  
  
  for(uint32_t i=0, end_i=m_all_data.size(); i<end_i; ++i)
    {
      WRATHImage::ImageFormatArray curve_fmt;

      m_glyph_glsl[i].m_texture_page_data_size=2;

      for(int type=0; type<WRATHTextureFont::GlyphGLSL::num_linearity_types; ++type)
        {
          if(i&with_scaling)
            {
              m_glyph_glsl[i].m_fragment_processor[type]
                .add_macro("WRATH_CURVE_ANALYTIC_STORE_SCALING");
            }
          
          if(i&two_channel)
            {
              m_glyph_glsl[i].m_fragment_processor[type]
                .add_macro("WRATH_CURVE_ANALYTIC_TWO_CHANNEL_WORK_AROUND");
            }
          
          if(i&separate_curve)
            {
              m_glyph_glsl[i].m_fragment_processor[type]
                .add_macro("WRATH_CURVE_ANALYTIC_SEPARATE_CURVES");
            }
      
          #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
            {
              m_glyph_glsl[i].m_fragment_processor[type].add_macro("WRATH_CURVE_ANALYTIC_USE_LA_LOOKUP");
            }
          #endif
        }

      


      m_glyph_glsl[i].m_vertex_processor[WRATHTextureFont::GlyphGLSL::linear_glyph_position]
        .add_source("font_curve_analytic_linear.vert.wrath-shader.glsl",
                    WRATHGLShader::from_resource);

      m_glyph_glsl[i].m_fragment_processor[WRATHTextureFont::GlyphGLSL::linear_glyph_position]
        .add_source("font_curve_analytic_base.frag.wrath-shader.glsl", WRATHGLShader::from_resource)
        .add_source("font_curve_analytic_linear.frag.wrath-shader.glsl", WRATHGLShader::from_resource);


      m_glyph_glsl[i].m_vertex_processor[WRATHTextureFont::GlyphGLSL::nonlinear_glyph_position]
        .add_source("font_curve_analytic_nonlinear.vert.wrath-shader.glsl",
                    WRATHGLShader::from_resource);

      m_glyph_glsl[i].m_fragment_processor[WRATHTextureFont::GlyphGLSL::nonlinear_glyph_position]
        .add_source("font_curve_analytic_base.frag.wrath-shader.glsl", WRATHGLShader::from_resource)
        .add_source("font_curve_analytic_nonlinear.frag.wrath-shader.glsl",
                    WRATHGLShader::from_resource);


      for(int type=0; type<WRATHTextureFont::GlyphGLSL::num_linearity_types; ++type)
        {
          if(i&with_scaling)
            {
              m_glyph_glsl[i].m_fragment_processor[type]
                .remove_macro("WRATH_CURVE_ANALYTIC_STORE_SCALING");
            }
          
          if(i&two_channel)
            {
              m_glyph_glsl[i].m_fragment_processor[type]
                .remove_macro("WRATH_CURVE_ANALYTIC_TWO_CHANNEL_WORK_AROUND");
            }
          
          if(i&separate_curve)
            {
              m_glyph_glsl[i].m_fragment_processor[type]
                .remove_macro("WRATH_CURVE_ANALYTIC_SEPARATE_CURVES");
            }
      
          #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
            {
              m_glyph_glsl[i].m_fragment_processor[type].remove_macro("WRATH_CURVE_ANALYTIC_USE_LA_LOOKUP");
            }
          #endif
        }

      m_glyph_glsl[i].m_custom_data_use.push_back(0);

      m_glyph_glsl[i].m_global_names.push_back("wrath_curve_analytic_font_compute_distance");
      m_glyph_glsl[i].m_global_names.push_back("wrath_CurveAnalyticTexCoord_Position");
      m_glyph_glsl[i].m_global_names.push_back("wrath_CurveAnalyticBottomLeft");
      m_glyph_glsl[i].m_global_names.push_back("wrath_CurveAnalyticGlyphIndex");
      build_sampler_names_and_format(i, 
                                     m_glyph_glsl[i].m_sampler_names,
                                     curve_fmt);

      m_all_data[i]=WRATHNew GeometryDataImageSet(i, curve_fmt);

    }
}


common_data_type::
~common_data_type()
{
  for(unsigned int i=0, end_i=m_all_data.size(); i<end_i; ++i)
    {
      WRATHDelete(m_all_data[i]);
    }
}

void
common_data_type::
build_sampler_names_and_format(int I,
                               std::vector<std::string> &sampler_names,
                               WRATHImage::ImageFormatArray &curve_fmt)
{
  
  int current_layer(0);
  
  sampler_names.push_back("wrath_CurveAnalyticIndexTexture");

  if(I&separate_curve)
    {
      /*
        Separate curves requires:
         - 4 channel 16F: M-Coefficients and Position (broken into 2 if two_channel workaround is on)
         - 2 channel 16F  : Q-Transformation
         - 1 channel 16F   : Scale (only if scaling information included)
         - 1 channel 8     : Next Curve ID
         - RGBA4  : Rule values
         
        Total= 5+1 or 6+1 which is 6 or 7, thus
               can mix with a coverage texture for
               those GPU's supporting (only) 8 texture
               units.

        Notes:
         - Q-Transformation and Scale can be, on paper
           combined into RGB16F [not too sure if this works on N9]
 
         - With some pain, Rule and NextIndex texture could be
           cobined into one LA8
       */

      append_rgba16f(current_layer, "wrath_CurveAnalyticM_P_Texture",
                     sampler_names, curve_fmt,
                     I&two_channel);

      append_la16f(current_layer, "wrath_CurveAnalyticQTexture",
                   sampler_names, curve_fmt);
      
      if(I&with_scaling)
        {
          append_custom(current_layer, "wrath_CurveAnalyticScaleTexture",
                        sampler_names, curve_fmt,
                        WRATHImage::ImageFormat()
                        .internal_format(HALF_FLOAT_INTERNAL_FORMAT_1CHANNEL)
                        .pixel_type(HALF_FLOAT_PIXEL_TYPE)
                        .pixel_data_format(PIXEL_TYPE_1CHANNEL)
                        .magnification_filter(GL_NEAREST)
                        .minification_filter(GL_NEAREST)
                        .automatic_mipmap_generation(false)
                        .max_mip_level(0));
        }

      append_custom(current_layer, "wrath_CurveAnalyticNextCurveTexture",
                    sampler_names, curve_fmt,
                    WRATHImage::ImageFormat()
                    .internal_format(PIXEL_TYPE_1CHANNEL)
                    .pixel_type(GL_UNSIGNED_BYTE)
                    .pixel_data_format(PIXEL_TYPE_1CHANNEL)
                    .magnification_filter(GL_NEAREST)
                    .minification_filter(GL_NEAREST)
                    .automatic_mipmap_generation(false)
                    .max_mip_level(0));

      /*
        we could compress this down to 1 byte, and then 
        combine it with NextCurveTexture...
       */
      append_custom(current_layer, "wrath_CurveAnalyticRuleTexture",
                    sampler_names, curve_fmt,
                    WRATHImage::ImageFormat()
                    .internal_format(GL_RGBA)
                    .pixel_type(GL_UNSIGNED_SHORT_4_4_4_4)
                    .pixel_data_format(GL_RGBA)
                    .magnification_filter(GL_NEAREST)
                    .minification_filter(GL_NEAREST)
                    .automatic_mipmap_generation(false)
                    .max_mip_level(0));
      
                     
    }
  else
    {
      append_rgba16f(current_layer, "wrath_CurveAnalyticABTexture",
                     sampler_names, curve_fmt,
                     I&two_channel);

      append_rgba16f(current_layer, "wrath_CurveAnalyticQTexture",
                     sampler_names, curve_fmt,
                     I&two_channel);

      if(I&with_scaling)
        {
          append_rgba16f(current_layer, "wrath_CurveAnalyticP2Texture",
                     sampler_names, curve_fmt,
                     I&two_channel);
        }
      else
        {
          append_la16f(current_layer, "wrath_CurveAnalyticP2Texture",
                       sampler_names, curve_fmt);
        }

      append_custom(current_layer, "wrath_CurveAnalyticRuleTexture",
                    sampler_names, curve_fmt,
                    WRATHImage::ImageFormat()
                    .internal_format(GL_RGBA)
                    .pixel_type(GL_UNSIGNED_SHORT_4_4_4_4)
                    .pixel_data_format(GL_RGBA)
                    .magnification_filter(GL_NEAREST)
                    .minification_filter(GL_NEAREST)
                    .automatic_mipmap_generation(false)
                    .max_mip_level(0));
    }  
}

void
common_data_type::
append_custom(int &layer, const std::string &pname,
              std::vector<std::string> &sampler_names,
              WRATHImage::ImageFormatArray &fmt,
              const WRATHImage::ImageFormat &v)
{
  fmt
    .format(layer, v);

  sampler_names.push_back(pname);
  ++layer;
}


void
common_data_type::
append_la16f(int &layer, const std::string &pname,
             std::vector<std::string> &sampler_names,
             WRATHImage::ImageFormatArray &fmt)
{
  
  append_custom(layer, pname, sampler_names, fmt,
                WRATHImage::ImageFormat()
                .internal_format(HALF_FLOAT_INTERNAL_FORMAT_2CHANNEL)
                .pixel_type(HALF_FLOAT_PIXEL_TYPE)
                .pixel_data_format(PIXEL_TYPE_2CHANNEL)
                .magnification_filter(GL_NEAREST)
                .minification_filter(GL_NEAREST)
                .automatic_mipmap_generation(false)
                .max_mip_level(0));
                                 
}


void
common_data_type::
append_rgba16f(int &layer, const std::string &pname,
               std::vector<std::string> &sampler_names,
               WRATHImage::ImageFormatArray &fmt,
               bool as_2_textures)
{
  
  if(as_2_textures)
    {
      append_la16f(layer, pname, sampler_names, fmt);
      append_la16f(layer, pname + "_2nd", sampler_names, fmt);
    }
  else
    {
      append_custom(layer, pname, sampler_names, fmt,
                    WRATHImage::ImageFormat()
                    .internal_format(HALF_FLOAT_INTERNAL_FORMAT_4CHANNEL)
                    .pixel_type(HALF_FLOAT_PIXEL_TYPE)
                    .pixel_data_format(GL_RGBA)
                    .magnification_filter(GL_NEAREST)
                    .minification_filter(GL_NEAREST)
                    .automatic_mipmap_generation(false)
                    .max_mip_level(0));
    }
}


const WRATHTextureFont::GlyphGLSL*
common_data_type::
glyph_glsl(uint32_t flags)
{
  return &m_glyph_glsl[flags];
}

WRATHImage*
common_data_type::
allocate_all_filled_index_texture(ivec2 sz)
{
  WRATHImage *R;

  /*
    should we add slack to the image (via BoundarySize?)
   */
  WRATHImage::BoundarySize bd_size;
  vecN<std::vector<uint8_t>, 1> clear_value;

  clear_value[0].push_back(completely_full_texel);

  bd_size.m_maxY=1;
  bd_size.m_maxX=1;

  R=WRATHNew WRATHImage(sz,
                        m_index_fmt,
                        bd_size,
                        m_allocator);

  R->clear_sub_image(m_index_fmt,
                     clear_value,
                     ivec2(0,0),
                     sz);

  return R;
}


///////////////////////////////////////////
//IndexTextureData methods
IndexTextureData::
IndexTextureData(TaggedOutlineData &outline_data,
                 ivec2 bitmap_size):
  m_bitmap_sz(bitmap_size),
  m_outline_data(outline_data),
  m_index_pixels( m_bitmap_sz.x()*m_bitmap_sz.y(), 0),
  m_intersection_data(boost::extents[m_bitmap_sz.x()][m_bitmap_sz.y()]),
  m_winding_values(boost::extents[m_bitmap_sz.x()][m_bitmap_sz.y()])
{
  m_outline_data.compute_analytic_values(m_intersection_data, m_reverse_components, true);
  m_outline_data.compute_winding_numbers(m_winding_values, ivec2(-1, -1));

  for(int I=0; I<m_outline_data.number_components(); ++I)
    {
      if(m_reverse_components[I])
        {
          outline_data.reverse_component(I);
        }
    }

  /*
    we need to also reverse the data of m_intersection_data
    for those records that use a curve that was reversed:
   */
  for(int x=0; x<m_bitmap_sz.x(); ++x)
    {
      for(int y=0;y<m_bitmap_sz.y(); ++y)
        {
          WRATHFreeTypeSupport::analytic_return_type &current(m_intersection_data[x][y]);
          for(int side=0; side<4; ++side)
            {
              for(std::vector<WRATHFreeTypeSupport::simple_line>::iterator
                    iter=current.m_intersecions[side].begin(), 
                    end=current.m_intersecions[side].end();
                  iter!=end; ++iter)
                {
                  int contourID;
                  contourID=iter->m_source.m_bezier->contourID();
                  if(m_reverse_components[contourID])
                    {
                      iter->observe_curve_reversal();
                    }
                }
            }
        }
    }
}

WRATHImage*
IndexTextureData::
allocate_index_texture_and_fill(const GeometryDataImageSet::allocation_location &geometry_loc)
{
  WRATHImage *R;

  WRATHassert(m_bitmap_sz.x()>=0);
  WRATHassert(m_bitmap_sz.y()>=0);

  /*
    should we add slack to the image (via BoundarySize?)
   */
  WRATHImage::BoundarySize bd_size;

  bd_size.m_maxY=1;
  bd_size.m_maxX=1;

  R=common_data().allocate_index_image(m_bitmap_sz, bd_size);

  for(int x=0;x<m_bitmap_sz.x();++x)
    {
      for(int y=0;y<m_bitmap_sz.y();++y)
        {
          uint8_t &pixel(m_index_pixels[x+ y*m_bitmap_sz.x()]);
          
          pixel=select_index(x, y, geometry_loc);
        }
    }


#if 0
  /*
    now we do something.. amusing where if a texel is 
    empty we let it checks its neighbors and decide
    which of those neighbors is best:
   */
  {
    std::vector<uint8_t> tweaked_values(m_index_pixels);
    for(int x=0;x<m_bitmap_sz.x();++x)
      {
        for(int y=0;y<m_bitmap_sz.y();++y)
          {
            uint8_t pixel(m_index_pixels[x+ y*m_bitmap_sz.x()]);
            if(pixel==completely_full_texel or pixel==completely_empty_texel)
              {
                tweaked_values[x+ y*m_bitmap_sz.x()]=hunt_neighbor_curves(geometry_loc,
                                                                          m_index_pixels, 
                                                                          x, y, m_bitmap_sz);
              }
          }
      }


    std::swap(tweaked_values, m_index_pixels);
  }
#endif


  R->respecify_sub_image(0, //layer,
                         0, //LOD,
                         R->image_format(0).m_pixel_format, //pixel format
                         m_index_pixels,
                         ivec2(0,0),
                         m_bitmap_sz);


  return R;
}


bool
IndexTextureData::
curve_hugs_edge(const WRATHFreeTypeSupport::BezierCurve *curve,
                const ivec2 &texel_bl, const ivec2 &texel_tr,
                int thresshold)
{
  if(curve->degree()!=1)
    {
      return false;
    }

  ivec2 pt0(curve->pt0()), pt1(curve->pt1());

  if(pt0.x()==pt1.x())
    {
      if( std::abs(pt0.x()-texel_bl.x())<thresshold or
          std::abs(pt0.x()-texel_tr.x())<thresshold)
        {
          return true;
        }
    }
  else if(pt0.y()==pt1.y())
    {
      if( std::abs(pt0.y()-texel_bl.y())<thresshold or
          std::abs(pt0.y()-texel_tr.y())<thresshold)
        {
          return true;
        }
    }

  return false;

}

std::pair<float, const WRATHFreeTypeSupport::BezierCurve*>
IndexTextureData::
compute_feature_importance(curve_cache &curves,
                           curve_cache::iterator iter,
                           const ivec2 &texel_bl, const ivec2 &texel_tr,
                           float texel_area)
{
  const WRATHFreeTypeSupport::BezierCurve *a(iter->first), *b(NULL);
  
  WRATHFreeType_STREAM(m_outline_data.dbg(), "\n\t\t texel ["
                       << texel_bl <<":" << texel_tr
                       << "] " 
                       << "\n\t\t\textremal points[0]= {"
                       << WRATHUtil::print_range(a->extremal_points(0).begin(),
                                                 a->extremal_points(0).end())
                       << "}\n\t\t\textremal points[1]= {"
                       << WRATHUtil::print_range(a->extremal_points(1).begin(),
                                                 a->extremal_points(1).end())
                       << "}\n\t\t\tcurve=[ "
                       << WRATHUtil::print_range(a->control_points().begin(),
                                                 a->control_points().end())
                       << "\n\t\t\tIntCount=" << iter->second.size());
  
  if(iter->second.size()>=2)
    {
      vec2 pt0, pt1;
      vecN<vec2, 2> pt2s;
      float area0, area0a, area0b, area1;

      pt0=get_point(texel_bl, texel_tr,
                    iter->second.begin()->first, //side
                    iter->second.begin()->second.front());

      pt1=get_point(texel_bl, texel_tr,
                    iter->second.rbegin()->first, //side
                    iter->second.rbegin()->second.front());


 
      pt2s=get_corner_points(texel_bl, texel_tr,
                             iter->second.begin()->first,
                             iter->second.rbegin()->first,
                             pt0);

      pt1-=pt0;
      pt2s[0]-=pt0;
      pt2s[1]-=pt0;


      area0a=0.5*std::abs( pt1.x()*pt2s[0].y() - pt2s[0].x()*pt1.y());
      area0b=0.5*std::abs( pt1.x()*pt2s[1].y() - pt2s[1].x()*pt1.y());

      area0=area0a + area0b;
      area1=texel_area-area0;

      vecN<vec2,3> TA(pt0, pt1+pt0, pt0+pt2s[0]);
      vecN<vec2,3> TB(pt0, pt1+pt0, pt0+pt2s[1]);

      WRATHFreeType_STREAM(m_outline_data.dbg(), 
                           " value=" << std::abs(area1-area0)
                           << " area0=" << area0 
                           << " area1=" << area1
                           << "\n\t\t\tarea0a=" << area0a
                           << " from TA=" << TA
                           << " sideA= " << iter->second.begin()->first
                           << "\n\t\t\tarea0b=" << area0b
                           << " from TB=" << TB
                           << " sideB= " << iter->second.rbegin()->first);
      

      return std::make_pair(std::abs(area1-area0), a);
    }
  else
    {
      WRATHassert(iter->second.size()==1);

      /*
        An end point ends inside the texel, thus we
        need to compute the "triangle" of the curve
        that uses that end point.
       */
      curve_cache::iterator neighbor;
      vec2 pt0, pt1, pt2;
      vecN<vec2, 2> pt3;
      std::pair<float, const WRATHFreeTypeSupport::BezierCurve*> return_value;

      neighbor=curves.find(m_outline_data.next_neighbor(a));
      if(neighbor==curves.end())
        {
          neighbor=curves.find(m_outline_data.prev_neighbor(a));
          if(neighbor==curves.end())
            {
              /*
                the curve goes in and out the same side,
                i.e. the curve is a quadratic.. we will be 
                lazy and pretend the area can be approximated
                by a triangle, we will use the extremal
                point of curve.
               */
              b=a;
              return_value.second=b;

              int side(iter->second.begin()->first);
              enum WRATHFreeTypeSupport::boundary_type v(static_cast<enum WRATHFreeTypeSupport::boundary_type>(side));
              enum WRATHUtil::coordinate_type side_type(WRATHFreeTypeSupport::side_type(v));
              int coord(WRATHUtil::fixed_coordinate(side_type));
              
              const char *side_type_labels[]=
                {
                  "left_boundary",
                  "right_boundary",
                  "below_boundary",
                  "above_boundary",
                  "no_boundary",
                };

              const char *coord_labels[2]=
                {
                  "x_fixed",
                  "y_fixed"
                };

              WRATHunused(side_type_labels);
              WRATHunused(coord_labels);

              WRATHFreeType_STREAM(m_outline_data.dbg(), "\n\t\t\tExtremal, side="
                                   << side_type_labels[side] << " side_type="
                                   << coord_labels[side_type] << " coord="
                                   << coord_labels[coord] << "=" << coord);

              if(a->extremal_points(coord).empty())
                {
                  /*
                    likely the pair of curve-a was tossed out because it was
                    parallel to a side and close to that side,
                    we make it so that this entry is still a canidate,
                    that will lose against all others be using 10X the
                    texel area as the area-diff value.
                   */
                  return std::make_pair(10.0f*texel_area, a);
                }
              pt0=a->extremal_points(coord).front();
              neighbor=iter;
            }
          else
            {
              b=neighbor->first;
              pt0=b->fpt1();
              return_value.second=b;
            }
        }
      else 
        {
          b=neighbor->first;
          pt0=b->fpt0();
          return_value.second=a;
        }

      int sideA, sideB;

      sideA=iter->second.begin()->first;
      sideB=neighbor->second.rbegin()->first;

      
      /*
        make sure that sideA/iter is either on
        the left or bottom side.
        This is needed because when we compute
        the area of a potential pentagon by
        computing the area of a triangle fan.
       */
      if(sideA==WRATHFreeTypeSupport::above_boundary
         or sideA==WRATHFreeTypeSupport::right_boundary)
        {
          std::swap(sideA, sideB);
          std::swap(iter, neighbor);
        }


      pt1=get_point(texel_bl, texel_tr,
                    sideA,
                    iter->second.begin()->second.front());

      pt2=get_point(texel_bl, texel_tr,
                    sideB, 
                    neighbor->second.rbegin()->second.front());


      if(sideA!=sideB)
        {
          pt3=get_corner_points(texel_bl, texel_tr, 
                                sideA, sideB,
                                pt0);
          
          float area0a, area0b, area0c, area0, area1;


          area0a=compute_area(pt0, pt1, pt3[0]);
          area0b=compute_area(pt0, pt3[0], pt3[1]);
          area0c=compute_area(pt0, pt3[1], pt2);
          
          area0=area0a + area0b + area0c;
          area1=texel_area-area0;
          return_value.first=std::abs(area1-area0);
          
          vecN<vec2,3> TA(pt0, pt1, pt3[0]);
          vecN<vec2,3> TB(pt0, pt3[0], pt3[1]);
          vecN<vec2,3> TC(pt0, pt3[1], pt2);
          
          WRATHFreeType_STREAM(m_outline_data.dbg(), 
                               " value=" << std::abs(area1-area0)
                               << " area0=" << area0 
                               << " area1=" << area1
                               << "\n\t\t\tarea0a=" << area0a
                               << " from TA=" << TA
                               << " sideA= " << sideA
                               << "\n\t\t\tarea0b=" << area0b
                               << " from TB=" << TB
                               << " sideB= " << sideB
                               << "\n\t\t\tarea0c=" << area0c
                               << " from TC=" << TC);
        }
      else
        {
          float area0, area1;

          area0=compute_area(pt0, pt1, pt2);
          area1=texel_area-area0;
          return_value.first=std::abs(area1-area0);
          
          vecN<vec2,3> T(pt0, pt1, pt2);
          
          WRATHFreeType_STREAM(m_outline_data.dbg(), 
                               " value=" << std::abs(area1-area0)
                               << " area0=" << area0 
                               << " area1=" << area1
                               << " from T=" << T
                               << " side= " << sideA);
        }


      return return_value;
    }
}


enum return_code
IndexTextureData::
sub_select_index(uint8_t &pixel,
                 const curve_cache &curves,
                 int x, int y,  
                 const GeometryDataImageSet::allocation_location &geometry_loc,
                 int winding_value)
{
  enum return_code R(routine_fail);

  switch(curves.size())
    {
    case 0:
      {
        bool is_full;
        is_full=(winding_value!=0);

        if(is_full)
          {
            pixel=static_cast<uint8_t>(completely_full_texel);
          }
        else
          {
            pixel=static_cast<uint8_t>(completely_empty_texel);
          }
        return routine_success;
      }
      break;

    case 1:
      {
        const WRATHFreeTypeSupport::BezierCurve *a, *b;
        ivec2 texel_center;
        int da, db;
        
        /*
          we need to choose do we take a and a->next_neighbor
          or a->previous_neighbor and a:
        */
        a=curves.begin()->first;
        b=m_outline_data.prev_neighbor(a);
        
        texel_center=m_outline_data.point_from_bitmap(ivec2(x, y));
        da=(texel_center - a->pt1()).L1norm();
        db=(texel_center - b->pt1()).L1norm();
        
        if(da<db)
          {
            pixel=static_cast<uint8_t>(a->curveID());
          }
        else
          {
            pixel=static_cast<uint8_t>(b->curveID());
          }
        R=routine_success;
      }
      break;
      
    case 2:
      {
        const WRATHFreeTypeSupport::BezierCurve *a, *b;
              
        a=curves.begin()->first;
        b=curves.rbegin()->first;
        
        WRATHassert(a!=b);
        if(m_outline_data.next_neighbor(a)==b)
          {
            pixel=static_cast<uint8_t>(a->curveID());
            R=routine_success;
          }
        else if(m_outline_data.next_neighbor(b)==a)
          {
            pixel=static_cast<uint8_t>(b->curveID());
            R=routine_success;
          }
      }

    }
  pixel+=geometry_loc.first.x();
  return R;
}

uint8_t
IndexTextureData::
sub_select_index_hard_case(curve_cache &curves,
                           int x, int y,
                           const ivec2 &texel_bl, 
                           const ivec2 &texel_tr,
                           const GeometryDataImageSet::allocation_location &geometry_loc)
{
  WRATHFreeType_STREAM(m_outline_data.dbg(),
                       "\n\t(glyph code=" << m_outline_data.glyph_code()
                       << "): ACK@: " << ivec2(x,y) << " cnt=" << curves.size());
  WRATHunused(x);
  WRATHunused(y);
  
  const WRATHFreeTypeSupport::BezierCurve *best_canidate(NULL);
  float current_distance(0.0f);
  float texel_area;
  
  texel_area=std::abs(texel_bl.x()-texel_tr.x())*std::abs(texel_bl.y()-texel_tr.y());
  
  for(curve_cache::iterator iter=curves.begin(),
        end=curves.end(); iter!=end; ++iter)
    {
      std::pair<float, const WRATHFreeTypeSupport::BezierCurve*> v;
      
      v=compute_feature_importance(curves, iter,
                                   texel_bl, texel_tr,
                                   texel_area);
      
      if(v.second!=NULL)
        {
          if(best_canidate==NULL or v.first<current_distance)
            {
              best_canidate=v.second;
              current_distance=v.first;
            }
        }
 
    }
  WRATHassert(best_canidate!=NULL);
  return best_canidate->curveID() + geometry_loc.first.x();
}

void
IndexTextureData::
remove_edge_huggers(curve_cache &curves,
                    const ivec2 &texel_bl, 
                    const ivec2 &texel_tr)
{
  
  int threshold(8);

  for(curve_cache::iterator iter=curves.begin(), end=curves.end(); iter!=end; )
    {
      if(curve_hugs_edge(iter->first, texel_bl, texel_tr, threshold))
        {
          curve_cache::iterator r(iter);

          ++iter;
          curves.erase(r);
        }
      else
        {
          ++iter;
        }
    }
}

bool
IndexTextureData::
intersection_should_be_used(int side,
                            const WRATHFreeTypeSupport::simple_line *intersection)
{
  /*
    because we record intersection with end points,
    we need to filter out those intersections
    that with a end point AND the curve is going
    out from the texel at the end point.
    
    side is a value from the enumeration 
    WRATHFreeTypeSupport::boundary_type

    basic beans:
     if m_intersection type is not intersect_interior,
     then get the derivative.

     From there make a dot product with the outward vector
     perpindicular to the edge named by side.
     If it is positive, remove the edge.
   */


  if(intersection->m_intersection_type==WRATHFreeTypeSupport::intersect_interior)
    {
      return true;
    }

  ivec2 deriv;

  deriv=(intersection->m_intersection_type==WRATHFreeTypeSupport::intersect_at_0)?
    intersection->m_source.m_bezier->deriv_ipt0():
    intersection->m_source.m_bezier->deriv_ipt1();

  switch(side)
    {
    case WRATHFreeTypeSupport::left_boundary:
      return deriv.x()>=0;

    case WRATHFreeTypeSupport::right_boundary:
      return deriv.x()<=0;

    case WRATHFreeTypeSupport::below_boundary:
      return deriv.y()>=0;

    case WRATHFreeTypeSupport::above_boundary:
      return deriv.y()<=0;

    default:
      return true;
    }
}


uint8_t
IndexTextureData::
select_index(int x, int y,  
             const GeometryDataImageSet::allocation_location &geometry_loc)
{
  uint8_t pixel(0);
  curve_cache curves;
  ivec2 texel_bl, texel_tr;
  WRATHFreeTypeSupport::analytic_return_type &current(m_intersection_data[x][y]);
  int winding_value(m_winding_values[x][y]);

  texel_bl=m_outline_data.point_from_bitmap(ivec2(x,y),
                                            WRATHFreeTypeSupport::bitmap_begin);

  texel_tr=m_outline_data.point_from_bitmap(ivec2(x+1,y+1),
                                            WRATHFreeTypeSupport::bitmap_begin);

  /*
    we need to build a list of _all_ curves
    that intersect the texel. 
  */
  for(int side=0; side<4; ++side)
    {
      for(std::vector<WRATHFreeTypeSupport::simple_line>::const_iterator
            iter=current.m_intersecions[side].begin(), end=current.m_intersecions[side].end();
          iter!=end; ++iter)
        {
          if(intersection_should_be_used(side, &*iter) )
            {
              curves[iter->m_source.m_bezier][side].push_back(&*iter);
            }
        }
    }

  switch(curves.size())
    {
    case 0:
    case 1:
    case 2:
      if(routine_success==sub_select_index(pixel, curves, x, y, 
                                           geometry_loc, 
                                           winding_value))
        {
          return pixel;
        }

    default:
        {
          remove_edge_huggers(curves, texel_bl, texel_tr);
          if(routine_success!=sub_select_index(pixel, curves, x, y, 
                                               geometry_loc, 
                                               winding_value))
            {
              pixel=sub_select_index_hard_case(curves, x, y, texel_bl, texel_tr, 
                                               geometry_loc);
            }
        }
    }

  return pixel;
}


void
IndexTextureData::
hunt_neighbor_curves_helper(std::set<int> &pset,
                            int x, int y,
                            int dx, int dy,
                            ivec2 sz,
                            const std::vector<uint8_t> &input)
{
  x+=dx;
  y+=dy;

  if(x>=0 and x<sz.x() and y>=0 and y<sz.y())
    {
      uint8_t p;

      p=input[x + sz.x()*y];
      if(p!=completely_empty_texel and p!=completely_full_texel)
        {
          pset.insert(p);
        }
    }
}

uint8_t
IndexTextureData::
hunt_neighbor_curves(const GeometryDataImageSet::allocation_location &geometry_loc,
                     const std::vector<uint8_t> &input,
                     int x, int y, ivec2 sz)
{
  /*
    basic idea: check each of the neighbor pixels,
    add their entry to our checking set and
    use that entry from the set which is closest
    to (x,y)
   */
  std::set<int> curve_set;
  for(int dx=-1;dx<=1; ++dx)
    {
      for(int dy=-1;dy<=1; ++dy)
        {
          hunt_neighbor_curves_helper(curve_set, 
                                      x, y, dx, dy, sz, input);
        }
    }

  ivec2 texel_center(m_outline_data.point_from_bitmap(ivec2(x,y)) );
  int best_choice(input[x+sz.x()*y]), min_distance(-1); 

  for(std::set<int>::iterator iter=curve_set.begin(),
        end=curve_set.end(); iter!=end; ++iter)
    {
      const WRATHFreeTypeSupport::BezierCurve *a;
      int curveID(*iter-geometry_loc.first.x());
      int dist;

      a=m_outline_data.bezier_curve(curveID);
      dist=(texel_center - a->pt1()).L1norm();
      if(min_distance<0 or dist<min_distance)
        {
          best_choice=*iter;
          min_distance=dist;
        } 
    }

  WRATHassert(best_choice>=0 and best_choice<=255);
  return static_cast<uint8_t>(best_choice);
}

////////////////////////////////
// CurveExtractor methods
CurveExtractor::
CurveExtractor(bool make_rotation_unitary,
               const WRATHFreeTypeSupport::BezierCurve *c,
               const TaggedOutlineData &outline_data,
               bool reverse_curve)
{
  std::vector<vec2> work_room(c->control_points().size());
  vec2 linear_coeff;



  for(unsigned int i=0, endi=c->control_points().size(); i<endi; ++i)
    {
      work_room[i]=outline_data.bitmap_from_point(c->control_points()[i],
                                                  WRATHFreeTypeSupport::bitmap_begin);
    }

  if(reverse_curve)
    {
      std::reverse(work_room.begin(), work_room.end());
      m_derivative=vecN<int64_t, 2>(-c->deriv_ipt1());
    }
  else
    {
      m_derivative=vecN<int64_t, 2>(c->deriv_ipt0());      
    }

  for(std::vector<vec2>::reverse_iterator iter=work_room.rbegin(),
        end=work_room.rend(); iter!=end; ++iter)
    {
      *iter -= work_room[0];
    }
 
  WRATHUtil::BernsteinPolynomial<vec2> poly(work_room);
  poly.generate_polynomial(work_room);

  

  m_c=(c->degree()==2);

  linear_coeff=work_room[1];
  if(m_c)
    {
      vec2 quadratic_coeff;
      float div_q;

      quadratic_coeff=work_room[2];

      if(!make_rotation_unitary)
        {
          div_q=dot(quadratic_coeff, quadratic_coeff);
        }
      else
        {
          div_q=quadratic_coeff.magnitude();
        }

      m_q=vec2(quadratic_coeff.y(), -quadratic_coeff.x());

      m_0=dot(m_q, linear_coeff)/div_q;
      m_1=dot(quadratic_coeff, linear_coeff)/div_q;

      m_q/=div_q;
      m_quad_coeff=div_q;
    }
  else
    {
      float div_q;

      m_q=linear_coeff;
      div_q=m_q.magnitude();
      m_q/=div_q;

      m_0=div_q;
      m_1=0.0f;

      m_quad_coeff=0.0f;
    }

  if(m_c)
    {
      int sgn;
      enum WRATHUtil::reverse_control_points_t tt;
      std::vector<ivec2> as_integer_polynomial;

      tt=reverse_curve?
        WRATHUtil::reverse_control_points:
        WRATHUtil::dont_reverse_control_points;
      
      as_integer_polynomial.resize(c->control_points().size());
      WRATHUtil::generate_polynomial_from_bezier(const_c_array<ivec2>(c->control_points()), 
                                                 as_integer_polynomial,
                                                 TranslateControlPointFilter(c->control_points(), tt),
                                                 tt);

      

      sgn=(m_0>0.0f)?1:-1;
      m_ray.x()=sgn*as_integer_polynomial[2].y();
      m_ray.y()=-sgn*as_integer_polynomial[2].x();

      m_accelleration=vecN<int64_t, 2>(as_integer_polynomial[2]);

      WRATHassert(m_derivative.x()==as_integer_polynomial[1].x());
      WRATHassert(m_derivative.y()==as_integer_polynomial[1].y());
    }
  else
    {
      m_ray=m_derivative;
      m_accelleration=vecN<int64_t, 2>(0, 0);
    }

}


///////////////////////////////////
// AnalyticData methods
AnalyticData::
AnalyticData(enum completely_texel_t v)
{
  /*
    the shader for curve analytic lines
    computes:

    ta_ta = pa_pb/A0_B0.

    and "ignores" a time value (ta or tb)
    if it is negative.

    So what we do is that we make it so
    that ta_tb always come out negative,
    and let the rule value do its magic
    [OR rule --> both negative means fill]
    [AND rule --> both negative means don't fill]
   */

  /*
    Set p2 to not translate. This is because
    the point fed to the shader is in "bitmap"
    coordinates and thus both coordinates
    are non-negative.
   */
  m_p2=vec2(0.0f, 0.0f);
  
  /*
    set A0 and B0 as -1 and A1, B1 as zero:
   */
  m_a0_b0_a1_b1=vec4(-1.0f, -1.0f, 0.0f, 0.0f);

  /*
    Set the rotation transformations to be the identity:
   */
  m_qa_qb=vec4(1.0f, 0.0f, 1.0f, 0.0f);

  /*
    set quad coeff's as 0.
   */
  m_quad_coeffA=m_quad_coeffB=0.0f;

  /*
    no quadratic
   */
  m_cA=m_cB=false;

  /*
    now compute the rule.
   */
  if(v==completely_full_texel)
    {
      m_rule=or_rule;
    }
  else
    {
      m_rule=and_rule;
    }
  m_tangled=false;
  m_tangential_curves=false;

  m_id_curveA=v;
  m_id_curveB=v;
}

AnalyticData::
AnalyticData(bool make_rotation_unitary,
             const TaggedOutlineData &outline_data,
             const WRATHFreeTypeSupport::BezierCurve *c)
{
  const WRATHFreeTypeSupport::BezierCurve *alpha(c);
  const WRATHFreeTypeSupport::BezierCurve *beta(outline_data.next_neighbor(alpha));

  CurveExtractor alpha_data(make_rotation_unitary, alpha, outline_data, true);
  CurveExtractor beta_data(make_rotation_unitary, beta, outline_data, false);
  
  m_id_curveA=alpha->curveID();
  m_id_curveB=beta->curveID();

  m_p2=outline_data.bitmap_from_point(alpha->fpt1(),  //should be same as beta->fpt0()...
                                      WRATHFreeTypeSupport::bitmap_begin);
  m_a0_b0_a1_b1=vec4(alpha_data.m_0, beta_data.m_0,
                     alpha_data.m_1, beta_data.m_1);

  m_qa_qb=vec4(alpha_data.m_q.x(), alpha_data.m_q.y(),
               beta_data.m_q.x(), beta_data.m_q.y());
  

  m_quad_coeffA=alpha_data.m_quad_coeff;
  m_quad_coeffB=beta_data.m_quad_coeff;

  m_cA=alpha_data.m_c;
  m_cB=beta_data.m_c;

  /*
    now compute to determine if
    the rule is AND-rule or OR-rule.
   */
  vecN<int64_t, 2> alpha_deriv, beta_deriv, Jb;
  int64_t dd;

  
  alpha_deriv=vecN<int64_t, 2>(-alpha->deriv_ipt1());
  beta_deriv=vecN<int64_t, 2>(beta->deriv_ipt0());
  Jb=apply_J(beta_deriv);
  dd=dot(Jb, alpha_deriv);
  m_tangential_curves=false;

  if(dd==0)
    {
      vec2 falpha_deriv, fbeta_deriv;
      vec2 falpha_double_deriv(0.0f, 0.0f), fbeta_double_deriv(0.0f, 0.0f);

      falpha_deriv=-alpha->deriv_fpt1();
      fbeta_deriv=beta->deriv_fpt0();


      if(beta->degree()==2)
        {
          fbeta_double_deriv=vec2( beta->curve().x()[2],
                                   beta->curve().y()[2]);

        }
      
      if(alpha->degree()==2)
        {
          falpha_double_deriv=vec2(alpha->curve().x()[2],
                                   alpha->curve().y()[2]);
        }

      float rescale, fd;

      rescale=fbeta_deriv.magnitude()/falpha_deriv.magnitude();
      //fd=dot(falpha_deriv+falpha_double_deriv,
      //     apply_J( fbeta_deriv*rescale + fbeta_double_deriv  ));

      fd=dot( apply_J(fbeta_double_deriv), falpha_deriv)
        + rescale*dot( apply_J(fbeta_deriv), falpha_double_deriv);

      if(fd<0.0f)
        {
          m_rule=or_rule;
        }
      else
        {
          m_rule=and_rule;
        }
      m_tangential_curves=true;

    }
  else if(dd<0)
    {
      m_rule=or_rule;
    }
  else
    {
      m_rule=and_rule;
    }

  
  m_tangled=(alpha_data.m_c and alpha_data.tangled(beta_data.m_ray))
    or (beta_data.m_c and beta_data.tangled(alpha_data.m_ray));
  
  if(!m_tangential_curves)
    {
      m_tangled=m_tangled
        or (alpha_data.m_c and alpha_data.tangled(beta_data.m_derivative))
        or (beta_data.m_c and beta_data.tangled(alpha_data.m_derivative));
    }
  else if(alpha_data.m_c and beta_data.m_c 
          and false)
    {
      m_tangled=m_tangled
        or alpha_data.tangled(beta_data.m_accelleration)
        or beta_data.tangled(alpha_data.m_accelleration);
    }
  
  

  /*
  if(m_tangled)
    {
      WRATHFreeType_STREAM(outline_data.dbg(),
                           "\n\t(glyph code=" << outline_data.glyph_code()
                           << "): tangled");
    }
  */
    
}

///////////////////////////
// AnalyticDataPacket methods
AnalyticDataPacket::
AnalyticDataPacket(uint32_t flags,
                   const TaggedOutlineData &outline_data,
                   int curve_offset)
{
  
  std::vector<AnalyticData> curve_sets;

  curve_sets.reserve(outline_data.number_curves());
  for(unsigned int i=0, endi=outline_data.number_curves(); i<endi; ++i)
    {
      /*
        the test for if a curve should be 
        considered reversed is not reliable...
       */
      curve_sets.push_back( AnalyticData(flags&with_scaling,
                                         outline_data, 
                                         outline_data.bezier_curve(i)) );
    }
  pack(flags, curve_sets, curve_offset);
}

AnalyticDataPacket::
AnalyticDataPacket(uint32_t flags,
                   enum completely_texel_t v)
{
  std::vector<AnalyticData> one_curve;

  one_curve.push_back(AnalyticData(v));

  pack(flags, one_curve, 0);
}


void
AnalyticDataPacket::
pack(uint32_t flags,
     const std::vector<AnalyticData> &curve_sets,
     int curve_offset)
{
  unsigned int N(curve_sets.size());
  

  if(N==0)
    {
      return;
    }

  c_array<uint16_t> ca_cb_rule;
  
  
  /*
    now go through the lovely process of getting the numbers
    and packing them into bytes for the layers:
   */
  if(flags&separate_curve)
    {
      c_array<vecN<uint16_t,2> >  q;
      c_array<uint8_t> next;

      if(flags&two_channel)
        {
          c_array<vecN<uint16_t,2> > m, p2;

          m=add_layer< vecN<uint16_t,2> >(N);
          p2=add_layer< vecN<uint16_t,2> >(N);
          for(unsigned int i=0; i<N; ++i)
            {
              const AnalyticData &numbers(curve_sets[i]);

              WRATHUtil::convert_to_halfp_from_float(m[i], numbers.a0_a1());
              WRATHUtil::convert_to_halfp_from_float(p2[i], numbers.m_p2);
            }
        }
      else
        {
          c_array<vecN<uint16_t,4> > m_p2;

          m_p2=add_layer< vecN<uint16_t,4> >(N);
          for(unsigned int i=0; i<N; ++i)
            {
              const AnalyticData &numbers(curve_sets[i]);
              WRATHUtil::convert_to_halfp_from_float(m_p2[i], numbers.a0_a1_p2());
            }
        }

      q=add_layer< vecN<uint16_t,2> >(N); 
      for(unsigned int i=0; i<N; ++i)
        {
          const AnalyticData &numbers(curve_sets[i]);
          WRATHUtil::convert_to_halfp_from_float(q[i], numbers.qa());
        }

      if(flags&with_scaling)
        {
          c_array< vecN<uint16_t,1> > scale;

          scale=add_layer<vecN<uint16_t,1> >(N);
          for(unsigned int i=0; i<N; ++i)
            {
              const AnalyticData &numbers(curve_sets[i]);
              vecN<float, 1> v(numbers.m_quad_coeffA);

              WRATHUtil::convert_to_halfp_from_float(scale[i], v);
            }
          
        }
      
      next=add_layer<uint8_t>(N);
      for(unsigned int i=0; i<N; ++i)
        {
          const AnalyticData &numbers(curve_sets[i]);
          int v(curve_offset+numbers.m_id_curveB);

          WRATHassert( (curve_offset==0 and (v==0 or v==255))  
                       or (curve_offset>0));

          next[i]=static_cast<uint8_t>(v);
      
        }

      ca_cb_rule=add_layer<uint16_t>(N);
    }
  else
    {
      if(flags&two_channel)
        {
          c_array<vecN<uint16_t, 2> > a0_b0, a1_b1, qa, qb;
          c_array<vecN<uint16_t, 2> > p2;

          a0_b0=add_layer< vecN<uint16_t,2> >(N);
          a1_b1=add_layer< vecN<uint16_t,2> >(N);
          qa=add_layer< vecN<uint16_t,2> >(N);
          qb=add_layer< vecN<uint16_t,2> >(N);
          p2=add_layer< vecN<uint16_t,2> >(N);

          for(unsigned int i=0; i<N; ++i)
            {
              const AnalyticData &numbers(curve_sets[i]);
              
              WRATHUtil::convert_to_halfp_from_float(a0_b0[i], numbers.a0_b0());
              WRATHUtil::convert_to_halfp_from_float(a1_b1[i], numbers.a1_b1());
              WRATHUtil::convert_to_halfp_from_float(qa[i], numbers.qa());
              WRATHUtil::convert_to_halfp_from_float(qb[i], numbers.qb());
              WRATHUtil::convert_to_halfp_from_float(p2[i], numbers.m_p2);
            }

          if(flags&with_scaling)
            {
              c_array<vecN<uint16_t, 2> > scale_ab;

              scale_ab=add_layer< vecN<uint16_t,2> >(N);
              for(unsigned int i=0; i<N; ++i)
                {
                  const AnalyticData &numbers(curve_sets[i]);
                  vec2 scaling(numbers.m_quad_coeffA, numbers.m_quad_coeffB);

                  WRATHUtil::convert_to_halfp_from_float(scale_ab[i], scaling);
                }
            }
        }
      else
        {
          c_array<vecN<uint16_t, 4> > a0_b0_a1_b1, qa_qb;

          a0_b0_a1_b1=add_layer< vecN<uint16_t,4> >(N);
          qa_qb=add_layer< vecN<uint16_t,4> >(N);
          for(unsigned int i=0; i<N; ++i)
            {
              const AnalyticData &numbers(curve_sets[i]);

              WRATHUtil::convert_to_halfp_from_float(a0_b0_a1_b1[i], numbers.m_a0_b0_a1_b1);
              WRATHUtil::convert_to_halfp_from_float(qa_qb[i], numbers.m_qa_qb);
            }

          if(flags&with_scaling)
            {
              c_array<vecN<uint16_t, 4> > p2_scale;
              
              p2_scale=add_layer< vecN<uint16_t,4> >(N);
              for(unsigned int i=0; i<N; ++i)
                {
                  const AnalyticData &numbers(curve_sets[i]);

                  WRATHUtil::convert_to_halfp_from_float(p2_scale[i], numbers.p2_scale_ab());
                }
            }
          else
            {
              c_array<vecN<uint16_t, 2> > p2;

              p2=add_layer< vecN<uint16_t,2> >(N);
              for(unsigned int i=0; i<N; ++i)
                {
                  const AnalyticData &numbers(curve_sets[i]);

                  WRATHUtil::convert_to_halfp_from_float(p2[i], numbers.m_p2); 
                }
            }
        }
      ca_cb_rule=add_layer<uint16_t>(N);
    }

  
  for(unsigned int i=0; i<N; ++i)
    {
      const AnalyticData &numbers(curve_sets[i]);
      
      ca_cb_rule[i]=0;
      if(numbers.m_cA)
        {
          ca_cb_rule[i]|=(15<<12);
        }

      if(numbers.m_cB)
        {
          ca_cb_rule[i]|=(15<<8);
        }

      if(numbers.m_rule==and_rule)
        {
          ca_cb_rule[i]|=(15<<4);
        }
      
      if(numbers.m_tangled)
        {
          ca_cb_rule[i]|=15;
        }
    }
}


void
AnalyticDataPacket::
relieve_layers(std::vector< std::vector<uint8_t> > &bytes)
{
  std::list<LayerBytes>::iterator iter, end;
  int L;

  bytes.resize(m_layers.size());
  for(iter=m_layers.begin(), end=m_layers.end(), L=0; iter!=end; ++iter, ++L)
    {
      WRATHassert(iter->first==L);
      std::swap(iter->second, bytes[L]);
    }
}


/////////////////////////
// GeometryDataImage methods
GeometryDataImage::
GeometryDataImage(const WRATHImage::ImageFormatArray &fmt,
                  uint32_t flags,
                  const_c_array<std::vector<uint8_t> > always_on_bits,
                  const_c_array<std::vector<uint8_t> > always_off_bits):
  m_flags(flags),
  m_current_y(0)
{
  /*
    Create m_image to have the entire 256x256
    texture to itself.
   */
  m_image=WRATHNew LocalImage(this, fmt);
  common_data().note_new_curve_texture();


  m_image->clear_sub_image(fmt,
                           always_off_bits,
                           ivec2(completely_empty_texel, 0),
                           ivec2(1, 256));

  m_image->clear_sub_image(fmt,
                           always_on_bits,
                           ivec2(completely_full_texel, 0),
                           ivec2(1, 256));
                  
}

GeometryDataImage::
~GeometryDataImage()
{
  if(m_image!=NULL)
    {
      WRATHDelete(m_image);
    }
}


ivec2
GeometryDataImage::
allocate(int num_pts)
{
  WRATHassert(max_allocate_allowed()>=num_pts);

  std::multimap<int, int>::iterator iter;
  ivec2 return_value;

  iter=m_finder.lower_bound(num_pts);
  if(iter!=m_finder.end())
    {
      return_value.y()=iter->second;
      m_finder.erase(iter);
    }
  else
    {
      return_value.y()=m_current_y;
      ++m_current_y;
    }

  WRATHassert(return_value.y()>=0 and return_value.y()<=255);
  
  return_value.x()=m_lines[return_value.y()].allocate(num_pts); 
  update_finder(return_value.y());

  common_data().note_curve_texture_utilization(num_pts);

  return return_value;  
}

void
GeometryDataImage::
set_values(ivec2 xy,
           const TaggedOutlineData &raw_outline_data)
{
  /*
    Store the curves of raw_outline_data
    to [R.m_begin, R.m_end]x{xy.y}
    where R.m_begin=xy.x and R.m_end=xy.x + raw_outline_data.number_curves()

    The data stored is that at texel(i+R.m_begin, y)
    we store:
      alpha values come from raw_outline_data.curve(i)
      and beta values come from raw_outline_data.curve(i)->next_neighbor()
   */
  AnalyticDataPacket pkt(m_flags, raw_outline_data, xy.x());
  for(std::list<AnalyticDataPacket::LayerBytes>::iterator 
        iter=pkt.m_layers.begin(), end=pkt.m_layers.end();
      iter!=end; ++iter)
    {
      std::vector<uint8_t> &raw_bytes(iter->second);
      int layer(iter->first);

      m_image->respecify_sub_image(layer, //layer
                                   0, //LOD
                                   m_image->image_format()[layer].m_pixel_format, //pixel format
                                   raw_bytes, //pixel data
                                   xy, //position
                                   ivec2(raw_outline_data.number_curves(), 1)); //size
    }

  
  
}


///////////////////////////////
// GeometryDataImageSet methods
GeometryDataImageSet::
GeometryDataImageSet(uint32_t flags,
                     const WRATHImage::ImageFormatArray &fmt):
  m_separate_curves(flags&separate_curve),
  m_flags(flags),
  m_fmt(fmt)
{
  
  /*
     set m_always_on_bits and m_always_off_bits,
     these are passed to the ctor of GeometryDataImage
     when the set makes an GeometryDataImage to
     initialize the 0'th and 1'st column.
   */
  AnalyticDataPacket off(m_flags, completely_empty_texel);
  AnalyticDataPacket on(m_flags, completely_full_texel);

  off.relieve_layers(m_always_off_bits);
  on.relieve_layers(m_always_on_bits);

  WRATHassert(m_always_off_bits.size()==m_always_on_bits.size());

}

GeometryDataImageSet::
~GeometryDataImageSet()
{
  for(std::vector<GeometryDataImage*>::iterator iter=m_pool.begin(),
        end=m_pool.end(); iter!=end; ++iter)
    {
      if(*iter!=NULL) 
        {
          WRATHDelete(*iter);
        }
    }
}

GeometryDataImageSet::allocation_location
GeometryDataImageSet::
allocate_and_fill(const TaggedOutlineData &pdata)
{
  allocation_location R;
  
  R=allocate(pdata.number_curves());
  R.second->set_values(R.first, pdata);
  
  return R;
}

GeometryDataImageSet::allocation_location
GeometryDataImageSet::
allocate(int cnt)
{
  WRATHAutoLockMutex(m_mutex);
  
  std::multimap<int, GeometryDataImage*>::iterator iter;
  GeometryDataImage *pImage(NULL);
  GeometryDataImageSet::allocation_location return_value;

  iter=m_finder.lower_bound(cnt);
  if(iter!=m_finder.end())
    {
      pImage=iter->second;
      m_finder.erase(iter);
    }
  else
    {
      pImage=WRATHNew GeometryDataImage(m_fmt, m_flags,
                                        m_always_on_bits, m_always_off_bits);
      m_pool.push_back(pImage);
    }

  return_value.first=pImage->allocate(cnt);
  return_value.second=pImage;
  update_finder(pImage);

  return return_value;
}



 


///////////////////////////////////////
// WRATHTextureFontFreeType_CurveAnalytic methods
WRATHTextureFontFreeType_CurveAnalytic::
WRATHTextureFontFreeType_CurveAnalytic(WRATHFreeTypeSupport::LockableFace::handle pface, 
                                       const WRATHTextureFontKey &presource_name):
  WRATHTextureFontFreeTypeT<WRATHTextureFontFreeType_CurveAnalytic>(pface, presource_name),
  m_flags(common_data().current_flags()),
  m_curvature_collapse(curvature_collapse())
{
  WRATHassert((ttf_face()->face()->face_flags&FT_FACE_FLAG_SCALABLE)!=0);
  m_page_tracker.connect(boost::bind(&WRATHTextureFontFreeType_CurveAnalytic::on_create_texture_page, this,
                                     _2, _4));
}


  
WRATHTextureFontFreeType_CurveAnalytic::
~WRATHTextureFontFreeType_CurveAnalytic()
{
#if defined(WRATH_FONT_GENERATION_STATS)
  /*
    I want to know how long it took to 
    generate the glyphs on average
   */
  std::cout << "[CurveAnalytic]" << simple_name() << " "
            << glyph_data_stats()
            << " spread across " 
            << m_page_tracker.number_texture_pages()
            << " pages\n";
#endif
}


float
WRATHTextureFontFreeType_CurveAnalytic::
normalized_glyph_code_value(const glyph_data_type &G)
{
  return G.fetch_custom_float(0);
}

int
WRATHTextureFontFreeType_CurveAnalytic::
number_texture_pages(void)
{
  return m_page_tracker.number_texture_pages();
}

const WRATHTextureFont::GlyphGLSL*
WRATHTextureFontFreeType_CurveAnalytic::
glyph_glsl(void)
{
  return common_data().glyph_glsl(m_flags);
}

void
WRATHTextureFontFreeType_CurveAnalytic::
on_create_texture_page(ivec2 texture_size,
                       std::vector<float> &custom_data)
{
  custom_data.resize(2);
  custom_data[0]=1.0f/static_cast<float>(std::max(1, texture_size.x()) );
  custom_data[1]=1.0f/static_cast<float>(std::max(1, texture_size.y()) );
}

int
WRATHTextureFontFreeType_CurveAnalytic::
texture_page_data_size(void) const
{
  return 2; //reciprocal texture size
}

float
WRATHTextureFontFreeType_CurveAnalytic::
texture_page_data(int texture_page, int idx) const
{
  return (0<=idx and idx<2)?
    m_page_tracker.custom_data(texture_page)[idx]:
    0;
}


const_c_array<WRATHTextureChoice::texture_base::handle>
WRATHTextureFontFreeType_CurveAnalytic::
texture_binder(int texture_page)
{
  return m_page_tracker.texture_binder(texture_page);
}

GLint
WRATHTextureFontFreeType_CurveAnalytic::
texture_creation_size(void)
{
  return common_data().texture_creation_size();
}

GLint
WRATHTextureFontFreeType_CurveAnalytic::
effective_texture_creation_size(void)
{
  return common_data().effective_texture_creation_size();
}

bool
WRATHTextureFontFreeType_CurveAnalytic::
force_power2_texture(void)
{
  return common_data().force_power2_texture();
}

bool
WRATHTextureFontFreeType_CurveAnalytic::
include_scaling_data(void)
{
  return common_data().include_scaling_data();
}

void
WRATHTextureFontFreeType_CurveAnalytic::
texture_creation_size(GLint v)
{
  common_data().texture_creation_size(v);
}

void
WRATHTextureFontFreeType_CurveAnalytic::
force_power2_texture(bool v)
{
  common_data().force_power2_texture(v);
}

void
WRATHTextureFontFreeType_CurveAnalytic::
include_scaling_data(bool v)
{
  common_data().include_scaling_data(v);
}

WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
WRATHTextureFontFreeType_CurveAnalytic::
texture_consumption_curve(void)
{
  return common_data().texture_consumption_curve();
}

WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
WRATHTextureFontFreeType_CurveAnalytic::
texture_consumption_index(void)
{
  return common_data().texture_consumption_index();
}

void
WRATHTextureFontFreeType_CurveAnalytic::
two_channel_texture_work_around(bool v)
{
  common_data().two_channel_texture_work_around(v);
}

bool
WRATHTextureFontFreeType_CurveAnalytic::
two_channel_texture_work_around(void)
{
  return common_data().two_channel_texture_work_around();
}

void
WRATHTextureFontFreeType_CurveAnalytic::
curvature_collapse(float v)
{
  common_data().curvature_collapse(v);
}

float
WRATHTextureFontFreeType_CurveAnalytic::
curvature_collapse(void)
{
  return common_data().curvature_collapse();
}

void
WRATHTextureFontFreeType_CurveAnalytic::
store_separate_curves(bool v)
{
  common_data().store_separate_curves(v);
}

bool
WRATHTextureFontFreeType_CurveAnalytic::
store_separate_curves(void)
{
  return common_data().store_separate_curves();
}



WRATHTextureFont::glyph_data_type*
WRATHTextureFontFreeType_CurveAnalytic::
generate_character(WRATHTextureFont::glyph_index_type G)
{
  local_glyph_data_type *return_value(NULL);
  ivec2 glyph_advance;
  ivec2 bitmap_sz, bitmap_offset;
  /*
    Step 1: use FreeType to load the glyph data:    
  */
  
  WRATHLockMutex(ttf_face()->mutex());

  FT_Set_Pixel_Sizes(ttf_face()->face(), pixel_size(), pixel_size());
  FT_Set_Transform(ttf_face()->face(), NULL, NULL);
  /*
    hinting helps prevent multiple end point in a single texel..
   */
  FT_Load_Glyph(ttf_face()->face(), G.value(), FT_LOAD_DEFAULT);
  FT_Render_Glyph(ttf_face()->face()->glyph, FT_RENDER_MODE_NORMAL);

  glyph_advance=ivec2(ttf_face()->face()->glyph->advance.x,
                      ttf_face()->face()->glyph->advance.y);

  bitmap_sz=ivec2(ttf_face()->face()->glyph->bitmap.width,
                  ttf_face()->face()->glyph->bitmap.rows);

  bitmap_offset=ivec2(ttf_face()->face()->glyph->bitmap_left,
                      ttf_face()->face()->glyph->bitmap_top 
                      - ttf_face()->face()->glyph->bitmap.rows);

  /*
    Get the curve data.
   */
  std::vector<WRATHFreeTypeSupport::point_type> pts;
  MakeEvenFilter::handle filter(WRATHNew MakeEvenFilter());
  #ifdef WRATHDEBUG
    WRATHFreeTypeSupport::geometry_data gmt(&std::cout, pts, filter);
  #else
    WRATHFreeTypeSupport::geometry_data gmt(NULL, pts, filter);
  #endif
    /*
      usually we set the inflate factor to be 4,
      from that:
       - we want all end points of curves to be even integers
       - some points in the outline from FreeType are given implicitely
         as an _average_ or 2 points
      However, we generate quadratics from cubics which
      generates end points with a divide by _64_, so we make the 
      scale factor that much bigger. However we also need to
      make sure we do not overflow. So we need to check what
      is the size of the glyph and proceed from there..
    */
  int outline_scale_factor(4);

  WRATHFreeTypeSupport::CoordinateConverter coordinate_converter(outline_scale_factor, bitmap_sz, bitmap_offset, 0);
  CollapsingContourEmitter contour_emitter(m_curvature_collapse,
                                           ttf_face()->face()->glyph->outline, 
                                           coordinate_converter,
                                           G.value());
  TaggedOutlineData outline_data(&contour_emitter, gmt);
  
  WRATHUnlockMutex(ttf_face()->mutex());//no longer will refer the the FT_Face now.
  

  WRATHImage *pIndex;
  GeometryDataImageSet::allocation_location geometry_loc;

  if(outline_data.number_curves()<=254)
    {
      /*
        Get a location to pack the curve data
        and pack the curve data.
      */
      IndexTextureData index_generator(outline_data, bitmap_sz);
      geometry_loc=common_data().get_geometry_data_set(m_flags).allocate_and_fill(outline_data);

      pIndex=index_generator.allocate_index_texture_and_fill(geometry_loc);
    }
  else
    {
      /*
        if there are too many curves to fit on one raster
        line of the curve texture, we need to do "something"
        
        this is the meaning of pain,
        we need to split the glyph into regions
        where each region has no more than
        254 curves in use. The regions should also
        be disjoint. Once we have done that, then
        we make the glyph's main rectangle as
        an empty rectangle and the regions each as
        a minor rectangles of the glyph.. TODO!
       */

      /*
        cheese muffin for now, just make the glyph all black...
       */
      WRATHwarning("Warning Glyph#" << G.value()
                   << " (character code=" 
                   << character_code(G).m_value
                   << ") of font \"" 
                   << simple_name()
                   << "\" is too complicated!");

      geometry_loc.first=ivec2(0,0);
      geometry_loc.second=NULL;
      pIndex=common_data().allocate_all_filled_index_texture(bitmap_sz);
    }
  

  /*
    Now finally allocate the glyph.
   */
  return_value=WRATHNew local_glyph_data_type(pIndex, geometry_loc, 
                                              outline_data.number_curves());
  
  /*
    Get the texture page and set the glyph properties.
   */
  int pg;
  

  if(geometry_loc.second!=NULL)
    {
      vecN<WRATHImage*, 1> data_image(geometry_loc.second->image());
      pg=m_page_tracker.get_page_number(pIndex, data_image);
    }
  else
    {
      pg=m_page_tracker.get_page_number(pIndex);
    }


  glyph_data_type &glyph(*return_value);
  glyph
    .font(this)
    .iadvance(glyph_advance)
    .texture_page(pg)
    .texel_values(pIndex->minX_minY(), pIndex->size())
    .origin(bitmap_offset)
    .bounding_box_size(pIndex->size())
    .character_code(character_code(G))
    .glyph_index(G);

  return return_value;

}
