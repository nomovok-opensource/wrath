/*! 
 * \file WRATHFreeTypeSupport.cpp
 * \brief file WRATHFreeTypeSupport.cpp
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

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

#include <map>
#include "WRATHFreeTypeSupport.hpp"
/********************************************

Explanation of analytic distance calculation
algorithm:

  Distance texture stores at a pixel p
the signed taxi-cab distance to the outline of the
font, where the sign is negative if the pixel
is outside and positive if the pixel is inside.

 We compute the taxi-cab distance to the outline,
 which is given by:

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
in OutlineData::compute_fixed_line_values().

The points where the derivative of f may be zero
are when a(t)=b(t) or a(t)=-b(t) are handled
in OutlineData::compute_zero_derivative_values().

The points t=0 and t=1 correspond to the end
points of the Bezier curve, since successive
curves in an outline start where the previous
curve ends, to test all points just means that we
test the starting point of each Bezier curve,
this is done in OutlineData::compute_outline_point_values().

Optimizations:

 A point of the outline is really only worth
considering to minimize the distance if it 
is within 2 pixels of the pixel p, thus
in OutlineData::compute_outline_point_values()
and OutlineData::compute_zero_derivative_values(),
we do for each point B(t) found:

 for(those (x,y) within 2 pixels of B(t))
 {
   do_calculation()
 }
    

Thus the minimizing for the critical
points of all the curves and the points
of the outline is done in O(number points) time.
  
  In compute_fixed_line_values(), we do:

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
  
Hence total computation time is at worst
O( B*(width+height)+ width*height + N), 
N=#points of outline, B=#Bezier curves

Other important tricks:
  (1) points of out line are stored as integers multiplied by 4, 
  (2) center points of bitmap are of the form 4*N + 1, this way
      center points of bitmap never have same coordinate as 
  any point of the font, this is needed to get a reliable inside/outside
  test using the vertical and horizontal lines.
  (3) roots are counted with multiplicity, this too is needed
  to get a reliable inside/outside test using the vertical and 
  horizontal lines.

  (4) solvers work on integers, so that remove exact zero and one
  roots and also remove exactly when results are not within
  the range (0,1).

  NOTE: the solver for cubics does not have the analytic exact 
  ability to check if a root is between (0,1) and 
  relies on the floating point representation.

 **********************************************/



namespace
{
  typedef std::pair<enum WRATHFreeTypeSupport::boundary_type, const WRATHFreeTypeSupport::simple_line*> grab_entry; 
  typedef const WRATHFreeTypeSupport::BezierCurve* grab_key;
  typedef std::map<grab_key, std::list<grab_entry> > grab_map;

  template<typename T, unsigned int N>
  vecN<T,N>
  compute_midpoint(const vecN<T,N> &a, const vecN<T,N> &b)
  {
    vecN<T,N> c(a+b);
    return c/T(2);
  }

  /*
    Note: does midpoint of midpoint of midpoint,
    i.e. "divides" by 8.
   */
  template<typename T>
  class CubicBezierHelper
  {
  public:
    typedef vecN<T,2> point;


    template<typename P>
    CubicBezierHelper(const P &q0,
                      const P &q1,
                      const P &q2,
                      const P &q3):
      m_pts( point(q0), point(q1), point(q2), point(q3)),
      p0(m_pts[0]),
      p1(m_pts[1]),
      p2(m_pts[2]),
      p3(m_pts[3])
    {
       p0_1=compute_midpoint(p0, p1);
       p1_2=compute_midpoint(p1, p2);
       p2_3=compute_midpoint(p2, p3);
       
       p01_12=compute_midpoint(p0_1, p1_2);
       p12_23=compute_midpoint(p1_2, p2_3);

       pMid=compute_midpoint(p01_12, p12_23);
    }
                      
    
    vecN<point, 4> m_pts;
    point &p0, &p1, &p2, &p3;  //original points
    point p0_1, p1_2, p2_3; // p_i_j --> midpoint of pi and pj
    point p01_12, p12_23; // p_ab_ij --> midpoint of p_a_b and p_i_j
    point pMid;
  };
           
  bool
  is_flat_curve(ivec2 p0, ivec2 p1, ivec2 p2)
  {
    /*
      flat quadratics are bad curves, so if a curve
      is very flat, we collapse it into a line.
     */
    p1-=p0;
    p2-=p0;

    return p1.x()*p2.y() == p2.x()*p1.y();
  }

  bool
  is_flat_curve(WRATHFreeTypeSupport::geometry_data dbg,
                GLushort i0, GLushort i1, GLushort i2)
  {
    return is_flat_curve(dbg.pt(i0), dbg.pt(i1), dbg.pt(i2));
                         
  }
  
  WRATHFreeTypeSupport::BezierCurve*
  create_line_if_flat(WRATHFreeTypeSupport::geometry_data dbg,
                      GLushort i0, GLushort i1, GLushort i2)
  {
    if(is_flat_curve(dbg, i0, i1, i2))
      {
        return WRATHNew WRATHFreeTypeSupport::BezierCurve(dbg, i0, i2);
      }
    else
      {
        return WRATHNew WRATHFreeTypeSupport::BezierCurve(dbg, i0, i1, i2);
      }
  }
                      

  void
  grab_simple_lines(grab_map &hits_found,
                    const boost::multi_array<WRATHFreeTypeSupport::analytic_return_type, 2> &dataLOD0,
                    int fixed_value, range_type<int> range,
                    enum WRATHUtil::coordinate_type coord, 
                    enum WRATHFreeTypeSupport::boundary_type which_to_grab)
  {
    ivec2 pix;

    pix[WRATHUtil::fixed_coordinate(coord)]=fixed_value;

    for(pix[WRATHUtil::varying_coordinate(coord)]=range.m_begin;
        pix[WRATHUtil::varying_coordinate(coord)]<range.m_end; 
        ++pix[WRATHUtil::varying_coordinate(coord)])
      {
        const WRATHFreeTypeSupport::analytic_return_type &R(dataLOD0[pix.x()][pix.y()]);
        for(int j=0, end_j=R.m_intersecions[which_to_grab].size(); j<end_j; ++j)
          {
            const WRATHFreeTypeSupport::simple_line &L(R.m_intersecions[which_to_grab][j]);
            hits_found[L.m_source.m_bezier].push_back(grab_entry(which_to_grab, &L));
          }
      }
  }


  float
  compute_distance_sign(const vec2 &texel_pt,
                        const vec2 &curve_pt,
                        const vec2 &curve_deriv)
  {
    vec2 delta(texel_pt-curve_pt);
    vec2 normal_vector(curve_deriv.y(), -curve_deriv.x());
    float d;

    d=dot(delta, normal_vector);
    return (d>0.0f)?1.0f:-1.0f;
  }


  int
  compute_tag_value(const vec2 &prev, const vec2 &next)
  {
    int return_value(0);

    if(prev.y()*next.y()<0.0f)
      {
        return_value|=WRATHFreeTypeSupport::y_extremal_flag;
      }
    if(prev.x()*next.x()<0.0f)
      {
        return_value|=WRATHFreeTypeSupport::x_extremal_flag;
      }
    return return_value;
  }

  bool
  count_as_multiplicity2(enum WRATHUtil::coordinate_type tp, int flag)
  {
    const int masks[2]=
      {
        WRATHFreeTypeSupport::x_extremal_flag, //WRATHUtil::x_fixed=0
        WRATHFreeTypeSupport::y_extremal_flag, //WRATHUtil::y_fixed=1
      };
    WRATHassert(tp==WRATHUtil::x_fixed or tp==WRATHUtil::y_fixed);

    return 0!=(flag&masks[tp]);
  }
  
  template<typename iterator>
  int
  sum_array(iterator b, iterator e)
  {
    int R(0);
    for(;b!=e; ++b)
      {
        R+=(*b);
      }
    return R;
  }
  
  void
  remove_end_point_solutions(c_array<int> &feed)
  {
    while(feed.size()>0 and 0==feed[0])
      {
        feed=feed.sub_array(1, feed.size() -1);
      }

    while(feed.size()>0 and 0==sum_array(feed.begin(), feed.end()))
      {
        for(int k=feed.size()-1; k>0; --k)
          {
            feed[k-1]+=feed[k];
          }
        feed=feed.sub_array(1, feed.size()-1);
      }
  }

  class face_with_private_library:public WRATHFreeTypeSupport::LockableFace
  {
  public:
    face_with_private_library(FT_Face ft_fc, FT_Library ft_lib):
      WRATHFreeTypeSupport::LockableFace(ft_fc, true),
      m_lib(ft_lib)
    {}

    ~face_with_private_library(void)
    {
      /*
        note: we set the flag as true so that
        the dtor of WRATHFreeTypeSupport::LockableFace
        would NOT FT_Done_Face the face, but we will
        so that we can FT_Done_FreeType after closing
        the FT_Face.
       */
      WRATHLockMutex(mutex());
      FT_Done_Face(face());
      FT_Done_FreeType(m_lib);
      WRATHUnlockMutex(mutex());
    }

  private:
    FT_Library m_lib;
  };

}

namespace WRATHFreeTypeSupport
{
  enum boundary_type
  opposite_boundary(enum boundary_type v)
  {
    const enum boundary_type Rs[5]=
      {
        /*[left_boundary]=*/ right_boundary,
        /*[right_boundary]=*/ left_boundary,
        /*[below_boundary]=*/ above_boundary,
        /*[above_boundary]=*/ below_boundary,
        /*[no_boundary]=*/ no_boundary,
      };
    WRATHassert(v<5);
    return Rs[v];
  }

  enum boundary_type
  neighbor_boundary(enum boundary_type v)
  {
    
    const enum boundary_type Rs[5]=
      {
        /*[left_boundary]=*/ above_boundary,
        /*[right_boundary]=*/ below_boundary,
        /*[below_boundary]=*/ left_boundary,
        /*[above_boundary]=*/ right_boundary,
        /*[no_boundary]=*/ no_boundary,
      };
    WRATHassert(v<5);
    return Rs[v];
  }

  enum WRATHUtil::coordinate_type
  side_type(enum boundary_type v)
  {
    enum WRATHUtil::coordinate_type Rs[4]=
      {
        /*[left_boundary]=*/ WRATHUtil::x_fixed,
        /*[right_boundary]=*/ WRATHUtil::x_fixed,
        /*[below_boundary]=*/ WRATHUtil::y_fixed,
        /*[above_boundary]=*/ WRATHUtil::y_fixed,
      };
    WRATHassert(v<4);
    return Rs[v]; 
  }

