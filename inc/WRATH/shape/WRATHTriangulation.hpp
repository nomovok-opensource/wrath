/*! \file WRATHTriangulation.hpp */
/*
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
#ifndef __WRATH_TRIANGULATION_HPP__
#define __WRATH_TRIANGULATION_HPP__

#include "WRATHConfig.hpp"
#include <vector>
#include <set>
#include <list>
#include <map>
#include <stdint.h>
#include <iterator>
#include <boost/utility.hpp>
#include "WRATHNew.hpp"
#include "vectorGL.hpp"
#include "WRATHTriangulationTypes.tcc"
#include "c_array.hpp"

/*! \addtogroup Shape
 * @{
 */


/*!\class WRATHTriangulation
  A WRATHTriangulation triangulates a set of points
  subject to user defined edge conditions. The API is iterator
  centric to allow for more generic use. The algorithm,
  code outline (and code itself) is based closely
  upon poly2tri, found at http://code.google.com/p/poly2tri/.
  However, in contrast to poly2tri, WRATHTriangulation supports:
  - integer types natively, i.e. using integer types for points removed any round off error
  - decomposition of triangulation into connected components, each of which has a winding number computed
  - user defined edge conditions can be supplied via closed outline (which affect computation of winding numbers) or via seperate edges (which will NOT affect computation of winding numbers)
  

  WRATHTriangulation requires that no two edges intersect
  except at their end points. Additionally, no two edges
  my overlap either. If both of these conditions are not
  met, WRATHTriangulation will either fail in creating
  the triangulation or create an invalid triangulation.

  The type T for the coordinates of the points may be one of
  - int8_t
  - int16_t
  - int32_t
  - float
  - double
  - any "floating" point type (for example double128 if such a thing exists).

  Other integer types besides int8_t, int16_t and int32_t are
  NOT supported and will likely triangulate incorrectly.

  The type T_point_index is the type of user indices,
  the default value is uint16_t (i.e. GL_UNSINGEND_SHORT).
 */
template<typename T, typename T_point_index=uint16_t>
class WRATHTriangulation:public boost::noncopyable
{
private:
  ///@cond
  class ConnectedComponent;
  ///@endcond

public:

  /*!\typedef point
    Points processed by a WRATHTriangulation.
   */
  typedef vecN<T, 2> point;

  /*!\typedef point_index
    Index type for triangulation and edge specification
   */
  typedef T_point_index point_index;

  /*!\class TriangulatedComponent
    A TriangulatedComponent represents 
    one connected component of a triangulation
    of points and edges. This class is essentially
    just a handle, as such copying it is not an issue.
   */ 
  class TriangulatedComponent
  {
  public:
    /*!\class edge_data
      An edge_data holds the data about an edge
      of a TriangulatedComponent. 
     */
    class edge_data
    {
    public:
      /*!\var m_edge_indices
        Gives the user specified indices that 
        make up the edge.
       */
      vecN<point_index,2> m_edge_indices;

      /*!\var m_interior_index
        Gives the user specified index
        of the "3rd" point of the triangle
        from which this edge originates
       */
      point_index m_interior_index;

      /*!\var m_neighbor_component
        Gives the TriangulatedComponent on
        the "other" side of the edge. If
        there is no component on the other
        side, then \ref m_neighbor_component
        is an invalid handle.
       */
      TriangulatedComponent m_neighbor_component;
    };

    /*!\fn bool valid
      Returns true if and only if this
      TriangulatedComponent refers to an
      actual connected component of a
      WRATHTriangulation.
     */
    bool
    valid(void) const
    {
      return m_data!=NULL;
    }

    /*!\fn int winding_number
      Returns the winding number of the
      triangulation of this connected
      component.
     */
    int 
    winding_number(void) const;
    
    /*!\fn triangulation
      Returns the triangulation for the component as
      an array of _user_ defined indices as dictated
      by \ref add_outline(), \ref add_edge(),
      \ref add_edges(), \ref add_point() and 
      \ref add_points().
     */
    const std::vector<point_index>&
    triangulation(void) const;

    /*!\fn const std::vector<edge_data>& edges
      Returns the list of edges of the boundary of the
      TriangulatedComponent as an array of pairs of 
      _user_ defined indices as dictated by \ref add_outline(), 
      \ref add_point() and \ref add_points(). 
     */
    const std::vector<edge_data>&
    edges(void) const;

