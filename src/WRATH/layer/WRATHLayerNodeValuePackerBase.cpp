/*! 
 * \file WRATHLayerNodeValuePackerBase.cpp
 * \brief file WRATHLayerNodeValuePackerBase.cpp
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
#include "WRATHLayerNodeValuePackerBase.hpp"
#include "WRATHLayerItemNodeBase.hpp"

/*
  Implementation overview:
  
  1) The data to send to GL is triple buffered with the
     member m_data_to_pack_to_GL

  2) The function pack_data is called at the end of each
     simulation frame before the triple buffer ID's are
     updated (via the connection saved in m_sim_signal).
     This function essentially calls extract_data()
     on each node in the node list, m_nodes, so that 
     either the correct index of m_data_to_pack_to_GL is
     written to directly or the values are copied
     into the correct index of m_data_to_pack_to_GL

 */




/////////////////////////////////////////////////
// WRATHLayerNodeValuePackerBase::ActiveNodeValues methods
WRATHLayerNodeValuePackerBase::ActiveNodeValues::
ActiveNodeValues(void)
{}

WRATHLayerNodeValuePackerBase::ActiveNodeValues&
WRATHLayerNodeValuePackerBase::ActiveNodeValues::
absorb(const ActiveNodeValues &obj, const Filter::const_handle &hnd)
{
  /*
    basically walk obj, and add its entries:
   */
  for(map_type::const_iterator 
	source_iter=obj.m_data.begin(),
	source_end=obj.m_data.end(); 
      source_iter!=source_end; ++source_iter)
    {
      if(!hnd.valid() or hnd->absorb_active_node_value(source_iter->second))
        {
          map_type::iterator dest_iter;
          
          dest_iter=fetch_source_iterator(source_iter->first);
          std::copy(source_iter->second.m_labels.begin(), 
                    source_iter->second.m_labels.end(),
                    std::inserter(dest_iter->second.m_labels, 
                                  dest_iter->second.m_labels.end()));
        }
    }

  return *this;
}

WRATHLayerNodeValuePackerBase::ActiveNodeValues::map_type::iterator
WRATHLayerNodeValuePackerBase::ActiveNodeValues::
fetch_source_iterator(unsigned int source_index)
{
  map_type::iterator iter;

  iter=m_data.find(source_index);
  if(iter==m_data.end())
    {
      ActiveNodeValue entry;
      
      entry.m_source_index=source_index;
      entry.m_offset=number_active();

      if(m_permutation_array.size() <= source_index)
	{
	  m_permutation_array.resize(1+source_index, -1);
	}
      m_permutation_array[source_index]=entry.m_offset;
      
      iter=m_data.insert(map_type::value_type(entry.m_source_index, entry)).first;
    }
  return iter;
}

WRATHLayerNodeValuePackerBase::ActiveNodeValues&
WRATHLayerNodeValuePackerBase::ActiveNodeValues::
add_source(int source_index, const std::string &label)
{
  if(source_index>=0)
    {
      map_type::iterator iter;

      iter=fetch_source_iterator(source_index);
      iter->second.m_labels.insert(label);
    }
  return *this;
}

namespace
{
  bool 
  compare_map_values(const WRATHLayerNodeValuePackerBase::ActiveNodeValues::map_type::value_type &lhs,
                     const WRATHLayerNodeValuePackerBase::ActiveNodeValues::map_type::value_type &rhs)
  {
    return lhs.first < rhs.first;
  }
}

bool
WRATHLayerNodeValuePackerBase::ActiveNodeValues::
contains(const ActiveNodeValues &obj) const
{
  return std::includes(m_data.begin(), m_data.end(),
                       obj.m_data.begin(), obj.m_data.end(),
                       compare_map_values);
}

bool
WRATHLayerNodeValuePackerBase::ActiveNodeValues::
same(const ActiveNodeValues &obj) const
{
  /*
    there is actually a better way to do this, 
    we could walk the range ourselves, but 
    I am lazy.
   */
  return contains(obj) and obj.contains(*this);
}

//////////////////////////////////////////////////////
// WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection methods
WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection&
WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection::
absorb(const ActiveNodeValuesCollection &obj, GLenum shader_stage, 
       const ActiveNodeValues::Filter::const_handle &hnd)
{
  map_type::const_iterator iter;

  iter=obj.entries().find(shader_stage);
  if(iter!=obj.entries().end())
    {
      absorb(iter->second, shader_stage, hnd);
    }
  return *this;
}

bool
WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection::
active_entry(GLenum shader_stage) const
{
  map_type::const_iterator iter;

  iter=m_entries.find(shader_stage);
  return iter!=m_entries.end() and iter->second.number_active()>0;
}



