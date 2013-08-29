/*! 
 * \file WRATHBBox.hpp
 * \brief file WRATHBBox.hpp
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




#ifndef __WRATH_BBOX_HPP__
#define __WRATH_BBOX_HPP__

#include "WRATHConfig.hpp"
#include "WRATHBBoxForwardDeclare.hpp"
#include <iostream>
#include "vectorGL.hpp"

/*! \addtogroup Utility
 * @{
 */

/*!\class WRATHBBox
  A WRATHBBox represents a bounding box,
  the number of coordinate and the type
  are both template parameters. The box
  is a _closed_ bounding box, as such
  if the edges of 2 boxes are the same
  they are considered to intersect.
  \tparam N number of coordinates, for example
          N=2 means a rectangle, N=3 means an
          axis aligned box in 3 dimensions, etc.
  \tparam T type of the coordinates, default value
            for parameter is float.
 */
template<unsigned int N, typename T>
class WRATHBBox
{
public:

  /*!\fn WRATHBBox(void)
    Ctor. Initializes the WRATHBBox as empty.
   */
  WRATHBBox(void):
    m_empty(true)
  {}

  /*!\fn WRATHBBox(const vecN<T,N> &, const vecN<T,N>&)
    Ctor. Initializes the WRATHBBox with the
    specified corners.
    \param pmin_corner min-corner of bounding box
    \param pmax_corner max-corner of bounding box
   */
  WRATHBBox(const vecN<T,N> &pmin_corner,
            const vecN<T,N> &pmax_corner):
    m_empty(true)
  {
    set_or(pmin_corner);
    set_or(pmax_corner);
  }

  /*!\fn void clear
    Reset the bounding box as empty.
   */
  void
  clear(void)
  {
    m_empty=true;
  }

  /*!\fn void translate
    Translate the bounding box by a set amount.
    \param amount vector by which to move the bounding box
   */
  void
  translate(const vecN<T,N> &amount)
  { 
    if(!empty())
      {
        m_min+=amount;
        m_max+=amount;
      }
  }

  /*!\fn void scale
    Scales the bounding box by a set amount.
    \param v factor by which to scale the bounding box
   */
  void
  scale(T v)
  {
     if(!empty())
      {
        v=std::abs(v);
        m_min*=v;
        m_max*=v;
      }
  }

  /*!\fn void set_or(const WRATHBBox&)
    Enlarge this bounding box
    to enclose another bounding box.
    \param obj box to enclose.
   */
  void
  set_or(const WRATHBBox &obj)
  {
    if(!obj.empty())
      {
        set_or(obj.min_corner());
        set_or(obj.max_corner());
      }
  }

  /*!\fn void set_or(const vecN<T,N>&)
    Enlarge this bounding box to enclose a point.
    \param pt point to enclose.
   */
  void
  set_or(const vecN<T,N> &pt)
  {
    if(m_empty)
      {
        m_empty=false;
        m_min=pt;
        m_max=pt;
      }
    else
      {
        for(unsigned int I=0;I<N;++I)
          {
            m_min[I]=std::min(m_min[I], pt[I]);
            m_max[I]=std::max(m_max[I], pt[I]);
          }
      }
  }

  /*!\fn bool empty
    Returns true if this WRATHBBox is empty.
   */
  bool
  empty(void) const
  {
    return m_empty;
  }

  /*!\fn bool intersects(const WRATHBBox&) const
    Returns true if this WRATHBBox intersects with another WRATHBBox.
    \param obj WRATHBBox to test against.
   */
  bool
  intersects(const WRATHBBox &obj) const
  {
    bool return_value(!m_empty and !obj.m_empty);
    for(unsigned int I=0;I<N and return_value; ++I)
      {
        return_value= ( std::max(m_min[I], obj.m_min[I])
                        <=
                        std::min(m_max[I], obj.m_max[I]) );
      }
    return return_value;
  }

  /*!\fn bool intersects(const vecN<T,N>&) const
    Returns true if this WRATHBBox intersects with a point.
    \param pt point to test against.
   */
  bool
  intersects(const vecN<T,N> &pt) const
  {
    bool return_value(!m_empty);
    for(unsigned int I=0;I<N and return_value; ++I)
      {
        return_value= (m_min[I]<=pt[I] and pt[I]<=m_max[I]);
      }
    return return_value;
  }

  /*!\fn WRATHBBox intersection(const WRATHBBox &obj) const
    Computes and returns the intersection of this WRATHBBox with another WRATHBBox.
    \param obj WRATHBBox to test against.
   */
  WRATHBBox
  intersection(const WRATHBBox &obj) const
  {
    WRATHBBox return_value;

    return_value.m_empty=m_empty or obj.m_empty;
    for(unsigned int I=0;I<N and !return_value.m_empty; ++I)
      {
        return_value.m_min[I]=std::max(m_min[I], obj.m_min[I] );
        return_value.m_max[I]=std::min(m_max[I], obj.m_max[I] );

        return_value.m_empty=(return_value.m_min[I]>return_value.m_max[I]);
      }
    return return_value;
  }

  /*!\fn const vecN<T,N>& min_corner
    returns the minimum corner of this
    WRATHBBox. If this WRATHBBox is empty,
    then WRATHasserts.
   */
  const vecN<T,N>&
  min_corner(void) const
  {
    WRATHassert(!empty());
    return m_min;
  }

  /*!\fn const vecN<T,N>& max_corner
    returns the maximum corner of this
    WRATHBBox. If this WRATHBBox is empty,
    then WRATHasserts.
   */
  const vecN<T,N>&
  max_corner(void) const
  {
    WRATHassert(!empty());
    return m_max;
  }

  /*!\fn vecN<T,N> length(void) const
    If empty() returns true, returns T(0).
    Otherwise returns max_corner() - min_corner().
   */
  vecN<T,N>
  length(void) const
  {
    return m_empty?
      T(0):
      m_max - m_min;
  }

  /*!\fn T length(unsigned int) const
    Provided as a conveniance, equivalent to
    \code
    length()[I]
    \endcode
   */
  T
  length(unsigned int I) const
  {
    return m_empty?
      T(0):
      m_max[I] - m_min[I];
  }

  /*!\fn std::ostream& operator<<(std::ostream&, const WRATHBBox &)
    print the state of a WRATHBBox to
    an std::ostream.
    \param ostr std::ostream to which to print
    \param bb box to print
   */
  friend
  std::ostream&
  operator<<(std::ostream &ostr, const WRATHBBox &bb)
  {
    if(bb.empty())
      {
        ostr << "empty-box";
      }
    else
      {
        ostr << "{ min=" << bb.min_corner()
             << ", max=" << bb.max_corner()
             << " }";
      }
    return ostr;
  }

private:
  vecN<T,N> m_min, m_max;
  bool m_empty;
};


/*! @} */




#endif