  /////////////////////////////////////
  //WRATHFreeTypeSupport::geometry_data methods
  GLushort
  geometry_data::
  push_back(const ivec2 &in_pt, char in_tag) const
  {
    enum point_type::point_classification cl;
    char curve_tag( FT_CURVE_TAG(in_tag));
    
    switch(curve_tag)
      {
      default:
      case FT_CURVE_TAG_ON:
        cl=point_type::on_curve;
      break;
      
      case FT_CURVE_TAG_CONIC:
        cl=point_type::conic_off_curve;
        break;
        
      case FT_CURVE_TAG_CUBIC:
        cl=point_type::cubic_off_curve;
      }
    
    GLushort return_value(m_pt_array.size());
    point_type v(in_pt, cl);
    
    if(m_filter.valid())
      {
        v.position()=m_filter->apply_filter(v.position(), v.classification());
      }

    m_pt_array.push_back(v);
    return return_value;
    
  }

    
  /////////////////////////////////////////
  // WRATHFreeTypeSupport::BezierCurve methods

  BezierCurve::
  BezierCurve(geometry_data dbg, 
              const std::vector<GLushort> &indices):
    m_raw_index(indices),
    m_curveID(-1),
    m_contourID(-1),
    m_tag_pt0(-1), m_tag_pt1(-1)
  {    
    /*
      check if curve is lower degree that it is.
      For now we only check degree=2:
     */
    if(indices.size()==3
       and is_flat_curve(dbg, indices[0], indices[1], indices[2]))
      {
        m_raw_index.pop_back();
        m_raw_index[1]=indices[2];
      }

    init(dbg);    
  }

  BezierCurve::
  BezierCurve(geometry_data dbg, 
              GLushort ind0, GLushort ind1):
    m_raw_index(2),
    m_curveID(-1),
    m_contourID(-1),
    m_tag_pt0(-1), m_tag_pt1(-1)
  {
    m_raw_index[0]=ind0;
    m_raw_index[1]=ind1;
    
    init(dbg);    
  }

  BezierCurve::
  BezierCurve(geometry_data dbg, 
              GLushort ind0, GLushort ind1, GLushort ind2):
    m_raw_index(3),
    m_curveID(-1),
    m_contourID(-1),
    m_tag_pt0(-1), m_tag_pt1(-1)
  {
    if(is_flat_curve(dbg, ind0, ind1, ind2))
      {
        m_raw_index.pop_back();
        m_raw_index[0]=ind0;
        m_raw_index[1]=ind2;
      }
    else
      {
        m_raw_index[0]=ind0;
        m_raw_index[1]=ind1;
        m_raw_index[2]=ind2;
      }

    init(dbg);
  }

  BezierCurve::
  BezierCurve(geometry_data dbg, 
              GLushort ind0, GLushort ind1, GLushort ind2, GLushort ind3):
    m_raw_index(4),
    m_curveID(-1),
    m_contourID(-1),
    m_tag_pt0(-1), m_tag_pt1(-1)
  {

    /*
      TODO: check if the curve is really a quadratic
      (or linear) and adjust accordingly.
     */

    m_raw_index[0]=ind0;
    m_raw_index[1]=ind1;
    m_raw_index[2]=ind2;
    m_raw_index[3]=ind3;
    
    init(dbg);
  }

  void
  BezierCurve::
  reverse_curve(void)
  {
    std::reverse(m_raw_index.begin(), m_raw_index.end());
    std::reverse(m_raw_curve.begin(), m_raw_curve.end());

    //regenerate raw polynomial:
    const_c_array<ivec2> R(&m_raw_curve[0], m_raw_curve.size());
    WRATHUtil::generate_polynomial_from_bezier(R, m_curve);

    //swap some datums..
    std::swap(m_tag_pt0, m_tag_pt1);
    std::swap(m_pt0, m_pt1);
    std::swap(m_deriv_ipt0, m_deriv_ipt1);
    std::swap(m_deriv_fpt0, m_deriv_fpt1);

    //negate derivatives:
    m_deriv_ipt0=-m_deriv_ipt0;
    m_deriv_ipt1=-m_deriv_ipt1;

    m_deriv_fpt0=-m_deriv_fpt0;
    m_deriv_fpt1=-m_deriv_fpt1;
    
    //tweak m_maximal_minimal_points
    for(std::vector<maximal_minimal_point_type>::iterator iter=m_maximal_minimal_points.begin(), 
          end=m_maximal_minimal_points.end();
        iter!=end; ++iter)
      {
        iter->m_t=1.0f - iter->m_t;
        iter->m_derivative= -iter->m_derivative;
      }
  }

  enum return_code
  BezierCurve::
  approximate_cubic(geometry_data dbg, 
                    vecN<BezierCurve*, 4> &out_curves) const
  {
    if(degree()!=3)
      {
        out_curves=vecN<BezierCurve*, 4>(NULL);
        return routine_fail;
      }

    /*
      should we do the arithmatic in integer or float?

      Should we do the arithmatic in 64bit ints
      and scale the imput before and after hand
      to avoid successive rounding uglies?

      To get perfect avoiding of such, requires multiplying
      by _64_ since this_curve.pMid has an 8 in the denomitor
      of the source m_raw_curve and each of alpha and beta
      are from that, another factor of 8, together it is 64.
     */
    CubicBezierHelper<int> this_curve(m_raw_curve[0], m_raw_curve[1], m_raw_curve[2], m_raw_curve[3]);
    CubicBezierHelper<int> alpha(this_curve.p0, this_curve.p0_1, this_curve.p01_12, this_curve.pMid);
    CubicBezierHelper<int> beta(this_curve.pMid, this_curve.p12_23, this_curve.p2_3, this_curve.p3);

    ivec2 pA, pB, pC, pD;

    pA=compute_midpoint(this_curve.p0_1, compute_midpoint(this_curve.p0_1, this_curve.p0));
    pB=compute_midpoint(this_curve.p01_12, compute_midpoint(this_curve.p01_12, this_curve.pMid));
    pC=compute_midpoint(this_curve.p12_23, compute_midpoint(this_curve.p12_23, this_curve.pMid));
    pD=compute_midpoint(this_curve.p2_3, compute_midpoint(this_curve.p2_3, this_curve.p3));

    /*
      the curves are:
       [p0, pA, alpha.pMid]
       [alpha.pMid, pB, pMid]
       [pMid, pC, beta.pMid]
       [beta.pMid, pD, p3]
     */
    GLushort iA, ialphaMid, iB, iMid, iC, ibetaMid, iD;

    iA=dbg.push_back(pA, FT_CURVE_TAG_CONIC);
    ialphaMid=dbg.push_back(alpha.pMid, FT_CURVE_TAG_ON);

    iB=dbg.push_back(pB, FT_CURVE_TAG_CONIC);
    iMid=dbg.push_back(this_curve.pMid, FT_CURVE_TAG_ON);

    iC=dbg.push_back(pC, FT_CURVE_TAG_CONIC);
    ibetaMid=dbg.push_back(beta.pMid, FT_CURVE_TAG_ON);

    iD=dbg.push_back(pD, FT_CURVE_TAG_CONIC);

    out_curves[0]=create_line_if_flat(dbg, m_raw_index[0], iA, ialphaMid);
    out_curves[1]=create_line_if_flat(dbg, ialphaMid, iB, iMid);
    out_curves[2]=create_line_if_flat(dbg, iMid, iC, ibetaMid);
    out_curves[3]=create_line_if_flat(dbg, ibetaMid, iD, m_raw_index[3]);

    //out_curves[0]=WRATHNew BezierCurve(dbg, m_raw_index[0], iA, ialphaMid);
    //out_curves[1]=WRATHNew BezierCurve(dbg, ialphaMid, iB, iMid);
    //out_curves[2]=WRATHNew BezierCurve(dbg, iMid, iC, ibetaMid);
    //out_curves[3]=WRATHNew BezierCurve(dbg, ibetaMid, iD, m_raw_index[3]);

    return routine_success;
  }

  enum return_code
  BezierCurve::
  approximate_cubic(geometry_data dbg, 
                    vecN<BezierCurve*, 2> &out_curves) const
  {
    if(degree()!=3)
      {
        out_curves=vecN<BezierCurve*, 2>(NULL);
        return routine_fail;
      }

    CubicBezierHelper<int> this_curve(m_raw_curve[0], m_raw_curve[1], m_raw_curve[2], m_raw_curve[3]);
    GLushort iMid;

    iMid=dbg.push_back(this_curve.pMid, FT_CURVE_TAG_ON);

    out_curves[0]=create_line_if_flat(dbg, m_raw_index[0], m_raw_index[1], iMid);
    out_curves[1]=create_line_if_flat(dbg, iMid, m_raw_index[2], m_raw_index[3]);
    //out_curves[0]=WRATHNew BezierCurve(dbg, m_raw_index[0], m_raw_index[1], iMid);
    //out_curves[1]=WRATHNew BezierCurve(dbg, iMid, m_raw_index[2], m_raw_index[3]);
   
    return routine_success;
  }
  
  BezierCurve*
  BezierCurve::
  approximate_cubic(geometry_data dbg) const
  {
    if(degree()!=3)
      {
        return NULL;
      }

    ivec2 c;
    int ic;

    c=compute_midpoint(m_raw_curve[1], m_raw_curve[2]);
    ic=dbg.push_back(c, FT_CURVE_TAG_CONIC);

    return create_line_if_flat(dbg, m_raw_index[0], ic, m_raw_index[3]);
  }


