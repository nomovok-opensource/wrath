/*! 
 * \file KANTextureFontTTF.cpp
 * \brief file KANTextureFontTTF.cpp
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


#include <sstream>
#include <limits>
#include <math.h>
#include <fstream>
#include <boost/multi_array.hpp>
#include <sys/time.h>
#include "WRATHTextureFontTTF.hpp"
#include "WRATHUtil.hpp"
#include "glGet.hpp"
#include "c_array.hpp"
#include "ostream_utility.hpp"
#include "WRATHGLProgram.hpp"



/********************************************

Explanation of algorithm:

  Distance texture stores at a pixel p
the signed taxi-cab distance to the outline of the
font, where the sign is negative if the pixel
is outside and positive if the pixel is inside.

 We compute the taxi-cab distance to the outline.

 Naively, it is:

 d(p,outline)= min { d(B,p) | B is bezier curve of outline }

 However there are lots of shortcuts we can take:

firstly, the distance function is not smooth:

  d( (x,y), (a,b) ) = |x-a| + |y-b|

for a curve (a(t),b(t)), minimize f on 0<=t<=1 for:

 f(t) = | x-a(t)| + |y-b(t)|

has it's minimum at the a point where the derivative
of f does not exists or where it's derivative is
0 or when t=0 or when t=1.

points where the derivative does not exist corresponds
to
 x=a(t) or y=b(t)

which means we only need to compute those
points O(width) + O(height) times, this is done 
in OutlineData::compute_fixed_lines().

The points where the derivative of f is zero
or when t=0 is handled in OutlineData::compute_point_list().
The pount t=1 does not need to be handled because
the next Bezeir curve in the outline's t=0 is
the current curves t=1.

Optimizations:

 A point of the outline is really only worth
considering to minimize the distance if it 
is within 2 pixels of the pixel p, thus
in OutlineData::compute_point_list(), we do:

for(each curve B)
{
  for(each critical point of B and t=0, B(t))
    {
      for(those (x,y) within 2 pixels of B(t))
        {
          do_calculation()
        }
    }
}

Thus the minimizing for the critical
points of all the curves and the points
of the outline is done in O(number points) time.
  
  In compute_fixed_lines(), we do:

for(each x of bitmap)
  {
    for(each curve B)
      {
        add points of B that intersect vertical line with x-coordinate x to a list L
      }
    sort L
    for(each y of bitmap)
      {
         track and index in L so that points after index are bigger than y
         check distance to current index and previous index
      }
  }                          
then similar switching roles of x and y.
  
Thus that computation is at worst O(B*width + height*width)

Hence total computation time is 
O( N*(width+height)+width*height), N=#points of outline.


Other important tricks:
  (1) points of out line are stored as integers multiplied by 4, 
  (2) center points of bitmap are of the form 4*N + 1, this way
      center points of bitmap never have same coordinate as 
  any point of the font, this is needed to get a reliable inside/outside
  test usingthe vertical and horizontal lines.
  (3) roots are counted with multiplicity, this too is needed
  to get a reliable inside/outside test usingthe vertical and 
  horizontal lines.

  (4) solvers work on integers, so that remove exact zero and one
  roots and also remove exactly when results are not within
  the range (0,1).

  NOTE: the solver for cubics does not have the analytic exact 
  ability to check if a root is between (0,1) and 
  relies on the floating point representation.

 **********************************************/


                            

                              

#define DEFAULT_PIXEL_VALUE pixel_type(0,0,0,0)

namespace
{
  bool&
  sm_use_mipmapping(void)
  {
    static bool R(false);
    return R;
  }

  GLint&
  sm_texture_creation_width(void)
  {
    static GLint v(1024);
    return v;
  }

  int
  number_mipmaps(ivec2 sz)
  {
    int m;
    for(m=0; sz.x()>1 and sz.y()>1; ++m, sz.x()/=2, sz.y()/=2)
      {}
    return m;
  }

  int 
  glyph_size_round_up(int I)
  {
    int v;

    v= (I<4 or (I%4)==0)?
    I:
    I+4-(I%4);

    return v;
  }

  inline
  GLubyte
  pixel_value_from_distance(float dist, bool outside)
  {  
    GLubyte v;

    v=static_cast<GLubyte>( std::min(static_cast<int>(127.0f*dist), 127));
    
    //note that 127 is "-0" and 128 is "+0".
    return 
    (outside)?
    127-v:
    128+v;
  }

  uint32_t
  time_difference(const struct timeval &end, const struct timeval &begin)
  {
    return (end.tv_sec-begin.tv_sec)*1000+
        (end.tv_usec-begin.tv_usec)/1000;
  }

  class geometry_data
  {
  public:
    geometry_data(std::ostream &ostr, 
                  std::vector<WRATHTextureFontTTF::point_type> &pts,
                  std::vector<range_type<GLushort> > &inds):
      m_debug_stream(ostr),
      m_pt_array(pts),
      m_index_array(inds)
    {}

    std::ostream&
    stream(void) const
    {
      return m_debug_stream;
    }

    std::vector<WRATHTextureFontTTF::point_type>&
    pts(void) const
    {
      return m_pt_array;
    }

    ivec2
    pt(int I) const
    {
      return ivec2(m_pt_array[I].position().x(),
                   m_pt_array[I].position().y());
    }

    enum WRATHTextureFontTTF::point_type::point_classification
    tag(int I) const
    {
      return m_pt_array[I].classification();
    }

    GLushort
    push_back(const ivec2 &in_pt, char in_tag) const
    {
      enum WRATHTextureFontTTF::point_type::point_classification cl;
      char curve_tag( FT_CURVE_TAG(in_tag));

      switch(curve_tag)
        {
        default:
        case FT_CURVE_TAG_ON:
          cl=WRATHTextureFontTTF::point_type::on_curve;
          break;

        case FT_CURVE_TAG_CONIC:
          cl=WRATHTextureFontTTF::point_type::conic_off_curve;
          break;

        case FT_CURVE_TAG_CUBIC:
          cl=WRATHTextureFontTTF::point_type::cubic_off_curve;
        }

      GLushort return_value(m_pt_array.size());
      WRATHTextureFontTTF::point_type v(in_pt, cl);

      m_pt_array.push_back(v);
      return return_value;

    }

    std::vector<range_type<GLushort> >&
    indices(void) const
    {
      return m_index_array;
    }

  private:
    std::ostream &m_debug_stream;
    std::vector<WRATHTextureFontTTF::point_type> &m_pt_array;
    std::vector<range_type<GLushort> > &m_index_array;
  };

  
  class BezierCurve;
  class distance_tracker
  {
  public:

    distance_tracker(void):
      m_ready(false)
    {}

    void
    init(float v)
    {
      m_value=v;
      m_value2=v;
      m_value2_has_meaning=false;
      m_init_value=v;

      m_ready=false;
      m_corner_distances.clear();
      m_curve_distances.clear();
    }

    
    void
    update_value(float v, const BezierCurve *curve);

    void
    update_value(float v, const BezierCurve *curve_c0, const BezierCurve *curve_c1);

    void
    finalize(void);

    float
    value(void) const
    {
      WRATHassert(m_ready);
      return m_value;
    }

    float
    value2(void) const
    {
      WRATHassert(m_ready);
      return m_value2_has_meaning?
        m_value2:
        m_value;
    }

    bool
    value2_has_meaning(void) const
    {
      return m_value2_has_meaning;
    }

  private:
    typedef std::pair<const BezierCurve*, const BezierCurve*> corner_key;
    typedef const BezierCurve* curve_key;

    std::map<corner_key, float> m_corner_distances;
    std::map<curve_key, float> m_curve_distances;

    float m_value, m_value2, m_init_value;
    bool m_value2_has_meaning, m_ready;

    void 
    use_corner_value(std::map<corner_key, float>::iterator);

    void
    use_curve_value(std::map<curve_key, float>::iterator);
  };

  
  class inside_outside_test_results
  {
  private:
    ivec4 m_solution_count;

  public:

    inside_outside_test_results(void):
      m_solution_count(0,0,0,0)
    {}

    enum sol_type
      {
        left,
        right, 
        above,
        below,
      };
    
    void
    reset(void)
    {
      m_solution_count=ivec4(0,0,0,0);
    }

    int
    raw_value(enum sol_type tp)
    {
      return m_solution_count[tp];
    }

    void
    increment(enum sol_type tp, int ct)
    {
      m_solution_count[tp]+=ct;
    }

    bool
    reliable_test(void) const
    {
      return (m_solution_count[0]&1)==(m_solution_count[1]&1)
        and (m_solution_count[0]&1)==(m_solution_count[2]&1)
        and (m_solution_count[0]&1)==(m_solution_count[3]&1);
    }

    
    bool
    inside(void) const
    {
      /**/
      int votes_inside(0);

      for(unsigned int i=0;i<m_solution_count.size();++i)
        {
          votes_inside+=(m_solution_count[i]&1);
        }

      return votes_inside>=2;
      /**/
      
    }

    bool
    outside(void) const
    {
      return !inside();
    }

