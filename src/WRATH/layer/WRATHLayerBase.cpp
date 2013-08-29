/*! 
 * \file WRATHLayerBase.cpp
 * \brief file WRATHLayerBase.cpp
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
#include "WRATHLayerBase.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHStaticInit.hpp"


/*
  Implementation overview

  1) Fetching of DataHandles happens behind locking m_mutex.
     

  2) A MetaGroup, via MetaGroupBase has a "main"
     WRATHItemGroup, given by MetaGroupBase::m_main_group,
     with it's own private index buffer. 
     That main group is for when all
     WRATDrawOrder values are NULL. For the case
     where any of them is not-NULL, it has a map, 
     MetaGroup::m_item_groups, keyed by WRATDrawOrder
     values with value as a pointer to a WRATHItemGroup.
     These WRATHItemGroup all share the same index buffer,
     given by MetaGroup::m_shared_index_buffer.
     Reserving/unreserving of slots is handled by
     MetaGroup essentially via the interface
     WRATHLayerBase::GLStateOfNodeCollection. Each
     render pass has it's own GLStateOfNodeCollection.
     However, the _same_ GLStateOfNodeCollection
     are used regardless of the value for the WRATHDrawOrder.
     This allows the WRATHDrawOrder to vary without
     forcing a drawcall break always. Whenever a node is used 
     the MetaGroup a counter (m_use_count) is incremented by the
     adder and whenever a node is no longer using it is decremented. 
     This incrementing and decrementing is done by hand in
     WRATHLayerBase::create_no_lock() and WRATHLayerBase::release_group_no_lock().
     If the counter goes to zero (function MetaGroup::in_use()) the MetaGroup is deleted.

  3) The book-keeping of finding a pre-existing MetaGroup from a key
     is handled via the std::map, WRATHLayerBase::m_map which is
     keyed by by the tuple (WRATHLayerBase::key_type)
     - WRATHAttibuteStore
     - implicit handle
     - std::vector<WRATHCompiledItemDrawState> 
     Note that varyng the WRATHDrawOrder values is not picked
     up by the key. The map have values as WRATHLayerBase::value_type.
     A WRATHLayerBase::value_type represents all MetaGroup 
     (stored via pointer) for a given key. It has a set of MetaGroup* 
     indicating those MetaGroup that have free slots and it as a map 
     keyed by WRATHLayerItemNodeBase* with value as a MetaGroup* 
     giving _THE_ MetaGroup that has the given node in one of it's
     slots. As a side note, when a MetaGroup goes out of scope it purges
     itself from the value_type bookkeeper 
     (see WRATHLayerBase::value_type::purge_meta_group_nolock)

  4) The design of WRATH dictates that the attribute value at index 0 indicates 
     an entirely clipped element. To handle this, slot #0 is always occupied in
     a MetaGroup by a node pointed to by MetaGroup::m_non_visible_node. This
     value is passed from the ctor arguments and created with the function
     create_completely_clipped_node() member function of the return value
     of WRATHLayerItemNodeBase::node_functions(). 
     
     
 */


namespace
{ 
  template<typename S>
  class absorb
  {
  public:
    typedef typename S::const_handle return_type;
    typedef typename S::element_type_collection collection;
    typedef typename collection::const_iterator iterator;

    static
    return_type
    fetch(const return_type &H, const collection &pset)
    {                                           
      if(pset.empty())
        {
          return H;
        }
      else
        {
          collection it(H->elements());
          for(iterator iter=pset.begin(), end=pset.end(); iter!=end; ++iter)
            {
              it.insert(*iter);
            } 
          return WRATHCompiledItemDrawState::fetch(it);
        }
    }
  };
 
   


  void
  set_draw_spec(WRATHDrawCallSpec &output,
                const WRATHAttributeStore::handle &attribute_store,
                const WRATHIndexGroupAllocator::handle &index_store,
                const WRATHCompiledItemDrawState &in_state,
                const WRATHSubItemDrawState &subkey,
                int implicit_store)
  {
    output.m_program=in_state.m_drawer->program();
    output.m_attribute_format_location=attribute_store->attribute_format_location();
    output.m_force_draw_order=NULL;
    output.m_draw_command=index_store->draw_command();
    output.m_data_source=attribute_store->buffer_object_vector(implicit_store);

    output.m_gl_state_change=absorb<WRATHGLStateChange>::fetch(in_state.m_gl_state_change, subkey.m_gl_state_change);  
    output.m_bind_textures=absorb<WRATHTextureChoice>::fetch(in_state.m_textures, subkey.m_textures); 
    output.m_uniform_data=absorb<WRATHUniformData>::fetch(in_state.m_uniforms, subkey.m_uniforms);   
  }

}

