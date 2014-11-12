/*! 
 * \file WRATHShapePreStroker.cpp
 * \brief file WRATHShapePreStroker.cpp
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
#include <stdint.h>
#include "WRATHShapePreStroker.hpp"

namespace
{
  typedef WRATHShapeSimpleTessellatorPayload::TessellatedOutline Outline;
  typedef WRATHShapeSimpleTessellatorPayload::TessellatedEdge Edge;
  typedef WRATHShapeSimpleTessellatorPayload::CurvePoint CurvePoint;
  typedef WRATHShapePreStrokerPayload::JoinPoint JoinPoint;
  typedef WRATHShapePreStrokerPayload::MiterJoinPoint MiterJoinPoint;
  typedef WRATHShapePreStrokerPayload::CapPoint CapPoint;

  class common_cap_data
  {
  public:
    common_cap_data(const Outline::handle &O,
                    const CurvePoint &pt,
                    bool pis_starting_cap);

    void
    do_rounded_cap(float curve_thresh,
                   std::vector<CapPoint> &pts,
                   std::vector<GLushort> &inds);

    void
    do_square_cap(std::vector<CapPoint> &pts,
                  std::vector<GLushort> &inds);

  private:
    bool is_starting_cap;
    float lambda;
    int outlineID;
    vec2 p, v, n;
  };

  class common_join_data
  {
  public:
    
    common_join_data(const Outline::handle &O,
                     const Edge::handle &pre,
                     const Edge::handle &post);

    void
    do_bevel_join(std::vector<JoinPoint> &pts,
                  std::vector<GLushort> &inds);

    void
    do_miter_join(std::vector<MiterJoinPoint> &pts,
                  std::vector<GLushort> &inds);

    void
    do_rounded_join(float curve_thresh,
                    std::vector<JoinPoint> &pts,
                    std::vector<GLushort> &inds);

  private:

    float det, lambda;
    int outlineID, pre_edge_pointID, post_edge_pointID;
    vec2 p0, p1, n0, n1, v0, v1, avg_p;
  };
}

//////////////////////////////////////
//common_cap_data methods
common_cap_data::
common_cap_data(const Outline::handle &O,
                const CurvePoint &pt,
                bool pis_starting_cap):
  is_starting_cap(pis_starting_cap),
  lambda(is_starting_cap?-1.0f:1.0f),
  outlineID(O->outlineID()),
  p(pt.position()),
  v(lambda*pt.direction()),
  n(-v.y(), v.x())
{
}

void
common_cap_data::
do_rounded_cap(float curve_thresh,
               std::vector<CapPoint> &pts,
               std::vector<GLushort> &inds)
{
  int i, num_pts;
  float theta, delta_theta;
  GLushort first(pts.size());

  CapPoint center(outlineID, is_starting_cap, p, vec2(0.0f, 0.0f) );
  pts.push_back(center);

  num_pts=static_cast<int>(float(M_PI)/curve_thresh);
  num_pts=std::max(num_pts, 3);
  delta_theta=float(M_PI)/static_cast<float>(num_pts-1);

  CapPoint st(outlineID, is_starting_cap, p, n);
  pts.push_back(st);

  for(i=1, theta=delta_theta; i<num_pts-1; ++i, theta+=delta_theta)
    {
      vec2 q;
      float s, c;

      s=sinf(theta);
      c=cosf(theta);
      //      sincosf(theta, &s, &c);

      CapPoint qq(outlineID, is_starting_cap, p, v*s+n*c);
      pts.push_back(qq);
    }

  CapPoint ed(outlineID, is_starting_cap, p, -n);
  pts.push_back(ed);

  for(int i=1; i<num_pts; ++i)
    {
      inds.push_back(first);
      inds.push_back(first+i);
      inds.push_back(first+i+1);
    }
}

void
common_cap_data::
do_square_cap(std::vector<CapPoint> &pts,
              std::vector<GLushort> &indices)
{
  vecN<vec2, 4> ps_offsets;

  ps_offsets[0]=n;
  ps_offsets[1]=ps_offsets[0] + v*0.5f;
  ps_offsets[2]=ps_offsets[1] - 2.0f*n;
  ps_offsets[3]= -ps_offsets[0];

  GLushort first(pts.size());
  CapPoint center(outlineID, is_starting_cap, p, vec2(0.0f, 0.0f));

  pts.push_back(center);
  for(int i=0;i<4;++i)
    {
      CapPoint c(outlineID, is_starting_cap, p, ps_offsets[i]);
      pts.push_back(c);
    }

  const GLushort inds[]=
    {
      0,1,2,
      0,2,3,
      0,3,4
    };

  for(int i=0;i<9;++i)
    {
      indices.push_back(inds[i]+first);
    }
}


////////////////////////////
// common_join_data methods
common_join_data::
common_join_data(const Outline::handle &O,
                 const Edge::handle &pre,
                 const Edge::handle &post):
  lambda(1.0f),
  outlineID(O->outlineID()),
  pre_edge_pointID(pre->point_id()),
  post_edge_pointID(post->point_id())
{
   /*
     note that p0 should be the same as p1
     
     TODO: since we made the curve-reversal fix
     in BezierCurve, is the code to handle if
     p0 and p1 are not equal really needed?
   */
  p0=pre->curve_points().back().position();
  p1=post->curve_points().front().position();
  avg_p=0.5f*(p0+p1);

  v0=pre->curve_points().back().direction();
  v1=post->curve_points().front().direction();
      
  n0=pre->curve_points().back().normal();
  n1=post->curve_points().front().normal();
  
  det=dot(v1, n0);
  if(det>0.0f)
    {
      /*
        If I told you why lambda gets negated your head will
        explode. I like exploding heads, so here goes:
        
        we have two curves:
        a(t) with a(1)=p
        and
        b(t) with b(0)=p.
        
        Each curve is surrounded by two curves,
        for a(t) those are
        a0(t) = a(t) + w*n_a(t)
        a1(t) = a(t) - w*n_a(t)
        where n_a(t) is the normal vector at t from the curve a.
        That is computed as:
        
        n_a(t) = J( a'(t) ) / ||a'(t)||
        
        where J(x,y)=(-y,x). Really bad if a'(t) is zero.
        Lets hope a(t) is intelligently parameterized.
        
        Now things get interesting at the join.
        We want to draw just one freaking triangle,
        consisting of the point where the curve
        a(t) and b(t) meet (i.e. at p) and the points
        A and B where A is a0(1) or a1(1) and
        B is b0(0) or b1(1). Note that if we
        use a0(1) for A then we use b0(0) for B
        so all it comes down to is do we
        add or subtract the normal vector.
        The answer here is observed that the two
        edges meet at an angle, we want the
        side that makes the obliquie angle. 
        With some grunging the vector algebra 
        it comes down to weather or not dot(v0,n1)
        is negative or positive, Indeed:
        
        The first curve is coming _into_ p at velocity v0,
        the second curve is going away from p at velocity v1.
        
        let p0 be a point on the first curve before p:
        
        p0= p - s*v0
        
        and p1 be a point on the second curve after p:
        
        p1= p + t*v1
        
        with s and t both positive.
        Let q= (p0+p1)/2. The point q is
        guaranteed to be "on the side
        of the acute angle of p"
        
        With this in mind, if either of <q-p, n0> 
        and <q-p, n1> is  positive then we want 
        to add by -w*n rather than w*n:
        
        lets compute:
        <q-p, n1> = 0.5*<tv1-sv0, tv1 -sv0, J(v0) >
        = 0.5*t*<v1, J(v0)>
        
        and also
        <q-p, n0> = -0.5*s*<v0, J(v1)>
        =  0.5*s*< J(v0), v1 >
        =  0.5*s*< v1, J(v0) >
        
        (the 2nd to last line from that transpose(J)=-J)
        Notice that if one is positive so is the other.
        
        thus we need only compute the sign
        of <v1, J(v0)> and if it is positive
        we want to add by -w*n rather than w*n
        
        On a side note, another way to see this
        is to determine if the angle
        between v0 and -v1 is positive
        or negative, i.e. just compute
        the sign of the z-component
        of the cross product of (v0,0)
        with (-v1, 0). Notice that this
        too comes down to the sign of
        <v1, J(v0)> as well.
        
        Did your head explode?
      */
      lambda=-1.0f;
    }
}

