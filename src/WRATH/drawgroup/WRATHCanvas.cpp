/*! 
 * \file WRATHCanvas.cpp
 * \brief file WRATHCanvas.cpp
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
#include "WRATHCanvas.hpp"


namespace
{
  template<typename S>
  WRATHCanvas::DataHandle
  create_and_allocate_worker(WRATHCanvas *pthis,
                             const WRATHAttributeStoreKey &k,
                             int req_number_elements,
                             S &R,
                             const WRATHCompiledItemDrawStateCollection &pkey,
                             const WRATHCanvas::SubKeyBase &psubkey,
                             unsigned int implicit_store)
  {
    WRATHCanvas::DataHandle return_value;

    if(!pthis->accepts_subkey(psubkey))
      {
        return return_value;
      }

    WRATHAttributeStore::handle attrib_store;

    attrib_store=pthis->attribute_store(k, req_number_elements, R);
    if(!attrib_store.valid())
      {
        return return_value;
      }

    return_value=pthis->create(attrib_store, pkey, psubkey, implicit_store);
    WRATHassert(return_value.valid());

    return_value.set_implicit_attribute_data(R);
    return return_value;
  }
}


//////////////////////////////////////////
// WRATHCanvas::DataHandle methods
int
WRATHCanvas::DataHandle::
allocate_attribute_data(int number_elements) const
{
  int R;
  
  R=attribute_store()->allocate_attribute_data(number_elements);
  if(R!=-1)
    {
      set_implicit_attribute_data( range_type<int>(R, R+number_elements));
    }
  return R;
}

enum return_code
WRATHCanvas::DataHandle::
allocate_attribute_data(int number_elements, range_type<int> &R) const
{
  enum return_code ret;
  
  ret=attribute_store()->allocate_attribute_data(number_elements, R);
  if(routine_success==ret)
    {
      set_implicit_attribute_data(R);
    }
  return ret;
}

enum return_code
WRATHCanvas::DataHandle::
fragmented_allocate_attribute_data(int number_elements,
                                   std::vector< range_type<int> > &out_allocations) const
{
  unsigned int sz(out_allocations.size());
  enum return_code ret;
  
  ret=attribute_store()->fragmented_allocate_attribute_data(number_elements, 
                                                            out_allocations);
  
  if(routine_success==ret)
    {
      if(sz<out_allocations.size())
        {
          const_c_array<range_type<int> > Rs(&out_allocations[sz], out_allocations.size()-sz);
          set_implicit_attribute_data(Rs);
        }
    }                    
  return ret;
}


//////////////////////////////////////////
// WRATHCanvas methods
WRATHCanvas::DataHandle
WRATHCanvas::
create_and_allocate(const WRATHAttributeStoreKey &k,
                    int req_number_elements_continuous,
                    range_type<int> &R,
                    const WRATHCompiledItemDrawStateCollection &pkey,
                    const SubKeyBase &psubkey, unsigned int implicit_store)
{
  return create_and_allocate_worker(this, k, req_number_elements_continuous,
                                    R, pkey, psubkey, implicit_store);
}

WRATHCanvas::DataHandle
WRATHCanvas::
create_and_allocate(const WRATHAttributeStoreKey &k,
                    int req_number_elements_continuous,
                    std::vector<range_type<int> > &R,
                    const WRATHCompiledItemDrawStateCollection &pkey,
                    const SubKeyBase &psubkey, unsigned int implicit_store)
{
  R.clear();
  return create_and_allocate_worker(this, k, req_number_elements_continuous,
                                    R, pkey, psubkey, implicit_store);
}

enum return_code
WRATHCanvas::
transfer(DataHandle &in_group)
{
  if(in_group.parent()==this)
    {
      return routine_success;
    }

  /*
    the size of the implicit attribute must be the
    same between the two canvas objects..
   */
  if(!same_implicit_attribute_type(in_group.parent()))
    {
      return routine_fail;
    }

  DataHandle R;

  
  R=create(in_group.attribute_store(),
           in_group.item_draw_state(), 
           *in_group.custom_data()->subkey(),
           in_group.implicit_store());
  
  in_group.release_group();
  in_group=R;

  return routine_success;
}

enum return_code
WRATHCanvas::
transfer(DataHandle &in_group,
                   const_c_array< range_type<int> > out_allocations)
{
  enum return_code R;

  if(in_group.parent()==this)
    {
      return routine_success;
    }

  R=transfer(in_group);

  if(R==routine_success)
    {
      in_group.set_implicit_attribute_data(out_allocations);
    }

  return R;
  
}


void
WRATHCanvas::DataHandle::
set_implicit_attribute_data(const_c_array<range_type<int> > R) const
{
  WRATHassert(valid());
  WRATHassert(m_implicit_buffer_object!=NULL);
  custom_data()->set_implicit_attribute_data(R, m_implicit_buffer_object);
}