    /*!\fn const std::vector<ConnectedComponent>& edge_neighbors
      Returns the edge neighbor data for _each_
      triangle of the TriangulatedComponent.

      The array returned by triangulation() is a list
      of indices for triangles. The I'th triangle
      is:

      T(i)=[ a(i), b(i), c(i) ]

      where
      - a(i)=triangulation()[3*i], 
      - b(i)=triangulation()[3*i+1]
      - c(i)=triangulation()[3*i+2]
      
      Let 
      - A denote the edge of T(i) opposite a(i)
      - B denote the edge of T(i) opposite b(i)
      - C denote the edge of T(i) opposite c(i)
      
      For each edge, the otherside of an edge is either
      another triangle or nothing. The return value
      of edge_neighbors()[3*i] (for example) returns
      the TriangulatedComponent for that neighbor triangle
      for the edge oppositve a(i). If there is no triangle
      then the TriangulatedComponent is not valid.
     */
    const std::vector<ConnectedComponent>&
    edge_neighbors(void) const;

    /*!\fn ID
      Each TriangulatedComponent has a unique
      ID (relative to a WRATHTriangulation),
      returns that ID if this is a valid
      TriangulatedComponent and -1 otherwise.
      The value ID() satistifes that
      WRATHTriangulation::connected_components()[I].ID()==I
      whenever 0 <= I < WRATHTriangulation::connected_components().size().
     */
    int
    ID(void) const
    {
      return (valid())?
        m_data->m_ID:
        -1;
    }

    /*!\fn TriangulatedComponent
      Public ctor creates a TriangulatedComponent
      whose valid() method returns false.
     */
    TriangulatedComponent(void):
      m_data(NULL)
    {}

  private:
    friend class WRATHTriangulation;

    explicit 
    TriangulatedComponent(const ConnectedComponent *pdata):
      m_data(pdata)
    {}

    const ConnectedComponent *m_data;
  };


  /*!\typedef edge_data
    Convenience typedef for TriangulatedComponent::edge_data
   */
  typedef typename TriangulatedComponent::edge_data edge_data;

  /*!\fn WRATHTriangulation
    Ctor, initializes as no triangulation ready
    and no points, outlines or edges added.
   */
  WRATHTriangulation(void);

  ~WRATHTriangulation();
   

  /*!\fn void add_outline
    Add an outline specified by a sequence 
    of points.
    Letting [begin,end) correspond to {x1,x2,..,xN},
    where N=std\::distance(begin,end), the points
    x1,x2,..,xN are added and the edges
    [x1,x2], [x2,x3], ..., [xN-1,xN], [xN,x1]
    are also added. Edges added by add_outline()
    *DO* affect the winding computation of
    connected components.

    \tparam iterator iterator type \ref point_index type
    \tparam Pfunctor functor type providing a \code 
                     point operator()(point_index) const
                     \endcode
                     method to produce a vertex from an
                     index
    \param begin iterator to 1st index of the outline to add
    \param end iterator to one past the last index of the outline to add
    \param P functor object to produce the position of the point from a user index.
   */
  template<typename iterator, typename Pfunctor>
  void
  add_outline(iterator begin, iterator end, Pfunctor P)
  {
    m_outlines.push_back( std::vector<point_index>() );

    for(;begin!=end;++begin)
      {
        point_index user_indx, indx;
        point pt;

        user_indx=*begin;
        pt=P(user_indx);

        indx=get_raw_point_index(pt, user_indx);
        m_outlines.back().push_back(indx);
      }

    if(m_work_horse!=NULL)
      {
        WRATHDelete(m_work_horse);
        m_work_horse=NULL;
      }
  }

  /*!\fn void add_point
    Add a single point to the triangulation.
    \param P coordinate of point
    \param I user index of the point.
   */
  void
  add_point(point P, point_index I)
  {
    if(add_point_implement(P, I) and m_work_horse!=NULL)
      {
        WRATHDelete(m_work_horse);
        m_work_horse=NULL;
      }
  }