    friend
    std::ostream&
    operator<<(std::ostream &ostr, const inside_outside_test_results &obj)
    {
      ostr << obj.m_solution_count;
      return ostr;
    }
  };

  class distance_return_type
  {
  public:    
    distance_tracker m_distance;
    inside_outside_test_results m_solution_count;
  };


  
  /*!
    Generate the polynomail curves from an array of FT_Vector points.
    The return value indicates the degree of the polynomial.

    The return_value std::vector output is packed as follows:

     return_value.x() holds the polynomial for the x-coordiante as follows:
     x(t) = return_value.x()[0] + t*return_value.x()[1] + t*t*return_value.x()[2] + .. 

     Degree 1 B(p0, p1, t)         = (1-t)p0 + t*p1
     Degree 2 B(p0, p1, p2, t)     = (1-t)(1-t)p0 + 2t(1-t)p1 + t*t*p2 
     Degree 3 B(p0, p1, p2, p3, t) = (1-t)(1-t)(1-t)p0 + 3(1-t)(1-t)tp1 + 3(1-t)t*t*p2 + t*t*t*p3
    
   */
  void 
  generate_polynomial_from_bezier(const_c_array<ivec2> pts, vecN<std::vector<int>, 2> &return_value);

  
  class solution_point
  {
  public:
    int m_multiplicity;
    float m_value;
    const BezierCurve *m_bezier;

    solution_point(int multiplicity, float v, const BezierCurve *cv):
      m_multiplicity(multiplicity),
      m_value(v),
      m_bezier(cv)
    {}

    bool
    operator<(const solution_point &obj) const
    {
      return m_value<obj.m_value;
    }

    friend
    std::ostream&
    operator<<(std::ostream &ostr, const solution_point &obj)
    {
      ostr << "(v=" << obj.m_value 
           << ", mult=" << obj.m_multiplicity
           << ")";
      return ostr;
    }
  };

  
  enum recourd_route_type
    {
      record_only_of_0_1,
      record_all,
    };
  

  /*!
    Each of solve_linear, solve_quadratic and solve_cubic
    solve for a polynomial, if the enumeration tp
    is recourd_only_of_0_1, then only those routes in the
    range (0,1) are recorded, if it is recourd_all, then 
    all routes are recorded. Roots outside of (0,1) are
    given a negative multiplicity.
   */
  void
  solve_linear(c_array<int> poly, std::vector<solution_point> &return_value, enum recourd_route_type tp);

  void
  solve_quadratic(c_array<int> poly, std::vector<solution_point> &return_value, enum recourd_route_type tp);

  void
  solve_cubic(c_array<int> poly, std::vector<solution_point> &return_value, enum recourd_route_type tp);
    
  void
  find_zero_points(c_array<int> poly, std::vector<solution_point> &return_value, enum recourd_route_type tp);

  enum coordinate_type
    {
      x_fixed=0,
      y_fixed=1,
    };

  class BezierCurve
  {
  public:
    BezierCurve *m_next_curve, *m_prev_curve;


    BezierCurve(void)
    {}
    
    BezierCurve(const geometry_data &dbg, GLushort ind0, GLushort ind1);
    BezierCurve(const geometry_data &dbg, GLushort ind0, GLushort ind1, GLushort ind2);
    BezierCurve(const geometry_data &dbg, GLushort ind0, GLushort ind1, GLushort ind2, GLushort ind3);

    const ivec2&
    pt0(void) const
    {
      return m_raw_curve[0];
    }

    void
    maximal_minimal_points(std::vector<std::pair<int,vec2> > &pts) const;

    void 
    compute_line_intersection(int in_pt, enum coordinate_type tp, std::vector<solution_point> &out_pts) const;

  private:

    
    vec2
    compute_pt_at_t(float t) const
    {
      return compute_pt_at_t_worker(t, 
                                    const_c_array<ivec2>(&m_raw_curve[0], m_raw_curve.size()-1),
                                    const_c_array<ivec2>(&m_raw_curve[1], m_raw_curve.size()-1));
    }

    static
    vec2
    compute_pt_at_t_worker(float t, const_c_array<ivec2> p0, const_c_array<ivec2> p1);

    void
    init(void);

    std::vector<ivec2> m_raw_curve; //stored as a bezier curve, i.e. control and end points.
    vecN<std::vector<int>, 2> m_curve; //stored as a polynomial
    vec2 m_pt0;

  };

  class OutlineData
  {
  public:

    OutlineData(const FT_Outline &outline, 
                const ivec2 &bitmap_size, 
                const ivec2 &bitmap_offset,
                float max_dist_value,
                const geometry_data &dbg);

    const distance_return_type&
    compute_distance(int x, int y);
   
       
    int
    point_from_bitmap_x(int x);

    int
    point_from_bitmap_y(int y);

    int
    point_from_bitmap_coord(int ip, enum coordinate_type tp);

    int
    bitmap_x_from_point(float x);

    int
    bitmap_y_from_point(float y);
   
  private:
    
    range_type<GLushort> 
    add_curves_from_contour(const_c_array<FT_Vector> pts,
                            const_c_array<char> pts_tag,
                            const ivec2 &offset, int scale,
                            const geometry_data &dbg);

    void
    compute_fixed_lines(void);

    void
    compute_point_list(void);

    std::vector<BezierCurve> m_bezier_curves;
    std::vector<range_type<int> > m_curve_sets;

    ivec2 m_min_xy, m_max_xy, m_offset;
    int m_scale;
    ivec2 m_bitmap_size, m_bitmap_offset;

    boost::multi_array<distance_return_type, 2> m_distance_values;
    float m_distance_scale_factor;
  };


  void
  push_back_multiplicity(std::vector<solution_point> &victim, 
                         float t, enum recourd_route_type tp)
  {
    int mult;

    mult=(t>0.0f and t<1.0f)?1:-1;

    if(mult==1 or tp==record_all)
      {
        victim.push_back(solution_point(mult, t, NULL) );
      }
  }


  void 
  generate_polynomial_from_bezier(const_c_array<ivec2> pts, vecN<std::vector<int>, 2> &return_value)
  {
    //we only handle degree 1,2 and 3
    WRATHassert(pts.size()==2 or pts.size()==3 or pts.size()==4);

    

    vecN<std::vector<int>, 2> p, q;
    if(pts.size()==2)
      {
        p.x().resize(1);
        p.y().resize(1);
        q.x().resize(1);
        q.y().resize(1);

        p.x()[0]=pts[0].x();
        p.y()[0]=pts[0].y();

        q.x()[0]=pts[1].x();
        q.y()[0]=pts[1].y();
      }
    else
      {
        //for now we do a lazy recusion: lazy for us to write,
        //but harder on CPU, TODO: avoid recursion.
        generate_polynomial_from_bezier(const_c_array<ivec2>(&pts[0], pts.size() -1), p);
        generate_polynomial_from_bezier(const_c_array<ivec2>(&pts[1], pts.size() -1), q);
      }

    //resize
    return_value.x().resize( pts.size() );
    return_value.y().resize( pts.size() );

    //now combine them:
    std::copy(p.x().begin(), p.x().end(), return_value.x().begin());
    std::copy(p.y().begin(), p.y().end(), return_value.y().begin());
    
    for(unsigned int i=1;i<pts.size(); ++i)
      {
        return_value.x()[i]+= q.x()[i-1] - p.x()[i-1];
        return_value.y()[i]+= q.y()[i-1] - p.y()[i-1];
      }
  }

  void
  solve_linear(c_array<int> poly, std::vector<solution_point> &return_value, enum recourd_route_type tp)
  {
    WRATHassert(poly.size()==2);
    int mult;

    if(poly[1]<0)
      {
        poly[1]=-poly[1];
        poly[0]=-poly[0];
      }
    
    mult=(poly[0]<0 and poly[0]+poly[1]>0)?1:-1;

    if(poly[1]!=0 and (mult==1 or tp==record_all) )
      {
        float v;

        v=static_cast<float>(-poly[0])/static_cast<float>(poly[1]);
        return_value.push_back( solution_point(mult, v, NULL) );
      }
  }

