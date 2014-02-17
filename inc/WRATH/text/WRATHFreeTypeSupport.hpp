/*! 
 * \file WRATHFreeTypeSupport.hpp
 * \brief file WRATHFreeTypeSupport.hpp
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




#ifndef WRATH_HEADER_FreeType_SUPPORT_HPP_
#define WRATH_HEADER_FreeType_SUPPORT_HPP_

#include "WRATHConfig.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H

#include <sstream>
#include <limits>
#include <math.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <boost/multi_array.hpp>
#include <sys/time.h>
#include <boost/signals2.hpp>
#include <boost/bind.hpp>
#include "WRATHUtil.hpp"
#include "c_array.hpp"
#include "ostream_utility.hpp"
#include "WRATHInterleavedAttributes.hpp"
#include "WRATHTextureFont.hpp"
#include "WRATHUtil.hpp"
#include "WRATHPolynomial.hpp"
#include "WRATHFontDatabase.hpp"



/*! \addtogroup Text
 * @{
 */


/*!\namespace WRATHFreeTypeSupport
  Common routines for manipulating FreeType data structures.
 */
namespace WRATHFreeTypeSupport
{
  /*!\class point_type
    For each glyph, there is a vectoral representation.
    A point_type gives the points of the outlines of a font.
    The color indicates the source of the points as follows:
    \n (0xFF,0x00,0x00,0) {red} on outline control point
    \n (0x00,0xFF,0x00,1) {green} off outline quadratic control point
    \n (0x00,0x00,0xFF,2) {blue} off outline  cubic control point
  */
  class point_type:public WRATHInterleavedAttributes< vecN<int,2>, vecN<GLubyte,4> >
  {
  public:
    /*!\typedef base_class
      Conveniance typedef to base class, 
      WRATHInterleavedAttributes< vecN<int,2>, vecN<GLubyte,4> >
     */
    typedef WRATHInterleavedAttributes< vecN<int,2>, vecN<GLubyte,4> > base_class;

    enum
      {
        /*!
          Location within the point_type
          of the point data.
         */
        point_location=0,

        /*!
          Location within the point_type
          of the color data. The color 
          is set by the points classifiaction
          type.
         */
        color_location=1,
      };

    /*!\enum point_classification
      Enumeration used to specify the classification
      of a control point of a Bezier curve.
     */
    enum point_classification
      {
        /*!
          Point is an end point of a
          Bezier curve.
         */
        on_curve=0,

        /*!
          Point is an off curve control
          point of a quadratic Bezier
          curve.
         */
        conic_off_curve=1,

        /*!
          Point is an off curve control
          point of a cubic Bezier curve.
         */
        cubic_off_curve=2,
      };

    /*!\fn point_type()
      Empty ctor, does NOT initialize any
      of the data.
     */
    point_type()
    {}

    /*!\fn point_type(const ivec2&, enum point_classification)
      Ctor, sets the position as passed and the color
      based from the point_classification.
      Note that the .w() components of the color
      will hold the point_classification value.
      \param pos position of point
      \param cl the points classification
     */
    point_type(const ivec2 &pos,
               enum point_classification cl)
    {
      static vecN<GLubyte,4> cols[3]=
        {
          vecN<GLubyte,4>(0xFF, 0x00, 0x00, on_curve),
          vecN<GLubyte,4>(0x00, 0xFF, 0x00, conic_off_curve),
          vecN<GLubyte,4>(0x00, 0x00, 0xFF, cubic_off_curve)
        };

      position().x()=pos.x();
      position().y()=pos.y();

      WRATHassert(cl<3);
      color()=cols[cl];
    }

    /*!\fn const vecN<int,2>& position(void) const
      Returns the position of the point.
     */
    const vecN<int,2>&
    position(void) const
    {
      return get<point_location>();
    }

    /*!\fn vecN<int,2>& position(void)
      Returns a reference to 
      the position of the point.
     */
    vecN<int,2>&
    position(void)
    {
      return get<point_location>();
    }

    /*!\fn const vecN<GLubyte,4>& color(void) const
      Returns the color of the point,
      the .w() component indicates the
      point's classification.
     */
    const vecN<GLubyte,4>&
    color(void) const
    {
      return get<color_location>();
    }

    /*!\fn vecN<GLubyte,4>& color(void)
      Returns a reference to the color 
      of the point, the .w() component 
      indicates the point's classification.
     */
    vecN<GLubyte,4>&
    color(void) 
    {
      return get<color_location>();
    }

    /*!\fn  enum point_classification classification
      Returns the classification of the point,
      as stored in color().w().
     */
    enum point_classification
    classification(void) const
    {
      return static_cast<enum point_classification>(color().w());
    }

    /*!\fn attribute_key(vecN<opengl_trait_value, N>&)
      See WRATHInterleavedAttributes::attribute_key().
     */
    template<unsigned int N>
    static
    void
    attribute_key(vecN<opengl_trait_value,N> &attrs)
    {
      base_class::attribute_key(attrs);
      if(N>=2)
        {
          attrs[1].m_normalized=GL_TRUE;
        }
    }
  };

  /*!\class geometry_data_filter
    A geometry_data_filter acts as a preprocessor
    to point data before it is added to a 
    \ref geometry_data object.
   */
  class geometry_data_filter:
    public WRATHReferenceCountedObjectT<geometry_data_filter>
  {
  public:
    /*!\fn ivec2 apply_filter(const ivec2&, enum point_type::point_classification) const
      To be implemented by a derived class 
      to (optionally) modify the position 
      of a point to be added to a \ref
      geometry_data object. Return value is the modified value
      \param in_pt coordinate of input point
      \param cl point classification
     */
    virtual
    ivec2
    apply_filter(const ivec2 &in_pt, enum point_type::point_classification cl) const=0;
  };

  class solution_point;
  
  /*!\class geometry_data
    A geometry_data is a "holder" for geometric
    and debug data extracted from a FreeType font.
   */
  class geometry_data
  {
  public:
    /*!\fn geometry_data(std::ostream*, std::vector<point_type>&, geometry_data_filter::handle)
      Ctor. 
      \param ostr std::ostream to which to log debug 
                  messages. if NULL, no debug messages
                  are logged.
      \param pts std::vector to hold the point data
                 of the outline of a glyph.
      \param h filter to modify point coordinates
     */
    geometry_data(std::ostream *ostr, 
                  std::vector<point_type> &pts,
                  geometry_data_filter::handle h=geometry_data_filter::handle()):
      m_debug_stream(ostr),
      m_pt_array(pts),
      m_filter(h)
    {}

    /*!\fn geometry_data(std::vector<point_type>&, geometry_data_filter::handle)
      Ctor, set the debug stream to NULL, thus
      does not emit debug log messages.
      \param pts std::vector to hold the point data
                 of the outline of a glyph.
      \param h filter to modify point coordinates
     */
    explicit
    geometry_data(std::vector<point_type> &pts,
                  geometry_data_filter::handle h=geometry_data_filter::handle()):
      m_debug_stream(NULL),
      m_pt_array(pts),
      m_filter(h)
    {}

    /*!\fn std::ostream& debug_stream
      Returns the debug stream used by this
      geometry_data(). If this geometry_data
      was constucted to not have a debug stream,
      then the reference is NULL.
     */
    std::ostream&
    debug_stream(void) const
    {
      WRATHassert(m_debug_stream!=NULL);
      return *m_debug_stream;
    }

    /*!\fn bool debug_stream_valid
      Returns true if and only if
      this geometry_data has a
      debug stream.
     */
    bool
    debug_stream_valid(void) const
    {
      return m_debug_stream!=NULL;
    }

    /*!\fn std::vector<point_type>& pts
      Returns a reference to the std::vector
      holding the point data.
     */
    std::vector<point_type>&
    pts(void) const
    {
      return m_pt_array;
    }

    /*!\fn ivec2 pt
      Returns the coordinates of the
      named point.
      \param I index of point
     */
    ivec2
    pt(int I) const
    {
      return ivec2(m_pt_array[I].position().x(),
                   m_pt_array[I].position().y());
    }

    /*!\fn enum point_type::point_classification tag(int)
      Returns the tag of the named point
      \param I index of point
     */
    enum point_type::point_classification
    tag(int I) const
    {
      return m_pt_array[I].classification();
    }

    /*!\fn GLushort push_back(const ivec2&, char) const
      Add a point, returns the index of the added point.
      \param in_pt position of the point
      \param in_tag FT tag of point, i.e. from the Freetype
                    data FT_Outline::tags.

     */
    GLushort
    push_back(const ivec2 &in_pt, char in_tag) const;

  private:
    std::ostream *m_debug_stream;
    std::vector<point_type> &m_pt_array;
    geometry_data_filter::handle m_filter;
  };
  
  class BezierCurve;

  /*!\class solution_point
    A solution_point stores a solution to a polynomial
    together with a multiplicity.
   */
  class solution_point
  {
  public:
    /*!\var m_multiplicity
      algrebraic multiplicity of root.
     */
    int m_multiplicity;

    /*!\var m_value
      value of intersecion.
     */
    float m_value; 

    /*!\var m_time
      time of intersecion
     */
    float m_time;

    /*!\var m_bezier
      curve of intersecion.
     */
    const BezierCurve *m_bezier;

    /*!\var m_derivative
      the value of the derivative
      at the intersection.
     */
    vec2 m_derivative;

    /*!\fn solution_point(int, float, const BezierCurve*, float)
      Ctor. Initializes derivative as 0.0.
      \param multiplicity algebraic multiplicity of solution
      \param v value of intersection
      \param t time of intersection
      \param cv BezierCurve of intersection
     */
    solution_point(int multiplicity, float v, 
                   const BezierCurve *cv, float t):
      m_multiplicity(multiplicity),
      m_value(v),
      m_time(t),
      m_bezier(cv),
      m_derivative(0.0f, 0.0f)
    {}

    /*!\fn solution_point(int, float, const BezierCurve*)
      Ctor. Sets m_value to be the same as m_time.
      Initializes derivative as 0.0.
      \param multiplicity algebraic multiplicity of solution
      \param t time of intersection
      \param cv BezierCurve of intersection
     */
    solution_point(int multiplicity, float t, const BezierCurve *cv):
      m_multiplicity(multiplicity),
      m_value(t),
      m_time(t),
      m_bezier(cv),
      m_derivative(0.0f, 0.0f)
    {}

    /*!\fn bool operator<(const solution_point&) const
      Sorting of solution_point is by the
      the value of intersection, m_value.
      Hence operator< is equivalent to
      comparing m_value.
      \param obj solution_point solution point to which to compare
     */
    bool
    operator<(const solution_point &obj) const
    {
      return m_value<obj.m_value;
    }

    /*!\fn void observe_curve_reversal
      If after computing/getting a solution_point,
      one needs to reverse a curve, then the
      data from the solution_point needs to be
      updated to note the curve reversal.
     */
    void
    observe_curve_reversal(void)
    {
      m_time=1.0f- m_time;
      m_derivative=-m_derivative;
    }

    /*!\fn std::ostream& operator<<(std::ostream &, const solution_point &)
      Overload of operator<< to print the values of a \ref solution_point
      to an std::ostream
      \param ostr std::ostream to which to print
      \param obj value to print
     */
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
  
  /*!\class distance_tracker
    A distance_tracker tracks 
    the distance to the BezierCurve
    from a point.
   */
  class distance_tracker
  {
  public:
    /*!\fn distance_tracker
      Default ctor indicating no canidates, 
      to consider canidates whose distance 
      is no more than 96.0f
     */
    distance_tracker(void):
      m_value(96.0f)
    {}
   
    /*!\fn void init(float)
      Reinitialize the distance_tracker.
      \param v the initial distance value to which 
                   to set this distance_tracker,
                   canidates are ignored if their
                   distance is greater than v.
     */
    void
    init(float v)
    {
      m_value=v;
    }
    
    /*!\fn void update_value
      "Add" a canidate distance for the nearest BezierCurve
      \param v unsigned distance to the curve
     */
    void
    update_value(float v)
    {
      m_value=std::min(v, m_value);
    }
    
    /*!\fn float value
      Returns the signed distance value to the
      closest canidate curve or point.
     */
    float
    value(void) const
    {
      return m_value;
    }
    
  private:
    float m_value;
  };

