/*! 
 * \file WRATHShapePreStroker.hpp
 * \brief file WRATHShapePreStroker.hpp
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


#ifndef WRATH_HEADER_SHAPE_PRE_STROKER_HPP_
#define WRATH_HEADER_SHAPE_PRE_STROKER_HPP_


#include "WRATHConfig.hpp"
#include "WRATHBBox.hpp"
#include "WRATHShape.hpp"
#include "WRATHInterleavedAttributes.hpp"
#include "WRATHShapeSimpleTessellator.hpp"



/*! \addtogroup Shape
 * @{
 */


/*!\class WRATHShapePreStrokerPayload
  A WRATHShapePreStrokerPayload holds the payload
  needed for dynamic stroking of a \ref WRATHShape.
  It does not hold a WRATHShape stroked, rather
  it holds a set of data that allows for a WRATHShape
  to change the width of stroking dynamically.
  That data is a WRATHShapeSimpleTessellatorPayload
  for the stroking of the path of the WRATHShape
  together with the data of a WRATHShapePreStrokerPayload:
  data for stroking of joins and caps.
 */
class WRATHShapePreStrokerPayload:
  public WRATHReferenceCountedObjectT<WRATHShapePreStrokerPayload>
{
public:
  /*!\class JoinPointBase
    JointPointBase is a base class
    for objects that represent the geometry
    data needed to stroke a join.
   */
  class JoinPointBase
  {
  public:

    /*!\fn JoinPointBase
      Ctor. You really should not call this yourself.
      \param outID value that \ref outlineID() will return
      \param pre_id value that \ref pointID_beforeJoin() will return
      \param post_id value that \ref pointID_afterJoin() will return
      \param pos value that \ref pre_position() will return
    */
    JoinPointBase(unsigned int outID,
                  unsigned int pre_id, unsigned int post_id,
                  const vec2 &pos):
      m_src_point_id_before(pre_id),
      m_src_point_id_after(post_id),
      m_outlineID(outID),
      m_pre_position(pos)
    {}

    /*!\fn const vec2& pre_position
      Returns the pre-position of JoinPointBase.
      This value when combined with a value
      returned by a derived class function will
      give the actual position of the point
      of the join.
     */
    const vec2&
    pre_position(void) const
    {
      return m_pre_position;
    }

    /*!\fn outlineID
      Returns the outlineID of the outline that 
      from which this point was generated, this ID
      if the ID to feed WRATHShape<T>::outline(unsigned int)
      to get the \ref WRATHOutline from which this point
      came. 
     */
    unsigned int
    outlineID(void) const
    {
      return m_outlineID;
    }

    /*!\fn unsigned int pointID_beforeJoin
      Returns the point ID of the starting point
      of the edge before the join of which this 
      join_point is a part.
      Let S be the pointer to the WRATHShape<T>
      that generated the stroking, then
      \code   
         S->outline(outlineID())->pt(pointID_beforeJoin())->to_position()
      \endcode
      gives the position of the join. 
     */
    unsigned int
    pointID_beforeJoin(void) const
    {
      return m_src_point_id_before;
    }
    
    /*!\fn unsigned int pointID_afterJoin
     Returns the point ID of the starting point
     of the edge after the join of which this 
     join_point is a part.
     Let S be the pointer to the WRATHShape<T>
     that generated the stroking, then
     \code   
     S->outline(outlineID())->pt(pointID_afterJoin())->position()
     \endcode
     gives the position of the join. 
     */
    unsigned int
    pointID_afterJoin(void) const
    {
      return m_src_point_id_after;
    }  

  private:
    unsigned int m_src_point_id_before, m_src_point_id_after;
    unsigned int m_outlineID;
    vec2 m_pre_position;
  };

  /*!\class JoinPoint
    A JoinPoint represents a point to stroke
    either a bevel or rounded join.
   */
  class JoinPoint:public JoinPointBase
  {
  public:
    /*!\fn JoinPoint
      Ctor. You really should not call this yourself.
      \param outID value that \ref outlineID() will return
      \param pre_id value that \ref pointID_beforeJoin() will return
      \param post_id value that \ref pointID_afterJoin() will return
      \param pos value that \ref pre_position() will return
      \param v value that \ref offset_vector() will return
    */
    JoinPoint(unsigned int outID,
              unsigned int pre_id, unsigned int post_id,
              const vec2 &pos, const vec2 &v):
      JoinPointBase(outID, pre_id, post_id, pos),
      m_offset_vector(v)
    {}

    /*!\fn const vec2& offset_vector(void) const
      Offset vector of the JoinPoint,
      used to compute the JoinPoint
      which is given by
      pre_position() + stroke_width*offset_vector().      
     */
    const vec2&
    offset_vector(void) const
    {
      return m_offset_vector;
    }

    /*!\fn const vec2& offset_vector(float) const
      Offset vector of the JoinPoint,
      used to compute the JoinPoint
      which is given by
      pre_position() + stroke_width*offset_vector().     
      This version takes as argument
      the miter limit which is NOT used,
      this method is provided so that
      a common template can be used for both
      JoinPoint and MiterJoinPoint objects.      
     */
    const vec2&
    offset_vector(float) const
    {
      return m_offset_vector;
    }

  private:
    vec2 m_offset_vector;
  };

  /*!\class MiterJoinPoint
    A MiterJoinPoint represents a point to stroke
    a point of a miter join.
   */
  class MiterJoinPoint:public JoinPointBase
  {
  public:
    /*!\fn MiterJoinPoint(unsigned int,
                          unsigned int, unsigned int,
                          const vec2&, const vec2&)
      Ctor. You really should not call this yourself.
      \param outID value that \ref outlineID() will return
      \param pre_id value that \ref pointID_beforeJoin() will return
      \param post_id value that \ref pointID_afterJoin() will return
      \param pos value that \ref pre_position() will return
      \param v value that helps compute the value that \ref offset_vector() will return
    */
    MiterJoinPoint(unsigned int outID,
                   unsigned int pre_id, unsigned int post_id,
                   const vec2 &pos, const vec2 &v):
      JoinPointBase(outID, pre_id, post_id, pos),
      m_v(v),
      m_depends_on_miter_limit(false)
    {}

    /*!\fn MiterJoinPoint(unsigned int,
                          unsigned int, unsigned int,
                          const vec2&, const vec2&,
                          const vec2&, float, float)
      Ctor. You really should not call this yourself.
      \param outID value that \ref outlineID() will return
      \param pre_id value that \ref pointID_beforeJoin() will return
      \param post_id value that \ref pointID_afterJoin() will return
      \param pos value that \ref pre_position() will return
      \param v value that helps compute the value that \ref offset_vector() will return
      \param n value that helps compute the value that \ref offset_vector() will return
      \param lhs value that helps compute the value that \ref offset_vector() will return
      \param rhs value that helps compute the value that \ref offset_vector() will return
    */
    MiterJoinPoint(unsigned int outID,
                   unsigned int pre_id, unsigned int post_id,
                   const vec2 &pos, 
                   const vec2 &v,
                   const vec2 &n,
                   float lhs, float rhs):
      JoinPointBase(outID, pre_id, post_id, pos),
      m_v(v),
      m_n(n),
      m_lhs(lhs),
      m_rhs(rhs),
      m_depends_on_miter_limit(true)
    {}

    /*!\fn vec2 offset_vector
      Offset vector of the JoinPoint,
      used to compute the JoinPoint
      which is given by
      pre_position() + stroke_width*offset_vector(miter_limit).      
      \param miter_limit miter limit of stroking
     */
    vec2
    offset_vector(float miter_limit) const;

  private:
    vec2 m_v, m_n;
    float m_lhs, m_rhs;
    bool m_depends_on_miter_limit;
  };


  /*!\class CapPoint
    A CapPoint represents a point used to draw
    a cap of stroking.
   */
  class CapPoint
  {
  public:

    /*!\fn CapPoint
      Ctor. You really should not call this yourself.
      \param mm value that \ref at_start_of_edge() will return
      \param outID value that \ref outlineID() will return
      \param p value that \ref pre_position() will return
      \param v value that \ref offset_vector() will return
     */
    CapPoint(bool mm,
             unsigned int outID,
             const vec2 &p, const vec2 &v):
      m_at_start_of_edge(mm),
      m_outlineID(outID),
      m_pre_position(p),
      m_offset_vector(v)
    {}

    /*!\fn const vec2& pre_position
      Returns the pre-position of JoinPoint,
      the actual position of the JoinPoint is 
      given by
      pre_position() + stroke_width*offset_vector().
     */
    const vec2&
    pre_position(void) const
    {
      return m_pre_position;
    }

    /*!\fn const vec2& offset_vector(void) const
      Offset vector of the CapPoint,
      used to compute the CapPoint
      which is given by
      pre_position() + stroke_width*offset_vector().      
     */
    const vec2&
    offset_vector(void) const
    {
      return m_offset_vector;
    }

    /*!\fn const vec2& offset_vector(float) const
      Offset vector of the CapPoint,
      used to compute the CapPoint
      which is given by
      pre_position() + stroke_width*offset_vector().       
      This version takes as argument
      the miter limit which is NOT used,
      this method is provided so that
      a common template can be used for both
      CapPoint and MiterJoinPoint objects.      
     */
    const vec2&
    offset_vector(float) const
    {
      return m_offset_vector;
    }

    /*!\fn unsigned int outlineID
      Returns the outlineID of the outline that 
      from which this point was generated.
     */
    unsigned int
    outlineID(void) const
    {
      return m_outlineID;
    }

    /*!\fn bool at_start_of_edge
      Returns true if the cap point is a point from a cap
      at the start of an outline, returns false if the
      cap point is a point from the cap at end of an outline      
     */
    bool
    at_start_of_edge(void) const
    {
      return m_at_start_of_edge;
    }
    
  private:
    bool m_at_start_of_edge;
    unsigned int m_outlineID;
    vec2 m_pre_position, m_offset_vector;
  };

  /*!\enum generate_bit_fields
    Enumeration to name the bit flags for generation
    parameters of a WRATHShapePreStroker
   */
  enum generate_bit_fields
    {
      /*!
        Indicates to generate data for drawing
        square caps at the end points of each
        outline, see also \ref square_cap_pts() 
        and \ref square_cap_indices().
       */
      generate_square_caps=1,

      /*!
        Indicates to generate data for drawing
        rounded caps at the end points of each
        outline, see also \ref rounded_cap_pts() 
        and \ref rounded_cap_indices().
       */
      generate_rounded_caps=2,

      /*!
        Conveniance to indicate to generate
        the data for rounded and square caps,
        see \ref generate_square_caps and \ref
        generate_rounded_caps.
       */
      generate_caps=generate_square_caps|generate_rounded_caps,

      /*!
        Indicates to generate data for drawing
        miter joins at the joins of each point of
        each outline, see also \ref miter_join_pts() 
        and \ref miter_join_indices().
       */
      generate_miter_joins=4,

      /*!
        Indicates to generate data for drawing
        bevel joins at the joins of each point of
        each outline, see also \ref bevel_join_pts() 
        and \ref bevel_join_indices().
       */
      generate_bevel_joins=8,

      /*!
        Indicates to generate data for drawing
        rounded joins at the joins of each point of
        each outline, see also \ref rounded_join_pts() 
        and \ref rounded_join_indices().
       */
      generate_rounded_joins=16,

      /*!
        Conveniance to indicate to generate
        the data for rounded, bevel and miter
        joins see \ref  generate_miter_joins,
        \ref generate_bevel_joins and \ref
        generate_rounded_joins.
       */
      generate_joins=generate_miter_joins|generate_bevel_joins|generate_rounded_joins,

      /*!
        Conveniance to indicate to generate
        the data all types of joins and caps,
        see \ref generate_caps and \ref 
        generate_joins.
       */
      generate_all=generate_caps|generate_joins
    };

  /*!\class PayloadParams
    Conveniance class for specifying generation
    of WRATHShapePreStrokerPayload and a 
    WRATHShapeSimpleTessellatorPayload payload.
   */
  class PayloadParams
  {
  public:

    /*!\typedef PayloadType
      Type that uses this paramter type
     */
    typedef WRATHShapePreStrokerPayload PayloadType;

    /*!\fn PayloadParams
      Ctor
      \param args value to which to initialize \ref m_tess_params
      \param pflags value to which to initialize \ref m_flags
     */
    PayloadParams(const WRATHShapeSimpleTessellatorPayload::PayloadParams &args
                  =WRATHShapeSimpleTessellatorPayload::PayloadParams(),
                  uint32_t pflags=~0):
      m_tess_params(args),
      m_flags(pflags)
    {}

    /*!\fn PayloadParams& flags
      Sets \ref m_flags
      \param v value to use
     */
    PayloadParams&
    flags(uint32_t v)
    {
      m_flags=v;
      return *this;
    }

    /*!\fn PayloadParams& tess_params
      Sets \ref m_tess_params 
      \param v value to use
     */
    PayloadParams&
    tess_params(const WRATHShapeSimpleTessellatorPayload::PayloadParams &v)
    {
      m_tess_params=v;
      return *this;
    }

    /*!\fn bool operator==(const PayLoadParams&) const
      Equality operator.
      \param rhs value to which to compare
     */
    bool
    operator==(const PayloadParams &rhs) const
    {
      return (m_flags&generate_all)==(rhs.m_flags&generate_all)
        and m_tess_params==rhs.m_tess_params;
    }

    /*!\var m_tess_params
      Tessellation paramaters.
    */
    WRATHShapeSimpleTessellatorPayload::PayloadParams m_tess_params;
    
    /*!\var m_flags
      Bitfield specifying what types of joins
      and caps to generate, see 
      \ref generate_square_caps,
      \ref generate_rounded_caps,
      \ref generate_caps,
      \ref generate_miter_joins,
      \ref generate_bevel_joins,
      \ref generate_rounded_joins,
      \ref generate_joins,
      and \ref generate_all
     */
    uint32_t m_flags;
  };


  /*!\fn WRATHShapePreStrokerPayload(const WRATHShapeSimpleTessellatorPayload::handle&)
    Ctor. Create a WRATHShapePreStrokerPayload 
    generting call cap and join types from
    the tessellation of a WRATHShape<T> held in a
    WRATHShapeSimpleTessellatorPayload.
    \param ph tessellation of a WRATHShape<T>
  */
  explicit
  WRATHShapePreStrokerPayload(const WRATHShapeSimpleTessellatorPayload::handle &ph):
    m_flags(generate_all),
    m_h(ph)
  {
    generate_data();
  }

  /*!\fn WRATHShapePreStrokerPayload(uint32_t, const WRATHShapeSimpleTessellatorPayload::handle&)
    Ctor. Create a WRATHShapePreStrokerPayload 
    generting indicated cap and join types from
    the tessellation of a WRATHShape<T> held in a
    WRATHShapeSimpleTessellatorPayload.
    \param pflags bit-flags indicating what types of joins
                  and caps to generate. 
    \param ph tessellation of a WRATHShape<T>
  */
  WRATHShapePreStrokerPayload(uint32_t pflags,
                              const WRATHShapeSimpleTessellatorPayload::handle &ph):
    m_flags(pflags&generate_all),
    m_h(ph)
  {
    generate_data();
  }
  
  /*!\fn uint32_t flags
    Returns the flags used to generate this
    WRATHShapePreStrokerPayload. The flags
    determined which (if any) cap and join
    data is generated.
   */
  uint32_t
  flags(void) const
  {
    return m_flags;
  }
  
    
  /*!\fn float effective_curve_thresh
    Returns the effective curvature threshhold
    used when generating rounded caps and
    rounded joins.
   */
  float
  effective_curve_thresh(void) const
  {
    return m_effective_curve_thresh;
  }
  
  /*!\fn const WRATHShapeSimpleTessellatorPayload::handle& tessellation_src
    Returns a handle to the source tessellation
    data.
   */
  const WRATHShapeSimpleTessellatorPayload::handle&
  tessellation_src(void) const
  {
    return m_h;
  }

  /*!\fn const_c_array<CapPoint> square_cap_pts
    Returns the point data for square caps.
    Point data is generated if and only if 
    the bit \ref generate_square_caps
    is up in \ref flags().
   */
  const_c_array<CapPoint>
  square_cap_pts(void) const
  {
    return m_square_caps.m_pts;
  }

  /*!\fn const_c_array<GLushort> square_cap_indices
    Returns the index data for square caps.
    Index data is indices into \ref square_cap_pts() 
    for triangles. Index data is generated if
    and only if the bit \ref generate_square_caps
    is up in \ref flags().    
   */
  const_c_array<GLushort>
  square_cap_indices(void) const
  {
    return m_square_caps.m_indices;
  }
  
  /*!\fn const_c_array<CapPoint> rounded_cap_pts
    Returns the point data for rounded caps.
    Point data is generated if and only if 
    the bit \ref generate_rounded_caps
    is up in \ref flags().
   */
  const_c_array<CapPoint>
  rounded_cap_pts(void) const
  {
    return m_rounded_caps.m_pts;
  }

  /*!\fn const_c_array<GLushort> rounded_cap_indices
    Returns the index data for rounded caps.
    Index data is indices into \ref rounded_cap_pts() 
    for triangles. Index data is generated if
    and only if the bit \ref generate_rounded_caps
    is up in \ref flags().    
   */
  const_c_array<GLushort>
  rounded_cap_indices(void) const
  {
    return m_rounded_caps.m_indices;
  }
  
  /*!\fn const_c_array<MiterJoinPoint> all_miter_join_pts
    Returns the point data for miter joins.
    Point data is generated if and only if 
    the bit \ref generate_miter_joins
    is up in \ref flags(). 
   */
  const_c_array<MiterJoinPoint>
  all_miter_join_pts(void) const
  {
    return m_miter_joins.all_pts();
  }

  /*!\fn const_c_array<MiterJoinPoint> core_miter_join_pts
    Returns the point data for miter joins
    not including those joins between the end
    point and a beginning point of any 
    WRATHOutline. Point data is generated 
    if and only if the bit \ref generate_miter_joins
    is up in \ref flags(). Also, core_miter_join_pts()
    is a sub-array of all_miter_join_pts().
   */
  const_c_array<MiterJoinPoint>
  core_miter_join_pts(void) const
  {
    return m_miter_joins.pts_up_to_marker();
  }

  /*!\fn const_c_array<MiterJoinPoint> miter_join_pts
    Conveniance function to return all the join
    points for drawing closed or opened outlines.
    \param all_joins if true, return all_miter_join_pts(),
                     otherwise return core_miter_join_pts().
   */
  const_c_array<MiterJoinPoint>
  miter_join_pts(bool all_joins) const
  {
    return m_miter_joins.pts(all_joins);
  }

  /*!\fn const_c_array<GLushort> all_miter_join_indices
    Returns the index data for miter joins.
    Index data is indices into \ref all_miter_join_pts() 
    for triangles. Index data is generated if
    and only if the bit \ref generate_miter_joins
    is up in \ref flags().    
   */
  const_c_array<GLushort>
  all_miter_join_indices(void) const
  {
    return m_miter_joins.all_indices();
  }

  /*!\fn const_c_array<GLushort> core_miter_join_indices
    Returns the index data for miter joins
    not including those join between the end
    point and a beginning point of any 
    WRATHOutline. Index data is indices into 
    \ref core_miter_join_pts() for triangles. 
    Index data is generated if and only if the 
    bit \ref generate_miter_joins is up in 
    \ref flags(). Also, core_miter_join_indices()
    is a sub-array of all_miter_join_indices().   
   */
  const_c_array<GLushort>
  core_miter_join_indices(void) const
  {
    return m_miter_joins.ind_up_to_marker();
  }

  /*!\fn const_c_array<GLushort> miter_join_indices
    Conveniance function to return all the join
    indices for drawing closed or opened outlines.
    \param all_joins if true, return all_miter_join_indices(),
                     otherwise return core_miter_join_indices().
   */
  const_c_array<GLushort>
  miter_join_indices(bool all_joins) const
  {
    return m_miter_joins.inds(all_joins);
  }

  /*!\fn const_c_array<JoinPoint> all_bevel_join_pts
    Returns the point data for bevel joins.
    Point data is generated if and only if 
    the bit \ref generate_bevel_joins
    is up in \ref flags().
   */
  const_c_array<JoinPoint>
  all_bevel_join_pts(void) const
  {
    return m_bevel_joins.all_pts();
  }

  /*!\fn const_c_array<JoinPoint> core_bevel_join_pts
    Returns the point data for bevel joins
    not including those joins between the end
    point and a beginning point of any 
    WRATHOutline. Point data is generated 
    if and only if the bit \ref generate_bevel_joins
    is up in \ref flags(). Also, core_bevel_join_pts()
    is a sub-array of all_bevel_join_pts().
   */
  const_c_array<JoinPoint>
  core_bevel_join_pts(void) const
  {
    return m_bevel_joins.pts_up_to_marker();
  }

  /*!\fn const_c_array<JoinPoint> bevel_join_pts
    Conveniance function to return all the join
    points for drawing closed or opened outlines.
    \param all_joins if true, return all_bevel_join_pts(),
                     otherwise return core_bevel_join_pts().
   */
  const_c_array<JoinPoint>
  bevel_join_pts(bool all_joins) const
  {
    return m_bevel_joins.pts(all_joins);
  }

  /*!\fn const_c_array<GLushort> all_bevel_join_indices 
    Returns the index data for bevel joins.
    Index data is indices into \ref all_bevel_join_pts() 
    for triangles. Index data is generated if
    and only if the bit \ref generate_bevel_joins
    is up in \ref flags().    
   */
  const_c_array<GLushort>
  all_bevel_join_indices(void) const
  {
    return m_bevel_joins.all_indices();
  }
  
  /*!\fn const_c_array<GLushort> core_bevel_join_indices
    Returns the index data for bevel joins
    not including those join between the end
    point and a beginning point of any 
    WRATHOutline. Index data is indices into 
    \ref core_bevel_join_pts() for triangles. 
    Index data is generated if and only if the 
    bit \ref generate_bevel_joins is up in 
    \ref flags(). Also, core_bevel_join_indices()
    is a sub-array of all_bevel_join_indices().    
   */
  const_c_array<GLushort>
  core_bevel_join_indices(void) const
  {
    return m_bevel_joins.ind_up_to_marker();
  }

  /*!\fn const_c_array<GLushort> bevel_join_indices  
    Conveniance function to return all the join
    indices for drawing closed or opened outlines.
    \param all_joins if true, return all_bevel_join_indices(),
                     otherwise return core_bevel_join_indices().
   */
  const_c_array<GLushort>
  bevel_join_indices(bool all_joins) const
  {
    return m_bevel_joins.inds(all_joins);
  }

  /*!\fn const_c_array<JoinPoint> all_rounded_join_pts
    Returns the point data for rounded joins.
    Point data is generated if and only if 
    the bit \ref generate_rounded_joins
    is up in \ref flags().
   */
  const_c_array<JoinPoint>
  all_rounded_join_pts(void) const
  {
    return m_rounded_joins.all_pts();
  }

  /*!\fn const_c_array<JoinPoint> core_rounded_join_pts
    Returns the point data for rounded joins
    not including those joins between the end
    point and a beginning point of any 
    WRATHOutline. Point data is generated 
    if and only if the bit \ref generate_rounded_joins
    is up in \ref flags(). Also, core_rounded_join_pts()
    is a sub-array of all_rounded_join_pts().
   */
  const_c_array<JoinPoint>
  core_rounded_join_pts(void) const
  {
    return m_rounded_joins.pts_up_to_marker();
  }

  /*!\fn const_c_array<JoinPoint> rounded_join_pts 
    Conveniance function to return all the join
    points for drawing closed or opened outlines.
    \param all_joins if true, return all_rounded_join_pts(),
                     otherwise return core_rounded_join_pts().
   */
  const_c_array<JoinPoint>
  rounded_join_pts(bool all_joins) const
  {
    return m_rounded_joins.pts(all_joins);
  }

  /*!\fn const_c_array<GLushort> all_rounded_join_indices 
    Returns the index data for rounded joins.
    Index data is indices into \ref all_rounded_join_pts() 
    for triangles. Index data is generated if
    and only if the bit \ref generate_rounded_joins
    is up in \ref flags().    
   */
  const_c_array<GLushort>
  all_rounded_join_indices(void) const
  {
    return m_rounded_joins.all_indices();
  }

  /*!\fn const_c_array<GLushort> core_rounded_join_indices
    Returns the index data for rounded joins
    not including those join between the end
    point and a beginning point of any 
    WRATHOutline. Index data is indices into 
    \ref core_rounded_join_pts() for triangles. 
    Index data is generated if and only if the 
    bit \ref generate_rounded_joins is up in 
    \ref flags(). Also, core_rounded_join_indices()
    is a sub-array of all_rounded_join_indices().         
   */
  const_c_array<GLushort>
  core_rounded_join_indices(void) const
  {
    return m_rounded_joins.ind_up_to_marker();
  }

  /*!\fn const_c_array<GLushort> rounded_join_indices
    Conveniance function to return all the join
    indices for drawing closed or opened outlines.
    \param all_joins if true, return all_rounded_join_indices(),
                     otherwise return core_rounded_join_indices().
   */
  const_c_array<GLushort>
  rounded_join_indices(bool all_joins) const
  {
    return m_rounded_joins.inds(all_joins);
  }

  /*!\fn handle generate_payload(const WRATHShape<T>&, const PayloadParams&)
    Template meta-function used by WRATHShape to generate
    WRATHShapePreStrokerPayload objects on demand.
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
    return WRATHNew WRATHShapePreStrokerPayload(pp.m_flags, tess);
  }

  /*!\fn handle generate_payload(const WRATHShape<T>&)
    Template meta-function used by WRATHShape to generate
    WRATHShapePreStrokerPayload objects on demand.
    Will use any pre-existing WRATHShapeSimpleTessellatorPayload
    stored in the WRATHShape object and will generate a
    WRATHShapePreStrokerPayload object holding data for
    stroking all cap and join types.
    \param pshape WRATHShape from which to generate the payload
   */
  template<typename T>
  static
  handle
  generate_payload(const WRATHShape<T> &pshape)
  {
    WRATHShapeSimpleTessellatorPayload::handle tess;

    tess=pshape.template fetch_payload<WRATHShapeSimpleTessellatorPayload>();
    return WRATHNew WRATHShapePreStrokerPayload(tess); 
  }


private:

  typedef WRATHShapeSimpleTessellatorPayload::TessellatedOutline Outline;
  typedef WRATHShapeSimpleTessellatorPayload::TessellatedEdge Edge;
  typedef WRATHShapeSimpleTessellatorPayload::CurvePoint CurvePoint;

  template<typename T>
  class DataPacket
  {
  public:
    std::vector<T> m_pts;
    std::vector<GLushort> m_indices;
  };

  template<typename T>
  class DataPacketWithMarkers
  {
  public:
    std::vector<T> m_pts;
    std::vector<GLushort> m_indices;
    unsigned int m_pt_marker;
    unsigned int m_ind_marker;

    void
    set_markers(void)
    {
      m_pt_marker=m_pts.size();
      m_ind_marker=m_indices.size();
    }

    const_c_array<T>
    pts_up_to_marker(void) const
    {
      const T *ptr(m_pts.empty()?NULL:&m_pts[0]);
      return const_c_array<T>(ptr, m_pt_marker);
    }

    const_c_array<GLushort>
    ind_up_to_marker(void) const
    {
      const GLushort *ptr(m_indices.empty()?NULL:&m_indices[0]);
      return const_c_array<GLushort>(ptr, m_ind_marker);
    }

    const_c_array<T>
    all_pts(void) const
    {
      return const_c_array<T>(m_pts);
    }

    const_c_array<GLushort>
    all_indices(void) const
    {
      return const_c_array<GLushort>(m_indices);
    }

    const_c_array<T>
    pts(bool all) const
    {
      return (all)?
        all_pts():
        pts_up_to_marker();
    }

    const_c_array<GLushort>
    inds(bool all) const
    {
      return (all)?
        all_indices():
        ind_up_to_marker();
    }
  };

  void
  generate_data(void);

  void
  handle_outline(const Outline::handle &O);

  void
  handle_cap(const Outline::handle &O,
             const CurvePoint &pt,
             bool is_starting_cap);


  void
  handle_join(const Outline::handle &O,
              const Edge::handle &pre,
              const Edge::handle &post);

  int m_flags;
  float m_effective_curve_thresh;
  WRATHShapeSimpleTessellatorPayload::handle m_h;
  DataPacket<CapPoint> m_square_caps, m_rounded_caps;
  DataPacketWithMarkers<MiterJoinPoint> m_miter_joins;
  DataPacketWithMarkers<JoinPoint> m_bevel_joins, m_rounded_joins;
};

/*! @} */

#endif
