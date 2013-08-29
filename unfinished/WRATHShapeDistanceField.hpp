/*! 
 * \file WRATHShapeDistanceField.hpp
 * \brief file WRATHShapeDistanceField.hpp
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


#ifndef __WRATH_SHAPE_DISTANCE_FIELD_HPP__
#define __WRATH_SHAPE_DISTANCE_FIELD_HPP__


#include "WRATHConfig.hpp"
#include <boost/multi_array.hpp>
#include "WRATHReferenceCountedObject.hpp"
#include "WRATHShape.hpp"

#include "WRATHShapeDistanceFieldImplementTypes.hpp"


/*! \addtogroup Shape
 * @{
 */

/*!\class WRATHShapeDistanceField
  A WRATHShapeDistanceField creates a distance
  field from a WRATHShape. A WRATHShapeDistanceField
  _only_ supports the following interpolator types:
  - \ref WRATHOutline<T>::BezierInterpolator consisting of no more than 4 control points, i.e. no worse than cubic
  - \ref WRATHOutline<T>::ArcInterpolator
  All other interpolators will be interpreted as line segment interpolators,
  you have been warned.

  TODO: Not implemented YET!!.
*/
class WRATHShapeDistanceField
{
public:

  /*!\enum texel_location_type
    When converting to and from bit map coordinates,
    this enumneration is used to specify from where
    within the texel.
   */
  enum texel_location_type
    {
      /*
        Indicates beginning of texel
       */
      texel_begin,

      /*!
        Indicates center of texel
       */
      texel_center,
    };

  /*!\class coordinate_coverter
    A coordinate_coverter converts point coordinates
    coming from a \ref WRATHShape to coordiantes
    on an image and vica-versa.
    \tparam T value type used in the WRATHShape<T>
   */
  template<typename T>
  class coordinate_coverter:
    public WRATHReferenceCountedObjectT<coordinate_coverter>
  {
  public:

    virtual
    ~coordinate_coverter()
    {}

    /*!\fn T shape_coordinate_from_bitmap_coordinate(int, int, enum texel_location_type) const
      To be implemented by a derived class to convert
      an x (or y) coordiante on an image into a coordinate
      of the WRATHShape
      \param pt value to convert
      \param coord if 0 pt is an x-coordinate, if 1 pt is a y-coordinates
      \param t texel type to indicate where within a texel
     */
    virtual
    T
    shape_coordinate_from_bitmap_coordinate(int pt, int coord,
                                            enum texel_location_type t) const=0;

    /*!\fn vecN<T,2> shape_coordinate_from_bitmap_coordinate(ivec2, enum texel_location_type) const
      Converts an image coordinate into a WRATHShape coordiante
      \param pt image coordiante
      \param t texel type to indicate where within a texel
     */
    virtual
    vecN<T,2>
    shape_coordinate_from_bitmap_coordinate(ivec2 pt,
                                            enum texel_location_type t) const
    {
      vecN<T,2> R;

      R[0]=shape_coordinate_from_bitmap_coordinate(pt[0], 0, t);
      R[1]=shape_coordinate_from_bitmap_coordinate(pt[1], 1, t);
      return R;
    }

    /*!\fn int bitmap_coordinate_from_shape_coordinate(T, int, enum texel_location_type) const
      To be implemented by a derived class to convert
      an x (or y) coordiante to an image from a coordinate of the WRATHShape
      \param pt value to convert
      \param coord if 0 pt is an x-coordinate, if 1 pt is a y-coordinates
      \param t texel type to indicate where within a texel
     */
    virtual
    int
    bitmap_coordinate_from_shape_coordinate(T pt, int coord,
                                            enum texel_location_type t) const=0;

    /*!\fn ivec2 bitmap_coordinate_from_shape_coordinate(vecN<T,2>, enum texel_location_type) const
      Converts to an image coordinate from a WRATHShape coordiante
      \param pt WRATHShape coordiante
      \param t texel type to indicate where within a texel
     */
    virtual
    ivec2
    bitmap_coordinate_from_shape_coordinate(vecN<T,2> pt,
                                            enum texel_location_type t) const 
    {
      ivec2 R;

      R[0]=bitmap_coordinate_from_shape_coordinate(pt[0], 0, t);
      R[1]=bitmap_coordinate_from_shape_coordinate(pt[1], 1, t);
      return R;
    }

    virtual
    int
    bitmap_radius_from_shape_radius(float r) const=0;
  };

  enum 
    {
      intersections_left=0,
      intersections_right,
      intersections_up,
      intersections_down      
    };

  class distance_field_value
  {
  public:
    distance_field_value(void):
      m_distance(1024.0f),
      m_winding_number(0),
      m_intersection_counts(0, 0, 0, 0)
    {}

    float m_distance;
    int m_winding_number;
    vecN<int, 4> m_intersection_counts;
  };


  template<typename T>
  WRATHShapeDistanceField(const WRATHShape<T> &pshape,
                          const typename coordinate_coverter<T>::handle &h,
                          ivec2 distance_field_size,
                          float max_distance);

  ~WRATHShapeDistanceField();


  const boost::multi_array<distance_field_value>&
  distance_field(void) const;

private:
  
  void
  compute_distance_field(void);

  float m_max_distance;
  std::vector<WRATHShapeDistanceFieldImplement::interpolator_base*> m_curves;
  boost::multi_array<distance_field_value> m_values;
};


/*! @} */

#include "WRATHShapeDistanceFieldImplement.hpp"

#endif