  /*!\class inside_outside_test_results
    An inside_outside_test_results is used to 
    calculate if a point is inside or outside
    of an outline as according to the odd/even
    fill rule and/or a non-zero winding rule.
    Internally it records the number
    of intersections (counted with multiplicity)
    with an outline in the directions: left,
    right, above and below. The even/odd test is 
    considered reliable if the parity of all is 
    the same. For the winding test it records
    a winding integer computed from a horizontal
    ray test.
   */ 
  class inside_outside_test_results
  {
  public:
    /*!\enum sol_type
      Enumeration to specify which
      direction to get the count
      of intersections.
     */
    enum sol_type
      {
        /*!
          left side of texel
         */
        left,

        /*!
          right side of texel
         */
        right, 

        /*!
           top side of texel
         */
        above,

        /*!
           bottom side of texel
         */
        below,
      };

    /*!\fn inside_outside_test_results
      Ctor. Initializes as having no intersections.
     */
    inside_outside_test_results(void):
      m_solution_count(0,0,0,0),
      m_winding_count(0)
    {}
    
    /*!\fn void reset
      Reset this inside_outside_test_results to
      have no intersections and for the windings
      to be zero.
     */
    void
    reset(void)
    {
      m_solution_count=ivec4(0,0,0,0);
      m_winding_count=0;
    }

    /*!\fn int raw_value
      Returns the number of intersections
      recorded in the named direction.
      \param tp direction
     */
    int
    raw_value(enum sol_type tp)
    {
      return m_solution_count[tp];
    }

    /*!\fn void increment(enum sol_type, int)
      Increment the number of intersections
      in the named direction by a specified
      anount.
      \param tp direction
      \param ct amount to which to increment.
     */
    void
    increment(enum sol_type tp, int ct)
    {
      m_solution_count[tp]+=ct;
    }

    
    /*!\fn void increment_winding(int)
      Call this function to increment the winding
      number.
      \param count amount by which to increment
     */
    void
    increment_winding(int count)
    {
      m_winding_count+=count;
    }

    /*!\fn void decrement_winding
      Call this function to decrement the winding
      number.
      \param count amount by which to decrement
     */
    void
    decrement_winding(int count)
    {
      m_winding_count-=count;
    }

    /*!\fn int winding_number
      Getter for the winding count.
     */
    int
    winding_number(void) const
    {
      return m_winding_count;
    }

    /*!\fn bool reliable_test
      Returns true if the parity of the
      number of intersetions is the same
      in each direction.
     */
    bool
    reliable_test(void) const
    {
      return (m_solution_count[0]&1)==(m_solution_count[1]&1)
        and (m_solution_count[0]&1)==(m_solution_count[2]&1)
        and (m_solution_count[0]&1)==(m_solution_count[3]&1);
    }

    /*!\fn bool inside
      Returns true if the parities in 2 or more
      directions is odd.      
     */
    bool
    inside(void) const
    {
      int votes_inside(0);
      for(unsigned int i=0;i<m_solution_count.size();++i)
        {
          votes_inside+=(m_solution_count[i]&1);
        }
      return votes_inside>=2;      
    }

    /*!\fn bool outside
      Equivalent to !inside().
     */
    bool
    outside(void) const
    {
      return !inside();
    }

    /*!\fn std::ostream& operator<<(std::ostream &, const inside_outside_test_results&)
      Overload of operator<< to print the values of a \ref inside_outside_test_results
      to an std::ostream
      \param ostr std::ostream to which to print
      \param obj value to print
     */
    friend
    std::ostream&
    operator<<(std::ostream &ostr, const inside_outside_test_results &obj)
    {
      ostr << obj.m_solution_count;
      return ostr;
    }

  private:
    ivec4 m_solution_count;
    int m_winding_count;
  };

  /*!\enum intersection_type
    Enumeration describing nature of intersection
    between a bezier curve and horizontal or
    verical line.
   */
  enum intersection_type
    {
      /*!
        Intersection occurs precisely at the
        start of the curve
       */
      intersect_at_0,

      /*!
        Intersection occurs precisely at the
        end of the curve        
       */
      intersect_at_1,

      /*!
        Intserseciont occurs in the interior
        of the curve
       */
      intersect_interior
    };
  
  /*!\class simple_line
    (Badly named class). Records the intersection
    of a curve against a point together with the 
    slope of the BezierCurve at the intersetion.
   */
  class simple_line
  {
  public:
    /*!\fn simple_line(void)
      Ctor to initialize as no intersection.
     */
    simple_line(void):
      m_source(0.0f, 0.0f, NULL, -1.0f),
      m_index_of_intersection(-1)
    {}

    /*!\fn simple_line(const solution_point&, float, const vec2&)
      Ctor to initialize as the intersection from S.
      \param S intersection
      \param v point of intersection (an x or y coordinate)
      \param deriv derivative of Bezier curve at time of intersection
     */
    simple_line(const solution_point &S, float v, const vec2 &deriv):
      m_source(S),
      m_value(v),
      m_index_of_intersection(-1),
      m_intersection_type(intersect_interior)
    {
      m_source.m_derivative=deriv;
    }

    /*!\fn void observe_curve_reversal
      If after computing/getting a solution_point,
      one needs to reverse a curve, then the
      data from the solution_point needs to be
      updated to note the curve reversal.
     */
    void
    observe_curve_reversal(void)
    {
      const enum intersection_type reversal[]=
        {
          intersect_at_1,
          intersect_at_0,
          intersect_interior
        };
      m_source.observe_curve_reversal();
      m_intersection_type=reversal[m_intersection_type];
    }

    /*!\fn bool operator<(const simple_line&) const
      comparison operator for sorting, simple_line 's
      are sorted entirely by m_value, i.e
      is equivalent to m_value<obj.m_value.
      \param obj simple line to which to compare
     */
    bool
    operator<(const simple_line &obj) const
    {
      return m_value<obj.m_value;
    }

    /*!\var m_source
      curve and time that created intersection
    */
    solution_point m_source;

    /*!\var m_value
      point of intersection (is a x or y coordinate)
    */
    float m_value;

    /*!\var m_index_of_intersection
      index of intersection, which gives
      which curve intersect starting from counting below,
      i.e. returns the number of intersections
      below (or to the left) of the intersection
      recored by this simple_line.
      if is -1 then no choice was found.
    */
    int m_index_of_intersection;

    /*!\var m_intersection_type
      Indicates if the intersection is with the
      interior of the curve or if the intersection
      is with an end point of the curve (and which
      endpoint).
     */
    enum intersection_type m_intersection_type;
  };

  
  /*!\enum boundary_type
    Enumeration of boundaries of a texel.
   */
  enum boundary_type
    {
      /*!
        Left edge of texel
       */
      left_boundary,

      /*!
         Right edge of texel
       */
      right_boundary,

      /*!
         bottom edge of texel
       */
      below_boundary,

      /*!
         top edge of texel
       */
      above_boundary,

      /*!
        indicates not boundary,
        used for when no intersections are detected.
       */      
      no_boundary
    };

  /*!\fn enum boundary_type opposite_boundary(enum boundary_type)
    Returns the boundary enumeration opposite
    of a specified boundary enumeration
    \param v value to query
   */
  enum boundary_type
  opposite_boundary(enum boundary_type v);

  /*!\fn enum boundary_type neighbor_boundary(enum boundary_type)
    Returns the boundary enumeration clock-wise
    neighbor of a specified boundary enumeration
    \param v value to query
   */
  enum boundary_type
  neighbor_boundary(enum boundary_type v);

  /*!\fn enum WRATHUtil::coordinate_type side_type(enum boundary_type)
    Returns the side type of a boundary enumeration
    \param v value to query
   */
  enum WRATHUtil::coordinate_type
  side_type(enum boundary_type v);

  /*!\fn bool is_max_side_type(enum boundary_type)
    Returns if a boundary enumeration is
    a "max" side.
    \param v value to query
   */
  inline
  bool
  is_max_side_type(enum boundary_type v)
  {
    return v==right_boundary or v==above_boundary;
  }

  /*!\fn bool is_min_side_type(enum boundary_type)
    Returns if a boundary enumeration is
    a "min" side.
    \param v value to query
   */
  inline
  bool
  is_min_side_type(enum boundary_type v)
  {
    return v==left_boundary or v==below_boundary;
  }

  /*!\class analytic_return_type
    An analytic_return_type holds the intersection
    of a texel against an outline.
   */
  class analytic_return_type
  {
  public:
    /*!\fn analytic_return_type
      Default ctor, initialize
      - \ref m_empty as true
      - \ref m_parity_count as (0,0,0,0)
      - \ref m_intersecions as empty
     */
    analytic_return_type(void):
      m_parity_count(0,0,0,0),
      m_empty(true)
    {}
    
    /*!\var m_intersecions
      list of intersection with the named boundary
      indexed by boundary_type.
    */
    vecN< std::vector<simple_line>, 4> m_intersecions;
    
    /*!\var m_parity_count
      \ref m_parity_count[I] gives the number of curves 
      that intesect the line "below". For example:
      m_parity_count[\ref left_boundary] gives the number
      of curves on the vertical line along the   
      left boundary of the texel that are below
      the texel.
    */
    vecN<int, 4> m_parity_count;

    /*!\var m_empty
      if true, then there are no intersections.
     */
    bool m_empty;
  };

  /*!\class distance_return_type
    A distance_return_type holds the distance
    from a point to an outline toegether
    with a inside_outside_test_results to
    indicate if the point is inside or outside
    of the outline.
   */
  class distance_return_type
  {
  public:    
    /*!\var m_distance
      Holds the distance to the outline.
     */
    distance_tracker m_distance;

    /*!\var m_solution_count
      Holds the data to determine if the
      point is inside or outside of the
      outline.
     */
    inside_outside_test_results m_solution_count;
  };
  
  enum 
    {
      /*!
        Indicates that and end point
        of a Bezier curve has that 
        the x-value is extremal, i.e.
        if a vertical line passes through
        the point, then the intersection
        should be counted with multiplicity
        2.
       */
      x_extremal_flag=1,

      /*!
        Indicates that and end point
        of a Bezier curve has that 
        the y-value is extremal, i.e.
        if a horizontal line passes through
        the point, then the intersection
        should be counted with multiplicity
       */     
      y_extremal_flag=2,
    };

  /*!\class BezierCurve
    Class the representation of a Bezier curve
    of degree no more than 3 (i.e. lines, quadratics
    and cubics are supported).
   */
  class BezierCurve
  {
  public:
    /*!\class maximal_minimal_point_type
      Data type to hold evalulation data of the
      BezierCurve where the sum or difference
      of derivative in x and of derivative in y
      is zero.
     */
    class maximal_minimal_point_type
    {
    public:
      /*!\var m_multiplicity     
        the multiplicity of the polynomial solution.
       */
      int m_multiplicity;

      /*!\var m_t      
        the value of the paremeter factor of the solution
       */
      float m_t;

      /*!\var m_pt      
        the value of the curve at the point
       */
      vec2 m_pt;

      /*!\var m_derivative      
        the value of the derivative of the curve at the point
       */
      vec2 m_derivative;
    };

    /*!\fn BezierCurve(void)    
      Empty ctor.
     */
    BezierCurve(void):
      m_curveID(-1),
      m_contourID(-1)
    {}

    /*!\fn BezierCurve(geometry_data, GLushort, GLushort)
      Initialize the BezierCurve to represent a line
      segment.
      \param dbg source for point data for the curve
      \param ind0 _INDEX_ of the 0'th control point
      \param ind1 _INDEX_ of the 1'th control point
     */    
    BezierCurve(geometry_data dbg, 
                GLushort ind0, GLushort ind1);

    /*!\fn BezierCurve(geometry_data, GLushort, GLushort, GLushort)
      Initialize the BezierCurve to represent a 
      quadratic curve.
      \param dbg source for point data for the curve
      \param ind0 _INDEX_ of the 0'th control point
      \param ind1 _INDEX_ of the 1'th control point
      \param ind2 _INDEX_ of the 2'th control point
     */    
    BezierCurve(geometry_data dbg, 
                GLushort ind0, GLushort ind1, GLushort ind2);
    