//////////////////////////////////////////
// WRATHLayerBase::DrawerBase methods
WRATHLayerBase::DrawerBase::
~DrawerBase()
{
  for(std::vector<GLStateOfLayer*>::iterator 
        iter=m_GLStateOfLayers.begin(), end=m_GLStateOfLayers.end();
      iter!=end; ++iter)
    {
      WRATHDelete(*iter);
    }        
}

void
WRATHLayerBase::DrawerBase::
append_GLStateOfLayers(WRATHLayerBase *layer,
                           WRATHSubItemDrawState &sk)
{
  for(std::vector<GLStateOfLayer*>::iterator 
        iter=m_GLStateOfLayers.begin(), end=m_GLStateOfLayers.end();
      iter!=end; ++iter)
    {
      (*iter)->append_state(layer, sk);
    }
}




//////////////////////////////////////////
// WRATHLayerBase::CustomData methods
bool
WRATHLayerBase::CustomData::
operator<(const CustomData &obj) const
{
  #define EPIC_LAZY(X) do { if (X!=obj.X) return X<obj.X; } while(0)
  EPIC_LAZY(slot());
  EPIC_LAZY(m_subkey.m_node);
  EPIC_LAZY(m_meta);
  return false;

  #undef EPIC_LAZY
}

void
WRATHLayerBase::CustomData::
set_implicit_attribute_data(const_c_array< c_array<NodeIndexAttribute> > Rs) const
{
  for(const_c_array<c_array<NodeIndexAttribute> >::iterator iter=Rs.begin(),
        end=Rs.end(); iter!=end; ++iter)
    {
      std::fill(iter->begin(), iter->end(), m_value);
    }
}

//////////////////////////////////////////
// WRATHLayerBase::MetaGroupBase methods
WRATHLayerBase::MetaGroupBase::
MetaGroupBase(const WRATHAttributeStore::handle &attr_store, 
              unsigned int implicit_slot,
              const_c_array<WRATHCompiledItemDrawState> draw_state,
              WRATHLayerBase *player):
  m_main_group(NULL),
  m_use_count(0),
  m_number_slots(0),
  m_main_group_specs(draw_state.size())
{
  WRATHassert(!draw_state.empty());
  
  WRATHIndexGroupAllocator::handle index_allocator;

  index_allocator=WRATHNew WRATHIndexGroupAllocator(draw_state[0].m_primitive_type,
                                                    draw_state[0].m_buffer_object_hint,
                                                    attr_store);
  
  //need to make sure the implicit store
  //exists before setting the DrawCall values.
  attr_store->add_implicit_store(implicit_slot);
  

  for(unsigned int i=0, endi=draw_state.size(); i<endi; ++i)
    {
      const WRATHCompiledItemDrawState &st(draw_state[i]);
      WRATHItemGroup::DrawCall &current(m_main_group_specs[i]);
      WRATHSubItemDrawState subkey;
      DrawerBase *dr;

      dr=dynamic_cast<DrawerBase*>(st.m_drawer);
      if(dr!=NULL)
        {
          GLStateOfNodeCollection *pkt;
          unsigned int dr_slots(dr->number_slots());
          
          pkt=dr->allocate_node_packet(player);
          pkt->append_state(subkey);
          dr->append_GLStateOfLayers(player, subkey);
          m_node_gl.push_back(pkt);
          if(dr_slots>0)
            {
              m_number_slots=(m_number_slots>0)?
                std::min(m_number_slots, dr_slots):
                dr_slots;
            }
        }
      current.first=player->fetch_raw_data_nolock(st.m_draw_type);
      set_draw_spec(current.second, 
                    attr_store, index_allocator,
                    st, subkey, implicit_slot);
    }
  m_main_group=WRATHNew WRATHItemGroup(index_allocator, m_main_group_specs, 
                                       draw_state, 
                                       implicit_slot);
}