  void
  solve_quadratic(c_array<int> poly, std::vector<solution_point> &return_value, enum recourd_route_type tp)
  {
    int desc;

    WRATHassert(poly.size()==3);

    
    if(poly[2]==0)
      {
        solve_linear(c_array<int>(poly.c_ptr(), 2), return_value, tp);
        return;
      }

    //t=0 is ruled out
    if(poly[0]==0)
      {
        if(tp==record_all)
          {
            return_value.push_back(solution_point(-1, 0.0f, NULL));
          }
        solve_linear( c_array<int>(poly.c_ptr()+1, 2), return_value, tp);
        return;
      }

    
    int sum=poly[2]+poly[1]+poly[0];

    if(sum==0) //t=1 is a solution, we throw it away.
      {
        //so poly(t)=at^2+ bt + -(a+b)
        // = (t-1)(at+a+b)
        vecN<int,2> v;

        if(tp==record_all)
          {
            return_value.push_back( solution_point(-1, 1.0f, NULL));
          }

        v[1]=poly[2];
        v[0]=poly[1]+poly[2];
        solve_linear( c_array<int>(v.c_ptr(), 2), return_value, tp);
        return;
      }

    desc=poly[1]*poly[1] - 4*poly[0]*poly[2];
    if(desc<0)
      {
        //both roots not real.
        return;
      }

    //double root.
    if(desc==0)
      {
        vecN<int,2> v;
        int mult;

        v[0]=poly[1];
        v[1]=2*poly[2];

        if(v[1]<0)
          {
            v[0]=-v[0];
            v[1]=-v[1];
          }

        mult=(v[0]<0 and v[0]+v[1]>0)?1:-1;

        if(mult==1 or tp==record_all)
          {
            float t;

            t=static_cast<float>(-v[0])/static_cast<float>(v[1]);
            return_value.push_back( solution_point(2*mult, t, NULL) );
          }
        return;
      }

    //make leading co-efficient positive:
    if(poly[2]<0)
      {
        poly[2]=-poly[2];
        poly[1]=-poly[1];
        poly[0]=-poly[0];
        sum=-sum;
      }

    int two_a_plus_b;
    bool plus_radical_root_want, negative_radical_root_want;
    two_a_plus_b=2*poly[2]+poly[1];

    plus_radical_root_want=
      (two_a_plus_b>=0 and sum>=0) // <=1 condition
      and 
      (poly[0]<=0 or poly[1]<=0);  // >=0 condition

    negative_radical_root_want=
      (two_a_plus_b>=0 or sum<=0)  //<=1 condition
      and
      (poly[1]<=0 and poly[0]>=0);  // >=0 condition


    if(plus_radical_root_want or negative_radical_root_want or tp==record_all)
      { 

        float a, b, c, radical;
        a=static_cast<float>(poly[2]);
        b=static_cast<float>(poly[1]);
        c=static_cast<float>(poly[0]);

        radical=std::sqrt( static_cast<float>(desc) );
    
        float v0, v1;
        
        v0=(-b+radical)/(2.0f*a);
        v1=(-b-radical)/(2.0f*a);

        if(plus_radical_root_want or tp==record_all)
          {
            return_value.push_back(solution_point(plus_radical_root_want?1:-1,
                                                  v0, NULL));
          }
        
        if(negative_radical_root_want or tp==record_all)
          {
            return_value.push_back(solution_point(negative_radical_root_want?1:-1,
                                                  v1, NULL));
          }
      }
  }

  
  void
  solve_cubic(c_array<int> poly, std::vector<solution_point> &return_value, enum recourd_route_type tp)
  {
    WRATHassert(poly.size()==4);

    if(poly[0]==0)
      {
        solve_quadratic( c_array<int>(poly.c_ptr()+1, 2), return_value, tp);

        if(tp==record_all)
          {
            return_value.push_back(solution_point(-1, 0.0f, NULL));
          }

        return;
      }

    if(poly[3]==0)
      {
        solve_quadratic(c_array<int>(poly.c_ptr(), 3), return_value, tp);
        return;
      }
    
    if(poly[3]+poly[2]+poly[1]+poly[0]==0)
      {
        //icky t=1 is valid solution, generate the qudratic..
        vecN<int,3> v;

        if(tp==record_all)
          {
            return_value.push_back(solution_point(-1, 1.0f, NULL));
          }

        v[0]=poly[3]+poly[2]+poly[1];
        v[1]=poly[3]+poly[2];
        v[2]=poly[3];
        solve_quadratic(c_array<int>(v.c_ptr(),3), return_value, tp);
        return;
      }


    float L, p, q, C, temp, dd;
    vecN<float, 3> a;
    
    L=static_cast<float>(poly[3]);
    a[2]=static_cast<float>(poly[2])/L;
    a[1]=static_cast<float>(poly[1])/L;
    a[0]=static_cast<float>(poly[0])/L;

    p=(3.0f*a[1] - a[2]*a[2])/3.0f;
    q=(9.0f*a[1]*a[2]-27*a[0]-2*a[2]*a[2]*a[2])/27.0f;

    dd=a[2]/3.0f;

    if( 3*poly[1]*poly[3] == poly[2]*poly[2] )
      {
        push_back_multiplicity(return_value, -dd + cbrtf(q), tp);
        return;
      }

    
    temp=sqrtf(3.0f/fabs(p));
    C=0.5f*q*temp*temp*temp;

    temp=1.0f/temp;
    temp*=2.0f;

    if(p>0.0f)
      {
        push_back_multiplicity(return_value, temp*sinhf( asinhf(C)/3.0f) -  dd, tp);
      }
    else
      {
        if(C>=1.0f- 0.00001f)
          {
            push_back_multiplicity(return_value, temp*coshf( acoshf(C)/3.0f) - dd, tp);
          }
        else if(C<=-1.0f + 0.000001f)
          {
            push_back_multiplicity(return_value, - temp*coshf( acoshf(-C)/3.0f) - dd, tp);
          }
        else
          {
            float theta;

            theta=acosf(C);
            
            push_back_multiplicity(return_value, temp*cosf( (theta          )/3.0f) - dd, tp);
            push_back_multiplicity(return_value, temp*cosf( (theta+2.0f*M_PI)/3.0f) - dd, tp);
            push_back_multiplicity(return_value, temp*cosf( (theta+4.0f*M_PI)/3.0f) - dd, tp);
            
          }
      }
  }
  

  void
  find_zero_points(c_array<int> poly, std::vector<solution_point> &return_value, enum recourd_route_type tp)
  {
    //we only handle degree 1,2 and 3

    if(poly.size()<=1)
      {
        return;
      }
    WRATHassert(poly.size()==2 or poly.size()==3 or poly.size()==4);

    switch(poly.size())
      {
      case 2:
        solve_linear(poly, return_value, tp);
        break;

      case 3:
        solve_quadratic(poly, return_value, tp);
        break;

      case 4:
        solve_cubic(poly, return_value, tp);
        break;

      default:
        std::cerr << "\nInvalid degree, polynomial has "
                  << poly.size() << " coefficients, i.e has degree "
                  << poly.size() - 1;
      }
  }

  BezierCurve::
  BezierCurve(const geometry_data &dbg, GLushort ind0, GLushort ind1):
    m_next_curve(NULL), m_prev_curve(NULL),
    m_raw_curve(2)
  {
    m_raw_curve[0]=dbg.pt(ind0);
    m_raw_curve[1]=dbg.pt(ind1);
    
    init();    
  }

  BezierCurve::
  BezierCurve(const geometry_data &dbg, GLushort ind0, GLushort ind1, GLushort ind2):
    m_next_curve(NULL), m_prev_curve(NULL),
    m_raw_curve(3)
  {
    m_raw_curve[0]=dbg.pt(ind0);
    m_raw_curve[1]=dbg.pt(ind1);
    m_raw_curve[2]=dbg.pt(ind2);
    
    init();
  }

  BezierCurve::
  BezierCurve(const geometry_data &dbg, GLushort ind0, GLushort ind1, GLushort ind2, GLushort ind3):
    m_next_curve(NULL), m_prev_curve(NULL),
    m_raw_curve(4)
  {
    m_raw_curve[0]=dbg.pt(ind0);
    m_raw_curve[1]=dbg.pt(ind1);
    m_raw_curve[2]=dbg.pt(ind2);
    m_raw_curve[3]=dbg.pt(ind3);
    
    init();
  }
  
  void
  BezierCurve::
  compute_line_intersection(int in_pt, 
                            enum coordinate_type tp,
                            std::vector<solution_point> &out_pts) const
  {
    int sz;
    vecN<int, 4> work_array;
    std::vector<solution_point> ts;
        
    WRATHassert(m_curve.x().size()==m_curve.y().size());
    WRATHassert(m_curve.x().size()==m_raw_curve.size());
    sz=m_curve.x().size();
    
    WRATHassert(sz==2 or sz==3 or sz==4);
    
    
    //find the zeros of the "tp"-distance function:
    std::copy(m_curve[tp].begin(), m_curve[tp].end(), work_array.begin());
    work_array[0]-=in_pt;
    
    WRATHassert(work_array[0]);
    find_zero_points(c_array<int>(work_array.c_ptr(), sz), ts, record_only_of_0_1);

    for(unsigned int i=0, end_i=ts.size(); i<end_i; ++i)
      {
        vec2 pt;

        pt=compute_pt_at_t(ts[i].m_value);
        out_pts.push_back( solution_point(ts[i].m_multiplicity, pt[1-tp], this) );
      }
  }
  