    /*!\fn BezierCurve(geometry_data, GLushort, GLushort, GLushort, GLushort)
      Initialize the BezierCurve to represent a 
      cube curve.
      \param dbg source for point data for the curve
      \param ind0 _INDEX_ of the 0'th control point
      \param ind1 _INDEX_ of the 1'th control point
      \param ind2 _INDEX_ of the 2'th control point
      \param ind3 _INDEX_ of the 3'th control point
     */     
    BezierCurve(geometry_data dbg, 
                GLushort ind0, GLushort ind1, GLushort ind2, GLushort ind3);

    /*!\fn BezierCurve(geometry_data, const std::vector<GLushort> &)
      Initialize the BezierCurve to represent a curve of run time
      determined degree.
      \param dbg source for point data for the curve
      \param indices array of _INDICES_ of the control points
     */
    BezierCurve(geometry_data dbg, 
                const std::vector<GLushort> &indices);

    /*!\fn void reverse_curve
      Reverses the curve, i.e. reparemterize the
      curve c(t) to c(1-t), which is equivalent to
      reversing the order of the control points.
     */
    void
    reverse_curve(void);

    /*!\fn vecN<BezierCurve*,2> split_curve
      Creates 2 new BezierCurves, the returns value's
      .x() field holds this curve of the range [0, 0.5]
      reparameterized to [0,1] and the .y() field
      holds this curve of the range [0.5, 1] 
      reparameterized to [0,1].
      \param dbg source for point data for the curve
     */
    vecN<BezierCurve*,2>
    split_curve(geometry_data dbg) const;

    /*!\fn enum return_code approximate_cubic(geometry_data, 
                                              vecN<BezierCurve*, 4>&) const
      If this BezierCurve is a cubic, then approximate
      the cubic to 4 quardratic curves. If the curve
      is not a cubic, return routine_fail and sets
      out_curves as an array of NULL-pointers.
      Note: approximation of a cubic by 4 quadratics 
      usually gives a very good approximation, see
      the demo: http://timotheegroleau.com/Flash/articles/cubic_bezier_in_flash.htm,
      under the section "Fixed MidPoint approach",
      this is the approach used.
      \param dbg source for point data for the curve
                 (point data WILL be added)
      \param out_curves location to place the new curves      
     */
    enum return_code
    approximate_cubic(geometry_data dbg, 
                      vecN<BezierCurve*, 4> &out_curves) const;

    /*!\fn enum return_code approximate_cubic(geometry_data, 
                                              vecN<BezierCurve*, 2>&) const
      If this BezierCurve is a cubic, then approximate
      the cubic to 2 quardratic curves. If the curve
      is not a cubic, return routine_fail and sets
      out_curves as an array of NULL-pointers.
      Note: approximation of a cubic by 2 quadratics 
      can be poor (but still is better than by a single)
      you have been warned.
      \param dbg source for point data for the curve
                 (point data WILL be added)
      \param out_curves location to place the new curves      
     */
    enum return_code
    approximate_cubic(geometry_data dbg, 
                      vecN<BezierCurve*, 2> &out_curves) const;

    
    /*!\fn BezierCurve* approximate_cubic(geometry_data) const
      If this BezierCurve is a cubic, then approximate
      the cubic to a single quardratic curve. If the curve
      is not a cubic, returns NULL. Note: the approximation
      is quite poor, you have been warned.

      \param dbg source for point data for the curve
                 (point data WILL be added)
     */
    BezierCurve*
    approximate_cubic(geometry_data dbg) const;

    /*!\fn enum return_code approximate_cubic(geometry_data, 
                                              vecN<BezierCurve*, 1>&) const
      If this BezierCurve is a cubic, then approximate
      the cubic to 1 quardratic curve. If the curve
      is not a cubic, return routine_fail and sets
      out_curves as an array of NULL-pointers.
      Note: approximation of a cubic by 1 quadratics 
      can be quite poor, you have been warned.

      \param dbg source for point data for the curve
                 (point data WILL be added)
      \param out_curves location to place the new curve      
     */
    enum return_code
    approximate_cubic(geometry_data dbg, 
                      vecN<BezierCurve*, 1> &out_curves) const
    {
      out_curves[0]=approximate_cubic(dbg);
      return out_curves[0]!=NULL?
        routine_success:
        routine_fail;
    }
    
    /*!\fn const std::vector<ivec2>& control_points
      Returns the control points
      of this BezierCurve.
     */
    const std::vector<ivec2>&
    control_points(void) const
    {
      return m_raw_curve;
    }
    
    /*!\fn const ivec2& control_point(int) const
      Returns the named control point.
      \param I index of control point
     */
    const ivec2&
    control_point(int I) const
    {
      WRATHassert(I>=0 and I<degree());
      return m_raw_curve[I];
    }
    
    /*!\fn const vecN<std::vector<int>, 2>& curve
      returns the parameterization of the curve
      on [0,1] packed as:\n

      \f$x(t) = \sum_{i=0}^{curve.x().size()-1} ( curve().x()[i] * t^i )\f$\n
      \f$y(t) = \sum_{i=0}^{curve.y().size()-1} ( curve().y()[i] * t^i )\f$\n
     */
    const vecN<std::vector<int>, 2>& 
    curve(void) const
    {
      return m_curve;
    }

    /*!\fn const std::vector<GLushort>& control_point_indices
      Returns the indices within
      the constructing \ref geometry_data of the 
      control points of this BezierCurve.
     */
    const std::vector<GLushort>&
    control_point_indices(void) const
    {
      return m_raw_index; 
    }

    /*!\fn const ivec2& pt0
      Returns the starting point of the BezierCurve,
      i.e. the 0th control point which is the same
      as the BezierCurve evaluated at t=0.0.
     */
    const ivec2&
    pt0(void) const
    {
      return m_raw_curve.front();
    }

    /*!\fn const ivec2& pt1
      Returns the ending point of the BezierCurve,
      i.e. the last control point which is the same
      as the BezierCurve evaluated at t=1.0.
     */
    const ivec2&
    pt1(void) const
    {
      return m_raw_curve.back();
    }

    /*!\fn const ivec2& deriv_ipt0
      Returns the velocity vector
      as integer values
      at the starting point of the
      BezierCurve.
     */
    const ivec2&
    deriv_ipt0(void) const
    {
      return m_deriv_ipt0;
    }

    /*!\fn const ivec2& deriv_ipt1    
      Returns the velocity vector
      as integer values
      at the ending point of the
      BezierCurve.
     */
    const ivec2&
    deriv_ipt1(void) const
    {
      return m_deriv_ipt1;
    }


    /*!\fn int tag_pt0    
      Returns the extermal tag data of the starting
      point of the BezierCurve, see x_extremal_flag 
      and y_extremal_flag.   
     */
    int 
    tag_pt0(void)
    {
      return m_tag_pt0;
    }

    /*!\fn int tag_pt1    
      Returns the extermal tag data of the ending 
      point of the BezierCurve, see x_extremal_flag 
      and y_extremal_flag.      
     */
    int
    tag_pt1(void)
    {
      return m_tag_pt1;
    }

    /*!\fn const vec2& fpt0    
      Returns the starting point of the BezierCurve
      as a floating point point.
     */
    const vec2&
    fpt0(void) const
    {
      return m_pt0;
    }

    /*!\fn const vec2& fpt1    
      Returns the ending point of the BezierCurve
      as a floating point point.
     */    
    const vec2&
    fpt1(void) const
    {
      return m_pt1;
    }

    /*!\fn const vec2& deriv_fpt0    
      Returns the velocity vector
      as floating point values
      at the starting point of the
      BezierCurve.
     */
    const vec2&
    deriv_fpt0(void) const
    {
      return m_deriv_fpt0;
    }

    /*!\fn const vec2& deriv_fpt1    
      Returns the velocity vector
      as floating point values
      at the ending point of the
      BezierCurve.
     */
    const vec2&
    deriv_fpt1(void) const
    {
      return m_deriv_fpt1;
    }

    /*!\fn const std::vector<maximal_minimal_point_type>& maximal_minimal_points    
      Returns the points where the sum or
      difference of the derivatives of the
      coordiante functions is 0, packed as
      .first is the multiplicity of solution
      point and .second is the evaluation 
      of the BezierCurve at the point.
     */
    const std::vector<maximal_minimal_point_type>& 
    maximal_minimal_points(void) const
    {
      return m_maximal_minimal_points;
    }

    /*!\fn const std::vector<vec2>& extremal_points
      Returns the extremal points of the curve,
      i.e. those where the named derivative is 0.
      \param coord if coord is 0, returns where the x-derivative is 0,
                   if coord is 1, returns where the y-derivative is 0,
                   all other values are invalid.
     */
    const std::vector<vec2>&
    extremal_points(int coord) const
    {
      return m_extremal_points[coord];
    }

    /*!\fn void compute_line_intersection(int, enum WRATHUtil::coordinate_type,
                                          std::vector<simple_line>&, bool) const
      Compute the intersecion of the curve with a 
      hozizontal or vertical line.
      \param in_pt coordinate of line
      \param tp type of line, x_fixed indicates
                a vertical line and y_fixed indicates
                a horizontal line.
      \param out_lines record of intersection to add to if
                       an intersecion is found.
      \param include_pt_intersections if true also include intersections
                                      of the line with end points of the curve
     */
    void
    compute_line_intersection(int in_pt, enum WRATHUtil::coordinate_type tp, 
                              std::vector<simple_line> &out_lines,
                              bool include_pt_intersections) const;

    /*!\fn void compute_line_intersection(int, enum WRATHUtil::coordinate_type,
                                          std::vector<solution_point>&, bool) const
      Compute the intersecion of the curve with a 
      hozizontal or vertical line.
      \param in_pt coordinate of line
      \param tp type of file, x_fixed indicates
                a vertical line and y_fixed indicates
                a horizontal line.
      \param out_pts record of intersection to add to if
                     an intersecion is found.
      \param compute_derivatives if true, compute the value of
                                 the derivatives at the intersecion
                                 points, placing them into the 
                                 m_derivative field, otherwise
                                 leave that field as vec2(0,0).
     */
    void 
    compute_line_intersection(int in_pt, enum WRATHUtil::coordinate_type tp, 
                              std::vector<solution_point> &out_pts,
                              bool compute_derivatives) const;

    /*!\fn void print_info
      Print data (in a human readable format)
      of this BezierCurve to an std::ostream.
      \param ostr stream to which to print data.
     */
    void
    print_info(std::ostream &ostr) const;

    /*!\fn vec2 compute_pt_at_t    
      Evaluate the curve at a point.
      \param t point to evaluate curve, t=0
               is the beginning of the curve,
               t=1 is the end.
     */
    vec2
    compute_pt_at_t(float t) const
    {
      return compute_pt_at_t_worker(t, 
                                    const_c_array<ivec2>(&m_raw_curve[0], m_raw_curve.size()-1),
                                    const_c_array<ivec2>(&m_raw_curve[1], m_raw_curve.size()-1));
    }

    /*!\fn vec2 compute_deriv_at_t    
      Evaluate the derivative of the curve at a point.
      \param t point to evaluate curve, t=0
               is the beginning of the curve,
               t=1 is the end.
     */    
    vec2
    compute_deriv_at_t(float t) const;

    /*!\fn const vec2& min_corner    
      Returns the min-corner of bounding
      box of the curve.
     */
    const vec2& 
    min_corner(void) const
    {
      return m_min_corner;
    }

    /*!\fn const vec2& max_corner    
      Returns the max-corner of bounding
      box of the curve.
     */
    const vec2& 
    max_corner(void) const
    {
      return m_max_corner;
    }

    /*!\fn int curveID(void) const    
      Returns the curve ID as set in the ctor.
     */
    int
    curveID(void) const
    {
      return m_curveID;
    }

    /*!\fn void curveID(int)    
      Change the curve ID.
      \param i new value to which to set the curveID
     */
    void
    curveID(int i)
    {
      m_curveID=i;
    }

    /*!\fn int contourID(void) const    
      Returns the outline ID as set in the ctor.
     */
    int
    contourID(void) const
    {
      return m_contourID;
    }

    /*!\fn void contourID(int)
      Change the outline ID.
      \param i new value to which to set the contourID
     */
    void
    contourID(int i)
    {
      m_contourID=i;
    }

    /*!\fn int degree
      Returns the degree of the curve.
     */
    int
    degree(void) const
    {
      return std::max( static_cast<int>(m_raw_curve.size()), 1) - 1;
    }