WRATHLayerBase::MetaGroupBase::
~MetaGroupBase()
{
  WRATHPhasedDelete(m_main_group);
  for(std::vector<GLStateOfNodeCollection*>::iterator 
        iter=m_node_gl.begin(), 
        end=m_node_gl.end();
      iter!=end; ++iter)
    {
      WRATHPhasedDelete(*iter);
    }
}

//////////////////////////////////////////
// WRATHLayerBase::MetaGroup methods
WRATHLayerBase::MetaGroup::
MetaGroup(const WRATHAttributeStore::handle &attr_store, 
          unsigned int implicit_slot,
          const std::vector<WRATHCompiledItemDrawState> &draw_state,
          value_type *v, WRATHLayerBase *player,
          WRATHLayerItemNodeBase *non_visible_node):
  MetaGroupBase(attr_store, implicit_slot, draw_state, player),
  m_slot_allocator(MetaGroupBase::m_number_slots),
  m_value(v),
  m_non_visible_node(non_visible_node)
{
  if(MetaGroupBase::m_number_slots>0)
    {
      int slot_zero;
      slot_zero=add_element(m_non_visible_node);
      WRATHassert(slot_zero==0);
      WRATHunused(slot_zero);
    }
  m_shared_index_buffer=WRATHNew WRATHBufferAllocator(attr_store->buffer_allocator()->triple_buffer_enabler(),
                                                      draw_state[0].m_buffer_object_hint);
}

WRATHLayerBase::MetaGroup::
~MetaGroup()
{
  WRATHassert(!in_use());
  if(m_value!=NULL)
    {
      m_value->purge_meta_group_nolock(this, m_slot_allocator.active_elements(), m_non_visible_node);
    }

  for(item_map::iterator 
        iter=m_item_groups.begin(), 
        end=m_item_groups.end(); 
      iter!=end; ++iter)
    {
      WRATHPhasedDelete(iter->second);
    }
  WRATHPhasedDelete(m_shared_index_buffer);
  WRATHPhasedDelete(m_non_visible_node);
}

WRATHItemGroup*
WRATHLayerBase::MetaGroup::
item_group(const std::vector<WRATHDrawOrder::const_handle> &force_draw_orders)
{
  for(unsigned int i=0, endi=force_draw_orders.size(); i<endi; ++i)
    {
      if(force_draw_orders[i].valid())
        {
          return fetch_item_group(force_draw_orders);
        }
    }
  return m_main_group;
}

WRATHItemGroup*
WRATHLayerBase::MetaGroup::
fetch_item_group(const std::vector<WRATHDrawOrder::const_handle> &force_draw_orders)
{

  item_map::iterator iter;

  iter=m_item_groups.find(force_draw_orders);
  if(iter!=m_item_groups.end())
    {
      return iter->second;
    }

  WRATHItemGroup *G;

  std::vector<WRATHItemGroup::DrawCall> specs(m_main_group_specs);
  WRATHIndexGroupAllocator::handle index_store;

  /*
    a WRATHItemGroup of a specific draw order is identical
    to m_main_group except that:
    - it uses a different draw order vector
    - it uses a different index store

    The index store shares the index buffer with all
    WRATHItemGroup that specify a draw order.
   */
  index_store=WRATHNew WRATHIndexGroupAllocator(m_main_group->item_draw_state().primitive_type(),
                                                m_shared_index_buffer,
                                                m_main_group->attribute_store());

  for(unsigned int i=0, endi=specs.size(); i<endi; ++i)
    {
      specs[i].second.m_draw_command=index_store->draw_command();
      specs[i].second.m_force_draw_order=force_draw_orders[i];
    }

  
  G=WRATHNew WRATHItemGroup(index_store,
                            specs,
                            WRATHCompiledItemDrawStateCollection(m_main_group->item_draw_state().draw_states(),
                                                                 force_draw_orders),
                            m_main_group->implicit_store());

  m_item_groups[force_draw_orders]=G;
  return G;
}

void
WRATHLayerBase::MetaGroup::
skip_bookkeeping_cleanup(void)
{
  m_value=NULL;
  m_use_count=0;
}

