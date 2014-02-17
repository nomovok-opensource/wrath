/*! 
 * \file WRATHShapeSimpleTessellator.hpp
 * \brief file WRATHShapeSimpleTessellator.hpp
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


#ifndef WRATH_HEADER_SHAPE_SIMPLE_TESSELLATOR_HPP_
#define WRATH_HEADER_SHAPE_SIMPLE_TESSELLATOR_HPP_

#include "WRATHConfig.hpp"
#include "WRATHBBox.hpp"
#include "WRATHShape.hpp"
#include "WRATHInterleavedAttributes.hpp"


/*! \addtogroup Shape
 * @{
 */

/*!\class WRATHShapeSimpleTessellatorPayload
  A WRATHShapeSimpleTessellatorPayload is the payload
  of a WRATHShapeSimpleTessellator<T>. It consists
  of a sequence of points, each point having
  a normal vector.
 */
class WRATHShapeSimpleTessellatorPayload:
  public WRATHReferenceCountedObjectT<WRATHShapeSimpleTessellatorPayload>
{
public:
  
  /*!\class PayloadParams 
    Parameter object to specify how a 
    WRATHShapeSimpleTessellator tessellates.
  */
  class PayloadParams
  {
  public:
    /*!\typedef PayloadType
      Type that uses this paramter type
     */
    typedef WRATHShapeSimpleTessellatorPayload PayloadType;
    
    /*!\fn PayloadParams
      Ctor initializes \ref m_curve_tessellation as 60 and
      \ref m_max_recurse as 4
     */
    PayloadParams(void):
      m_curve_tessellation(60), //a circle is then tessellated to 60 pts.
      m_max_recurse(4)
    {}
    
    /*!\fn PayloadParams& max_recurse
      Sets \ref m_max_recurse.
      \param v value to use
    */
    PayloadParams&
    max_recurse(int v)
    {
      m_max_recurse=v;
      return *this;
    }
    
    /*!\fn PayloadParams& curve_tessellation
      Sets \ref m_curve_tessellation.
      \param v value to use
    */
    PayloadParams&
    curve_tessellation(unsigned int v)
    {
      m_curve_tessellation=v;
      return *this;
    }
    
    /*!\fn float curve_tessellation_threshhold
      Returns the cumalative curve threshhold 
      before another point is added for tessellation,
      i.e. returns (2*PI)/(\ref m_curve_tessellation).
    */
    float
    curve_tessellation_threshhold(void) const
    {
      return (2.0f*static_cast<float>(M_PI))/static_cast<float>(std::max(1u,m_curve_tessellation)); 
    }
    
    /*!\fn bool operator==(const PayloadParams&) const
      comparison operator
      \param rhs value to which to compare
    */
    bool
    operator==(const PayloadParams &rhs) const
    {
      return m_max_recurse==rhs.m_max_recurse
        and m_curve_tessellation==rhs.m_curve_tessellation;
    }

    /*!\var m_curve_tessellation
      When tessellating curves, a WRATHShapeStroker
      computes the curve curvature along the curve.
      (for reference the integral of the curve
      curvature along any circle against arc-length
      is 2*PI). This value controls how much the
      curve curvatuve  is allowed to accumulate
      between points when tessellating a curve.
      The value is given in number of points per
      2*PI curvature (for example a value of N
      means a circle is tesselated to N points).
    */
    unsigned int m_curve_tessellation;
    
    /*!\var m_max_recurse
      Maximum number of recursion levels 
      of tessellation to perform
      when stroking paths. In particular,
      the maximum number of points a 
      path is decomposed into is
      \f$ 1+ 2^{m\_max\_recurse} \f$.
      Default value is 4
    */
    int m_max_recurse;
  };


  /*!\class CurvePoint
    A CurvePoint represents a point on the
    interpolate between two points of a 
    WRATHOutline. A CurvePoint holds the 
    position of the point together with 
    vector perpindicular to the curve 
    created by the interpolator at that point.
   */
  class CurvePoint:
    public WRATHInterleavedAttributes<vec2, vec2, float>
  {
  public:
    
    enum
      {
        /*!
          Location within the tuple for the position
          of the point. The position is a vec2.
         */
        position_location=0,

        /*!
          Location within the tuple for the
          normalized direction. That direction
          is perpindicular to the edge at the
          point. 
         */
        normal_location=1,

        /*!
          Location within the tuple of the
          interpolation (i.e. time distance)
          where the point is located along 
          the edge.
         */
        time_location=2,
      };

    /*!\fn CurvePoint(void)
      Empty ctor, does not intialize CurvePoint values.
     */
    CurvePoint(void)
    {}

    /*!\fn CurvePoint CurvePoint(const vec2&, const vec2&, float)
      Ctor.
      \param pt position of point
      \param dir normal vector to edge at the point
      \param t "when" along the edge, 0.0 indicates the start and 1.0 indicates the end
     */
    CurvePoint(const vec2 &pt, const vec2 &dir, float t)
    {
      position()=pt;
      normal()=dir;
      time()=t;
    }

    /*!\fn vec2& position(void)
      Returns a reference to the position of the point.
      Equivalent to
      \code
      get<position_location>()
      \endcode
     */
    vec2&
    position(void) 
    {
      return get<position_location>();
    }
    
    /*!\fn vec2& position(void) const
      Returns the position of the point.
      Equivalent to
      \code
      get<position_location>()
      \endcode
     */
    const vec2&
    position(void) const
    {
      return get<position_location>();
    }
        
    /*!\fn vec2& normal(void)
      Returns a reference to the normal vector of the point.
      Equivalent to
      \code
      get<normal_location>()
      \endcode
     */    
    vec2&
    normal(void) 
    {
      return get<normal_location>();
    }
    
    /*!\fn const vec2& normal(void) const
      Returns a the normal vector of the point.
      Equivalent to
      \code
      get<normal_location>()
      \endcode
     */    
    const vec2&
    normal(void) const
    {
      return get<normal_location>();
    }

    /*!\fn float& time(void)
      Returns a reference to the interpolate of the point,
      i.e. the "time distance" of the point between the
      two points of the WRATHOutline from which the point
      comes.
      Equivalent to
      \code
      get<time_location>()
      \endcode
     */    
    float& 
    time(void) 
    {
      return get<time_location>();
    }

    /*!\fn float time(void) const
      Returns a the interpolate of the point,
      i.e. the "time distance" of the point between the
      two points of the WRATHOutline from which the point
      comes.
      Equivalent to
      \code
      get<time_location>()
      \endcode
     */    
    float 
    time(void) const
    {
      return get<time_location>();
    }

    /*!\fn vec2 direction(void) const
      Returns the normalized tangent vector
      at the point (i.e. the unit vector tangent
      to the edge at the point).
     */
    vec2
    direction(void) const
    {
      vec2 v;

      v.x()=normal().y();
      v.y()=-normal().x();
      return v;
    }
  };

  /*!\class TessellatedEdge
    A TessellatedEdge represents the tessellation
    of an edge of a WRATHOutline. The points
    are stored in order along the tessellation
    of the edge and includes both end points of 
    the edge.
   */
  class TessellatedEdge:
    public WRATHReferenceCountedObjectT<TessellatedEdge>
  {
  public:
    /*!\fn TessellatedEdge
      Ctor. Creates a TessellatedEdge.
      \param id the edge ID of the WRATHOutline 
      \param next_id the edge ID of the next edge of the WRATHOutline
      \param poutlineID the ID of the WRATHOutline
      \param pts a _reference_ of vector of CurvePoint POD's.
                 The vector is swapped with that of the created
                 TessellatedEdge, not copied.
      \param indices a _reference_ of a vector of GLushort's giving
                     the line pair indices for the edge (i.e. for 
                     example to draw with GL_LINES). The vector is
                     swapped with that of the created TessellatedEdge, 
                     not copied.
     */
    TessellatedEdge(int id, int next_id,
                    std::vector<CurvePoint> &pts,
                    std::vector<GLushort> &indices,
                    int poutlineID):
      m_point_id(id),
      m_next_point_id(next_id),
      m_outlineID(poutlineID)
    {
      std::swap(m_curve_line_indices, indices);
      std::swap(m_curve_points, pts);
      compute_bounding_box();
    }

    /*!\fn  const std::vector<CurvePoint>& curve_points
      Returns the points created by the
      interpolates of the edge of a
      WRATHOutline. The points
      are stored in order along the 
      tessellation of the edge and includes 
      both end points of the edge.
    */
    const std::vector<CurvePoint>&
    curve_points(void) const
    {
      return m_curve_points;
    }
    
    /*!\fn const std::vector<GLushort>& curve_line_indices
      Returns the indices into \ref curve_points()
      for use with GL_LINES for drawing the edge
      of a \ref WRATHOutline.
    */
    const std::vector<GLushort>&
    curve_line_indices(void) const
    {
      return m_curve_line_indices;
    }

    /*!\fn int point_id
      Returns the point index to feed to
      \ref WRATHOutline::pt() to
      get the starting point of the
      edge.
     */
    int
    point_id(void) const
    {
      return m_point_id;
    }

    /*!\fn next next_point_id
      Returns the point index to feed to
      \ref WRATHOutline::pt() to
      get the ending point of the
      edge.
     */
    int
    next_point_id(void) const
    {
      return m_next_point_id;
    }

    /*!\fn int outlineID
      Returns the outline ID of the outline
      of the edge, see \ref WRATHShape::outline().
     */
    int
    outlineID(void)
    {
      return m_outlineID;
    }

    /*!\fn const WRATHBBox<2>& bounding_box
      The bounding box of the edge.
    */
    const WRATHBBox<2>&
    bounding_box(void) const
    {
      return m_box;
    }

  private:
    
    void 
    compute_bounding_box(void);

    std::vector<CurvePoint> m_curve_points;
    std::vector<GLushort> m_curve_line_indices;
    int m_point_id, m_next_point_id, m_outlineID;
    WRATHBBox<2> m_box;
  };

  /*!\class TessellatedOutline
    A TessellatedOutline represents the tesselation
    of a WRATHOutline, essentially a collection 
    of TessellatedEdge's.
   */
  class TessellatedOutline:
    public WRATHReferenceCountedObjectT<TessellatedOutline>
  {
  public:

    /*!\fn TessellatedOutline
      Ctor.
      \param ID WRATHOutline ID that the created TessellatedOutline represents
      \param pedges a _reference_ to a vector of TessellatedEdge::handle 's.
                    The vector is swapped with that of the created
                    TessellatedOutline, not copied.
     */
    explicit
    TessellatedOutline(int ID, std::vector<TessellatedEdge::handle> &pedges):
      m_outlineID(ID)
    {
      std::swap(pedges, m_edges);
      if(!m_edges.empty())
        {
          m_edge_to_last_point=(m_edges.size()>1)?
            m_edges[m_edges.size()-2]:
            m_edges[0];
        }
      compute_bounding_box();
    }
                       

    /*!\fn int outlineID
      Returns the outline ID of the outline,
      this ID can be used to fetch the WRATHOutline
      from the WRATHShape, see \ref WRATHShape::outline().
     */
    int
    outlineID(void)
    {
      return m_outlineID;
    }

    /*!\fn const std::vector<TessellatedEdge::handle>& edges
      The actual tessellated edges of the
      WRATHOutline.
     */
    const std::vector<TessellatedEdge::handle>&
    edges(void) const
    {
      return m_edges;
    }

    /*!\fn const TessellatedEdge::handle& edge_to_last_point
      Returns the edge that ends at the last point
      of the WRATHOutline. This edge is usually 
      NOT edges().back() because edges().back() is
      the edge from the last point of the outline
      to the first point of the outline.
     */
    const TessellatedEdge::handle&
    edge_to_last_point(void) const
    {
      return m_edge_to_last_point;
    }

    /*!\fn const WRATHBBox<2>& bounding_box
      Return the bounding box of the outline.
    */
    const WRATHBBox<2>&
    bounding_box(void) const
    {
      return m_box;
    }
  private:
    
    void
    compute_bounding_box(void);

    std::vector<TessellatedEdge::handle> m_edges;
    TessellatedEdge::handle m_edge_to_last_point;
    int m_outlineID;
    WRATHBBox<2> m_box;
  };

  
  /*!\fn WRATHShapeSimpleTessellatorPayload
    Ctor. Create a WRATHShapeSimpleTessellatorPayload from
    tessellation parameters and a WRATHShape<T>
    \param pshape WRATHShape<T> to tessellate
    \param pp tessellation parameters 
   */
  template<typename T>
  WRATHShapeSimpleTessellatorPayload(const WRATHShape<T> &pshape,
                                     const PayloadParams &pp=PayloadParams());

  /*!\fn const WRATHBBox<2>& bounding_box
    Return the bounding box of the payload.
   */
  const WRATHBBox<2>&
  bounding_box(void) const
  {
    return m_box;
  }

  /*!\fn const std::vector<TessellatedOutline::handle>& tessellation
    Actual tessellation of the WRATHShape.
    Each entry in the returned array is a
    WRATHOutline tessellated. Additionally
    one is guaranteed that 
    \code
    tessellation()[ID].outlineID()==ID
    \endcode
   */
  const std::vector<TessellatedOutline::handle>&
  tessellation(void) const
  {
    return m_tessellation;
  }

  /*!\fn const PayloadParams& parameters
    Parameters that generated the payload.
   */
  const PayloadParams&
  parameters(void) const
  {
    return m_parameters;
  }

  /*!\fn handle generate_payload
    Template function used by WRATHShape to generate
    WRATHShapeSimpleTessellatorPayload objects on demand.
   */
  template<typename T>
  static
  handle
  generate_payload(const WRATHShape<T> &pshape,
                   const PayloadParams &pp=PayloadParams())
  {
    return WRATHNew WRATHShapeSimpleTessellatorPayload(pshape, pp);
  }

private:

  void
  compute_bounding_box(void);

  std::vector<TessellatedOutline::handle> m_tessellation;
  PayloadParams m_parameters;
  WRATHBBox<2> m_box;
};




/*! @} */
#include "WRATHShapeSimpleTessellatorImplement.tcc"

#endif