  /*!\fn void add_edge
    Add an edge to appear in the triangulation.
    Edges added with add_edge() do NOT affect
    the computation of the winding number.
    However, edges added with add_edge() can
    affect connected component computation.
    Specifically, for computing connected
    components edges from add_edge() (and 
    \ref add_edges() ) have the same role
    as edges added with add_outline()
    in computing the connected components
    of a triangulation, even though they do
    not affect the winding number computation
    of any connected components.
    \param P0 coordinate of start point of edge
    \param I0 user index of start point of edge
    \param P1 coordinate of end point of edge
    \param I1 user index of end point of edge
   */
  void
  add_edge(point P0, point_index I0,
           point P1, point_index I1)
  {
    point_index A0, A1;

    A0=get_raw_point_index(P0, I0);
    A1=get_raw_point_index(P0, I1);

    if(m_work_horse!=NULL)
      {
        WRATHDelete(m_work_horse);
        m_work_horse=NULL;
      }
    
    m_contraint_edges.push_back(vecN<point_index,2>(A0,A1)); 
  }

  /*!\fn void add_edges
    Add a sequence of edges to appear in the triangulation.
    Edges added with add_edges() do NOT affect
    the computation of the winding number.
    However, edges added with add_edges() can
    affect connected component computation.
    Specifically, for computing connected
    components edges from add_edges() (and 
    \ref add_edge() ) have the same role
    as edges added with add_outline()
    in computing the connected components
    of a triangulation, even though they do
    not affect the winding number computation
    of any connected components.

    \tparam iterator iterator type an user edge type
    \tparam Ifunctor0 functor object providing the method \code
            point_index operator(iterator) const
            \endcode
            to produce the index of the starting point
            of an edge
    \tparam Ifunctor0 functor object providing the method \code
            point_index operator(iterator) const
            \endcode
            to produce the index of the ending point
            of an edge
    \tparam Pfunctor functor type providing a \code 
                     point operator()(point_index) const
                     \endcode
                     method to produce a vertex from an
                     index
    \param begin iterator to first edge to add
    \param end iterator one past the last edge to add
    \param I0 functor object that given an iterator produces
              the user index for the starting point of the edge
              refered to by the iterator
    \param I1 functor object that given an iterator produces
              the user index for the ending point of the edge
              refered to by the iterator
    \param P functor object that given
   */
  template<typename iterator, typename Ifunctor0, typename Ifunctor1, typename Pfunctor>
  void
  add_edges(iterator begin, iterator end, Ifunctor0 I0, Ifunctor1 I1, Pfunctor P)
  {
    for(;begin!=end; ++begin)
      {
        point_index user_idx0, user_idx1;
        point_index raw_idx0, raw_idx1;

        user_idx0=I0(begin);
        user_idx1=I1(begin);

        raw_idx0=get_raw_point_index( P(user_idx0), user_idx0);
        raw_idx1=get_raw_point_index( P(user_idx1), user_idx1);
        m_contraint_edges.push_back(vecN<point_index,2>(raw_idx0, raw_idx1)); 
      }
    
    if(m_work_horse!=NULL)
      {
        WRATHDelete(m_work_horse);
        m_work_horse=NULL;
      }
  }

  
  /*!\fn void add_points
    Add a set of points to the triangulation.
    \tparam Pfunctor functor type providing a \code 
                     point operator()(point_index) const
                     \endcode
                     method to produce a vertex from an
                     index
    \param begin iterator to 1st user-index of the points to add
    \param end iterator to one past the last user-index of the points to add
    \param P functor object to produce the position of the point from a _user_ index.
   */
  template<typename iterator, typename Pfunctor>
  void
  add_points(iterator begin, iterator end, Pfunctor P)
  {
    bool added_points(false);

    for(;begin!=end;++begin)
      {
        point_index user_indx, indx;
        point pt;
        bool added_pt;

        user_indx=*begin;
        pt=P(user_indx);

        added_pt=add_point_implement(pt, user_indx);
        added_points=added_points or added_pt;
      }

    if(added_points and m_work_horse!=NULL)
      {
        WRATHDelete(m_work_horse);
        m_work_horse=NULL;
      }
  }

  /*!\fn const std::vector<TriangulatedComponent>& connected_components
    Returns a reference to the TriangulatedComponent 's
    of the WRATHTriangulation. If an outline or point 
    has beed added since the last call to 
    connected_components() or if connected_components() 
    was never called, then it is computed. The objects of the
    traingulation are destroyed and invalidated whenever the
    triangulation is regenerated. The TriangulatedComponent is
    a _thin_ object, as such saving TriangulatedComponent
    between regeneration of triangulation is a recipe
    for crashing behavior.
   */
  const std::vector<TriangulatedComponent>&
  connected_components(void);

