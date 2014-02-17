/*  -*- C++ -*-
  Code's structure and implementation are based off of:
  http://code.google.com/p/poly2tri/
  whose License is:

  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *
  * * Redistributions of source code must retain the above copyright notice,
  *   this list of conditions and the following disclaimer.
  * * Redistributions in binary form must reproduce the above copyright notice,
  *   this list of conditions and the following disclaimer in the documentation
  *   and/or other materials provided with the distribution.
  * * Neither the name of Poly2Tri nor the names of its contributors may be
  *   used to endorse or promote products derived from this software without specific
  *   prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  
  
 */

#if !defined(WRATH_HEADER_TRIANGULATION_HPP_) || defined(WRATH_HEADER_TRIANGULATION_IMPLEMENT_TCC_)
#error "Direction inclusion of private header file WRATHTriangulationImplement.tcc"
#endif

#include "WRATHConfig.hpp"

#define WRATH_HEADER_TRIANGULATION_IMPLEMENT_TCC_

#include <limits>
#include <algorithm>

namespace
{
}

//////////////////////////////////////////////
//WRATHTriangulation<T, T_point_index>::TriangulatedComponent methods
template<typename T, typename T_point_index>
inline
int
WRATHTriangulation<T, T_point_index>::TriangulatedComponent::
winding_number(void) const
{
  WRATHassert(valid());
  return m_data->winding_number();
}


template<typename T, typename T_point_index>
inline
const std::vector<typename WRATHTriangulation<T, T_point_index>::point_index>&
WRATHTriangulation<T, T_point_index>::TriangulatedComponent::
triangulation(void) const
{
  return m_data->triangle_commands();
}

template<typename T, typename T_point_index>
inline
const std::vector<typename WRATHTriangulation<T, T_point_index>::TriangulatedComponent::edge_data>&
WRATHTriangulation<T, T_point_index>::TriangulatedComponent::
edges(void) const
{
  return m_data->edges();
}

template<typename T, typename T_point_index>
inline
const std::vector<typename WRATHTriangulation<T, T_point_index>::ConnectedComponent>&
WRATHTriangulation<T, T_point_index>::TriangulatedComponent::
edge_neighbors(void) const
{
  WRATHassert(valid());
  return m_data->edge_neighbors();
}

/////////////////////////////////////////////
//WRATHTriangulation<T, T_point_index>::EdgeData methods
template<typename T, typename T_point_index>
WRATHTriangulation<T, T_point_index>::EdgeData::
EdgeData(PointData *p1, PointData *p2):
  m_p(p1),
  m_q(p2),
  m_reversed(false),
  m_p_neighbor(NULL),
  m_q_neighbor(NULL),
  m_p_observe_intersection(false),
  m_q_observe_intersection(false)
{
  WRATHassert(p1!=NULL and p2!=NULL);
  
  if(p1->y()==p2->y())
    {
      m_edge_classification=edge_flat;
      m_edge_sign=0;
    }
  else if(p1->y()<p2->y())
    {
      m_edge_classification=edge_rising;
      m_edge_sign=1;
    }
  else
    {
      m_edge_classification=edge_falling;
      m_edge_sign=-1;
    }


  if(!PointData::compare_pts(p1,p2))
    {
      std::swap(m_p, m_q);
      m_reversed=true;
    }

  m_q->m_edges.push_back(this);

  WRATHassert(m_p->y()<=m_q->y());
  m_three_time_low_y=T(3)*m_p->y();
  m_three_time_high_y=T(3)*m_q->y();

  /*
    Let J(x,y) = (-y,x).

    Given an edge [e,f], the winding rule needs to 
    compute the sign of:

    F(e,f,t):= ( <t, J(f-e)> + <f, J(e)> )/(f_y - e_y)
    
    We have arranged it so that e.y<f_y, so we need to compute
    the sign of:

    F(e,f,t)=<t, J(f-e)> + <f, J(e)>

    but we are given 3t, note that:

    F(3e, 3f, 3t) = 9F(e, f, t)
                  = <3t, 3J(f-e) > + 9<f,J(e) >
                  = 3*( <3t, J(f-e)> + 3<f,J(e)> )

    The winding rule contribution is as follows:
    Compute:
       <3t, J(f-e)> + 3<f,J(e)>
    if positive then return m_edge_sign.

   */
  vecN<product_type, 2> ee(m_p->x(), m_p->y());
  vecN<product_type, 2> ff(m_q->x(), m_q->y());
  vecN<product_type, 2> ff_minus_ee(ff-ee);

  m_compute_offset= product_type(3)*dot(ff, vecN<product_type, 2>(-ee.y(), ee.x()) );
  m_twisted_edge_delta=vecN<product_type, 2>(-ff_minus_ee.y(), ff_minus_ee.x());
}

template<typename T, typename T_point_index>
int
WRATHTriangulation<T, T_point_index>::EdgeData::
compute_winding_contribution(const vecN<product_type,2> &triangle_center_times3)
{
  if(triangle_center_times3.y()<m_three_time_low_y)
    {
      return 0;
    }
  else if(triangle_center_times3.y()>m_three_time_high_y)
    {
      return 0;
    }
  else if(triangle_center_times3.y()==m_three_time_low_y)
    {
      bool test;

      test=m_p_observe_intersection and
        triangle_center_times3.x() < T(3)*m_p->x();

      return (test)?m_edge_sign:0;
    }
  else if(triangle_center_times3.y()==m_three_time_high_y)
    {
      bool test;

      test=m_q_observe_intersection and
        triangle_center_times3.x() < T(3)*m_q->x();

      return (test)?m_edge_sign:0;
    }
  else
    {
      product_type v;

      v=dot(m_twisted_edge_delta, triangle_center_times3) 
        + m_compute_offset;

      return (v>product_type(0))?m_edge_sign:0;
    }
}


