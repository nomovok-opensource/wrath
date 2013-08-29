/*! 
 * \file WRATHOutline.hpp
 * \brief file WRATHOutline.hpp
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




#ifndef __WRATH_OUTLINE_HPP__
#define __WRATH_OUTLINE_HPP__

#include "WRATHConfig.hpp"
#include <vector>
#include <boost/utility.hpp>
#include "vecN.hpp"
#include "vectorGL.hpp"
#include "c_array.hpp"
#include "WRATHStateStream.hpp"
#include "WRATHStateStreamManipulators.hpp"

/*! \addtogroup Shape
 * @{
 */


/*!\class WRATHOutline
  A WRATHOutline represents a collection
  of points to indicate a closed outline.
  Points are added in the order they appear.
  The end points should not be repeated in 
  the specification. For example the boundary
  of a triangle consisting of the points
  A, B and C is specified via:

  \code
  WRATHOutline<T> triangle;
  vecN<T,2> A, B, C;
  triangle << A << B << C;
  \endcode

  In addition, each point can have an
  interpolator object \ref WRATHOutline::Interpolator
  attached to it to control the interpretation
  of interpolating between the point
  the WRATHOutline<T>::Interpolator is 
  attached and the next point. 
 */
template<typename T>
class WRATHOutline:boost::noncopyable
{
public:

  /*!\typedef position_type
    Conveniance typedef.
   */
  typedef vecN<T,2> position_type;

  /*!\class Interpolator
    A Interpolator is used to
    dictate the positional interpolation
    between one point of a WRATHOutline
    to the next. For example, it can be used
    to hold control points to indicate a
    Bezier Curve.
   */
  class Interpolator:boost::noncopyable
  {
  public:
    /*!\fn Interpolator
      Empty ctor.
     */
    Interpolator(void):
      m_owner(NULL)
    {}

    virtual
    ~Interpolator()
    {}

    /*!\fn WRATHOutline* outline
      Once this Interpolator is placed
      within a \ref point that is in turn
      added to a WRATHOutline, returns
      that WRATHOutline.
     */
    WRATHOutline*
    outline(void) const
    {
      return m_owner;
    }

    /*!\fn unsigned int point_index
      Once this Interpolator is placed
      within a \ref point that is in turn
      added to a WRATHOutline, returns
      the location within \ref points()
      of the point that holds this
      Interpolator. Will assert
      if \ref outline() returns NULL.
     */
    unsigned int
    point_index(void) const
    {
      WRATHassert(m_owner!=NULL);
      return m_point_index;
    }

    /*!\fn unsigned int to_point_index
      Once this Interpolator is placed
      within a \ref point that is in turn
      added to a WRATHOutline, returns
      the location of the point within 
      \ref points() of the point that this
      Interpolator is to interpolate 
      to. Will assert if \ref outline() returns 
      NULL. Note that this value is the first
      point of the WRATHOutline if the point
      of this WRATHInterpolator is the last
      point of the WRATHOutline. Additionally,
      in that case the return value of 
      to_point_index() changes if a point is
      added to the WRATHOutline.
     */
    unsigned int
    to_point_index(void) const
    {
      unsigned int R(m_point_index+1);
      WRATHassert(m_owner!=NULL);
      return (R==m_owner->m_points.size())?
        0:R;
    }

    /*!\fn const position_type& position
      Provided as a conveniance, returns the position
      of the point that has this Interpolator.
      Will assert if \ref outline() returns NULL.
      Equivalent to:
      \code
       outline()->points()[point_index()]
      \endcode
     */
    const position_type&
    position(void) const
    {
      WRATHassert(m_owner!=NULL);
      WRATHassert(m_owner->m_points[m_point_index].m_interpolator==this);
      return m_owner->m_points[m_point_index].m_position;
    }

