/*! 
 * \file WRATHAntiAliasFillShapeAttributePacker.cpp
 * \brief file WRATHAntiAliasFillShapeAttributePacker.cpp
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
#include "WRATHAntiAliasFillShapeAttributePacker.hpp"
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
      get<WRATHAntiAliasFillShapeAttributePacker::position_location>()=v;
    }

    void
    hint(float I)
    {
      get<WRATHAntiAliasFillShapeAttributePacker::hint_distance_location>()=static_cast<float>(I);
    }

    

    /*
      this is silly, so we do not have to get hairy with the
      template type, we make the float fetch and assignent possible
      via silliness....
     */
    void
    texture_y_coordinate_gradient(float) 
    {
    }
    
  };

  class attribute_type_with_y_gradient:
    public WRATHInterleavedAttributes<vec2, float>
  {
  public:

    void
    position(const vec2 &v)
    {
      get<WRATHAntiAliasFillShapeAttributePacker::position_location>()=v;
    }

    void
    hint(float I)
    {
      get<WRATHAntiAliasFillShapeAttributePacker::hint_distance_location>()=static_cast<float>(I);
    }
    
    void
    texture_y_coordinate_gradient(float v) 
    {
      get<WRATHAntiAliasFillShapeAttributePacker::gradient_y_coordinate_location>()=v;
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

  
  typedef WRATHShapeTriangulatorPayload::PointBase PointBase;
  typedef WRATHAntiAliasFillShapeAttributePacker::FillingParameters FillingParameters;
  typedef WRATHShapeTriangulatorPayload::FilledComponent FilledComponent;
  
  /*
    FakeIteratorHelper lets FakePtIterator
    "just" work by providing the needed
    re-indexing information to map from
    a FakePtIterator internal index to an
    attribute to a split triangulation index.

    We use a FakePtIteratorHelper whenever
    we add a filled component to use its
    split attribute and triangulation data
   */
  class FakeIteratorHelper
  {
  public:

    FakeIteratorHelper(const WRATHShapeTriangulatorPayload::handle &h):
      m_C(h->components().begin()->second), //any component will do actually
      m_split_attribute_range(h->number_points_without_splits(),
                              h->number_points_without_splits()),
      m_subtract_offset(0)
    {}

    FakeIteratorHelper(int offset, const FilledComponent &C):
      m_C(C),
      m_split_attribute_range(C.split_points_range()),
      m_offset(offset)
    {}

    bool
    is_split_vertex(unsigned int I) const
    {
      WRATHassert(I<m_split_attribute_range.m_end);
      WRATHassert(I>=m_split_attribute_range.m_begin 
                  or I<m_C.induced_pts().size() + m_C.unbounded_pts().size() + m_C.pts().size());
      return I>=m_split_attribute_range.m_begin;
    }

    unsigned int 
    convert_index(unsigned int I) const
    {
      return is_split_vertex(I)?
        I-m_split_attribute_range.m_begin+m_offset:
        I;
    }

    const PointBase*
    point(unsigned int I)
    {
      return m_C.point(convert_index(I));
    }

    const FilledComponent&
    src(void) const 
    {
      return m_C;
    }

  private:
    const FilledComponent &m_C;
    int m_offset;
    range_type<unsigned int> m_split_attribute_range;
  };

  /*
    A FakeIndexIterator converts a specific range
    of index value to a different range.
    This is needed for that only some
    of the filled components use the vertex
    data of the split triangulation
   */
  class FakeIndexIterator
  {
  public:
    FakeIndexIterator(const FakePtIteratorHelper &helper,
                      fake_iterator_begin_t):
      m_src(helper),
      m_iter(0)
    {}

    FakeIndexIterator(const FakePtIteratorHelper &helper,
                      fake_iterator_end_t):
      m_src(helper),
      m_iter(m_src.src().split_triangulation_indices().size()),
    {}

    bool
    operator==(const FakeIndexIterator &rhs) const
    {
      WRATHassert(&m_src==rhs.m_src);
      return m_iter==rhs.m_iter;
    }

    bool
    operator!=(const FakeIndexIterator &rhs) const
    {     
      WRATHassert(&m_src==rhs.m_src);
      return m_iter!=rhs.m_iter;
    }

    FakeIndexIterator&
    operator++(void)
    {
      ++m_iter;
      return *this;
    }

    GLushort
    operator*(void) const
    {
      return m_src.convert_index( m_src.src().split_triangulation_indices()[m_iter] );
    }

  private:
    const FakePtIteratorHelper m_src;
    unsigned int m_iter;
  };


  template<typename A>
  class FakePtIterator
  {
  public:

    FakePtIterator(const FakeIteratorHelper &m_src,
                   enum fake_iterator_begin_t,
                   const FillingParameters &pp,
                   std::pair<bool, float> f):
      m_src(h),
      m_iter(0), 
      m_pp(pp), 
      m_texture_y_coordinate_gradient(f)
    {}

    FakePtIterator(const FakeIteratorHelper &m_src,
                   enum fake_iterator_end_t,
                   const FillingParameters &pp,
                   std::pair<bool, float> f):
      m_src(h),
      m_iter(m_src->number_points_without_splits()), 
      m_pp(pp), 
      m_texture_y_coordinate_gradient(f)
    {}


    bool
    operator==(const FakePtIterator &rhs) const
    {
      WRATHassert(&m_src==&rhs.m_src);
      return m_iter==rhs.m_iter;
    }

    bool
    operator!=(const FakePtIterator &rhs) const
    {     
      WRATHassert(&m_src==&rhs.m_src);
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
      const PointBase *pt;

      //aa_hint is defaulted to being 1.0 == outside of shape
      //when we add the indices of a filled component that is filled
      //by the fill rule, those attributes then have the aa_hint
      //value set as 0.0
      A R;
      pt=m_src.point(m_iter);
      R.position(pt->m_position + m_pp.m_translate);
      R.hint(1.0f);

      if(m_texture_y_coordinate_gradient.first)
        {
          R.texture_y_coordinate_gradient(m_texture_y_coordinate_gradient.second);
        }
      return R;
    }

  private:
    const FakeIteratorHelper &m_src;
    unsigned int m_iter;
    std::pair<bool, float> m_texture_y_coordinate_gradient;
  };

  template<typename A>
  void
  add_component(WRATHAttributePackerHelper<A> &worker, 
                c_array<GLushort> opaque_index_array,
                c_array<GLushort> translucent_index_data,
                const std::map<int, FilledComponent> &c,
                const WRATHAntiAliasFillShapeAttributePacker::FillingParameters &fill_params)
  {
    WRATHDefaultIndexWriter<GLushort> opaque_index_writer(opaque_index_array);
    WRATHDefaultIndexWriter<GLushort> translucent_index_writer(translucent_index_array);

    for(std::map<int, FilledComponent>::const_iterator 
          iter=c.begin(), end=c.end(); iter!=end; ++iter)
    {
      if(!iter->second.triangle_indices().empty())
        {
          if(fill_params.fill(iter->first))
            {
              worker.add_indices(iter->second.triangle_indices().begin(),
                                 iter->second.triangle_indices().end(),
                                 opaque_index_writer);

              /*
                the attributes used in iter->second.triangle_indices()
                need to have their AA hint set to 0
               */
              for(const_c_array<GLushort>::iterator 
                    index_iter=iter->second.triangle_indices().begin(),
                    index_end=iter->second.triangle_indices().end(); 
                  index_iter!=index_end; ++index_iter)
                {
                  GLushort index_in_store;
                  c_array<A> attr_ptr;

                  index_in_store=worker.index_remapper()[*index_iter];
                  attr_ptr=worker.attribute_store().pointer<A>(index_in_store, 1);
                  attr_ptr[0].hint(0.0f);
                }
            }
          else
            {
              unsigned int num_added;

              /*
                the worker places those attributes
                added as the index range

                [ worker.().size(), worker.().size() + num_added )

                the index range for the split indices from
                the filled component.
               */
              FakeIteratorHelper helper(worker.index_remapper().size(), iter->second);

              num_added=iter->second.split_induced_pts().size();
              worker.add_attribute_data(num_added,
                                        FakePtIterator<A>(helper, fake_iterator_begin, 
                                                          fill_params, texture_coordinate_y_gradient),
                                        FakePtIterator<A>(helper, fake_iterator_end, 
                                                          fill_params, texture_coordinate_y_gradient));

              worker.add_indices(FakeIndexIterator(helper, fake_iterator_begin),
                                 FakeIndexIterator(helper, fake_iterator_end),
                                 translucent_index_writer);
                                       
            }
        }
    }
  }


  typedef const char *attribute_label_type;    
}




////////////////////////////////////////////////////
// WRATHAntiAliasFillShapeAttributePacker methods
const_c_array<attribute_label_type>
WRATHAntiAliasFillShapeAttributePacker::
attribute_names(bool include_y_gradient_attribute)
{
  static const attribute_label_type attribute_labels[]=
    {
      "pos",
      "in_aa_hint",
      "gradient_y_coordinate",
    };
  static const_c_array<attribute_label_type> withoutGradientY(attribute_labels, 2);
  static const_c_array<attribute_label_type> withGradientY(attribute_labels, 3);

  return (include_y_gradient_attribute)?
    withGradientY:
    withoutGradientY;
  
}


WRATHShapeAttributePackerBase::allocation_requirement_type
WRATHAntiAliasFillShapeAttributePacker::
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
          else
            {
              /*
                non-filled components contribute additional vertices
                for their split triangulations.
               */
              int sz(iter->second.split_points_range().m_end - iter->second.split_points_range().m_end);

              A.m_secondary_number_indices+=iter->second.split_triangulation_indices().size();
              A.m_number_attributes+=sz;
            }
        }
    }
  return A;
}