/////////////////////////////////////////////////////
// WRATHLayerNodeValuePackerBase::ProcessedActiveNodeValuesCollection methods
namespace
{
  typedef WRATHLayerNodeValuePackerBase::ActiveNodeValues::Filter::const_handle Filter;
  typedef WRATHLayerNodeValuePackerBase::NodeDataPackParametersCollection::packing_group helper_map_key;

  /*
    helper class for ctor of ProcessedActiveNodeValuesCollection
   */
  class helper_map_value
  {
  public:
    std::map<GLenum, Filter> m_shader_stages;
    int m_index;
  };


  class helper_map:public std::map<helper_map_key, helper_map_value>
  {
  public:

    void
    note_shader(helper_map_key K, GLenum shader, const Filter &f)
    {
      iterator iter;

      iter=find(K);
      if(iter==end())
	{
	  helper_map_value V;

	  V.m_index=size();
	  iter=insert( value_type(K, V)).first;
	}
      iter->second.m_shader_stages[shader]=f;
    }
  }; 
}


void
WRATHLayerNodeValuePackerBase::ProcessedActiveNodeValuesCollection::
set(const NodeDataPackParametersCollection &parameters,
    const ActiveNodeValuesCollection &input,
    const std::map<GLenum, ActiveNodeValues::Filter::const_handle> &active_shader_stages)
{

  m_values.clear();
  m_index_for_stage.clear();
  m_original_collection=input;

  /*
    helper_map is an std::map so that:
     1) keyed by packer_set
     2) value is a struct holding
        a) an index (m_index)
	b) a list of shaders used by the packer_set of the key (std::map<GLenum, Filter>)
     the function note_shader correctly updates the map to note the shader and filter applied to it.
   */
  helper_map helper;

  for(std::map<GLenum, ActiveNodeValues::Filter::const_handle>::const_iterator 
	iter=active_shader_stages.begin(),
	end=active_shader_stages.end();
      iter!=end; ++iter)
    {
      if(input.active_entry(iter->first))
        {
          helper.note_shader(parameters.get_shader_packer(iter->first), iter->first, iter->second);
        }
    }

 
  m_values.resize(helper.size());
  for(helper_map::const_iterator 
	iter=helper.begin(),
	end=helper.end();
      iter!=end; ++iter)
    {
      const helper_map_value &current(iter->second);

      m_values[current.m_index].first=parameters.packer_set_parameters(iter->first);

      /*
        Recall that a fixed shader stage appears in 
        at most one element in helper_map. That element
        also specifies the filter, which was taken
        from active_shader_stages
       */
      for(std::map<GLenum, ActiveNodeValues::Filter::const_handle>::const_iterator 
	    siter=current.m_shader_stages.begin(),
	    send=current.m_shader_stages.end();
	  siter!=send; ++siter)
	{
	  ActiveNodeValuesCollection::map_type::const_iterator mm;
	  
	  m_index_for_stage[siter->first]=current.m_index;
	  mm=input.entries().find(siter->first);
	  if(mm!=input.entries().end() and siter->second.valid())
	    {
	      m_values[current.m_index].second.absorb(mm->second, siter->second);
	    }
	}
    }
}



////////////////////////////////////////////////////////
// WRATHLayerNodeValuePackerBase::per_packer_datum methods
WRATHLayerNodeValuePackerBase::per_packer_datum::
per_packer_datum(WRATHLayerNodeValuePackerBase *pparent):
  m_parent(pparent),
  m_packing_type(NodeDataPackParameters::packed_by_node),
  m_float_alignment(1),
  m_padded_row_size_in_floats(0),
  m_overflow_padding(0),
  m_number_active(0),
  m_data_to_pack_to_GL(m_data_to_pack_to_GL_padded[0],
                       m_data_to_pack_to_GL_padded[1],
                       m_data_to_pack_to_GL_padded[2])
{}