    /*!\fn const position_type& to_position
      Provided as a conveniance, returns the position
      of the point that has this Interpolator
      is interpolating to.  Will assert if \ref outline() 
      returns NULL. Equivalent to:
      \code
       outline()->points()[to_point_index()]
      \endcode
     */
    const position_type&
    to_position(void) const
    {
      WRATHassert(m_owner!=NULL);
      WRATHassert(m_owner->m_points[m_point_index].m_interpolator==this);
      return m_owner->m_points[to_point_index()].m_position;
    }

  private:
    friend class WRATHOutline;

    void
    register_interpolator(WRATHOutline *own, unsigned int I)
    {
      WRATHassert(m_owner==NULL);
      m_owner=own;
      m_point_index=I;
    }

    WRATHOutline *m_owner;
    unsigned int m_point_index;
  };

  /*!\class BezierInterpolator
    Derived class of Interpolator to indicate a Bezier curve.
    Supports Bezier curves of _any_ degree. If
    size of \ref m_control_points is N (i.e. N
    control points) then degree of Bezier curve is N+1.
   */
  class BezierInterpolator:public Interpolator
  {
  public:
    /*!\fn BezierInterpolator(void)
      Ctor. No control points, thus interpolation
      is same as NULL Interpolator.
     */
    BezierInterpolator(void)
    {}

    /*!\fn BezierInterpolator(const position_type&)
      Ctor. One control points, thus interpolation
      is a quadratic curve.
      \param ct control point
     */
    BezierInterpolator(const position_type &ct):
      m_control_points(1, ct)
    {}

    /*!\fn BezierInterpolator(const position_type&, const position_type&)
      Ctor. Two control points, thus interpolation
      is a cubic curve.
      \param ct1 1st control point
      \param ct2 2nd control point
     */
    BezierInterpolator(const position_type &ct1, 
                       const position_type &ct2)
    {
      m_control_points.push_back(ct1);
      m_control_points.push_back(ct2);
    }

    /*!\var m_control_points
      Control points of Bezier curve, made
      public.
     */
    std::vector<position_type> m_control_points;
  };

  /*!\class ArcInterpolator
    An ArcInterpolator is for connecting one point
    to the next via an arc of a circle.
   */
  class ArcInterpolator:public Interpolator
  {
  public:

    /*!\fn ArcInterpolator
      Ctor.
      \param pangle The angle of the arc in radians,
                    the value must be in the range
                    0<=angle<2*M_PI. Values outside
                    of this range have undefined results.
      \param pcounter_clockwise specifies the direction 
                                of arc. If true the the arc
                                is drawn counterclockwise.
                                _ASSUMING_ that the coordinate 
                                system is that y-increases 
                                upwards and x-increases to the right.
     */
    explicit
    ArcInterpolator(float pangle,
                    bool pcounter_clockwise=false):
      m_angle(pangle),
      m_counter_clockwise(pcounter_clockwise)
    {}

    /*!\var m_angle
      Defines the angle of the arc
     */
    float m_angle;

    /*!\var m_counter_clockwise
      Defines if the arc is going clockwise
      or counter clockwise _ASSUMING_
      that the coordinate system is
      that y-increases upwards and
      x-increases to the right.
     */
    bool m_counter_clockwise;
  };

  /*!\class GenericInterpolator
    A GenericInterpolator represents a generic
    interface for computing interpolation
    between two edge points of a WRATHOutline.
   */
  class GenericInterpolator:public Interpolator
  {
  public:
    
    /*!\fn void compute
      To be implemented by a derived class to compute
      datum of the curve at a time t.
      \param in_t (input) argument fed to parameterization of which
                  this GenericInterpolator represents with
                  in_t=0.0 indicating the start of the curve
                  and out_t=1.0 the end of the curve
      \param outp (output) reference to place position of the curve
      \param outp_t (output) reference to place the first derivative
      \param outp_tt (output) reference to place the second derivative
     */
    virtual
    void
    compute(float in_t,
            vec2 &outp,
            vec2 &outp_t,
            vec2 &outp_tt) const=0;
  };