  /*!\fn const std::vector<point_index>& even_odd_rule_triangulation
    Returns the triangulation of all those connected
    components which would be filled with the odd-even
    fill rule. 
   */
  const std::vector<point_index>&
  even_odd_rule_triangulation(void)
  {
    create_work_horse_if_necessary();
    return m_even_odd_rule_triangulation;
  }

  /*!\fn const std::vector<point_index>& winding_rule_triangulation
    Returns the triangulation of all those connected
    components which would be filled with the non-zero
    winding fill rule. 
   */
  const std::vector<point_index>&
  winding_rule_triangulation(void)
  {
    create_work_horse_if_necessary();
    return m_winding_rule_triangulation;
  }

  /*!\fn bool point_triangulation_fail
    Returns true if the triangulation
    of the points together with the 
    edges aded via \ref add_outline()
    failed. The usual cause of failure
    is when an added edge intersects
    another added edge.
   */
  bool
  point_triangulation_fail(void);

  /*!\fn bool connected_component_computation_fail
    Returns true if and only if the
    computation of the connected components
    failed of if the triangulation failed
    (see \ref point_triangulation_fail()).
    Failure occurs if the added edges do
    not form closed outlines or if edges
    intersect edges.
   */
  bool
  connected_component_computation_fail(void);


  /*!\fn void clear
    Clears all points and outlines added 
    to the triangulation and resets the state,
    thus any existing objects of triangulation 
    are destroyed.
   */
  void
  clear(void);

private:

  /// @cond
  typedef typename WRATHTriangulationPrivate::DataType<T>::product_type product_type;
  typedef typename WRATHTriangulationPrivate::DataType<T>::product_product_type product_product_type;
    
  class PointData;
  class EdgeData;
  class TriangleData;
  class BasinData;
  class NodeData;
  class EdgeEvent;
  class AdvancingFrontData;
  class ConnectedComponent;

  
  class input_point
  {
  public:
    input_point(void)
    {}

    input_point(const point &pt,
                point_index i):
      m_position(pt),
      m_index(i)
    {}

    bool
    operator<(const input_point &rhs) const
    {
      if(m_index!=rhs.m_index)
        {
          return m_index<rhs.m_index;
        }
      return m_position<rhs.m_position;
    }

    friend 
    std::ostream&
    operator<<(std::ostream &ostr, const input_point &obj)
    {
      ostr << "[" << obj.m_position << ": " << obj.m_index << "]";
      return ostr;
    }

    point m_position;
    point_index m_index;
  };

  enum triangle_orientation
    {
      cw,
      ccw,
      colinear,
    };

  static
  bool
  InScanArea(PointData *pA, PointData *pB, PointData *pC, PointData *pD);

  static
  bool
  InCircle(PointData *pA, PointData *pB, PointData *pC, PointData *pD);

  static
  enum triangle_orientation
  orientation(PointData *pA, PointData *pB, PointData *pC);

  static
  bool
  AbsoluteValueOfNodeAngleGreaterThanHalfPI(NodeData*);

  static
  bool
  BasinAngleLessThan3PIOver4(NodeData*);

  

  /*
    Intermediate data and classes.
   */
  class PointData
  {
  public:
    
    
    PointData(const point &ppt, int L):
      m_pt(ppt),
      m_location(L),
      m_is_induced_point(false)
    {}

    explicit
    PointData(const point &ppt):
      m_pt(ppt),
      m_location(-1),
      m_is_induced_point(true)
    {}

    const point&
    pt(void)
    {
      return m_pt;
    }

    T 
    x(void)
    {
      return pt().x();
    }

    T
    y(void)
    {
      return pt().y();
    }

    point_index
    id(void)
    {
      WRATHassert(m_location>=0);
      return static_cast<point_index>(m_location);
    }

    bool
    is_induced_point(void) const
    {
      return m_is_induced_point;
    }

    bool
    real_point(void)
    {
      return !m_is_induced_point;
    }

    static
    bool
    compare_pts(PointData *p, PointData *q);

    std::vector<EdgeData*> m_edges;

  private:
    point m_pt;
    int m_location;
    bool m_is_induced_point;
  };

