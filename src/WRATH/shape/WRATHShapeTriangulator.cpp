/*! 
 * \file WRATHShapeTriangulator.cpp
 * \brief file WRATHShapeTriangulator.cpp
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

#include <boost/signals2/signal.hpp>
#include <boost/bind.hpp>
#include <sys/time.h>
#include <set>
#include "WRATHUtil.hpp"
#include "WRATHShapeTriangulator.hpp"
#include "WRATHTessGLU.hpp"



/*
  On the surface triangulation should *just* be use WRATHTessGLU to
  create the triangulation, but we want many things:

   - all fill components, classified by winding number including winding 0
     -- for winding 0, there is an unbounded component, bound it by
        the 4 points of a box that is a touch larger than the bouding
        box of the original shape

   - rich boundary data on the fill components:
     -- for each boundary edge, the triangle that uses from the filled component
     -- for each boundary edge, the triangle on the other side that uses
     -- boundary edges sorted into closed contours so that one can
        extract the contours of a filled component

   - split triangulation which splits edges and triangles as follows
     -- if an internal edge is from two vertices which are both 
        used by a boundary edge, then that internal edge is split
     -- if a triangle is so that all it's vertices are used by
        by a boundary edge, then that triangle is split into 3 triangles
     -- all vertices introduces by splitting come after induced
        points of usual triangulation AND the split points from
        a fill component are in a continuous block
     -- rich boundary data tracks both the split and non-split 
        triangulaion data

   - trianglulation and split triangulation sorted by connected
     component of the fill component


  The way we do this as follows:

  1) we first triangulate as usual with WRATHGLUTess with the
     fill rule being non-zero. As combine vertex commands come
     in we record the source of the combine and store the triangle
     indices keyed by winding number [class NonZeroFill]

  2) then we triangulate again, adding a bounding box contour
     that is larger tan the original bounding box. The triangulation
     should be that all points it induces are found in (1)
     [class ZeroFill]

  3) we build an edge list, each edge comprising up to two
     half edges. These half edges store the triangle that uses
     them along with various piece of information. This
     edge list is global to a triangulation (see PointHolder::create_split_triangles_and_edge_data)

  4) for each winding component we build (see PointHolder::create_split_triangles_and_edge_data_winding_component)
      - a count for each edge how many times it is used
      - triangle neighbor list (i.e. for each triangle what are it's neighbors)
     from that data we can build:
      - classify ech triangle into a connected component
      - sort triangles by connected component
      - know what vertices are used by boundary edges of the fill component
      - create split trianglulation

  5) using the edge data global to the triangulation, we then can
     - know what edges boundary edges between components cheaply
     - create the contour information for each fill component
       (see PointHolder::sort_edges_into_contours)

  What is kind of *dumb* is that in truth GLU tessellator has
  this information already in it's mesh data structure,
  but that data is not really exposed.

  Additionally, invoking the tessellator twice is icky.
  The RIGHT thing to do would be to modify GLU tessellator
  as follows:

    - allow for GLU tessellator to emit triangles even if winding is 0
    - BUT do not emit any triangles for a component if it uses certain vertices
    - AND from WRATHShapeTriangulator, have  _two_ bounding box contours wrap
      around the shape, in opposite order. Set the GLU tessellator to ignore
      any component (face) that uses triangles from the most outside box

  This would allow for us to have the tessellator run only once.
 */


/*
  magic numbers used for creating contours,
   BOUNDARY_EDGE_UNTOUCHED --> never examined
   BOUNDARY_EDGE_BEING_PROCESSED --> currently on a list that will become a contour
 */
#define BOUNDARY_EDGE_UNTOUCHED -1
#define BOUNDARY_EDGE_BEING_PROCESSED -2

namespace
{
  
  typedef WRATHShapeTriangulatorPayload::BoundaryEdge BoundaryEdge;
  typedef WRATHShapeTriangulatorPayload::BoundaryHalfEdge BoundaryHalfEdge;
  typedef WRATHShapeTriangulatorPayload::Point Point;
  typedef WRATHShapeTriangulatorPayload::PointBase PointBase;
  typedef WRATHShapeTriangulatorPayload::InducedPoint InducedPoint;
  typedef WRATHShapeTriangulatorPayload::SplitPoint SplitPoint;
  typedef WRATHShapeTriangulatorPayload::FilledComponent FilledComponent;
  typedef range_type<unsigned int> contour_range;
  typedef range_type<unsigned int> component_range;
  typedef boost::tuple< std::vector<unsigned int>,
                        std::vector<BoundaryEdge>,
                        std::vector<unsigned int>,
                        range_type<unsigned int>,
                        std::vector<contour_range>,
                        std::vector<component_range>,
                        std::vector<component_range> > per_winding;

  class MiddleBarrierMarker
  {
  public:
    MiddleBarrierMarker(void):
      m_triangle_index(0),
      m_split_triangle_index(0),
      m_split_points_mark(0)
    {}

    unsigned int m_triangle_index;
    unsigned int m_split_triangle_index;
    unsigned int m_split_points_mark;
  };

  class BoundaryEdgeConnectedComponentIDComparer
  {
  public:
    bool
    operator()(const BoundaryEdge &lhs, const BoundaryEdge &rhs) const
    {
      return lhs.m_half_edge.m_connected_component_ID < rhs.m_half_edge.m_connected_component_ID;
    }
  };
  
  class BoundaryEdgeContourComparer
  {
  public:
    bool
    operator()(const BoundaryEdge &lhs, const BoundaryEdge &rhs) const
    {
      /*
        sort first by contour then by edge id
       */
      return lhs.m_contour_ID < rhs.m_contour_ID
        or (lhs.m_contour_ID==rhs.m_contour_ID and lhs.m_contour_edge_ID < rhs.m_contour_edge_ID);
    }
  };
  
  typedef std::vector<unsigned int> VertexUserList;
  class ContourElement
  {
  public:
    BoundaryEdge *m_edge;
    std::map<unsigned int, VertexUserList>::iterator m_vertex;
    unsigned int m_next_vertex;
  };

  class Edge:public vecN<unsigned int, 2>
  {
  public:
    Edge(unsigned int a, unsigned int b):
      vecN<unsigned int, 2>(std::min(a,b), std::max(a,b))
    {}
  };

  class HalfEdge
  {
  public:
    HalfEdge(void):
      m_v0(-1),
      m_v1(-1),
      m_winding(0),
      m_bd(NULL),
      m_opposite_vertex(-1),
      m_triangle_location(0),
      m_split_triangle_list_location(-1),
      m_connected_component_ID(-1)
    {}

    /*
      triangle orientation is from v0 to v1, thus
      it is given by [v0, v1, opp]
     */
    HalfEdge(unsigned int v0, unsigned int v1,
             int wn,
             std::vector<WRATHShapeTriangulatorPayload::BoundaryEdge> &bd, 
             unsigned int opp,
             unsigned int triangle_location):
      m_v0(v0),
      m_v1(v1),
      m_winding(wn),
      m_bd(&bd),
      m_opposite_vertex(opp),
      m_triangle_location(triangle_location),
      m_split_opposite_vertex(-1),
      m_split_triangle_list_location(-1),
      m_connected_component_ID(-1)
    {}

    unsigned int m_v0, m_v1;

    int m_winding;
    std::vector<WRATHShapeTriangulatorPayload::BoundaryEdge> *m_bd;
    unsigned int m_opposite_vertex; //index of opposite vertex

    unsigned int m_triangle_location; //location of triangle in index list

    unsigned int m_split_opposite_vertex; //opposite vertex in splitting triangulation
    unsigned int m_split_triangle_list_location; //location of triangle in index list of split triangle.

    //connected component ID
    int m_connected_component_ID;

    void
    set_boundary_half_edge(BoundaryHalfEdge &E) const
    {
      E.m_opposite_vertex=m_opposite_vertex;
      E.m_triangle_location=m_triangle_location;
      E.m_split_opposite_vertex=m_split_opposite_vertex;
      E.m_split_triangle_location=m_split_triangle_list_location;
      E.m_connected_component_ID=m_connected_component_ID;
    }

    void
    set_boundary_edge(Edge edge,
                      const HalfEdge *neighbor,
                      BoundaryEdge &E)
    {
      WRATHassert(Edge(m_v0, m_v1)==edge);
      WRATHunused(edge);

      E.m_v0=m_v0;
      E.m_v1=m_v1;
      set_boundary_half_edge(E.m_half_edge);

      if(neighbor!=NULL)
        {
          WRATHassert( Edge(neighbor->m_v0, neighbor->m_v1)==edge);
          E.m_neighbor.first=true;
          E.m_neighbor.second=neighbor->m_winding;
          neighbor->set_boundary_half_edge(E.m_neighbor_half_edge);
        }
      else
        {
          E.m_neighbor.first=false;
        }
    }
  };

  
  template<typename T, unsigned int N=2>
  class GenericEdgeData
  {
  public:
    GenericEdgeData(void):
      m_count(0)
    {}

    T*
    add_data(const T &h)
    {

      WRATHassert(m_count<N);
      m_data[m_count]=h;

      T *return_value(&m_data[m_count]);
      ++m_count;

      return return_value;
    }

    unsigned int m_count;
    vecN<T, N> m_data;
  };

 
  typedef GenericEdgeData<HalfEdge> EdgeData;
  typedef GenericEdgeData<unsigned int> TriangleEdgeData;
  typedef GenericEdgeData<unsigned int, 3> TriangleNeighbors;


  /*
    class for holding the point data, it makes
    and stores all the point data needed along the way.
   */
  class PointHolder
  {
  public:
    PointHolder(std::vector<Point> &pts,
                std::vector<InducedPoint> &ind_pts,
                std::vector<PointBase> &unbounded_pts,
                std::vector<SplitPoint> &split_ind_pts,
                std::map<int, per_winding> &all_per_winding_datas,
                per_winding &winding_zero_unbounded_components,
                per_winding &winding_zero_bounded_components,
                const WRATHShapeSimpleTessellatorPayload::handle &in_data,
                WRATHShapeTriangulatorPayload *master,
                const std::string &label);

