/*! 
 * \file WRATHGenericStrokeAttributePacker.cpp
 * \brief file WRATHGenericStrokeAttributePacker.cpp
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
#include "WRATHGenericStrokeAttributePacker.hpp"
#include "WRATHAttributePackerHelper.hpp"

namespace
{
  typedef WRATHGenericStrokeAttributePacker::OutputAttributeProducer OutputAttributeProducer;
  typedef WRATHShapePreStrokerPayload::MiterJoinPoint MiterJoinPoint;
  typedef WRATHShapePreStrokerPayload::JoinPoint JoinPoint;
  typedef WRATHShapePreStrokerPayload::CapPoint CapPoint;
  typedef WRATHShapeSimpleTessellatorPayload::CurvePoint CurvePoint;

  class bevel_join_facade
  {
  public:
    bevel_join_facade(const JoinPoint &pt):
      m_pt(pt)
    {}

    const JoinPoint &m_pt;
  };

  class miter_join_facade
  {
  public:
    miter_join_facade(const MiterJoinPoint &pt):
      m_pt(pt)
    {}

    const MiterJoinPoint &m_pt;
  };

  class round_join_facade
  {
  public:
    round_join_facade(const JoinPoint &pt):
      m_pt(pt)
    {}

    const JoinPoint &m_pt;
  };

  class cap_facade
  {
  public:
    cap_facade(const CapPoint &pt):
      m_pt(pt)
    {}
   
    const CapPoint &m_pt;
  };

  class curve_facade
  {
  public:
    curve_facade(float n,
                 const CurvePoint &pt):
      m_normal_direction_multiplier(n),
      m_pt(pt)
    {}
   
    float m_normal_direction_multiplier;
    const CurvePoint &m_pt;
  };

  class attribute_walker
  {
  public:
    struct initialize_args
    {
      initialize_args(int v,
                      const OutputAttributeProducer *o):
        m_attribute_size(v),
        m_o(o)
      {}

      int m_attribute_size;
      const OutputAttributeProducer *m_o;
    };

    attribute_walker(struct initialize_args v):
      m_params(v)
    {}

    void
    set(WRATHAbstractDataSink &attribute_store,
        const range_type<int> &R)
    {
      range_type<int> S(R.m_begin*m_params.m_attribute_size, 
                        R.m_end*m_params.m_attribute_size);
      m_destination=attribute_store.pointer<uint8_t>(S.m_begin, S.m_end-S.m_begin);
    }

    void
    write_value(int I,
                bevel_join_facade rhs)
    {
      m_params.m_o->generate_attribute_bevel(destination_at(I), rhs.m_pt, I);
    }

    void
    write_value(int I,
                miter_join_facade rhs)
    {
      m_params.m_o->generate_attribute_miter(destination_at(I), rhs.m_pt, I);
    }

    void
    write_value(int I,
                round_join_facade rhs)
    {
      m_params.m_o->generate_attribute_round(destination_at(I), rhs.m_pt, I);
    }

    void
    write_value(int I,
                cap_facade rhs)
    {
      m_params.m_o->generate_attribute_cap(destination_at(I), rhs.m_pt, I);
    }

    void
    write_value(int I,
                curve_facade rhs)
    {
      m_params.m_o->generate_attribute_edge_pt(destination_at(I),
                                               rhs.m_normal_direction_multiplier, 
                                               rhs.m_pt, I);
    } 

  private:
    c_array<uint8_t>
    destination_at(int I)
    {
      return m_destination.sub_array(I*m_params.m_attribute_size, 
                                     m_params.m_attribute_size);
    }

    c_array<uint8_t> m_destination;
    initialize_args m_params;
  };


  template<typename T, typename Facade>
  class JoinCapFakeIterator
  {
  public:
    explicit
    JoinCapFakeIterator(typename const_c_array<T>::iterator iter):
      m_iter(iter)
    {}

    bool
    operator==(const JoinCapFakeIterator &rhs) const
    {
      return m_iter==rhs.m_iter;
    }

    bool
    operator!=(const JoinCapFakeIterator &rhs) const
    {
      return !operator==(rhs);
    }

    JoinCapFakeIterator&
    operator++(void)
    {
      ++m_iter;
      return *this;
    }

    Facade
    operator*(void) const
    {
      return Facade(*m_iter);
    }

  private:
    typename const_c_array<T>::iterator m_iter;
  };

  typedef JoinCapFakeIterator<JoinPoint, bevel_join_facade> FakeBevelJoinIterator;
  typedef JoinCapFakeIterator<MiterJoinPoint, miter_join_facade> FakeMiterJoinIterator;
  typedef JoinCapFakeIterator<JoinPoint, round_join_facade> FakeRoundJoinIterator;
  typedef JoinCapFakeIterator<CapPoint, cap_facade> FakeCapIterator;




  class edge_point_container_facade
  {
  public:
    typedef WRATHShapeSimpleTessellatorPayload::TessellatedEdge TessellatedEdge;

    edge_point_container_facade(TessellatedEdge::handle E,
                                bool draw_edges_as_double_quads):
      m_curve_points(E->curve_points()),
      m_multer(draw_edges_as_double_quads?3:2),
      m_number_quads_per_edge(draw_edges_as_double_quads?2:1),
      m_draw_edges_as_double_quads(draw_edges_as_double_quads)
    {}

    unsigned int
    number_attributes(void) const
    {
      return m_multer*m_curve_points.size();
    }
    
    curve_facade
    get_attribute(unsigned int I) const
    {
      unsigned int sourceI(I/m_multer);
      const float hfs[3]={1.0f, -1.0f, 0.0};
      float hf(hfs[I%m_multer]);

      return curve_facade(hf, m_curve_points[sourceI]);
    }

    unsigned int
    number_quads(void) const
    {
      return (m_curve_points.empty())?
        0:
        m_number_quads_per_edge*(m_curve_points.size()-1);
    }

    unsigned int 
    number_indices(void) const
    {
      return number_quads()*6;
    }

    unsigned int
    get_index(unsigned int I) const
    {
      unsigned int E, addendum;
      const unsigned int subs_single_quad[6]=
        {
          0,1,2,
          1,2,3
        }; 

      const unsigned int subs_double_quad[12]=
        {
          0,4,2,
          4,2,5,
          1,4,5,
          1,5,3
        };

      /*
        E=which edge, an edge may generate 1 or 2 quads.
        We want that the divide is rounded down!
       */
      E=I/(m_number_quads_per_edge*6);
      addendum=(m_draw_edges_as_double_quads)?
        subs_double_quad[I%12]:
        subs_single_quad[I%6];

      /*
        Now that we have which edge and which offset
        we need to multiple E by the number of 
        attributes per quad:
       */
      return m_multer*E + addendum;
    }

    class attribute_iterator
    {
    public:
      attribute_iterator(unsigned int pI, const edge_point_container_facade &psrc):
        m_I(pI), m_src(psrc)
      {}
      
      curve_facade
      operator*(void) const
      {
        return m_src.get_attribute(m_I);
      }
      
      attribute_iterator&
      operator++(void)
      {
        ++m_I;
        return *this;
      }
      
      bool
      operator==(const attribute_iterator &rhs) const
      {
        WRATHassert(&m_src==&rhs.m_src);
        return m_I==rhs.m_I;
      }
      
      bool
      operator!=(const attribute_iterator &rhs) const
      {
        return !operator==(rhs);
      }
      
    private:
      unsigned int m_I;
      const edge_point_container_facade &m_src;
    };


    class index_iterator
    {
    public:
      index_iterator(unsigned int pI, 
                     const edge_point_container_facade &psrc):
        m_I(pI), m_src(psrc)
      {}
      
      unsigned int
      operator*(void) const
      {
        return m_src.get_index(m_I);
      }
      
      index_iterator&
      operator++(void)
      {
        ++m_I;
        return *this;
      }
      
      bool
      operator==(const index_iterator &rhs) const
      {
        WRATHassert(&m_src==&rhs.m_src);
        return m_I==rhs.m_I;
      }
      
      bool
      operator!=(const index_iterator &rhs) const
      {
        return !operator==(rhs);
      }
      
    private:
      unsigned int m_I;
      const edge_point_container_facade &m_src;
    };

    index_iterator
    begin_index(void) const
    {
      return index_iterator(0, *this);
    }

    index_iterator
    end_index(void) const
    {
      return index_iterator(number_indices(), *this);
    }

    attribute_iterator
    begin_attribue(void) const
    {
      return attribute_iterator(0, *this);
    }

    attribute_iterator
    end_attribute(void) const
    {
      return attribute_iterator(number_attributes(), *this);
    }


  private:
    const std::vector<CurvePoint> &m_curve_points;
    unsigned int m_multer, m_number_quads_per_edge;
    bool m_draw_edges_as_double_quads;
  };
}