  vecN<BezierCurve*,2>
  BezierCurve::
  split_curve(geometry_data dbg) const
  {
    vecN<BezierCurve*,2> R(NULL, NULL);

    /*
      we need to handle each of the degree's separately
     */
    switch(m_raw_curve.size())
      {
      default:
        {
          std::cerr << "Invalid curve size for splitting: " 
                    << m_raw_curve.size() << " recovering via fakery\n";
        }
        break;

      case 2:
        {
          ivec2 ptU;
          GLushort ind;

          ptU=(pt0()+pt1())/2;
          ind=dbg.push_back(ptU, FT_CURVE_TAG_ON);

          R[0]=WRATHNew BezierCurve(dbg, m_raw_index[0], ind);
          R[0]->contourID(m_contourID);

          R[1]=WRATHNew BezierCurve(dbg, ind, m_raw_index[1]);
          R[1]->contourID(m_contourID);
        }
        break;

      case 3:
        {
          /*
            quadratic:

            q(t) = (1-t)^2 a + 2*t*(1-t) b + t^2 c

            then q(0)=a, q(1)=c and q(1/2)= a/4 + b/2 + c/4.
            Hence b= 2*( q(1/2) - a/4 - b/4) = 2*q(1/2) - a/2 - b/2
            
            thus our new curves:

            [0, 1/2] --> p(t)= [a, Y, q(1/2) ]
            where Y = 2*q(1/4) - a/2 - q(1/2)/2 
            Note that q(1/4) = (9*a + 6*b + c)/16

            [1/2,1] --> r(t) = [q(1/2), Z, c]
            where Z = 2*q(3/4) - q(1/2)/2 - c/2
           */

          ivec2 a, b, c;
          ivec2 Y, Z, q12;
          ivec2 four_q12;
          ivec2 sixteen_q14, sixteen_q12, sixteen_q34;
          ivec2 eight_Y, eight_Z, eight_q12;
          int iZ, iY, iq12;

          a=m_raw_curve[0];
          b=m_raw_curve[1];
          c=m_raw_curve[2];

          four_q12=(a+2*b+c);
          eight_q12=2*four_q12;
          sixteen_q12=2*eight_q12;

          sixteen_q14=(9*a+6*b+c);
          sixteen_q34=(a+6*b+9*c);

          //Y=2*q(1/4) - a/2 - q(1/2)/2
          //so 8Y= 16*q(1/4) - 4*a - 4*q(1/2)
          eight_Y= sixteen_q14 - 4*a - four_q12;
          Y=eight_Y/8;

          //Z=2*q(3/4) - q(1/2)/2 - c/2
          //so 8Z= 16*q(3/4) - 4*q(1/2) - 4*c;
          eight_Z=sixteen_q34 - four_q12 - 4*c;
          Z=eight_Z/8;

          q12=four_q12/4;
          
          iq12=dbg.push_back(q12, FT_CURVE_TAG_ON);
          iY=dbg.push_back(Y, FT_CURVE_TAG_CONIC);
          iZ=dbg.push_back(Z, FT_CURVE_TAG_CONIC);
          WRATHunused(iZ);
          
          R[0]=WRATHNew BezierCurve(dbg, m_raw_index[0], iY, iq12);
          R[0]->contourID(m_contourID);
          
          R[1]=WRATHNew BezierCurve(dbg, iq12, iY, m_raw_index[2]);
          R[1]->contourID(m_contourID);
        }
        break;

      case 4:
        {
          CubicBezierHelper<int> this_curve(m_raw_curve[0], m_raw_curve[1], m_raw_curve[2], m_raw_curve[3]);
          CubicBezierHelper<int> alpha(this_curve.p0, this_curve.p0_1, this_curve.p01_12, this_curve.pMid);
          CubicBezierHelper<int> beta(this_curve.pMid, this_curve.p12_23, this_curve.p2_3, this_curve.p3);

          GLushort ip0_1, ip01_12, ipMid, ip12_23, ip2_3;

          ip0_1=dbg.push_back(this_curve.p0_1, FT_CURVE_TAG_CUBIC);
          ip01_12=dbg.push_back(this_curve.p01_12, FT_CURVE_TAG_CUBIC);

          ipMid=dbg.push_back(this_curve.pMid, FT_CURVE_TAG_ON);

          ip12_23=dbg.push_back(this_curve.p12_23, FT_CURVE_TAG_CUBIC);
          ip2_3=dbg.push_back(this_curve.p2_3, FT_CURVE_TAG_CUBIC);

          R[0]=WRATHNew BezierCurve(dbg, m_raw_index[0], ip0_1, ip01_12, ipMid);
          R[1]=WRATHNew BezierCurve(dbg, ipMid, ip12_23, ip2_3, m_raw_index[3]);
        }
        break;
          
      }

    return R;
  }
  
  void
  BezierCurve::
  init_pt_tags(const BezierCurve *prev_curve,
               const BezierCurve *next_curve)
  {
    WRATHassert(m_tag_pt0==-1 and m_tag_pt1==-1);

    vec2 d0, d1, p1, n0;

    d0=compute_pt_at_t(0.0f);   
    p1=prev_curve->compute_pt_at_t(1.0f);
    m_tag_pt0=compute_tag_value(p1, d0);

    d1=compute_pt_at_t(1.0f);
    n0=next_curve->compute_pt_at_t(0.0f);
    m_tag_pt1=compute_tag_value(d1, n0);
  }

  void
  BezierCurve::
  print_info(std::ostream &ostr) const
  {
    ostr << "{ " << m_raw_curve[0];
    for(unsigned int i=1;i<m_raw_curve.size(); ++i)
      {
        ostr << ", " << m_raw_curve[i];
      }
    ostr << " } degree=" << degree();
  }

  void
  BezierCurve::
  compute_line_intersection(int in_pt, 
                            enum WRATHUtil::coordinate_type tp,
                            std::vector<solution_point> &out_pts,
                            bool compute_derivatives) const
  {
    int sz;
    vecN<int, 4> work_array;
    std::vector<WRATHUtil::polynomial_solution_solve> ts;

   
        
    WRATHassert(m_curve.x().size()==m_curve.y().size());
    WRATHassert(m_curve.x().size()==m_raw_curve.size());
    sz=m_curve.x().size();

    if(sz==2
       and in_pt==pt0()[WRATHUtil::fixed_coordinate(tp)]
       and in_pt==pt1()[WRATHUtil::fixed_coordinate(tp)])
      {
        //this is a vertial or horizontal line parrallel to line.
        //out_pts.push_back( solution_point(1, pt0()[WRATHUtil::varying_coordinate(tp)], this, 0.0f));
        //out_pts.push_back( solution_point(1, pt1()[WRATHUtil::varying_coordinate(tp)], this, 1.0f));
        //WRATHassert(false);
        return;
      }

    WRATHassert(m_tag_pt0!=-1);
    WRATHassert(m_tag_pt1!=-1);

    if(in_pt==pt0()[WRATHUtil::fixed_coordinate(tp)] and !count_as_multiplicity2(tp, m_tag_pt0))
      {
        out_pts.push_back( solution_point(1, pt0()[WRATHUtil::varying_coordinate(tp)], this, 0.0f));
        if(compute_derivatives)
          {
            out_pts.back().m_derivative=deriv_fpt0();
          }
      }

    if(in_pt==pt1()[WRATHUtil::fixed_coordinate(tp)] and !count_as_multiplicity2(tp, m_tag_pt1))
      {
        out_pts.push_back( solution_point(1, pt0()[WRATHUtil::varying_coordinate(tp)], this, 1.0f));
        if(compute_derivatives)
          {
            out_pts.back().m_derivative=deriv_fpt1();
          }
      }
    

    WRATHassert(sz==2 or sz==3 or sz==4);
    
    std::copy(m_curve[WRATHUtil::fixed_coordinate(tp)].begin(), 
              m_curve[WRATHUtil::fixed_coordinate(tp)].end(), work_array.begin());
    work_array[0]-=in_pt;
    
    c_array<int> feed(work_array.c_ptr(), sz);    
    remove_end_point_solutions(feed);
    
    if(!feed.empty())
      {
        WRATHassert(0!=feed[0]);
        WRATHassert(0!=sum_array(feed.begin(), feed.end()));
        WRATHUtil::solve_polynomial(feed, ts, false);
        
        for(unsigned int i=0, end_i=ts.size(); i<end_i; ++i)
          {
            vec2 pt;
            
            pt=compute_pt_at_t(ts[i].m_t);
            out_pts.push_back( solution_point(ts[i].m_multiplicity, 
                                              pt[WRATHUtil::varying_coordinate(tp)], 
                                              this, ts[i].m_t) );
            
            if(compute_derivatives)
              {
                out_pts.back().m_derivative=compute_deriv_at_t(ts[i].m_t);
              }
          }
      }
  }
  
  void
  BezierCurve::
  compute_line_intersection(int in_pt, enum WRATHUtil::coordinate_type tp, 
                            std::vector<simple_line> &out_pts,
                            bool include_pt_intersections) const
  {
    int sz;
    vecN<int, 4> work_array;
    std::vector<WRATHUtil::polynomial_solution_solve> ts;
        
    WRATHassert(m_curve.x().size()==m_curve.y().size());
    WRATHassert(m_curve.x().size()==m_raw_curve.size());
    sz=m_curve.x().size();
    
    WRATHassert(sz==2 or sz==3 or sz==4);

    if(in_pt==pt0()[WRATHUtil::fixed_coordinate(tp)] and include_pt_intersections)
      {
        solution_point V(1, 0.0f, this);

        out_pts.push_back(simple_line(V, 
                                      fpt0()[varying_coordinate(tp)],
                                      deriv_fpt0()) );
        out_pts.back().m_intersection_type=intersect_at_0;

      }

    if(in_pt==pt1()[WRATHUtil::fixed_coordinate(tp)] and include_pt_intersections)
      {
        solution_point V(1, 1.0f, this);

        out_pts.push_back(simple_line(V, 
                                      fpt1()[varying_coordinate(tp)],
                                      deriv_fpt1()) );
        out_pts.back().m_intersection_type=intersect_at_1;
      }

    if(sz==2
       and in_pt==pt0()[WRATHUtil::fixed_coordinate(tp)]
       and in_pt==pt1()[WRATHUtil::fixed_coordinate(tp)])
      {
        //this is a vertial or horizontal line parrallel 
        //with same point as the in_pt. 
        return;
      }
    
    std::copy(m_curve[WRATHUtil::fixed_coordinate(tp)].begin(), 
              m_curve[WRATHUtil::fixed_coordinate(tp)].end(), work_array.begin());
    work_array[0]-=in_pt;
    
    
    c_array<int> feed(work_array.c_ptr(), sz);    
    remove_end_point_solutions(feed);

    if(!feed.empty())
      {
        WRATHassert(0!=feed[0]);
        WRATHassert(0!=sum_array(feed.begin(), feed.end()));
        WRATHUtil::solve_polynomial(feed, ts, false);

        for(unsigned int i=0, end_i=ts.size(); i<end_i; ++i)
          {
            vec2 pt, deriv;
            
            pt=compute_pt_at_t(ts[i].m_t);
            deriv=compute_deriv_at_t(ts[i].m_t);
            
            solution_point V(ts[i].m_multiplicity,
                             ts[i].m_t, this);
            
            out_pts.push_back(simple_line(V, pt[WRATHUtil::varying_coordinate(tp)], deriv) );
          }
      }
  }

