// -*- C++ -*-

/*! 
 * \file WRATHShapeDistanceFieldImplement.tcc
 * \brief file WRATHShapeDistanceFieldImplement.tcc
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


#if !defined(__WRATH_SHAPE_DISTANCE_FIELD_HPP__) || defined(__WRATH_SHAPE_DISTANCE_FIELD_IMPL_TCC__)
#error "Direction inclusion of private header file WRATHShapeDistanceFieldImplement.tcc" 
#endif

#include "WRATHConfig.hpp"

#define __WRATH_SHAPE_DISTANCE_FIELD_IMPL_TCC__
namespace WRATHShapeDistanceFieldImplement
{
  inline
  enum WRATHShapeDistanceField::texel_location_type
  loc_type(bool at_texel_center)
  {
    return at_texel_center?
      texel_center:
      texel_begin;  
  }

  template<typename T>
  class edge_interpolator:public interpolator_base
  {
  public:
    edge_interpolator(const typename WRATHShapeDistanceField::coordinate_coverter<T>::handle &h,
                      const typename WRATHOutline<T>::Interpolator *obj):      
      m_h(h)
    {
      vecN<T,2> st(obj->position()), ed(obj->to_position());

      m_curve.x()[0]=st.x();
      m_curve.y()[0]=st.y();
      
      m_curve.x()[1]=ed.x() - st.x();
      m_curve.y()[1]=ed.y() - st.y();


      m_bitmap_bbox.set_or( h.(st) - ivec2(1,1) );
      m_bitmap_bbox.set_or( h.(ed) + ivec2(1,1) );
    }

    ~edge_interpolator()
    {}

    void
    compute_line_intersection(int in_bitmap_pt,
                              enum WRATHUtil::coordinate_type tp,
                              std::vector<solution_point> &append_to,
                              bool at_texel_center)
    {
      std::vector<polynomial_solution_solve> sols;
      int fixed_coord, varying_coord;

      fixed_coord=WRATHUtil::fixed_coordinate(tp);
      varying_coord=WRATHUtil::varying_coordinate(tp);

      vecN<T,2> poly(m_curve[fixed_coord]);

      poly[0]-=m_h.shape_coordinate_from_bitmap_coordinate(in_bitmap_pt, 
                                                           fixed_coord,
                                                           loc_type(at_texel_center));

      solve_linear(poly, sols);

      for(std::vector<polynomial_solution_solve>::iterator
            iter=sols.begin(), end=sols.end(); iter!=end; ++iter)
        {
          solution_point p;

          p.m_multiplicity=iter->m_multiplicity;
          evaluate(iter->m_t, p.m_value, p.m_derivative);
        }
    }

  private:
    /*
      m_curve.x() = parameterization of x-coordinate in shape coordinates
      m_curve.y() = parameterization of y-coordinate in shape coordinates
     */
    vecN<vecN<T,2>, 2> m_curve;
    typename WRATHShapeDistanceField::coordinate_coverter<T>::handle m_h;
  };


  template<typename T>
  class bezier_interpolator:public interpolator_base
  {
  public:
    bezier_interpolator(const typename WRATHShapeDistanceField::coordinate_coverter<T>::handle &h,
                        const typename WRATHOutline<T>::BezierInterpolator *obj):
      m_h(h)
    {

      m_evaluator.resize(obj->m_control_points.size()+2);

      m_evaluator.front()=obj->position();
      std::copy(obj->m_control_points.begin(),
                obj->m_control_points.end(),
                &m_evaluator[1]);
      m_evaluator.back()=obj->to_position();
      m_deriv_evaluator=m_evaluator.compute_derivative();

      WRATHUtil::generate_polynomial_from_bezier(m_evaluator, m_curve);
    }
      

    ~bezier_interpolator()
    {}

    void
    compute_line_intersection(int in_bitmap_pt,
                              enum WRATHUtil::coordinate_type tp,
                              std::vector<solution_point> &append_to)
    {
      vecN<T, 4> work_array;
      int fixed_coord, varying_coord;
      std::vector<polynomial_solution_solve> sols;
      
      fixed_coord=WRATHUtil::fixed_coordinate(tp);
      varying_coord=WRATHUtil::varying_coordinate(tp);
      std::copy(m_curve[fixed_coord].begin(), m_curve[fixed_coord].end(), work_array.begin());
      work_array[0]-=m_h.shape_coordinate_from_bitmap_coordinate(in_bitmap_pt, 
                                                                 WRATHUtil::fixed_coordinate(tp),
                                                                 loc_type(at_texel_center));

      c_array<T> poly( &work_array[0], m_curve[fixed_coord].size());
      solve_polynomial(poly, sols);

      

      for(std::vector<polynomial_solution_solve>::iterator
            iter=sols.begin(), end=sols.end(); iter!=end; ++iter)
        {
          solution_point p;

          p.m_multiplicity=iter->m_multiplicity;
          evaluate(iter->m_t, p.m_value, p.m_derivative);
        }
      
    }
  private:
    /*
      m_curve.x() = parameterization of x-coordinate in shape coordinates
      m_curve.y() = parameterization of y-coordinate in shape coordinates
     */
    vecN< std::vector<T>, 2> m_curve;
    WRATHUtil::BernsteinPolynomial m_evaluator, m_deriv_evaluator;
    typename WRATHShapeDistanceField::coordinate_coverter<T>::handle m_h;
  };

  template<typename T>
  class arc_interpolator:public interpolator_base
  {
  public:
    arc_interpolator(const typename WRATHShapeDistanceField::coordinate_coverter<T>::handle &h,
                     const typename WRATHOutline<T>::ArcInterpolator *obj);
    ~arc_interpolator();

    void
    compute_line_intersection(int in_bitmap_pt,
                              enum WRATHUtil::coordinate_type tp,
                              std::vector<solution_point> &append_to);

  private:
    
  };


  template<typename T>
  interpolator_base*
  construct_interpolator(const typename WRATHOutline<T>::BaseInterpolator *ptr,
                         const typename WRATHShapeDistanceField::coordinate_coverter<T>::handle &h)
  {
    typedef typename WRATHOutline<T>::ArcInterpolator arc_t;
    typedef typename WRATHOutline<T>::BezierInterpolator bez_t;

    arc_t *arc_ptr;
    bez_t *bez_ptr;

    bez_ptr=dynamic_cast<const bez_t*>(ptr);
    if(bez_ptr!=NULL)
      {
        return WRATHNew bezier_interpolator<T>(h, bez_ptr);
      }

    arc_ptr=dynamic_cast<const arc_t*>(ptr);
    if(arc_ptr!=NULL)
      {
        return WRATHNew arc_interpolator<T>(h, arc_ptr);
      }

    return WRATHNew edge_interpolator(h, ptr);
  }

}


////////////////////////////////////
// WRATHShapeDistanceField methods
template<typename T>
WRATHShapeDistanceField::
WRATHShapeDistanceField(const WRATHShape<T> &pshape,
                        const typename coordinate_coverter<T>::handle &h,
                        ivec2 distance_field_size,
                        float max_distance):
  
  m_max_distance(max_distance),
  m_values(boost::extents[distance_field_size.x()][distance_field_size.y()])
{
  for(int i=0, endi=pshape.number_outlines(); i!=endi; ++i)
    {
      const WRATHOutline<T> *outline;

      outline=pshape.outline(i);
      for(typename std::vector<typename WRATHOutline<T>::point>::const_iterator
            iter=outline->points().begin(), end=outline->points().end();
          iter!=end; ++iter)
        {
          WRATHShapeDistanceFieldImplement::interpolator_base *ptr;

          ptr=WRATHShapeDistanceFieldImplement::construct_interpolator<T>(iter->interpolator(), h);
          m_curves.push_back(ptr);
        }
    }

  compute_distance_field();
}