void
common_join_data::
do_bevel_join(std::vector<WRATHShapePreStrokerPayload::JoinPoint> &pts,
              std::vector<GLushort> &inds)
{
  JoinPoint before(outlineID, 
                   pre_edge_pointID, post_edge_pointID,
                   p0, lambda*n0);
  
  JoinPoint after(outlineID, 
                  pre_edge_pointID, post_edge_pointID,
                  p1, lambda*n1);
  
  JoinPoint tip(outlineID, 
                pre_edge_pointID, post_edge_pointID,
                p0, vec2(0.0f,0.0f) );
  
  JoinPoint btip(outlineID, 
                 pre_edge_pointID, post_edge_pointID,
                 p1, vec2(0.0f,0.0f) );
  
  
  GLushort first(pts.size());
  
  pts.push_back(before);
  pts.push_back(after);
  pts.push_back(tip);
  pts.push_back(btip);
  inds.push_back(first);
  inds.push_back(first+1);
  inds.push_back(first+2);
  inds.push_back(first+1);
  inds.push_back(first+2);
  inds.push_back(first+3);
}

void
common_join_data::
do_miter_join(std::vector<WRATHShapePreStrokerPayload::MiterJoinPoint> &pts,
              std::vector<GLushort> &inds)
{
  float a, rhs, lhs;
  float sb(det>0.0f?-1.0f:1.0f);
  vec2 q_offset0, q_offset1;

  /*
    now compute q.
    q is at the point where the lines
    l(t)= p0 + w*n0 + t*v0,
    m(t)= p1 + w*n1 - t*v1
    intersect.
    if that point q is too far from p,
    then we fake it!
    
    the development for computing q is:
    
    l(t)=m(s)
    
    p + sbw*n0 + t*v0 = p + sbw*n1 - s*v1 = q
    
    which becomes: recall that n0=J(v0) and n1=J(v1)
    
    M*(t,s) = sbw*n1 - sbw*n0
    where M is 
    | |   |  |
    | v0  v1 |
    | |   |  |
    
    (i.e. first column is v0 and second column is v1).
    
    then the above becomes:
    
    M*A = sbw*(n1-n0), where A=(t,s)
    
    now det(M) = v0.x*v1.y - v1.x*v0.y
    = (-v0.y)*(v1.x) + (v0.x)*(v1.y)
    = <n0, v1>
    
    and adj(M) is
    |  -- -n1 -- |
    |  -- +n0 -- |
    
    thus
    
    t= sbw*< -n1/detM, -n1 - n0>
    = sbw/detM ( -<n1,n1> + <n1,n0> ) 
    = sbw/<n0,v1> * ( <n1,n0> -1 )
    
    
    hence 
    q= p +sbw*n0 +  sbw*v0*(<n1,n0>-1)/<v1,n0>)
    
    and ||p-q-sbw*n0|| = w*abs(<n1,n0>-1)/abs(<v1,n0>)
    
    we require that is no more that m*w, i.e.
    
    i.e.
    
    abs(<n1,n0>-1) <= m*abs(<v1,n0>)

    and the miter offset point is given by:

    sb*(n0 + v0*abs(<n1,n0>-1)/abs(<v1,n0>))

    MiterJoinPoint stores the numbers:
     lhs=<n1,n0>-1
     rhs=<v1,n0>
     n=sb*n0
     v=sb*v0

    the offset vector is n + v*lhs/rhs when abs(lhs/rhs) < miter_limit
    and n+v*miter_limit*sign(lhs*rhs) otherwise

    Question is this better or worse than using 
    (normalized v1-v0) for when the miter limit
    is exceeeded? 
  */
  
  a=dot(n0,n1);
  lhs=a-1;
  rhs=det;


  MiterJoinPoint pp(outlineID, 
                    pre_edge_pointID, post_edge_pointID,
                    avg_p, vec2(0.0f, 0.0f));
  
  
  MiterJoinPoint pp0(outlineID, 
                     pre_edge_pointID, post_edge_pointID,
                     p0, vec2(0.0f, 0.0f));
  
  
  MiterJoinPoint qq0(outlineID, 
                     pre_edge_pointID, post_edge_pointID,
                     p0, sb*n0);

  
  MiterJoinPoint qq(outlineID, 
                    pre_edge_pointID, post_edge_pointID,
                    avg_p, sb*v0, sb*n0, lhs, rhs);
  
  MiterJoinPoint qq1(outlineID, 
                     pre_edge_pointID, post_edge_pointID,
                     p1, sb*n1);
  
  
  MiterJoinPoint pp1(outlineID, 
                     pre_edge_pointID, post_edge_pointID,
                     p1, vec2(0.0f, 0.0f));
  
  

  GLushort first(pts.size());
  
  pts.push_back(pp);
  pts.push_back(pp0);
  pts.push_back(qq0);
  pts.push_back(qq);
  pts.push_back(qq1);
  pts.push_back(pp1);
  
  
  inds.push_back(first);
  inds.push_back(first+1);
  inds.push_back(first+2);

  inds.push_back(first);
  inds.push_back(first+2);
  inds.push_back(first+3);

  inds.push_back(first);
  inds.push_back(first+3);
  inds.push_back(first+4);

  inds.push_back(first);
  inds.push_back(first+4);
  inds.push_back(first+5);

  inds.push_back(first);
  inds.push_back(first+5);
  inds.push_back(first+1);
  
}