WRATHShapeAttributePackerBase::allocation_requirement_type
WRATHGenericStrokeAttributePacker::
allocation_requirement(WRATHShapePreStrokerPayload::handle h,
                       const StrokingParameters &pp,
                       bool draw_edges_as_double_quads)
{
  WRATHShapeAttributePackerBase::allocation_requirement_type A;

  WRATHassert(h.valid());

  if(pp.m_generate_flags&WRATHShapePreStrokerPayload::generate_miter_joins)
    {
      A.m_number_attributes+=h->miter_join_pts(pp.m_close_outline).size();
      A.m_primary_number_indices+=h->miter_join_indices(pp.m_close_outline).size();
    }

  if(pp.m_generate_flags&WRATHShapePreStrokerPayload::generate_bevel_joins)
    {
      A.m_number_attributes+=h->bevel_join_pts(pp.m_close_outline).size();
      A.m_primary_number_indices+=h->bevel_join_indices(pp.m_close_outline).size();
    }

  if(pp.m_generate_flags&WRATHShapePreStrokerPayload::generate_rounded_joins)
    {
      A.m_number_attributes+=h->rounded_join_pts(pp.m_close_outline).size();
      A.m_primary_number_indices+=h->rounded_join_indices(pp.m_close_outline).size();
    }

  if(pp.m_generate_flags&WRATHShapePreStrokerPayload::generate_square_caps)
    {
      A.m_number_attributes+=h->square_cap_pts().size();
      A.m_primary_number_indices+=h->square_cap_indices().size();
    }

  if(pp.m_generate_flags&WRATHShapePreStrokerPayload::generate_rounded_caps)
    {
      A.m_number_attributes+=h->rounded_cap_pts().size();
      A.m_primary_number_indices+=h->rounded_cap_indices().size();
    }

  /*
    TODO: observe the stroke style.
   */
  if(pp.m_stroke_curves!=no_stroke)
    {
      WRATHShapeSimpleTessellatorPayload::handle Tsrc(h->tessellation_src());
      
      for(unsigned int iO=0, endO=Tsrc->tessellation().size();
          iO!=endO; ++iO)
        {
          unsigned int endE;
          
          endE=Tsrc->tessellation()[iO]->edges().size();
          if(!pp.stroke_closed(iO))
            {
              --endE;
            }
          
          for(unsigned int iE=0; iE<endE; ++iE)
            {
              edge_point_container_facade R(Tsrc->tessellation()[iO]->edges()[iE], 
                                            draw_edges_as_double_quads);
              A.m_number_attributes+=R.number_attributes();
              A.m_primary_number_indices+=R.number_indices();
            }
        }
    }

  return A;
}