    /*!\fn void init_pt_tags
      Internal routine used by OutlineData, do not 
      touch!
     */
    void
    init_pt_tags(const BezierCurve *prev_curve,
                 const BezierCurve *next_curve);
   
  private:

    static
    vec2
    compute_pt_at_t_worker(float t, const_c_array<ivec2> p0, const_c_array<ivec2> p1);

    void
    compute_maximal_minimal_points(void);

    void
    compute_extremal_points(void);

    void
    compute_bounding_box(void);

    void
    init(geometry_data dbg);

    std::vector<GLushort> m_raw_index; //indices of the control points of the curve.
    std::vector<ivec2> m_raw_curve; //stored as a bezier curve, i.e. control and end points.
    vecN<std::vector<int>, 2> m_curve; //stored as a polynomial
    vec2 m_pt0, m_pt1;
    vec2 m_deriv_fpt0, m_deriv_fpt1;
    ivec2 m_deriv_ipt0, m_deriv_ipt1;
    std::vector<maximal_minimal_point_type> m_maximal_minimal_points;
    vecN<std::vector<vec2>, 2> m_extremal_points;

    vec2 m_min_corner;
    vec2 m_max_corner;
    int m_curveID;
    int m_contourID;
    int m_tag_pt0, m_tag_pt1;
  };

  /*!\class ContourEmitterBase
    ContourEmitterBase defines an interface to produce
    contour data; the data is transmitted by emitting 
    signals.
   */
  class ContourEmitterBase
  {
  public:
    /*!\typedef signal_emit_curve
      conveniance typedef for the signal type
      when a curve is emitted.
     */
    typedef boost::signals2::signal<void (BezierCurve*) > signal_emit_curve;

    /*!\typedef signal_end_contour
      conveniance typedef for the signal type
      when a contour ends
     */
    typedef boost::signals2::signal<void () > signal_end_contour;

    virtual
    ~ContourEmitterBase(void) 
    {}

    /*!\fn void produce_contours
      To be implemented by a derived class to construct
      \ref BezierCurve objecs and to emit them via
      emit_curve(). Between connected outlines
      to emit an end contour signal by calling 
      \ref emit_end_contour()
      \param data object in which to place point data of
                  constructed \ref BezierCurve objects
     */
    virtual
    void
    produce_contours(geometry_data data)=0;

    /*!\fn void emit_curve
      To be called during produce_contours() to
      emit a curve
      \param c curve to emit
     */
    void
    emit_curve(BezierCurve *c)
    {
      m_c(c);
    }

    /*!\fn void emit_end_contour
      To be called during produce_contours() to
      emit an end of contour signal.
     */
    void
    emit_end_contour(void)
    {
      m_o();
    }

    /*!\fn boost::signals2::connection connect_emit_curve
      Connect to the emit curve signal
      \param c slot to call on emit curve signal
     */
    boost::signals2::connection
    connect_emit_curve(signal_emit_curve::slot_type c)
    {
      return m_c.connect(c);
    }

    /*!\fn boost::signals2::connection connect_emit_end_contour
      Connect to the emit end of outline signal
      \param o slot to call on emit end of contour signal
     */
    boost::signals2::connection
    connect_emit_end_contour(signal_end_contour::slot_type o)
    {
      return m_o.connect(o);
    }

  private:
    signal_emit_curve m_c;
    signal_end_contour m_o;
  };

  /*!\class ContourEmitterFromFT_Outline
    Contour emitter from data of an FT_Outline.
    One must make sure that the FT_Outline data
    stays valid for the lifetime of the
    ContourEmitterFromFT_Outline object
    is in scope.
   */
  class ContourEmitterFromFT_Outline:public ContourEmitterBase
  {
  public:
    /*!\fn ContourEmitterFromFT_Outline
      Ctor.
      \param outline FT_Outline from which to emit data
      \param pscale_factor scaling factor to apply to outline when
                           create \ref BezierCurve objects
     */ 
    ContourEmitterFromFT_Outline(const FT_Outline &outline, int pscale_factor):
      m_outline(outline),
      m_scale_factor(pscale_factor)
    {}

    virtual
    void
    produce_contours(geometry_data data);

  private:

    void
    add_curves_from_contour(geometry_data data,
                            bool reverse_orientation,
                            const_c_array<FT_Vector> pts,
                            const_c_array<char> pts_tag,
                            int scale);

    const FT_Outline &m_outline;
    int m_scale_factor;
  };


  /*!\class RawOutlineData
    A RawOutlineData object simple holds the
    Bezeir Curves of an FT_Outline.  
   */
  class RawOutlineData
  {
  public:
    /*!\fn RawOutlineData(const FT_Outline&, int, geometry_data)    
      Ctor. Load the Bezier curves from an
      FT_Outline and store them with a scaling
      factor applied.

      \param outline FT_Outline object from which 
                     to extract contours.
      \param pscale_factor scaling factor to apply to control
                           points upon extraction
      \param dbg actual store for point data of the 
                 contours along with debug text output
               
     */
    RawOutlineData(const FT_Outline &outline, 
                   int pscale_factor,
                   geometry_data dbg);

    /*!\fn RawOutlineData(ContourEmitterBase*, geometry_data)
      Ctor. Create the Bezier curves using
      a \ref ContourEmitterBase derived object.

      \param emitter emitter object used to emit the curves
      \param dbg actual store for point data of the 
                 contours along with debug text output
      
     */
    RawOutlineData(ContourEmitterBase *emitter,
                   geometry_data dbg);
    

    virtual
    ~RawOutlineData();
    
    
    /*!\fn const BezierCurve* prev_neighbor
      Returns the BezierCurve neighboring
      on where the passed curve begins.
      \param C BezierCurve
     */
    const BezierCurve*
    prev_neighbor(const BezierCurve *C) const;    

    /*!\fn const BezierCurve* next_neighbor
      Returns the BezierCurve neighboring
      on where the passed curve ends.
      \param C BezierCurve
     */
    const BezierCurve*
    next_neighbor(const BezierCurve *C) const;

    /*!\fn const BezierCurve* bezier_curve
      Return the curve of the named ID.
      \param ID curve ID 
     */
    const BezierCurve*
    bezier_curve(int ID) const
    {
      WRATHassert(ID>=0);
      WRATHassert(ID<static_cast<int>(m_bezier_curves.size()));
      return m_bezier_curves[ID];
    }

    /*!\fn int number_curves
      Returns the number of curves of the
      outline.
     */
    int
    number_curves(void) const
    {
      return m_bezier_curves.size();
    }

    /*!\fn const geometry_data& dbg
      Returns the holder for the debug data,
      point data and contour partitions of the outline.
     */
    const geometry_data&
    dbg(void) const
    {
      return m_dbg;
    }

    /*!\fn const range_type<int>& component
      An FT_Outline is often made up from several
      disconnected loops of curves (for example the
      letter O has two circels). This function returns
      what curves make up a named component as a
      range of indices to use with bezier_curve(int).
      \param C which component, must be no more than number_components().
     */
    const range_type<int>&
    component(int C) const
    {
      WRATHassert(C>=0 and static_cast<unsigned int>(C)<m_curve_sets.size());
      return m_curve_sets[C];
    }

    /*!\fn int number_components
      Returns the number of connected components
      of the outline, see also component().
     */
    int
    number_components(void) const
    {
      return m_curve_sets.size();
    }

    /*!\fn const std::vector<range_type<int> > components
      Returns the array which describes
      the components, \sa component(int) and
      \sa number_components(void).
     */
    const std::vector<range_type<int> >&
    components(void) const
    {
      return m_curve_sets;
    }

    /*!\fn void reverse_component
      Reverse the orientation of a component
      \param ID index into return value of \ref components() of
                contour to reverse
     */
    void
    reverse_component(int ID);

  private:
    

    void
    build_outline(ContourEmitterBase *emitter);
    
    void
    mark_contour_end(void);

    void
    catch_curve(BezierCurve *c);

    std::vector<BezierCurve*> m_bezier_curves;
    std::vector<range_type<int> > m_curve_sets;
    geometry_data m_dbg;

  };

  /*!\enum bitmap_conversion_t
    Enumeration specifying conversion to
    and from bitmap "offsetting", i.e.
    to/from center/begin of bitmap
   */
  enum bitmap_conversion_t
    {
      /*!
        When coverting from bitmap coordinates,
        use the position of the center of the texel
       */
      bitmap_center,

      /*!
        When coverting from bitmap coordinates,
        use the position of the start of the texel        
       */
      bitmap_begin
    };

  /*!\class CoordinateConverter
    A class for converting to and from bitmap
    coordinates of Freetype data. The coordinate
    systems are bitmap-coordinates, refering to
    a texel within a bitmap and point-coordinates
    referring to coordinates of points of the 
    outline of an FT_Outline. Points coordinates, 
    following freetype, are already multiplied 
    by a 64 (representing that FT_Point data in
    FreeType is in the format 24.6).
   */
  class CoordinateConverter
  {
  public:
    /*!\fn CoordinateConverter
      Ctor. 
      \param pscale_factor scaling factor _composited_
                           with the factor of 64 applied
                           to point coordinates.
      \param bitmap_size size of L1 distance texture
                         to compute in same units
                         at the data of outline.
      \param bitmap_offset offset of L1 distance texture
                           in same units as bitmap_size,
                           the offset is usually non-zero
                           for those glyphs that "droop"
                           below, such as "p".
      \param pinternal_offset value to which to use for
                              internal_offset()
     */
    CoordinateConverter(int pscale_factor,
                        const ivec2 &bitmap_size, 
                        const ivec2 &bitmap_offset,
                        int pinternal_offset=-1);
    
    /*!\fn vec2 normalized_glyph_coordinate
      Returns a coordinate as found in the 
      BezierCurve 's of bezier_curves() into
      a normalized coordinate within the glyph,
      i.e. 0=left/bottom, 1=top/right.
      \param ipt coordinate in same units and coordinate
                 system as the coordinates of the BezierCurve 's
                 of this OutlineData
     */
    vec2
    normalized_glyph_coordinate(const ivec2 &ipt) const;

    /*!\fn int scale_factor
      Returns the scale factor, i.e. the factor applied
      to the points from FreeType, this value is set
      at the ctor.
     */
    int 
    scale_factor(void) const
    {
      return m_scale_factor;
    }
    
    /*!\fn const ivec2& bitmap_offset
      return the bitmap offset as passed in the ctor.
     */
    const ivec2&
    bitmap_offset(void) const
    {
      return m_bitmap_offset;
    }

    /*!\fn int internal_offset
      Point coordinates are also shifted
      by a small amout to guarantee that
      points of a curve are never on the
      boundary of a texel of the bitmap.
      This returns that small shifing 
      values (typically -1).
     */
    int
    internal_offset(void) const
    {
      return m_internal_offset;
    }

    /*!\fn const ivec2& bitmap_size
      Returns the size of the underlying
      bitmap that is used in computations,
      this value is set at ctor.
     */
    const ivec2&
    bitmap_size(void) const
    {
      return m_bitmap_size;
    }

    /*!\fn int point_from_bitmap_x
      Converts from coordinates of bitmap 
      to coordinates of the BezierCurve
      objects.
      \param x x-coordinate to convert
      \param t bitmap conversion convention
     */
    int
    point_from_bitmap_x(int x, enum bitmap_conversion_t t=bitmap_center) const
    {
      return point_from_bitmap_coord(x, WRATHUtil::x_fixed, t);
    }

    /*!\fn int point_from_bitmap_y
      Converts from coordinates of bitmap 
      to coordinates of the BezierCurve
      objects.
      \param y y-coordinate to convert
      \param t bitmap conversion convention
     */
    int
    point_from_bitmap_y(int y, enum bitmap_conversion_t t=bitmap_center) const
    {
      return point_from_bitmap_coord(y, WRATHUtil::y_fixed, t);
    }

    /*!\fn int bitmap_x_from_point
      Converts TO coordinates of bitmap 
      from coordinates of the BezierCurve
      objects.
      \param x x-coordinate to convert
      \param t bitmap conversion convention
     */
    int
    bitmap_x_from_point(float x, enum bitmap_conversion_t t=bitmap_center) const
    {
      return bitmap_coord_from_point(x, WRATHUtil::x_fixed, t);
    }

