/*! 
 * \file WRATHDefaultFillShapeAttributePacker.cpp
 * \brief file WRATHDefaultFillShapeAttributePacker.cpp
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
#include "WRATHDefaultFillShapeAttributePacker.hpp"
#include "WRATHDefaultStrokeAttributePacker.hpp"
#include "WRATHAttributePackerHelper.hpp"



namespace
{
  class attribute_type:
    public WRATHInterleavedAttributes<vec2>
  {
  public:
    void
    position(const vec2 &v)
    {
      get<WRATHDefaultFillShapeAttributePacker::position_location>()=v;
    }
  };

  enum fake_iterator_begin_t
    {
      fake_iterator_begin
    };
  
  enum fake_iterator_end_t
    {
      fake_iterator_end
    };

  template<typename A>
  class FakePtIterator
  {
  public:
    typedef WRATHShapeTriangulatorPayload::PointBase PointBase;
    typedef WRATHDefaultFillShapeAttributePacker::FillingParameters FillingParameters;

    FakePtIterator(const WRATHShapeTriangulatorPayload::handle &h, 
                   enum fake_iterator_begin_t,
                   const FillingParameters &pp):
      m_src(h),
      m_iter(0), 
      m_pp(pp)
    {}

    FakePtIterator(const WRATHShapeTriangulatorPayload::handle &h, 
                   enum fake_iterator_end_t,
                   const FillingParameters &pp):
      m_src(h),
      m_iter(m_src->number_points_without_splits()), 
      m_pp(pp)
    {}


    bool
    operator==(const FakePtIterator &rhs) const
    {
      WRATHassert(m_src==rhs.m_src);
      WRATHassert(&m_pp==&rhs.m_pp);
      return m_iter==rhs.m_iter;
    }

    bool
    operator!=(const FakePtIterator &rhs) const
    {     
      WRATHassert(m_src==rhs.m_src);
      WRATHassert(&m_pp==&rhs.m_pp);
      return m_iter!=rhs.m_iter;
    }

    FakePtIterator&
    operator++(void)
    {
      ++m_iter;
      return *this;
    }

    A
    operator*(void) const
    {
      A R;

      R.position(m_src->point(m_iter)->m_position + m_pp.m_translate);
      return R;
    }

  private:
    WRATHShapeTriangulatorPayload::handle m_src;
    unsigned int m_iter;
    const FillingParameters &m_pp;
  };

  template<typename A>
  void
  add_indices(WRATHAttributePackerHelper<A, GLushort> &worker, c_array<GLushort> index_array,
              const std::map<int, WRATHShapeTriangulatorPayload::FilledComponent> &c,
              const WRATHDefaultFillShapeAttributePacker::FillingParameters &fill_params)
  {
    WRATHDefaultIndexWriter<GLushort> index_writer(index_array);
    for(std::map<int, WRATHShapeTriangulatorPayload::FilledComponent>::const_iterator 
          iter=c.begin(), end=c.end(); iter!=end; ++iter)
    {
      if(!iter->second.triangle_indices().empty() and fill_params.fill(iter->first))
        {
          worker.add_indices(iter->second.triangle_indices().begin(),
                             iter->second.triangle_indices().end(),
                             index_writer);
        }
    }
  }


  typedef const char *attribute_label_type;    
}




////////////////////////////////////////////////////
// WRATHDefaultFillShapeAttributePacker methods
const_c_array<attribute_label_type>
WRATHDefaultFillShapeAttributePacker::
attribute_names(void)
{
  static const attribute_label_type attribute_labels[]=
    {
      "pos",
    };
  return const_c_array<attribute_label_type>(attribute_labels, 1);
}


WRATHShapeAttributePackerBase::allocation_requirement_type
WRATHDefaultFillShapeAttributePacker::
allocation_requirement(WRATHShapeTriangulatorPayload::handle h,
                       const FillingParameters &fill_params)
{
  WRATHShapeAttributePackerBase::allocation_requirement_type A;

  WRATHassert(h.valid());
  if(h.valid())
    {
      A.m_number_attributes=h->number_points_without_splits();
      for(std::map<int, WRATHShapeTriangulatorPayload::FilledComponent>::const_iterator 
            iter=h->components().begin(),
            end=h->components().end(); iter!=end; ++iter)
        {
          if(fill_params.fill(iter->first))
            {
              A.m_primary_number_indices+=iter->second.triangle_indices().size();
            }
        }
    }
  return A;
}

GLenum
WRATHDefaultFillShapeAttributePacker::
attribute_key(WRATHAttributeStoreKey &attrib_key)
{ 
  attrib_key.type_and_format(type_tag<attribute_type>());
  return GL_TRIANGLES;  
}

void
WRATHDefaultFillShapeAttributePacker::
set_attribute_data(WRATHShapeTriangulatorPayload::handle h,
                   WRATHAbstractDataSink &attribute_store,
                   const std::vector<range_type<int> > &attr_location,
                   WRATHAbstractDataSink *index_group,
                   const FillingParameters &fill_params)
{
  WRATHassert(h.valid());
  WRATHassert(&attribute_store!=NULL);
  WRATHassert(index_group!=NULL);

  WRATHShapeAttributePackerBase::allocation_requirement_type AA;
  AA=allocation_requirement(h, fill_params);

  WRATHassert(WRATHAttributeStore::total_size(attr_location)>=static_cast<unsigned int>(AA.m_number_attributes));
  
  if(AA.m_number_attributes==0 or AA.m_primary_number_indices==0)
    {
      return;
    }

  WRATHAutoLockMutex(attribute_store.mutex());
  WRATHAutoLockMutex(index_group->mutex());


  c_array<GLushort> index_array;
  index_array=index_group->pointer<GLushort>(0, AA.m_primary_number_indices);

  
  WRATHAttributePackerHelper<attribute_type, GLushort> worker(attribute_store,
                                                              attr_location.begin(), 
                                                              attr_location.end());
  
  worker.set_attribute_src(h->number_points_without_splits(),
                           FakePtIterator<attribute_type>(h, fake_iterator_begin, fill_params), 
                           FakePtIterator<attribute_type>(h, fake_iterator_end, fill_params));
  add_indices(worker, index_array, h->components(), fill_params);

}