  vec2
  BezierCurve::
  compute_deriv_at_t(float t) const
  {
    vec2 return_value(0.0f, 0.0f);

    for(int coord=0; coord<2; ++coord)
      {
        float factor;
        int i, end_i;

        for(factor=1.0f, i=1, end_i=m_curve[coord].size(); i<end_i; ++i, factor*=t)
          {
            return_value[coord]+= static_cast<float>(i*m_curve[coord][i])*factor;
          }
      }
    return return_value;
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
    // it is O(N^3) but for now, I don't want to bother or care..
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
  init(geometry_data dbg)
  {
    m_raw_curve.resize(m_raw_index.size());
    for(unsigned int K=0, endK=m_raw_index.size(); K<endK; ++K)
      {
        m_raw_curve[K]=dbg.pt(m_raw_index[K]);
      }

    //generate raw polynomial:
    const_c_array<ivec2> R(&m_raw_curve[0], m_raw_curve.size());
    WRATHUtil::generate_polynomial_from_bezier(R, m_curve);

    //generate the points where dx/dt=dy/dt or dx/dt=-dy/dt
    compute_maximal_minimal_points();

    //generate the points where dx/dt=0 or dy/dt=0
    compute_extremal_points();

    //find the bounding box of the curve
    compute_bounding_box();

    m_pt0=vec2( static_cast<float>(m_raw_curve.front().x()), 
                static_cast<float>(m_raw_curve.front().y()) );

    m_pt1=vec2( static_cast<float>(m_raw_curve.back().x()), 
                static_cast<float>(m_raw_curve.back().y()) );

   
    m_deriv_ipt0.x()=(m_curve.x().size()>1)?
      m_curve.x()[1]:0;

    m_deriv_ipt0.y()=(m_curve.y().size()>1)?
      m_curve.y()[1]:0;

    m_deriv_ipt1=ivec2(0,0);
    for(unsigned int i=1, end_i=m_curve.x().size(); i<end_i; ++i)
      {
        m_deriv_ipt1.x()+= i*m_curve.x()[i];
      }
    for(unsigned int i=1, end_i=m_curve.y().size(); i<end_i; ++i)
      {
        m_deriv_ipt1.y()+= i*m_curve.y()[i];
      }
    


    m_deriv_fpt0=vec2( static_cast<float>(m_deriv_ipt0.x()),
                       static_cast<float>(m_deriv_ipt0.y()));

    m_deriv_fpt1=vec2( static_cast<float>(m_deriv_ipt1.x()),
                       static_cast<float>(m_deriv_ipt1.y()));
  }

  void
  BezierCurve::
  compute_extremal_points(void)
  {
    if(m_curve.x().size()<2)
      {
        return;
      }

    for(int coord=0; coord<2; ++coord)
      {
        vecN<int, 3> work_array;
        std::vector<WRATHUtil::polynomial_solution_solve> ts;

        for(int k=1, end_k=m_curve[coord].size(); k<end_k; ++k)
          {
            work_array[k-1]=k*m_curve[coord][k];            
          }

        WRATHUtil::solve_polynomial(c_array<int>(work_array.c_ptr(), m_curve[coord].size()-1), 
                                    ts, false);

        for(unsigned int i=0, end_i=ts.size(); i<end_i; ++i)
          {
            vec2 q;
            
            q=compute_pt_at_t(ts[i].m_t);
            m_extremal_points[coord].push_back(q);
          }
      }

    
  }

  void
  BezierCurve::
  compute_maximal_minimal_points(void) 
  {
    //now save the points for where the derivative is 0.
    int sz;
    vecN<int, 4> work_arrayDelta, work_arraySum;
    std::vector<WRATHUtil::polynomial_solution_solve> ts;
    
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
        WRATHUtil::solve_polynomial(c_array<int>(work_arraySum.c_ptr(), sz-1), ts, false);
        WRATHUtil::solve_polynomial(c_array<int>(work_arrayDelta.c_ptr(), sz-1), ts, false);

        for(unsigned int i=0, end_i=ts.size(); i<end_i; ++i)
          {
            vec2 q;

            q=compute_pt_at_t(ts[i].m_t);

            m_maximal_minimal_points.push_back(maximal_minimal_point_type());
            m_maximal_minimal_points.back().m_multiplicity=ts[i].m_multiplicity;
            m_maximal_minimal_points.back().m_t=ts[i].m_t;
            m_maximal_minimal_points.back().m_pt=q;
            m_maximal_minimal_points.back().m_derivative=compute_deriv_at_t(ts[i].m_t);
          }
      }
  }

  void
  BezierCurve::
  compute_bounding_box(void)
  {
    

    m_min_corner.x()=static_cast<float>(std::min(m_raw_curve.front().x(), 
                                                 m_raw_curve.back().x()));

    m_min_corner.y()=static_cast<float>(std::min(m_raw_curve.front().y(), 
                                                 m_raw_curve.back().y()));

    m_max_corner.x()=static_cast<float>(std::max(m_raw_curve.front().x(), 
                                                 m_raw_curve.back().x()));

    m_max_corner.y()=static_cast<float>(std::max(m_raw_curve.front().y(), 
                                                 m_raw_curve.back().y()));


    for(std::vector<maximal_minimal_point_type>::const_iterator 
          iter=m_maximal_minimal_points.begin(),
          end=m_maximal_minimal_points.end(); iter!=end; ++iter)
      {
        m_min_corner.x()=std::min(iter->m_pt.x(), m_min_corner.x());
        m_min_corner.y()=std::min(iter->m_pt.y(), m_min_corner.y());

        m_max_corner.x()=std::max(iter->m_pt.x(), m_max_corner.x());
        m_max_corner.y()=std::max(iter->m_pt.y(), m_max_corner.y());
      }

    for(int i=0;i<2;++i)
      {
        for(std::vector<vec2>::const_iterator 
              iter=m_extremal_points[i].begin(),
              end=m_extremal_points[i].end(); iter!=end; ++iter)
          {
            m_min_corner.x()=std::min(iter->x(), m_min_corner.x());
            m_min_corner.y()=std::min(iter->y(), m_min_corner.y());
            
            m_max_corner.x()=std::max(iter->x(), m_max_corner.x());
            m_max_corner.y()=std::max(iter->y(), m_max_corner.y());
          }
      }

  }

  /////////////////////////////////////////////
  // WRATHFreeTypeSupport::ContourEmitterFromFT_Face metods
  
  void
  ContourEmitterFromFT_Outline::
  produce_contours(geometry_data dbg)
  {
    const_c_array<FT_Vector> pts;
    const_c_array<char> pts_tag;
    int last_contour_end(0);
    bool reverse_orientation;

    reverse_orientation=(m_outline.flags&FT_OUTLINE_REVERSE_FILL)!=0;
   


    for(int c=0, end_c=m_outline.n_contours; c<end_c; ++c)
      {
        int sz;
        range_type<int> O(0,0);

        sz=m_outline.contours[c] - last_contour_end + 1;
        
        pts=const_c_array<FT_Vector>(m_outline.points+last_contour_end, sz);
        pts_tag=const_c_array<char>(m_outline.tags+last_contour_end, sz);

        add_curves_from_contour(dbg, reverse_orientation, 
                                pts, pts_tag, m_scale_factor); 
        emit_end_contour();

        last_contour_end=m_outline.contours[c]+1;

      }
  }


  void
  ContourEmitterFromFT_Outline::
  add_curves_from_contour(geometry_data dbg,
                          bool reverse_orientation,
                          const_c_array<FT_Vector> pts,
                          const_c_array<char> pts_tag,
                          int scale)
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
    int start_index(dbg.pts().size()), end_index;
    std::vector<BezierCurve*> work_curves;

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

