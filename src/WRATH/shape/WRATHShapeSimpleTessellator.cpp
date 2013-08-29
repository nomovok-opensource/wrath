/*! 
 * \file WRATHShapeSimpleTessellator.cpp
 * \brief file WRATHShapeSimpleTessellator.cpp
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
#include <complex>
#include "WRATHShapeSimpleTessellator.hpp"
#include "WRATHPolynomial.hpp"

namespace
{        
  

  class analytic_point_data_with_time:
    public WRATHShapeSimpleTessellatorPrivateImplement::analytic_point_data
  {
  public:
    analytic_point_data_with_time(const WRATHShapeSimpleTessellatorPrivateImplement::analytic_point_data &obj,
                                  float t):
      WRATHShapeSimpleTessellatorPrivateImplement::analytic_point_data(obj),
      m_time(t)
    {
      compute_curvature();
    }

    
    analytic_point_data_with_time(const WRATHShapeSimpleTessellatorPrivateImplement::interpolator_base *edge, 
                                  float t):
      m_time(t)
    {
      edge->compute(m_time, *this);
      compute_curvature();
    }

    

    analytic_point_data_with_time(void)
    {}

    bool
    operator<(const analytic_point_data_with_time &rhs) const
    {
      return m_time<rhs.m_time;
    }

    float
    time(void) const
    {
      return m_time;
    }

    float
    K_times_speed(void) const
    {
      return m_K_times_speed;
    }

  private:

    void
    compute_curvature(void)
    {
      float preK, speed_sq;
      const float epsilonsq(0.0000001f*0.0000001f);

      /*
        K= || p_t X p_tt || / ||p_t||^3

        thus

        K*||p_t|| = || p_t X p_tt || / ||p_t||^2
      */

      preK= m_p_t.x()*m_p_tt.y() - m_p_tt.x()*m_p_t.y();

      
      speed_sq=std::max(epsilonsq,
                        dot(m_p_t, m_p_t));

      m_K_times_speed=std::abs(preK)/speed_sq;
    }

    float m_K_times_speed, m_time;
  };

  bool
  needs_to_recurse(float delta_t,
                   const analytic_point_data_with_time &st,
                   const analytic_point_data_with_time &mid,
                   const analytic_point_data_with_time &ed,
                   float curc_tthresh)
  {
    float K0, K1, K;

    K0=st.K_times_speed()*delta_t;
    K1=ed.K_times_speed()*delta_t;
    K=mid.K_times_speed()*delta_t;

    return K0+K1+2.0f*K > 4.0f*curc_tthresh;
  }

  
  void
  do_tessellation_worker(std::vector<analytic_point_data_with_time> &output_pts,
                         unsigned int index_of_start, unsigned int index_of_end,
                         const WRATHShapeSimpleTessellatorPrivateImplement::interpolator_base *edge, 
                         float curc_tthresh, int max_recurse)
  {
    if(max_recurse<=0)
      {
        return;
      }


    unsigned int mid_pt_index;
    float delta_t;
    float start_t(output_pts[index_of_start].time());
    float end_t(output_pts[index_of_end].time());

    analytic_point_data_with_time mid_pt(edge, (start_t + end_t)*0.5f);

    delta_t=(end_t - start_t)*0.5f;
    WRATHassert(delta_t>=0.0f);

    mid_pt_index=output_pts.size();
    output_pts.push_back(mid_pt);

    /*
      get the reference to the points _after_ the push back
      incase the std::vector has to move it's contents.
     */
    const analytic_point_data_with_time &st(output_pts[index_of_start]);
    const analytic_point_data_with_time &ed(output_pts[index_of_end]);

    
    if(needs_to_recurse(delta_t, st, mid_pt, ed, curc_tthresh))
      {
        do_tessellation_worker(output_pts,
                               index_of_start, mid_pt_index,
                               edge, curc_tthresh, max_recurse-1);

        do_tessellation_worker(output_pts,
                               mid_pt_index, index_of_end,
                               edge, curc_tthresh, max_recurse-1);
      }
    

  }


  void
  do_tessellation(int max_recurse,
                  std::vector<analytic_point_data_with_time> &output_pts,
                  const WRATHShapeSimpleTessellatorPrivateImplement::interpolator_base *edge, 
                  float curc_tthresh)
  {

    output_pts.push_back( analytic_point_data_with_time(edge->start_pt(), 0.0f));
    output_pts.push_back( analytic_point_data_with_time(edge->end_pt(), 1.0f));

    if(!edge->is_flat())
      {
        do_tessellation_worker(output_pts, 0, 1, edge, curc_tthresh, max_recurse);
        std::sort(output_pts.begin(), output_pts.end());
      }
  }

  

  template<typename iterator>
  void
  compute_bounding_box_stuff(iterator begin, iterator end, WRATHBBox<2> &v)
  {
    for(;begin!=end;++begin)
      {
        v.set_or((*begin)->bounding_box());
      }
  }
  
}