  vec2
  BezierCurve::
  compute_pt_at_t_worker(float t, const_c_array<ivec2> p0, const_c_array<ivec2> p1)
  {
    //basic idea:
    // B(p0,p1,....., pN, t) = (1-t)*B(p0,p1,...,pN-1, t) + t*B(p1,p2,...,pN, t)
    // this algorthm is more numerially stable than multiplying out
    // a polynomial, but is is *cough* O(2^N), but considering we have 
    // only N=1,2 or 3 it does not matter. We can refine this so
    // it is O(N^2) but for now, I don't want to bother or care..
    vec2 q0, q1;
    
    WRATHassert(p0.size()>0);
    if(p0.size()==1)
      {
        q0=vec2(p0[0].x(), p0[0].y());
      }
    else
      {
        q0=compute_pt_at_t_worker(t, 
                                  const_c_array<ivec2>(p0.c_ptr(), p0.size()-1),
                                  const_c_array<ivec2>(p0.c_ptr()+1, p0.size()-1));
      }
    
    WRATHassert(p1.size()>0);
    if(p1.size()==1)
      {
        q1=vec2(p1[0].x(), p1[0].y());
      }
    else
      {
        q1=compute_pt_at_t_worker(t, 
                                  const_c_array<ivec2>(p1.c_ptr(), p1.size()-1),
                                  const_c_array<ivec2>(p1.c_ptr()+1, p1.size()-1));
      }
    return q0*(1.0f-t) + q1*t;
    
  }

  void
  BezierCurve::
  init(void)
  {
    //generate raw polynomial:
    generate_polynomial_from_bezier(const_c_array<ivec2>(&m_raw_curve[0], m_raw_curve.size()), m_curve);
  }

  void
  BezierCurve::
  maximal_minimal_points(std::vector<std::pair<int,vec2> > &pts) const
  {
    //now save the points for where the derivative is 0.
    int sz;
    vecN<int, 4> work_arrayDelta, work_arraySum;
    std::vector<solution_point> ts;
    
    WRATHassert(m_curve.x().size()==m_curve.y().size());
    WRATHassert(m_curve.x().size()==m_raw_curve.size());
    sz=m_curve.x().size();
    
    if(sz>1)
      {
        for(int i=1;i<sz;++i)
          {
            work_arraySum[i-1] = i*(m_curve.x()[i] + m_curve.y()[i]);
            work_arrayDelta[i-1] = i*(m_curve.x()[i] - m_curve.y()[i]);
          }
        
        //find the zeros of the derivatives of difference and sum of the coordinate functions.
        find_zero_points(c_array<int>(work_arraySum.c_ptr(), sz-1), ts, record_only_of_0_1);
        find_zero_points(c_array<int>(work_arrayDelta.c_ptr(), sz-1), ts, record_only_of_0_1);

        for(unsigned int i=0, end_i=ts.size(); i<end_i; ++i)
          {
            vec2 q;
            q=compute_pt_at_t(ts[i].m_value);
            pts.push_back(std::make_pair(ts[i].m_multiplicity, q));
          }
      }
  }

  void
  distance_tracker::
  update_value(float v, const BezierCurve *curve)
  {
    //if(v<m_init_value)
      {
        std::map<curve_key, float>::iterator iter;

        iter=m_curve_distances.find(curve);
        if(iter!=m_curve_distances.end())
          {
            iter->second=std::min(v, iter->second);
          }
        else
          {
            m_curve_distances[curve]=v;
          }
      }
  }

  void
  distance_tracker::
  update_value(float v, const BezierCurve *curve_c0, 
               const BezierCurve *curve_c1)
  {
    if(curve_c1<curve_c0)
      {
        std::swap(curve_c0, curve_c1);
      }

    //if(v<m_init_value)
      {
        std::map<corner_key, float>::iterator iter;

        iter=m_corner_distances.find(corner_key(curve_c0, curve_c1));
        if(iter!=m_corner_distances.end())
          {
            iter->second=std::min(iter->second, v);
          }
        else
          {
            m_corner_distances[ corner_key(curve_c0, curve_c1) ]=v;
          }
      }
  }

  void
  distance_tracker::
  use_curve_value(std::map<curve_key, float>::iterator curve_iter)
  {
    //a curve is the closest, we now look for 
    //the two nearest neighbors of the found curve:
    std::map<curve_key, float>::iterator iter1, iter2;
    float v1, v2;
    
    
    iter1=m_curve_distances.find(curve_iter->first->m_prev_curve);
    iter2=m_curve_distances.find(curve_iter->first->m_next_curve);
    
    v1=(iter1!=m_curve_distances.end())?iter1->second:(1.0f+m_init_value);
    v2=(iter2!=m_curve_distances.end())?iter2->second:(1.0f+m_init_value);
    
    
    m_value=curve_iter->second;
    m_value2=std::min(v1, v2);
    m_value2_has_meaning=( iter1!=m_curve_distances.end() or iter2!=m_curve_distances.end());
  }

  void 
  distance_tracker::
  use_corner_value(std::map<corner_key, float>::iterator corner_iter)
  {
    //corner is closest, so we need to test the two edges 
    //that make up the corner too.
    std::map<curve_key, float>::iterator iter1, iter2;
    float v1, v2;

    iter1=m_curve_distances.find(corner_iter->first.first);
    iter2=m_curve_distances.find(corner_iter->first.second);
    
    v1=(iter1!=m_curve_distances.end())?iter1->second:(1.0f+m_init_value);
    v2=(iter2!=m_curve_distances.end())?iter2->second:(1.0f+m_init_value);
    
    
    m_value=corner_iter->second;
    m_value2=std::min(v1, v2);
    m_value2_has_meaning=( iter1!=m_curve_distances.end() or iter2!=m_curve_distances.end());
  }

  void
  distance_tracker::
  finalize(void)
  {
    WRATHassert(!m_ready);

    //basic idea: find the closest distance values.
    std::map<corner_key, float>::iterator corner_iter(m_corner_distances.begin());
    std::map<curve_key, float>::iterator curve_iter(m_curve_distances.begin());

    for(std::map<corner_key, float>::iterator iter=corner_iter, 
          end=m_corner_distances.end(); iter!=end; ++iter)
      {
        if(iter->second < corner_iter->second)
          {
            corner_iter=iter;
          }
      }

    for(std::map<curve_key, float>::iterator iter=curve_iter, 
          end=m_curve_distances.end(); iter!=end; ++iter)
      {
        if(iter->second < curve_iter->second)
          {
            curve_iter=iter;
          }
      }

    if(!m_corner_distances.empty() and !m_curve_distances.empty())
      {
        //take the smallest one:
        if(corner_iter->second < curve_iter->second)
          {
            use_corner_value(corner_iter);
          }
        else
          {
            use_curve_value(curve_iter);
          }
      }
    else if(!m_corner_distances.empty())
      {
        use_corner_value(corner_iter);
      }
    else if(!m_curve_distances.empty())
      {
        use_curve_value(curve_iter);
      }

    m_ready=true;
  }
  