            implicit_pt*=scale;
            dbg.push_back(implicit_pt, FT_CURVE_TAG_ON);
          }

        ivec2 add_pt(pts[k].x, pts[k].y);

        add_pt*=scale;
        dbg.push_back( add_pt, pts_tag[k]);
      } 
    end_index=dbg.pts().size();

    point_type::point_classification prev_tag(dbg.tag(start_index));
    point_type::point_classification prev_prev_tag(dbg.tag(end_index-1));
    point_type::point_classification tag;




    for(int k=start_index+1, end_k=end_index; k<=end_k; ++k)
      {
        int real_k(k==end_k?start_index:k);

        tag=dbg.tag(real_k);


        if(tag==point_type::on_curve and
           prev_tag==point_type::on_curve)
          {
            int pt0(k-1), pt1(real_k);

            if(reverse_orientation)
              {
                std::swap(pt0, pt1);
              }

            work_curves.push_back(WRATHNew BezierCurve(dbg, pt0, pt1));
          }
        else if(tag==point_type::on_curve 
                and prev_tag==point_type::conic_off_curve
                and prev_prev_tag==point_type::on_curve) 
          {
            int k_minus_2;

            k_minus_2=(k>start_index+1)?
              k-2:
              end_k-1;

            int pt0(k_minus_2), pt1(k-1), pt2(real_k);
            if(reverse_orientation)
              {
                std::swap(pt0, pt2);
              }


            work_curves.push_back(WRATHNew BezierCurve(dbg, pt0, pt1, pt2));
          }
        else if(tag==point_type::cubic_off_curve 
                and prev_tag==point_type::cubic_off_curve
                and prev_prev_tag==point_type::on_curve)
          {
            int next_k;
            int k_minus_2;

            if(real_k+1<end_k)
              {
                next_k=k+1;
              }
            else if(real_k+1==end_k)
              {
                next_k=start_index;
              }
            else // thus real_k==end_k
              {
                next_k=start_index+1;
              }

            k_minus_2=(k>start_index+1)?
              k-2:
              end_k-1;

            int pt0(k_minus_2), pt1(k-1), pt2(real_k), pt3(next_k);
            if(reverse_orientation)
              {
                std::swap(pt0, pt3);
                std::swap(pt1, pt2);
              }


            work_curves.push_back(WRATHNew BezierCurve(dbg, pt0, pt1, pt2, pt3));
          }
        prev_prev_tag=prev_tag;
        prev_tag=tag;
      }

    if(reverse_orientation)
      {
        std::reverse(work_curves.begin(), work_curves.end());
      }
    
    for(std::vector<BezierCurve*>::iterator iter=work_curves.begin(),
          end=work_curves.end(); iter!=end; ++iter)
      {
        emit_curve(*iter);
      }
  }


  /////////////////////////////////////////////
  // WRATHFreeTypeSupport::RawOutlineData methods
  RawOutlineData::
  RawOutlineData(const FT_Outline &outline, 
                 int pscale_factor,
                 geometry_data pdbg):
    m_dbg(pdbg)
  {
    ContourEmitterFromFT_Outline emitter(outline, pscale_factor);
    build_outline(&emitter);
  }

  RawOutlineData::
  RawOutlineData(ContourEmitterBase *emitter,
                 geometry_data pdbg):
    m_dbg(pdbg)
  {
    build_outline(emitter);
  }

  RawOutlineData::
  ~RawOutlineData()
  {
    for(std::vector<BezierCurve*>::iterator iter=m_bezier_curves.begin(),
          end=m_bezier_curves.end(); iter!=end; ++iter)
      {
        BezierCurve *c(*iter);
        WRATHDelete(c);
      }
  }

  void
  RawOutlineData::
  reverse_component(int ID)
  {
    WRATHassert(ID>=0 and ID<static_cast<int>(m_curve_sets.size()) );

    c_array<BezierCurve*> all_curves(m_bezier_curves);
    c_array<BezierCurve*> component(all_curves.sub_array(m_curve_sets[ID]));
 
    std::reverse(component.begin(), component.end());

    for(int C=m_curve_sets[ID].m_begin; C!=m_curve_sets[ID].m_end; ++C)
      {
        m_bezier_curves[C]->reverse_curve();
        m_bezier_curves[C]->curveID(C);
      }
    
  }

  void
  RawOutlineData::
  build_outline(ContourEmitterBase *emitter)
  {
    boost::signals2::connection c0, c1;

    WRATHassert(emitter!=NULL);
    c0=emitter->connect_emit_curve( boost::bind(&RawOutlineData::catch_curve,
                                                this, _1));

    c1=emitter->connect_emit_end_contour( boost::bind(&RawOutlineData::mark_contour_end,
                                                      this));

    emitter->produce_contours(m_dbg);


    //init_pt_tags
    for(int i=0, endi=m_bezier_curves.size(); i<endi; ++i)
      {
        BezierCurve *c(m_bezier_curves[i]);
        c->init_pt_tags(prev_neighbor(c), next_neighbor(c));
      }

    c0.disconnect();
    c1.disconnect();
  }

  void
  RawOutlineData::
  mark_contour_end(void)
  {
    int begin_value;

    if(m_curve_sets.empty())
      {
        begin_value=0;
      }
    else
      {
        begin_value=m_curve_sets.back().m_end;
      }

    m_curve_sets.push_back(range_type<int>(begin_value, m_bezier_curves.size()));
  }

  void
  RawOutlineData::
  catch_curve(BezierCurve *c)
  {
    c->contourID(m_curve_sets.size());
    c->curveID(m_bezier_curves.size());
    m_bezier_curves.push_back(c);
  }

  const BezierCurve*
  RawOutlineData::
  prev_neighbor(const BezierCurve *c) const
  {
    if(c==NULL)
      {
        return NULL;
      }

    int contourID(c->contourID());
    int curveID(c->curveID());

    if(contourID<0 or contourID>=static_cast<int>(m_curve_sets.size()))
      {
        return NULL;
      }

    const range_type<int> &R(m_curve_sets[contourID]);

    if(curveID>=R.m_end or curveID<R.m_begin)
      {
        return NULL;
      }

    int I;

    I=(curveID==R.m_begin)?
      R.m_end-1:
      curveID-1;

    return m_bezier_curves[I];
      
  }

  const BezierCurve*
  RawOutlineData::
  next_neighbor(const BezierCurve *c) const
  {
    if(c==NULL)
      {
        return NULL;
      }

    int contourID(c->contourID());
    int curveID(c->curveID());

    if(contourID<0 or contourID>=static_cast<int>(m_curve_sets.size()))
      {
        return NULL;
      }

    const range_type<int> &R(m_curve_sets[contourID]);

    if(curveID>=R.m_end or curveID<R.m_begin)
      {
        return NULL;
      }

    int I;
    I=(curveID==R.m_end-1)?
      R.m_begin:
      curveID+1;

    return m_bezier_curves[I];
  }

  /////////////////////////////////////////////
  // WRATHFreeTypeSupport::CoordinateConverter methods
  CoordinateConverter::
  CoordinateConverter(int pscale_factor,
                      const ivec2 &pbitmap_size, 
                      const ivec2 &pbitmap_offsett,
                      int pinternal_offset):
    m_scale_factor(pscale_factor),
    m_internal_offset(pinternal_offset),    
    m_bitmap_size(pbitmap_size),
    m_bitmap_offset(pbitmap_offsett),
    m_half_texel_size(32*pscale_factor),
    m_distance_scale_factor(1.0f/static_cast<float>(pscale_factor)) //reciprocal of scale_factor().
  {
    //glyph normalization constants:
    m_glyph_bottom_left=vec2( point_from_bitmap_x(0) - m_half_texel_size,
                              point_from_bitmap_y(0) - m_half_texel_size);
    m_glyph_top_right=vec2(point_from_bitmap_x(m_bitmap_size.x()-1) + m_half_texel_size,
                           point_from_bitmap_y(m_bitmap_size.y()-1) + m_half_texel_size);
    m_glyph_size=m_glyph_top_right-m_glyph_bottom_left;

    if(m_bitmap_size.x()>0 and m_bitmap_size.y()>0)
      {
        m_glyph_size_reciprocal=vec2(1.0f, 1.0f)/m_glyph_size;
      }
    else
      {
        m_glyph_size_reciprocal=vec2(0.0f, 0.0f);
      }

    //texel normalization constants 
    m_texel_size_i=2*ivec2(m_half_texel_size, m_half_texel_size);
    m_texel_size_f=vec2(m_texel_size_i.x(),
                        m_texel_size_i.y());
  }

  vec2
  CoordinateConverter::
  normalized_glyph_coordinate(const ivec2 &ipt) const
  {
    vec2 fpt(ipt.x(), ipt.y());
    return (fpt-m_glyph_bottom_left)*m_glyph_size_reciprocal;
  }

  ivec2
  CoordinateConverter::
  texel(ivec2 pt0) const
  {
    pt0-=ivec2(m_internal_offset, m_internal_offset);
    pt0/=scale_factor();
    pt0/=64;
    return pt0;
  }

  bool
  CoordinateConverter::
  same_texel(ivec2 pt0,
             ivec2 pt1) const
  {
    return texel(pt0)==texel(pt1);
  }

  
  //////////////////////////////////////////////
  // WRATHFreeTypeSupport::OutlineData methods
  OutlineData::
  OutlineData(const FT_Outline &outline, 
              const ivec2 &bitmap_size, 
              const ivec2 &bitmap_offset,
              geometry_data pdbg):
    CoordinateConverter(4, bitmap_size, bitmap_offset),
    RawOutlineData(outline, scale_factor(), pdbg)
  {}

  OutlineData::
  OutlineData(ContourEmitterBase *emitter,
              int pscale_factor,
              const ivec2 &bitmap_size, 
              const ivec2 &bitmap_offset,
              geometry_data pdbg):
    CoordinateConverter(pscale_factor, bitmap_size, bitmap_offset),
    RawOutlineData(emitter, pdbg)
  {}

  OutlineData::
  OutlineData(ContourEmitterBase *emitter,
              const CoordinateConverter &converter,
              geometry_data pdbg):
    CoordinateConverter(converter),
    RawOutlineData(emitter, pdbg)
  {}

  void
  OutlineData::
  compute_bounding_box(const BezierCurve *c, 
                       ivec2 &out_min, ivec2 &out_max) const
  {
    vec2 pmin, pmax;

    pmin=bitmap_from_point(c->min_corner());
    pmax=bitmap_from_point(c->max_corner());

    out_min.x()=std::max(0, -1+static_cast<int>(pmin.x()));
    out_min.y()=std::max(0, -1+static_cast<int>(pmin.y()));

    out_max.x()=std::min(bitmap_size().x(), 2+static_cast<int>(pmax.x()));
    out_max.y()=std::min(bitmap_size().y(), 2+static_cast<int>(pmax.y()));
  }
    

  void
  OutlineData::
  compute_distance_values(boost::multi_array<distance_return_type, 2> &victim, 
                          float max_dist_value, bool compute_winding_number) const  
  {
    int radius;

    radius=std::floor(max_dist_value/64.0f);

    init_distance_values(victim, max_dist_value);
    compute_outline_point_values(victim, radius);
    compute_zero_derivative_values(victim, radius);
    compute_fixed_line_values(victim, compute_winding_number);
  }

  void
  OutlineData::
  init_distance_values(boost::multi_array<distance_return_type, 2> &victim, 
                       float max_dist_value) const
  {
    //init victim:
    for(int x=0;x<bitmap_size().x();++x)
      {
        for(int y=0;y<bitmap_size().y();++y)
          {
            victim[x][y].m_distance.init(max_dist_value);
          }
      }
  }

  void
  OutlineData::
  compute_outline_point_values(boost::multi_array<distance_return_type, 2> &victim,
                               int radius) const
  {
    for(unsigned int i=0, end_i=number_curves(); i<end_i; ++i)
      {
        vec2 fpt;
        ivec2 ipt;
        const BezierCurve *curve(bezier_curve(i));

        fpt.x()=curve->pt0().x();
        fpt.y()=curve->pt0().y();
        
        ipt.x()=bitmap_x_from_point(fpt.x());
        ipt.y()=bitmap_y_from_point(fpt.y());
        
        for(int x=std::max(0, ipt.x()-radius), 
              end_x=std::min(ipt.x()+radius+1, bitmap_size().x());
            x<end_x; ++x)
          {
            for(int y=std::max(0, ipt.y()-radius),
                  end_y=std::min(ipt.y()+radius+1, bitmap_size().y());
                y<end_y;++y)
              {
                float dc;
                vec2 candidate;
                vec2 pt( static_cast<float>(point_from_bitmap_x(x)),
                         static_cast<float>(point_from_bitmap_y(y)));


                candidate=pt - fpt;
                dc=candidate.L1norm();
                dc*=distance_scale_factor();
                                   
                victim[x][y].m_distance.update_value(dc);
              }
          }
        
      }
  }

  void
  OutlineData::
  compute_zero_derivative_values(boost::multi_array<distance_return_type, 2> &victim,
                                 int radius) const
  {
    for(unsigned int i=0, end_i=number_curves(); i<end_i; ++i)
      {
        for(std::vector<BezierCurve::maximal_minimal_point_type>::const_iterator 
              iter=bezier_curve(i)->maximal_minimal_points().begin(),
              end=bezier_curve(i)->maximal_minimal_points().end(); 
            iter!=end; ++iter)
          {
            ivec2 ipt;

            ipt.x()=bitmap_x_from_point(iter->m_pt.x());
            ipt.y()=bitmap_y_from_point(iter->m_pt.y());

            
            for(int x=std::max(0, ipt.x()-radius), 
                  end_x=std::min(ipt.x()+radius+1, bitmap_size().x());
                x<end_x; ++x)
              {
                for(int y=std::max(0, ipt.y()-radius),
                      end_y=std::min(ipt.y()+radius+1, bitmap_size().y());
                    y<end_y;++y)
                  {
                    WRATHassert(iter->m_multiplicity>0);

                    vec2 candidate;
                    float dc;
                    vec2 pt( static_cast<float>(point_from_bitmap_x(x)),
                             static_cast<float>(point_from_bitmap_y(y)));
                    
                    candidate=pt - iter->m_pt;
                    dc=candidate.L1norm();
                    dc*=distance_scale_factor();

                    victim[x][y].m_distance.update_value(dc);
                      
                  }
              }
          }
      }
    
  }

  void
  OutlineData::
  compute_fixed_line_values(boost::multi_array<distance_return_type, 2> &victim,
                            bool compute_winding_number) const
  {
    std::vector< std::vector<solution_point> > work_room;

    //note we only use the x_fixed computation to compute the winding numbers!
    compute_fixed_line_values(WRATHUtil::x_fixed, victim, work_room, compute_winding_number);
    compute_fixed_line_values(WRATHUtil::y_fixed, victim, work_room, false);
  }

  void
  OutlineData::
  compute_fixed_line_values(enum WRATHUtil::coordinate_type coord_tp, 
                            boost::multi_array<distance_return_type, 2> &victim,
                            std::vector< std::vector<solution_point> > &work_room,
                            bool compute_winding_number) const
  {
    const enum inside_outside_test_results::sol_type sol[2][2]=
      {
        {inside_outside_test_results::below, inside_outside_test_results::above}, //x_fixed
        {inside_outside_test_results::left, inside_outside_test_results::right} //y_fixed
      };
    
    int coord(coord_tp);
    enum WRATHUtil::coordinate_type other_coord_tp;

    work_room.resize(std::max(static_cast<int>(work_room.size()), 
                              bitmap_size()[coord]) );

    for(int i=0;i<bitmap_size()[coord];++i)
      {
        work_room[i].clear();
      }

    for(int i=0, end_i=number_curves(); i<end_i; ++i)
      {
        int start_pt, end_pt;

        start_pt=bitmap_coord_from_point(bezier_curve(i)->min_corner()[coord], coord_tp);
        end_pt=bitmap_coord_from_point(bezier_curve(i)->max_corner()[coord], coord_tp);

        for(int c=std::max(0, start_pt-1), 
              end_c=std::min(bitmap_size()[coord], end_pt+2);
            c<end_c; ++c)
          {
            int ip;
            ip=point_from_bitmap_coord(c, coord_tp);

            bezier_curve(i)->compute_line_intersection(ip, coord_tp, work_room[c],
                                                       compute_winding_number);
          }

      }

        
    other_coord_tp=static_cast<enum WRATHUtil::coordinate_type>(1-coord);

    for(int c=0, end_c=bitmap_size()[coord]; c<end_c; ++c)
      {  
        int ip;
        std::vector<solution_point> &L(work_room[c]);
        int total_count(0);
        
        ip=point_from_bitmap_coord(c, coord_tp);
        std::sort(L.begin(), L.end());

        WRATHunused(ip);

        for(int i=0, end_i=L.size(); i<end_i; ++i)
          {
            WRATHassert(L[i].m_multiplicity>0);
            total_count+=std::max(0, L[i].m_multiplicity);
          }
        
        for(int other_c=0, end_other_c=bitmap_size()[1-coord], 
              sz=L.size(), current_count=0, current_index=0;
            other_c<end_other_c; ++other_c)
          {
            float p;
            ivec2 pixel;
            int prev_index, prev_count;
            
            pixel[coord]=c;
            pixel[1-coord]=other_c;
            
            p=static_cast<float>( point_from_bitmap_coord(other_c, other_coord_tp) );
            prev_index=current_index;
            prev_count=current_count;
            WRATHunused(prev_count);
            
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

                dc= std::abs(p-L[cindex].m_value);
                dc*=distance_scale_factor();
                victim[pixel.x()][pixel.y()].m_distance.update_value(dc);
                
              }

                        
            victim[pixel.x()][pixel.y()].
              m_solution_count.increment(sol[coord][0], current_count);
            
            victim[pixel.x()][pixel.y()].
              m_solution_count.increment(sol[coord][1], total_count - current_count);
          }
       
        
        
        if(compute_winding_number)
          {
            
            std::vector<int> cts;

            increment_sub_winding_numbers(L, coord_tp, cts);   
         
            for(int sum=0, x=0,
                  end_x=bitmap_size()[1-coord];
                x<end_x; ++x)
              {
                ivec2 pix;

                pix[coord]=c;
                pix[1-coord]=x;

                //make so that sum is the number of curves
                //for which texel center is to the right of that are rising
                //minus the number curvers for which texel center is to the right 
                //that are falling, i.e. the number of curves before the texel 
                //center that are rising minus the number of curves before the texel 
                //that are falling
                sum+=cts[x];
                
                victim[pix.x()][pix.y()].
                  m_solution_count.increment_winding(sum);
              }
            
 
          }
 
      }
  }

  void
  OutlineData::
  increment_sub_winding_numbers(const std::vector<solution_point> &L,
                                enum WRATHUtil::coordinate_type coord_tp,
                                std::vector<int> &cts) const
  {
    /*
      Basic idea: each intersection recorded 
      in L, has a precise pixel
      it is within. Within that pixel
      it is either to the left or right.
      We view the curve as rising if it's
      y-derivative at the intersecion is
      positive and as falling if it's y-derivative
      is negative. We will regard the curve as to
      the left if the x value is less than
      the center of the pixel, and will regard
      it as to the right otherwise.
      
      We will walk through the intersection list
      work_room[c], getting the x-pixel coordinate, xx,
      of the intersection and incrementing rise_cts[xx]
      if the curve is to left and is rising and
      incrementing fall_cts[xx] if the curve it to
      the right and falling.

      cts[x] = # of intersection in the range
               [center of pixel x-1, center pixel x]
               that are rising
                    - 
               # of intersection in the range
               [center of pixel x-1, center pixel x]
               that are falling
    */

    int coord(coord_tp);
    enum WRATHUtil::coordinate_type other_coord_tp;

    other_coord_tp=static_cast<enum WRATHUtil::coordinate_type>(1-coord);
    cts.resize(bitmap_size()[1-coord]+1, 0);

    for(int i=0, sz=L.size(); i<sz; ++i)
      {
        bool accept_intersection;

        accept_intersection=L[i].m_multiplicity==1
          and (L[i].m_bezier->degree()>1 or
               L[i].m_bezier->pt0()[coord]!=L[i].m_bezier->pt1()[coord]);


        if(accept_intersection)
          {
            float pxx, fxx, dy;
            int xx;
            bool intersection_after_center;
            int v;
            
            pxx=L[i].m_value;                
            fxx=bitmap_from_point(pxx, 1-coord); 
            
            xx=static_cast<int>(fxx);
            intersection_after_center=(pxx>point_from_bitmap_coord(xx, other_coord_tp));
            
            WRATHassert(xx>=0 and xx<bitmap_size()[1-coord]);
            
            dy=L[i].m_derivative[coord];
            v=(dy>0.0)?1:-1;
            
            if(intersection_after_center)
              {
                //intersecion is on the range center of x to center x+1
                cts[xx+1]+=v;
              }
            else 
              {
                //intersecion is on the range center of x-1 to center of x
                cts[xx]+=v;
              }
          }
      }
  }

  void
  OutlineData::
  compute_winding_numbers(boost::multi_array<int, 2> &victim,
                          ivec2 offset_from_center) const
  {
    std::fill(victim.data(), 
              victim.data()+victim.num_elements(), 0);

    for(int y=0;y<bitmap_size().y(); ++y)
      {
        std::vector<solution_point> solves;
        std::vector<int> cts;

        for(int i=0, end_i=number_curves(); i<end_i; ++i)
          {
            int ip;
            ip=point_from_bitmap_y(y) + offset_from_center.y();
            bezier_curve(i)->compute_line_intersection(ip, WRATHUtil::y_fixed, solves, true);
          }

        
        increment_sub_winding_numbers(solves, WRATHUtil::y_fixed, cts);

        for(int sum=0, x=0; x<bitmap_size().x(); ++x)
          {
                        
            //make so that sum is the number of curves
            //for which texel center is to the right of that are rising
            //minus the number curvers for which texel center is to the right 
            //that are falling, i.e. the number of curves before the texel 
            //center that are rising minus the number of curves before the texel 
            //that are falling
            sum+=cts[x];
            victim[x][y]+=sum;
          }
      }
  }
    

  void
  OutlineData::
  compute_analytic_values(boost::multi_array<analytic_return_type, 2> &victim,
                          std::vector<bool> &component_reversed,
                          bool include_pt_intersections) const
  {
    std::vector<int> reverse_curve_count(number_curves(), 0);
    std::vector<int> reverse_contour_count(number_components(), 0);

    /*
      for each curve we compute seperately if it thinks it
      should be reversed by simple incrementing if a texel
      says it is filled but is not and decrementing if
      texel beign filled matches with contour filled
     */
    compute_analytic_curve_values_fixed(WRATHUtil::x_fixed, victim, reverse_curve_count,
                                        include_pt_intersections);

    compute_analytic_curve_values_fixed(WRATHUtil::y_fixed, victim, reverse_curve_count,
                                        include_pt_intersections);

    /*
      For each contour, decide if the contour is
      reversed as follows: the contour is reversed
      is more curves consider themselve reversed than not.
     */
    for(int curveID=0; curveID<number_curves(); ++curveID)
      {
        int contourID( bezier_curve(curveID)->contourID());
        if(reverse_curve_count[curveID]>0)
          {
            reverse_contour_count[contourID]++;
          }
        else
          {
            reverse_contour_count[contourID]--;
          }
      }

    /*
      now that each contour is known if it reverses,
      transcribe that data to reverse_curve
     */
    component_reversed.resize(number_components());
    for(int contourID=0; contourID<number_components(); ++contourID)
      {
        component_reversed[contourID]= (reverse_contour_count[contourID]>0);
      }


  }

  void
  OutlineData::
  compute_analytic_curve_values_fixed(enum WRATHUtil::coordinate_type coord,
                                      boost::multi_array<analytic_return_type, 2> &victim,
                                      std::vector<int> &reverse_curve_count,
                                      bool include_pt_intersections) const
  {
    enum WRATHUtil::coordinate_type other_coord;
    enum boundary_type prev_bound, bound;

    other_coord=static_cast<enum WRATHUtil::coordinate_type>(1-coord);
    if(coord==WRATHUtil::x_fixed)
      {
        prev_bound=right_boundary;
        bound=left_boundary;
      }
    else
      {
        prev_bound=above_boundary;
        bound=below_boundary;
      }


    for(int x=0; x<=bitmap_size()[coord]; ++x)
      {
        std::vector<simple_line> L;
        int point_x;
        float texel_top, texel_bottom;
        int total_count;
        int current_index;

        //left edge:
        point_x=point_from_bitmap_coord(x, coord, bitmap_begin);
        L.clear();

        for(int curve=0, end_curve=number_curves(); curve<end_curve; ++curve)
          {
            bezier_curve(curve)->compute_line_intersection(point_x, coord, L,
                                                           include_pt_intersections);
          }

        std::sort(L.begin(), L.end());
        
        total_count=L.size();
        for(int i=0; i<total_count; ++i)
          {
            L[i].m_index_of_intersection=i;
            WRATHassert(L[i].m_source.m_bezier!=NULL);
          }
        
        if(total_count==0)
          continue;

        current_index=0;
        texel_top=static_cast<float>(point_from_bitmap_coord(0, other_coord, bitmap_begin));
        while(current_index<total_count
              and L[current_index].m_value<=texel_top)
          {
            ++current_index;
          }

        //at this point in time now, L lists all those curves that intersect 
        //the left edge of the texel column x, and thus the right edge of the
        //previous texel column.

        for(int y=0; y<bitmap_size()[other_coord]; ++y)
          {
            int prev_index;
            ivec2 prev_pixel;
            ivec2 pixel;

            pixel[coord]=x;
            pixel[1-coord]=y;

            prev_pixel[coord]=x-1;
            prev_pixel[1-coord]=y;


            texel_bottom=texel_top;
            texel_top=static_cast<float>(point_from_bitmap_coord(y+1, other_coord, bitmap_begin)); 
            prev_index=current_index;
          

            //increment current_index until it crosses into the texel.
            while(current_index<total_count
                  and L[current_index].m_value<=texel_top)
              {
                ++current_index;
              }
            
            //prev_index gives the number of curves below texel_bottom:
            if(x>0)
              {
                victim[prev_pixel.x()][prev_pixel.y()].m_parity_count[prev_bound]=prev_index;

                bool filled;
                filled= ( (prev_index&1)!=0);
                
                /*
                  the curve is below the "bottom" of the texel,
                  thus the vector from the curve to the bottom
                  of the texel is v=(0,1) if coord==WRATHUtil::x_fixed
                  and v=(1,0) if coord==WRATHUtil::y_fixed
                  Let n=normal_vector = J(derivative),
                  the we care about the sign of d=dot(n, v).
                  J(x,y)=(-y,x), thus:
                     WRATHUtil::x_fixed --> v=(0,1), thus d=m_derivative[0]
                     WRATHUtil::y_fixed --> v=(1,0), thus d=-m_derivative[1]
                */
                if(prev_index<total_count)
                  {
                    float dsign;
                    
                    dsign=L[prev_index].m_source.m_derivative[coord];

                    if(std::abs(dsign)>0.01f)
                      {
                        bool v;
                        
                        v=(dsign<0.0f) xor filled
                          xor (coord==WRATHUtil::x_fixed);
                        
                        /*
                          increment if this texel thinks it is reversed, otherwise
                          decrement.
                         */
                        if(v)
                          {
                            ++reverse_curve_count[L[prev_index].m_source.m_bezier->curveID()];
                          }
                        else 
                          {
                            --reverse_curve_count[L[prev_index].m_source.m_bezier->curveID()];
                          }
                      } 
                  }
                    
              }
              
            
            if(x<bitmap_size()[coord])
              {
                victim[pixel.x()][pixel.y()].m_parity_count[bound]=prev_index;
              }
            
            //at this point in time if prev_index and current index differ, then
            //L[prev_index], ..., L[current_index-1] are the curves within this texel. 
            //Regardless the canidate curves to use are L[i] for prev_index-1<=i<=current_index
            int start_k=std::max(0, prev_index-2);
            int end_k=std::min(current_index+2, total_count);
            
            for(int k=start_k; k<end_k; ++k)
              {
                //only record if the curve intersects the edge of the texel:
                if(L[k].m_value<=texel_top and L[k].m_value>=texel_bottom)
                  {
                    if(x>0)
                      {
                        victim[prev_pixel.x()][prev_pixel.y()].m_intersecions[prev_bound].push_back(L[k]);
                        victim[prev_pixel.x()][prev_pixel.y()].m_empty=false;
                      }
                    
                    if(x<bitmap_size()[coord])
                      {
                        victim[pixel.x()][pixel.y()].m_intersecions[bound].push_back(L[k]);
                        victim[pixel.x()][pixel.y()].m_empty=false;
                      }
                  }
              }
            
          }
      }
  }

                
  
  int
  OutlineData::
  compute_localized_affectors_LOD(int LOD,
                                  const boost::multi_array<analytic_return_type, 2> &dataLOD0,
                                  const ivec2 &LOD_bitmap_location,
                                  c_array<curve_segment> out_curves) const
  {
    int N(1<<LOD);
    grab_map hits_found;
    ivec2 bitmap_location(LOD_bitmap_location.x()<<LOD, LOD_bitmap_location.y()<<LOD);
    ivec2 texel_bottom_left(compute_texel_bottom_left(bitmap_location));
    ivec2 texel_top_right(compute_texel_top_right(bitmap_location+ivec2(N-1,N-1)));
    vec2 texel_bottom_leftf(texel_bottom_left.x(),
                            texel_bottom_left.y());

    /*
      We only care about the texel "of the boundary"
      of the NxN chunk [x,x+N]x[y,y+N] where
      N=1<<LOD, x=LOD_bitmap_location<<LOD, y=LOD_bitmap_location<<N
     */
    grab_simple_lines(hits_found,
                      dataLOD0,
                      std::min(bitmap_location.y(), bitmap_size().y()-1), 
                      range_type<int>(std::max(bitmap_location.x(), 0),
                                      std::min(bitmap_location.x()+N, bitmap_size().x())),
                      
                      WRATHUtil::y_fixed,
                      below_boundary);

    grab_simple_lines(hits_found,
                      dataLOD0,
                      std::min(bitmap_location.y()+N-1, bitmap_size().y()-1), 
                      range_type<int>(std::max(bitmap_location.x(), 0),
                                      std::min(bitmap_location.x()+N, bitmap_size().x())),
                      
                      WRATHUtil::y_fixed,
                      above_boundary);

    grab_simple_lines(hits_found,
                      dataLOD0,
                      std::min(bitmap_location.x(), bitmap_size().x()-1),
                      range_type<int>(std::max(bitmap_location.y(), 0),
                                      std::min(bitmap_location.y()+N, bitmap_size().y())),
                      
                      WRATHUtil::x_fixed,
                      left_boundary);

    grab_simple_lines(hits_found,
                      dataLOD0,
                      std::min(bitmap_location.x()+N-1, bitmap_size().x()-1),
                      range_type<int>(std::max(bitmap_location.y(), 0),
                                      std::min(bitmap_location.y()+N, bitmap_size().y())),
                      
                      WRATHUtil::x_fixed,
                      right_boundary);

    return compute_localized_affectors_worker(hits_found,
                                              texel_bottom_left,
                                              texel_top_right,
                                              out_curves);
  }


  int
  OutlineData::
  compute_localized_affectors_worker(const grab_map &hits_found,
                                     const ivec2 &texel_bottom_left,
                                     const ivec2 &texel_top_right,
                                     c_array<curve_segment> out_curves) const
  {

    //now that we have the hits, we now need to
    //create the localized curves. For now,
    //we just make everythign a line segment.
    //For quadratic (and higher) curves we just use the points
    //where the curve enters and leaves the texel.
    int return_value(0), max_return_value(out_curves.size());
    vec2 texel_bottom_leftf(texel_bottom_left.x(),
                            texel_bottom_left.y());

    for(grab_map::const_iterator iter=hits_found.begin(), end=hits_found.end(); 
        iter!=end and return_value<max_return_value; ++iter, ++return_value)
      {
        const BezierCurve *curve(iter->first);
        float min_t(100.0f), max_t(-100.0f);
        int found(0);
        enum boundary_type min_t_boundary(no_boundary);
        enum boundary_type max_t_boundary(no_boundary);
        
        for(std::list<grab_entry>::const_iterator liter=iter->second.begin(),
              lend=iter->second.end(); liter!=lend; ++liter)
          {
            const simple_line &L(*liter->second);
            
            ++found;
            if(min_t>L.m_source.m_value)
              {
                min_t=L.m_source.m_value;
                min_t_boundary=liter->first;
              }
            
            if(max_t<L.m_source.m_value)
              {
                max_t=L.m_source.m_value;         
                max_t_boundary=liter->first;
              }  
          }
        
        WRATHassert(found>0);
        
        if(found<2)
          {
            //then time t=1 or t=0 is within the texel:
            if( curve->pt0().x()>=texel_bottom_left.x()
                and curve->pt0().y()>=texel_bottom_left.y()
                and curve->pt0().x()<=texel_top_right.x()
                and curve->pt0().y()<=texel_top_right.y())            
              {
                min_t=0.0f;
                min_t_boundary=no_boundary;
              }
            else
              {
                max_t=1.0f;
                max_t_boundary=no_boundary;
              }
          }
      

        //within the loop we only set the times, later we will set the coordinates.
        out_curves[return_value].m_points.clear();
        out_curves[return_value].m_points.push_back(min_t);

                
        
        //insert additional points on the curve too
        //when the curve is NOT a line:
        for(int K=1;K<curve->degree();++K)
          {
            float t;
            vec2 pt, ptN;

            t=min_t + (max_t-min_t)*static_cast<float>(K)/static_cast<float>(curve->degree());
            out_curves[return_value].m_points.push_back(t);
          }

        out_curves[return_value].m_points.push_back(max_t);

        out_curves[return_value].m_enter=min_t_boundary;
        out_curves[return_value].m_exit=max_t_boundary;
        out_curves[return_value].m_curve=curve;
        
      }

    for(int c=0;c<return_value;++c)
      {
        for(std::vector<per_point_data>::iterator 
              iter=out_curves[c].m_points.begin(),
              end=out_curves[c].m_points.end();
            iter!=end; ++iter)
          {
            per_point_data &ctrl_pt(*iter);
            vec2 raw_p;

            raw_p=out_curves[c].m_curve->compute_pt_at_t(ctrl_pt.m_time);
                
            ctrl_pt.m_glyph_normalized_coordinate=(raw_p - glyph_bottom_left())/glyph_size();
            ctrl_pt.m_texel_normalized_coordinate=(raw_p - texel_bottom_leftf)/texel_size_f();
            
            //get the value in pixels of this LOD without rounding off.           
            ctrl_pt.m_bitmap_coordinate=bitmap_from_point(raw_p);
          }
      }

   
    return return_value;
  }
    
  
  
  int
  OutlineData::
  compute_localized_affectors(const analytic_return_type &R,
                              const ivec2 &bitmap_location,
                              c_array<curve_segment> out_curves) const
  {
    grab_map hits_found;
    ivec2 texel_bottom_left(compute_texel_bottom_left(bitmap_location));
    ivec2 texel_top_right(compute_texel_top_right(bitmap_location));

    for(int i=0;i<4;++i)
      {
        for(int j=0, end_j=R.m_intersecions[i].size(); j<end_j; ++j)
          {
            const simple_line &L(R.m_intersecions[i][j]);
            enum boundary_type enum_j;
            
            enum_j=static_cast<enum boundary_type>(j);
            hits_found[L.m_source.m_bezier].push_back(grab_entry(enum_j, &L));
          }
      }

    return compute_localized_affectors_worker(hits_found,
                                              texel_bottom_left,
                                              texel_top_right,
                                              out_curves);
 
  }

  void
  OutlineData::
  print_analytic_generation_data(std::ostream &str,
                                 const boost::multi_array<analytic_return_type, 2> &analytic_data) const
  {
    const char* coord_names[]=
      {
        "left_boundary",
        "right_boundary",
        "below_boundary",       
        "above_boundary",
      };

    str << "\nCurveList:";
    for(std::vector<range_type<int> >::const_iterator iter=components().begin(),
          end=components().end(); iter!=end; ++iter)
      {
        for(int curve_index=iter->m_begin; curve_index!=iter->m_end; ++curve_index)
          {
            str << "\n\t\t(" << bezier_curve(curve_index)
                << ")";
            bezier_curve(curve_index)->print_info(str);
          }

        str << "\n";
      }


    str << "\n\tAnalyic Data:";
    for(int y=0;y<bitmap_size().y();++y)
      {
        for(int x=0;x<bitmap_size().x(); ++x)
          {
            std::set<const BezierCurve*> curves;
            int cnt(0);
            
            curves.clear();         
            for(int coord=0;coord<4;++coord)
              {
                cnt+=analytic_data[x][y].m_intersecions[coord].size();
                for(std::vector<simple_line>::const_iterator 
                      iter=analytic_data[x][y].m_intersecions[coord].begin(), 
                      end=analytic_data[x][y].m_intersecions[coord].end();
                    iter!=end; ++iter)
                  {
                    curves.insert( iter->m_source.m_bezier);
                  }
              }
            
            if(curves.empty())
              continue;
            
            str << "\n\n\t\t" << ivec2(x,y) 
                << "=" 
                << ivec2(point_from_bitmap_x(x)-half_texel_size(), 
                         point_from_bitmap_y(y)-half_texel_size())
                << "--"
                << ivec2(point_from_bitmap_x(x)+half_texel_size(), 
                                    point_from_bitmap_y(y)+half_texel_size()); 
            
            str << " has " << cnt;

            
            if(cnt>=3)
              {
                str << "(EMG)";
              }

             str << " hits total from "
                 << curves.size();

             if(curves.size()>=3)
               {
                 str << "(OMG)";
               }
             str << " curves";
            
            for(int coord=0;coord<4;++coord)
              {
                str << "\n\t\t\t" << coord_names[coord]
                    << "\n\t\t\t\tParity=" << analytic_data[x][y].m_parity_count[coord]
                    << "\n\t\t\t\t" << analytic_data[x][y].m_intersecions[coord].size()
                    << " texel Intersections:";
                for(std::vector<simple_line>::const_iterator 
                      iter=analytic_data[x][y].m_intersecions[coord].begin(), 
                      end=analytic_data[x][y].m_intersecions[coord].end();
                    iter!=end; ++iter)
                  {
                    str << "\n\t\t\t\t\tCurve(" << iter->m_source.m_bezier << "):";
                    iter->m_source.m_bezier->print_info(str);
                    
                    str << "\n\t\t\t\t\tIndex=" 
                        << iter->m_index_of_intersection
                        << " @t=" << iter->m_source.m_value
                        << " pt= " << iter->m_value
                        << ":" 
                        << iter->m_source.m_bezier->compute_pt_at_t(iter->m_source.m_value);
                  }
                
                
              }
          }
      }
  }
    
     
  ///////////////////////////////////////
  // ScanLineDistanceRenderer methods
  void
  ScanLineDistanceRenderer::
  ft_render_call_back(const int y,
                      const int count,
                      const FT_Span * const spans,
                      void * const user) 
  {
    ScanLineDistanceRenderer *ptr;
    ptr=reinterpret_cast<ScanLineDistanceRenderer*>(user);
    for(int i=0; i<count; ++i)
      {
        ptr->m_spans.push_back( Span(spans[i],y) );
      } 
  }
  
  
  int
  ScanLineDistanceRenderer::
  bitmap_location_y(int iny) const
  {
    iny-=m_half_texel_size;
    iny-=m_image_offset.y();
    return iny/m_scaling_factor;
  }

  int
  ScanLineDistanceRenderer::
  bitmap_location_x(int inx) const
  {
    inx-=m_half_texel_size;
    inx-=m_image_offset.x();
    return inx/m_scaling_factor;
  }

  int
  ScanLineDistanceRenderer::
  point_location_y(int bitmap_iny) const
  {
    //bitmap_iny*=m_scaling_factor;
    bitmap_iny<<=m_power2_render_inflate;
    return bitmap_iny+m_image_offset.y()+m_half_texel_size;
  }

  int
  ScanLineDistanceRenderer::
  point_location_x(int bitmap_inx) const
  {
    //bitmap_inx*=m_scaling_factor;
    bitmap_inx<<=m_power2_render_inflate;
    return bitmap_inx+m_image_offset.x()+m_half_texel_size;
  }
  
  bool
  ScanLineDistanceRenderer::
  is_texel_center_y(int iny) const
  {
    iny-=m_image_offset.y();
    iny+=m_scaling_factor/2;
    return (iny%m_scaling_factor)==0;
  }
  
  bool
  ScanLineDistanceRenderer::
  is_texel_center_x(int inx) const
  {
    inx-=m_image_offset.x();
    inx+=m_scaling_factor/2;
    return (inx%m_scaling_factor)==0;
  }

  
  void
  ScanLineDistanceRenderer::
  generate_spans(FT_Face pface, int char_index)
  {
    FT_Matrix ft_matrix;
    
    ft_matrix.xy=ft_matrix.yx=0;
    ft_matrix.xx=ft_matrix.yy=(1<<(16+m_power2_render_inflate));
    
    //set transformation and load the glyph:
    FT_Set_Transform(pface, &ft_matrix, NULL);
    FT_Load_Glyph(pface, char_index, FT_LOAD_DEFAULT);
    
    //use FT's callback renering:
    FT_Raster_Params ftparams;
    memset(&ftparams, 0, sizeof(ftparams));

    ftparams.flags=FT_RASTER_FLAG_AA| FT_RASTER_FLAG_DIRECT;
    ftparams.gray_spans=ft_render_call_back;
    ftparams.user=this;
    
    FT_Outline_Render(pface->glyph->library,
                      &pface->glyph->outline, 
                      &ftparams);
    
    FT_Set_Transform(pface, NULL, NULL);
  }

  //pass1: mark texels as inside for those within spans
  //that are "covered"
  int
  ScanLineDistanceRenderer::
  do_pass1(void)
  {
    int pixels_affected(0);

    for(std::vector<Span>::const_iterator iter=m_spans.begin(),
          end=m_spans.end(); iter!=end; ++iter)
      {
        const Span &sp(*iter);

        if(sp.m_coverage and is_texel_center_y(sp.m_y))
          {
            int begin_x, end_x, y;

            begin_x=bitmap_location_x(sp.m_x_begin)-1;
            end_x=bitmap_location_x(sp.m_x_end)+1;
            y=bitmap_location_y(sp.m_y);

            begin_x=std::max(0, std::min(begin_x, m_bitmap_sz.x()));
            end_x=std::max(0, std::min(end_x, m_bitmap_sz.x()));

            //TODO:
            // change begin_x and end_x so that we can skip
            // the test if(px>=sp.m_x_begin and px<sp.m_x_end)
            // within the loop

            if(y<m_bitmap_sz.y() and y>=0)
              {
                for(int x=begin_x; x<end_x; ++x, ++pixels_affected)
                  {
                    int px;

                    px=point_location_x(x);
                    if(px>=sp.m_x_begin and px<sp.m_x_end)
                      {
                        m_pixel_data[x][y].m_covered=true;
                      }
                  }
              }
          }
      }

    return pixels_affected;
    
  }

  //pass2:
  // iterator over the spans, between spans that have
  // pixels between them, insert spans that
  // have "no coverage". 
  void
  ScanLineDistanceRenderer::
  do_pass2(int pixel_radius)
  {
    if(m_spans.empty())
      {
        return;
      }

    //sort the spans first by y-coordinate then by m_x_begin:
    std::sort(m_spans.begin(), m_spans.end());

    int current_y;
    int beg_x(m_image_offset.x()), end_x(m_image_offset.x()+m_bitmap_sz.x()*m_scaling_factor), last_x;

   

    last_x=beg_x;
    current_y=m_image_offset.y();

    for(std::vector<Span>::const_iterator iter=m_spans.begin(), 
          end=m_spans.end(); iter!=end; ++iter)
      {
        const Span &sp(*iter);

        WRATHassert(current_y<=sp.m_y);

        if(current_y!=sp.m_y)
          {
            handle_span(pixel_radius, Span(current_y, false, last_x, end_x));
            ++current_y;

            for(;current_y<sp.m_y;++current_y)
              {
                handle_span(pixel_radius, Span(current_y, false, beg_x, end_x));
              }
            last_x=beg_x;
          }

        WRATHassert(current_y==sp.m_y);
        WRATHassert(last_x<=sp.m_x_begin);
        if(last_x<sp.m_x_begin)
          {
            handle_span(pixel_radius, Span(current_y, false, last_x, sp.m_x_begin));
          }

        handle_span(pixel_radius, sp);
        last_x=sp.m_x_end;
      }

    
    handle_span(pixel_radius, Span(current_y, false, last_x, end_x));
    handle_span(pixel_radius, Span(current_y+1, false, beg_x, end_x));
  }

  void
  ScanLineDistanceRenderer::
  handle_span(int bitmap_radius, const Span &sp)
  {
    int by, begx, endx, begy, endy;

    begx=bitmap_location_x(sp.m_x_begin);
    endx=bitmap_location_x(sp.m_x_end);
    by=bitmap_location_y(sp.m_y);
    
    begx=std::max(0, begx-bitmap_radius);
    endx=std::min(endx+bitmap_radius, m_bitmap_sz.x());

    begy=std::max(0, by-bitmap_radius);
    endy=std::min(by+bitmap_radius+1, m_bitmap_sz.y());

    for(int y=begy;y<endy;++y)
      {
        int py;

        py=point_location_y(y);
        for(int x=begx; x<endx; ++x)
          {
            int px;
            
            px=point_location_x(x);
            if(sp.m_coverage!=m_pixel_data[x][y].m_covered)
              {
                int dist;

                dist=abs(py - sp.m_y);
                if(px<sp.m_x_begin)
                  {
                    dist+= std::abs(sp.m_x_begin - px);
                  }
                else if(px>=sp.m_x_end)
                  {
                    dist+= std::abs(px-sp.m_x_end)+1;
                  }

                m_pixel_data[x][y].m_distance=std::min(m_pixel_data[x][y].m_distance, dist);
                
              }
          }
      }
  }



  LockableFace::handle
  load_face(const WRATHFontDatabase::Font::const_handle &fnt)
  {
    FT_Face face(NULL);
    FT_Library lib(NULL);
    int load_lib_error_code, load_font_error_code;

    if(!fnt.valid())
      {
        return LockableFace::handle();
      }

    load_lib_error_code=FT_Init_FreeType(&lib);
    if(load_lib_error_code!=0)
      {
        return LockableFace::handle();
      }
  
    if(!fnt->memory_source().valid())
      {
        load_font_error_code=FT_New_Face(lib,
                                         fnt->name().c_str(),
                                         fnt->face_index(),
                                         &face);
      }
    else
      {
        load_font_error_code=FT_New_Memory_Face(lib,
                                                fnt->memory_source()->data().c_ptr(),
                                                fnt->memory_source()->data().size(),
                                                fnt->face_index(),
                                                &face);
      }
          

    if(load_font_error_code!=0 or face==NULL or (face->face_flags&FT_FACE_FLAG_SCALABLE)==0) 
      {
        if(face!=NULL)
          {
            FT_Done_Face(face);
          }
        FT_Done_FreeType(lib);
        return LockableFace::handle();
      }
    
    return WRATHNew face_with_private_library(face, lib);
  }


 



}