    void
    add_contours(WRATHTessGLU*);

    void
    add_bounding_contour(WRATHTessGLU*);

    void*
    on_combine_vertex(vec2 vertex_position,
                      const_c_array<void*> vertex_source_datums,
                      const_c_array<float> vertex_weights,
                      unsigned int &increment_on_create_pt);

    const WRATHShapeTriangulatorPayload::PointBase*
    point(unsigned int I) const
    {
      return m_master->point(I);
    }

    static
    void
    check_filled_component(const std::map<int, FilledComponent> &others,
                           const FilledComponent &C,
                           const WRATHShapeTriangulatorPayload *payload);
                                 

    static
    void
    check_triangle_consistency_ignore_order(unsigned int triangle_loc,
                                            const_c_array<unsigned int> triangle_indices,
                                            unsigned int v0, unsigned int v1, unsigned int v2);

    static
    void
    check_triangle_consistency(unsigned int triangle_loc,
                               const_c_array<unsigned int> triangle_indices,
                               unsigned int v0, unsigned int v1, unsigned int v2);


    void
    sort_edges_into_contours(c_array<BoundaryEdge> edges, 
                             std::vector<contour_range> &C);
      
  private:

    void
    generate_bounding_box(void);

    void
    sort_into_contours(unsigned int &contourID, c_array<BoundaryEdge> edges);

   
    void
    build_contour_at(unsigned int &contourID,
                     std::map<unsigned int, VertexUserList>::iterator iter,
                     c_array<BoundaryEdge> edges,
                     std::map<unsigned int, VertexUserList> &vertex_users,
                     std::vector<ContourElement> &current_contour,
                     std::map<unsigned int, unsigned int> &vertex_branch_points);
    
    void
    build_contour(unsigned int &contourID,
                  c_array<ContourElement> elements);
                  

    void
    add_box_points(const vec2 &pmin, const vec2 &pmax);

    unsigned int
    number_points_needed(void);

    void
    triangulate(const std::string &label);

    void
    create_split_triangles_and_edge_data(void);
    
    void
    create_split_triangles_and_edge_data_winding_component(std::map<int, per_winding>::iterator iter,
                                                           std::map<Edge, EdgeData> &all_them_edges,
                                                           std::vector<bool> &vertex_flags,
                                                           MiddleBarrierMarker &markers,
                                                           int &winding0_unbounded_component);
    
    
    range_type<unsigned int> //returns the ranage of vertex induces made from splitting
    create_split_triangles(const std::map<Edge, TriangleEdgeData> &edge_counts,
                           const_c_array<unsigned int> src_triangles,
                           const std::vector<bool> &vertex_flags,
                           std::map<Edge, unsigned int> &split_edge_vertices,
                           const_c_array<HalfEdge*> triangle_half_edges,
                           std::vector<unsigned int> &out_triangles,
                           std::vector<component_range> &out_component_ranges,
                           MiddleBarrierMarker *middle_barrier_in_out);

    /*
      returns the component ID of the unbounded component
      (if there is one). Returns -1 on not finding it
     */
    int
    compute_unbounded_component(c_array<unsigned int> src_triangles,
                                c_array<HalfEdge*> triangle_half_edges);
    

    /*
      sorts triangles by connected component,
      updates location of half edges.
      Additionally, if lastC is non-negative,
      then makes that component the last component
      to appear in the sorting (and updates
      all component ID's in the process)

      component_location[C] store the range of indices
      into src_triangles which are on component C
     */
    void
    sort_triangles_by_component(int number_components,
                                c_array<unsigned int> src_triangles,
                                c_array<HalfEdge*> triangle_half_edges,
                                int lastC,
                                std::vector<range_type<unsigned int> > &component_location);

    void
    sort_triangles_by_component(int number_components,
                                c_array<unsigned int> src_triangles,
                                c_array<HalfEdge*> triangle_half_edges,
                                std::vector<range_type<unsigned int> > &component_location)
    {
      sort_triangles_by_component(number_components, src_triangles, triangle_half_edges, -1, component_location); 
    }
    

    
    void
    create_separated_winding0_data(const MiddleBarrierMarker &markers,
                                   int unbounded_component);

    
    void
    add_split_triangle(unsigned int v0, unsigned int v1, unsigned int v2,
                       std::vector<unsigned int> &triangle_list,
                       const vecN<HalfEdge*, 3> &triangle_half_edges);
    
    void
    add_split_triangle(const vecN<unsigned int, 3> &tri,
                       std::vector<unsigned int> &triangle_list,
                       const std::vector<bool> &vertex_flags,
                       std::map<Edge, unsigned int> &split_edge_vertices,
                       const std::map<Edge, TriangleEdgeData> &edge_counts,
                       const vecN<HalfEdge*, 3> &triangle_half_edges);

    unsigned int
    get_edge_split(const Edge &edge,
                   std::map<Edge, unsigned int> &split_edge_vertices);

    int //returns number of components
    mark_component_IDs(const_c_array<TriangleNeighbors> triangle_neighbors,
                       const_c_array<HalfEdge*> triangle_half_edges);
    

    bool //returns true if triangle was not set to a component ID yet
    mark_component_IDs(int componentID,
                       unsigned int idx,
                       const_c_array<TriangleNeighbors> triangle_neighbors,
                       const_c_array<HalfEdge*> triangle_half_edges);


    std::vector<Point> &m_pts;
    std::vector<InducedPoint> &m_ind_pts;
    std::vector<PointBase> &m_unbounded_pts;
    std::vector<SplitPoint> &m_split_ind_pts;
    std::map<int, per_winding> &m_all_per_winding_datas;
    per_winding &m_winding_zero_unbounded_components;
    per_winding &m_winding_zero_bounded_components;
    WRATHShapeSimpleTessellatorPayload::handle m_in_data;
    WRATHShapeTriangulatorPayload *m_master;

    std::vector< range_type<unsigned int> > m_contours;
    std::vector<unsigned int*> m_surrounding_contour;

    unsigned int m_current_pt;
    WRATHBBox<2> m_bbox;
    unsigned int m_unbounded_pts_begin;

    /*
      since we are adding points and thus resizing the array,
      we cannot use pointer to a WRATHShapeTriangulatorPayload::PointBase
      as the "void*" data of a vertex. So instead, we use a pointer
      to an integer, and make sure that integer is always alive,
      by putting it in a linked list. This is gross. Should look into
      
      int --> void* --> int safe-casting
     */
    std::list<unsigned int> m_pt_IDs;

    /*
      basic idea,
        whenever a combine vertex is called, first check
        if it is already in m_values, if it is return the
        premade one, otherwise make a new one.
     */
    typedef std::list<unsigned int*> point_id_list;
    std::map<ivec4, point_id_list> m_values;

    int m_split_edges; //number internal edges split total
    int m_split_triangles; //number of triangles split into 3


    //triangulation errors
    bool m_nonzero_winding_triangulation_error;
    bool m_zero_winding_triangulation_error;
  };


  class CommonFill:public WRATHTessGLU
  {
  public:
    CommonFill(PointHolder *pt_holder):
      WRATHTessGLU(tessellate_triangles_only),
      m_point_holder(pt_holder),
      m_error(false),
      m_combine_vertices_added(0),
      m_current_send(NULL),
      m_vertex_count(0)
    {}
    
    bool
    triangulation_error(void)
    {
      return m_error;
    }

    virtual
    void
    edge_flag(enum edge_type, void *polygon_data)
    {
      WRATHassert(polygon_data==NULL);
      WRATHunused(polygon_data);
    }

    
    virtual
    void 
    on_end_primitive(void *polygon_data)
    {
      WRATHassert(polygon_data==NULL);
      WRATHunused(polygon_data);
    }

    virtual
    void 
    on_error(error_type error, void *polygon_data)
    {
      WRATHassert(polygon_data==NULL);
      WRATHunused(polygon_data);
      m_error=m_error or error==tessellation_error;
    }

    virtual
    void*
    on_combine_vertex(vec2 vertex_position,
                      const_c_array<void*> vertex_source_datums,
                      const_c_array<float> vertex_weights, 
                      void *polygon_data)
    {
      WRATHassert(polygon_data==NULL);
      WRATHunused(polygon_data);
      return m_point_holder->on_combine_vertex(vertex_position, 
                                               vertex_source_datums, vertex_weights, 
                                               m_combine_vertices_added); 
    }
    
    unsigned int
    combine_vertices_added(void) const
    {
      return m_combine_vertices_added;
    }

    void
    set_current_send(std::vector<unsigned int> *p)
    {
      m_current_send=p;
    }

    /*
      every 3 vertices emit a triangle.
      However we do not wish to emit degenerate
      triangles.
     */
    virtual
    void
    on_emit_vertex(void *vertex_data, void *polygon_data)
    {
      WRATHassert(polygon_data==NULL);
      WRATHunused(polygon_data);

      unsigned int *ptr, current_vertex;
      ptr=static_cast<unsigned int*>(vertex_data);
      current_vertex=*ptr;

      m_current_triangle[m_vertex_count]=current_vertex;
      ++m_vertex_count;

      if(m_vertex_count==3)
        {
          /*
            sould we add a test to check if the triangle
            is degenerate?
           */
          m_vertex_count=0;
          m_current_send->push_back(m_current_triangle[0]);
          m_current_send->push_back(m_current_triangle[1]);
          m_current_send->push_back(m_current_triangle[2]);
        }

    }

  private:
    PointHolder *m_point_holder;
    bool m_error;
    unsigned int m_combine_vertices_added;
    std::vector<unsigned int> *m_current_send;

    vecN<unsigned int, 3> m_current_triangle;
    int m_vertex_count;
  };

  class NonZeroFill:public CommonFill
  {
  public:

    NonZeroFill(PointHolder *pt_holder,
                std::map<int, per_winding> &indices):
      CommonFill(pt_holder),
      m_all_triangle_indices(indices)
    {
       begin_polygon();
       pt_holder->add_contours(this);
       end_polygon();
    }
      

