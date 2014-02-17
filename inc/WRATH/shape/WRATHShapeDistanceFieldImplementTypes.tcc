// -*- C++ -*-

/*! 
 * \file WRATHShapeDistanceFieldImplementTypes.tcc
 * \brief file WRATHShapeDistanceFieldImplementTypes.tcc
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


#if !defined(WRATH_HEADER_SHAPE_DISTANCE_FIELD_HPP_) || defined(WRATH_HEADER_SHAPE_DISTANCE_FIELD_IMPL_TYPES_TCC_)
#error "Direction inclusion of private header file WRATHShapeDistanceFieldImplementTypes.tcc" 
#endif

#include "WRATHConfig.hpp"

#define WRATH_HEADER_SHAPE_DISTANCE_FIELD_IMPL_TYPES_TCC_

namespace WRATHShapeDistanceFieldImplement
{
  class solution_point
  {
  public:
    vec2 m_value;
    vec2 m_derivative;
    int m_multiplicity;
  };

  /*
    interpolator_base entire purpose in life
    is to compute the intersection of a
    WRATHOutline<T>::InterpolatorBase derived
    object against a vertical or horizontal
    line. Notice that the object type is not
    a template type, it defines only an interface.
   */
  class interpolator_base
  {
  public:
    virtual
    ~interpolator_base()
    {}

    const WRATHBBox<2,int>&
    bitmap_bbox(void) const
    {
      return m_bitmap_bbox;
    }

    /*
      Compute the intersection of a horizontal or
      vertical line with the interpolator.
      The value in_bitmap_pt is in BITMAP coordinates.
     */
    virtual
    void
    compute_line_intersection(int in_bitmap_pt,
                              enum WRATHUtil::coordinate_type tp,
                              std::vector<solution_point> &append_to,
                              bool at_texel_center)=0;

    /*
      Returns an array of "additional" L1-distance
      minimizing canidates. Values are in _BITMAP_
      coordinates.
     */
    const std::vector<vec2>&
    additional_points_to_check(void)
    {
      return m_points_to_check;
    }

  protected:
    WRATHBBox<2,int> m_bitmap_bbox;
    std::vector<vec2> m_points_to_check;
  };

  
  
}