int
WRATHLayerBase::MetaGroup::
slot_location(WRATHLayerItemNodeBase *v)
{
  if(MetaGroupBase::m_number_slots>0)
    {
      return m_slot_allocator.slot_location(v);
    }
  else
    {
      WRATHwarning("Attempt to find slot_location from group having m_number_slots=0");  
      return (v!=NULL and v!=m_non_visible_node)?
        1:0;
    }
}

bool
WRATHLayerBase::MetaGroup::
slot_allocated_for_node(WRATHLayerItemNodeBase *v)
{
  return (MetaGroupBase::m_number_slots>0)?
    m_slot_allocator.slot_allocated_for_value(v):
    true;
}

bool
WRATHLayerBase::MetaGroup::
has_slots_available(void)
{
  return (MetaGroupBase::m_number_slots>0)?
    m_slot_allocator.free_slots_available():
    true;
}


int
WRATHLayerBase::MetaGroup::
add_element(WRATHLayerItemNodeBase *node)
{
  if(MetaGroupBase::m_number_slots==0)
    {
      WRATHwarning("Attempt to add_element to group having m_number_slots=0");  
      return (node!=NULL and node!=m_non_visible_node)?
        1:0;
    }


  bool is_new_slot;
  int slot, high_slot;

  is_new_slot=!slot_allocated_for_node(node);
  slot=m_slot_allocator.add_element(node);
  high_slot=m_slot_allocator.highest_slot_allocated();

  WRATHassert(slot!=-1);
  if(is_new_slot)
    {
      for(std::vector<GLStateOfNodeCollection*>::iterator 
            iter=m_node_gl.begin(), end=m_node_gl.end();
          iter!=end; ++iter)
        {
          (*iter)->assign_slot(slot, node, high_slot);
        }
    }

  return slot;
}

void
WRATHLayerBase::MetaGroup::
remove_element(WRATHLayerItemNodeBase *node)
{
  if(MetaGroupBase::m_number_slots==0)
    {
      return;
    }

  WRATHassert(slot_allocated_for_node(node));

  int slot, high_slot;

  slot=slot_location(node);
  m_slot_allocator.remove_element(node);

  high_slot=m_slot_allocator.highest_slot_allocated();

  if(!slot_allocated_for_node(node))
    {
      for(std::vector<GLStateOfNodeCollection*>::iterator 
            iter=m_node_gl.begin(),
            end=m_node_gl.end();
          iter!=end; ++iter)
        {
          (*iter)->assign_slot(slot, NULL, high_slot);
        }

      /*
        now clear this from m_value->m_has:
       */
      std::map<WRATHLayerItemNodeBase*, MetaGroup*>::iterator miter;

      miter=m_value->m_has.find(node);
      WRATHassert(miter!=m_value->m_has.end());
      WRATHassert(miter->second==this);
      
      m_value->m_has.erase(miter);
      m_value->m_has_free_slots.insert(this);      
    }

}

//////////////////////////////////////////
// WRATHLayerBase::value_type methods
void
WRATHLayerBase::value_type::
purge_meta_group_nolock(MetaGroup *ptr,
                        const WRATHSlotAllocator<WRATHLayerItemNodeBase*>::map_type &list,
                        WRATHLayerItemNodeBase *exclude)
{
  m_has_free_slots.erase(ptr);
  for(WRATHSlotAllocator<WRATHLayerItemNodeBase*>::map_type::const_iterator iter=list.begin(),
        end=list.end(); iter!=end; ++iter)
    {
      WRATHLayerItemNodeBase *node(iter->first);
      std::map<WRATHLayerItemNodeBase*, MetaGroup*>::iterator miter;

      if(node!=exclude)
        {
          miter=m_has.find(node);
          WRATHassert(miter!=m_has.end());
          m_has.erase(miter);
        }
    }
}


/////////////////////////////////////////////
// WRATHLayerBase methods
WRATHLayerBase::
WRATHLayerBase(const WRATHTripleBufferEnabler::handle &tr,
               const WRATHDrawOrderComparer::handle sorter):
  WRATHCanvas(tr, type_tag<NodeIndexAttribute>(), 
                           NodeIndexAttribute( GLubyte(0) )),
  m_sorter(sorter)
{
}