    virtual
    void 
    on_begin_primitive(enum primitive_type tp, int winding_number, void *polygon_data)
    {
      WRATHassert(polygon_data==NULL);
      WRATHunused(polygon_data);
      WRATHassert(tp==triangles);
      WRATHunused(tp);
      set_current_send(&m_all_triangle_indices[winding_number].get<0>());
    }

    virtual 
    bool 
    fill_region(int winding_number, void *polygon_data)
    {
      WRATHassert(polygon_data==NULL);
      WRATHunused(polygon_data);

      return winding_number!=0;
    }

  private:    
    std::map<int, per_winding> &m_all_triangle_indices;
  };


  class ZeroFill:public CommonFill
  {
  public:
    ZeroFill(PointHolder *pt_holder,
             std::vector<unsigned int> &indices):
      CommonFill(pt_holder)
    {
      set_current_send(&indices);
      begin_polygon();
      pt_holder->add_contours(this);
      pt_holder->add_bounding_contour(this);
      end_polygon();
    }

    ~ZeroFill(void)
    {
      /*
        ZeroFill should not add any vertices.
       */
      WRATHassert(combine_vertices_added()==0);
    }

    virtual
    void 
    on_begin_primitive(enum primitive_type tp, int winding_number, void *polygon_data)
    {
      WRATHassert(polygon_data==NULL);
      WRATHunused(polygon_data);

      WRATHassert(tp==triangles);
      WRATHunused(tp);

      WRATHassert(winding_number==1);
      WRATHunused(winding_number);
    }

        
    virtual 
    bool 
    fill_region(int winding_number, void *polygon_data)
    {
      WRATHassert(polygon_data==NULL);
      WRATHunused(polygon_data);

      return winding_number==1;
    }
  };

}


  
std::ostream&
operator<<(std::ostream &ostr, const ContourElement &E)
{
  ostr << "{" << E.m_vertex->first << "," << E.m_next_vertex 
       << "}(" << E.m_edge << ") " << *E.m_edge;
  return ostr;
}
  

//////////////////////////////////////
// PointHolder methods
PointHolder::
PointHolder(std::vector<Point> &pts,
            std::vector<InducedPoint> &ind_pts,
            std::vector<PointBase> &unbounded_pts,
            std::vector<SplitPoint> &split_ind_pts,
            std::map<int, per_winding> &all_per_winding_datas,
            per_winding &winding_zero_unbounded_components,
            per_winding &winding_zero_bounded_components,
            const WRATHShapeSimpleTessellatorPayload::handle &in_data,
            WRATHShapeTriangulatorPayload *master,
            const std::string &label):
  m_pts(pts),
  m_ind_pts(ind_pts),
  m_unbounded_pts(unbounded_pts),
  m_split_ind_pts(split_ind_pts),
  m_all_per_winding_datas(all_per_winding_datas),
  m_winding_zero_unbounded_components(winding_zero_unbounded_components),
  m_winding_zero_bounded_components(winding_zero_bounded_components),
  m_in_data(in_data),
  m_master(master),
  m_current_pt(0),
  m_split_edges(0),
  m_split_triangles(0)
{
  /*
    first compute the bounding box, if the box size is zero,
    then the shape is degenerate and the payload shall just be
    empty: no triangles, no points, no edges, etc.
   */
  generate_bounding_box();

  if(m_bbox.empty() 
     or m_bbox.length(0)==0.0f
     or m_bbox.length(1)==0.0f)
    {
      return;
    }

  int lastEndContour(0);
  m_pts.resize(number_points_needed());
  m_contours.reserve(m_in_data->tessellation().size());

  for(unsigned int outlineID=0, endOutlineID=m_in_data->tessellation().size(); 
      outlineID<endOutlineID; ++outlineID)
    {
      const WRATHShapeSimpleTessellatorPayload::TessellatedOutline::handle &O(m_in_data->tessellation()[outlineID]);

      for(unsigned int e=0, ende=O->edges().size(); e!=ende; ++e)
        {
          WRATHShapeSimpleTessellatorPayload::TessellatedEdge::handle E(O->edges()[e]);
          
          /*
            recall that TessellatedEdge's curve_point array includes both
            the begin and end of the edge, thus we can always skip the first
            point because it is the last point of the previous edge. 
          */
          for(int v=1, endV=E->curve_points().size(); v<endV; ++v, ++m_current_pt)
            {
              m_pts[m_current_pt]=WRATHShapeTriangulatorPayload::Point(m_current_pt, 
                                                                       E->curve_points()[v].position(), 
                                                                       E, v);
              m_pt_IDs.push_back(m_current_pt);
            }
        }
      m_contours.push_back(range_type<unsigned int>(lastEndContour, m_current_pt));
      lastEndContour=m_current_pt;
    }

  /*
    now create the unbounded points.
   */
  vec2 min_corner(m_bbox.min_corner());
  vec2 max_corner(m_bbox.max_corner());
  vec2 center(0.5f*(max_corner+min_corner));
  vec2 radius(0.5f*(max_corner-min_corner));

  add_box_points( center - 1.1f*radius, center + 1.1f*radius);

  
  /*
    now triangulate
   */
  triangulate(label);

  /*
    create split triangles and boundary edge data
   */
  create_split_triangles_and_edge_data();
}

void
PointHolder::
generate_bounding_box(void)
{
  for(unsigned int outlineID=0, endOutlineID=m_in_data->tessellation().size(); 
      outlineID<endOutlineID; ++outlineID)
    {
      const WRATHShapeSimpleTessellatorPayload::TessellatedOutline::handle &O(m_in_data->tessellation()[outlineID]);

      for(unsigned int e=0, ende=O->edges().size(); e!=ende; ++e)
        {
          WRATHShapeSimpleTessellatorPayload::TessellatedEdge::handle E(O->edges()[e]);
          for(int v=1, endV=E->curve_points().size(); v<endV; ++v)
            {
              vec2 pt(E->curve_points()[v].position());
              m_bbox.set_or(pt);
            }
        }
    }
}


void
PointHolder::
triangulate(const std::string &label)
{
  //now get the non-zero fills
  NonZeroFill fill_nz(this, m_all_per_winding_datas);

  m_nonzero_winding_triangulation_error=fill_nz.triangulation_error();
  if(m_nonzero_winding_triangulation_error)
    {
      WRATHwarning("Warning: triangulation for non-zero winding failed on shape \"" << label << "\"");
    }

  //get the zero fills
  ZeroFill fill_z(this, m_all_per_winding_datas[0].get<0>());
  m_zero_winding_triangulation_error=fill_nz.triangulation_error();
  if(m_zero_winding_triangulation_error)
    {
      WRATHwarning("Warning: triangulation failed for zero winding on shape \"" << label << "\"");
    }

}



void
PointHolder::
create_split_triangles_and_edge_data(void)
{
  std::map<Edge, EdgeData> all_them_edges;
  std::vector<bool> vertex_flags(m_master->total_number_points(), false);
  MiddleBarrierMarker markers;
  int winding0_unbounded_component(-1);
  

  /*

    Comment: we are implictiely assuming that no two points
    share the same position. Hope for the best.
   */

  /*
    now we need to build our edge data. An edge comprises
    of upto 2 half edges, each half edge has:
     - the opposite vertices of each triangle that uses it
     - the winding number of the triangles
    
   */ 
  for(std::map<int, per_winding>::iterator iter=m_all_per_winding_datas.begin(),
        end=m_all_per_winding_datas.end(); iter!=end; ++iter)
    {
      create_split_triangles_and_edge_data_winding_component(iter,
                                                             all_them_edges, vertex_flags,
                                                             markers, winding0_unbounded_component);
    }
      
  

  /*
    now use all_them_edges to fill up BoundaryData
   */
  for(std::map<Edge, EdgeData>::iterator iter=all_them_edges.begin(),
        end=all_them_edges.end(); iter!=end; ++iter)
    {
      Edge edge(iter->first);
      EdgeData edge_data(iter->second);

      if(edge_data.m_count==1)
        {
          edge_data.m_data[0].m_bd->push_back(BoundaryEdge());
          edge_data.m_data[0].set_boundary_edge(edge, NULL, edge_data.m_data[0].m_bd->back());
        }
      else if(edge_data.m_count==2 and edge_data.m_data[0].m_winding!=edge_data.m_data[1].m_winding)
        {
          WRATHassert(edge_data.m_data[0].m_bd!=edge_data.m_data[1].m_bd);
          WRATHassert(edge_data.m_data[0].m_bd!=NULL);
          WRATHassert(edge_data.m_data[1].m_bd!=NULL);

          edge_data.m_data[0].m_bd->push_back(BoundaryEdge());
          edge_data.m_data[1].m_bd->push_back(BoundaryEdge());
          
          BoundaryEdge &E0(edge_data.m_data[0].m_bd->back());
          BoundaryEdge &E1(edge_data.m_data[1].m_bd->back());
          
          edge_data.m_data[0].set_boundary_edge(edge, &edge_data.m_data[1], E0);
          edge_data.m_data[1].set_boundary_edge(edge, &edge_data.m_data[0], E1);
        }
    }
  

  /*
    for each winding, sort the edges into contour data
   */
  for(std::map<int, per_winding>::iterator iter=m_all_per_winding_datas.begin(),
        end=m_all_per_winding_datas.end(); iter!=end; ++iter)
    {
      sort_edges_into_contours(iter->second.get<1>(), iter->second.get<4>());
    }


  /*
    now create the data for m_winding_zero_unbounded_components
    and m_winding_zero_bounded_components
   */
  if(!m_zero_winding_triangulation_error)
    {
      create_separated_winding0_data(markers, winding0_unbounded_component);
    }

}