    /*!\fn int bitmap_y_from_point    
      Converts TO coordinates of bitmap 
      from coordinates of the BezierCurve
      objects.
      \param y y-coordinate to convert
      \param t bitmap conversion convention
     */
    int
    bitmap_y_from_point(float y, enum bitmap_conversion_t t=bitmap_center) const
    {
      return bitmap_coord_from_point(y, WRATHUtil::y_fixed, t);
    }

    /*!\fn vec2 bitmap_from_point(vec2, enum bitmap_conversion_t) const
      Converts from coordinates of bitmap 
      to coordinates of the BezierCurve
      objects.
      \param p coordinate to convert
      \param t bitmap conversion convention
     */
    vec2
    bitmap_from_point(vec2 p, enum bitmap_conversion_t t=bitmap_center) const
    {
      p-=vec2(m_internal_offset, m_internal_offset);
      p/=static_cast<float>(scale_factor());
      if(t==bitmap_center)
        {
          p-=vec2(32.0f, 32.0f);
        }
      p/=64.0f;
      p-=vec2( static_cast<float>(m_bitmap_offset.x()),
               static_cast<float>(m_bitmap_offset.y()) );
      return p;
    }
    
    /*!\fn vec2 bitmap_from_point(ivec2, enum bitmap_conversion_t) const    
      Converts from coordinates of bitmap 
      to coordinates of the BezierCurve
      objects.
      \param p coordinate to convert
      \param t bitmap conversion convention
     */
    vec2
    bitmap_from_point(ivec2 p,
                      enum bitmap_conversion_t t=bitmap_center) const
    {
      return bitmap_from_point(vec2(p), t);
    }

    /*!\fn int bitmap_coord_from_point(float, enum WRATHUtil::coordinate_type, 
                                       enum bitmap_conversion_t)  const    
      Converts from coordinates of bitmap 
      to coordinates of the BezierCurve
      objects.
      \param v coordinate to convert
      \param tp which coordinate (use WRATHUtil::x_fixed for x-coordinate
                and WRATHUtil::y_fixed for y-coordinate)
      \param t bitmap conversion convention
     */
    int
    bitmap_coord_from_point(float v, enum WRATHUtil::coordinate_type tp, 
                            enum bitmap_conversion_t t=bitmap_center) const
    {
      v-=static_cast<float>(m_internal_offset);
      v/=static_cast<float>(scale_factor());
      if(t==bitmap_center)
        {
          v-=32.0f;
        }
      v/=64.0f;
      v-=static_cast<float>(m_bitmap_offset[tp]);
      return static_cast<int>(v);
    }

    /*!\fn int point_from_bitmap_coord(int, enum WRATHUtil::coordinate_type, 
                                       enum bitmap_conversion_t)  const    
      Converts TO coordinates of bitmap 
      from coordinates of the BezierCurve
      objects.
      \param ip coordinate to convert
      \param tp which coordinate (use WRATHUtil::x_fixed for x-coordinate
                and WRATHUtil::y_fixed for y-coordinate)
      \param t bitmap conversion convention
     */
    int
    point_from_bitmap_coord(int ip, enum WRATHUtil::coordinate_type tp, 
                            enum bitmap_conversion_t t=bitmap_center) const
    { 
      ip+=m_bitmap_offset[tp];
      ip= ip*64 + ((t==bitmap_center)?32:0);
      ip*=scale_factor();
      ip+=m_internal_offset;
      return ip;
    }

    /*!\fn ivec2 point_from_bitmap
      Converts TO coordinates of bitmap from coordinates of the BezierCurve
      objects.
      \param ip ivec2 coordinates to convert
      \param t bitmap conversion convention
     */
    ivec2
    point_from_bitmap(ivec2 ip, enum bitmap_conversion_t t=bitmap_center) const
    {
      
      ip+=m_bitmap_offset;
      ip= ip*64 + ivec2( ((t==bitmap_center)?32:0) );
      ip*=scale_factor();
      ip+=ivec2(m_internal_offset);
      return ip;
    }

    /*!\fn float bitmap_from_point(float, int coordinate, enum bitmap_conversion_t) const    
      Converts tp coordinates of bitmap 
      from coordinates of the BezierCurve
      objects.
      \param p coordinate to convert
      \param coordinate index of coordinate (use 0 for x-coordinate, 1 for y-coordinate)
      \param t bitmap conversion convention
     */
    float
    bitmap_from_point(float p, int coordinate, 
                      enum bitmap_conversion_t t=bitmap_center) const
    {
      p-=m_internal_offset;
      p/=static_cast<float>(scale_factor());
      p-=((t==bitmap_center)?32.0f:0.0f);
      p/=64.0f;
      p-=static_cast<float>(m_bitmap_offset[coordinate]);
      return p;
    }
       
    /*!\fn float distance_scale_factor
      Returns the reciprocal of \ref scale_factor().
     */
    float
    distance_scale_factor(void) const
    {
      return m_distance_scale_factor;
    }

    /*!\fn bool same_texel
      Returns the true if and only if the
      2 points passed are within the same
      texel of the bitmap.
     */
    bool
    same_texel(ivec2 pt0,
               ivec2 pt1) const;
       
    /*!\fn texel(ivec2) const

      Returns the texel that a point 
      is within.

      \param pt the point to check
     */
    ivec2
    texel(ivec2 pt) const;

    /*!\fn int half_texel_size
      Returns half the size of a bitmap texel
      in point coordinates (i.e. 32*scale_factor()).
     */
    int
    half_texel_size(void) const
    {
      return m_half_texel_size;
    }
       
    /*!\fn const vec2& texel_size_f
      Returns the texel size as a floating point value.
     */    
    const vec2&
    texel_size_f(void) const
    {
      return m_texel_size_f;
    }
       
    /*!\fn ivec2 compute_texel_bottom_left
      Returns the point coordinate of the 
      bottom left corner of a texel.
      \param bitmap_location location of texel within bitmap
     */
    ivec2
    compute_texel_bottom_left(const ivec2 &bitmap_location) const
    {
      return ivec2(point_from_bitmap_x(bitmap_location.x()) - m_half_texel_size,
                   point_from_bitmap_y(bitmap_location.y()) - m_half_texel_size);
    }
       
    /*!\fn ivec2 compute_texel_top_right
      Returns the point coordinate of the 
      top right corner of a texel.
      \param bitmap_location location of texel within bitmap
     */
    ivec2
    compute_texel_top_right(const ivec2 &bitmap_location) const
    {
      return ivec2(point_from_bitmap_x(bitmap_location.x()) + m_half_texel_size,
                   point_from_bitmap_y(bitmap_location.y()) + m_half_texel_size);                   
    }
       
    /*!\fn const vec2& glyph_size
      Returns the size of the glyph, essentially the size,
      in point coordinates between the bottom left corner
      of the bottom left texel and the top right corner
      of the top right texel.
     */
    const vec2&
    glyph_size(void) const
    {
      return m_glyph_size;
    }
       
    /*!\fn const vec2& glyph_top_right
      Returns the point coordinate of the top right corner
      of the top right texel.
     */
    const vec2&
    glyph_top_right(void) const
    {
      return m_glyph_top_right;
    }
       
    /*!\fn const vec2& glyph_bottom_left
      Returns the point coordinate of the bottom left corner
      of the bottom left texel.
     */
    const vec2&
    glyph_bottom_left(void) const
    {
      return m_glyph_bottom_left;
    }

  private:
    int m_scale_factor;
    int m_internal_offset;
    ivec2 m_bitmap_size, m_bitmap_offset;
    int m_half_texel_size;
    float m_distance_scale_factor;

    vec2 m_glyph_bottom_left, m_glyph_top_right, m_glyph_size;
    vec2 m_glyph_size_reciprocal;
    ivec2 m_texel_size_i;
    vec2 m_texel_size_f;

    
  };