WRATHLayerNodeValuePackerBase::per_packer_datum::
per_packer_datum(WRATHLayerNodeValuePackerBase *pparent,
                 const ActiveNodeValues &used_per_node_values,
                 const NodeDataPackParameters &packing_params,
                 int one_plus_highest_index):
  m_parent(pparent),
  m_permutation_array(used_per_node_values.m_permutation_array),
  m_packing_type(packing_params.m_packing_type),
  m_float_alignment(packing_params.m_float_alignment),
  m_number_active(used_per_node_values.number_active())
{
  
  m_permutation_array.resize(one_plus_highest_index, -1);

  /*
    substitute the -1 for values starting at m_number_active
   */
  for(int 
        fillSlot=m_number_active, 
        i=0, 
        endi=m_permutation_array.size();
      i<endi; ++i)
    {
      if(m_permutation_array[i]==-1)
        {
          m_permutation_array[i]=fillSlot;
          ++fillSlot;
        }
    }


  int row_size_in_floats, padding, modulas, number_rows;

  row_size_in_floats=(m_packing_type==NodeDataPackParameters::packed_by_node)?
    m_number_active:
    m_parent->m_payload->m_number_slots;
  
  number_rows=(m_packing_type==NodeDataPackParameters::packed_by_node)?
    m_parent->m_payload->m_number_slots:
    m_number_active;


  modulas=(m_float_alignment>0)?
    row_size_in_floats%m_float_alignment:
    0;

  padding=(modulas!=0)?
    (m_float_alignment - modulas):
    0;

  m_padded_row_size_in_floats=row_size_in_floats + padding;
  m_overflow_padding=m_permutation_array.size()  - m_number_active;

  for(int i=0; i<3; ++i)
    {
      const_c_array<float> entire_array;
      int new_size;

      new_size=m_padded_row_size_in_floats*number_rows + m_overflow_padding;
      m_data_to_pack_to_GL_padded[i].resize(new_size, 0.0f);

      /*
        note that we drop the padding.
       */
      entire_array=m_data_to_pack_to_GL_padded[i];
      m_data_to_pack_to_GL[i]=entire_array.sub_array(0, m_padded_row_size_in_floats*number_rows);
    }

  m_pack_work_room.resize(m_permutation_array.size());  
}
  
void
WRATHLayerNodeValuePackerBase::per_packer_datum::
pack_data(int number_slots)
{
  c_array<float> write_to;

  write_to=m_data_to_pack_to_GL_padded[m_parent->triple_buffer_enabler()->current_simulation_ID()];
  if(m_packing_type==NodeDataPackParameters::packed_by_node)
    {
      c_array<float> node_write_to;

      for(int node=0; node<number_slots; ++node)
        {
          /*
            note that the values past m_payload->m_used_per_item_uniforms.number_active()
            are written over by the next node.
           */
          node_write_to=write_to.sub_array(node*m_padded_row_size_in_floats, 
                                           m_permutation_array.size());

          if(m_parent->m_nodes[node]!=NULL)
            {
              m_parent->m_nodes[node]->extract_values(reorder_c_array<float>(node_write_to, m_permutation_array));
            }
        }
    }
  else
    {
      for(int node=0; node<number_slots; ++node)
        {
          if(m_parent->m_nodes[node]!=NULL)
            {
              m_parent->m_nodes[node]->extract_values(reorder_c_array<float>(m_pack_work_room, m_permutation_array));
              for(int V=0; V<m_number_active; ++V)
                {
                  write_to[V*m_padded_row_size_in_floats + node]=m_pack_work_room[V];
                }
            }
        }
    }
}

/////////////////////////////////////////////////////
// WRATHLayerNodeValuePackerBase::DataToGL methods
WRATHLayerNodeValuePackerBase*
WRATHLayerNodeValuePackerBase::DataToGL::
parent(void) const
{
  const per_packer_datum *p(static_cast<const per_packer_datum*>(m_actual_data));
  return p->m_parent;
}


enum WRATHLayerNodeValuePackerBase::NodeDataPackParameters::data_packing_type 
WRATHLayerNodeValuePackerBase::DataToGL::
packing_type(void) const
{
  const per_packer_datum *p(static_cast<const per_packer_datum*>(m_actual_data));
  return p->m_packing_type;
}

int
WRATHLayerNodeValuePackerBase::DataToGL::
float_alignment(void) const
{
  const per_packer_datum *p(static_cast<const per_packer_datum*>(m_actual_data));
  return p->m_float_alignment;
}

const_c_array<float>
WRATHLayerNodeValuePackerBase::DataToGL::
data_to_pack_to_GL(void) const
{
  const per_packer_datum *p(static_cast<const per_packer_datum*>(m_actual_data));
  int I(p->m_parent->triple_buffer_enabler()->present_ID());
  return p->m_data_to_pack_to_GL[I];
}


int
WRATHLayerNodeValuePackerBase::DataToGL::
number_slots_to_pack_to_GL(void) const
{
  const per_packer_datum *p(static_cast<const per_packer_datum*>(m_actual_data));
  int I(p->m_parent->triple_buffer_enabler()->present_ID());
  return p->m_parent->m_number_slots_to_pack_to_GL[I];
}