void
PointHolder::
create_split_triangles_and_edge_data_winding_component(std::map<int, per_winding>::iterator iter,
                                                       std::map<Edge, EdgeData> &all_them_edges,
                                                       std::vector<bool> &vertex_flags,
                                                       MiddleBarrierMarker &markers,
                                                       int &winding0_unbounded_component)
{
  c_array<unsigned int> triangles(iter->second.get<0>());
  std::vector<unsigned int> &split_triangles(iter->second.get<2>());
  std::vector<BoundaryEdge> &edges(iter->second.get<1>());
  std::map<Edge, TriangleEdgeData> edge_counts;
  int winding_number(iter->first), number_components;
  std::vector<TriangleNeighbors> triangle_neighbors(triangles.size()/3);
  std::vector<HalfEdge*> triangle_half_edges(triangles.size(), NULL); //records the half edges used by each triangle
  std::vector<component_range> &component_locations(iter->second.get<5>());
  std::vector<component_range> &split_component_ranges(iter->second.get<6>());
    
  if(triangles.empty())
    {
      return;
    }

  /*
    WRATHGLUTess appears to orient triangles
    one way for positive winding and another
    way for negative winding. We got the
    tessellator to do winding number 0
    by adding a bounding box contour, that
    made winding 0 effective winding 1.
    Thus, we reverse the triangle orders
    for triangles whenever the winding < 0.
   */
  if(winding_number<0)
    {
      for(unsigned int idx=0, t=0, endt=triangles.size()/3; t<endt; ++t, idx+=3)
        {
          std::swap( triangles[idx], triangles[idx+1]);
        }
    }
  

  split_triangles.reserve(triangles.size());

    
  for(unsigned int idx=0, t=0, endt=triangles.size()/3; t<endt; ++t, idx+=3)
    {
      vecN<unsigned int, 3> tri(triangles[idx], triangles[idx+1], triangles[idx+2]);

      for(int e=0; e<3; ++e)
        {
          int next_e( (e+1)%3), opp_e( (e+2)%3);
          unsigned int v(tri[e]), next_v(tri[next_e]), opp_v(tri[opp_e]);
          HalfEdge half_edge(v, next_v, winding_number, edges, opp_v, idx);
          Edge E(v, next_v);
          HalfEdge *hf;
          
          hf=all_them_edges[E].add_data(half_edge);
          
          //record the triangle and half edge used on this edge
          edge_counts[E].add_data(t);
          
          /*
            note the implicit ordering of triangle_half_edges:
            - edge of tri[e], tri[(e+1)%3] is recorded in triangle_half_edges[idx+e]
          */
          triangle_half_edges[idx+e]=hf; 
          WRATHassert(triangle_half_edges[idx+e]->m_triangle_location==idx);
        }
    }
  
  
  /*
    set the vertex flags as all false, vertices used by 
    any boundary edges will be set to true.
    
    Recall that Edge[0], Edge[1] name the vertice of the
    edge and TriangleEdgeData records what triangle uses 
    edge.
  */
  std::fill(vertex_flags.begin(), vertex_flags.end(), false);
  for(std::map<Edge, TriangleEdgeData>::iterator 
        e_iter=edge_counts.begin(), 
        e_end=edge_counts.end(); 
      e_iter!=e_end; ++e_iter)
    {
      WRATHassert(e_iter->second.m_count>0 and e_iter->second.m_count<3);
      
      if(e_iter->second.m_count==1)
        {
          /*
            mark the vertices of the half edge as being
            on a boundary edge
          */
          vertex_flags[ e_iter->first[0] ]=true;
          vertex_flags[ e_iter->first[1] ]=true;
        }
      else 
        {
          WRATHassert(e_iter->second.m_count==2);
          /*
            two triangles share this edge, thus they are neighbors.
          */
          triangle_neighbors[e_iter->second.m_data[0] ].add_data(e_iter->second.m_data[1]);
          triangle_neighbors[e_iter->second.m_data[1] ].add_data(e_iter->second.m_data[0]);
        }
    }
  
  MiddleBarrierMarker *markers_ptr(NULL);
  
  /*
    we need to track the connected component ID
    of each half edge made so that later we
    can realize the contours of the boundary edges
  */
  number_components=mark_component_IDs(triangle_neighbors, triangle_half_edges);
  
  if(winding_number==0)
    {
      /*
        get the unbounded component
        and then sort the triangles by component
      */
      WRATHassert(winding0_unbounded_component==-1);
      winding0_unbounded_component=compute_unbounded_component(triangles, triangle_half_edges);

      WRATHassert(winding0_unbounded_component>=0);
      WRATHassert(winding0_unbounded_component < number_components);

      sort_triangles_by_component(number_components,
                                  triangles, triangle_half_edges,
                                  winding0_unbounded_component,
                                  component_locations);
      
      //after sorting the unbouned component is the last component
      winding0_unbounded_component=component_locations.size()-1;
      
      //save the locaion at which the unbounded triangles start
      markers.m_triangle_index=component_locations.back().m_begin;
      
      WRATHassert(winding0_unbounded_component>=0);
      WRATHassert(winding0_unbounded_component<number_components);
      WRATHunused(number_components);
      
      //the function create_split_triangles takes as input a pointer to MiddleBarrierMarker
      //if non-NULL, the other fields are then filled
      markers_ptr=&markers;
      
    }  
  else
    {
      //just sort by component, there should be no unbounedd component
      WRATHassert(-1==compute_unbounded_component(triangles, triangle_half_edges));
      sort_triangles_by_component(number_components,
                                  triangles, triangle_half_edges,
                                  component_locations);
      
    }
  
  /*
    records the location of the vertices added for the split edges
  */
  std::map<Edge, unsigned int> split_edge_vertices;
  iter->second.get<3>()=
    create_split_triangles(edge_counts,
                           triangles,
                           vertex_flags, 
                           split_edge_vertices,
                           triangle_half_edges,
                           split_triangles,
                           split_component_ranges,
                           markers_ptr);
  
  WRATHassert(split_component_ranges.size()==component_locations.size());
}


void
PointHolder::
add_box_points(const vec2 &min_corner, const vec2 &max_corner)
{
  vecN<vec2, 4> pts;

  pts[0]=vec2(min_corner.x(), min_corner.y());
  pts[1]=vec2(min_corner.x(), max_corner.y());
  pts[2]=vec2(max_corner.x(), max_corner.y());
  pts[3]=vec2(max_corner.x(), min_corner.y());

  m_unbounded_pts_begin=m_current_pt;
  for(unsigned int i=0; i<4; ++i, ++m_current_pt)
    {
      m_unbounded_pts.push_back(WRATHShapeTriangulatorPayload::PointBase(m_current_pt, pts[i]));
      m_pt_IDs.push_back(m_current_pt);
      m_surrounding_contour.push_back(&m_pt_IDs.back());
    }
}

void
PointHolder::
add_bounding_contour(WRATHTessGLU *tess)
{
  tess->begin_contour();
  for(unsigned int i=0; i<4; ++i)
    {
      WRATHassert(m_unbounded_pts[i].m_ID==*m_surrounding_contour[i]);
      tess->add_vertex(m_unbounded_pts[i].m_position, m_surrounding_contour[i]);
    }
  tess->end_contour();
}

unsigned int
PointHolder::
number_points_needed(void)
{
  unsigned int return_value(0);

  for(unsigned int outlineID=0, endOutlineID=m_in_data->tessellation().size();
      outlineID<endOutlineID; ++outlineID)
    {
      const WRATHShapeSimpleTessellatorPayload::TessellatedOutline::handle &O(m_in_data->tessellation()[outlineID]);

      for(unsigned int e=0, ende=O->edges().size(); e!=ende; ++e)
        {
          WRATHShapeSimpleTessellatorPayload::TessellatedEdge::handle E(O->edges()[e]);

          if(E->curve_points().size()>1)
            {
              return_value+=E->curve_points().size() - 1;
            }
        }
    }

  return return_value;
}

void
PointHolder::
add_contours(WRATHTessGLU *tess)
{
  /*
    this is a convoluted because we need to send
    the ID as the address within an element
    within m_pt_IDs, which is a linked list.
   */
  unsigned int currentID(0);
  std::list<unsigned int>::iterator iter(m_pt_IDs.begin());

  for(unsigned int C=0, endC=m_contours.size(); C<endC; ++C)
    {
      tess->begin_contour();
      for(unsigned int pt=m_contours[C].m_begin; pt<m_contours[C].m_end; ++pt, ++currentID, ++iter)
        {
          WRATHassert(iter!=m_pt_IDs.end());
          WRATHassert(*iter==currentID);
          tess->add_vertex(m_pts[currentID].m_position, &(*iter));
        }
      tess->end_contour();
    }
}


void*
PointHolder::
on_combine_vertex(vec2 vertex_position,
                  const_c_array<void*> vertex_source_datums,
                  const_c_array<float> vertex_weights,
                  unsigned int &increment_on_create_pt)
{
  vecN<unsigned int, 4> ids;
  const_c_array<unsigned int> ids_carray(ids);
  ivec4 map_key(-1, -1, -1, -1);
  
  for(unsigned int i=0; i<vertex_source_datums.size(); ++i)
    {
      unsigned int *ptr;

      ptr=static_cast<unsigned int*>(vertex_source_datums[i]);
      ids[i]=*ptr;
      map_key[i]=*ptr;

      WRATHassert(!point(ids[i])->is_unbounded_point());
    }
  
  /*
    sort map_key:
  */
  std::sort(map_key.begin(), map_key.end());

  /*
    first see if the combine had already been done:
  */
  std::map<ivec4, point_id_list>::iterator iter;
  iter=m_values.find(map_key);
  if(iter!=m_values.end())
    {
      for(point_id_list::iterator i=iter->second.begin(), e=iter->second.end(); i!=e; ++i)
        {
          vec2 delta;

          delta=point(**i)->m_position - vertex_position;
          if(delta.L1norm()<0.00001f)
            {
              return *i;
            }
        }
    }
  
  ids_carray=ids_carray.sub_array(0, vertex_source_datums.size());
  m_ind_pts.push_back(WRATHShapeTriangulatorPayload::InducedPoint(m_current_pt, vertex_position,
                                                                  vertex_weights, ids_carray));
  
  m_pt_IDs.push_back(m_current_pt);
  m_values[map_key].push_back(&m_pt_IDs.back());
  ++m_current_pt;

  ++increment_on_create_pt;
  return &m_pt_IDs.back();
}