  enum edge_classification_type
    {
      edge_rising,
      edge_falling,
      edge_flat
    };

  
  /*!
    Points of edge are sorted first by y-coordinate
    then by x-coordinate.
   */
  class EdgeData
  {
  public:

    EdgeData(PointData *p1, PointData *p2);

    PointData*
    p(void) const
    {
      return m_p;
    }

    PointData*
    q(void) const
    {
      return m_q;
    }

    void
    set_previous_neighbor(EdgeData *pprev);

    
    /*
      sets the point edge-classifications
      for all edges of the outline of this.
     */
    void
    set_classifications(void)
    {
      set_classifications_implement(NULL);
    }

    int
    compute_winding_contribution(const vecN<product_type,2> &triangle_center_times3);

    static
    bool
    compare_edge_functor_sort_by_low_y(EdgeData *lhs, EdgeData *rhs)
    {
      WRATHassert(lhs!=NULL);
      WRATHassert(rhs!=NULL);
      return lhs->m_three_time_low_y < rhs->m_three_time_low_y;
    }

    static
    bool
    compare_edge_functor_sort_by_reverse_high_y(EdgeData *lhs, EdgeData *rhs)
    {
      WRATHassert(lhs!=NULL);
      WRATHassert(rhs!=NULL);
      return lhs->m_three_time_high_y > rhs->m_three_time_high_y;  
    }

    class hunt_low_y
    {
    public:

      bool
      operator()(const T &lhs, const T &rhs) const
      {
        return lhs<rhs;
      }

      bool
      operator()(EdgeData *lhs, const T &rhs) const
      {
        return operator()(lhs->m_three_time_low_y, rhs);
      }

      bool
      operator()(const T &lhs, EdgeData *rhs) const
      {
        return operator()(lhs, rhs->m_three_time_low_y);
      }

      bool
      operator()(EdgeData *lhs, EdgeData *rhs) const
      {
        return operator()(lhs->m_three_time_low_y,
                          rhs->m_three_time_low_y);
      }
    };

    class reverse_hunt_high_y
    {
    public:

      bool
      operator()(const T &lhs, const T &rhs) const
      {
        return lhs>rhs;
      }

      bool
      operator()(EdgeData *lhs, const T &rhs) const
      {
        return operator()(lhs->m_three_time_high_y, rhs);
      }

      bool
      operator()(const T &lhs, EdgeData *rhs) const
      {
        return operator()(lhs, rhs->m_three_time_high_y);
      }

      bool
      operator()(EdgeData *lhs, EdgeData *rhs) const
      {
        return operator()(lhs->m_three_time_high_y,
                          rhs->m_three_time_high_y);
      }
    };

  private:

    void
    set_classifications_implement(EdgeData*);
    
    EdgeData*
    next_neighbor(void)
    {
      return (m_reversed)?
        m_p_neighbor:
        m_q_neighbor;
    }

    EdgeData*
    prev_neighbor(void)
    {
      return (m_reversed)?
        m_q_neighbor:
        m_p_neighbor;
    }

    bool&
    next_classification(void)
    {
      return (m_reversed)?
        m_p_observe_intersection:
        m_q_observe_intersection;
    }

    bool&
    prev_classification(void)
    {
      return (m_reversed)?
        m_q_observe_intersection:
        m_p_observe_intersection;
    }

   
    void
    set_next_neighbor(EdgeData *pnext);
    
    PointData *m_p, *m_q;
    bool m_reversed;
    
    EdgeData *m_p_neighbor, *m_q_neighbor;
    enum edge_classification_type m_edge_classification;
    bool m_p_observe_intersection, m_q_observe_intersection;

    T m_three_time_low_y, m_three_time_high_y;

    vecN<product_type,2> m_twisted_edge_delta;
    product_type m_compute_offset;
    int m_edge_sign;
  };

  class edge_set:public vecN<bool,3>
  {
  public:
    explicit
    edge_set(vecN<PointData*,3> &pts):
      vecN<bool,3>(false, false, false),
      m_pts_ref(pts)
    {}
    
    void
    mark_edge(EdgeData*);
    
    void
    mark_edge(PointData*, PointData*);
    
    bool
    edge_cw(PointData*);
    
    bool
    edge_ccw(PointData*);
    
    void
    edge_cw(PointData*, bool);
    
    void
    edge_ccw(PointData*, bool);

