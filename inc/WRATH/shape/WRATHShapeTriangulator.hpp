/*! 
 * \file WRATHShapeTriangulator.hpp
 * \brief file WRATHShapeTriangulator.hpp
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


#ifndef WRATH_HEADER_SHAPE_TRIANGULATOR_HPP_
#define WRATH_HEADER_SHAPE_TRIANGULATOR_HPP_

#include "WRATHConfig.hpp"
#include <boost/tuple/tuple.hpp>
#include "WRATHShapeSimpleTessellator.hpp"

/*! \addtogroup Shape
 * @{
 */

/*!\class WRATHShapeTriangulatorPayload
  A WRATHShapeTriangulatorPayload carries the payload for
  triangulating, i.e. filling, a WRATHShape<T>. It's starting
  point is a \ref WRATHShapeSimpleTessellatorPayload, i.e. it
  triangulates the edges of a WRATHShape<T> whose tessellation
  is given by a \ref WRATHShapeSimpleTessellatorPayload.
 */
class WRATHShapeTriangulatorPayload:
  public WRATHReferenceCountedObjectT<WRATHShapeTriangulatorPayload>
{
public:
    
  /*!\class PointBase
    Common base class for point for shape triangulation.
   */
  class PointBase
  {
  public:    
    /*!\fn PointBase(int, const vec2&)
      Ctor.
      \param ID value to which to initialize \ref m_ID as
      \param p value to which to initialize \ref m_position as
     */ 
    PointBase(int ID, const vec2 &p):
      m_position(p),
      m_ID(ID)
    {}

    /*!\fn PointBase(void)
      Initializes \ref m_ID as 0 and
      \ref m_position as (0,0)
     */ 
    PointBase(void):
      m_position(0.0f, 0.0f),
      m_ID(0)
    {}

    virtual
    ~PointBase()
    {}

    /*!\fn bool is_unbounded_point
      Triangulation adds an additional rectangle that bounds
      the bounding box of the original WRATHShapeT. If
      the point comes from these points, then 
      is_unbounded_point() returns true, otherwise
      returns false.
     */
    virtual
    bool
    is_unbounded_point(void) const
    {
      return true;
    }

    /*!\var m_position
      Position of point
     */
    vec2 m_position;

    /*!\var m_ID
      ID of the point relative to the WRATHShapeTriangulatorPayload,
      the paramter to feed \ref WRATHShapeTriangulatorPayload::point()
      and FilledComponent::point() to return this point.
     */
    unsigned int m_ID;
  };

  /*!\class Point
    Point type of triangulation for those points
    coming directly from a point of a 
    \ref WRATHShapeSimpleTessellatorPayload::TessellatedEdge.
   */
  class Point:public PointBase
  {
  public:
    /*!\fn Point(int, const vec2 &,
                 const WRATHShapeSimpleTessellatorPayload::TessellatedEdge::handle&,
                 int)
       Ctor.
       \param ID passed to \ref PointBase ctor (ID of point)
       \param p passed to \ref PointBase ctor (position of point)
       \param pE to which to initialize \ref m_E as
       \param pcurve_pointID to which to initialize \ref m_curve_pointID as
     */ 
    Point(int ID, const vec2 &p,
          const WRATHShapeSimpleTessellatorPayload::TessellatedEdge::handle &pE, 
          int pcurve_pointID):
      PointBase(ID, p),
      m_E(pE),
      m_curve_pointID(pcurve_pointID)
    {}

    /*!\fn Point(void)
      Ctor. Calls PointBase(void). Initializes
      \ref m_curve_pointID as -1.
     */
    Point(void):
      m_curve_pointID(-1)
    {}

    virtual
    bool
    is_unbounded_point(void) const
    {
      return false;
    }

    /*!\var m_E
      A handle to the tessellated edge 
      from which the point originates.
     */
    WRATHShapeSimpleTessellatorPayload::TessellatedEdge::handle m_E;

    /*!\var m_curve_pointID
      The a curve point ID of a tessellated edge 
      from which the point originates. This ID is 
      the index into m_E->curve_points() of the point.
     */
    int m_curve_pointID;
  };
  
  /*!\class InducedPoint
    An induced point is a point not in a 
    WRATHShapeSimpleTessellatorPayload::TessellatedEdge
    but formed from tessellation. The source points
    may or may not be points from the tesselated edge
    as well, i.e. an induced point may have as one
    of it's sources another induced point. An 
    induced point is a convex combination of up to
    4 points generated before it (some of which may also
    be induced points).
   */
  class InducedPoint:public PointBase
  {
  public:
    /*!\fn InducedPoint
      Ctor. Constructs InducedPoint as a convex combination
            of previously made points
      \param ID ID passed to \ref PointBase ctor (ID of point)
      \param p passed to \ref PointBase ctor (position of point)
      \param pconvex_coeff convex coefficients
      \param pt_source_ids ID's of point, must be same size of pconvex_coeff
     */
    InducedPoint(int ID, const vec2 &p,
                 const_c_array<float> pconvex_coeff,
                 const_c_array<unsigned int> pt_source_ids);

    virtual
    bool
    is_unbounded_point(void) const
    {
      return false;
    }

    /*!\fn unsigned int number_sources
      Returns the number of points that created
      the induced point. 
     */
    unsigned int
    number_sources(void) const
    {
      return m_count;
    }

    /*!\fn const_c_array<float> convex_coeff
      Returns the convex coefficients of the 
      points from which the InducedPoint comes.
     */
    const_c_array<float>
    convex_coeff(void) const
    {
      return const_c_array<float>(m_convex_coeff).sub_array(0, m_count);
    }

    /*!\fn const PointBase* source_point
      Returns the named source point from which 
      the InducedPoint comes. One needs the original
      payload to get the point.
      \param h payload of which the InducedPoint is a part
      \param I index of source point, 0<= I < number_sources()
     */
    const PointBase*
    source_point(const WRATHShapeTriangulatorPayload::handle &h,
                 unsigned int I) const
    {
      WRATHassert(h.valid());
      WRATHassert(I<m_count);
      return h->point(m_sources_ids[I]);
    }

    /*!\fn unsigned int source_point_index
      Returns the index to feed to WRATHShapeTriangulatorPayload::point()
      (or WRATHShapeTriangulatorPayload::FilledComponent::point())
      to get the named point from which the InducedPoint comes
      \param I index of source point, 0< =I < number_sources()
     */
    unsigned int
    source_point_index(unsigned int I) const
    {
      WRATHassert(I<m_count);
      return m_sources_ids[I];
    }

  private:
    unsigned int m_count;
    vec4 m_convex_coeff;
    vecN<unsigned int, 4> m_sources_ids;
  };

  /*!\class SplitPoint
    A split point is a point made from splitting
    an internal edge or splitting a triangle.
   */
  class SplitPoint:public InducedPoint
  {
  public:
    /*!\fn SplitPoint
      Ctor.
      \param ID ID passed to \ref InducedPoint() ctor (ID of point)
      \param p passed to \ref InducedPoint() ctor (position of point)
      \param pconvex_coeff passed to \ref InducedPoint() ctor
      \param pt_source_ids passed to \ref InducedPoint() ctor
     */
    SplitPoint(int ID, const vec2 &p,
               const_c_array<float> pconvex_coeff,
               const_c_array<unsigned int> pt_source_ids):
      InducedPoint(ID, p, pconvex_coeff, pt_source_ids)
    {}

    /*!\fn bool from_split_edge
      Returns true if and only if the
      SplitPoint came from splitting an
      edge.
     */
    bool
    from_split_edge(void) const
    {
      return convex_coeff().size()==2;
    }
  };

  /*!\class BoundaryHalfEdge
    A BoundaryHalfEdge describes data that applies
    to only one half of an edge, for example the
    3rd vertex of the triangle on a side of an edge.
   */
  class BoundaryHalfEdge
  {
  public:
    /*!\fn BoundaryHalfEdge
      Default ctor, inits:
      - \ref m_opposite_vertex as 0 
      - \ref m_triangle_location as 0 
      - \ref m_split_opposite_vertex as 0 
      - \ref m_split_triangle_location as 0  
      - \ref m_connected_component_ID as -1  
     */
    BoundaryHalfEdge(void):
      m_opposite_vertex(0),
      m_triangle_location(0),
      m_split_opposite_vertex(0),
      m_split_triangle_location(0),
      m_connected_component_ID(-1)
    {}

    /*!\var m_opposite_vertex
      The point ID of the opposite vertex of the 
      triangle that uses this half edge in the 
      non-splitting triangulation.
     */
    unsigned int m_opposite_vertex;

    /*!\var m_triangle_location
      Returns an index into FilledComponent::triangle_indices()
      to be passed to the FilledComponent from which this
      BoundaryEdge comes, of the triangle that uses this 
      half edge, that triangle will be made from the vertices
      \ref BoundaryEdge::m_v0, \ref BoundaryEdge::m_v1 and \ref m_opposite_vertex
     */
    unsigned int m_triangle_location;

    /*!\var m_split_opposite_vertex
      The point ID of the opposite vertex of the 
      triangle that uses this half edge in the 
      splitting triangulation.
     */
    unsigned int m_split_opposite_vertex;

    /*!\var m_split_triangle_location
      Returns an index into FilledComponent::split_triangulation_indices()
      to be passed to the FilledComponent from which this
      BoundaryEdge comes, of the triangle that used this edge,
      that triangle will be made from the vertices
      \ref BoundaryEdge::m_v0, \ref BoundaryEdge::m_v1 and \ref m_split_opposite_vertex
     */
    unsigned int m_split_triangle_location;

    /*!\var m_connected_component_ID
      A given \ref FilledComponent might have multiple
      connected components, this value gives
      the ID of the connected component of the
      triangle of this half edge
     */
    int m_connected_component_ID;
  };

  /*!\class BoundaryEdge
    A BoundaryEdge stores information of an
    edge of a component of the triangulation:
    - vertices of edge
    - 3rd vertex of triangle that used the edge 
    - vertex of triangle in other component
    - winding number of triangle on other component
    - location of triangle(s) that use the edge
   */
  class BoundaryEdge
  {
  public:
    /*!\fn BoundaryEdge
      Ctor to intialize BoundaryEdge object
     */
    BoundaryEdge(void);
    
    /*!\var m_v0
      Point ID of one vertex of the BoundaryEdge
     */
    unsigned int m_v0;
    
    /*!\var m_v1
      Point ID of the other vertex of the BoundaryEdge
     */
    unsigned int m_v1;

    /*!\var m_half_edge
      Gives data about the triangle and split
      triangle that uses this edge
     */
    BoundaryHalfEdge m_half_edge;

    /*!\var m_contour_ID
      Names the index in which to pass to
      FilledComponent::contour(unsigned int)
      for the contour on which this edge is 
      a part.
     */
    int m_contour_ID;

    /*!\var m_contour_edge_ID
      Names the sub-index of the edge
      within the contour. Specifically,
      it is that
      \code
      this == &C.contour(m_contour_ID)[m_contour_edge_ID]
      \endcode
      where C is the \ref FilledComponent in
      which this BoundaryEdge resides
     */
    int m_contour_edge_ID;

    /*!\var m_neighbor
      If the edge is shared by another
      triangle, .first is true and .second
      gives the winding number of that triangle.
      If no triangle shares that edge, then 
      .first is false and .second is 0.
     */
    std::pair<bool, int> m_neighbor;

    /*!\var m_neighbor_half_edge
      If .first of \ref m_neighbor is true, then 
      gives data on the triangle on the other 
      side of the edge.
     */
    BoundaryHalfEdge m_neighbor_half_edge;
  };
 
  /*!\class FilledComponent
    A FilledComponent represents a set
    of triangles of filling with a common
    winding number.
    Triangulation induces 2 different useful
    triangulations:
    - standard triangulation 
    - splitting triangulation. In this triangulation, internal
      edges whose both vertices are a part of an external edge
      are split in the middle. In addition, those triangles 
      for which each of their vertices are on an external edge
      are split in their middle (into 3 triangles). 
   */
  class FilledComponent
  {
  public:
    /*!\fn FilledComponent
      Ctor, initializing FilledComponent as invalid
     */ 
    FilledComponent(void):
      m_winding_number(0),
      m_split_points_range(0, 0)
    {}

    /*!\fn bool valid
      Returns true if this FilledComponent
      references valid data.
     */
    bool
    valid(void) const
    {
      return m_array_keeper.valid();
    }

    /*!\fn int winding_number
      Returns the winding number
      of this FilledComponent.
     */
    int
    winding_number(void) const
    {
      WRATHassert(valid());
      return m_winding_number;
    }

    /*!\fn const_c_array<unsigned int> triangle_indices
      Returns the indices that triangules
      the filling of this FilledComponent.
      These are indices to be fed into points().
     */
    const_c_array<unsigned int>
    triangle_indices(void) const
    {
      WRATHassert(valid());
      return m_triangle_indices;
    }

    /*!\fn const_c_array< range_type<unsigned int> > connected_component_ranges
      Returns an array of ranges applicably
      to triangle_indices(). The range at
      index C are those triangle indices
      on the connected component C.
     */
    const_c_array< range_type<unsigned int> >
    connected_component_ranges(void) const
    {
      WRATHassert(valid());
      return m_component_ranges;
    }

    /*!\fn unsigned int number_connected_components
      Provided as a conveniance, returns
      the number of connected components,
      equivalent to
      \code
      connected_component_ranges().size();
      \endcode
     */
    unsigned int
    number_connected_components(void) const
    {
      return connected_component_ranges().size();
    }

    /*!\fn const_c_array<unsigned int> conencted_component
      Returns the triangle indices for the
      named connected component, equivalent to
      \code
      triangle_indices().sub_array( connected_component_ranges()[C] )
      \endcode
      \param C connected component ID
     */
    const_c_array<unsigned int>
    conencted_component(unsigned int C) const
    {
      WRATHassert(valid());
      return m_triangle_indices.sub_array(m_component_ranges[C]);
    }

    /*!\fn const_c_array<BoundaryEdge> boundary_edges
      Returns the boundary edge data of the
      FilledComponent.
     */
    const_c_array<BoundaryEdge>
    boundary_edges(void) const
    {
      WRATHassert(valid());
      return m_boundary_edges;
    }

    /*!\fn int number_contours
      Returns the number of closed
      contours of the FilledComponent
     */
    int
    number_contours(void) const
    {
      return m_contours.size();
    }

    /*!\fn const_c_array<BoundaryEdge> contour
      Returns the named contour of the
      FilledComponent
      \param C contour to fetch, must have that
               0<= C < number_contours()
     */
    const_c_array<BoundaryEdge>
    contour(unsigned int C) const
    {
      return m_boundary_edges.sub_array(m_contours[C]);
    }

    /*!\fn const_c_array< range_type<unsigned int> > contours
      Returns an array of ranges into boundary_edges()
      stating the closed contours of the FilledCompenent.
      Note that contour(C) is equivalent to
      boundary_edges().sub_array(contours()[C]).
     */
    const_c_array< range_type<unsigned int> > 
    contours(void) const
    {
      return m_contours;
    }

    /*!\fn const_c_array<unsigned int> split_triangulation_indices
      Returns the indices of the splitting 
      triangulation, this triangulation
      is a refinement of the triangulation
      returned by triangle_indices(). 
      If a vertex is used by a boundary
      edge it is labeled as a boundary vertex.
      The refinement is the following:
      - if a triangle has that each of it's vertices are 
        marked as boundary vertices, then a new point
        is made at the triangle center and the triangle
        is then split into 3 triangles
      - if an edge which is not a boundary edge, but
        both of it's vertices are boundary vertices,
        then the edge is split it middle
     */
    const_c_array<unsigned int>
    split_triangulation_indices(void) const
    {
      WRATHassert(valid());
      return m_split_triangulation_indices;
    }

    /*!\fn const_c_array< range_type<unsigned int> > connected_component_split_ranges
      Returns an array of ranges applicably
      to split_trianglulation_indices(). The range at
      index C are those split triangle indices
      on the connected component C.
     */
    const_c_array< range_type<unsigned int> >
    connected_component_split_ranges(void) const
    {
      WRATHassert(valid());
      return m_split_component_ranges;
    }

    /*!\fn const_c_array<unsigned int> connected_component_split
      Returns the split triangle indices for the
      named connected component, equivalent to
      \code
      split_trianglulation_indices().sub_array( connected_component_split_ranges()[C] )
      \endcode
      \param C connected component ID
     */
    const_c_array<unsigned int>
    connected_component_split(unsigned int C) const
    {
      WRATHassert(valid());
      return m_triangle_indices.sub_array(m_split_component_ranges[C]);
    }

    /*!\fn range_type<unsigned int> split_points_range
      Returns the range of indices to feed
      to \ref point() that are created
      solely for the splitting triangulation.
      It is guaranteed that these points will
      come after all induced points of all
      components.
     */
    range_type<unsigned int>
    split_points_range(void) const
    {
      return m_split_points_range;
    }

    /*!\fn const_c_array<Point> pts
      Returns the points that come directly
      from the tessellated edges used by this
      FilledComponent and all FilledComponents
      of the WRATHShapeTriangulatorPayload.
      This is the _same_ return value
      as WRATHShapeTriangulatorPayload::pts(),
      as such includes a great deal of points
      that this FilledComponent does not
      use.
    */
    const_c_array<Point>
    pts(void) const;

    /*!\fn const_c_array<InducedPoint> induced_pts
      Returns the induced points used by this
      FilledComponent and all FilledComponents
      of the WRATHShapeTriangulatorPayload.
      This is the _same_ return value
      as WRATHShapeTriangulatorPayload::pts(),
      as such includes a great deal of points
      that this FilledComponent does not
      use.
    */
    const_c_array<InducedPoint>
    induced_pts(void) const;

    /*!\fn const_c_array<PointBase> unbounded_pts
      Returns the unbounded points. These are points
      added by the payload from 2 surrounding contours,
      each a bounding box (but in opposite direction)
      of the shape.
    */
    const_c_array<PointBase>
    unbounded_pts(void) const;

    /*!\fn const_c_array<SplitPoint> split_induced_pts
      Returns the induced points coming from
      splitting edges and triangles of this
      FilledComponent only.
     */
    const_c_array<SplitPoint>
    split_induced_pts(void) const;
    
    /*!\fn const PointBase* point
      Returns the named point, with the convention
      that:
      - first come points from the contour (i.e. pts())
      - second comes unbounded points (i.e. unbounded_pts())
      - third come induced points (i.e. induced_pts())
      - forth comes induced points from splitting (i.e. split_induced_pts())
     */
    const PointBase*
    point(unsigned int I) const;

    /*!\fn WRATHShapeSimpleTessellatorPayload::handle tessellated_payload_source
      Returns the WRATHShapeSimpleTessellatorPayload
      used to construct the triangulation.
    */
    WRATHShapeSimpleTessellatorPayload::handle
    tessellated_payload_source(void) const;

  private:
    friend class WRATHShapeTriangulatorPayload;

    int m_winding_number;
    const_c_array<unsigned int> m_triangle_indices;
    const_c_array<unsigned int> m_split_triangulation_indices;
    const_c_array<BoundaryEdge> m_boundary_edges;
    range_type<unsigned int> m_split_points_range;
    const_c_array< range_type<unsigned int> > m_contours;
    const_c_array< range_type<unsigned int> > m_component_ranges;
    const_c_array< range_type<unsigned int> > m_split_component_ranges;

    //the actual arrays are stored in a WRATHReferenceCountedObject
    //as such, to make sure the arrays stay in scope as long as one
    //is using this, we add a handle to that object, to keep those
    //array in scope.
    WRATHReferenceCountedObject::handle_t<WRATHReferenceCountedObject> m_array_keeper;
  };
  


  /*!\fn WRATHShapeTriangulatorPayload(const WRATHShapeSimpleTessellatorPayload::handle&,
                                       const std::string&)
  
    Ctor. Construct a WRATHShapeTriangulatorPayload from
    the edges whose tessellation is stored in a 
    WRATHShapeSimpleTessellatorPayload.
    \param in_data holds the tessellated edges of a WRATHShape
    \param data_label string with which to label the created payload data
   */
  WRATHShapeTriangulatorPayload(const WRATHShapeSimpleTessellatorPayload::handle &in_data,
                                const std::string &data_label);

  
  ~WRATHShapeTriangulatorPayload();

  /*!\fn const_c_array<Point> pts
    Returns the points of the triangulation coming
    drectly from the tessellated edges
   */
  const_c_array<Point>
  pts(void) const
  {
    return m_datum->pts();
  }

  /*!\fn const_c_array<InducedPoint> induced_pts
    Returns the induced points of the triangulation
  */
  const_c_array<InducedPoint>
  induced_pts(void) const
  {
    return m_datum->induced_pts();
  }

  /*!\fn const_c_array<PointBase> unbounded_pts
    Returns the unbounded points. These are points
    added by the payload from 2 surrounding contours,
    each a bounding box (but in opposite direction)
    of the shape.
  */
  const_c_array<PointBase>
  unbounded_pts(void) const
  {
    return m_datum->unbounded_pts();
  }

  /*!\fn const_c_array<SplitPoint> split_induced_pts
    Returns the induced points coming from
    splitting edges and triangles.
  */
  const_c_array<SplitPoint>
  split_induced_pts(void) const
  {
    return m_datum->split_induced_pts();
  }
  
  /*!\fn const PointBase* point
    Returns the named point, with the convention
      that:
      - first come points from the contour (i.e. pts())
      - second comes unbounded points (i.e. unbounded_pts())
      - third come induced points (i.e. induced_pts())
      - forth comes induced points from splitting (i.e. split_induced_pts())
  */
  const PointBase*
  point(unsigned int I) const
  {
    return m_datum->point(I);
  }

  /*!\fn number_points_without_splits
    Returns the number of points of the triangulation,
    NOT including those points from splitting edges
    and triangles, i.e returns the total number of points
    from:
    - original points of the tessellation
    - points to specify the bounding box (4 of them)
    - induced points from triangulation (when the path intersects itself)
   */
  unsigned int
  number_points_without_splits(void) const
  {
    return m_datum->number_points_without_splits();
  }

  /*!\fn unsigned int total_number_points
    Returns the total number of points of the triangulation,
    this includes:
    - original points of the tessellation
    - points to specify the bounding box (4 of them)
    - induced points from triangulation (when the path intersects itself)
    - induced points from splitting edges and triangles
   */
  unsigned int
  total_number_points(void) const
  {
    return m_datum->total_number_points();
  }

  /*!\fn const std::map<int, FilledComponent>& components
    Returns the components of the
    triangulation (indices of the ConnectedComponent
    objects of the returned array are indices
    into \ref pts() ) to be used for drawing 
    the filled shape, the map is keyed by
    winding number with values FilledComponent.
   */
  const std::map<int, FilledComponent>&
  components(void) const
  {
    return m_components;
  }

  /*!\fn const FilledComponent& winding_zero_unbounded_component
    Returns a FilledComponent object which
    only has those triangles with winding
    number zero which are a part of the
    unbounded connected component of the
    fill component of winding zero.
   */
  const FilledComponent&
  winding_zero_unbounded_component(void) const
  {
    return m_winding_zero_unbounded_components;
  }

  /*!\fn const FilledComponent& winding_zero_bounded_component
    Returns a FilledComponent object which
    only has those triangles with winding
    number zero which are not part of the
    unbounded portion.
   */
  const FilledComponent&
  winding_zero_bounded_component(void) const
  {
    return m_winding_zero_bounded_components;
  }

  /*!\fn WRATHShapeSimpleTessellatorPayload::handle tessellated_payload_source
    Returns the WRATHShapeSimpleTessellatorPayload
    used to construct the triangulation.
   */
  WRATHShapeSimpleTessellatorPayload::handle
  tessellated_payload_source(void) const
  {
    return m_datum->m_src;
  }

  /*!\class PayloadParams
    Typedef used by WRATHShape template function
    to generate WRATHShapeTriangulatorPayload 
    objects on demand.
   */
  class PayloadParams
  {
  public:
    /*!\typedef PayloadType
      Type that uses this paramter type
     */
    typedef WRATHShapeTriangulatorPayload PayloadType;

    /*!\fn PayloadParams
      Ctor
      \param v value to which to initialize \ref m_tess_params
     */
    PayloadParams(const WRATHShapeSimpleTessellatorPayload::PayloadParams &v=
                  WRATHShapeSimpleTessellatorPayload::PayloadParams()):
      m_tess_params(v)
    {}

    /*!\fn tess_params
      Sets \ref m_tess_params 
      \param v value to use
     */
    PayloadParams&
    tess_params(const WRATHShapeSimpleTessellatorPayload::PayloadParams &v)
    {
      m_tess_params=v;
      return *this;
    }

    /*!\fn bool operator==(const PayloadParams&) const
      Equality operator.
      \param rhs value to which to compare
     */
    bool
    operator==(const PayloadParams &rhs) const
    {
      return m_tess_params==rhs.m_tess_params;
    }

    /*!\var m_tess_params
      Tessellation paramaters.
    */
    WRATHShapeSimpleTessellatorPayload::PayloadParams m_tess_params;
  };


  /*!\fn handle generate_payload(const WRATHShape<T>&, const PayloadParams&)
    Template function used by WRATHShape to generate
    WRATHShapeTriangulatorPayload objects on demand.
    If the existing WRATHShapeSimpleTessellatorPayload of
    the WRATHShape was created with different parameters,
    then routine will trigger the WRATHShape object to
    store and generate a new WRATHShapeSimpleTessellatorPayload
    object created using the tessellation parameters specified.
 
    \param pshape WRATHShape from which to generate payload
    \param pp payload parameters specifying tessellation parameters
              and what join and cap types to generate
   */
  template<typename T>
  static
  handle
  generate_payload(const WRATHShape<T> &pshape,
                   const PayloadParams &pp)
  {
    WRATHShapeSimpleTessellatorPayload::handle tess;

    tess=pshape.template fetch_matching_payload<WRATHShapeSimpleTessellatorPayload>(pp.m_tess_params);
    return WRATHNew WRATHShapeTriangulatorPayload(tess, pshape.label());
  }

  /*!\fn handle generate_payload(const WRATHShape<T>&)
    Template meta-function used by WRATHShape to generate
    WRATHShapeTriangulatorPayload objects on demand.
    Will use any pre-existing WRATHShapeSimpleTessellatorPayload
    stored in the WRATHShape object and will generate a
    WRATHShapeTriangulatorPayload using that stored tessellation.

    \param pshape WRATHShape from which to generate the payload
   */
  template<typename T>
  static
  handle
  generate_payload(const WRATHShape<T> &pshape)
  {
    WRATHShapeSimpleTessellatorPayload::handle tess;

    tess=pshape.template fetch_payload<WRATHShapeSimpleTessellatorPayload>();
    return WRATHNew WRATHShapeTriangulatorPayload(tess, pshape.label()); 
  }

private:

  /*
    get<0> --> triangle indices
    get<1> --> edges
    get<2> --> split triangle indices
    get<3> --> range of point ID's holding point needed for splitting
    get<4> --> array of ranges naming the contours of the component
    get<5> --> array of ranges of triangle connected components
    get<6> --> array of ranges of split triangle connected components
   */
  typedef range_type<unsigned int> contour_range;
  typedef range_type<unsigned int> component_range;
  typedef boost::tuple< std::vector<unsigned int>,
                        std::vector<BoundaryEdge>,
                        std::vector<unsigned int>,
                        range_type<unsigned int>,
                        std::vector<contour_range>,
                        std::vector<component_range>,
                        std::vector<component_range> > per_winding;

 

  class DatumKeeper:
    public WRATHReferenceCountedObjectT<DatumKeeper>
  {
  public:
    DatumKeeper(const WRATHShapeSimpleTessellatorPayload::handle &src):
      m_src(src)
    {}

    WRATHShapeSimpleTessellatorPayload::handle m_src;

    /*
      keyed by winding number.
     */
    std::map<int, per_winding> m_all_per_winding_datas;

    /*
      The triangles from m_all_per_winding_datas[0]
      that are a part of the unbounded components
     */
    per_winding m_winding_zero_unbounded_components;

    /*
      The triangles from m_all_per_winding_datas[0]
      that are a part of the bounded components
     */
    per_winding m_winding_zero_bounded_components;


    /*
      all pts, points are shared across different components.
     */
    std::vector<Point> m_pts;
    std::vector<PointBase> m_unbounded_pts;
    std::vector<InducedPoint> m_induced_pts;

    /*
      points from splitting edges
     */
    std::vector<SplitPoint> m_split_induced_pts;

    
    const PointBase*
    point(unsigned int I) const;
    
    unsigned int
    total_number_points(void) const
    {
      return m_pts.size() + m_unbounded_pts.size()
        + m_induced_pts.size() + m_split_induced_pts.size();
    }

    unsigned int
    number_points_without_splits(void) const
    {
      return m_pts.size() + m_unbounded_pts.size() + m_induced_pts.size();
    }

    const_c_array<Point>
    pts(void) const
    {
      return m_pts;
    }

    const_c_array<PointBase>
    unbounded_pts(void) const
    {
      return m_unbounded_pts;
    }

    const_c_array<InducedPoint>
    induced_pts(void) const
    {
      return m_induced_pts;
    }

    const_c_array<SplitPoint>
    split_induced_pts(void) const
    {
      return m_split_induced_pts;
    }
  };

  void
  extract_component_data(void);

  void
  set_filled_component(FilledComponent &C, 
                       int winding,
                       per_winding &d);

  std::map<int, FilledComponent> m_components;
  FilledComponent m_winding_zero_unbounded_components;
  FilledComponent m_winding_zero_bounded_components;
  DatumKeeper::handle m_datum;
};