int
PointHolder::
mark_component_IDs(const_c_array<TriangleNeighbors> triangle_neighbors,
                   const_c_array<HalfEdge*> triangle_half_edges)
{
  int componentID(0);
  for(unsigned int t=0, endt=triangle_neighbors.size(), idx=0; t<endt; ++t, idx+=3)
    {
      if(mark_component_IDs(componentID,
                            t,
                            triangle_neighbors,
                            triangle_half_edges))
        {
          ++componentID;
        }         
    }

  #ifdef WRATHDEBUG
  {
    /*
      check that each half edge of a fixed triangle has the
      same connected component value
     */
    for(unsigned int t=0, endt=triangle_neighbors.size(), idx=0; t<endt; ++t, idx+=3)
      {
        int C(triangle_half_edges[idx]->m_connected_component_ID);

        WRATHassert(C!=-1);
        WRATHassert(triangle_half_edges[idx+1]->m_connected_component_ID==C);
        WRATHassert(triangle_half_edges[idx+2]->m_connected_component_ID==C);
      }
  }
  #endif


  return componentID;
}

bool
PointHolder::
mark_component_IDs(int componentID,
                   unsigned int t,
                   const_c_array<TriangleNeighbors> triangle_neighbors,
                   const_c_array<HalfEdge*> triangle_half_edges)
{
  unsigned int idx(3*t);

  if(triangle_half_edges[idx]->m_connected_component_ID==-1)
    {
      WRATHassert(triangle_half_edges[idx+1]->m_connected_component_ID==-1);
      WRATHassert(triangle_half_edges[idx+2]->m_connected_component_ID==-1);

      triangle_half_edges[idx+0]->m_connected_component_ID=componentID;
      triangle_half_edges[idx+1]->m_connected_component_ID=componentID;
      triangle_half_edges[idx+2]->m_connected_component_ID=componentID;
      
      for(unsigned int I=0; I<triangle_neighbors[t].m_count; ++I)
        {
          mark_component_IDs(componentID, 
                             triangle_neighbors[t].m_data[I],
                             triangle_neighbors,
                             triangle_half_edges);
        }
      return true;
    }

  return false;
}

void
PointHolder::
sort_triangles_by_component(int number_components,
                            c_array<unsigned int> src_triangles,
                            c_array<HalfEdge*> triangle_half_edges,
                            int lastC,
                            std::vector<range_type<unsigned int> > &component_location)
{
  if(lastC!=-1)
    {
      /*
        permute number_components-1 for lastC in
        the half edges
       */
      int swp(number_components-1);

      for(c_array<HalfEdge*>::iterator 
            iter=triangle_half_edges.begin(),
            end=triangle_half_edges.end();
          iter!=end; ++iter)
        {
          HalfEdge *h(*iter);
          if(h->m_connected_component_ID==swp)
            {
              h->m_connected_component_ID=lastC;
            }
          else if(h->m_connected_component_ID==lastC)
            {
              h->m_connected_component_ID=swp;
            }
        }
      lastC=number_components-1;
    }

  /*
    create a count of the number of indices that each 
    component has.
   */
  std::vector<int> component_size(number_components, 0);
  for(c_array<HalfEdge*>::iterator 
        iter=triangle_half_edges.begin(),
        end=triangle_half_edges.end();
      iter!=end; ++iter)
    {
      HalfEdge *h(*iter);
      int C(h->m_connected_component_ID);

      WRATHassert(C>=0 and C<number_components);
      ++component_size[C];
    }

  /*
    there is probably some way via iterator magic to make this
    an inplace sort, but I do not see how at this time to do it.
   */
  std::vector<unsigned int> unsorted_triangles(src_triangles.begin(), src_triangles.end());
  std::vector<HalfEdge*> unsorted_half_edges(triangle_half_edges.begin(), triangle_half_edges.end());
  std::vector<unsigned int> connected_component_loc(number_components);

  /*
    make connected_component_loc[C] = component_size[C-1] + component_size[C-2] + .. + component_size[0],
    also record this data to component_location as well
   */
  component_location.resize(number_components);
  component_location[0].m_begin=0;
  component_location.back().m_end=unsorted_triangles.size();
  connected_component_loc[0]=0;
  for(int C=1; C<number_components; ++C)
    {
      connected_component_loc[C] = connected_component_loc[C-1] + component_size[C-1];
      component_location[C].m_begin=connected_component_loc[C];
      component_location[C-1].m_end=connected_component_loc[C];
    }

  /*
    mark the location to which we write as empty as a way to
    provide some sanity checking along the way.
   */
  HalfEdge *null(NULL);
  std::fill(triangle_half_edges.begin(), triangle_half_edges.end(), null);
  std::fill(src_triangles.begin(), src_triangles.end(), m_master->total_number_points());


  for(unsigned int t=0, endt=src_triangles.size()/3, idx=0; t<endt; ++t, idx+=3)
    {
      unsigned int loc;
      int C(unsorted_half_edges[idx]->m_connected_component_ID);
      WRATHassert(C>=0 and C<number_components);

      loc=connected_component_loc[C];
      connected_component_loc[C]+=3;

      for(int v=0; v<3; ++v)
        {
          WRATHassert(unsorted_triangles[idx+v]< m_master->total_number_points());
          WRATHassert(unsorted_half_edges[idx+v]!=NULL);
          WRATHassert(unsorted_half_edges[idx+v]->m_connected_component_ID==C);
          WRATHassert(triangle_half_edges[loc+v]==NULL);
          WRATHassert(src_triangles[loc+v]==m_master->total_number_points());

          src_triangles[loc+v]=unsorted_triangles[idx+v];
          unsorted_triangles[idx+v]=m_master->total_number_points();

          triangle_half_edges[loc+v]=unsorted_half_edges[idx+v];
          unsorted_half_edges[idx+v]=NULL;

          triangle_half_edges[loc+v]->m_triangle_location=loc;

          /*
            if the point is unbounded, then the triangle MUST be
            in the unbounded component
           */
          WRATHassert(triangle_half_edges[loc+v]->m_connected_component_ID==C);
          WRATHassert(C==number_components-1 or !point(src_triangles[loc+v])->is_unbounded_point()); 
        }
    }

  #ifdef WRATHDEBUG
  {
    /*
      Check:
      - each half edge has same connected component value
      - if a point is unbouned then it must be on an unbounded component
      - if the triangle locaion is past return_value, then it must be on an unbounded component
     */
    for(int C=0; C<number_components; ++C)
      {
        WRATHassert(component_location[C].m_begin%3==0);
        WRATHassert(component_location[C].m_end%3==0);
        for(unsigned int idx=component_location[C].m_begin; idx<component_location[C].m_end; ++idx)
          {
            unsigned int t(idx/3);

            WRATHassert(triangle_half_edges[idx]!=NULL);
            WRATHassert(triangle_half_edges[idx]->m_triangle_location==3*t);
            WRATHassert(C==triangle_half_edges[idx]->m_connected_component_ID);
            WRATHassert(C==lastC or !point(src_triangles[idx])->is_unbounded_point()); 
            WRATHassert(component_location[C].m_begin <= triangle_half_edges[idx]->m_triangle_location);
            WRATHassert(component_location[C].m_end > triangle_half_edges[idx]->m_triangle_location);
          }
      }
  }
  #endif
  
}

int
PointHolder::
compute_unbounded_component(c_array<unsigned int> src_triangles,
                            c_array<HalfEdge*> triangle_half_edges)
{
  int unbounded_component(-1);

  for(unsigned int t=0, endt=src_triangles.size()/3, idx=0; unbounded_component==-1 and t<endt; ++t, idx+=3)
    {
      int C(triangle_half_edges[idx]->m_connected_component_ID);
      WRATHassert(C>=0);          

      for(int v=0; unbounded_component==-1 and v<3; ++v)
        {
          WRATHassert(C==triangle_half_edges[idx+v]->m_connected_component_ID);

          const PointBase *pt;
          pt=point( src_triangles[idx+v] );
          if(pt->is_unbounded_point())
            {
              unbounded_component=C;
            }
        }
    }

  return unbounded_component;
}