  range_type<GLushort> 
  OutlineData::
  add_curves_from_contour(const_c_array<FT_Vector> pts,
                          const_c_array<char> pts_tag,
                          const ivec2 &offset, int scale,
                          const geometry_data &dbg)
  {
     

    /*
      A Freetype contour is NOT one line segment or spline,
      rather it is a set of such "packed". The docs for Freetype state:
      (http://www.freetype.org/freetype2/docs/glyphs/glyphs-6.html)
      
      two successive points with FT_CURVE_TAG_ON: 
      a line segment between those two points
      
      a FT_CURVE_TAG_CONIC between two FT_CURVE_TAG_ON
      quadratic spline with control point the middle point and 
      connects the two end points
      
      2X FT_CURVE_TAG_CUBIC, between two FT_CURVE_TAG_ON
      cubic spline curve with the 2 control points being the middle
      ones connecting the two end points
      
      2X FT_CURVE_TAG_CONIC between two FT_CURVE_TAG_ON
      is eqivalent to changing the 2X FT_CURVE_TAG_CONIC to
      FT_CURVE_TAG_CONIC, FT_CURVE_TAG_ON, FT_CURVE_TAG_CONIC
      with the new middle point added at the midpoint of the
      two FT_CURVE_TAG_CONIC points
    */


    //now build a point stream where all implicit 
    //points are explicity created, we also need
    //to keep a track the point types.
    GLushort start_index(dbg.pts().size()), end_index;

    for(int k=0, end_k=pts.size(); k<end_k; ++k)
      {
        int prev_k=(k==0)?
          end_k-1:
          k-1;

        if( FT_CURVE_TAG(pts_tag[k])==FT_CURVE_TAG_CONIC
            and FT_CURVE_TAG(pts_tag[prev_k])==FT_CURVE_TAG_CONIC)
          {
            ivec2 implicit_pt;

            implicit_pt.x()=( pts[k].x + pts[prev_k].x )/2;
            implicit_pt.y()=( pts[k].y + pts[prev_k].y )/2;

            implicit_pt+=offset;
            implicit_pt*=scale;

            dbg.push_back(implicit_pt, FT_CURVE_TAG_ON);
          }

        ivec2 add_pt(pts[k].x, pts[k].y);
        add_pt+=offset;
        add_pt*=scale;
        dbg.push_back( add_pt, pts_tag[k]);
      } 
    end_index=dbg.pts().size();

    WRATHTextureFontTTF::point_type::point_classification prev_tag(dbg.tag(start_index));
    WRATHTextureFontTTF::point_type::point_classification prev_prev_tag(dbg.tag(end_index-1));
    WRATHTextureFontTTF::point_type::point_classification tag;

    dbg.stream() << "\n\t\t" << pts.size() << " points in made " << end_index-start_index << " points.";

    for(GLushort k=start_index+1, end_k=end_index; k<=end_k; ++k)
      {
        GLushort real_k(k==end_k?start_index:k);

        tag=dbg.tag(real_k);


        if(tag==WRATHTextureFontTTF::point_type::on_curve and
           prev_tag==WRATHTextureFontTTF::point_type::on_curve)
          {
            m_bezier_curves.push_back(BezierCurve(dbg, k-1, real_k));
          }
        else if(tag==WRATHTextureFontTTF::point_type::on_curve 
                and prev_tag==WRATHTextureFontTTF::point_type::conic_off_curve
                and prev_prev_tag==WRATHTextureFontTTF::point_type::on_curve) 
          {
            GLushort k_minus_2;

            k_minus_2=(k>start_index+1)?
              k-2:
              end_k-1;

            m_bezier_curves.push_back(BezierCurve(dbg, k_minus_2, k-1, real_k));
          }
        else if(tag==WRATHTextureFontTTF::point_type::cubic_off_curve and prev_tag==WRATHTextureFontTTF::point_type::cubic_off_curve
                and prev_prev_tag==WRATHTextureFontTTF::point_type::on_curve)
          {
            GLushort next_k;
            GLushort k_minus_2;

            if(real_k+1<end_k)
              {
                next_k=k+1;
              }
            else if(real_k+1==end_k)
              {
                next_k=start_index;
              }
            else if(real_k==end_k)
              {
                next_k=start_index+1;
              }

            k_minus_2=(k>start_index+1)?
              k-2:
              end_k-1;

            m_bezier_curves.push_back(BezierCurve(dbg, k_minus_2, k-1, real_k, next_k));
          }

        prev_prev_tag=prev_tag;
        prev_tag=tag;
        
      }
    
        
    return range_type<GLushort>(start_index, end_index);
  }

 
  OutlineData::
  OutlineData(const FT_Outline &outline, 
              const ivec2 &bitmap_size, 
              const ivec2 &bitmap_offset,
              float max_dist_value,
              const geometry_data &dbg):
    m_min_xy(0,0),
    m_max_xy(0,0),
    m_scale(4),
    m_bitmap_size(bitmap_size),
    m_bitmap_offset(bitmap_offset),
    m_distance_values(boost::extents[bitmap_size.x()][bitmap_size.y()]),
    m_distance_scale_factor(0.25f)
  {
    const_c_array<FT_Vector> pts;
    const_c_array<char> pts_tag;
    int last_contour_end(0);
        
    for(int i=0;i<outline.n_points and false;++i)
      {
        ivec2 pt;
        
        pt=ivec2(static_cast<int>(outline.points[i].x),
                 static_cast<int>(outline.points[i].y));
        
        
        m_min_xy.x()=std::min(m_min_xy.x(), pt.x());
        m_min_xy.y()=std::min(m_min_xy.y(), pt.y());
        
        m_max_xy.x()=std::max(m_max_xy.x(), pt.x());
        m_max_xy.y()=std::max(m_max_xy.y(), pt.y());
      }

    dbg.stream() << "\n\t" << outline.n_contours << " contours:";

    m_offset=ivec2(0,0)-m_min_xy;
    for(int c=0, end_c=outline.n_contours; c<end_c; ++c)
      {
        int sz;
        range_type<GLushort> O(0,0);
        int beg_outline_set, end_outline_set;

        sz=outline.contours[c] - last_contour_end + 1;
        pts=const_c_array<FT_Vector>(outline.points+last_contour_end, sz);
        pts_tag=const_c_array<char>(outline.tags+last_contour_end, sz);
        
        beg_outline_set=m_bezier_curves.size();

        O=add_curves_from_contour(pts, pts_tag, m_offset, m_scale, dbg); 
        dbg.indices().push_back(O);

        last_contour_end=outline.contours[c]+1;

        m_curve_sets.push_back(range_type<int>(beg_outline_set, m_bezier_curves.size()));
      }

    //now that all curves are created, we now record for each curve it's neighbors:
    for(std::vector<range_type<int> >::iterator iter=m_curve_sets.begin(),
          end=m_curve_sets.end(); iter!=end; ++iter)
      {
        for(int curve_index=iter->m_begin; curve_index!=iter->m_end; ++curve_index)
          {
            int curve_prev;

            curve_prev=
            (curve_index==iter->m_begin)?
            iter->m_end-1:curve_index-1;

            m_bezier_curves[curve_prev].m_next_curve=&m_bezier_curves[curve_index];
            m_bezier_curves[curve_index].m_prev_curve=&m_bezier_curves[curve_prev];
            
          }
      }

    //init m_distance_values:
    for(int x=0;x<bitmap_size.x();++x)
      {
        for(int y=0;y<bitmap_size.y();++y)
          {
            m_distance_values[x][y].m_distance.init(2.0f*max_dist_value);
          }
      }

    compute_fixed_lines();
    compute_point_list();

    //finalize m_distance_values:
    for(int x=0;x<bitmap_size.x();++x)
      {
        for(int y=0;y<bitmap_size.y();++y)
          {
            m_distance_values[x][y].m_distance.finalize();
          }
      }
  }

  inline
  int
  OutlineData::
  point_from_bitmap_x(int x)
  {
    x+=m_bitmap_offset.x();
    x= x*64 + 32;
    x+=m_offset.x();
    x*=m_scale;
    ++x;
    return x;
  }

  inline
  int
  OutlineData::
  point_from_bitmap_y(int y)
  {
    y+=m_bitmap_offset.y();
    y= y*64 + 32;
    y+=m_offset.y();
    y*=m_scale;
    ++y;
    return y;
  }

  inline
  int
  OutlineData::
  point_from_bitmap_coord(int ip, enum coordinate_type tp)
  {
    
    ip+=m_bitmap_offset[tp];
    ip= ip*64 + 32;
    ip+=m_offset[tp];
    ip*=m_scale;
    ++ip;
    return ip;
  }

  inline
  int
  OutlineData::
  bitmap_x_from_point(float x)
  {
    x-=1.0f;
    x/=static_cast<float>(m_scale);
    x-=static_cast<float>(m_offset.x());
    x-=32.0f;
    x/=64.0f;
    x-=static_cast<float>(m_bitmap_offset.x());
    return static_cast<int>(x);
  }

  inline
  int
  OutlineData::
  bitmap_y_from_point(float y)
  {
    y-=1.0f;
    y/=static_cast<float>(m_scale);
    y-=static_cast<float>(m_offset.y());
    y-=32.0f;
    y/=64.0f;
    y-=static_cast<float>(m_bitmap_offset.y());
    return static_cast<int>(y);
  }


  const distance_return_type&
  OutlineData::
  compute_distance(int bx, int by)
  {
    return m_distance_values[bx][by];
  }

  void
  OutlineData::
  compute_point_list(void)
  {
    for(unsigned int i=0, end_i=m_bezier_curves.size(); i<end_i; ++i)
      {
        vec2 fpt;
        ivec2 ipt;
        unsigned int next_i;

        fpt.x()=m_bezier_curves[i].pt0().x();
        fpt.y()=m_bezier_curves[i].pt0().y();
        
        ipt.x()=bitmap_x_from_point(fpt.x());
        ipt.y()=bitmap_y_from_point(fpt.y());

        next_i=(i+1)%end_i;

        for(int x=std::max(0, ipt.x()-2), 
              end_x=std::min(ipt.x()+3, m_bitmap_size.x());
            x<end_x; ++x)
          {
            for(int y=std::max(0, ipt.y()-2),
                  end_y=std::min(ipt.y()+3, m_bitmap_size.y());
                y<end_y;++y)
              {
                vec2 candidate;
                float dc;
                vec2 pt( static_cast<float>(point_from_bitmap_x(x)),
                         static_cast<float>(point_from_bitmap_y(y)));
                
                candidate=pt - fpt;
                dc=candidate.L1norm();
                dc*=m_distance_scale_factor;
                
                m_distance_values[x][y].m_distance.update_value(dc, 
                                                                m_bezier_curves[i].m_prev_curve,
                                                                &m_bezier_curves[i]);
              }
          }
      }

    for(unsigned int i=0, end_i=m_bezier_curves.size(); i<end_i; ++i)
      {
        std::vector<std::pair<int,vec2> > pts_to_check;

        m_bezier_curves[i].maximal_minimal_points(pts_to_check);

        for(std::vector<std::pair<int,vec2> >::iterator iter=pts_to_check.begin(),
              end=pts_to_check.end(); iter!=end; ++iter)
          {
            
            ivec2 ipt;
        
            ipt.x()=bitmap_x_from_point(iter->second.x());
            ipt.y()=bitmap_y_from_point(iter->second.y());
            
            for(int x=std::max(0, ipt.x()-2), 
                  end_x=std::min(ipt.x()+3, m_bitmap_size.x());
                x<end_x; ++x)
              {
                for(int y=std::max(0, ipt.y()-2),
                      end_y=std::min(ipt.y()+3, m_bitmap_size.y());
                    y<end_y;++y)
                  {
                    WRATHassert(iter->first>0);

                    vec2 candidate;
                    float dc;
                    vec2 pt( static_cast<float>(point_from_bitmap_x(x)),
                             static_cast<float>(point_from_bitmap_y(y)));
                    
                    candidate=pt - iter->second;
                    dc=candidate.L1norm();
                    dc*=m_distance_scale_factor;
                    
                    m_distance_values[x][y].m_distance.update_value(dc, &m_bezier_curves[i]);
                      
                  }
              }
          }
      }
    
  }