  /*!\class point
    Each point of a WRATHOutline has a
    position and an interpolator
    dictating how to interpolate from
    the point to the next point. 
   */
  class point
  {
  public:
    /*!\fn point(const position_type&, Interpolator*)
      Ctor. 
      \param pt the position of the point of the outline
      \param i interpolator object used to interpolate
               position from this point to the next. Passing
               NULL indicates that the interpolation is a
               line segment. Note that the Interpolator 
               becomes _owned_ by the WRATHOutline once the 
               point_type is added to the WRATHOutline.
     */
    point(const position_type &pt,
          Interpolator *i=NULL):
      m_position(pt),
      m_interpolator(i)
    {}

    /*!\fn const position_type& position
      Returns the position of the point of the outline
     */
    const position_type&
    position(void) const
    {
      return m_position;
    }

    /*!\fn const Interpolator* interpolator
      Object used to perform paramerterized
      interpolation between this point and the
      next point of the outline.
     */  
    const Interpolator*
    interpolator(void) const
    {
      return m_interpolator;
    }

  private:
    friend class WRATHOutline;

    position_type m_position;
    Interpolator *m_interpolator;
  };

  /*!\class control_point
    Conveniance class to use with an overloaded
    operator<< to add control points between
    points.
   */
  class control_point
  {
  public:
    /*!\fn control_point(const position_type&)
      Ctor. 
      \param p position to which to set the control point
     */
    explicit
    control_point(const position_type &p):
      m_value(p)
    {}

    /*!\fn control_point(T, T)
      Ctor. 
      \param px x-position to which to set the control point
      \param py y-position to which to set the control point
     */
    explicit
    control_point(T px, T py):
      m_value(px, py)
    {}

    /*!\var m_value
      Position of control point
     */
    position_type m_value;
  };


  /*!\fn WRATHOutline
    Ctor. 
    \param pID typically outlines are part of a 
               WRATHShape<T>, which has multiple
               outlines. The pID field is used
               so that the WRATHOutline<T> can know
               "where" it lives within a WRATHShape<T>.
               The return value of \ref ID() is
               the value pID passed.
   */
  explicit
  WRATHOutline(unsigned int pID):
    m_ID(pID)
  {}

  virtual
  ~WRATHOutline()
  {
    clear();
  }

  /*!\fn unsigned int ID
    Returns the value passed at ctor for pID.
   */
  unsigned int
  ID(void) const
  {
    return m_ID;
  }
  
  /*!\fn const WRATHStateStream& state_stream
    Returns a const reference to the WRATHStateStream
    that holds all the state changes.
   */
  const WRATHStateStream&
  state_stream(void) const 
  {
    return m_state_stream;
  }

  /*!\fn void clear
    Clears the WRATHOutline, i.e
    set to no points and no state.
   */
  void
  clear(void)
  {
    for(typename std::vector<point>::const_iterator
          iter=m_points.begin(), end=m_points.end(); iter!=end; ++iter)
      {
        if(iter->m_interpolator!=NULL)
          {
            WRATHDelete(iter->m_interpolator);
          }
      }

    m_state_stream.reset();
    m_points.clear();
    on_change();
  }

  /*!\fn const std::vector<point>& points
    Returns the points of this WRATHOutline.
   */
  const std::vector<point>&
  points(void) const
  {
    return m_points;
  }

  /*!\fn const point& pt
    Returns the named point of this WRATHOutline.
   */
  const point&
  pt(int i) const
  {
    return m_points[i];
  }
  
  /*!\fn unsigned int add_point(const position_type&, Interpolator*)
    Add a point to this WRATHOutline, returns the index
    of the point added.

    \param pt position of the point
    \param ptr interpolator to use to the next point.
               The WRATHOutline object will own the 
               Interpolator object. If passed NULL
               for the interpolator will use a 
               newly created \ref BezierInterpolator object 
               instead of NULL for the \ref Interpolator of
               the added point (this newly created 
               \ref Interpolator is also owned by the
               WRATHOutline).
   */
  unsigned int
  add_point(const position_type &pt, Interpolator *ptr)
  {
    if(ptr==NULL)
      {
        ptr=WRATHNew BezierInterpolator();
      }

    ptr->register_interpolator(this, m_points.size());
    m_points.push_back(point(pt,ptr));

    m_state_stream.increment_time_to_value(m_points.size());
    on_change();

    return m_points.size()-1;
  }

