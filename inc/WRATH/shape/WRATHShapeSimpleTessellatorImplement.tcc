// -*- C++ -*-

/*! 
 * \file WRATHShapeSimpleTessellatorImplement.tcc
 * \brief file WRATHShapeSimpleTessellatorImplement.tcc
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


#if !defined(__WRATH_SHAPE_SIMPLE_TESSELLATOR_HPP__) || defined(__WRATH_SHAPE_SIMPLE_TESSELLATOR_IMPLEMENT_TCC__)
#error "Direction inclusion of private header file WRATHShapeSimpleTessellatorImplement.tcc" 
#endif

#include "WRATHConfig.hpp"

#define __WRATH_SHAPE_SIMPLE_TESSELLATOR_IMPLEMENT_TCC__

namespace WRATHShapeSimpleTessellatorPrivateImplement
{

  class analytic_point_data
  {
  public:
    /*
      position, 1st and 2nd derivatives.
      These are needed to compute normal and curvature contributions.
     */
    vec2 m_p, m_p_t, m_p_tt;
    
    analytic_point_data(void)
    {}

    analytic_point_data(float px, float py):
      m_p(px, py),
      m_p_t(0.0f, 0.0f),
      m_p_tt(0.0f, 0.0f)
    {}

    analytic_point_data(const vec2 &p):
      m_p(p),
      m_p_t(0.0f, 0.0f),
      m_p_tt(0.0f, 0.0f)
    {}
  };

  /*
    Base class used to compute point positions
    for tessellation, etc. We use this base
    class so that we avoid excessive amounts
    of implementation in a template header
    file. Think of interpolator_base as a
    wrapper over WRATHOutline<T>::InterpolatorBase
   */
  class interpolator_base
  {
  public:

    virtual
    ~interpolator_base()
    {}

    
    bool
    is_flat(void) const
    {
      return m_is_flat;
    }

    const analytic_point_data&
    start_pt(void) const
    {
      return m_start_pt;
    }

    const analytic_point_data&
    end_pt(void) const
    {
      return m_end_pt;
    }

    virtual
    void
    compute(float t, analytic_point_data &output) const=0;

  protected:
    
    template<typename T>
    vec2
    cast_to_float(const vecN<T,2> &inv)
    {
      return vec2(static_cast<float>(inv.x()),
                  static_cast<float>(inv.y()));
    }

    bool m_is_flat;
    analytic_point_data m_start_pt, m_end_pt;
  };

  
  class edge_interpolator:public interpolator_base
  {
  public:
    template<typename T>
    edge_interpolator(type_tag<T>,
                      const typename WRATHOutline<T>::point &input)
    {
      typename WRATHOutline<T>::position_type st, e;

      m_is_flat=true;
      st=input.interpolator()->position();
      e=input.interpolator()->to_position();

      m_start_pt=analytic_point_data(cast_to_float(st));
      m_end_pt=analytic_point_data(cast_to_float(e));

      m_delta=m_end_pt.m_p - m_start_pt.m_p;
      m_start_pt.m_p_t=m_delta;
      m_end_pt.m_p_t=m_delta;
    }

    virtual
    void
    compute(float t, analytic_point_data &output) const
    {
      output.m_p=m_start_pt.m_p + t*m_delta;
      output.m_p_t=m_delta;
      output.m_p_tt=vec2(0.0f, 0.0f);
    }

  private:
    vec2 m_delta;
  };
  
  
  class bezier_interpolator:public interpolator_base
  {
  public:
    template<typename T>
    bezier_interpolator(type_tag<T>, 
                        const typename WRATHOutline<T>::BezierInterpolator *input)
    {
      typedef typename WRATHOutline<T>::position_type input_position_type;
      input_position_type worker;

      m_points.push_back(cast_to_float(input->position()));

      for(typename std::vector<input_position_type>::const_iterator
            iter=input->m_control_points.begin(), end=input->m_control_points.end();
          iter!=end; ++iter)
        {
          m_points.push_back(cast_to_float(*iter));
        }
      m_points.push_back(cast_to_float(input->to_position()));

      init();
    }

    virtual
    void
    compute(float t, analytic_point_data &output) const;

  private:

    void
    init(void);

    
    std::vector<vec2> m_points;
    vecN<std::vector<vec2>, 3> m_polynomial;
    vecN<std::vector<vec2>, 3> m_reverse_polynomial;
  };

  
  class arc_interpolator:public interpolator_base
  {
  public:
    template<typename T>
    arc_interpolator(type_tag<T>,
                     const typename WRATHOutline<T>::ArcInterpolator *arc)
    {
      init(arc->m_angle,
           arc->m_counter_clockwise,
           cast_to_float(arc->position()),
           cast_to_float(arc->to_position()));
    }

    virtual
    void
    compute(float t, analytic_point_data &output) const;

  private:
    void
    init(float angle,
         bool ccw,
         const vec2 &st, 
         const vec2 &ed);

    float m_radius;
    float m_angle_speed, m_angle_speed_sq;
    float m_angle0;
    vec2 m_center;
  };
  
  template<typename T>
  class generic_interpolator:public interpolator_base
  {
  public:
    generic_interpolator(const typename WRATHOutline<T>::GenericInterpolator *input):
      m_input(input)
    {
      typename WRATHOutline<T>::position_type st, e;

      m_is_flat=false;

      st=input->position();
      e=input->to_position();

      compute(0.0f, m_start_pt);
      compute(1.0f, m_end_pt);
    }

    virtual
    void
    compute(float t, analytic_point_data &output) const
    {
      m_input->compute(t, 
                       output.m_p,
                       output.m_p_t,
                       output.m_p_tt);
                       
    }

  private:
    const typename WRATHOutline<T>::GenericInterpolator *m_input;
  };

  template<typename T>
  interpolator_base*
  construct_interpolator(const typename WRATHOutline<T>::point &input)
  {
    /*
      Basic idea: check for the types and construct the
      correct edge_computer_base object.

      Is this really wise to create an interpolator
      based off of types? Should we allow for something
      more generic??

      Only C++ coding style can convert what would
      be a switch in C to a sequence of if's with
      dynamic_cast<>... or maybe I am just an idiot.
     */
    typedef typename WRATHOutline<T>::BezierInterpolator bezier_t;
    typedef typename WRATHOutline<T>::ArcInterpolator arc_t;
    typedef typename WRATHOutline<T>::GenericInterpolator gen_t;

    const arc_t *arc;
    const bezier_t *bez;
    const gen_t *gen;

    arc=dynamic_cast<const arc_t*>(input.interpolator());
    if(arc!=NULL)
      {
        return WRATHNew arc_interpolator(type_tag<T>(), arc);
      }

    gen=dynamic_cast<const gen_t*>(input.interpolator());
    if(gen!=NULL)
      {
        return WRATHNew generic_interpolator<T>(gen);
      }
    
    bez=dynamic_cast<const bezier_t*>(input.interpolator());
    if(bez!=NULL and !bez->m_control_points.empty())
      {
        return WRATHNew bezier_interpolator(type_tag<T>(), bez);
      }

    return WRATHNew edge_interpolator(type_tag<T>(), input);
  }

  /*
    real implementation to tessellation is in geometry_computer
   */
  class geometry_computer
  {
  public:  
    typedef std::vector<interpolator_base*> outline_type;

    template<typename T>
    geometry_computer(const WRATHShape<T> &pshape, 
                      const WRATHShapeSimpleTessellatorPayload::PayloadParams &params,
                      std::vector<WRATHShapeSimpleTessellatorPayload::TessellatedOutline::handle> &rtessellation_data,
                      WRATHBBox<2> &rbox):
      m_tessellation(rtessellation_data),
      m_box(rbox)
    {
      for(int i=0, endi=pshape.number_outlines(); i<endi; ++i)
        {
          add_outline<T>(pshape.outline(i));
        }
      compute_implement(params);
    }

    virtual
    ~geometry_computer();

  private:

    template<typename T>
    void
    add_outline(const WRATHOutline<T> *outline)
    {
      m_input_outline_data.push_back(outline_type());

      for(typename std::vector<typename WRATHOutline<T>::point>::const_iterator
            iter=outline->points().begin(), end=outline->points().end();
          iter!=end; ++iter)
        {
          m_input_outline_data.back().push_back(construct_interpolator<T>(*iter));
        }
    }

    /*
      This is the method that does the actual tesselation, etc.
     */
    void
    compute_implement(const WRATHShapeSimpleTessellatorPayload::PayloadParams &params);

    WRATHShapeSimpleTessellatorPayload::TessellatedOutline::handle
    create_outline(int outlineID,
                   const outline_type &outline,
                   const WRATHShapeSimpleTessellatorPayload::PayloadParams &param);

    WRATHShapeSimpleTessellatorPayload::TessellatedEdge::handle
    create_edge(const interpolator_base *edge,
                int pointID, int nextPointID,
                const WRATHShapeSimpleTessellatorPayload::PayloadParams &params,
                int outlineID);

    std::vector<WRATHShapeSimpleTessellatorPayload::TessellatedOutline::handle> &m_tessellation;
    WRATHBBox<2> &m_box;

    std::list<outline_type> m_input_outline_data;
  };

}



///////////////////////////////////////////////
// WRATHShapeSimpleTessellatorPayload methods
template<typename T>
inline
WRATHShapeSimpleTessellatorPayload::
WRATHShapeSimpleTessellatorPayload(const WRATHShape<T> &pshape,
                                   const WRATHShapeSimpleTessellatorPayload::PayloadParams &pp):
  m_parameters(pp)
{
  /*
    Ctor of geometry_computer fills m_tessellation and m_box
   */
  WRATHShapeSimpleTessellatorPrivateImplement::geometry_computer compute(pshape, pp,
                                                                         m_tessellation,
                                                                         m_box);
  
}