////////////////////////////////
//TessellatedEdge methods
void
WRATHShapeSimpleTessellatorPayload::TessellatedEdge::
compute_bounding_box(void) 
{
  for(std::vector<CurvePoint>::const_iterator iter=m_curve_points.begin(),
        end=m_curve_points.end(); iter!=end; ++iter)
    {
      m_box.set_or(iter->position());
    }
}


///////////////////////////
//TessellatedOutline methods
void
WRATHShapeSimpleTessellatorPayload::TessellatedOutline::
compute_bounding_box(void) 
{
  compute_bounding_box_stuff(m_edges.begin(), m_edges.end(), m_box);
}


/////////////////////////////////////////
// WRATHShapeSimpleTessellatorPrivateImplement::bezier_interpolator methods
void
WRATHShapeSimpleTessellatorPrivateImplement::bezier_interpolator::
init(void)
{
  WRATHUtil::generate_polynomial_from_bezier(const_c_array<vec2>(m_points), 
                                             m_polynomial[0]);
  
  std::reverse(m_points.begin(), m_points.end());
  WRATHUtil::generate_polynomial_from_bezier(const_c_array<vec2>(m_points), 
                                             m_reverse_polynomial[0]);


  WRATHassert(m_polynomial[0].size()==m_reverse_polynomial[0].size());
  for(int i=1;i<3;++i)
    {
      if(m_polynomial[i-1].size()>1)
        {
          m_polynomial[i].resize(m_polynomial[i-1].size()-1);
          m_reverse_polynomial[i].resize(m_reverse_polynomial[i-1].size()-1);
          for(int N=1, endN=m_polynomial[i-1].size(); N!=endN; ++N)
            {
              m_polynomial[i][N-1]=m_polynomial[i-1][N]*static_cast<float>(N);
              m_reverse_polynomial[i][N-1]=m_reverse_polynomial[i-1][N]*static_cast<float>(N);
            }
        }
    }


  compute(0.0f, m_start_pt);
  compute(1.0f, m_end_pt);

  /*
    there is the potential for round off error,
    as such we use the point positions as indicated
    by m_points.
   */
  //m_start_pt.m_p=m_points.front();
  //m_end_pt.m_p=m_points.back();

  m_is_flat=false;
}

void
WRATHShapeSimpleTessellatorPrivateImplement::bezier_interpolator::
compute(float t, analytic_point_data &output) const
{
  vecN<vec2, 3> return_values( vec2(0.0f, 0.0f), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f));
  const vecN<std::vector<vec2>, 3> *poly_to_use_ptr(&m_polynomial);
  float deriv_multiplies(1.0f);

  if(t>0.5f)
    {
      t=1.0-t;
      poly_to_use_ptr=&m_reverse_polynomial;
      deriv_multiplies=-1.0f;
    }

  const vecN<std::vector<vec2>, 3> &poly_to_use(*poly_to_use_ptr);
  unsigned int I(0);
  float t_power(1.0f);

  /*
    I need psychiatric help. If you unwind this loop
    you will find that 
    return_value[Z]=poly_to_use[Z][0] + t*poly_to_use[Z][1] + t*t*poly_to_use[Z][2] + ... 
   */
  for(int P=2; P>=0; --P)
    {
      for(;I<poly_to_use[P].size(); ++I, t_power*=t)
        {
          for(int Z=0; Z<=P; ++Z)
            {
              return_values[Z]+= poly_to_use[Z][I]*t_power;
            }
        }
    }



  output.m_p=return_values[0];
  output.m_p_t=deriv_multiplies*return_values[1];
  output.m_p_tt=return_values[2];
}

////////////////////////////////////////////////////
// WRATHShapeSimpleTessellatorPrivateImplement::arc_interpolator methods
void
WRATHShapeSimpleTessellatorPrivateImplement::arc_interpolator::
init(float angle,
     bool ccw,
     const vec2 &st, 
     const vec2 &ed)
{  
  float negate_dir;
  vec2 delta, v(ed-st);
  vec2 n(-v.y(), v.x());
  float coeff, s, c;

  negate_dir=(ccw)?
    1.0f:-1.0f;
  
  angle=std::max(angle, 0.0001f);
  sincosf(angle*0.5f, &s, &c);

  coeff=negate_dir*0.5f*c/s;
  delta=n*coeff;
  
  m_center= (st+ed)*0.5f + delta;

  vec2 to_st(st - m_center);

  m_radius=to_st.magnitude();

  m_angle0=atan2f(to_st.y(), to_st.x());
  m_angle_speed=negate_dir*angle;
  m_angle_speed_sq=angle*angle;

  compute(0.0f, m_start_pt);
  compute(1.0f, m_end_pt);
  
  m_start_pt.m_p=st;
  m_end_pt.m_p=ed;
  m_is_flat=false;
}