  /*!\class OutlineData
    An OutlineData represents the outline of a
    glyph to compute L1 distance textures.
    The points of the BezierCurve stored within
    in an OutlineData are NOT in the same units
    as the FT_Outline used to construct the
    OutlineData.
   */
  class OutlineData:
    public CoordinateConverter,
    public RawOutlineData
  {
  public:

    /*!\fn OutlineData(const FT_Outline&, const ivec2&, const ivec2&, geometry_data)    
      Ctor. 
      \param outline FT_Outline of a glyph
      \param bitmap_size size of L1 distance texture
                         to compute in same units
                         at the data of outline.
      \param bitmap_offset offset of L1 distance texture
                           in same units as bitmap_size,
                           the offset is usually non-zero
                           for those glyphs that "droop"
                           below, such as "p".
      \param pdbg data store for the OutlineData to use.
     */
    OutlineData(const FT_Outline &outline, 
                const ivec2 &bitmap_size, 
                const ivec2 &bitmap_offset,
                geometry_data pdbg);

    /*!\fn OutlineData(ContourEmitterBase*, int, const ivec2 &, const ivec2 &, geometry_data )    
      Ctor. 
      \param emitter object to emit curves 
      \param pscale_factor scaling factor used for coordinate
                           conversions. NOTE! Make sure that the
                           value is consistent with the curves
                           emitted by emitter.
      \param bitmap_size size of L1 distance texture
                         to compute in same units
                         at the data of outline.
                         NOTE! Make sure that the
                         value is consistent with the curves
                         emitted by emitter.
      \param bitmap_offset offset of L1 distance texture
                           in same units as bitmap_size,
                           the offset is usually non-zero
                           for those glyphs that "droop"
                           below, such as "p".
                           NOTE! Make sure that the
                           value is consistent with the curves
                           emitted by emitter.
      \param pdbg data store for the OutlineData to use.
     */
    OutlineData(ContourEmitterBase *emitter,
                int pscale_factor,
                const ivec2 &bitmap_size, 
                const ivec2 &bitmap_offset,
                geometry_data pdbg);

    /*!\fn OutlineData(ContourEmitterBase*, const CoordinateConverter &, geometry_data )    
      Ctor. 
      \param emitter object to emit curves 
      \param converter specifies coordinate conversion to and from
                       bitmap coordinate. NOTE! Make sure that the
                       value is consistent with the curves
                       emitted by emitter.
      \param pdbg data store for the OutlineData to use.
     */
    OutlineData(ContourEmitterBase *emitter,
                const CoordinateConverter &converter,
                geometry_data pdbg);
                   
       
    /*!\fn void compute_distance_values    
      Compute the L1 distance values.
      \param victim location to place the results, the
                    dimensions of victim must be exactly
                    the same as bitmap_size passed to the ctor.
      \param max_dist The recorded distance is saturated to max_dist
      \param compute_winding_number if true, compute the winding number
                                    as well for each texel.
                                 
                      
     */
    void
    compute_distance_values(boost::multi_array<distance_return_type, 2> &victim, 
                            float max_dist,
                            bool compute_winding_number) const;

    /*!\fn void compute_winding_numbers
      Compute the winding numbers, if you are calling already
      compute_distance_values(), extract the winding numbers 
      for it's output. This routine still needs to do polynomial
      solves (height of them).
      \param victim location to place the results, the
                    dimensions of victim must be exactly
                    the same as bitmap_size passed to the ctor.
      \param offset_from_center the winding number is computed at the
                                center of the texel plus this value
                                which is in "point" coordinates, i.e.
                                coordinates of the curves
     */
    void
    compute_winding_numbers(boost::multi_array<int, 2> &victim,
                            ivec2 offset_from_center=ivec2(0,0)) const;

    /*!\fn void compute_analytic_values
      For each texel, compute the intersection of the boundary of the
      texel with the outline.

      \param victim location to place result, the dimension must equal, exactly bitmap_size()
      \param component_reversed Freetype can give unreliable values for the
                                ordering of the points of a contour. Although
                                it provides the flags field with the
                                bit FT_OUTLINE_REVERSE_FILL, it can still
                                get it wrong, the parameter component_reversed
                                will be resized to  number_components() and
                                for each index I, store false if the component 
                                returned by component(I) is oriented correctly, and
                                true if the component is oriented reversed.
      \param include_pt_intersections if true, then if a curves end point intersects
                                      the boudnary include that too.
     */
    void
    compute_analytic_values(boost::multi_array<analytic_return_type, 2> &victim,
                            std::vector<bool> &component_reversed,
                            bool include_pt_intersections=false) const;

    
    /*!\class per_point_data
      Data type used to represents
      point in curve_segment class.
     */
    class per_point_data
    {
    public:
      /*!\fn per_point_data(void)
        Default ctor, all values are uninitialized.
       */
      per_point_data(void)
      {}

      /*!\fn per_point_data(float)
        Implicit ctor from a float, initializes
        only \ref m_time.
        \param t value with which to initialize \ref m_time
       */
      per_point_data(float t):
        m_time(t)
      {}

      /*!\var m_time
        When curve hits.
       */
      float m_time;

      /*!\var m_bitmap_coordinate
        Coordinate of intersection in same
        units as the the bitmap.
       */
      vec2 m_bitmap_coordinate;

      /*!\var m_glyph_normalized_coordinate
        Coordinate of intersection normalized
        to the _texture_.
       */
      vec2 m_glyph_normalized_coordinate;

      /*!\var m_texel_normalized_coordinate
        Coordinate of intersection normalized
        to the _pixel_.
       */
      vec2 m_texel_normalized_coordinate;
    };

    /*!\class curve_segment 
      Used to record an entry of a BezierCurve
      intersecting a texel.
     */
    class curve_segment
    {
    public:
      /*!\var m_control_points
        Control points of the curve.
       */
      std::vector<per_point_data> m_control_points;

      /*!\var m_enter
        When (and if) the curve enters
        the texel.
       */
      enum boundary_type m_enter;

      /*!\var m_exit
        When (and if) the curve exits
        the texel.
       */
      enum boundary_type m_exit;

      /*!\var m_curve
        the curve of the intersection
       */
      const BezierCurve *m_curve;

      /*!\fn bool endpoint_inside_of_texel
        Returns true if and only if
        the curve ends or begins within
        the texel.
       */
      bool
      endpoint_inside_of_texel(void) const
      {
        return (m_enter==no_boundary or m_exit==no_boundary);
      }
    };

    /*!\fn int compute_localized_affectors(const analytic_return_type &,
                                          const ivec2&, c_array<curve_segment>) const      
      Computes the curves intersecting a specified texel.
      Returns the number of curves found.

      \param R input data as produced by compute_analytic_values().
      \param bitmap_location location of texel, must be in same
                             units as bitmap_size of the ctor.
      \param out_curves pre-allocated array to store the output in
     */
    int
    compute_localized_affectors(const analytic_return_type &R,
                                const ivec2 &bitmap_location,
                                c_array<curve_segment> out_curves) const;

    /*!\fn int compute_localized_affectors(const boost::multi_array<analytic_return_type, 2> &,
                                           const ivec2&, c_array<curve_segment>) const    
      Computes the curves intersecting a named texel.
      Returns the number of curves found. Provdied as
      a conveniance, equivalent to 
      \code
      compute_localized_affectors(R[bitmap_location.x()][bitmap_location.y()], bitmap_location, out_curves)
      \endcode

      \param R input data matrix as produced by compute_analytic_values().
      \param bitmap_location location of texel, must be in same
                             units as bitmap_size of the ctor.
      \param out_curves pre-allocated array to store the output in
     */
    int
    compute_localized_affectors(const boost::multi_array<analytic_return_type, 2> &R,
                                const ivec2 &bitmap_location,
                                c_array<curve_segment> out_curves) const
    {
      return compute_localized_affectors( R[bitmap_location.x()][bitmap_location.y()],
                                          bitmap_location, out_curves);
    }

    /*!\fn int compute_localized_affectors_LOD
      Computes the curves intersecting a named texel for a given
      mipmap level LOD.
      \param LOD LOD level
      \param dataLOD0 input data as produced by compute_analytic_values().
      \param LOD_bitmap_location location of texel within bitmap LOD
      \param out_curves pre-allocated array to store the output in
     */
    int
    compute_localized_affectors_LOD(int LOD,
                                    const boost::multi_array<analytic_return_type, 2> &dataLOD0,
                                    const ivec2 &LOD_bitmap_location,
                                    c_array<curve_segment> out_curves) const;
                             
    /*!\fn void print_analytic_generation_data
      Print data as generated by compute_analytic_values()
      to an std::ostream in a human readable format.
      \param str std::ostream to which to print
      \param pdata output of compute_analytic_values() to print.
     */
    void
    print_analytic_generation_data(std::ostream &str,
                                   const boost::multi_array<analytic_return_type, 2> &pdata) const;

    /*!\fn void compute_bounding_box(const BezierCurve*, ivec2&, ivec2&) const
      Compute the bounding box of a curve in units
      of the bitmap_size of the ctor.
      \param c curve to query
      \param out_min place to write min coordinates of bounding box                             
      \param out_max place to write max coordinates of bounding box
     */
    void
    compute_bounding_box(const BezierCurve *c, 
                         ivec2 &out_min, ivec2 &out_max) const;


  private:
    void
    increment_sub_winding_numbers(const std::vector<solution_point> &L,
                                  enum WRATHUtil::coordinate_type coord_tp,
                                  std::vector<int> &cts) const;
    
    void
    compute_fixed_line_values(boost::multi_array<distance_return_type, 2> &victim,
                              bool compute_winding_number) const;

    void
    compute_fixed_line_values(enum WRATHUtil::coordinate_type coord_tp, 
                              boost::multi_array<distance_return_type, 2> &victim,
                              std::vector< std::vector<solution_point> > &work_room,
                              bool compute_winding_number) const;

    void
    compute_outline_point_values(boost::multi_array<distance_return_type, 2> &victim,
                                 int radius) const;

    void
    compute_zero_derivative_values(boost::multi_array<distance_return_type, 2> &victim,
                                   int radius) const;

    void
    init_distance_values(boost::multi_array<distance_return_type, 2> &victim,
                         float max_dist_value) const;
    void
    compute_analytic_curve_values_fixed(enum WRATHUtil::coordinate_type coord,
                                        boost::multi_array<analytic_return_type, 2> &victim,
                                        std::vector<int> &reverse_curve_count,
                                        bool include_pt_intersections) const;

    typedef std::pair<enum boundary_type, const simple_line*> grab_entry;
    typedef std::map<const WRATHFreeTypeSupport::BezierCurve*, std::list<grab_entry> > grab_map;

    int
    compute_localized_affectors_worker(const grab_map &hits_found,
                                       const ivec2 &texel_bottom_left,
                                       const ivec2 &texel_top_right,
                                       c_array<curve_segment> out_curves) const;

    
    
  };


  /*******************************
   Support for Distance texture generation
   by analyzing spans
   ******************************/

  /*!\class Span
    A Span represent a horizontal line segment
    that the FreeType call back renderer 
    produces, closely corresponds to the 
    type FT_Span.
   */
  class Span
  {
  public:    
    /*!\fn Span(const FT_Span&, int)    
      Ctor. Contruct a Span from an FT_Span, note that
      such a span is assumed to be within the glyph,
      as such m_coverage is initialized as true.
      \param ftspan FT_Span from which to contruct the Span
      \param y y-coordinate of the FT_Span
     */
    Span(const FT_Span &ftspan, int y):
      m_x_begin(ftspan.x),
      m_x_end(ftspan.x + ftspan.len),
      m_y(y),
      m_coverage(ftspan.coverage>=127)
    {}  

    /*!\fn Span(int, bool, int, int)    
      Ctor. Construct a Span specifying each of the values
      directly.
      \param y value to which m_y is assigned
      \param covered value to which m_coverage is assigned
      \param beg_x value to which m_x_begin is assigned
      \param end_x value to which m_x_end is assigned
     */
    Span(int y, bool covered, int beg_x, int end_x):
      m_x_begin(beg_x),
      m_x_end(end_x),
      m_y(y),
      m_coverage(covered)
    {}

    /*!\fn bool operator<(const Span&) const    
      Comparison opertor sorted first by m_y,
      then by m_x_begin.
     */
    bool
    operator<(const Span &obj) const
    {
      return (m_y!=obj.m_y)?
        m_y<obj.m_y:
        m_x_begin<obj.m_x_begin;
    }
    /*!\var m_x_begin    
      Gives the x-coordinate where the horizontal span starts.
     */
    int m_x_begin;

    /*!\var  m_x_end    
      Gives the x-coordinate where the horizontal span ends.
     */
    int m_x_end;

    /*!\var m_y    
      Gives the y-coordinate of the horizontal span.
     */
    int m_y;

    /*!\var m_coverage    
      Gives the coverage value of the span,
      i.e. is true if and only if the span
      is within the glyph.
     */
    bool m_coverage;
  };

  /*!\class ScanLineDistanceRenderer
    Class to use FreeType's call back renderer
    API to generate a distance textures
    from spans. _THE_ way to use a ScanLineDistanceRenderer is
    as follows:
        \code
          FT_Face ft_face; //FT face
          int glyph_index; //glyph index of glyph to render
      
          //load the glyph into freetype with the 
          //transformation set to identity
          FT_Set_Transform(ft_face, NULL, NULL);
          FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT);
    
          //tell Freetype2 to render the glyph to a bitmap,
          //this bitmap is located at ft_face->glyph->bitmap
          FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);

          //get the size and offset of the bitmap:
          bitmap_sz=ivec2(ft_face->glyph->bitmap.width,
                          ft_face->glyph->bitmap.rows);
          
          bitmap_offset.x()=ft_face->glyph->bitmap_left;
          bitmap_offset.y()=ft_face->glyph->bitmap_top-ft_face->glyph->bitmap.rows;

          //Create the ScanLineDistanceRenderer renderer
          //power2inflate is the log2 of the scale factor 
          //of the ScanLineDistanceRenderer, a recommended
          //value is 5 (i.e. generate scalines at 32X the
          //resolution.
          int power2inflate(5);

          //The radius is pixels(of the target image, i.e.
          //same "units" as bitmap_sz and bitmap_offset
          //to check to do min(disance) checks
          int pixel_radius(2);
          ScanLineDistanceRenderer SR(bitmap_sz, power2inflate, bitmap_offset);

          //run the algorithm
          SR.generate_spans.generate_spans(ft_face, glyph_index);
          SR.do_pass1();
          SR.do_pass2(pixel_radius);

          //distances are now fetched via 
          //SR.pixel_data()[x][y]
          
        \endcode
   */
  class ScanLineDistanceRenderer
  {
  public:
    /*!\class pixel_data_type
      Data type used to represent the data of a 
      pixel of a distance field texture.
     */ 
    class pixel_data_type
    {
    public:
      /*!\var m_covered
        True if and only if the center of
        the pixel of this pixel_data_type 
        is inside the glyph outline.
       */
      bool m_covered;

      /*!\var m_distance
        Returns the L1-distance, in units
        of the virtual distance textures,
        to the nearest pixel whose m_covered
        is the not of this pixel_data_type's
        m_covered. The distance in pixels
        of the virtual texture, not in the
        units of the bitmap, hence the largest
        value it can have is:
          (1<<power2inflate)*pixel_radius
        where power2inflate is as in the
        constructor of ScanLineDistanceRenderer 
        and pixel_radius is the argument passed
        to ScanLineDistanceRenderer::do_pass2(int).
       */
      int m_distance;
      
      /*!\fn pixel_data_type
        Ctor. Initialiazes \ref m_covered as false
        and \ref m_distance as 1<<12
       */
      pixel_data_type(void):
        m_covered(false),
        m_distance(1<<12)
      {}
    };
    
    /*!\fn ScanLineDistanceRenderer
      Ctor.
        \param bitmap_sz size of target distance texture,
                         usually the same as the bitmap rendered
                         by FreeType with the identity transformation.
        \param bitmap_offset the offset from the origin of the bitmap
                             rendered by freetype.
        \param power2inflate the log2 of the maginfication factor used
                             when generating the span data, i.e. a value
                             of 5 (the recommended value) means that the
                             span data will be generated at 32X the resolution
                             than the bitmap      
     */
    ScanLineDistanceRenderer(const ivec2 &bitmap_sz, 
                             int power2inflate, 
                             const ivec2 &bitmap_offset):
      m_bitmap_sz(bitmap_sz),
      m_bitmap_offset(bitmap_offset),
      m_power2_render_inflate(power2inflate),
      m_scaling_factor(1<<power2inflate),
      m_half_texel_size(m_scaling_factor/2),
      m_image_offset(m_scaling_factor*m_bitmap_offset),
      m_pixel_data(boost::extents[bitmap_sz.x()][bitmap_sz.y()])
    {}
    