  /*!\fn unsigned int add_point(point)
    Provided as a conveniance, equivalent to
    \code
    add_point(P.position(), P.interpolator());
    \endcode
    \param P point to add to WRATHOutline
   */
  unsigned int
  add_point(point P)
  {
    return add_point(P.position(), P.interpolator());
  } 

  /*!\fn enum return_code add_control_point
    If the last point added uses a \ref BezierInterpolator
    for it's interpolator, then adds a control point
    to that \ref BezierInterpolator and returns routine_sucess.
    If no points have been added or that last point
    added does not use a \ref BezierInterpolator, returns
    routine_fail.
   */
  enum return_code
  add_control_point(const position_type &pt)
  {
    BezierInterpolator *ptr;

    ptr=(m_points.empty())?
      NULL:
      dynamic_cast<BezierInterpolator*>(m_points.back().m_interpolator);

    if(ptr!=NULL)
      {
        ptr->m_control_points.push_back(pt);
        on_change();
      }

    return (ptr!=NULL)?
      routine_success:
      routine_fail;
  }

  /*!\fn enum return_code to_arc
    If the last point added uses a BezierInterpolator
    with no control points or uses a ArcInterpolator
    for it's interpolator, then sets the interpolator
    to be an ArcInterpolator of the specified angle
    and orientation and returns routine_success,
    otherwise returns routine_fail.
    \param angle angle in radians of arc-to
    \param is_ccw true if and only if arc is oriented
                  counter clockwise.
   */
  enum return_code
  to_arc(float angle, bool is_ccw=false)
  {
    BezierInterpolator *ptr;
    ArcInterpolator *aptr;

    if(m_points.empty())
      {
        return routine_fail;
      }

    ptr=dynamic_cast<BezierInterpolator*>(m_points.back().m_interpolator);
    aptr=dynamic_cast<ArcInterpolator*>(m_points.back().m_interpolator);

    if(ptr!=NULL and !ptr->m_control_points.empty())
      {
        return routine_fail;
      }

    if(ptr!=NULL)
      {
        WRATHDelete(m_points.back().m_interpolator);
        m_points.back().m_interpolator=WRATHNew ArcInterpolator(angle, is_ccw);
        m_points.back().m_interpolator->register_interpolator(this, m_points.size() - 1);
        on_change();
        return routine_success;
      }

    if(aptr!=NULL)
      {
        aptr->m_angle=angle;
        aptr->m_counter_clockwise=is_ccw;
        on_change();
        return routine_success;
      }

    return routine_fail;
  }


  /*!\fn WRATHOutline& operator<<(WRATHOutline&, point)
    overload operator<< to add a point to a WRATHOutline
    object. Equivalent to
    \code
    stream.add_point(P.position(), P.interpolator());
    \endcode
    If the value of P.interpolator is NULL, then a
    new BezierInterpolator is used instead.

    \param stream WRATHOutline to which to add a point
    \param P point to add to stream
   */
  friend
  WRATHOutline&
  operator<<(WRATHOutline &stream, point P)
  {
    stream.add_point(P.position(), P.interpolator());
    return stream;
  }

  /*!\fn WRATHOutline& operator<<(WRATHOutline&, const position_type&)
    overload operator<< to add a point to a WRATHOutline
    object. Note that points add this way will
    implicity use a BezierInterpolator for interpolation.
    (Though the BezierInterpolator will be empty initially). 
    Equivalent to
    \code
    stream.add_point(P, NULL);
    \endcode

    \param stream WRATHOutline to which to add a point
    \param P position of point to add to stream
   */
  friend
  WRATHOutline&
  operator<<(WRATHOutline &stream, const position_type &P)
  {
    stream.add_point(P, NULL);
    return stream;
  }