    vecN<PointData*, 2>
    edge_source(int p) const;
    
  private:
    vecN<PointData*,3> &m_pts_ref;
    
  };

  class TriangleData
  {
  public:

    TriangleData(PointData *p0, PointData *p1, PointData *p2);

    PointData*
    pt(int i)
    {
      return m_points[i];
    }

    int
    pt_index(PointData*);

    void
    legalize(PointData*);

    void
    legalize(PointData *opoint, PointData *npoint);

    PointData*
    point_cw(PointData*);

    PointData*
    point_ccw(PointData*);

    PointData*
    opposite_point(TriangleData *t, PointData *p);


    bool
    has_point(PointData *p)
    {
      return m_points[0]==p
        or m_points[1]==p
        or m_points[2]==p;
    }

    bool
    has_points(PointData *p, PointData *q)
    {
      return has_point(p) and has_point(q);
    }

    int
    edge_index(PointData *p, PointData *q);

    inline
    int
    edge_index(EdgeData *e)
    {
      WRATHassert(e!=NULL);
      return edge_index(e->p(), e->q());
    }


    bool
    has_edge(PointData *p, PointData *q)
    {
      return has_point(p) and has_point(q);
    }


    void
    mark_neighbor_data(TriangleData *t);

    void
    clear_neighbors(void);

    TriangleData*
    neighbor_across(PointData*);

    TriangleData*
    neighbor_cw(PointData*);

    TriangleData*
    neighbor_ccw(PointData*);

    TriangleData*
    neighbor(int i)
    {
      return m_neighbors[i];
    }

    void
    clear_delaunay_edges(void)
    {
      m_is_delaunay_edge[0]=m_is_delaunay_edge[1]=m_is_delaunay_edge[2]=false;
    }

    ConnectedComponent*
    connected_component(void)
    {
      return m_component;
    }

    
    void
    component_marker(ConnectedComponent *c);

    bool
    is_induced_triangle(void) const;

  private:

    void
    mark_neighbor_data(PointData *p, PointData *q, TriangleData *t);

    
    vecN<PointData*, 3> m_points;
    vecN<TriangleData*, 3> m_neighbors;
    ConnectedComponent *m_component;
    bool m_is_induced_triangle;

  public:
    edge_set m_is_constrained_edge; //i.e. edge comes from an outline
    edge_set m_is_delaunay_edge; //i.e. edge comes from Delunay triangulation
  };

  class ConnectedComponent
  {
  public:

    ConnectedComponent(void):
      m_ID(-1),
      m_winding_number_computed(false),
      m_winding_number(0),
      m_is_induced_component(false)
    {}

    void
    add_triangle(TriangleData *p);


    const std::list<TriangleData*>&
    triangles(void) const
    {
      return m_triangles;
    }

    const std::vector<point_index>&
    triangle_commands(void) const
    {
      return m_triangle_commands;
    }

    int
    winding_number(void) const
    {
      return m_winding_number;
    }
    
    void
    compute_winding_number(const std::vector<EdgeData*> &edge_list_sorted_by_lowy,
                           const std::vector<EdgeData*> &edge_list_sorted_by_highy,
                           std::vector<EdgeData*> &work_room1,
                           std::vector<EdgeData*> &work_room2);

    void
    compute_winding_number(const std::vector<EdgeData*> &edge_list);

    void
    build_triangle_commands(const std::vector<input_point> &pts);

    bool 
    is_induced_component(void) const
    {
      return m_is_induced_component;
    }

    void
    add_trianglulation(std::vector<point_index> &plist) const;

    const std::vector<edge_data>&
    edges(void) const
    {
      return m_edges;
    }

    const std::vector<TriangulatedComponent>&
    edge_neighbors(void)
    {
      return m_edge_neighbors;
    }

    void
    compute_edge_neighbors(void);

    static
    int
    compute_winding_number(const vecN<product_type, 2> &triangle_center_times3,
                           const std::vector<EdgeData*> &edge_list_sorted_by_lowy,
                           const std::vector<EdgeData*> &edge_list_sorted_by_highy,
                           std::vector<EdgeData*> &work_room1,
                           std::vector<EdgeData*> &work_room2);

    static
    int
    compute_winding_number(const vecN<product_type, 2> &triangle_center_times3,
                           const std::vector<EdgeData*> &edge_list);