/*!\fn std::ostream& operator<<(std::ostream &, const WRATHShapeTriangulatorPayload::BoundaryEdge &)
  Overload of operator<< to print the contents of a 
  \ref WRATHShapeTriangulatorPayload::BoundaryEdge 
  to an std::ostream
  \param ostr std::ostream to which to print
  \param bd data to print
 */
std::ostream&
operator<<(std::ostream &ostr, const WRATHShapeTriangulatorPayload::BoundaryEdge &bd);


/*! @} */

inline
const_c_array<WRATHShapeTriangulatorPayload::Point>
WRATHShapeTriangulatorPayload::FilledComponent::
pts(void) const
{
  const DatumKeeper *p;
  WRATHassert(valid());
  p=static_cast<const DatumKeeper*>(m_array_keeper.raw_pointer());
  return p->pts();
}

inline
const_c_array<WRATHShapeTriangulatorPayload::InducedPoint>
WRATHShapeTriangulatorPayload::FilledComponent::
induced_pts(void) const
{
  const DatumKeeper *p;
  WRATHassert(valid());
  p=static_cast<const DatumKeeper*>(m_array_keeper.raw_pointer());
  return p->induced_pts();
}

inline
const_c_array<WRATHShapeTriangulatorPayload::PointBase>
WRATHShapeTriangulatorPayload::FilledComponent::
unbounded_pts(void) const
{
  const DatumKeeper *p;
  WRATHassert(valid());
  p=static_cast<const DatumKeeper*>(m_array_keeper.raw_pointer());
  return p->unbounded_pts();
}

inline
const_c_array<WRATHShapeTriangulatorPayload::SplitPoint>
WRATHShapeTriangulatorPayload::FilledComponent::
split_induced_pts(void) const
{
  const DatumKeeper *p;
  WRATHassert(valid());
  p=static_cast<const DatumKeeper*>(m_array_keeper.raw_pointer());
  return p->split_induced_pts();
}

inline
const WRATHShapeTriangulatorPayload::PointBase*
WRATHShapeTriangulatorPayload::FilledComponent::
point(unsigned int I) const
{
  const DatumKeeper *p;
  WRATHassert(valid());
  p=static_cast<const DatumKeeper*>(m_array_keeper.raw_pointer());
  return p->point(I);
}

inline
WRATHShapeSimpleTessellatorPayload::handle
WRATHShapeTriangulatorPayload::FilledComponent::
tessellated_payload_source(void) const
{
  const DatumKeeper *p;
  WRATHassert(valid());
  p=static_cast<const DatumKeeper*>(m_array_keeper.raw_pointer());
  return p->m_src;
}


#endif