  void
  OutlineData::
  compute_fixed_lines(void)
  {
    const enum inside_outside_test_results::sol_type sol[2][2]=
    {
      {inside_outside_test_results::above, inside_outside_test_results::below}, //x_fixed
      {inside_outside_test_results::left, inside_outside_test_results::right} //y_fixed
    };

    for(int coord=0;coord<2;++coord)
      {
        enum coordinate_type coord_tp;
        enum coordinate_type other_coord_tp;
        
        coord_tp=static_cast<enum coordinate_type>(coord);
        other_coord_tp=static_cast<enum coordinate_type>(1-coord);

        for(int c=0, end_c=m_bitmap_size[coord]; c<end_c; ++c)
          {      
            int ip;
            std::vector<solution_point> L;
            int total_count(0);

            ip=point_from_bitmap_coord(c, coord_tp);
            L.clear();

            for(int i=0, end_i=m_bezier_curves.size(); i<end_i; ++i)
              {
                m_bezier_curves[i].compute_line_intersection(ip, coord_tp, L);
              }
            std::sort(L.begin(), L.end());

            for(int i=0, end_i=L.size(); i<end_i; ++i)
              {
                WRATHassert(L[i].m_multiplicity>0);
                total_count+=std::max(0, L[i].m_multiplicity);
              }
            
            for(int other_c=0, end_other_c=m_bitmap_size[1-coord], 
                  sz=L.size(), current_count=0, current_index=0;
                other_c<end_other_c; ++other_c)
              {
                 float p;
                 ivec2 pixel;
                 int prev_index;

                 pixel[coord]=c;
                 pixel[1-coord]=other_c;

                 p=static_cast<float>( point_from_bitmap_coord(other_c, other_coord_tp) );
                 prev_index=current_index;

                 while(current_index<sz
                       and L[current_index].m_value<=p)
                   {
                     current_count+=std::max(0, L[current_index].m_multiplicity);
                     ++current_index;
                   }
                 
                 for(int cindex=std::max(0, prev_index-1),
                       last_index=std::min(sz, current_index+2);
                     cindex<last_index; ++cindex)
                   {
                     
                     float dc;
                     vec2 pixel_location;
                     
                     pixel_location[coord]=ip;
                     pixel_location[1-coord]=L[cindex].m_value;          
                     
                     dc=std::abs( L[cindex].m_value - p);
                     dc*=m_distance_scale_factor;
                     m_distance_values[pixel.x()][pixel.y()].m_distance.update_value(dc, L[cindex].m_bezier);
                     
                   }
                 
                 m_distance_values[pixel.x()][pixel.y()].
                   m_solution_count.increment(sol[coord][0], current_count);

                 m_distance_values[pixel.x()][pixel.y()].
                   m_solution_count.increment(sol[coord][1], total_count - current_count);
              }
            
          }
      }


  }

  
}

///////////////////////////
// WRATHTextureFontTTF::texture_binder methods
WRATHTextureFontTTF::private_texture_binder::
private_texture_binder(WRATHTextureFontTTF *ttf):
  WRATHTextureChoice::texture(ttf->m_texture_name),
  m_ttf(ttf)
{}

void
WRATHTextureFontTTF::private_texture_binder::
bind_texture(GLenum)
{
  glBindTexture(GL_TEXTURE_2D, m_ttf->m_texture_name);
  m_ttf->flush_texture();
}


///////////////////////////////////
// WRATHTextureFontTTF::per_character_data methods
void
WRATHTextureFontTTF::per_character_data::
upload_data_to_texture(const ivec2& total_size)
{
  for(int m=0, end_m=m_mipmaps.size(); m<end_m; ++m)
    {

      if(!m_mipmaps[m].m_pixels.empty() 
         and m_mipmaps[m].m_texture_position.x()+m_mipmaps[m].m_texture_size.x()<=total_size.x()
         and m_mipmaps[m].m_texture_position.y()+m_mipmaps[m].m_texture_size.y()<=total_size.y())
        {
          glTexSubImage2D(GL_TEXTURE_2D,
                          m, //mipmap level
                          m_mipmaps[m].m_texture_position.x(), m_mipmaps[m].m_texture_position.y(), //location
                          m_mipmaps[m].m_texture_size.x(), m_mipmaps[m].m_texture_size.y(),//size
                          texture_format,
                          GL_UNSIGNED_BYTE,
                          &m_mipmaps[m].m_pixels[0]);
          
          
        }
    }
}

/////////////////////////////////////////
// WRATHTextureFontTTF methods
WRATHTextureFontTTF::
WRATHTextureFontTTF(FT_Face face,  
                  const std::string &pname,
                  int pixel_height):
  WRATHTextureFont(WRATHTextureFontKey(pname,pixel_height)),
  m_texture_name(0),
  m_texture_size(std::min(texture_creation_width(), glGet<int>(GL_MAX_TEXTURE_SIZE)), 
                 ceiling_power_2(pixel_height)),
  m_ttf_face(face),
  m_pixel_height(pixel_height),
  m_uses_mipmapping(use_mipmapping()),
  m_x(0), m_y(0), 
  m_current_line_max_height(0),
  m_resize_required(true),
  m_total_pixel_waste(0),
  m_total_pixel_use(0)
{
  WRATHassert( (face->face_flags&FT_FACE_FLAG_SCALABLE)!=0 and
          (face->face_flags&FT_FACE_FLAG_HORIZONTAL)!=0);

  FT_Set_Pixel_Sizes(face, m_pixel_height, 0);

   
  glGenTextures( 1, &m_texture_name);
  glBindTexture( GL_TEXTURE_2D,  m_texture_name);

  WRATHTextureChoice::texture *pt;
  pt=WRATHNew private_texture_binder(this);

  m_texture_binder=pt;
}