void
PointHolder::
create_separated_winding0_data(const MiddleBarrierMarker &marker,
                               int unbounded_component)
{
  /*
    Basically we just do the following:
    - _copy_ the boundary edges
    - for the unbounded BoundaryEdge data, modify the triangle and split triangle locations
    - _copy_ the triangle indices and split triangle indices
   */
  const per_winding &winding0(m_all_per_winding_datas[0]);

  WRATHassert(marker.m_triangle_index<=winding0.get<0>().size());
  WRATHassert(marker.m_split_triangle_index<=winding0.get<2>().size());


  /*
    copy BoundaryEdge data, the BoundaryEdge data
    from winding0 is already sorted first by component,
    then by contour and then by edge. The unbounded
    component is made to come last as well. Thus we are
    copying them in the _perfect_ order and all we
    need to do is create the contour ranges, along the
    way. To do that, we nest our loop first by
    contour range then by edge 
   */
  const_c_array<BoundaryEdge> src_edges(winding0.get<1>());
  for(std::vector<contour_range>::const_iterator 
        iter=winding0.get<4>().begin(),
        end=winding0.get<4>().end();
      iter!=end; ++iter)
    {
      const BoundaryEdge &firstEdge(winding0.get<1>()[iter->m_begin]);
      int C(firstEdge.m_half_edge.m_connected_component_ID);
      per_winding *dest;
      unsigned int tri_loc_offset, split_tri_loc_offset;
      unsigned int begin_contour;
      const_c_array<BoundaryEdge> bds(src_edges.sub_array(*iter));

      if(C==unbounded_component)
        {
          dest=&m_winding_zero_unbounded_components;
          tri_loc_offset=marker.m_triangle_index;
          split_tri_loc_offset=marker.m_split_triangle_index;
        }
      else
        {
          dest=&m_winding_zero_bounded_components;
          tri_loc_offset=0;
          split_tri_loc_offset=0;
        }

      begin_contour=dest->get<1>().size();

      for(const_c_array<BoundaryEdge>::iterator 
            edge_iter=bds.begin(),
            edge_end=bds.end();
          edge_iter!=edge_end; ++edge_iter)
        {
          const BoundaryEdge &B(*edge_iter);


          WRATHassert(C==B.m_half_edge.m_connected_component_ID);
          WRATHassert(B.m_half_edge.m_triangle_location>=tri_loc_offset);
          WRATHassert(B.m_half_edge.m_split_triangle_location>=split_tri_loc_offset);
          
          #ifdef WRATHDEBUG
          for(int 
                idx0=B.m_half_edge.m_triangle_location, 
                idx1=B.m_half_edge.m_split_triangle_location, 
                v=0; 
              v<3; ++v, ++idx0, ++idx1)
            {
              int vertex_id0(winding0.get<0>()[idx0]);
              int vertex_id1(winding0.get<2>()[idx1]);

              WRATHassert(C==unbounded_component or !point(vertex_id0)->is_unbounded_point());
              WRATHassert(C==unbounded_component or !point(vertex_id1)->is_unbounded_point());
            }
          #endif

          dest->get<1>().push_back(B);
          dest->get<1>().back().m_half_edge.m_triangle_location-=tri_loc_offset;
          dest->get<1>().back().m_half_edge.m_split_triangle_location-=split_tri_loc_offset;
          dest->get<1>().back().m_contour_ID=dest->get<4>().size();

          /*
            we do NOT need to modify the triangle location of the
            m_neighbor_half_edge field because the other side of the
            half edge comes from a non-zero winding fill componenet.
           */
        }

      dest->get<4>().push_back(contour_range(begin_contour,
                                             dest->get<1>().size()) );
    }
  
  /*
    set the split point ranges
   */
  m_winding_zero_bounded_components.get<3>().m_begin=winding0.get<3>().m_begin;
  m_winding_zero_bounded_components.get<3>().m_end=marker.m_split_points_mark;


  m_winding_zero_unbounded_components.get<3>().m_begin=marker.m_split_points_mark;
  m_winding_zero_unbounded_components.get<3>().m_end=winding0.get<3>().m_end;


  


  /*
    copy triangle indices
   */
  m_winding_zero_bounded_components.get<0>().resize(marker.m_triangle_index);
  std::copy(winding0.get<0>().begin(), winding0.get<0>().begin()+marker.m_triangle_index, 
            m_winding_zero_bounded_components.get<0>().begin());
  
  m_winding_zero_bounded_components.get<2>().resize(marker.m_split_triangle_index);
  std::copy(winding0.get<2>().begin(), winding0.get<2>().begin()+marker.m_split_triangle_index, 
            m_winding_zero_bounded_components.get<2>().begin());
  
  unsigned int number_unbounded_tris(winding0.get<0>().size() - marker.m_triangle_index);
  unsigned int number_unbounded_split_tris(winding0.get<2>().size() - marker.m_split_triangle_index);
  
  
  m_winding_zero_unbounded_components.get<0>().resize(number_unbounded_tris);
  std::copy(winding0.get<0>().begin()+marker.m_triangle_index, winding0.get<0>().end(), 
            m_winding_zero_unbounded_components.get<0>().begin());
  
  m_winding_zero_unbounded_components.get<2>().resize(number_unbounded_split_tris);
  std::copy(winding0.get<2>().begin()+marker.m_split_triangle_index, winding0.get<2>().end(), 
            m_winding_zero_unbounded_components.get<2>().begin());


}


unsigned int
PointHolder::
get_edge_split(const Edge &E,
               std::map<Edge, unsigned int> &split_edge_vertices)
{
  std::map<Edge, unsigned int>::iterator iter;
  

  iter=split_edge_vertices.find(E);
  if(iter!=split_edge_vertices.end())
    {
      return iter->second;
    }

  ++m_split_edges;

  /*
    create the induced point
   */
  unsigned int new_point;
  vec2 middle_half(0.5f, 0.5f);
  vec2 middle_pt;

  new_point=m_master->total_number_points();
  middle_pt=(point(E[0])->m_position + point(E[1])->m_position)*0.5f;
  SplitPoint ind_point(new_point, middle_pt,  middle_half, E);
  
  m_split_ind_pts.push_back(ind_point);
  split_edge_vertices[E]=new_point;

  return new_point;
}

void
PointHolder::
add_split_triangle(unsigned int v0, unsigned int v1, unsigned int v2,
                   std::vector<unsigned int> &triangle_list,
                   const vecN<HalfEdge*, 3> &triangle_half_edges)
{
  unsigned int triangle_loc(triangle_list.size());
  
  /*
    triangle_half_edges[e] stores the half edge used
    by the edge v[e] to v[ (e+1)%3 ]
   */

  if(triangle_half_edges[0]!=NULL)
    {
      triangle_half_edges[0]->m_split_triangle_list_location=triangle_loc;
      triangle_half_edges[0]->m_split_opposite_vertex=v2;
    }

  if(triangle_half_edges[1]!=NULL)
    {
      triangle_half_edges[1]->m_split_triangle_list_location=triangle_loc;
      triangle_half_edges[1]->m_split_opposite_vertex=v0;
    }

  if(triangle_half_edges[2]!=NULL)
    {
      triangle_half_edges[2]->m_split_triangle_list_location=triangle_loc;
      triangle_half_edges[2]->m_split_opposite_vertex=v1;
    }

  triangle_list.push_back(v0);
  triangle_list.push_back(v1);
  triangle_list.push_back(v2);
}

void
PointHolder::
add_split_triangle(const vecN<unsigned int, 3> &tri,
                   std::vector<unsigned int> &triangle_list,
                   const std::vector<bool> &vertex_flags,
                   std::map<Edge, unsigned int> &split_edge_vertices,
                   const std::map<Edge, TriangleEdgeData> &edge_counts,
                   const vecN<HalfEdge*, 3> &triangle_half_edges)
{
  for(int e=0; e<3; ++e)
    {
      int next_e( (1+e)%3), opp_e( (2+e)%3);
      Edge E(tri[e], tri[next_e]);

      /*
        we test E[0] and E[1] against the size of vertex_flags
        because those vertices might themselves be vertices
        from splitting an edge; those spit points are NOT
        tracked by vertex_flags. In addition, we do not split
        edges that include a boundary point. 
       */
      if(E[0]<vertex_flags.size() and E[1]<vertex_flags.size() 
         and vertex_flags[ E[0] ] and vertex_flags[ E[1] ]
         and !point(E[0])->is_unbounded_point() and !point(E[1])->is_unbounded_point() )
        {
          std::map<Edge, TriangleEdgeData>::const_iterator iter;

          /*
            we do not split boundary edges, only internal
            edges.
           */
          iter=edge_counts.find(E);
          WRATHassert(iter!=edge_counts.end());

          if(iter->second.m_count==2)
            {
              unsigned int edge_split;
              
              edge_split=get_edge_split(E, split_edge_vertices);

              vecN<unsigned int, 3> tri0( tri[e], edge_split, tri[opp_e]);
              vecN<HalfEdge*, 3> triangle_half_edges0(NULL, NULL, triangle_half_edges[opp_e]);

              vecN<unsigned int, 3> tri1( edge_split, tri[next_e], tri[opp_e]);
              vecN<HalfEdge*, 3> triangle_half_edges1(NULL, triangle_half_edges[next_e], NULL);
              
              /*
                make our life easier and just recurse
              */
              add_split_triangle(tri0, triangle_list, vertex_flags, split_edge_vertices, edge_counts, triangle_half_edges0);
              add_split_triangle(tri1, triangle_list, vertex_flags, split_edge_vertices, edge_counts, triangle_half_edges1);
              return;
            }
        }
    }

  
  /*
    we only split the triangle into 3 if
    each of the vertices are boundary vertices
   */
  if(tri[0]<vertex_flags.size() 
     and tri[1]<vertex_flags.size() 
     and tri[2]<vertex_flags.size() 
     and vertex_flags[ tri[0] ] 
     and vertex_flags[ tri[1] ]
     and vertex_flags[ tri[2] ] )
    {
      unsigned int new_point;
      vec3 middle_third(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f);
      vec2 middle_pt;

      //create point as center of the triangle:
      new_point=m_master->total_number_points();
      middle_pt= ( point(tri[0])->m_position + point(tri[1])->m_position + point(tri[2])->m_position )/3.0f;
      SplitPoint ind_point(new_point, middle_pt,  middle_third, tri);
      m_split_ind_pts.push_back(ind_point);

      add_split_triangle(new_point, tri[1], tri[2], triangle_list, 
                         vecN<HalfEdge*, 3>(NULL, triangle_half_edges[1], NULL) );

      add_split_triangle(tri[0], new_point, tri[2], triangle_list, 
                         vecN<HalfEdge*, 3>(NULL, NULL, triangle_half_edges[2]) );

      add_split_triangle(tri[0], tri[1], new_point, triangle_list, 
                         vecN<HalfEdge*, 3>(triangle_half_edges[0], NULL, NULL) );

      ++m_split_triangles;
    }
  else
    {
      add_split_triangle(tri[0], tri[1], tri[2], triangle_list, triangle_half_edges);
    }
}
  