GLenum
WRATHAntiAliasFillShapeAttributePacker::
attribute_key(WRATHAttributeStoreKey &attrib_key, 
              bool include_y_gradient_attribute)
{
  if(include_y_gradient_attribute)
    {
      attrib_key.type_and_format(type_tag<attribute_type_with_y_gradient>());
    }
  else
    {
      attrib_key.type_and_format(type_tag<attribute_type>());
    }
  return GL_TRIANGLES;  
}

void
WRATHAntiAliasFillShapeAttributePacker::
set_attribute_data(WRATHShapeTriangulatorPayload::handle h,
                   WRATHAttributeStore::handle attribute_store,
                   const std::vector<range_type<int> > &attr_location,
                   WRATHShapeAttributePackerBase::IndexGroupBase<GLushort> *opaque_index_group,
                   WRATHShapeAttributePackerBase::IndexGroupBase<GLushort> *translucent_index_group,
                   const FillingParameters &fill_params,
                   std::pair<bool, float> texture_coordinate_y_gradient)
{
  WRATHassert(h.valid());
  WRATHassert(attribute_store.valid());
  WRATHassert(index_group!=NULL);

  WRATHShapeAttributePackerBase::allocation_requirement_type AA;
  AA=allocation_requirement(h, fill_params);

  WRATHassert(WRATHAttributeStore::total_size(attr_location)>=static_cast<unsigned int>(AA.m_number_attributes));
  
  if(AA.m_number_attributes==0)
    {
      return;
    }

  WRATHAutoLockMutex(attribute_store->mutex());
  WRATHAutoLockMutex(index_group->mutex());


  c_array<GLushort> opaque_index_data(opaque_index_group->ptr());
  c_array<GLushort> translucent_index_data(translucent_index_group->ptr());
  WRATHassert(static_cast<int>(opaque_index_data.size())>=AA.m_primary_number_indices);
  WRATHassert(static_cast<int>(translucent_index_data.size())>=AA.m_secondary_number_indices);


  if(texture_coordinate_y_gradient.first)
    {
      FakeIteratorHelper helper(h);
      WRATHAttributePackerHelper<attribute_type_with_y_gradient> worker(attribute_store,
                                                                        attr_location.begin(), 
                                                                        attr_location.end());
      
      /*
        add all the attributes that are not split points.
       */
      worker.set_attribute_src(h->number_points_without_splits(),
                               FakePtIterator<attribute_type_with_y_gradient>(helper, fake_iterator_begin, 
                                                                              fill_params, texture_coordinate_y_gradient), 
                               FakePtIterator<attribute_type_with_y_gradient>(helper, fake_iterator_end,
                                                                              fill_params, texture_coordinate_y_gradient));
     

      add_indices(worker, opaque_index_data, translucent_index_data, h->components(), fill_params);
    }
  else
    {
      FakeIteratorHelper helper(h);
      WRATHAttributePackerHelper<attribute_type> worker(attribute_store,
                                                        attr_location.begin(), 
                                                        attr_location.end());
      
      worker.add_attribute_src(h->number_points_without_splits(),
                               FakePtIterator<attribute_type>(helper, fake_iterator_begin, 
                                                              fill_params, texture_coordinate_y_gradient), 
                               FakePtIterator<attribute_type>(helper, fake_iterator_end, 
                                                              fill_params, texture_coordinate_y_gradient));
      add_indices(worker, opaque_index_data, translucent_index_data, h->components(), fill_params);
    }


  

}