const_c_array<float>
WRATHLayerNodeValuePackerBase::DataToGL::
data_to_pack_to_GL_restrict(void) const
{
  const per_packer_datum *p(static_cast<const per_packer_datum*>(m_actual_data));
  int I(p->m_parent->triple_buffer_enabler()->present_ID());
  int num_slots(p->m_parent->m_number_slots_to_pack_to_GL[I]);
  int size(num_slots*p->m_padded_row_size_in_floats);
	    

  return(p->m_packing_type==NodeDataPackParameters::packed_by_node and p->m_padded_row_size_in_floats>0)?
    p->m_data_to_pack_to_GL[I].sub_array(0, size):
    p->m_data_to_pack_to_GL[I];
}

bool
WRATHLayerNodeValuePackerBase::DataToGL::
non_empty(void) const
{
  const per_packer_datum *p(static_cast<const per_packer_datum*>(m_actual_data));
  return p->m_number_active!=0;
}



/////////////////////////////////////////////////////
// WRATHLayerNodeValuePackerBase methods
WRATHLayerNodeValuePackerBase::
WRATHLayerNodeValuePackerBase(WRATHLayerBase *layer,
                              const SpecDataProcessedPayload::const_handle &ppayload,
			      const ProcessedActiveNodeValuesCollection &spec):
  WRATHLayerBase::GLStateOfNodeCollection(layer->triple_buffer_enabler()),
  m_payload(ppayload),  
  m_highest_slot(-1),
  m_number_slots_to_pack_to_GL(0, 0, 0),
  m_nodes(m_payload->m_number_slots, static_cast<WRATHLayerItemNodeBase*>(NULL)),
  m_empty_packer(this),
  m_packers_by_shader(spec.shader_entries())
{
    
  m_packers.resize(spec.number_indices(), NULL);
  for(int i=0, endi=spec.number_indices(); i<endi; ++i)
    {
      
      m_packers[i]=WRATHNew per_packer_datum(this, 
                                             spec.active_node_values(i),
                                             spec.packer_parameters(i),
                                             spec.original_data().one_plus_highest_index());
        
    }

  m_sim_signal=connect(WRATHTripleBufferEnabler::on_complete_simulation_frame,
                       WRATHTripleBufferEnabler::pre_update_no_lock,
                       boost::bind(&WRATHLayerNodeValuePackerBase::pack_data, this));
}


WRATHLayerNodeValuePackerBase::
~WRATHLayerNodeValuePackerBase()
{
  for(std::vector<per_packer_datum*>::const_iterator 
        iter=m_packers.begin(), 
        end=m_packers.end(); 
      iter!=end; ++iter)
    { 
      per_packer_datum *ptr(*iter);
      WRATHDelete(ptr);
    }
}

void
WRATHLayerNodeValuePackerBase::
on_place_on_deletion_list(void)
{
  m_sim_signal.disconnect();
}


void 
WRATHLayerNodeValuePackerBase::
assign_slot(int slot, WRATHLayerItemNodeBase* h, int highest_slot)
{
  WRATHAutoLockMutex(m_nodes_mutex);

  WRATHassert( (h==NULL) xor (m_nodes[slot]==NULL));
  m_nodes[slot]=h;
  m_highest_slot=highest_slot;
}

WRATHLayerNodeValuePackerBase::DataToGL
WRATHLayerNodeValuePackerBase::
data_to_gl(GLenum shader_stage)
{
  std::map<GLenum, int>::iterator iter;

  iter=m_packers_by_shader.find(shader_stage);
  return (iter!=m_packers_by_shader.end())?
    DataToGL(m_packers[iter->second]):
    DataToGL(&m_empty_packer);
}

WRATHLayerNodeValuePackerBase::DataToGL
WRATHLayerNodeValuePackerBase::
data_to_gl_indexed(unsigned int idx)
{
  return (idx<m_packers.size())?
    DataToGL(m_packers[idx]):
    DataToGL(&m_empty_packer);
}

void
WRATHLayerNodeValuePackerBase::
pack_data(void)
{
  /*
    TODO: should we triple buffer m_nodes
    instead to avoid the lock? Doing
    that though means that the array
    gets copied... alot. 
   */
  WRATHAutoLockMutex(m_nodes_mutex);
  int number_slots;



  number_slots=1 + m_highest_slot;
  m_number_slots_to_pack_to_GL[triple_buffer_enabler()->current_simulation_ID()]=number_slots;


  for(std::vector<per_packer_datum*>::const_iterator 
        iter=m_packers.begin(), end=m_packers.end(); iter!=end; ++iter)
    {
      per_packer_datum *ptr(*iter);
      ptr->pack_data(number_slots);
    }
}

int
WRATHLayerNodeValuePackerBase::
number_slots_to_pack_to_GL(void)
{
  int I(triple_buffer_enabler()->present_ID());
  return m_number_slots_to_pack_to_GL[I];
}