void
common_join_data::
do_rounded_join(float curve_thresh,
                std::vector<WRATHShapePreStrokerPayload::JoinPoint> &pts,
                std::vector<GLushort> &inds)
{
  GLushort first(pts.size());
  JoinPoint center(outlineID, 
                   pre_edge_pointID, post_edge_pointID,
                   avg_p, vec2(0.0f,0.0f));
  pts.push_back(center);

  JoinPoint pz0(outlineID, 
                pre_edge_pointID, post_edge_pointID,
                p0, -lambda*n0);
  pts.push_back(pz0);

  JoinPoint pc0(outlineID, 
                pre_edge_pointID, post_edge_pointID,
                p0, lambda*n0);
  pts.push_back(pc0);


  /*
    make an arc from avg_p+w*n0 to avg_p+w*n1
    on the circle with center at avg_p 
    of radius w. One key point hapenning
    is that the total amount of angle
    traversed from n0 to n1 should be
    no more than M_PI (i.e. half a circle).
    The trick we employ here is that first
    we rotate n0 and n1 so that n0 is (1,0).
    This is accomplised by applying a complex
    multiply to conjuage(n0). Atan2 gives a
    result in the range [-M_PI, M_PI], exactly
    what we want. So we do the "rounding" computation
    after rotating by conjugage(n0) and then
    rotate by n0 to get what we need.
   */
  float theta, delta_theta;
  int i, num_pts;
  std::complex<float> n0z(lambda*n0.x(), lambda*n0.y());
  std::complex<float> n1z(lambda*n1.x(), lambda*n1.y());
  std::complex<float> n1zRotated( n1z*std::conj(n0z));

  
  delta_theta=atan2(n1zRotated.imag(), n1zRotated.real());

  num_pts=static_cast<int>( std::abs(delta_theta)/curve_thresh);
  num_pts=std::max(num_pts, 3);
  
  delta_theta/=static_cast<float>(num_pts-1);
  for(theta=delta_theta, i=1; i<num_pts-1; ++i, theta+=delta_theta)
    {
      vec2 q;
      std::complex<float> cs;
      float s, c;

      //      sincosf(theta, &s, &c);
      s=sinf(theta);
      c=cosf(theta);
      cs=std::complex<float>(c, s);
      cs*=n0z;
      
      JoinPoint pt(outlineID, 
                   pre_edge_pointID, post_edge_pointID,
                   avg_p, vec2(cs.real(), cs.imag()) );

      pts.push_back(pt);
    }
  

  JoinPoint pc1(outlineID, 
                 pre_edge_pointID, post_edge_pointID,
                 p1, lambda*n1);
  pts.push_back(pc1);

  JoinPoint pz1(outlineID, 
                pre_edge_pointID, post_edge_pointID,
                p1, -lambda*n1);
  pts.push_back(pz1);

  int end_indx(pts.size());

  /*
    now make the triangles:
   */
  for(int i=first+1; i<end_indx-1; ++i)
    {
      inds.push_back(first);
      inds.push_back(i);
      inds.push_back(i+1);
    }
}