  /*!\fn WRATHOutline& operator<<(WRATHOutline&, const control_point&)
    overload operator<< to add a control point, note
    that has no effect if last point added does not
    use a BezierInterpolotor or if no points have
    yet been added. Equivalent to
    \code
    stream.add_control_point(P.m_value);
    \endcode

    \param stream WRATHOutline to which to add a point
    \param P position of control point to add to stream
   */
  friend
  WRATHOutline&
  operator<<(WRATHOutline &stream, const control_point &P)
  {
    stream.add_control_point(P.m_value);
    return stream;
  }


  /*!\fn WRATHOutline& operator<<(WRATHOutline&, 
                                  const WRATHStateStreamManipulators::set_state_type<S>&)
    overload operator<< to set state of a WRATHOutline object.
    \param stream WRATHOutline to which to manipulate
    \param obj object that specifies manipulation of stream
   */
  template<typename S>
  friend
  WRATHOutline&
  operator<<(WRATHOutline &stream, 
             const WRATHStateStreamManipulators::set_state_type<S> &obj)
  {
    stream.m_state_stream << obj;
    return stream;
  }

  /*!\fn WRATHOutline& operator<<(WRATHOutline&, const WRATHStateStreamManipulators::get_state_type<S>&)
    overload operator<< to get state of a WRATHOutline object.
    \param stream WRATHOutline to which to manipulate
    \param obj object that specifies manipulation of stream
   */
  template<typename S>
  friend
  WRATHOutline&
  operator<<(WRATHOutline &stream, 
             const WRATHStateStreamManipulators::get_state_type<S> &obj)
  {
    stream.m_state_stream << obj;
    return stream;
  }

  /*!\fn WRATHOutline& operator<<(WRATHOutline&, const WRATHStateStreamManipulators::push_state_type<S>&) 
    overload operator<< to push state of a WRATHOutline object.
    \param stream WRATHOutline to which to manipulate
    \param obj object that specifies manipulation of stream
   */
  template<typename S>
  friend
  WRATHOutline&
  operator<<(WRATHOutline &stream, 
             const WRATHStateStreamManipulators::push_state_type<S> &obj)
  {
    stream.m_state_stream << obj;
    return stream;
  }

  /*!\fn WRATHOutline& operator<<(WRATHOutline&, const WRATHStateStreamManipulators::pop_state_type<S>&)
    overload operator<< to pop state of a WRATHOutline object.
    \param stream WRATHOutline to which to manipulate
    \param obj object that specifies manipulation of stream
   */
  template<typename S>
  friend
  WRATHOutline&
  operator<<(WRATHOutline &stream, 
             const WRATHStateStreamManipulators::pop_state_type<S> &obj)
  {
    stream.m_state_stream << obj;
    return stream;
  }

  /*!\fn WRATHOutline& operator<<(WRATHOutline&, WRATHStateStreamManipulators::get_stream_size_type)
    overload operator<< to query the size of a WRATHOutline object.
    \param stream WRATHOutline to which to manipulate
    \param G object that holds a reference to place the results of the query
   */
  friend
  WRATHOutline&
  operator<<(WRATHOutline &stream, 
             WRATHStateStreamManipulators::get_stream_size_type G)
  {
    WRATHassert(G.m_target!=NULL);
    *G.m_target=stream.m_points.size();
    return stream;
  }

protected:

  /*!\fn on_change
    To be optionally implemented by a derived class.
    Called whenever the contents of WRATHOutline
    object change. Default is to do nothing.
   */
  virtual
  void
  on_change(void)
  {}

private:


  unsigned int m_ID;
  std::vector<point> m_points;
  WRATHStateStream m_state_stream;
};

/*!\typedef WRATHOutlineI
  Conveniance typedef for WRATHOutline\<int32_t\>
 */
typedef WRATHOutline<int32_t> WRATHOutlineI;

/*!\typedef WRATHOutlineF
  Conveniance typedef for WRATHOutline\<float\>
 */
typedef WRATHOutline<float> WRATHOutlineF;

/*! @} */
#endif