template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::EdgeData::
set_previous_neighbor(EdgeData *pprev)
{
  if(m_reversed)
    {
      WRATHassert(m_q_neighbor==NULL);
      m_q_neighbor=pprev;
    }
  else
    {
      WRATHassert(m_p_neighbor==NULL);
      m_p_neighbor=pprev;
    }
  pprev->set_next_neighbor(this);
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::EdgeData::
set_next_neighbor(EdgeData *pnext)
{
  if(m_reversed)
    {
      WRATHassert(m_p_neighbor==NULL);
      m_p_neighbor=pnext;
    }
  else
    {
      WRATHassert(m_q_neighbor==NULL);
      m_q_neighbor=pnext;
    }
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::EdgeData::
set_classifications_implement(EdgeData *start)
{
  if(this==start)
    {
      return;
    }

  if(start==NULL)
    {
      start=this;
    }

  /*
    Basic idea:
      If the next edge is the same type (i.e. rising vs falling)
      then the next edge will take the intersection.
      Thus we set next_classification() as false always
      and next->prev->classification() as depends.

      We need special logic to handle traversing 
      through flat edges.
   */

  if(m_edge_classification==edge_flat)
    {
      m_p_observe_intersection=false;
      m_q_observe_intersection=false;
      next_neighbor()->set_classifications_implement(start);
    }
  else
    {
      EdgeData *next;
      bool should_continue(true);

      /*
        increment to first edge that comes after this
        that does not have a flat edge.
        All intermediate edges will be classified
        as to be ignored. We need to increment to the
        1st edge that is not flat (if there is any),
        we might end up incrementing past the start,
        which is ok. If we do increment past (or to)
        the start we will not recurse to the next.
       */
      for(next=next_neighbor(); 
          next!=this and next->m_edge_classification==edge_flat; 
          next=next->next_neighbor())
        {
          next->m_p_observe_intersection=false;
          next->m_q_observe_intersection=false;
          should_continue=should_continue and next!=start;
        }

      WRATHassert(next!=this);
      if(m_edge_classification==next->m_edge_classification)
        {
          /*
            We let the next have the hit.
           */
          next_classification()=false;
          next->prev_classification()=true;
        }
      else
        {
          next_classification()=false;
          next->prev_classification()=false;
        }

      if(should_continue)
        {
          next->set_classifications_implement(start);
        }
    }
}

//////////////////////////////
// WRATHTriangulation<T, T_point_index>::edge_set methods
template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::edge_set::
mark_edge(EdgeData *e)
{
  WRATHassert(e!=NULL);
  mark_edge(e->p(), e->q());
}

template<typename T, typename T_point_index>
vecN<typename WRATHTriangulation<T, T_point_index>::PointData*, 2>
WRATHTriangulation<T, T_point_index>::edge_set::
edge_source(int I) const
{
  WRATHassert(I>=0 and I<3);
  switch(I)
    {
    case 0:
      return vecN<PointData*,2>(m_pts_ref[1], m_pts_ref[2]);

    case 1:
      return vecN<PointData*,2>(m_pts_ref[0], m_pts_ref[2]);

    case 2:
      return vecN<PointData*,2>(m_pts_ref[0], m_pts_ref[1]);

    default:
      return vecN<PointData*,2>(NULL, NULL);
      
    }
  
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::edge_set::
mark_edge(PointData *p, PointData *q)
{
  WRATHassert(p!=NULL);
  WRATHassert(q!=NULL);

  if( (q==m_pts_ref[0] and p==m_pts_ref[1])
      or (q==m_pts_ref[1] and p==m_pts_ref[0])) 
    {
      (*this)[2]=true;
    } 
  else if( (q==m_pts_ref[0] and p==m_pts_ref[2]) 
           or (q==m_pts_ref[2] and p==m_pts_ref[0])) 
    {
      (*this)[1]=true;
    } 
  else if( (q==m_pts_ref[1] and p==m_pts_ref[2]) 
           or (q==m_pts_ref[2] and p==m_pts_ref[1])) 
    {
      (*this)[0]=true;
    }
}


template<typename T, typename T_point_index>
bool
WRATHTriangulation<T, T_point_index>::edge_set::
edge_cw(PointData *p)
{
  WRATHassert(p!=NULL);

  if(p==m_pts_ref[0]) 
    {
      return (*this)[1];
    }
  else if(p==m_pts_ref[1]) 
    {
      return (*this)[2];
    }
  return (*this)[0];
}

template<typename T, typename T_point_index>
bool
WRATHTriangulation<T, T_point_index>::edge_set::
edge_ccw(PointData *p)
{
  WRATHassert(p!=NULL);

  if(p==m_pts_ref[0]) 
    {
      return (*this)[2];
    }
  else if(p==m_pts_ref[1]) 
    {
      return (*this)[0];
    }
  return (*this)[1];
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::edge_set::
edge_cw(PointData *p, bool v)
{
  WRATHassert(p!=NULL);

  if(p==m_pts_ref[0]) 
    {
      (*this)[1]=v;
    }
  else if(p==m_pts_ref[1]) 
    {
      (*this)[2]=v;
    }
  else
    {
      (*this)[0]=v;
    }
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::edge_set::
edge_ccw(PointData *p, bool v)
{
  WRATHassert(p!=NULL);
  if(p==m_pts_ref[0]) 
    {
      (*this)[2]=v;
    }
  else if(p==m_pts_ref[1]) 
    {
      (*this)[0]=v;
    }
  else
    {
      (*this)[1]=v;
    }
}

//////////////////////////////////
// WRATHTriangulation<T, T_point_index>::TriangleData methods
template<typename T, typename T_point_index>
WRATHTriangulation<T, T_point_index>::TriangleData::
TriangleData(PointData *p0, PointData *p1, PointData *p2):
  m_points(p0, p1, p2),
  m_neighbors(NULL, NULL, NULL),
  m_component(NULL),
  m_is_induced_triangle(p0->is_induced_point() or p1->is_induced_point() or p2->is_induced_point()),
  m_is_constrained_edge(m_points),
  m_is_delaunay_edge(m_points)
{}

template<typename T, typename T_point_index>
bool
WRATHTriangulation<T, T_point_index>::TriangleData::
is_induced_triangle(void) const
{
  return m_is_induced_triangle;
}


template<typename T, typename T_point_index>
int
WRATHTriangulation<T, T_point_index>::TriangleData::
pt_index(PointData *p)
{
  WRATHassert(p!=NULL);
  if(p==m_points[0])
    {
      return 0;
    }
  else if(p==m_points[1])
    {
      return 1;
    }
  else if(p==m_points[2])
    {
      return 2;
    }
  else
    {
      throw WRATHTriangulationPrivate::TriangulationException();
    }
}

template<typename T, typename T_point_index>
int
WRATHTriangulation<T, T_point_index>::TriangleData::
edge_index(PointData *p1, PointData *p2)
{
  WRATHassert(p1!=NULL);
  WRATHassert(p2!=NULL);

  if(m_points[0]==p1) 
    {
      if(m_points[1]==p2) 
        {
          return 2;
        } 
      else if(m_points[2]==p2) 
        {
          return 1;
        }
  } 
  else if(m_points[1]==p1) 
    {
      if(m_points[2]==p2) 
        {
          return 0;
        } 
      else if(m_points[0]==p2) 
        {
          return 2;
        }
    } 
  else if(m_points[2]==p1) 
    {
      if(m_points[0]==p2) 
        {
          return 1;
        } 
      else if(m_points[1]==p2) 
        {
          return 0;
        }
    }

  return -1;
  
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::TriangleData::
legalize(PointData *pt)
{
  WRATHassert(pt!=NULL);

  m_points[1]=m_points[0];
  m_points[0]=m_points[2];
  m_points[2]=pt;

  m_is_induced_triangle=m_points[0]->is_induced_point()
    or m_points[1]->is_induced_point()
    or m_points[2]->is_induced_point();
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::TriangleData::
legalize(PointData *opoint, PointData *npoint)
{
  WRATHassert(opoint!=NULL);
  WRATHassert(npoint!=NULL);

  if(opoint==m_points[0]) 
    {
      m_points[1]=m_points[0];
      m_points[0]=m_points[2];
      m_points[2]=npoint;
    } 
  else if(opoint==m_points[1]) 
    {
      m_points[2]=m_points[1];
      m_points[1]=m_points[0];
      m_points[0]=npoint;
    } 
  else if(opoint==m_points[2]) 
    {
      m_points[0]=m_points[2];
      m_points[2]=m_points[1];
      m_points[1]=npoint;
    } 
  else
    {
      throw WRATHTriangulationPrivate::TriangulationException();
    }

  m_is_induced_triangle=m_points[0]->is_induced_point()
    or m_points[1]->is_induced_point()
    or m_points[2]->is_induced_point();
}

template<typename T, typename T_point_index>
typename WRATHTriangulation<T, T_point_index>::PointData*
WRATHTriangulation<T, T_point_index>::TriangleData::
point_cw(PointData *pt)
{
  //slowerer: return m_points[ (2+pt_index(pt)%3 ];

  if(pt==m_points[0]) 
    {
      return m_points[2];
    } 
  else if(pt==m_points[1]) 
    {
      return m_points[0];
    } 
  else if(pt==m_points[2]) 
    {
      return m_points[1];
    }
  else
    {
      throw WRATHTriangulationPrivate::TriangulationException();
      return NULL;
    }
}

template<typename T, typename T_point_index>
typename WRATHTriangulation<T, T_point_index>::PointData*
WRATHTriangulation<T, T_point_index>::TriangleData::
point_ccw(PointData *pt)
{
  WRATHassert(pt!=NULL);
  //slower: return m_points[ (1+pt_index(pt)%3 ];

  if(pt==m_points[0]) 
    {
      return m_points[1];
    } 
  else if(pt==m_points[1]) 
    {
      return m_points[2];
    } 
  else if(pt==m_points[2]) 
    {
      return m_points[0];
    }
  else
    {
      throw WRATHTriangulationPrivate::TriangulationException();
      return NULL;
    }
}

template<typename T, typename T_point_index>
typename WRATHTriangulation<T, T_point_index>::PointData*
WRATHTriangulation<T, T_point_index>::TriangleData::
opposite_point(TriangleData *t, PointData *p)
{
  WRATHassert(t!=NULL);
  WRATHassert(p!=NULL);

  PointData *cw=t->point_cw(p);
  return point_cw(cw);
}


template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::TriangleData::
mark_neighbor_data(PointData *p1, 
                   PointData *p2, 
                   TriangleData *t)
{
  WRATHassert(t!=NULL);
  WRATHassert(p1!=NULL);
  WRATHassert(p2!=NULL);

  if((p1==m_points[2] and p2==m_points[1]) or (p1==m_points[1] and p2==m_points[2]))
    {
      m_neighbors[0]=t;
    }
  else if((p1==m_points[0] and p2==m_points[2]) or (p1==m_points[2] and p2==m_points[0]))
    {
      m_neighbors[1]=t;
    }
  else if((p1==m_points[0] and p2==m_points[1]) or (p1==m_points[1] and p2==m_points[0]))
    {
      m_neighbors[2]=t;
    }
  else
    {
      throw WRATHTriangulationPrivate::TriangulationException();
    }
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::TriangleData::
clear_neighbors(void)
{
  m_neighbors[0]=NULL;
  m_neighbors[1]=NULL;
  m_neighbors[2]=NULL;
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::TriangleData::
mark_neighbor_data(TriangleData *t)
{
  WRATHassert(t!=NULL);
  if(t->has_edge(m_points[1], m_points[2])) 
    {
      m_neighbors[0]=t;
      t->mark_neighbor_data(m_points[1], m_points[2], this);
    } 
  else if(t->has_edge(m_points[0], m_points[2])) 
    {
      m_neighbors[1]=t;
      t->mark_neighbor_data(m_points[0], m_points[2], this);
    }
  else if(t->has_edge(m_points[0], m_points[1])) 
    {
      m_neighbors[2]=t;
      t->mark_neighbor_data(m_points[0], m_points[1], this);
    }
}



template<typename T, typename T_point_index>
typename WRATHTriangulation<T, T_point_index>::TriangleData*
WRATHTriangulation<T, T_point_index>::TriangleData::
neighbor_across(PointData *p)
{
  WRATHassert(p!=NULL);

  if(p==m_points[0])
    {
      return m_neighbors[0];
    }
  else if(p==m_points[1])
    {
      return m_neighbors[1];
    }
  else if(p==m_points[2])
    {
      return m_neighbors[2];
    }
  else
    {
      throw WRATHTriangulationPrivate::TriangulationException();
      return NULL;
    }
}

template<typename T, typename T_point_index>
typename WRATHTriangulation<T, T_point_index>::TriangleData*
WRATHTriangulation<T, T_point_index>::TriangleData::
neighbor_cw(PointData *pt)
{
  //slowerer: return m_neighbors[ (1+pt_index(pt)%3 ];
  WRATHassert(pt!=NULL);

  if(pt==m_points[0]) 
    {
      return m_neighbors[1];
    } 
  else if(pt==m_points[1]) 
    {
      return m_neighbors[2];
    } 
  else if(pt==m_points[2]) 
    {
      return m_neighbors[0];
    }
  else
    {
      throw WRATHTriangulationPrivate::TriangulationException();
      return NULL;
    }
}

template<typename T, typename T_point_index>
typename WRATHTriangulation<T, T_point_index>::TriangleData*
WRATHTriangulation<T, T_point_index>::TriangleData::
neighbor_ccw(PointData *pt)
{
  //slowerer: return m_neighbors[ (2+pt_index(pt)%3 ];
  WRATHassert(pt!=NULL);

  if(pt==m_points[0]) 
    {
      return m_neighbors[2];
    } 
  else if(pt==m_points[1]) 
    {
      return m_neighbors[0];
    } 
  else if(pt==m_points[2]) 
    {
      return m_neighbors[1];
    }
  else
    {
      throw WRATHTriangulationPrivate::TriangulationException();
      return NULL;
    }
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::TriangleData::
component_marker(ConnectedComponent *c)
{
  if(m_component!=NULL)
    {
      if(m_component!=c)
        {
          throw WRATHTriangulationPrivate::ConnectedComponentException();
        }
      return;
    }

  m_component=c;
  m_component->add_triangle(this);
  for(int i=0; i<3; ++i)
    {
      if(!m_is_constrained_edge[i]
         and neighbor(i)!=NULL)
        {
          neighbor(i)->component_marker(c);
        }
      
      if(neighbor(i)!=NULL
         and !((m_is_constrained_edge[i]) xor (neighbor(i)->m_component==m_component)))
        {
          throw WRATHTriangulationPrivate::ConnectedComponentException(); 
        }
    }
}


////////////////////////////////////
//WRATHTriangulation<T, T_point_index>::ConnectedComponent methods
template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::ConnectedComponent::
add_triangle(TriangleData *p)
{
  m_triangles.push_back(p);
  if(p->m_is_constrained_edge[0]
     or p->m_is_constrained_edge[1]
     or p->m_is_constrained_edge[2])
    {
      m_triangles_with_constrained_edges.push_back(p);
    }

  if(p->is_induced_triangle())
    {
      m_is_induced_component=true;
    }
     

}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::ConnectedComponent::
compute_edge_neighbors(void)
{

  m_edge_neighbors.reserve(m_triangle_commands.size());
  for(typename std::list<TriangleData*>::const_iterator iter=m_triangles.begin(),
        end=m_triangles.end(); iter!=end; ++iter)
    {
      TriangleData *tri(*iter);

      WRATHassert(tri!=NULL);

      for(int i=0;i<3;++i)
        {
          TriangleData *neighbor_tri;
          ConnectedComponent *C(NULL);

          neighbor_tri=tri->neighbor(i);
          if(neighbor_tri!=NULL 
             and neighbor_tri->connected_component()!=NULL)
            {
              C=neighbor_tri->connected_component();
            }
          m_edge_neighbors.push_back(TriangulatedComponent(C));
        }
    }
}


template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::ConnectedComponent::
compute_winding_number(const std::vector<EdgeData*> &edge_list)
{
  WRATHassert(!m_winding_number_computed);
  m_winding_number_computed=true;
  
  

  m_winding_number=0;
  if(!triangles().empty())
    {
      m_winding_number=compute_winding_number(compute_3times_triangle_center(triangles().front()),
                                              edge_list);

      
    }
}

template<typename T, typename T_point_index>
int
WRATHTriangulation<T, T_point_index>::ConnectedComponent::
compute_winding_number(const vecN<product_type, 2> &pp_triangle_center_times3,
                       const std::vector<EdgeData*> &edge_list)
{
  int v(0);

  for(typename std::vector<EdgeData*>::const_iterator 
        iter=edge_list.begin(), end=edge_list.end();
      iter!=end; ++iter)
    {
      v+=(*iter)->compute_winding_contribution(pp_triangle_center_times3);
    }
  return v;
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::ConnectedComponent::
compute_winding_number(const std::vector<EdgeData*> &edge_list_sorted_by_low_y,
                       const std::vector<EdgeData*> &edge_list_reverse_sorted_by_high_y,
                       std::vector<EdgeData*> &work_room1,
                       std::vector<EdgeData*> &work_room2)
{
  WRATHassert(!m_winding_number_computed);
  m_winding_number_computed=true;

  
  m_winding_number=0;
  if(!triangles().empty())
    {
      m_winding_number=compute_winding_number(compute_3times_triangle_center(triangles().front()),
                                              edge_list_sorted_by_low_y,
                                              edge_list_reverse_sorted_by_high_y,
                                              work_room1, work_room2);
    }
}
                                              

template<typename T, typename T_point_index>
int
WRATHTriangulation<T, T_point_index>::ConnectedComponent::
compute_winding_number(const vecN<product_type, 2> &pp_triangle_center_times3,
                       const std::vector<EdgeData*> &edge_list_sorted_by_low_y,
                       const std::vector<EdgeData*> &edge_list_reverse_sorted_by_high_y,
                       std::vector<EdgeData*> &work_room1,
                       std::vector<EdgeData*> &work_room2)
{
  typedef typename std::vector<EdgeData*>::const_iterator ptr_const_array_iterator;
  typedef typename std::vector<EdgeData*>::iterator ptr_array_iterator;
  typename EdgeData::hunt_low_y low_y_hunter;
  typename EdgeData::reverse_hunt_high_y high_y_hunter;

  ptr_const_array_iterator first_element_with_low_y_too_big;
  ptr_const_array_iterator first_element_with_big_y_too_small;
  ptr_array_iterator workroom1_end, workroom2_end;
  ptr_array_iterator workroom1_iter, workroom2_iter;      
  int return_value(0);

  first_element_with_low_y_too_big=std::upper_bound(edge_list_sorted_by_low_y.begin(),
                                                    edge_list_sorted_by_low_y.end(),
                                                    static_cast<T>(pp_triangle_center_times3.y()), 
                                                    low_y_hunter);
  
  first_element_with_big_y_too_small=std::upper_bound(edge_list_reverse_sorted_by_high_y.begin(),
                                                      edge_list_reverse_sorted_by_high_y.end(),
                                                      static_cast<T>(pp_triangle_center_times3.y()), 
                                                      high_y_hunter);
  
  workroom1_end=std::copy(edge_list_sorted_by_low_y.begin(), 
                          first_element_with_low_y_too_big,
                          work_room1.begin());
  std::sort(work_room1.begin(), workroom1_end);
  workroom1_iter=work_room1.begin();
  
  workroom2_end=std::copy(edge_list_reverse_sorted_by_high_y.begin(),
                          first_element_with_big_y_too_small,
                          work_room2.begin());
  std::sort(work_room2.begin(), workroom2_end);
  workroom2_iter=work_room2.begin();
  
  while(workroom1_iter!=workroom1_end and workroom2_iter!=workroom2_end)
    {
      if(*workroom1_iter<*workroom2_iter) 
        {
          ++workroom1_iter;
        }
      else if(*workroom2_iter<*workroom1_iter)
        {
          ++workroom2_iter;
        }
      else 
        {
          EdgeData *ptr(*workroom1_iter);
          
          return_value+=ptr->compute_winding_contribution(pp_triangle_center_times3);
          
          ++workroom1_iter;
          ++workroom2_iter;
        }
    }
  
  return return_value;
}


template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::ConnectedComponent::
add_trianglulation(std::vector<point_index> &plist) const
{
  plist.reserve(plist.size() + m_triangle_commands.size());
  for(typename std::vector<point_index>::const_iterator 
        iter=m_triangle_commands.begin(),
        end=m_triangle_commands.end();
      iter!=end; ++iter)
    {
      plist.push_back(*iter);
    }
}


template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::ConnectedComponent::
build_triangle_commands(const std::vector<input_point> &pts)
{
  m_triangle_commands.reserve(3*triangles().size());
  WRATHassert(m_triangle_commands.empty());

  for(typename std::list<TriangleData*>::const_iterator
        t=triangles().begin(),
        t_end=triangles().end();
      t!=t_end; ++t)
    {
      TriangleData *tri(*t);

      if(!tri->is_induced_triangle())
        {
          for(int p=0;p<3;++p)
            {
              point_index ind;
              
              ind=tri->pt(p)->id();
              m_triangle_commands.push_back(pts[ind].m_index);
              
              if(tri->m_is_constrained_edge[p])
                {
                  vecN<PointData*, 2> edge_source;
                  vecN<point_index, 2> edge_inds;
                  TriangleData *across_tri(NULL);
                  ConnectedComponent *C(NULL);
                  bool skip_edge(false);
                  
                  edge_source=tri->m_is_constrained_edge.edge_source(p);
                  across_tri=tri->neighbor(p);
                  
                  if(across_tri!=NULL 
                     and !across_tri->connected_component()->is_induced_component())
                    {
                      C=across_tri->connected_component();
                      skip_edge=(across_tri->connected_component()==this);
                    }
                  
                  edge_inds[0]=pts[ edge_source[0]->id() ].m_index;
                  edge_inds[1]=pts[ edge_source[1]->id() ].m_index;
                  
                  if(!skip_edge)
                    {
                      m_edges.push_back(edge_data());

                      m_edges.back().m_edge_indices=edge_inds;
                      m_edges.back().m_neighbor_component=TriangulatedComponent(C);
                      m_edges.back().m_interior_index=pts[ tri->pt(p)->id() ].m_index;
                    }
                }
            }
        }
    }
}



////////////////////////////////////////////
// WRATHTriangulation<T, T_point_index>::AdvancingFrontData methods
template<typename T, typename T_point_index>
typename WRATHTriangulation<T, T_point_index>::NodeData* 
WRATHTriangulation<T, T_point_index>::AdvancingFrontData::
locate_node(T x)
{
  NodeData *node(m_search_node);

  WRATHassert(node!=NULL);

  if(x<node->m_value) 
    {
      while ((node=node->m_prev)!=NULL) 
        {
          if (x>=node->m_value) 
            {
              m_search_node=node;
              return node;
            }
        }
    } 
  else 
    {
      while ((node=node->m_next)!=NULL) 
        {
          if (x<node->m_value) 
            {
              m_search_node=node->m_prev;
              return node->m_prev;
            }
        }
    }

  return NULL;
}

template<typename T, typename T_point_index>
typename WRATHTriangulation<T, T_point_index>::NodeData* 
WRATHTriangulation<T, T_point_index>::AdvancingFrontData::
locate_point(PointData* point)
{
  T px(point->x());
  NodeData* node(m_search_node);
  T nx(node->m_point->x());
  
  
  WRATHassert(node!=NULL);

  if(px==nx) 
    {
      if(point!=node->m_point) 
        {
          if(point==node->m_prev->m_point) 
            {
              node=node->m_prev;
            } 
          else if(point==node->m_next->m_point) 
            {
              node=node->m_next;
            } 
          else 
            {
              throw WRATHTriangulationPrivate::TriangulationException();
            }
        }
    } 
  else if(px<nx) 
    {
      while ((node=node->m_prev)!= NULL)
        {
          if(point==node->m_point) 
            {
              break;
            }
        }
    } 
  else
    {
      while((node=node->m_next)!=NULL)
        {
          if (point==node->m_point)
            break;
        }
    }
  
  if(node!=NULL)
    {
      m_search_node=node;
    }
  
  return node;
}

////////////////////////////////////////////
// WRATHTriangulation<T, T_point_index>::WorkHorse methods
template<typename T, typename T_point_index>
WRATHTriangulation<T, T_point_index>::WorkHorse::
WorkHorse(const std::vector<input_point> &pts,
          const std::vector< std::vector<point_index> > &outlines,
          const std::vector< vecN<point_index,2> > &extra_edges):
  m_front(NULL),
  m_head(NULL),
  m_tail(NULL),
  m_af_head(NULL),
  m_af_tail(NULL),
  m_af_middle(NULL),
  m_point_triangulation_fail(false),
  m_connected_component_computation_fail(false)
{
  if(!pts.empty())
    {
      try
        {
          initialize(pts, outlines, extra_edges);
          create_advancing_front();      
          triangulate();
        }
      catch(...)
        {
          //should we only catch TriangulationException 's?
          m_point_triangulation_fail=true;
        }

      try
        {
          find_interior_triangles(pts);
        }
      catch(...)
        {
          //should we only catch ConnectedComponentException 's?
          m_connected_component_computation_fail=true;
        }
    }
}

template<typename T, typename T_point_index>
WRATHTriangulation<T, T_point_index>::WorkHorse::
~WorkHorse()
{
  local_delete_each(m_all_triangles.begin(), m_all_triangles.end());
  local_delete_each(m_edge_list.begin(), m_edge_list.end());
  local_delete_each(m_constraint_edges.begin(), m_constraint_edges.end());
  local_delete_each(m_nodes.begin(), m_nodes.end());
  local_delete_each(m_points.begin(), m_points.end());
  local_delete_each(m_components.begin(), m_components.end());

  if(m_front!=NULL)
    {
      WRATHDelete(m_front);
    }

  
  if(m_head!=NULL)
    {
      WRATHDelete(m_head);
    }

  if(m_tail!=NULL)
    {
      WRATHDelete(m_tail);
    }

  if(m_af_head!=NULL)
    {
      WRATHDelete(m_af_head);
    }

  if(m_af_tail!=NULL)
    {
      WRATHDelete(m_af_tail);
    }

  if(m_af_middle!=NULL)
    {
      WRATHDelete(m_af_middle);
    }
}



template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
init_points(const std::vector<input_point> &pts)
{
  m_points.resize( pts.size());
  for(unsigned int i=0, lasti=pts.size(); i<lasti; ++i)
    {
      WRATHassert(i<std::numeric_limits<point_index>::max());
      m_points[i]=WRATHNew PointData( pts[i].m_position, static_cast<int>(i));
    }
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
init_edges(const std::vector< std::vector<point_index> > &outlines,
           const std::vector< vecN<point_index, 2> > &extra_edges)
{
  for(unsigned int outline=0, last_outline=outlines.size();
      outline<last_outline; ++outline)
    {
      EdgeData *previous_edge, *outline_start_edge;

      previous_edge=NULL;
      outline_start_edge=NULL;

      for(unsigned int e=0, last_e=outlines[outline].size(); 
          e<last_e; ++e)
        {
          int next_e;
          int inda, indb;

          next_e=(e==last_e-1)?
            0:
            e+1;
          
          inda=outlines[outline][e];
          indb=outlines[outline][next_e];

          if(inda!=indb) //don't allow degenerate edges.
            {
              m_edge_list.push_back(WRATHNew EdgeData(m_points[inda], m_points[indb]) );

              if(previous_edge!=NULL)
                {
                  m_edge_list.back()->set_previous_neighbor(previous_edge);
                }
              else
                {
                  outline_start_edge=m_edge_list.back();
                }
              previous_edge=m_edge_list.back();
            }

        }

      if(outline_start_edge!=NULL)
        {
          WRATHassert(previous_edge!=NULL);
          outline_start_edge->set_previous_neighbor(previous_edge);
          outline_start_edge->set_classifications();
        }
          
    }

  for(typename std::vector< vecN<point_index, 2> >::const_iterator iter=extra_edges.begin(),
        end=extra_edges.end(); iter!=end; ++iter)
    {
      m_constraint_edges.push_back(WRATHNew EdgeData(m_points[iter->x()], m_points[iter->y()]) );
    }
}



template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
create_advancing_front(void)
{
  TriangleData *t;

  t=WRATHNew TriangleData(m_points[0], m_tail, m_head);
  m_all_triangles.push_back(t);

  m_af_head=WRATHNew NodeData(t->pt(1), t);
  m_af_middle=WRATHNew NodeData(t->pt(0), t);
  m_af_tail=WRATHNew NodeData(t->pt(2));
  m_front=WRATHNew AdvancingFrontData(m_af_head, m_af_tail);

  m_af_head->m_next=m_af_middle;
  m_af_middle->m_next=m_af_tail;
  m_af_middle->m_prev=m_af_head;
  m_af_tail->m_prev=m_af_middle;
}


template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
find_interior_triangles(const std::vector<input_point> &pts)
{
  //create partitions of all triangles into connected components.
  for(typename std::vector<TriangleData*>::iterator iter=m_all_triangles.begin(),
        end=m_all_triangles.end(); iter!=end; ++iter)
    {
      TriangleData *t(*iter);

      if(t->connected_component()==NULL)
        {
          m_components.push_back(WRATHNew ConnectedComponent());
          t->component_marker(m_components.back());
        }
    }

  /*
    ConnectedComponent::compute_winding_number requires
    work room and the edge lists presented in several
    different sorting so that it can avoid walking
    many edges.
   */
  std::vector<EdgeData*> edge_list_sorted_by_low_y(m_edge_list);
  std::vector<EdgeData*> edge_list_reverse_sorted_by_high_y(m_edge_list);
  std::vector<EdgeData*> work_room1(m_edge_list.size(), NULL);
  std::vector<EdgeData*> work_room2(m_edge_list.size(), NULL);


  std::sort(edge_list_sorted_by_low_y.begin(), 
            edge_list_sorted_by_low_y.end(),
            &EdgeData::compare_edge_functor_sort_by_low_y);

  std::sort(edge_list_reverse_sorted_by_high_y.begin(), 
            edge_list_reverse_sorted_by_high_y.end(),
            &EdgeData::compare_edge_functor_sort_by_reverse_high_y);

  for(typename std::vector<ConnectedComponent*>::const_iterator 
        iter=connected_componenents().begin(),
        end=connected_componenents().end();
      iter!=end; ++iter)
    {
      ConnectedComponent *C(*iter);
      
      /*
        Only non induced components build triangle
        commands, compute winding numbers and
        compute edge neighbors. Note that
        computing edge neighbors must be done 
        AFTER all triangles have been placed into
        a connected component.
       */
      if(!C->is_induced_component())
        {
          C->build_triangle_commands(pts);
          C->compute_winding_number(edge_list_sorted_by_low_y,
                                    edge_list_reverse_sorted_by_high_y,
                                    work_room1, work_room2);
          C->compute_edge_neighbors();
        }
    }
  
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
triangulate(void)
{
  for(unsigned int i=1, last_i=m_points.size(); i<last_i; ++i)
    {
      PointData *pt;
      NodeData *node;

      pt=m_points[i];

      node=point_event(pt);

      for(int e=0, last_e=pt->m_edges.size(); e<last_e; ++e)
        {
          edge_event(pt->m_edges[e], node);
        } 
    }
}

template<typename T, typename T_point_index>
typename WRATHTriangulation<T, T_point_index>::NodeData*
WRATHTriangulation<T, T_point_index>::WorkHorse::
point_event(PointData *pt)
{
  NodeData *node, *new_node;

  node=m_front->locate_node(pt->x());
  new_node=new_front_triangle(pt, node);
  
  if(pt->x()<= node->m_point->x())
    {      
      fill(node);
    }

  fill_advancing_front(new_node);
  return new_node;
}


template<typename T, typename T_point_index>
typename WRATHTriangulation<T, T_point_index>::NodeData*
WRATHTriangulation<T, T_point_index>::WorkHorse::
new_front_triangle(PointData *pt, 
                   NodeData *node)
{
  TriangleData *new_triangle;
  NodeData *new_node;

  new_triangle=WRATHNew TriangleData(pt, node->m_point, node->m_next->m_point);
  m_all_triangles.push_back(new_triangle);
  new_triangle->mark_neighbor_data(node->m_triangle);

  new_node=WRATHNew NodeData(pt);
  m_nodes.push_back(new_node);

  new_node->m_next=node->m_next;
  new_node->m_prev=node;
  node->m_next->m_prev=new_node;
  node->m_next=new_node;

  if(!legalize(new_triangle))
    {
      map_triangle_to_nodes(new_triangle);
    }

  return new_node;
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
map_triangle_to_nodes(WRATHTriangulation<T, T_point_index>::TriangleData *t)
{
  WRATHassert(t!=NULL);
  for(int i=0;i<3;++i)
    {
      if(t->neighbor(i)==NULL)
        {
          NodeData *n;
          
          n=m_front->locate_point(t->point_cw(t->pt(i)));
          if(n!=NULL)
            {
              n->m_triangle=t;
            }
        }
    }
}

template<typename T, typename T_point_index>
bool
WRATHTriangulation<T, T_point_index>::WorkHorse::
legalize(TriangleData *t)
{
  for(int i=0; i<3; ++i)
    {
      if(t->m_is_delaunay_edge[i])
        continue;

      TriangleData *ot;

      ot=t->neighbor(i);
      if(ot!=NULL)
        {
          PointData *p, *op;
          int oi;
          bool inside;

          p=t->pt(i);
          op=ot->opposite_point(t, p);
          oi=ot->pt_index(op);
         
          if(ot->m_is_constrained_edge[oi] or ot->m_is_delaunay_edge[oi]) 
            {
              t->m_is_constrained_edge[i]=ot->m_is_constrained_edge[oi];
              continue;
            }

          inside=InCircle(p, t->point_ccw(p), t->point_cw(p), op);
          if(inside)
            {
              t->m_is_delaunay_edge[i]=true;
              ot->m_is_delaunay_edge[oi]=true;
              rotate_triangle_pair(t, p, ot, op);
            

              if(!legalize(t))
                {
                  map_triangle_to_nodes(t);
                }
              
              if(!legalize(ot))
                {
                  map_triangle_to_nodes(ot);
                }
              
              t->m_is_delaunay_edge[i]=false;
              ot->m_is_delaunay_edge[oi]=false;
              return true;
            }
 
        }
    }
  return false;
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
rotate_triangle_pair(TriangleData *t, PointData *p,
                     TriangleData *ot, PointData *op)
{
  TriangleData *n1, *n2, *n3, *n4;
  
  n1=t->neighbor_ccw(p);
  n2=t->neighbor_cw(p);
  n3=ot->neighbor_ccw(op);
  n4=ot->neighbor_cw(op);

  bool ce1, ce2, ce3, ce4;
  ce1=t->m_is_constrained_edge.edge_ccw(p);
  ce2=t->m_is_constrained_edge.edge_cw(p);
  ce3=ot->m_is_constrained_edge.edge_ccw(op);
  ce4=ot->m_is_constrained_edge.edge_cw(op);

  bool de1, de2, de3, de4;
  de1=t->m_is_delaunay_edge.edge_ccw(p);
  de2=t->m_is_delaunay_edge.edge_cw(p);
  de3=ot->m_is_delaunay_edge.edge_ccw(op);
  de4=ot->m_is_delaunay_edge.edge_cw(op);

  t->legalize(p, op);
  ot->legalize(op, p);

  ot->m_is_delaunay_edge.edge_ccw(p, de1);
  t->m_is_delaunay_edge.edge_cw(p, de2);
  t->m_is_delaunay_edge.edge_ccw(op, de3);
  ot->m_is_delaunay_edge.edge_cw(op, de4);

  
  ot->m_is_constrained_edge.edge_ccw(p, ce1);
  t->m_is_constrained_edge.edge_cw(p, ce2);
  t->m_is_constrained_edge.edge_ccw(op, ce3);
  ot->m_is_constrained_edge.edge_cw(op, ce4);

  t->clear_neighbors();
  ot->clear_neighbors();
  if (n1!=NULL) ot->mark_neighbor_data(n1);
  if (n2!=NULL) t->mark_neighbor_data(n2);
  if (n3!=NULL) t->mark_neighbor_data(n3);
  if (n4!=NULL) ot->mark_neighbor_data(n4);

  t->mark_neighbor_data(ot);
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
fill(NodeData *node)
{
  TriangleData *t;


  WRATHassert(node!=NULL);
  WRATHassert(node->m_next!=NULL);
  WRATHassert(node->m_prev!=NULL);

  t=WRATHNew TriangleData(node->m_prev->m_point, node->m_point, node->m_next->m_point);
  m_all_triangles.push_back(t);

  t->mark_neighbor_data(node->m_prev->m_triangle);
  t->mark_neighbor_data(node->m_triangle);

  node->m_prev->m_next=node->m_next;
  node->m_next->m_prev=node->m_prev;

  if(!legalize(t))
    {
      map_triangle_to_nodes(t);
    }
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
fill_advancing_front(NodeData *n)
{
  NodeData *node;

  WRATHassert(n!=NULL);
  node=n->m_next;


  while(node->m_next!=NULL)
    {
      if(node->AbsoluteValueOfNodeAngleGreaterThanHalfPI()) 
        break;
    
      fill(node);
      node=node->m_next;
    }


  node=n->m_prev;
  while(node->m_prev!=NULL)
    {
      if(node->AbsoluteValueOfNodeAngleGreaterThanHalfPI()) 
        break;
    
      fill(node);
      node=node->m_prev;
    }


  if(n->m_next!=NULL and n->m_next->m_next!=NULL
     and n->BasinAngleLessThan3PIOver4())
    {
      fill_basin(n);
    }


}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
fill_basin(NodeData *node)
{
  WRATHassert(node!=NULL);
  WRATHassert(node->m_next!=NULL);
  WRATHassert(node->m_next->m_next!=NULL);

  if(orientation(node->m_point, node->m_next->m_point, node->m_next->m_next->m_point)== cw) 
    {
      m_basin.m_left_node=node->m_next->m_next;
    } 
  else 
    {
      m_basin.m_left_node=node->m_next;
    }

  // Find the bottom and right node
  m_basin.m_bottom_node=m_basin.m_left_node;
  while (m_basin.m_bottom_node->m_next!=NULL 
         and m_basin.m_bottom_node->m_point->y() >= m_basin.m_bottom_node->m_next->m_point->y()) 
    {
      m_basin.m_bottom_node=m_basin.m_bottom_node->m_next;
    }

  if(m_basin.m_bottom_node==m_basin.m_left_node) 
    {
      // No valid basin
      return;
    }

  m_basin.m_right_node=m_basin.m_bottom_node;
  while(m_basin.m_right_node->m_next!=NULL
        and m_basin.m_right_node->m_point->y() < m_basin.m_right_node->m_next->m_point->y()) 
    {
      m_basin.m_right_node=m_basin.m_right_node->m_next;
    }

  if(m_basin.m_right_node==m_basin.m_bottom_node) 
    {
      // No valid basins
      return;
    }
  
  m_basin.m_width = m_basin.m_right_node->m_point->x() - m_basin.m_left_node->m_point->x();
  m_basin.m_left_highest = m_basin.m_left_node->m_point->y() > m_basin.m_right_node->m_point->y();

  fill_basin_implement(m_basin.m_bottom_node);
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
fill_basin_implement(NodeData *node)
{
  WRATHassert(node!=NULL);

  if(is_shallow(node))
    {
      return;
    }

  fill(node);

  if (node->m_prev==m_basin.m_left_node 
      and node->m_next==m_basin.m_right_node) 
    {
      return;
    } 
  else if (node->m_prev==m_basin.m_left_node) 
    {
      enum triangle_orientation o;

      WRATHassert(node->m_next!=NULL);
      WRATHassert(node->m_next->m_next!=NULL);

      o=orientation(node->m_point, node->m_next->m_point, node->m_next->m_next->m_point);
      if(o==cw) 
        {
          return;
        }
      node=node->m_next;
    } 
  else if(node->m_next==m_basin.m_right_node) 
    {
      enum triangle_orientation o;
      
      WRATHassert(node->m_prev!=NULL);
      WRATHassert(node->m_prev->m_prev!=NULL);

      o=orientation(node->m_point, node->m_prev->m_point, node->m_prev->m_prev->m_point);
      if (o==ccw) 
        {
          return;
        }
    node=node->m_prev;
    } 
  else 
    {
      WRATHassert(node->m_prev!=NULL);
      WRATHassert(node->m_next!=NULL);

      if(node->m_prev->m_point->y() < node->m_next->m_point->y()) 
        {
          node=node->m_prev;
        } 
      else
        {
          node=node->m_next;
        }
    }

  fill_basin_implement(node);
}

template<typename T, typename T_point_index>
bool
WRATHTriangulation<T, T_point_index>::WorkHorse::
is_shallow(NodeData *node)
{
  WRATHassert(node!=NULL);

  T height;  

  if(m_basin.m_left_highest) 
    {
      WRATHassert(m_basin.m_left_node!=NULL);
      height=m_basin.m_left_node->m_point->y() - node->m_point->y();
    } 
  else 
    {
      WRATHassert(m_basin.m_right_node!=NULL);
      height=m_basin.m_right_node->m_point->y() - node->m_point->y();
    }

  if(m_basin.m_width > height) 
    {
      return true;
    }
  return false;
}


template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
edge_event(EdgeData *edge, WRATHTriangulation<T, T_point_index>::NodeData *node)
{
  WRATHassert(edge!=NULL);
  WRATHassert(node!=NULL);

  m_edge_event.m_constrained_edge=edge;
  m_edge_event.m_right=(edge->p()->x() > edge->q()->x());

  if(is_edge_side_of_triangle(node->m_triangle, edge->p(), edge->q())) 
    {
      return;
    }
  
  fill_edge_event(edge, node);
  edge_event(edge->p(), edge->q(), node->m_triangle, edge->q());
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
edge_event(PointData *ep, 
           PointData *eq, 
           TriangleData *triangle, 
           PointData *point)
{
  if(is_edge_side_of_triangle(triangle, ep, eq)) 
    {
      return;
    }

  WRATHassert(triangle!=NULL);

  PointData *p1(triangle->point_ccw(point));
  enum triangle_orientation o1(orientation(eq, p1, ep));
  if (o1==colinear) 
    {
      //std::cerr << "Colinear detected at " << __FILE__ << ":" << __LINE__ << "\n";
      throw WRATHTriangulationPrivate::TriangulationException();
    }

  PointData *p2(triangle->point_cw(point));
  enum triangle_orientation o2(orientation(eq, p2, ep));
  if (o2==colinear) 
    {
      //std::cerr << "Colinear detected at " << __FILE__ << ":" << __LINE__ << "\n";
      throw WRATHTriangulationPrivate::TriangulationException();      
      return;      
    }

  if(o1==o2) 
    {
      if(o1==cw) 
        {
          triangle=triangle->neighbor_ccw(point);
        }       
      else
        {
          triangle=triangle->neighbor_cw(point);
        }
      edge_event(ep, eq, triangle, point);
    } 
  else
    {
      flip_edge_event(ep, eq, triangle, point);
    }
}


template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
flip_edge_event(PointData *ep, 
                PointData *eq, 
                TriangleData *t, 
                PointData *p)
{
  TriangleData *ot(t->neighbor_across(p));

  if(ot==NULL)
    {
      //std::cerr << "\nFlip failed from missing triangle"
      //        << __FILE__ << ":" << __LINE__ 
      //        << "\n\n";
      throw WRATHTriangulationPrivate::TriangulationException();
      return;
    }
  
  PointData *op(ot->opposite_point(t, p));


  if (InScanArea(p, t->point_ccw(p), t->point_cw(p), op)) 
    {
      // Lets rotate shared edge one vertex CW
      rotate_triangle_pair(t, p, ot, op);
      map_triangle_to_nodes(t);
      map_triangle_to_nodes(ot);
      
      if (p==eq and op==ep) 
        {
          if (eq==m_edge_event.m_constrained_edge->q() 
              and ep==m_edge_event.m_constrained_edge->p())
            {
              t->m_is_constrained_edge.mark_edge(ep, eq);
              ot->m_is_constrained_edge.mark_edge(ep, eq);
              legalize(t);
              legalize(ot);
            } 
          else 
            {
              // XXX: I think one of the triangles should be legalized here?
            }
        } 
      else 
        {
          enum triangle_orientation o(orientation(eq, op, ep));
          t=next_flip_triangle(o, t, ot, p, op);
          flip_edge_event(ep, eq, t, p);
        }
    } 
  else 
    {
      PointData *newP(next_flip_point(ep, eq, ot, op));
      flip_scan_edge_event(ep, eq, t, ot, newP);
      edge_event(ep, eq, t, p);
    }
}


template<typename T, typename T_point_index>
typename WRATHTriangulation<T, T_point_index>::TriangleData*
WRATHTriangulation<T, T_point_index>::WorkHorse::
next_flip_triangle(enum triangle_orientation o, 
                   TriangleData *t, 
                   TriangleData *ot,
                   PointData *p, 
                   PointData *op)
{
  if(o==ccw) 
    {
      // ot is not crossing edge after flip
      int edge_index(ot->edge_index(p, op));
      ot->m_is_delaunay_edge[edge_index]=true;
      legalize(ot);
      ot->clear_delaunay_edges();
      return t;
    }

  // t is not crossing edge after flip
  int edge_index(t->edge_index(p, op));
  
  t->m_is_delaunay_edge[edge_index]=true;
  legalize(t);
  t->clear_delaunay_edges();
  return ot;
}

template<typename T, typename T_point_index>
typename WRATHTriangulation<T, T_point_index>::PointData*
WRATHTriangulation<T, T_point_index>::WorkHorse::
next_flip_point(PointData *ep, 
                PointData *eq, 
                TriangleData *ot, 
                PointData *op)
{
  enum triangle_orientation o2d(orientation(eq, op, ep));
  if(o2d==cw) 
    {
      // Right
      return ot->point_ccw(op);
    } 
  else if(o2d==ccw) 
    {
      // Left
      return ot->point_cw(op);
    } 
  else
    {
      //std::cerr << "\n[Unsupported] Opposing point along constrained edge "
      //        << __FILE__ << ":" << __LINE__ << "\n";
      throw WRATHTriangulationPrivate::TriangulationException();
      return ot->point_ccw(op);
    }
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
flip_scan_edge_event(PointData *ep, 
                     PointData *eq, 
                     TriangleData *flip_triangle, 
                     TriangleData *t, 
                     PointData *p)
{
  TriangleData *ot(t->neighbor_across(p));
  PointData *op(ot->opposite_point(t, p));

  if(ot==NULL)
    {
      throw WRATHTriangulationPrivate::TriangulationException();
    }

  if(InScanArea(eq, flip_triangle->point_ccw(eq), flip_triangle->point_cw(eq), op)) 
    {
      flip_edge_event(eq, op, ot, op);
    } 
  else
    {
      PointData *newP(next_flip_point(ep, eq, ot, op));
      flip_scan_edge_event(ep, eq, flip_triangle, ot, newP);
    }
}

template<typename T, typename T_point_index>
bool
WRATHTriangulation<T, T_point_index>::WorkHorse::
is_edge_side_of_triangle(TriangleData *triangle, 
                         PointData *ep, PointData *eq)
{
  int eindex;

  WRATHassert(triangle!=NULL);
  WRATHassert(ep!=NULL);
  WRATHassert(eq!=NULL);

  eindex=triangle->edge_index(ep, eq);
  if(eindex!=-1)
    {
      TriangleData *t(triangle->neighbor(eindex));

      triangle->m_is_constrained_edge[eindex]=true;
      if(t!=NULL)
        {
          t->m_is_constrained_edge.mark_edge(ep, eq);
        }
      return true;
    }
  return false;
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
fill_edge_event(EdgeData *edge, NodeData *node)
{
  if(m_edge_event.m_right) 
    {
      fill_right_above_edge_event(edge, node);
    } 
  else 
    {
      fill_left_above_edge_event(edge, node);
    }
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
fill_right_above_edge_event(EdgeData *edge, NodeData *node)
{
  WRATHassert(node!=NULL);
  WRATHassert(node->m_next!=NULL);
  WRATHassert(edge!=NULL);

  while(node->m_next->m_point->x() < edge->p()->x()) 
    {
      if(orientation(edge->q(), node->m_next->m_point, edge->p())==ccw) 
        {
          fill_right_below_edge_event(edge, node);
        } 
      else
        {
          node=node->m_next;
        }
    }
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
fill_right_below_edge_event(EdgeData *edge, NodeData *node)
{
  WRATHassert(node!=NULL);
  WRATHassert(edge!=NULL);

  if(node->m_point->x() < edge->p()->x()) 
    {
      WRATHassert(node->m_next!=NULL);
      WRATHassert(node->m_next->m_next!=NULL);

      if(orientation(node->m_point, 
                     node->m_next->m_point, 
                     node->m_next->m_next->m_point)==ccw) 
        {
          fill_right_concave_edge_event(edge, node);
        } 
      else
        {
          fill_right_convex_edge_event(edge, node);
          fill_right_below_edge_event(edge, node);
        }
    }
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
fill_right_concave_edge_event(EdgeData *edge, NodeData *node)
{
  WRATHassert(node!=NULL);
  WRATHassert(edge!=NULL);

  fill(node->m_next);
  WRATHassert(node->m_next!=NULL);

  if(node->m_next->m_point!=edge->p()) 
    {
      // Next above or below edge?
      if(orientation(edge->q(), 
                     node->m_next->m_point, 
                     edge->p())==ccw)
        {
          if (orientation(node->m_point, 
                          node->m_next->m_point, 
                          node->m_next->m_next->m_point)==ccw)
            {
              // Next is concave
              fill_right_concave_edge_event(edge, node);
            } 
          else 
            {
              // Next is convex
            }
        }
    }
}


template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
fill_right_convex_edge_event(EdgeData *edge, NodeData *node)
{
  WRATHassert(node!=NULL);
  WRATHassert(edge!=NULL);
  WRATHassert(node->m_next!=NULL);
  WRATHassert(node->m_next->m_next!=NULL);
  WRATHassert(node->m_next->m_next->m_next!=NULL);

  // Next concave or convex?
  if (orientation(node->m_next->m_point, 
                  node->m_next->m_next->m_point, 
                  node->m_next->m_next->m_next->m_point)==ccw) 
    {
      // Concave
      fill_right_concave_edge_event(edge, node->m_next);
    } 
  else
    {
      // Convex
      // Next above or below edge?
      if (orientation(edge->q(), 
                      node->m_next->m_next->m_point, edge->p())==ccw) 
        {
          // Below
          fill_right_convex_edge_event(edge, node->m_next);
        } 
      else
        {
          // Above
        }
    }
}


template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
fill_left_above_edge_event(EdgeData *edge, NodeData *node)
{
  WRATHassert(node!=NULL);
  WRATHassert(edge!=NULL);
  WRATHassert(node->m_prev!=NULL);

  while (node->m_prev->m_point->x() > edge->p()->x()) 
    {
      // Check if next node is below the edge
      if(orientation(edge->q(), node->m_prev->m_point, edge->p())==cw) 
        {
          fill_left_below_edge_event(edge, node);
        } 
      else 
        {
          node=node->m_prev;
        }
    }
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
fill_left_below_edge_event(EdgeData *edge, NodeData *node)
{
  WRATHassert(node!=NULL);
  WRATHassert(edge!=NULL);
  WRATHassert(node->m_prev!=NULL);
  WRATHassert(node->m_prev->m_prev!=NULL);

  if(node->m_point->x() > edge->p()->x()) 
    {
      if(orientation(node->m_point, 
                     node->m_prev->m_point, 
                     node->m_prev->m_prev->m_point)==cw) 
        {
          // Concave
          fill_left_concave_edge_event(edge, node);
        } 
      else 
        {
          // Convex
          fill_left_convex_edge_event(edge, node);
          // Retry this one
          fill_left_below_edge_event(edge, node);
        }
    }
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
fill_left_convex_edge_event(EdgeData *edge, NodeData *node)
{
  WRATHassert(node!=NULL);
  WRATHassert(edge!=NULL);
  WRATHassert(node->m_prev!=NULL);
  WRATHassert(node->m_prev->m_prev!=NULL);
  WRATHassert(node->m_prev->m_prev->m_prev!=NULL);

  // Next concave or convex?
  if(orientation(node->m_prev->m_point, 
                 node->m_prev->m_prev->m_point, 
                 node->m_prev->m_prev->m_prev->m_point)==cw) 
    {
      // Concave
      fill_left_concave_edge_event(edge, node->m_prev);
    } 
  else
    {
      // Convex
      // Next above or below edge?
      if (orientation(edge->q(), 
                      node->m_prev->m_prev->m_point, 
                      edge->p())==cw) 
        {
          // Below
          fill_left_convex_edge_event(edge, node->m_prev);
        } 
      else
        {
          // Above
        }
    }
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
fill_left_concave_edge_event(EdgeData *edge, NodeData *node)
{
  WRATHassert(node!=NULL);
  WRATHassert(edge!=NULL);

  fill(node->m_prev);
  WRATHassert(node->m_prev!=NULL);

  if (node->m_prev->m_point!=edge->p()) 
    {
      // Next above or below edge?
      if (orientation(edge->q(), node->m_prev->m_point, edge->p())==cw) 
        {
          WRATHassert(node->m_prev->m_prev!=NULL);

          // Below
          if (orientation(node->m_point, node->m_prev->m_point, node->m_prev->m_prev->m_point)==cw) 
            {
              // Next is concave
              fill_left_concave_edge_event(edge, node);
            } 
          else
            {
              // Next is convex
            }
        }
    }
}


////////////////////////////////////////////
// WRATHTriangulation<T, T_point_index> methods
template<typename T, typename T_point_index>
WRATHTriangulation<T, T_point_index>::
WRATHTriangulation(void):
  m_work_horse(NULL)
{
  
}

template<typename T, typename T_point_index>
WRATHTriangulation<T, T_point_index>::
~WRATHTriangulation(void)
{
  if(m_work_horse!=NULL)
    {
      WRATHDelete(m_work_horse);
    }
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::
clear(void)
{
  if(m_work_horse!=NULL)
    {
      WRATHDelete(m_work_horse);
      m_work_horse=NULL;
    }
  m_raw_pt_map.clear();
  m_raw_points.clear();
  m_outlines.clear();
  m_contraint_edges.clear();
}


template<typename T, typename T_point_index>
bool
WRATHTriangulation<T, T_point_index>::
point_triangulation_fail(void)
{
  create_work_horse_if_necessary();
  return m_work_horse->point_triangulation_fail();
}


template<typename T, typename T_point_index>
bool
WRATHTriangulation<T, T_point_index>::
connected_component_computation_fail(void)
{
  create_work_horse_if_necessary();
  return m_work_horse->connected_component_computation_fail();
}

template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::
create_work_horse_if_necessary(void)
{
  if(m_work_horse==NULL)
    {
      m_work_horse=WRATHNew WorkHorse(m_raw_points, m_outlines, m_contraint_edges);

      m_connected_components.clear();
      m_even_odd_rule_triangulation.clear();
      m_winding_rule_triangulation.clear();

      for(typename std::vector<ConnectedComponent*>::const_iterator 
            iter=m_work_horse->connected_componenents().begin(),
            end=m_work_horse->connected_componenents().end();
          iter!=end; ++iter)
        {
          ConnectedComponent *C(*iter);
          
          WRATHassert(C!=NULL);
          
          if(!C->is_induced_component())
            {
              C->m_ID=m_connected_components.size();
              m_connected_components.push_back(TriangulatedComponent(C));
              if(std::abs(C->winding_number())%2==1)
                {
                  C->add_trianglulation(m_even_odd_rule_triangulation);
                }
          
              if(C->winding_number()!=0)
                {
                  C->add_trianglulation(m_winding_rule_triangulation);
                }
            }
            
        }
      
    }
}

template<typename T, typename T_point_index>
const std::vector<typename WRATHTriangulation<T, T_point_index>::TriangulatedComponent>&
WRATHTriangulation<T, T_point_index>::
connected_components(void) 
{
  create_work_horse_if_necessary();
  return m_connected_components;
}





template<typename T, typename T_point_index>
bool
WRATHTriangulation<T, T_point_index>::NodeData::
AbsoluteValueOfNodeAngleGreaterThanHalfPI(void)
{

  product_type c;
  product_type ax, ay, bx, by;

  WRATHassert(m_next!=NULL);
  WRATHassert(m_prev!=NULL);

  ax=m_next->m_point->x() - m_point->x();
  ay=m_next->m_point->y() - m_point->y();
  bx=m_prev->m_point->x() - m_point->x();
  by=m_prev->m_point->y() - m_point->y();
  

  c=ax*bx + ay*by;
  return c<product_type(0);
}

template<typename T, typename T_point_index>
bool
WRATHTriangulation<T, T_point_index>::NodeData::
BasinAngleLessThan3PIOver4(void)
{
  point a;
  
  WRATHassert(m_next!=NULL);
  WRATHassert(m_next->m_next!=NULL);
  

  a=m_point->pt() - m_next->m_next->m_point->pt();
  return a.y()<=T(0) or a.x()>=-a.y();
}

template<typename T, typename T_point_index>
bool
WRATHTriangulation<T, T_point_index>::
InScanArea(PointData *pA, 
           PointData *pB, 
           PointData *pC, 
           PointData *pD)
{

  WRATHassert(pA!=NULL);
  WRATHassert(pB!=NULL);
  WRATHassert(pC!=NULL);
  WRATHassert(pD!=NULL);
  

  point d(pD->pt());
  point a(pA->pt() - d); 
  point b(pB->pt() - d);

  product_type axby, bxay;

  axby=product_type(a.x()) * product_type(b.y());
  bxay=product_type(a.y()) * product_type(b.x());
  
  if(axby<bxay)
    {
      return false;
    }
  
  point c(pC->pt() - d);
  product_type cxay, axcy;

  cxay=product_type(c.x()) * product_type(a.y());
  axcy=product_type(a.x()) * product_type(c.y());
  
  if(cxay<axcy)
    {
      return false;
    }

  return true;

}


template<typename T, typename T_point_index>
enum WRATHTriangulation<T, T_point_index>::triangle_orientation
WRATHTriangulation<T, T_point_index>::
orientation(PointData *pA, 
            PointData *pB, 
            PointData *pC)
{
  product_type L, R;

  WRATHassert(pA!=NULL);
  WRATHassert(pB!=NULL);
  WRATHassert(pC!=NULL);

  L=product_type(pA->x() - pC->x()) * product_type(pB->y() - pC->y());
  R=product_type(pA->y() - pC->y()) * product_type(pB->x() - pC->x());

  if(L==R)
    {
      
      return colinear;
    }

  if(L>R)
    {
      return ccw;
    }

  return cw;
}



template<typename T, typename T_point_index>
bool
WRATHTriangulation<T, T_point_index>::PointData::
compare_pts(PointData *p, PointData *q)
{
  WRATHassert(p!=NULL);
  WRATHassert(q!=NULL);
  
  //compare first by y then by x.
  point a(p->pt()), b(q->pt());

  //TODO: use a fuzzy compare possible instead when T
  //is a floating point type...
  return (a.y()==b.y())?
    a.x()<b.x():
    a.y()<b.y();
}



template<typename T, typename T_point_index>
bool
WRATHTriangulation<T, T_point_index>::
InCircle(PointData *pA, 
         PointData *pB, 
         PointData *pC, 
         PointData *pD)
{
  product_type adx=pA->x()-pD->x();
  product_type ady=pA->y()-pD->y();
  product_type bdx=pB->x()-pD->x();
  product_type bdy=pB->y()-pD->y();

  product_type adxbdy=adx*bdy;
  product_type bdxady=bdx*ady;
  product_type oabd=adxbdy-bdxady;

  if(oabd<=product_type(0))
    {
      return false;
    }

  product_type cdx=pC->x()-pD->x();
  product_type cdy=pC->y()-pD->y();

  product_type cdxady=cdx*ady;
  product_type adxcdy=adx*cdy;
  product_type ocad=cdxady-adxcdy;

  if(ocad<=product_type(0))
    {
      return false;
    }

  product_type bdxcdy=bdx*cdy;
  product_type cdxbdy=cdx*bdy;

  product_type alift=adx*adx + ady*ady;
  product_type blift=bdx*bdx + bdy*bdy;
  product_type clift=cdx*cdx + cdy*cdy;
  product_type ooo=bdxcdy-cdxbdy;

  //not a good situation: coordinate's are 32bit integers
  //thus alift, blift, clift are potentially 64bits as
  //are adxbdy, bdxady, bdxcdy and cdxbdy. Thus det is 
  //potentially a 128 bit integer, we get out of this
  //via that product_product_type is not an elementary type.
  product_product_type det;

  det.add_product(alift, ooo);
  det.add_product(blift, ocad);
  det.add_product(clift, oabd);

  return det.is_positive();

}


template<typename T, typename T_point_index>
void
WRATHTriangulation<T, T_point_index>::WorkHorse::
initialize(const std::vector<input_point> &pts,
           const std::vector< std::vector<point_index> > &outlines,
           const std::vector< vecN<point_index, 2> > &extra_edges)
{
  init_points(pts);
  init_edges(outlines, extra_edges);

  point max_pt(pts[0].m_position), min_pt(pts[0].m_position);

  for(unsigned int i=1, lasti=pts.size(); i<lasti; ++i)
    {
      max_pt.x()=std::max( pts[i].m_position.x(), max_pt.x());
      max_pt.y()=std::max( pts[i].m_position.y(), max_pt.y());

      min_pt.x()=std::min( pts[i].m_position.x(), min_pt.x());
      min_pt.y()=std::min( pts[i].m_position.y(), min_pt.y());
    }

  /*
    Essentially: rd=(max-min)/2,
    but as T may be an integer type
    we do some futzing.

    original poly2tri code used
    rd=0.3*(max-min).
   */

  vecN<T,2> rd;

  rd=(max_pt - min_pt)/T(2) + point( T(1), T(1));
  
  m_head=WRATHNew PointData( point(max_pt.x() + rd.x(), min_pt.y() - rd.y()) );
  m_tail=WRATHNew PointData( point(min_pt.x() - rd.x(), min_pt.y() - rd.y()) );
  std::sort(m_points.begin(), m_points.end(), PointData::compare_pts);
}