void
WRATHTextureFontTTF::
flush_texture(void)
{
  if(m_resize_required)
    {
      /*
        I hate GLES2. OpenGL provides glGetTexImage to read
        back the data of a texture, however GLES2 does NOT 
        provide this at all. Additionally, for rendering
        to a texture, the texture target must be GL_RGB
        or GL_RGBA. The method out of this is as follows:
         1) create an FBO, fbo.
         2) create a throwaway texture of the new resolution, temp_tex
         3) render to temp_tex, writing our channel data to the alpha channel
         4) call glReadPixels to get the data
         then repeat steps (3) and (4) for the other channel.

         If our texture is GL_RGBA, then FBO would work well..

         Sighs. 
         In light of the above, each glyph stores it's pixels,
         as such we can just iterate through our map and regenerate
         the texture.
      */
      m_resize_required=false;
      
      std::vector<pixel_type> zero_bytes;
      GLubyte *initialize_ptr(NULL);

      if(m_uses_mipmapping or true)
        {
          zero_bytes.resize(m_texture_size.x()*m_texture_size.y(), DEFAULT_PIXEL_VALUE );
          initialize_ptr=zero_bytes[0].c_ptr();
        }

      glTexImage2D( GL_TEXTURE_2D, //texture target
                    0, //mipmap level
                    texture_format, //internal format
                    m_texture_size.x(), m_texture_size.y(), //texture dimensions
                    0, //border size, must be 0 in GLES2 and GL3+ core profile
                    texture_format, //format of data input
                    GL_UNSIGNED_BYTE, //type of data input
                    initialize_ptr);

     if(m_uses_mipmapping)
        {
          for(int m=1, w=m_texture_size.x()/2, h=m_texture_size.y()/2;
              w>=1 or h>=1; w/=2, h/=2, ++m)
            {
               glTexImage2D( GL_TEXTURE_2D, //texture target
                             m, //mipmap level
                             texture_format, //internal format
                             std::max(1, w), std::max(1, h),
                             0, //border size, must be 0 in GLES2 and GL3+ core profile
                             texture_format, //format of data input
                             GL_UNSIGNED_BYTE, //type of data input
                             initialize_ptr);
            }

          glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        }
      else
        {
          glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
    
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      for(std::map<uint32_t, per_character_data>::iterator
            iter=m_character_data.begin(), end=m_character_data.end();
          iter!=end; ++iter)
        {
          iter->second.upload_data_to_texture(m_texture_size);
        }
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      
          

      m_dirty_characters.clear();
    }
  else if(!m_dirty_characters.empty())
    {      
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
           
      for(std::set<uint32_t>::iterator 
            siter=m_dirty_characters.begin(),
            send=m_dirty_characters.end();
          siter!=send; ++siter)
        {
          std::map<uint32_t, per_character_data>::iterator iter;

          iter=m_character_data.find(*siter);
          WRATHassert(iter!=m_character_data.end());

          iter->second.upload_data_to_texture(m_texture_size);
        }

      
      m_dirty_characters.clear();
    }
}

int
WRATHTextureFontTTF::
new_line_height(void)
{
  return m_ttf_face->size->metrics.ascender/64;
}

  

void
WRATHTextureFontTTF::
resize_texture(int new_height)
{
  if(new_height>m_texture_size.y())
    {
      m_resize_required=true;
      m_texture_size.y()=ceiling_power_2(new_height);
    }
}


void
WRATHTextureFontTTF::
generate_character(uint32_t glyph_index)
{
  
  WRATHassert(m_character_data.find(glyph_index)==m_character_data.end());

  m_character_data[glyph_index]=per_character_data();  
  per_character_data &glyph(m_character_data.find(glyph_index)->second);

  


  ivec2 bitmap_sz, bitmap_offset, glyph_size;
  size_t char_index;
  int advance;
  std::ostringstream temp_str;

  struct timeval start_time, end_time;
  gettimeofday(&start_time, NULL);

  
  geometry_data dbg(temp_str, m_font_points, glyph.m_point_indices);


  dbg.stream() << "Generating glyph '"
               << static_cast<char>(glyph_index)
               << "' ascii code=" << glyph_index;

  //Load the name glyph, this puts the glyph data
  //into m_ttf_face->glyph
  char_index=FT_Get_Char_Index(m_ttf_face, static_cast<unsigned int>(glyph_index));
  FT_Load_Glyph(m_ttf_face, char_index, FT_LOAD_DEFAULT);

  //tell Freetype2 to render the glyph to a bitmap,
  //this bitmap is located at m_ttf_face->glyph->bitmap
  FT_Render_Glyph(m_ttf_face->glyph, FT_RENDER_MODE_NORMAL);

  glyph.m_width=(m_ttf_face->glyph->metrics.horiAdvance>>6);

  bitmap_sz=ivec2(m_ttf_face->glyph->bitmap.width,
                  m_ttf_face->glyph->bitmap.rows);
      
  bitmap_offset=ivec2(m_ttf_face->glyph->bitmap_left,
                      m_ttf_face->glyph->bitmap_top - m_ttf_face->glyph->bitmap.rows);

  glyph_size=ivec2(m_ttf_face->glyph->bitmap.width,
                   m_ttf_face->glyph->bitmap.rows);

  if(m_uses_mipmapping)
    {      
      glyph_size.x()=( glyph_size.x()>0)?
        glyph_size_round_up(glyph_size.x()):
        0;
      
      glyph_size.y()=( glyph_size.y()>0)?
        glyph_size_round_up(glyph_size.y()):
        0;
      
      glyph.m_mipmaps.resize(std::max(1, number_mipmaps(glyph_size)) );
    }
  else
    {
      glyph.m_mipmaps.resize(1);
    }

  advance=glyph_size.x();  
  if(advance+m_x>m_texture_size.x())
    {
      m_x=0;
      m_y+=m_current_line_max_height;
      m_current_line_max_height=0;
    }

  WRATHassert(!m_uses_mipmapping or m_x==glyph_size_round_up(m_x));
  WRATHassert(!m_uses_mipmapping or m_y==glyph_size_round_up(m_y));

  glyph.m_mipmaps[0].m_texture_position=ivec2(m_x,m_y);
  glyph.m_mipmaps[0].m_texture_size=glyph_size;

  glyph.m_data=character_data_type( glyph.m_mipmaps[0].m_texture_position,
                                    ivec2(std::max(1,m_ttf_face->glyph->bitmap.width),
                                          std::max(1,m_ttf_face->glyph->bitmap.rows)),
                                    bitmap_offset,
                                    ivec2(m_ttf_face->glyph->metrics.horiAdvance>>6,
                                          m_ttf_face->glyph->metrics.vertAdvance>>6));


  
  m_current_line_max_height=std::max(m_current_line_max_height,
                                     glyph_size.y());

  resize_texture(m_y+glyph_size.y());
  
  
  glyph.m_mipmaps[0].m_pixels.resize(glyph_size.x()*glyph_size.y(), DEFAULT_PIXEL_VALUE );
  c_array<pixel_type> image_buffer(&glyph.m_mipmaps[0].m_pixels[0], glyph_size.x()*glyph_size.y());

  
  
  m_total_pixel_use+=glyph_size.x()*glyph_size.y();
  m_total_pixel_waste+=(glyph_size.x()*glyph_size.y() - bitmap_sz.y()*bitmap_sz.x());
  
  m_x+=advance;

  dbg.stream() << "\n\tBitmap size=" << bitmap_sz
               << "\n\tBitmap offset=" << bitmap_offset
               << "\n\tglyph_size=" << glyph_size
               << "\n\t  Waste of " << glyph_size.x()*glyph_size.y() - bitmap_sz.y()*bitmap_sz.x() << " pixels";

  //generate the outline data of the glyph:
  float max_dist(255.0f);
  OutlineData outline_data(m_ttf_face->glyph->outline, 
                           bitmap_sz, bitmap_offset, max_dist,
                           dbg);
  

  for(int yy=0; yy<bitmap_sz.y(); ++yy)
    {
      for(int xx=0; xx<bitmap_sz.x(); ++xx)
        {
          int location;
          ivec2 glyph_pos;
          bool outside;

          glyph_pos.x()=xx;
          glyph_pos.y()=yy;


          WRATHassert(glyph_pos.x()>=0 and glyph_pos.y()>=0);
          WRATHassert(glyph_pos.x()<glyph_size.x());
          WRATHassert(glyph_pos.y()<glyph_size.y());

          location= glyph_pos.x() + glyph_pos.y()*glyph_size.x();
          WRATHassert(location>=0 and location<glyph_size.x()*glyph_size.y());

          image_buffer[location].back()
            =m_ttf_face->glyph->bitmap.buffer[xx + (bitmap_sz.y()-1-yy)*m_ttf_face->glyph->bitmap.pitch];

          const distance_return_type &raw_dist(outline_data.compute_distance(xx, yy));

          
          outside=raw_dist.m_solution_count.outside();

          float v0;
          v0=std::min(raw_dist.m_distance.value()/max_dist, 1.0f);
          image_buffer[location].x()=pixel_value_from_distance(v0, outside);

          if(pixel_type::array_size>2)
            {
              float v1;
              v1=std::min(raw_dist.m_distance.value2()/max_dist, 1.0f);
              image_buffer[location].y()=pixel_value_from_distance(v1, outside);
            }
          
          if(pixel_type::array_size>3)
            {
              image_buffer[location].z()= (raw_dist.m_distance.value2_has_meaning())?255:0;
            }
        }
    }

  
  if(m_uses_mipmapping)
    {
      for(int m=1, end_m=glyph.m_mipmaps.size(); m<end_m; ++m)
        {
          FT_Matrix ft_matrix;
          FT_Vector ft_vector;
          
          ft_matrix.xy=ft_matrix.yx=0;
          ft_matrix.xx=ft_matrix.yy=( (1<<16)>>m);
          
          ft_vector.x=0;
          ft_vector.y=0;
          
          FT_Set_Transform(m_ttf_face, &ft_matrix, &ft_vector);
          
          FT_Load_Glyph(m_ttf_face, char_index, FT_LOAD_DEFAULT);
          FT_Render_Glyph(m_ttf_face->glyph, FT_RENDER_MODE_NORMAL);
          dbg.stream() << "\n\tMipmap level " << m
                       << " resolution=" 
                       << ivec2(m_ttf_face->glyph->bitmap.width,
                                m_ttf_face->glyph->bitmap.rows);


          glyph.m_mipmaps[m].m_texture_position=glyph.m_mipmaps[m-1].m_texture_position/2;
          glyph.m_mipmaps[m].m_texture_size=glyph.m_mipmaps[m-1].m_texture_size/2;
          glyph.m_mipmaps[m].m_pixels.resize(glyph.m_mipmaps[m].m_texture_size.x()*glyph.m_mipmaps[m].m_texture_size.y());

          c_array<pixel_type> mip_image_buffer(&glyph.m_mipmaps[m].m_pixels[0], 
                                               glyph.m_mipmaps[m].m_texture_size.x()*glyph.m_mipmaps[m].m_texture_size.y());

          /**
          OutlineData mip_outline_data(m_ttf_face->glyph->outline, 
                                       ivec2(m_ttf_face->glyph->bitmap.width,
                                             m_ttf_face->glyph->bitmap.rows),
                                       ivec2(m_ttf_face->glyph->bitmap_left,
                                             m_ttf_face->glyph->bitmap_top - m_ttf_face->glyph->bitmap.rows), 
                                       max_dist,
                                       dbg);
          **/

          for(int yy=0; yy<m_ttf_face->glyph->bitmap.rows and yy<glyph.m_mipmaps[m].m_texture_size.y(); ++yy)
            {
              for(int xx=0; xx<m_ttf_face->glyph->bitmap.width and xx<glyph.m_mipmaps[m].m_texture_size.x(); ++xx)
                {
                  int location;

                  location= xx + yy*glyph.m_mipmaps[m].m_texture_size.x();

                  mip_image_buffer[location].back()
                    =m_ttf_face->glyph->bitmap.buffer[xx + (m_ttf_face->glyph->bitmap.rows-1-yy)*m_ttf_face->glyph->bitmap.pitch];

                  /**/
                  int loc0, loc1, loc2, loc3;
                  loc0= (2*xx  ) + (2*yy  )*glyph.m_mipmaps[m-1].m_texture_size.x();
                  loc1= (2*xx  ) + (2*yy+1)*glyph.m_mipmaps[m-1].m_texture_size.x();
                  loc2= (2*xx+1) + (2*yy  )*glyph.m_mipmaps[m-1].m_texture_size.x();
                  loc3= (2*xx+1) + (2*yy+1)*glyph.m_mipmaps[m-1].m_texture_size.x();

                  GLuint v;
                  v=glyph.m_mipmaps[m-1].m_pixels[loc0].x();
                  v+=glyph.m_mipmaps[m-1].m_pixels[loc1].x();
                  v+=glyph.m_mipmaps[m-1].m_pixels[loc2].x();
                  v+=glyph.m_mipmaps[m-1].m_pixels[loc3].x();
                  v=std::min( GLuint(255), v/4);
                  mip_image_buffer[location].x()=v; 

                  /**
                  const distance_return_type &raw_dist(mip_outline_data.compute_distance(xx, yy));
                  bool outside;

                  if(raw_dist.m_solution_count.reliable_test())
                    {
                      outside=raw_dist.m_solution_count.outside();
                    }
                  else
                    {
                      outside=(mip_image_buffer[location].back()<=127);
                    }
                  mip_image_buffer[location].x()=pixel_value_from_distance(std::min(raw_dist.m_distance.value()/max_dist, 1.0f), outside);
                  **/
                }
            }
          

        }
    }

  FT_Set_Transform(m_ttf_face, NULL, NULL);
  

  gettimeofday(&end_time, NULL);
  dbg.stream() << "\n\tTime to generate: "
               << time_difference(end_time, start_time)
               << " ms.";


  
  
  glyph.m_debug_string_data=temp_str.str();

  m_dirty_characters.insert(glyph_index);

  

}



WRATHTextureFontTTF::
~WRATHTextureFontTTF()
{
  glWRATHDeleteTextures(1, &m_texture_name);

  if(m_ttf_face!=NULL)
    {
      FT_Done_Face(m_ttf_face);
    }
}

WRATHTextureFontTTF::per_character_data&
WRATHTextureFontTTF::
get_glyph(uint32_t glyph)
{
  std::map<uint32_t, per_character_data>::iterator iter;

  iter=m_character_data.find(glyph);
  if(iter==m_character_data.end())
    {
      generate_character(glyph);
      iter=m_character_data.find(glyph);
    }

  WRATHassert(iter!=m_character_data.end());
  return iter->second;
}


WRATHTextureFont::character_data_type
WRATHTextureFontTTF::
character_data(uint32_t glyph)
{
  return get_glyph(glyph).m_data;
}


const std::string&
WRATHTextureFontTTF::
debug_string_data(uint32_t glyph)
{
  return get_glyph(glyph).m_debug_string_data;
}

const std::vector<range_type<GLushort> >&
WRATHTextureFontTTF::
glyph_outlines(uint32_t glyph)
{
  return get_glyph(glyph).m_point_indices;
}

const std::vector<WRATHTextureFontTTF::point_type>&
WRATHTextureFontTTF::
font_geometry(void)
{
  return m_font_points;
}


namespace
{
  class ft_library_loader
  {
  public:
    FT_Library m_library;
    bool m_loaded;

    ft_library_loader(void)
    {
      int error_code;
      
      error_code=FT_Init_FreeType(&m_library);
      m_loaded= (error_code==0);
    }

    ~ft_library_loader()
    {
      if(m_loaded)
        {
          FT_Done_FreeType(m_library);
        }
    }

  private:
    ft_library_loader(const ft_library_loader&)
    {}
  };

  ft_library_loader&
  ft_library_object(void)
  {
    static ft_library_loader R;
    return R;
  }


}


FT_Face
WRATHTextureFontTTF::
load_face(const std::string &pfilename, int face_index)
{
  FT_Face face(NULL);

  if(ft_library_object().m_loaded)
    {
      int error_code;

      error_code=FT_WRATHNew_Face( ft_library_object().m_library,
                              pfilename.c_str(),
                              face_index,
                              &face );

      if(error_code and face!=NULL)
        {
          FT_Done_Face(face);
          face=NULL;
        }
    }

  return face;
}


WRATHTextureFontTTF*
WRATHTextureFontTTF::
fetch_font(int psize, const std::string &pfilename, int face_index)
{
  //first try to fetch the font:
  std::ostringstream ostr;
  WRATHTextureFontTTF *return_value;
  WRATHTextureFont *p;

  ostr << pfilename << "??" << face_index;

  p=WRATHTextureFont::retrieve_resource(WRATHTextureFontKey(ostr.str(), psize));
  return_value=dynamic_cast<WRATHTextureFontTTF*>(p);

  if(return_value==NULL)
    {
      FT_Face pface;
     
      pface=load_face(pfilename, face_index);

      if(pface!=NULL)
        {
          return_value=WRATHNew WRATHTextureFontTTF(pface, pfilename, psize);
        }
      else
        {
          return_value=NULL;
        }
    }

  return return_value;
}

bool
WRATHTextureFontTTF::
use_mipmapping(void)
{
  return sm_use_mipmapping();
}

void
WRATHTextureFontTTF::
use_mipmapping(bool v)
{
  sm_use_mipmapping()=v;
}

GLint
WRATHTextureFontTTF::
texture_creation_width(void)
{
  return sm_texture_creation_width();
}

void
WRATHTextureFontTTF::
texture_creation_width(GLint v)
{
  sm_texture_creation_width()=v;
}


void
test_find_zero_points(void)
{
  
  int degree;

  do
    {
      std::cout << "Degree Polynomial to test: "
                << std::flush;
      std::cin >> degree;

      degree=std::min(degree,3);

      std::vector<ivec2> rotties;
      std::vector<int> poly;

      rotties.resize(degree);
      poly.resize(degree+1, 0);

      std::cout << "Input product of linear terms (Ax+B)\n";

      for(int i=0; i<degree; ++i)
        {
          std::cout << "B(" << i << "):"
                    << std::flush;
          std::cin >> rotties[i].x();
          
          std::cout << "A(" << i << "):"
                    << std::flush;
          std::cin >> rotties[i].y();
        }

      switch(degree)
        {
        case 0:
          break;

        case 1:
          poly[0]=rotties[0].x();
          poly[1]=rotties[0].y();
          break;

        case 2:
          poly[0]=rotties[0].x()*rotties[1].x();
          poly[1]=rotties[0].x()*rotties[1].y()
            + rotties[0].y()*rotties[1].x();
          poly[2]=rotties[0].y()*rotties[1].y();
          break;

        case 3:
          poly[0]=rotties[0].x()*rotties[1].x()*rotties[2].x();

          poly[1]=rotties[0].x()*rotties[1].x()*rotties[2].y()
            + rotties[0].x()*rotties[1].y()*rotties[2].x()
            + rotties[0].y()*rotties[1].x()*rotties[2].x();

          poly[2]=rotties[0].y()*rotties[1].y()*rotties[2].x()
            + rotties[0].y()*rotties[1].x()*rotties[2].y()
            + rotties[0].x()*rotties[1].y()*rotties[2].y();
            
          poly[3]=rotties[0].y()*rotties[1].y()*rotties[2].y();
        }
      
      if(degree>0)
        {
          for(int i=0;i<degree;++i)
            {
              std::cout << " ( " 
                        << rotties[i].y()
                        << "t + "
                        << rotties[i].x()
                        << " ) ";
            }
          std::cout << "\n\t=";

          for(int i=0;i<=degree;++i)
            {
              if(i!=0)
                {
                  std::cout << " + ";
                }
              std::cout << poly[i] << "t^" << i;
            }
          std::cout << "\n";
          
          std::vector<solution_point> roots; 
          find_zero_points(c_array<int>(&poly[0],poly.size()), roots, record_all);

          std::cout << "Roots : " 
                    << print_range(roots.begin(), roots.end())
                    << "\n\n"
                    << "Test Raw Polynomial:\n";

          for(int i=0;i<=degree;++i)
            {
              std::cout << "Coefficent for ^" << i
                        << " term: " << std::flush;
              std::cin >> poly[i];
            }

          for(int i=0;i<=degree;++i)
            {
              if(i!=0)
                {
                  std::cout << " + ";
                }
              std::cout << poly[i] << "t^" << i;
            }
          std::cout << "\n";

          roots.clear();
          find_zero_points(c_array<int>(&poly[0],poly.size()), roots, record_all);

          std::cout << "Roots : " 
                    << print_range(roots.begin(), roots.end())
                    << "\n\n"
                    << "Test Raw Polynomial:\n";

          

        }

      

      
    }
  while(degree>0);
}