WRATHLayerBase::
~WRATHLayerBase()
{
  /*
    We put off deleting the MetaGroup's until dtor
    because of the folling ugly in phased deletion:
    WRATHWidget types for WRATHLayer inherit (eventually)
    from WRATHLayerItemNodeBase. The WRATHWidget classes
    inheritance is first WRATHLayerItemNodeBase-derived class,
    then a WRATHItem class. When the WRATHWidget is phased
    deleted, then the WRATHItem does not get deleted until
    _after_ the dtor of WRATHLayerItemNodeBase. Now, if
    a user does:

    WRATHPhasedDelete(m_widget);
    WRATHPhasedDelete(m_layer);

    then the order of operations is:
   
    WRATHLayerItemNodeBase::on_place_on_deletion_list
    WRATHLayerBase::on_place_on_deletion_list

    WRATHLayerItemNodeBase::phase_simulation_deletion
    WRATHLayerBase::phase_simulation_deletion

    WRATHLayerItemNodeBase::phase_render_deletion
    WRATHLayerBase::phase_render_deletion

    ~WRATHLayerItemNodeBase
    ~ItemOfWidget
    ~WRATHLayerBase

    we need to make sure the WRATHItemGroup objects
    are still alive when dtor or ItemOfWidget is
    called, thus we delay deleting the WRATHItemGroup
    objects until ~WRATHLayerBase. 
   */

  WRATHAutoLockMutex(m_mutex);
  for(std::set<MetaGroup*>::iterator iter=m_meta_groups.begin(),
        end=m_meta_groups.end(); iter!=end; ++iter)
    {
      (*iter)->skip_bookkeeping_cleanup();
      WRATHPhasedDelete(*iter);
    }
  m_meta_groups.clear();

  /*
    phase delete the WRATHRawDrawData objects _AFTER_ 
    the WRATHItemGroup's of each MetaGroup are phased deleted.
   */
  for(unsigned int i=0, endi=m_raw_datas.size(); i<endi; ++i)
    {
      for(std::map<int, WRATHRawDrawData*>::iterator iter=m_raw_datas[i].begin(),
            end=m_raw_datas[i].end(); iter!=end; ++iter)
        {
          WRATHPhasedDelete(iter->second);
        }
      m_raw_datas[i].clear();
    }
}

void
WRATHLayerBase::
on_place_on_deletion_list(void)
{
  WRATHAutoLockMutex(m_roots_mutex);
  for(std::map<type_info_key, WRATHLayerItemNodeBase*>::iterator 
        iter=m_roots.begin(), end=m_roots.end(); iter!=end; ++iter)
    {
      WRATHPhasedDelete(iter->second);
    }
  m_roots.clear();

  WRATHCanvas::on_place_on_deletion_list();
}


WRATHCanvas::DataHandle
WRATHLayerBase::
create(const WRATHAttributeStore::handle &attrib_store,
       const WRATHCompiledItemDrawStateCollection &pkey,
       WRATHLayerItemNodeBase *pNode, unsigned int implicit_store)
{
  DataHandle G;

  WRATHLockMutex(m_mutex);
  G=create_no_lock(attrib_store, implicit_store, pkey, pNode);
  WRATHUnlockMutex(m_mutex);

  return G;
  
}



void
WRATHLayerBase::
release_group(DataHandle &g)
{
  WRATHLockMutex(m_mutex);
  release_group_no_lock(g);
  WRATHUnlockMutex(m_mutex);  
}