    static
    vecN<product_type, 2>
    compute_3times_triangle_center(TriangleData *tri)
    {
      point triangle_center_times3;

      WRATHassert(tri!=NULL);
      triangle_center_times3=tri->pt(0)->pt() + tri->pt(1)->pt() + tri->pt(2)->pt();
      return vecN<product_type, 2>(triangle_center_times3.x(),
                                   triangle_center_times3.y());
    }

    int m_ID;

  private:


    std::list<TriangleData*> m_triangles;
    std::list<TriangleData*> m_triangles_with_constrained_edges;

    std::vector<point_index> m_triangle_commands;
    std::vector<edge_data> m_edges;
    std::vector<TriangulatedComponent> m_edge_neighbors;

    bool m_winding_number_computed;
    int m_winding_number;
    bool m_is_induced_component;
  };

  class BasinData
  {
  public:
    NodeData *m_left_node, *m_bottom_node, *m_right_node;
    T m_width;
    bool m_left_highest;

    BasinData(void):
      m_left_node(NULL), m_bottom_node(NULL), m_right_node(NULL),
      m_width(0), m_left_highest(false)
    {}

    void
    clear(void)
    {
      m_left_node=NULL;
      m_bottom_node=NULL;
      m_right_node=NULL;
      m_width=0;
      m_left_highest=false;
    }

  };

  class EdgeEvent
  {
  public:
    EdgeData *m_constrained_edge;
    bool m_right;

    EdgeEvent(void):
      m_constrained_edge(NULL),
      m_right(false)
    {}
  };

  class NodeData
  {
  public:
    PointData *m_point;
    TriangleData *m_triangle;
    NodeData *m_prev;
    NodeData *m_next;
    T m_value;

    explicit
    NodeData(PointData *pt, TriangleData *tri=NULL):
      m_point(pt), m_triangle(tri), 
      m_prev(NULL), m_next(NULL),
      m_value(pt->x())
    {}

    bool
    AbsoluteValueOfNodeAngleGreaterThanHalfPI(void);

    bool
    BasinAngleLessThan3PIOver4(void);
  };

  class AdvancingFrontData
  {
  public:
    NodeData *m_head, *m_tail, *m_search_node;

    AdvancingFrontData(NodeData *h, NodeData *t):
      m_head(h),
      m_tail(t),
      m_search_node(h)
    {}
    
    NodeData* 
    locate_node(T x);

    NodeData* 
    locate_point(PointData* point);
  };

  class WorkHorse
  {
  public:
    //ctor initializes and triangulates
    WorkHorse(const std::vector<input_point> &pts,
              const std::vector< std::vector<point_index> > &outlines,
              const std::vector< vecN<point_index,2> > &extra_edges);

    ~WorkHorse();

    const std::vector<ConnectedComponent*>&
    connected_componenents(void)
    {
      return m_components;
    }
    
    bool 
    point_triangulation_fail(void)
    {
      return m_point_triangulation_fail;
    }

    bool 
    connected_component_computation_fail(void)
    {
      return m_connected_component_computation_fail;
    }

  private:

    void
    initialize(const std::vector<input_point> &pts,
               const std::vector< std::vector<point_index> > &outlines,
               const std::vector< vecN<point_index, 2> > &extra_edges);

    void
    init_points(const std::vector<input_point> &pts);

    void
    init_edges(const std::vector< std::vector<point_index> > &outlines,
               const std::vector< vecN<point_index, 2> > &extra_edges);

  
    void
    create_advancing_front(void);

    void
    triangulate(void);

    NodeData*
    point_event(PointData*);

    NodeData*
    new_front_triangle(PointData*, NodeData*);

    bool
    legalize(TriangleData*);

    void
    map_triangle_to_nodes(TriangleData*);

    void
    rotate_triangle_pair(TriangleData*, PointData*, TriangleData*, PointData*);

    void
    fill(NodeData*);

    void
    fill_advancing_front(NodeData*);

    void
    fill_basin(NodeData*);

    void
    fill_basin_implement(NodeData*);

    bool
    is_shallow(NodeData*);

    void
    edge_event(EdgeData*, NodeData*);

    bool
    is_edge_side_of_triangle(TriangleData*, PointData*, PointData*);

    void
    fill_edge_event(EdgeData*, NodeData*);

    void
    fill_right_above_edge_event(EdgeData*, NodeData*);