range_type<unsigned int>
PointHolder::
create_split_triangles(const std::map<Edge, TriangleEdgeData> &edge_counts,
                       const_c_array<unsigned int> triangles,
                       const std::vector<bool> &vertex_flags, 
                       std::map<Edge, unsigned int> &split_edge_vertices,
                       const_c_array<HalfEdge*> triangle_half_edges,
                       std::vector<unsigned int> &out_triangles,
                       std::vector<component_range> &out_component_ranges,
                       MiddleBarrierMarker *middle_barrier_in_out)
{
  WRATHassert(!triangles.empty());

  /*
    middle_barrier_in_out is a funny little thing:
      if it is not-NULL then fill m_split_triangle_index
      and m_split_points_mark when idx==m_triangle_index
   */
  int prevC(0);
  out_component_ranges.push_back(component_range(0, 0));

  unsigned int begin_added(m_master->total_number_points());
  for(unsigned int t=0, endt=triangles.size()/3, idx=0; t<endt; ++t, idx+=3)
    {
      vecN<unsigned int, 3> tri(triangles[idx], triangles[idx+1], triangles[idx+2]);
      vecN<HalfEdge*, 3> hf(triangle_half_edges[idx], triangle_half_edges[idx+1], triangle_half_edges[idx+2]);

      WRATHassert(hf[0]!=NULL and hf[0]->m_triangle_location==idx);
      WRATHassert(hf[1]!=NULL and hf[1]->m_triangle_location==idx);
      WRATHassert(hf[2]!=NULL and hf[2]->m_triangle_location==idx);

      int C(hf[0]->m_connected_component_ID);
      WRATHassert(C==hf[1]->m_connected_component_ID);
      WRATHassert(C==hf[2]->m_connected_component_ID);
      WRATHassert(C>=prevC);
      
      if(prevC!=C)
        {
          unsigned int loc;
          
          loc=out_triangles.size();
          out_component_ranges.back().m_end=loc;
          out_component_ranges.push_back(component_range(loc, loc));
          prevC=C;
        }

      if(middle_barrier_in_out!=NULL and middle_barrier_in_out->m_triangle_index==idx)
        {
          /*
            record to location pointed to by middle_barrier_in_out
            and then set the pointer to NULL so we do not 
            record twice.
           */
          middle_barrier_in_out->m_split_triangle_index=out_triangles.size();
          middle_barrier_in_out->m_split_points_mark=m_master->total_number_points();
          middle_barrier_in_out=NULL;
        }

      add_split_triangle(tri, out_triangles, vertex_flags, split_edge_vertices, edge_counts, hf);
    }
  out_component_ranges.back().m_end=out_triangles.size();
  return range_type<unsigned int>(begin_added, m_master->total_number_points());
}


void
PointHolder::
sort_edges_into_contours(c_array<BoundaryEdge> edges, 
                         std::vector<contour_range> &C)
{
  

  if(edges.empty())
    {
      return;
    }

  /*
    basic idea:
     - first sort by contour ID, which is not a contour ID
       but actually a connected component ID
     - then call sort_contour() which will set both the
       contour ID and the contour edge ID
   */

  /*
    first sort, the sorting is done by the conneced component ID
    and the contour creating is done in connected component chunks
    by doing so, a vertex is used always exatly twice within one
    connected component chunk.
  */
  std::sort(edges.begin(), edges.end(), BoundaryEdgeConnectedComponentIDComparer());

  //now sort each sub-range of where the connected component ID does not vary:
  int last_id=edges[0].m_half_edge.m_connected_component_ID;
  unsigned int last_end=0;
  unsigned int contourID(0);
  range_type<unsigned int> R;

  for(int E=1, endE=edges.size(); E<endE; ++E)
    {
      if(edges[E].m_half_edge.m_connected_component_ID!=last_id)
        {
          R=range_type<unsigned int>(last_end, E);
          sort_into_contours(contourID, edges.sub_array(R));
          last_end=E;
          last_id=edges[E].m_half_edge.m_connected_component_ID;
        }
    }
  R=range_type<unsigned int>(last_end, edges.size());
  sort_into_contours(contourID, edges.sub_array(R));

  /*
    now that all Edges have been marked, now sort by 
     - first by contour 
     - second by edge id along contour

    Note that we will get implictely for free
    also sorted by connected component because
    we build the contour ID's by connected
    component and thus we know that
    if the edge_a.m_connected_component < edge_b.m_connected_component
    then automatically a.m_contour < b.m_contour
  */
  std::sort(edges.begin(), edges.end(), BoundaryEdgeContourComparer());

  /*
    now the edges should be sorted as follows:
    - first by connected component
    - second by contour ID
    - third by countour edge ID

    all that remains is to build C.
  */
  C.reserve(contourID);
  last_id=edges[0].m_contour_ID;
  last_end=0;
  for(int E=1, endE=edges.size(); E<endE; ++E)
    {
      if(edges[E].m_contour_ID!=last_id)
        {
          C.push_back(contour_range(last_end, E));
          last_id=edges[E].m_contour_ID;
          last_end=E;
        }
    }
  C.push_back(contour_range(last_end, edges.size()));
}
  
void
PointHolder::
sort_into_contours(unsigned int &contourID, c_array<BoundaryEdge> edges)
{
  WRATHassert(!edges.empty());

  /*
    vertex_neighbors[v] stores the edges that use the vertex,
    a vertex however might be used by many, many, edges,
    that is why we use std::vector<> to store what edges.
   */
  std::map<unsigned int, VertexUserList> vertex_users;
  for(unsigned int E=0, endE=edges.size(); E<endE; ++E)
    {
      vertex_users[ edges[E].m_v0 ].push_back(E);
      vertex_users[ edges[E].m_v1 ].push_back(E);
    }

  /*
    building a contour means walking vertex_users
    until each element is empty:
   */
  for(std::map<unsigned int, VertexUserList>::iterator 
        iter=vertex_users.begin(), end=vertex_users.end();
      iter!=end; ++iter)
    {
      while(!iter->second.empty())
        {
           /*
             current_contour is a list of edge ID's and vertex ID's 
             for the contour we are building
           */
          std::vector<ContourElement> current_contour;
          
          /*
            vertex_branch_points[v] gives the index into current_contour
            where that vertex was last encountered.
          */
          std::map<unsigned int, unsigned int> vertex_branch_points;
          
          build_contour_at(contourID,
                           iter,
                           edges, vertex_users,
                           current_contour, vertex_branch_points);
        }
    }

  
}

void
PointHolder::
build_contour_at(unsigned int &contourID,
                 std::map<unsigned int, VertexUserList>::iterator iter,
                 c_array<BoundaryEdge> edges,
                 std::map<unsigned int, VertexUserList> &vertex_users,
                 std::vector<ContourElement> &current_contour,
                 std::map<unsigned int, unsigned int> &vertex_branch_points)
{
  
    
  /*
    build a contour that starts at iter->first.

    The catch we need to deal with are those 
    vertices that branch to more than two edges;
    our strategy for dealing with them is as follows:
    - pick any edge that is not yet part of a contour
    - if we end up meeting a vert already in our list, that
      is not the head, then that vertex should also be
      a multi-branching vertex, in that case then form a contour
      from the 1st time at that multi-branch vertex to it's next
      showing up, and then continue again from the 1st time that
      multi-branch vertex appeared.
  */
  
  
  /*
    find an unused edge
   */
  unsigned int E, next_vertex;

  
  /*
    if there is more than two edges left on the 
    vertex to start at then it is a multi-branch point.
   */
  WRATHassert(!iter->second.empty());
  if(iter->second.size()>2)
    {
      vertex_branch_points[iter->first]=current_contour.size();
    }
  
  //grab the first edge not part of a contour
  //and not getting processed right now.
  do
    {
      E=iter->second.back();
      iter->second.pop_back();
    }
  while(edges[E].m_contour_ID!=BOUNDARY_EDGE_UNTOUCHED and !iter->second.empty());

  
  if(edges[E].m_contour_ID!=BOUNDARY_EDGE_UNTOUCHED)
    {
      /*
        all the edges in iter->second were already used,
        nothing to do then. In that case there had
        better not be a contour getting built
       */
      WRATHassert(current_contour.empty());
      return;
    }

  //mark the edge as going to be used by a contour
  edges[E].m_contour_ID=BOUNDARY_EDGE_BEING_PROCESSED;

  WRATHassert( edges[E].m_v0==iter->first or edges[E].m_v1==iter->first);
  next_vertex=( edges[E].m_v0==iter->first )?
    edges[E].m_v1:
    edges[E].m_v0;

  ContourElement C;
  C.m_edge=&edges[E];
  C.m_vertex=iter;
  C.m_next_vertex=next_vertex;

  current_contour.push_back(C);

  if(current_contour.back().m_next_vertex == current_contour.front().m_vertex->first)
    {
      build_contour(contourID, current_contour);
      return;
    }


  /*
    now check if next_vertex is already in
    vertex_branch_points, if it is, the elements
    from where it starts to the end from a contour.
   */
  std::map<unsigned int, unsigned int>::iterator multi_iter;
  multi_iter=vertex_branch_points.find(next_vertex);

  if(multi_iter!=vertex_branch_points.end())
    {
      /*
        there for the range of elements [multi_iter->second, current_contout.end())
        form a contour:
       */
      c_array<ContourElement> sub_range(current_contour);
      sub_range=sub_range.sub_array(range_type<unsigned int>(multi_iter->second, sub_range.size()));

      build_contour(contourID, sub_range);
      current_contour.resize(multi_iter->second);  

      /*
        note that we resize to remove the the branch point
        vertex, by doing so when it recurses below
        we will be have the contour just before the branching
        and the back() element stores the vertex to use.
       */
    }
 
 
  build_contour_at(contourID,
                   vertex_users.find(current_contour.back().m_next_vertex),
                   edges, vertex_users, current_contour,
                   vertex_branch_points);
  
}

void
PointHolder::
build_contour(unsigned int &contourID,
              c_array<ContourElement> elements)
{

  WRATHassert(!elements.empty());
  int C=elements[0].m_edge->m_half_edge.m_connected_component_ID;
  WRATHassert(C!=-1);
  WRATHunused(C);


  bool reverse_vertex_order;

  /*
    we make the value of m_edge->m_contour_edge_ID
    so that it walks along the edge in the correct
    oreintation 
   */
  reverse_vertex_order=elements.size()>1 
    and elements[0].m_edge->m_v1 != elements[1].m_edge->m_v0;


  for(int vv=0, 
        end_vv=elements.size(), 
        ii=(reverse_vertex_order)?end_vv-1:0,
        incr_ii=(reverse_vertex_order)?-1:1;
      vv<end_vv; ++vv, ii+=incr_ii)
    {
      WRATHassert(elements[vv].m_edge->m_contour_ID==BOUNDARY_EDGE_BEING_PROCESSED);
      WRATHassert(elements[vv].m_edge->m_half_edge.m_connected_component_ID==C);
      elements[vv].m_edge->m_contour_ID=contourID;
      elements[vv].m_edge->m_contour_edge_ID=ii;
    }
  ++contourID;
}