WRATHCanvas::DataHandle
WRATHLayerBase::
create_no_lock(const WRATHAttributeStore::handle &attr_store, 
               unsigned int implicit_slot,
               const WRATHCompiledItemDrawStateCollection &draw_state,
               WRATHLayerItemNodeBase *node)
{
  /*
    first get a value_type that matches the given key:
   */
  map_type::iterator iter;
  MetaGroup *mg;
  value_type *v;
  int slot;
  key_type key(attr_store, implicit_slot, draw_state.draw_states());

  WRATHassert(node!=NULL);

  iter=m_map.find(key);
  if(iter!=m_map.end())
    {
      v=&iter->second;
    }
  else
    {
      std::pair<map_type::iterator, bool> R;

      R=m_map.insert( map_type::value_type(key, value_type()));
      WRATHassert(R.second);
      v=&R.first->second;
    }

  std::map<WRATHLayerItemNodeBase*, MetaGroup*>::iterator findnode;

  findnode=v->m_has.find(node);
  if(findnode!=v->m_has.end())
    {
      WRATHassert(findnode->second!=NULL);
      mg=findnode->second;
      slot=mg->add_element(node);
    }
  else
    {
      if(!v->m_has_free_slots.empty())
        {
          mg=*v->m_has_free_slots.begin();
          v->m_has_free_slots.erase(v->m_has_free_slots.begin());
        }
      else
        {
          mg=WRATHNew MetaGroup(attr_store, implicit_slot, draw_state.draw_states(),
                                v, this, 
                                node->node_functions().create_completely_clipped_node(triple_buffer_enabler()));
          m_meta_groups.insert(mg);
        }
      v->m_has[node]=mg;
      slot=mg->add_element(node);
    }

  if(mg->has_slots_available())
    {
      v->m_has_free_slots.insert(mg);
    }

  /*
    now create the needed custom data:
   */
  CustomData C(slot, node, mg);
  std::map<CustomData, int>::iterator Citer;

  Citer=m_custom_data_objs.find(C);
  if(Citer==m_custom_data_objs.end())
    {
      std::pair<std::map<CustomData, int>::iterator, bool> R;

      R=m_custom_data_objs.insert( std::map<CustomData, int>::value_type(C, 0));
      Citer=R.first;
    }
  ++Citer->second;
  mg->increment_use_count();
  return DataHandle(mg->item_group(draw_state.force_draw_orders()), &Citer->first, this);
}

void
WRATHLayerBase::
release_group_no_lock(DataHandle &g)
{
  /*
    track down the MetaGroup, and release using the
    node that g uses. If this is the last group of
    MetaGroup, kill the MetaGroup
   */
  const CustomData *c;
  MetaGroup *mg;
  WRATHLayerItemNodeBase *node;

  c=dynamic_cast<const CustomData*>(g.custom_data());
  WRATHassert(c!=NULL);

  mg=c->m_meta;
  WRATHassert(mg!=NULL);

  node=c->m_subkey.m_node;
  WRATHassert(node!=NULL);

  mg->remove_element(node);
  mg->decrement_use_count();

  if(!mg->in_use())
    {
      m_meta_groups.erase(mg);
      WRATHDelete(mg);
    }

  std::map<CustomData, int>::iterator iter;
  g=DataHandle();

  iter=m_custom_data_objs.find(*c);
  WRATHassert(iter!=m_custom_data_objs.end());

  --iter->second;
  WRATHassert(iter->second>=0);
  if(iter->second==0)
    {
      m_custom_data_objs.erase(iter);
    }
}


WRATHRawDrawData*
WRATHLayerBase::
fetch_raw_data_nolock(WRATHDrawType p)
{
  std::map<int, WRATHRawDrawData*>::iterator iter;
  WRATHRawDrawData *R;

  iter=m_raw_datas[p.m_type].find(p.m_value);
  if(iter==m_raw_datas[p.m_type].end())
    {

      R=WRATHNew WRATHRawDrawData(triple_buffer_enabler(), m_sorter);
      m_raw_datas[p.m_type][p.m_value]=R;

      schedule_rendering_action( boost::bind(&WRATHLayerBase::add_raw_draw_data_to_array,
                                             this, p, R));

    }
  else
    {
      R=iter->second;
      WRATHassert(p.m_value==iter->first);
    }


  return R;
}

void
WRATHLayerBase::
add_raw_draw_command(WRATHDrawType draw_type,
                     WRATHRawDrawDataElement *b)
{
  WRATHAutoLockMutex(m_mutex);

  WRATHRawDrawData *ptr;
  ptr=fetch_raw_data_nolock(draw_type);
  schedule_simulation_action( boost::bind( &WRATHRawDrawData::add_element,
                                           ptr, b));
}

void
WRATHLayerBase::
add_raw_draw_data_to_array(WRATHDrawType p, WRATHRawDrawData *ptr)
{
  WRATHassert(m_render_raw_datas[p.m_type].find(p.m_value)==m_render_raw_datas[p.m_type].end());
  m_render_raw_datas[p.m_type][p.m_value]=ptr;
}