    void
    fill_right_below_edge_event(EdgeData*, NodeData*);

    void
    fill_right_concave_edge_event(EdgeData*, NodeData*);

    void
    fill_right_convex_edge_event(EdgeData*, NodeData*);

    void
    fill_left_above_edge_event(EdgeData*, NodeData*);

    void
    fill_left_below_edge_event(EdgeData*, NodeData*);

    void
    fill_left_concave_edge_event(EdgeData*, NodeData*);

    void
    fill_left_convex_edge_event(EdgeData*, NodeData*);

    void
    edge_event(PointData*, PointData*, TriangleData*, PointData*);

    void
    flip_edge_event(PointData*, PointData*, TriangleData*, PointData*);

    TriangleData*
    next_flip_triangle(enum triangle_orientation, TriangleData*, TriangleData*,
                       PointData*, PointData*);

    PointData*
    next_flip_point(PointData*, PointData*, TriangleData*, PointData*);

    void
    flip_scan_edge_event(PointData*, PointData*, TriangleData*, 
                         TriangleData*, PointData*);

    void
    find_interior_triangles(const std::vector<input_point> &pts);



    std::vector<PointData*> m_points; //points
    std::vector<EdgeData*> m_edge_list; //edges from outlines
    std::vector<EdgeData*> m_constraint_edges; //edges added with add_edge() that do not affect winding computation
    std::vector<NodeData*> m_nodes; //worker data of nodes
    std::vector<TriangleData*> m_all_triangles; //all triangles, includes the triangles of "holes"

    AdvancingFrontData *m_front;
    PointData *m_head;
    PointData *m_tail;
    NodeData *m_af_head, *m_af_tail, *m_af_middle;
    EdgeEvent m_edge_event;
    BasinData m_basin;

    std::vector<ConnectedComponent*> m_components;
    
    bool m_point_triangulation_fail;
    bool m_connected_component_computation_fail;
    bool m_bounding_rectangle_added;
  };

 
  template<typename iterator>
  static
  void
  local_delete_each(iterator begin, iterator end)
  {
    for(;begin!=end; ++begin)
      {
        WRATHDelete(*begin);
      }
  }
    
  bool
  add_point_implement(const point &P, point_index I)
  {
    typename std::map<point_index, int>::iterator iter;

    iter=m_raw_pt_map.find(I);
    if(iter!=m_raw_pt_map.end())
      {
        return false;
      }
    else
      {
        point_index return_value(m_raw_points.size());

        m_raw_pt_map[I]=return_value;
        m_raw_points.push_back(input_point(P,I));

        return true;
      }
  }

  point_index
  get_raw_point_index(const point &P, point_index I)
  {
    typename std::map<point_index, int>::iterator iter;

    iter=m_raw_pt_map.find(I);
    if(iter!=m_raw_pt_map.end())
      {
        return iter->second;
      }
    else
      {
        point_index return_value(m_raw_points.size());

        m_raw_pt_map[I]=return_value;
        m_raw_points.push_back(input_point(P,I));

        return return_value;
      }
  }

  void
  create_work_horse_if_necessary(void);

  /*
    input data keyed by user defined indices,
    with values as an index into m_raw_points.
   */
  std::map<point_index, int> m_raw_pt_map;

  /*
    Raw input data
  */
  std::vector<input_point> m_raw_points;

  
  //indices are indices into the array m_raw_points.
  std::vector< std::vector<point_index> > m_outlines;

  //indices are indices into the array m_raw_points
  std::vector< vecN<point_index,2> > m_contraint_edges;

  /*
    work data
   */
  WorkHorse *m_work_horse;

  /*
    output triangulation.
  */
  std::vector<TriangulatedComponent> m_connected_components;
  std::vector<point_index> m_even_odd_rule_triangulation, m_winding_rule_triangulation;
  
  ///@endcond
};


/* actual implementation */
#include "WRATHTriangulationImplement.tcc"

/*!\typedef WRATHTriangulationI
  Conveniance typedef to WRATHTriangulation\<int32_t\>
 */
typedef WRATHTriangulation<int32_t> WRATHTriangulationI;

/*!\typedef WRATHTriangulationF
  Conveniance typedef to WRATHTriangulation\<float\>
 */
typedef WRATHTriangulation<float> WRATHTriangulationF;

/*! @} */


#endif