    /*!\fn void generate_spans
      Generate the span data, this is the 1st step
      to perform. Once completed, the return value
      of spans() will be ready.
      \param pface FT_Face, i.e. the "font"
      \param glyph_index glyph index of pface of which
                         to compute the distance texture.
     */
    void
    generate_spans(FT_Face pface, int glyph_index);

    /*!\fn int do_pass1
      Execute the 1st pass on the span data
      generated in generate_spans(). After
      completion, the m_covered field 
      of each element of pixel_data() will be
      ready. Returns the number of elements (counting
      repition) of m_covered being set to true.
     */
    int
    do_pass1(void);

    /*!\fn void do_pass2    
      Compute the distance values, once completed
      the m_distance field of each element
      of pixel_data() will be ready.
      \param pixel_radius radius in pixel in native
                          resolution to check for minimum
                          distances.
     */
    void
    do_pass2(int pixel_radius);

    /*!\fn const boost::multi_array<pixel_data_type, 2>& pixel_data
      Returns a const renderence to the 
      distance texture values, call after calling 
      do_pass1() and do_pass2(int).
     */
    const boost::multi_array<pixel_data_type, 2>&
    pixel_data(void) const
    {
      return m_pixel_data;
    }
    
    /*!\fn const std::vector<Span>& spans
      Returns the spans used to generate the
      distance texture values, see 
      generate_spans(FT_Face,int).
     */
    const std::vector<Span>&
    spans(void) const
    {
      return m_spans;
    }

  private:

    int
    point_location_x(int bitmap_inx) const;

    int
    bitmap_location_y(int iny) const;

    int
    bitmap_location_x(int inx) const;

    int
    point_location_y(int bitmap_iny) const;

    bool
    is_texel_center_y(int iny) const;

    bool
    is_texel_center_x(int inx) const;

    void
    handle_span(int bitmap_radius, const Span &sp);

    static
    void
    ft_render_call_back(const int y,
                        const int count,
                        const FT_Span * const spans,
                        void * const user);
    
    
    ivec2 m_bitmap_sz, m_bitmap_offset;
    int m_power2_render_inflate; //power of 2
    int m_scaling_factor, m_half_texel_size;
    ivec2 m_image_offset;

    boost::multi_array<pixel_data_type, 2> m_pixel_data;
    std::vector<Span> m_spans;
  };

  /*!\class LockableFace
    A LockableFace is an FT_Face with a mutex.
    The purpose of a LockableFace is to allow
    for multiple objects to access the same
    FT_Face via locking the mutex.
   */
  class LockableFace:
    public WRATHReferenceCountedObjectT<LockableFace>
  {
  public:
    /*!\fn LockableFace(FT_Face, bool)
      Ctor.
      \param pface FT_Face that the created LockableFace uses.
      \param pis_shared if true, LockableFace does
                        not call the FT_Done_Face at dtor.
    */     
    explicit
    LockableFace(FT_Face pface, bool pis_shared):
      m_face(pface),
      m_is_shared(pis_shared),
      m_mutex(WRATHNew WRATHMutex()),
      m_own_mutex(true)
    {
      WRATHassert(m_face!=NULL);
    }

    /*!\fn LockableFace(FT_Face, WRATHMutex&, bool)
      Ctor.
      \param pface FT_Face that the created LockableFace uses.
      \param pmutex reference to the mutex that the created
                    LockableFace will use. It is an error if the
                    pmutex goes out of scope before the
                    LockableFace.
      \param pis_shared if true, LockableFace does
                        not call the FT_Done_Face at dtor.
    */     
    explicit
    LockableFace(FT_Face pface, WRATHMutex &pmutex, bool pis_shared):
      m_face(pface),
      m_is_shared(pis_shared),
      m_mutex(&pmutex),
      m_own_mutex(false)
    {
      WRATHassert(m_face!=NULL);
    }

          
    ~LockableFace()
    {
      if(!m_is_shared)
        {
          WRATHLockMutex(mutex());
          FT_Done_Face(m_face);
          WRATHUnlockMutex(mutex());
        }

      if(m_own_mutex)
        {
          WRATHDelete(m_mutex);
        }
    }

    /*!\fn WRATHMutex& mutex
      Returns the WRATHMutex of this
      LockableFace
     */
    WRATHMutex&
    mutex(void)
    {
      return *m_mutex;
    }

    /*!\fn FT_Face face
      Returns the FT_Face of 
      this LockableFace
     */
    FT_Face
    face(void)
    {
      return m_face;
    }
    
  private:
    FT_Face m_face;
    bool m_is_shared;
    WRATHMutex *m_mutex;
    bool m_own_mutex;
  };
  
  /*!\class CharacterMapSupport
    FreeType proveds a mapping from character codes
    to glyph indexes, (called a character mapping).
    A CharacterMapSupport gives a slightly easier
    way to handle this. For each glyph of a FT_Face,
    there is data associated to it, that type
    is the template parameter T. The class is thread safe
    and uses a private mutex when manipulating it's data
    and the mutex from a provided LockableFace when accessing
    FreeType.
    \tparam T data type to associated for each glyph of an FT_Face
   */
  template<typename T>
  class CharacterMapSupport:public boost::noncopyable
  {
  public:

    /*!\typedef glyph_index_type
      Conveniance typedef
     */
    typedef WRATHTextureFont::glyph_index_type glyph_index_type;

    /*!\typedef character_code_type
      Conveniance typedef
     */
    typedef WRATHTextureFont::character_code_type character_code_type;

    /*!\class Stats
      A class that can be inserted into an std::ostream
      stream to print the stats of a CharacterMapSupport
     */
    class Stats
    {
    public:
      /*!\fn Stats
        Ctor.
        \param p CharacterMapSupport from which to query values.
       */
      explicit
      Stats(CharacterMapSupport *p):
        m_parent(p)
      {}

      /*!\fn std::ostream& operator<<(std::ostream &, Stats)
        Overload of operator<< to print the values of a \ref Stats
        to an std::ostream
        \param str std::ostream to which to print
        \param obj value to print
      */
      friend
      std::ostream&
      operator<<(std::ostream &str, Stats obj)
      {
        obj.m_parent->print_stats(str);
        return str;
      }

      /*!\fn uint64_t time_to_generate(glyph_index_type)
        Calls \ref CharacterMapSupport::time_to_generate(glyph_index_type)
        on CharacterMapSupport passed in ctor, returning the value.
       */
      uint64_t
      time_to_generate(glyph_index_type glyph)
      {
        return m_parent->time_to_generate(glyph);
      }

      /*!\fn uint64_t total_time_to_generate(void)
        Calls \ref CharacterMapSupport::time_to_generate()
        on CharacterMapSupport passed in ctor, returning the value.
       */
      uint64_t
      total_time_to_generate(void)
      {
        return m_parent->total_time_to_generate();
      }

      /*!\fn const std::map<character_code_type, glyph_index_type>& glyphs
        Calls \ref CharacterMapSupport::glyphs()
        on CharacterMapSupport passed in ctor, returning the value.
       */
      const std::map<character_code_type, glyph_index_type>&
      glyphs(void) const
      {
        return m_parent->glyphs();
      }

      /*!\fn const std::map<glyph_index_type, character_code_type>& character_codes
        Calls \ref CharacterMapSupport::character_codes()
        on CharacterMapSupport passed in ctor, returning the value.
       */
      const std::map<glyph_index_type, character_code_type>&
      character_codes(void) const
      {
        return m_parent->character_codes();
      }

      /*!\fn int number_glyphs
        Calls \ref CharacterMapSupport::number_glyphs()
        on CharacterMapSupport passed in ctor, returning the value.
       */
      int
      number_glyphs(void) const
      {
        return m_parent->number_glyphs();
      }

      /*!\fn int number_glyphs_generated
        Calls \ref CharacterMapSupport::number_glyphs_generated()
        on CharacterMapSupport passed in ctor, returning the value.
       */
      int 
      number_glyphs_generated(void) 
      {
        return m_parent->number_glyphs_generated();
      }

    private:
      CharacterMapSupport *m_parent;
    };

    /*!\fn CharacterMapSupport
      Ctor, uses an existing LockableFace::handle
      \param h LockableFace::handle holding the FT_Face
               from which to build character map data.
    */     
    explicit
    CharacterMapSupport(LockableFace::handle h):
      m_ttf_face(h),
      m_total_time_to_generate(0),
      m_number_glyphs_generated(0)
    {
      init();
    }

    virtual
    ~CharacterMapSupport()
    {
      WRATHLockMutex(m_set_get_data_mutex);
      WRATHLockMutex(m_ttf_face->mutex());
      for(typename std::vector<data_type>::iterator iter=m_data.begin(),
            end=m_data.end(); iter!=end; ++iter)
        {
          if(iter->m_value!=NULL)
            {
              WRATHDelete(iter->m_value);
            }
        }
      WRATHUnlockMutex(m_ttf_face->mutex());
      WRATHUnlockMutex(m_set_get_data_mutex);
    }

    /*!\fn T* data
      Retrieve the data (and if necessary generate)
      from a glyph index. The generation is NOT mutex 
      locked. Thus one can generate glyphs in parallel.
      This method is thread safe and can be executed
      from multiple threads (even requesting the same 
      as yet ungenerated glyphs) safely and will not 
      generate any glyph more than once. Returns NULL
      if the glyph index is invalid or if the glyph index 
      is out of the range of the underlying FT_Face.
      \param glyph glyph index of data to retrieve.
     */
    T*
    data(glyph_index_type glyph)
    {
      
      if(glyph.valid() and glyph.value()<m_data.size())
        {
          WRATHLockMutex(m_set_get_data_mutex);
          if(m_data[glyph.value()].m_value==NULL and !m_data[glyph.value()].m_is_waiting)
            {
              T *ptr;

              /*
                m_data[glyph.value()] is NULL, 
                and is not being generated, set it 
                as waiting within the lock and then 
                immediately unlock.
               */
              m_data[glyph.value()].m_is_waiting=true;
              WRATHUnlockMutex(m_set_get_data_mutex);
              
              /*
                call generate_data outside of the mutex lock,
                derived classes will need to deal with their
                own locking privately, also record the time 
                to generate the glyph.
               */
              struct timeval start_time, end_time;
              uint32_t delta;

              gettimeofday(&start_time, NULL);
              ptr=generate_data(glyph);
              gettimeofday(&end_time, NULL);

              delta=1000000*(end_time.tv_sec-start_time.tv_sec) + (end_time.tv_usec-start_time.tv_usec);

              /*
                Relock and set the value.
               */
              WRATHLockMutex(m_set_get_data_mutex);
              m_data[glyph.value()].m_value=ptr;
              m_data[glyph.value()].m_is_waiting=false;
              m_data[glyph.value()].m_time_to_generate=delta;

              m_total_time_to_generate+=delta;
              ++m_number_glyphs_generated;
            }
          WRATHUnlockMutex(m_set_get_data_mutex);

          /*
            if another thread is generating the glyph,
            wait for it to generate it.
           */
          while(is_waiting(glyph.value()))
            {
              wait_a_little();
            }

          /*
            at this point, m_data will not be
            written to anymore, as such we
            do not need mutex protections.
           */
          return m_data[glyph.value()].m_value;
        }
      return NULL;
    }

    /*!\fn uint64_t time_to_generate(glyph_index_type)
      Returns the time to generate the named glyph in
      microseconds, i.e. millionths of a second.
      If the named glyph was not generated yet, 
      also generates it.
      \param glyph glyph index of data to retrieve.
     */
    uint64_t
    time_to_generate(glyph_index_type glyph)
    {
      if(glyph.valid() and glyph.value()<m_data.size())
        {
          data(glyph);
          return m_data[glyph.value()].m_time_to_generate;
        }
      else
        {
          return 0;
        }
    }

    /*!\fn total_time_to_generate(void)
      Returns the total number of microseconds, 
      i.e. millionths of a second, that have
      been consumed to generate the glyphs
      that have been generated.
     */
    uint64_t
    total_time_to_generate(void)
    {
      uint64_t R;

      WRATHLockMutex(m_set_get_data_mutex);
      R=m_total_time_to_generate;
      WRATHUnlockMutex(m_set_get_data_mutex);

      return R;
    }