void
WRATHShapeSimpleTessellatorPrivateImplement::arc_interpolator::
compute(float t, analytic_point_data &output) const
{
  float s, c;

  sincosf(m_angle0+t*m_angle_speed, &s, &c);

  s*=m_radius;
  c*=m_radius;
  output.m_p=m_center + vec2(c, s);
  output.m_p_t=vec2(-m_angle_speed*s, m_angle_speed*c);
  output.m_p_tt=vec2(-m_angle_speed_sq*c, -m_angle_speed_sq*s);


}


/////////////////////////////////////////
// WRATHShapeSimpleTessellatorPrivateImplement::geometry_computer methods
WRATHShapeSimpleTessellatorPrivateImplement::geometry_computer::
~geometry_computer()
{
  for(std::list<outline_type>::iterator iter=m_input_outline_data.begin(),
        end=m_input_outline_data.end(); iter!=end; ++iter)
    {
      for(outline_type::iterator i=iter->begin(), 
            e=iter->end(); i!=e; ++i)
        {
          WRATHDelete(*i);
        }          
    }
}

void
WRATHShapeSimpleTessellatorPrivateImplement::geometry_computer::
compute_implement(const WRATHShapeSimpleTessellatorPayload::PayloadParams &params)
{
  std::list<outline_type>::const_iterator iter, end;
  int outlineID;

  for(outlineID=0, iter=m_input_outline_data.begin(),
        end=m_input_outline_data.end(); iter!=end; ++iter)
    {
      if(!iter->empty())
        {
          m_tessellation.push_back(create_outline(outlineID, *iter, params));
          m_box.set_or(m_tessellation.back()->bounding_box());
        }
    }
}

WRATHShapeSimpleTessellatorPayload::TessellatedOutline::handle
WRATHShapeSimpleTessellatorPrivateImplement::geometry_computer::
create_outline(int outlineID,
               const outline_type &outline,
               const WRATHShapeSimpleTessellatorPayload::PayloadParams &params)
{
  WRATHShapeSimpleTessellatorPayload::TessellatedOutline::handle R;
  std::vector<WRATHShapeSimpleTessellatorPayload::TessellatedEdge::handle> edges;
  
  for(unsigned int i=1, endi=outline.size(); i!=endi; ++i)
    {
      //handle edge connecting pt[i-1] to pt[i]:
      edges.push_back(create_edge(outline[i-1], i-1, i, params, outlineID));
    }

  
  //handle the edge from the last point to the first
  edges.push_back(create_edge(outline.back(), 
                              outline.size()-1, 0,
                              params, outlineID));

  R=WRATHNew WRATHShapeSimpleTessellatorPayload::TessellatedOutline(outlineID,
                                                                    edges);

  return R;
}


WRATHShapeSimpleTessellatorPayload::TessellatedEdge::handle
WRATHShapeSimpleTessellatorPrivateImplement::geometry_computer::
create_edge(const interpolator_base *edge,
            int pointID, int nextPointID,
            const WRATHShapeSimpleTessellatorPayload::PayloadParams &params,
            int outlineID)
{
     
  /*
    We need to.. intelligently tessellate.
    We let another routine do that ugly work
    for us. Note that do_tessellation
    only adds the start end point of edge
    if edge->is_flat() returns true.
  */
  std::vector<analytic_point_data_with_time> tess_pts;
  
  do_tessellation(params.m_max_recurse, 
                  tess_pts, edge, 
                  params.curve_tessellation_threshhold());
  
  /*
    Now that we've got the points,
    lets just do to town and make the triangles
    The do_tessellation routine sorted the points 
    by time for us. 
  */
  int num_pts(tess_pts.size());
  GLushort prev(0);
  std::vector<WRATHShapeSimpleTessellatorPayload::CurvePoint> points;
  std::vector<GLushort> indices;
  
  for(int i=0;i<num_pts; ++i)
    {
      vec2 n;
      
      n=vec2(-tess_pts[i].m_p_t.y(), tess_pts[i].m_p_t.x());
      n.normalize();
      if(i!=0)
        {
          /*
            create a line segment from the previous point
            on the curve to the current point on the curve.
          */
          indices.push_back(prev);
          indices.push_back(prev+1);
        }
      prev=points.size();
      points.push_back(WRATHShapeSimpleTessellatorPayload::CurvePoint(tess_pts[i].m_p, n, 
                                                                      tess_pts[i].time()));
    } 

  return WRATHNew WRATHShapeSimpleTessellatorPayload::TessellatedEdge(pointID, 
                                                                      nextPointID,
                                                                      points, indices,
                                                                      outlineID);
}
    