void
WRATHGenericStrokeAttributePacker::
set_attribute_data(WRATHShapePreStrokerPayload::handle h,
                   WRATHAbstractDataSink &attribute_store,
                   const std::vector<range_type<int> > &attr_location,
                   WRATHAbstractDataSink *index_group,
                   const OutputAttributeProducer &P,
                   const StrokingParameters &pp,
                   bool draw_edges_as_double_quads) 
{
  struct attribute_walker::initialize_args jazz(P.attribute_size(), &P);

  WRATHassert(h.valid());
  WRATHassert(&attribute_store!=NULL);
  WRATHassert(index_group!=NULL);

  WRATHShapeAttributePackerBase::allocation_requirement_type AA;
  AA=allocation_requirement(h, pp, draw_edges_as_double_quads);

  WRATHassert(static_cast<int>(WRATHAttributeStore::total_size(attr_location))>=AA.m_number_attributes);

  if(AA.m_number_attributes==0 or AA.m_primary_number_indices==0)
    {
      return;
    }

  WRATHAutoLockMutex(attribute_store.mutex());
  WRATHAutoLockMutex(index_group->mutex());
  
  c_array<GLushort> index_array;
  index_array=index_group->pointer<GLushort>(0, AA.m_primary_number_indices);


  WRATHDefaultIndexWriter<GLushort> index_writer(index_array);
  WRATHGenericAttributePackerHelper<attribute_walker, GLushort> worker(attribute_store,
                                                                       attr_location.begin(), 
                                                                       attr_location.end(),
                                                                       jazz);

  if(pp.m_generate_flags&WRATHShapePreStrokerPayload::generate_miter_joins)
    {
      worker.add_data(h->miter_join_pts(pp.m_close_outline).size(),
                      FakeMiterJoinIterator(h->miter_join_pts(pp.m_close_outline).begin()), 
                      FakeMiterJoinIterator(h->miter_join_pts(pp.m_close_outline).end()),
                      h->miter_join_indices(pp.m_close_outline).begin(), 
                      h->miter_join_indices(pp.m_close_outline).end(),
                      index_writer);
    }
  
  if(pp.m_generate_flags&WRATHShapePreStrokerPayload::generate_bevel_joins)
    {
      worker.add_data(h->bevel_join_pts(pp.m_close_outline).size(),
                      FakeBevelJoinIterator(h->bevel_join_pts(pp.m_close_outline).begin()), 
                      FakeBevelJoinIterator(h->bevel_join_pts(pp.m_close_outline).end()),
                      h->bevel_join_indices(pp.m_close_outline).begin(), 
                      h->bevel_join_indices(pp.m_close_outline).end(),
                      index_writer  );
    }
  
  if(pp.m_generate_flags&WRATHShapePreStrokerPayload::generate_rounded_joins)
    {
      worker.add_data(h->rounded_join_pts(pp.m_close_outline).size(),
                      FakeRoundJoinIterator(h->rounded_join_pts(pp.m_close_outline).begin()), 
                      FakeRoundJoinIterator(h->rounded_join_pts(pp.m_close_outline).end()),
                      h->rounded_join_indices(pp.m_close_outline).begin(), 
                      h->rounded_join_indices(pp.m_close_outline).end(),
                      index_writer  );
    }
  
  if(pp.m_generate_flags&WRATHShapePreStrokerPayload::generate_square_caps)
    {
      worker.add_data(h->square_cap_pts().size(),
                      FakeCapIterator(h->square_cap_pts().begin()), 
                      FakeCapIterator(h->square_cap_pts().end()),
                      h->square_cap_indices().begin(), 
                      h->square_cap_indices().end(),
                      index_writer  );
    }
  
  if(pp.m_generate_flags&WRATHShapePreStrokerPayload::generate_rounded_caps)
    {
      worker.add_data(h->rounded_cap_indices().size(),
                      FakeCapIterator(h->rounded_cap_pts().begin()), 
                      FakeCapIterator(h->rounded_cap_pts().end()),
                      h->rounded_cap_indices().begin(), 
                      h->rounded_cap_indices().end(),
                      index_writer  );
    }
  

   /*
    TODO: observe the stroke style.
   */
  if(pp.m_stroke_curves!=no_stroke)
    {
      WRATHShapeSimpleTessellatorPayload::handle Tsrc(h->tessellation_src());

      for(unsigned int iO=0, endO=Tsrc->tessellation().size();
          iO!=endO; ++iO)
        {
          unsigned int endE;
          
          endE=Tsrc->tessellation()[iO]->edges().size();
          if(!pp.stroke_closed(iO))
            {
              --endE;
            }
          
          for(unsigned int iE=0; iE<endE; ++iE)
            {
              edge_point_container_facade R(Tsrc->tessellation()[iO]->edges()[iE], 
                                            draw_edges_as_double_quads);
              worker.add_data(R.number_attributes(),
                              R.begin_attribue(), R.end_attribute(),
                              R.begin_index(), R.end_index(),
                              index_writer);
            }
        }
    }
  

}