void
PointHolder::
check_triangle_consistency_ignore_order(unsigned int triangle_loc,
                                        const_c_array<unsigned int> triangle_indices,
                                        unsigned int v0, unsigned int v1, unsigned int v2)
{
  WRATHassert(triangle_loc<triangle_indices.size());

  vecN<unsigned int, 3> a(v0, v1, v2);
  vecN<unsigned int, 3> b(triangle_indices[triangle_loc],
                          triangle_indices[triangle_loc+1],
                          triangle_indices[triangle_loc+2]);

  std::sort(a.begin(), a.end());
  std::sort(b.begin(), b.end());
  WRATHassert(a==b);
}

void
PointHolder::
check_triangle_consistency(unsigned int triangle_loc,
                           const_c_array<unsigned int> triangle_indices,
                           unsigned int v0, unsigned int v1, unsigned int v2)
{
  WRATHassert(triangle_loc<triangle_indices.size());

  vecN<unsigned int, 3> a(v0, v1, v2);
  vecN<unsigned int, 3> b(triangle_indices[triangle_loc],
                          triangle_indices[triangle_loc+1],
                          triangle_indices[triangle_loc+2]);
  vecN<unsigned int, 3> c(b);

  /*
    we need to make triangle a and c "start" on the same
    edge
   */
  for(unsigned int e=0; c[0]!=a[0] and e<3; ++e)
    {
      if(a[0]==b[e])
        {
          /*
            make c be:
              b[e], b[e+1], b[e+2]
           */
          for(int k=0; k<3; ++k)
            {
              c[k] = b[ (e+k)%3 ];
            }
          WRATHassert(c[0]==a[0]);
        }
    }
  
  WRATHassert(a==c);
     
}

void
PointHolder::
check_filled_component(const std::map<int, FilledComponent> &others,
                       const FilledComponent &C,
                       const WRATHShapeTriangulatorPayload *payload)
{
  if(C.triangle_indices().empty())
    {
      WRATHassert(C.boundary_edges().empty());
      WRATHassert(C.split_triangulation_indices().empty());
      WRATHassert(C.number_contours()==0);
      WRATHunused(payload);
      return;
    }


  
  /*
    make sure that the boundary edge data makes sense.
   */
  WRATHassert(C.valid());
  for(const_c_array<BoundaryEdge>::iterator iter=C.boundary_edges().begin(),
        end=C.boundary_edges().end(); iter!=end; ++iter)
    {
      const BoundaryEdge &B(*iter);

      check_triangle_consistency(B.m_half_edge.m_triangle_location, 
                                 C.triangle_indices(), 
                                 B.m_v0, B.m_v1, B.m_half_edge.m_opposite_vertex);

      check_triangle_consistency(B.m_half_edge.m_split_triangle_location, 
                                 C.split_triangulation_indices(), 
                                 B.m_v0, B.m_v1, B.m_half_edge.m_split_opposite_vertex);
      

      if(B.m_neighbor.first)
        {
          std::map<int, FilledComponent>::const_iterator miter;
          miter=others.find(B.m_neighbor.second);

          WRATHassert(miter!=others.end());

          /*
            triangulation ordering across different winding numbers
            is not alway consistent, thus we juch check that
            the triangles are the same, but ignore the orientation.
            Regardless though, since the BoundaryEdge does store
            the location of the neighbor triangle, a user can still
            get the boundary triangle orientation.

            Winding0 triangles usually seem to be the culprit.
           */
          check_triangle_consistency_ignore_order(B.m_neighbor_half_edge.m_triangle_location, 
                                                  miter->second.triangle_indices(), 
                                                  B.m_v0, B.m_v1, 
                                                  B.m_neighbor_half_edge.m_opposite_vertex);
          
          check_triangle_consistency_ignore_order(B.m_neighbor_half_edge.m_split_triangle_location, 
                                                  miter->second.split_triangulation_indices(),
                                                  B.m_v0, B.m_v1, 
                                                  B.m_neighbor_half_edge.m_split_opposite_vertex);
        }
    }

  for(int contour=0, end_contour=C.number_contours(); contour<end_contour; ++contour)
    {
      for(int subE=0, end_subE=C.contour(contour).size(); subE<end_subE; ++subE)
        {
          int nextE(subE+1);
          nextE=(nextE==end_subE) ? 
            0: 
            nextE;

          const BoundaryEdge &a(C.contour(contour)[subE]);
          const BoundaryEdge &b(C.contour(contour)[nextE]);

          /*
            make sure that the end vertex of a 
            is the start vertex of b
           */
          WRATHassert(a.m_v1==b.m_v0);

          WRATHassert(a.m_contour_ID==contour);
          WRATHassert(a.m_contour_edge_ID==subE);
          WRATHassert(&a==&C.contour(a.m_contour_ID)[a.m_contour_edge_ID]);

          WRATHunused(a);
          WRATHunused(b);
        }
    }
}


//////////////////////////////////////
// WRATHShapeTriangulatorPayload::InducedPoint methods
WRATHShapeTriangulatorPayload::InducedPoint::
InducedPoint(int ID, const vec2 &p,
             const_c_array<float> pconvex_coeff,
             const_c_array<unsigned int> pt_source_ids):
  PointBase(ID, p),
  m_count(pt_source_ids.size())
{
  WRATHassert(pconvex_coeff.size()==pt_source_ids.size());
  WRATHassert(pt_source_ids.size()<=4);

  std::copy(pconvex_coeff.begin(), pconvex_coeff.end(), m_convex_coeff.begin());
  std::copy(pt_source_ids.begin(), pt_source_ids.end(), m_sources_ids.begin());
}




///////////////////////////////////////
// WRATHShapeTriangulatorPayload methods
WRATHShapeTriangulatorPayload::
WRATHShapeTriangulatorPayload(const WRATHShapeSimpleTessellatorPayload::handle &in_data,
                              const std::string &label)
{
  
  m_datum=WRATHNew DatumKeeper(in_data);

  WRATHassert(in_data.valid());
  /*
    Ctor of PointHolder does all the work
    of creating points, boundary data, triangulation, etc.
   */
  PointHolder wk(m_datum->m_pts, 
                 m_datum->m_induced_pts, 
                 m_datum->m_unbounded_pts, 
                 m_datum->m_split_induced_pts,
                 m_datum->m_all_per_winding_datas,
                 m_datum->m_winding_zero_unbounded_components,
                 m_datum->m_winding_zero_bounded_components,
                 in_data, this, label);

  
     
  extract_component_data();
    
}

WRATHShapeTriangulatorPayload::
~WRATHShapeTriangulatorPayload()
{
}

void
WRATHShapeTriangulatorPayload::
set_filled_component(FilledComponent &C, 
                     int winding,
                     per_winding &d)
{
  
  

  C.m_array_keeper=m_datum;
  C.m_winding_number=winding;
  C.m_triangle_indices=const_c_array<unsigned int>(d.get<0>());
  C.m_split_triangulation_indices=const_c_array<unsigned int>(d.get<2>());
  C.m_boundary_edges=const_c_array<BoundaryEdge>(d.get<1>());
  C.m_split_points_range=range_type<unsigned int>(d.get<3>());
  C.m_contours=const_c_array<contour_range>(d.get<4>());
  C.m_component_ranges=const_c_array<contour_range>(d.get<5>());
  C.m_split_component_ranges=const_c_array<contour_range>(d.get<6>());
}

void
WRATHShapeTriangulatorPayload::
extract_component_data(void)
{
  /*
    now fill m_components
   */
  for(std::map<int, per_winding>::iterator iter=m_datum->m_all_per_winding_datas.begin(),
        end=m_datum->m_all_per_winding_datas.end(); iter!=end; ++iter)
    {
      set_filled_component(m_components[iter->first], iter->first, iter->second);
    }

    set_filled_component(m_winding_zero_unbounded_components, 0, m_datum->m_winding_zero_unbounded_components);
    set_filled_component(m_winding_zero_bounded_components, 0, m_datum->m_winding_zero_bounded_components);

  #ifdef WRATHDEBUG
  {
    for(std::map<int, FilledComponent>::const_iterator iter=m_components.begin(),
          end=m_components.end(); iter!=end; ++iter)
      {
        PointHolder::check_filled_component(m_components, iter->second, this);
      }

   
      PointHolder::check_filled_component(m_components, m_winding_zero_unbounded_components, this);
      PointHolder::check_filled_component(m_components, m_winding_zero_bounded_components, this);
   
  }
  #endif
}


//////////////////////////////////////////////////
// WRATHShapeTriangulatorPayload::DatumKeeper methods
const WRATHShapeTriangulatorPayload::PointBase*
WRATHShapeTriangulatorPayload::DatumKeeper::
point(unsigned int I) const 
{
  unsigned int pt_sz(m_pts.size());
  
  if(I < pt_sz)
    {
      return &m_pts[I];
    }
    
  I-=pt_sz;
  pt_sz=m_unbounded_pts.size();
  if(I < pt_sz)
    {
      return &m_unbounded_pts[I];
    }
  
  I-=pt_sz;
  pt_sz=m_induced_pts.size();
  if(I<pt_sz)
    {
      return &m_induced_pts[I];
    }

  I-=pt_sz;
  return &m_split_induced_pts[I];
}



//////////////////////////////////
// WRATHShapeTriangulatorPayload::BoundaryEdge routines  
WRATHShapeTriangulatorPayload::BoundaryEdge::
BoundaryEdge(void):
  m_v0(0),
  m_v1(0),
  m_contour_ID(BOUNDARY_EDGE_UNTOUCHED),
  m_contour_edge_ID(-1),
  m_neighbor(false, 0)
{}


std::ostream&
operator<<(std::ostream &ostr, const WRATHShapeTriangulatorPayload::BoundaryEdge &bd)
{
  ostr << "[" << bd.m_v0
       << "," << bd.m_v1
       << "](C=" << bd.m_contour_ID
       << " V=" << bd.m_contour_edge_ID 
       << " #=" << bd.m_half_edge.m_connected_component_ID  
       << " t=" << bd.m_half_edge.m_triangle_location
       << ")";
  return ostr;
}


  