/////////////////////////////////////
// WRATHShapePreStrokerPayload methods
void
WRATHShapePreStrokerPayload::
generate_data(void)
{
  WRATHassert(m_h.valid());

  m_effective_curve_thresh=std::max(float(M_PI)/256.0f,
                                    m_h->parameters().curve_tessellation_threshhold());

  for(std::vector<Outline::handle>::const_iterator 
        iter=m_h->tessellation().begin(),
        end=m_h->tessellation().end(); iter!=end; ++iter)
    {
      /*
        Handle outline does NOT make the joins that
        are formed if the outline is closed
        We want to make those joins at the end of our 
        arrays, thus we do those at the end.
       */
      handle_outline(*iter);
    }

  /*
    Mark the location of where the joins
    for closing the outline will be made.
   */
  m_miter_joins.set_markers();
  m_bevel_joins.set_markers();
  m_rounded_joins.set_markers();

  for(std::vector<Outline::handle>::const_iterator 
        iter=m_h->tessellation().begin(),
        end=m_h->tessellation().end(); iter!=end; ++iter)
    {
      const Outline::handle &O(*iter);
      
      if(!O->edges().empty())
        {
          /*
            The join at the start point of the outline
           */
          handle_join(O, 
                      O->edges().back(), 
                      O->edges().front());

          /*
            The join at the end point of the outline
            which is the join from the 2nd to last
            edge to the last edge.
           */
          if(O->edges().size()>1)
            {
              unsigned int last(O->edges().size()-1);
              handle_join(O, 
                          O->edges()[last-1],
                          O->edges()[last]);
            }
        }
    }
}