    /*!\fn glyph_index_type glyph_index
      Returns the glyph index of the passed
      character code. If the character code
      is not in the character mapping of the
      font, then an invalid glyph_index
      is returned.

      \param C character code to get the glyph index.
     */
    glyph_index_type
    glyph_index(character_code_type C) const
    {
      typename std::map<character_code_type, glyph_index_type>::const_iterator iter;
      
      iter=m_glyph.find(C);
      return (iter!=m_glyph.end())?
        iter->second:
        glyph_index_type();
    }

    /*!\fn character_code_type character_code
      Attempt to guess the character code from
      a glyph index. If the glyph index is invalid
      or refers to a glyph outside of the valid
      glyph index range, returns the character
      code 0.
      \param G glyph index
     */
    character_code_type
    character_code(glyph_index_type G) 
    {
      typename std::map<glyph_index_type, character_code_type>::iterator iter;

      iter=m_ascii.find(G);
      return (iter!=m_ascii.end())?
        iter->second:
        character_code_type(0);
    }

    /*!\fn const std::map<character_code_type, glyph_index_type>& glyphs
      Returns the std::map with keys character_code_type
      and values glyph_index_type, this is the look
      up table used in glyph_index() and is filled
      at construction.
     */
    const std::map<character_code_type, glyph_index_type>&
    glyphs(void) const
    {
      return m_glyph;
    }
    
    /*!\fn const std::map<glyph_index_type, character_code_type>& character_codes
      Returns an std::map with keys glyph_index_type
      and values character_code_type, this table
      is filled at construction.
     */
    const std::map<glyph_index_type, character_code_type>&
    character_codes(void) const
    {
      return m_ascii;
    }

    /*!\fn int number_glyphs    
      Returns the number of glyphs
      of the underlying FT_Face of
      this CharacterMapSupport.
     */
    int
    number_glyphs(void) const
    {
      return m_data.size();
    }

    /*!\fn int number_glyphs_generated
      Returns the number of glyphs generated.
     */
    int
    number_glyphs_generated(void) 
    {
      int R;
      WRATHLockMutex(m_set_get_data_mutex);
      R=m_number_glyphs_generated;
      WRATHUnlockMutex(m_set_get_data_mutex);

      return R;
    }

    /*!\fn int generate_all_glyphs
      Generate all glyphs of the FT_Face 
      \param show_progress if true, print std::cout a progress bar.
      TODO: make the generation of glyphs happen
      in multiple threads.
     */
    int
    generate_all_glyphs(bool show_progress)
    {
      int total_count(m_data.size());
      float percentage_done;
      for(int i=0;i<total_count;++i)
        {
          glyph_index_type G(static_cast<uint32_t>(i));
          T *ptr;

          /* generate the value */
          ptr=data(G);
          WRATHunused(ptr);

          if(show_progress)
            {
              percentage_done=100.0f*static_cast<float>(i+1)/static_cast<float>(total_count);
              std::cout << "\r [";
              for(int M=0;M<50; ++M)
                {
                  char print_char;
                  
                  print_char=(percentage_done/2>M)?'=':' ';
                  std::cout << print_char;
                }
              std::cout << "] " << std::setw(4) 
                        << percentage_done << "% " 
                        << std::setw(5) << i+1
                        << "/" << std::setw(5) << total_count
                        << "     " << std::flush;
            }
          
        }  
      if(show_progress)
        {
          std::cout << "\n";
        }
      return total_count;
    }

    /*!\fn LockableFace::handle  face
      LockableFace of the character map.
     */
    LockableFace::handle 
    face(void) 
    {
      return m_ttf_face;
    }

    /*!\fn float new_line_height
      Conveniance function to guess a default
      line hight (for when a line contains no text),
      returned in pixel units.
      \param pixel_height to use for computing the value
     */
    float 
    new_line_height(int pixel_height)
    {
      float R;

      WRATHLockMutex(m_ttf_face->mutex());
      FT_Set_Pixel_Sizes(m_ttf_face->face(), pixel_height, 0);
      R=static_cast<float>(m_ttf_face->face()->size->metrics.ascender)/64.0f;
      WRATHUnlockMutex(m_ttf_face->mutex());

      return R;
    }

    /*!\fn ivec2 kerning_offset
      Conveniance function to fetch kerning values.
      Returns value in 26.6 fixed point in pixels
      (i.e. the floating point value is given by
      the return value divided by 64.0f).

      \param pixel_height to use for computing the kerning
      \param left_glyph glyph index of glpyh on the left
      \param right_glyph glyph index of glpyh on the right      
     */
    ivec2
    kerning_offset(int pixel_height,
                   glyph_index_type left_glyph,
                   glyph_index_type right_glyph)
    {
      ivec2 R(0,0);
      
      if(m_supports_kerning and left_glyph.valid() and right_glyph.valid())
        {
          FT_Vector v;
          
          WRATHLockMutex(m_ttf_face->mutex());

          FT_Set_Pixel_Sizes(m_ttf_face->face(), pixel_height, 0);
          if(0==FT_Get_Kerning(m_ttf_face->face(),
                               left_glyph.value(),
                               right_glyph.value(),
                               FT_KERNING_UNFITTED, &v))
            {
              R=ivec2(v.x, v.y);
            }
          WRATHUnlockMutex(m_ttf_face->mutex());
        }
      return R;
    }

    /*!\fn void print_stats(std::ostream&)
      Print the stats to an std::ostream
     */
    void
    print_stats(std::ostream &str)
    {
      uint64_t t;
      int count;
      
      t=total_time_to_generate();
      count=number_glyphs_generated();         
      str << "Avg time: " 
          << ( (count>0)? (t/(static_cast<float>(1000*count))): 0.0f)
          << " ms ("
          << count << " glyphs in "
          << t/1000 << " ms) of "
          << m_data.size() << " glyphs, character to index map size: "
          << m_glyph.size()
          << ", index to character map size: "
          << m_ascii.size();
    }

    /*!\fn Stats stats
      Provided as a readability conveniance, equivalent
      to 
      \code
      Stats(this)
      \endcode
     */
    Stats
    stats(void)
    {
      return Stats(this);
    }

  protected:
    /*!\fn generate_data
      To be implemented by a dervied class, called
      by this CharacterMapSupport to generate the
      data for a given index code on the first time
      (and only the first) time that glyph is
      requested.
      \param G glyph index to generate data of.
     */
    virtual
    T*
    generate_data(glyph_index_type G)=0;

  private:

    class data_type
    {
    public:
      bool m_is_waiting;
      T *m_value;
      uint64_t m_time_to_generate;

      data_type(void):
        m_is_waiting(false),
        m_value(NULL),
        m_time_to_generate(0)
      {}
    };

    void
    init(void)
    {
      //generate character map:
      FT_ULong C;
      FT_UInt G;

      WRATHLockMutex(m_ttf_face->mutex());

      for(C=FT_Get_First_Char(m_ttf_face->face(), &G);
          G!=0; 
          C=FT_Get_Next_Char(m_ttf_face->face(), C, &G))
        {
          character_code_type CC(static_cast<uint32_t>(C));
          glyph_index_type GG(static_cast<uint32_t>(G));

          m_glyph[CC]=GG;
          m_ascii[GG]=CC;
        }

      m_data.resize(m_ttf_face->face()->num_glyphs);
      m_supports_kerning=FT_HAS_KERNING(m_ttf_face->face());

      WRATHUnlockMutex(m_ttf_face->mutex());
    }

    bool
    is_waiting(int i)
    {
      bool p;

      WRATHLockMutex(m_set_get_data_mutex);
      p=m_data[i].m_is_waiting;
      WRATHUnlockMutex(m_set_get_data_mutex);
      
      return p;
    }
    
    void
    wait_a_little(void)
    {
      struct timespec alittle_time;

      //wait for 1ms
      alittle_time.tv_sec=0;
      alittle_time.tv_nsec=1000;
      nanosleep(&alittle_time, NULL);
    }

    LockableFace::handle m_ttf_face;
    uint64_t m_total_time_to_generate;
    int m_number_glyphs_generated;

    WRATHMutex m_set_get_data_mutex;

    std::map<character_code_type, glyph_index_type> m_glyph;
    std::map<glyph_index_type, character_code_type> m_ascii;
    std::vector<data_type> m_data;
    bool m_supports_kerning;
  };
  
  /*!\fn LockableFace::handle load_face(const WRATHFontDatabase::Font::const_handle &)
    Load a FT_Face from a file given a filename
    and a face_index returning a handle to a
    LockableFace. The FT_Face of the returned 
    handle has it's private FT_Library, as such
    can be used in parallel safely with other
    handles returned by load_face(). 

    \param fnt handle to WRATHFontDatabase::Font from which to source font data,
               specifies from where to source the Font (file, memory, etc)
               and what face index to use
   */
  LockableFace::handle
  load_face(const WRATHFontDatabase::Font::const_handle &fnt);

  /*!\fn LockableFace::handle load_face(const std::string &, int)
    Provided as a conveniance, equivalent to
    \code
    load_face(WRATHFontDatabase::fetch_font_entry(filename, face_index))
    \endcode
   */
  inline
  LockableFace::handle
  load_face(const std::string &filename, int face_index=0)
  {
    return load_face(WRATHFontDatabase::fetch_font_entry(filename, face_index));
  }

  /*!\fn WRATHTextureFont* fetch_font(int, const WRATHFontDatabase::Font::const_handle&)
    Checks if a font of type F from the specified WRATHFontDatabase::Font
    and pixel size has been created, if so returns it, otherwise
    creates a new font of type F of those parameters and returns it.
    The font's resource name will be the tuple comprising of
    - hnd
    - ppixel_height
    - typeid(F).name()

    The type F must have a constructor of the form:
    \code
    F(WRATHFreeTypeSupport::LockableFace::handle pface,
      const WRATHTextureFontKey &presource_name)
    \endcode
    
    \param psize pixel size to use
    \param fnt handle to WRATHFontDatabase::Font from which to source font data
  */
  template<typename F>
  static
  WRATHTextureFont*
  fetch_font(int psize, const WRATHFontDatabase::Font::const_handle &fnt)
  {
    //first try to fetch the font:
    F *return_value;
    WRATHTextureFont *p;
    WRATHTextureFontKey K;
    
    K=WRATHTextureFontKey(fnt, psize, typeid(F).name());
    p=WRATHTextureFont::retrieve_resource(K);
    return_value=dynamic_cast<F*>(p);
    
    if(return_value==NULL)
      {
        WRATHFreeTypeSupport::LockableFace::handle pface;
        
        pface=WRATHFreeTypeSupport::load_face(fnt);        
        if(pface.valid())
          {
            return_value=WRATHNew F(pface, K);
          }
        else
          {
            return_value=NULL;
          }
      }
    
    return return_value;
  }

  /*!\fn WRATHTextureFont* fetch_font(int, const std::string &, int)
    Provided as a conveniance, equivalent to
    \code
    fetch_font<F>(psize, WRATHFontDatabase::fetch_font_entry(pfilename, pface_index);
    \endcode
    \param psize pixel size to use
    \param pfilename filename from which to read font
    \param pface_index face index into file if file has multiple fonts within it.
  */
  template<typename F>
  static
  WRATHTextureFont*
  fetch_font(int psize, const std::string &pfilename, int pface_index=0)
  {
    WRATHFontDatabase::Font::const_handle fnt;

    fnt=WRATHFontDatabase::fetch_font_entry(pfilename, pface_index);
    return fetch_font<F>(psize, fnt);
  }
  
};

/*!\def WRATHFreeType_STREAM
  Prints to the debug stream of a WRATHFreeTypeSupport::geometry_data if
  a stream is attached to it.
  \param X is a WRATHFreeTypeSupport::geometry_data
  \param Y data to stream to debug stream of X                                          
 */
#ifdef WRATHDEBUG
#define WRATHFreeType_STREAM(X, Y) do {\
    if(X.debug_stream_valid()) { X.debug_stream() << Y; } } while(0)

#else

#define WRATHFreeType_STREAM(X, Y) do {} while(0)
#endif

/*! @} */

#endif