void
WRATHShapePreStrokerPayload::
handle_outline(const Outline::handle &O)
{
  if(O->edges().empty())
    {
      return;
    }

  for(int i=1, endi=O->edges().size()-1; i!=endi; ++i)
    {
      handle_join(O, 
                  O->edges()[i-1],
                  O->edges()[i]);
    }

  //make caps at the last point:
  //the last edge connects
  //the last point of a WRATHShape<T>
  //to the first point of the WRATHShape<T>,
  //so we want the edge before the last one:
  handle_cap(O, 
             O->edge_to_last_point()->curve_points().back(),
             false);
  
  
  //make caps at the first point:
  handle_cap(O, 
             O->edges().front()->curve_points().front(),
             true);
}

void
WRATHShapePreStrokerPayload::
handle_join(const Outline::handle &O,
            const Edge::handle &pre,
            const Edge::handle &post)
{
  if(m_flags&generate_joins)
    {
      common_join_data CJD(O, pre, post);

      if(m_flags&generate_bevel_joins)
        {
          CJD.do_bevel_join(m_bevel_joins.m_pts,
                            m_bevel_joins.m_indices);
        }

      

      if(m_flags&generate_miter_joins)
        {
          CJD.do_miter_join(m_miter_joins.m_pts,
                            m_miter_joins.m_indices);
        }

      if(m_flags&generate_rounded_joins)
        {
          CJD.do_rounded_join(m_effective_curve_thresh,
                              m_rounded_joins.m_pts,
                              m_rounded_joins.m_indices);
        }
      
    }

}

void
WRATHShapePreStrokerPayload::
handle_cap(const Outline::handle &O,
           const CurvePoint &pt,
           bool is_starting_cap)
{
  if(m_flags&generate_caps)
    {
      common_cap_data CCD(O, pt, is_starting_cap);

      if(m_flags&generate_square_caps)
        {
          CCD.do_square_cap(m_square_caps.m_pts,
                            m_square_caps.m_indices);
                            
        }

      if(m_flags&generate_rounded_caps)
        {
          CCD.do_rounded_cap(m_effective_curve_thresh,
                             m_rounded_caps.m_pts,
                             m_rounded_caps.m_indices);
        }
    }
}

//////////////////////////////////////////
//WRATHShapePreStrokerPayload::MiterJoinPoint methods
vec2
WRATHShapePreStrokerPayload::MiterJoinPoint::
offset_vector(float miter_limit) const
{
  if(m_depends_on_miter_limit)
    {
      if(std::abs(m_lhs)>miter_limit*std::abs(m_rhs))
        {
          if( (m_lhs<0.0f) xor (m_rhs<0.0f) )
            {
              miter_limit=-miter_limit;
            }
          return m_n+miter_limit*m_v;
        }
      else
        {
          return m_n+(m_lhs/m_rhs)*m_v;
        }
    }
  else
    {
      return m_v;
    }
}
