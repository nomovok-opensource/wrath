/*! 
 * \file WRATHLayerBase.hpp
 * \brief file WRATHLayerBase.hpp
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


#ifndef WRATH_HEADER_LAYER_BASE_HPP_
#define WRATH_HEADER_LAYER_BASE_HPP_

#include "WRATHConfig.hpp"
#include <vector>
#include <list>
#include <map>
#include <set>
#include <boost/utility.hpp>

#include "WRATHNew.hpp"
#include "WRATHUtil.hpp"
#include "WRATHRawDrawData.hpp"
#include "WRATHSlotAllocator.hpp"
#include "WRATHCanvas.hpp"


class WRATHLayerItemNodeBase;

/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerBase
  A WRATHLayerBase provides is a generic implementation of 
  WRATHCanvas. It's main purpose is to accellerate
  the creation of custom transformation and clipping heirarchy
  easily without needed to handle a number of the lower level
  (and common) detail oriented boilerplate code.

  The WRATHItemDrawState::m_drawer field of WRATHItemDrawState
  must point to a WRATHLayerBase::DrawerBase derived object.
 */
class WRATHLayerBase:public WRATHCanvas
{
public:
  /*!\class GLStateOfNodeCollection
    A GLStateOfNodeCollection is an interface to add to a
    GL state vector for the pupose of setting values
    extracted from a set of WRATHLayerItemNodeBase objects.
    This class defines the interface that \ref DrawerBase
    uses to append to a GL state vector to transmit
    data of a WRATHLayerItemNodeBase derived object into
    GL for shaders.
  */
  class GLStateOfNodeCollection:
    public WRATHTripleBufferEnabler::PhasedDeletedObject
  {
  public:
    /*!\fn GLStateOfNodeCollection
      Ctor.
      \param r handle to WRATHTripleBufferEnabler to which to sync
     */
    explicit
    GLStateOfNodeCollection(const WRATHTripleBufferEnabler::handle &r):
      WRATHTripleBufferEnabler::PhasedDeletedObject(r)
    {}
    
    virtual
    ~GLStateOfNodeCollection()
    {}
    
    /*!\fn void assign_slot
      To be implemented by a derived class to reserve
      the named slot for the passed WRATHLayerItemNodeBase 
      object. Passing NULL indicates that the slot is to be 
      freed and that the previous value at the slot may no 
      longer be in scope. An implementation may assume
      that assign_slot() will never turn a non-NULL value
      to a different non-NULL value AND may also assume
      it will not call assign_slot() passing NULL if that
      slot is already at NULL. It is assumed, however, that
      all slots are initialized as NULL.
      \param slot slot to which to assign h
      \param h node to which to assign the named slot
      \param highest_slot WRATHLayerBase internally tracks what slots are 
                          allocated and not. In addition, it also tracks
                          the highest slot ID allocated, it passes that value
                          as the last argument to assign_slot() so that a
                          \ref GLStateOfNodeCollection implementation can potentially
                          reduce the amount of data to transmit to GL.
      
    */
    virtual
    void
    assign_slot(int slot, WRATHLayerItemNodeBase* h, int highest_slot)=0;
    
    /*!\fn void append_state
      To be implemented by a derived class to append
      to the GL state vector that GL state needed
      to transmit the data of the nodes to GL.
      \param sk WRATHSubItemDrawState to which to append GL state
    */
    virtual
    void
    append_state(WRATHSubItemDrawState &sk)=0;
  };


  /*!\class GLStateOfLayer
    A GLStateOfLayer is an interface to transmit
    the state of a \ref WRATHLayerBase derived object
    to GL. 
  */
  class GLStateOfLayer:boost::noncopyable
  {
  public:
    
    virtual
    ~GLStateOfLayer()
    {}
    
    /*!\fn void append_state
      To be implemented by a derived class to create
      and then append to a WRATHSubItemDrawState 
      state whose value are from the state of 
      a WRATHLayerBase object. Examples such as 
      "current" matrix values [i.e. the appended GL 
      state query the WRATHLayerBase at _render_ time 
      to get the values to send to GL].
      \param layer WRATHLayerBase object from which to "get"
                   values during render. In particular the
                   object pointed to by layer is guaranteed
                   to be in scope whenever the uniforms
                   created and appended to sk are used
                   in a draw call
      \param sk WRATHSubItemDrawState to which to append 
                GL state
    */
    virtual
    void
    append_state(WRATHLayerBase *layer,
                 WRATHSubItemDrawState &sk) const=0;
  };
  

  /*!\class DrawerBase
    A DrawerBase is a drawer for draw groups
    of a WRATHLayerBase.  
   */
  class DrawerBase:public WRATHItemDrawer
  {
  public:

    /*!\typedef GLStateOfNodeCollection
      Conveniance typedef to WRATHLayerBase::GLStateOfNodeCollection
     */
    typedef WRATHLayerBase::GLStateOfNodeCollection GLStateOfNodeCollection;

    /*!\typedef GLStateOfLayer
      Conveniance typedef to WRATHLayerBase::GLStateOfNodeCollection
     */
    typedef WRATHLayerBase::GLStateOfLayer GLStateOfLayer;

    /*!\fn DrawerBase(WRATHMultiGLProgram*)
      Ctor.
      \param pr WRATHMultiGLProgram with which to draw
    */
    explicit
    DrawerBase(WRATHMultiGLProgram *pr):
      WRATHItemDrawer(pr)
    {}
    
    /*!\fn DrawerBase(WRATHMultiGLProgram*, const std::string&)
      Ctor.
      \param pr WRATHMultiGLProgram with which to draw
      \param presource_name resource name to give to created object
    */
    DrawerBase(WRATHMultiGLProgram *pr, 
               const std::string &presource_name):
      WRATHItemDrawer(pr, presource_name)
    {}
    
    virtual
    ~DrawerBase();
    
    /*!\fn void add_GLStateOfLayer
      A DrawBase object maintains a list of 
      GLStateOfLayer objects. Call this function
      to add an element to that list. Ownership
      of object added is set to this DrawerBase.
      \param obj pointer to GLStateOfLayer to add
    */
    void
    add_GLStateOfLayer(GLStateOfLayer *obj)
    {
      m_GLStateOfLayers.push_back(obj);
    }
    
    /*!\fn void append_GLStateOfLayers(WRATHLayerBase*, WRATHSubItemDrawState&)
      Essentially calls GLStateOfLayer::append_state()
      on sk for each \ref GLStateOfLayer object added
      in \ref add_GLStateOfLayer(). This method is
      used internally by WRATHLayerBase.
      \param layer WRATHLayerBase object
      \param sk WRATHSubItemDrawState key to have uniforms appended
    */
    void
    append_GLStateOfLayers(WRATHLayerBase *layer,
                           WRATHSubItemDrawState &sk);
    
    /*!\fn GLStateOfNodeCollection* allocate_node_packet(WRATHLayerBase*)
      To be implemented by a derived class to create
      a uniform_packed object for use in a draw group.
      \param layer WRATHLayerBase object requesting the packet
    */
    virtual
    GLStateOfNodeCollection*
    allocate_node_packet(WRATHLayerBase *layer) const=0;
    
    /*!\fn unsigned int number_slots
      To be implemented by a derived class to return
      the number of slots for nodes the WRATHMultiGLProgram
      of the DrawerBase object. A return value
      of 0 indicates that the WRATHMultiGLProgram is 
      so that the datum of a WRATHLayerItemNodeBase is 
      not used. In this case a WRATHLayerBase will set 
      the implicit attribute as follows:
      - implicit attribute=0 item is clipped, i.e. WRATHMultiGLProgram will clip the vertex
      - implicit attribute=1 item is not clipped, i.e. vertex is processed normally.
    */
    virtual
    unsigned int
    number_slots(void) const=0;
    
  private:
    std::vector<GLStateOfLayer*> m_GLStateOfLayers;
  };

  /*!\class SubKey
    The SubKeyBase derived type for a WRATHLayerBase
    is a SubKey. The contents of a SubKey is exactly
    a pointer to a WRATHLayerItemNodeBase. It is
    undefined behavior (read dereference wild pointers)
    to deallocate (via WRATHPhasedDelete) a
    WRATHLayerItemNodeBase while it is use by
    a DataHandle whose SubKeyBase object is a SubKey
    object whose contents is a pointer to said
    WRATHLayerItemNodeBase object
   */
  class SubKey:public SubKeyBase
  {
  public:
    /*!\fn SubKey(WRATHLayerItemNodeBase*)
      Ctor. Create a SubKey object
      \param p pointer to WRATHLayerItemNodeBase to which
               this SubKey refers.
     */
    SubKey(WRATHLayerItemNodeBase *p=NULL):
      m_node(p)
    {}

    /*!\fn SubKey(const SubKey&)
      Copy ctor.
     */
    SubKey(const SubKey &obj):
      m_node(obj.m_node)
    {}

    virtual
    SubKeyBase*
    create_copy(void) const
    {
      return WRATHNew SubKey(m_node);
    }

    /*!\var m_node
      WRATHLayerItemNodeBase to which this SubKey refers.
     */
    WRATHLayerItemNodeBase *m_node;
  };

  /*!\fn WRATHLayerBase
    Ctor. Construct a WRATHLayerBase.
    \param tr handle to a WRATHTripleBufferEnabler to
              which the users of the created object will
              sync. It is an error if the handle is not valid.
    \param sorter handle to WRATHDrawOrderComparer comparer
                  object which elements of created WRATHLayerBase
                  will be sorted (see \ref WRATHItemDrawState::m_force_draw_order)
                  An invalid handle indicates that the elements are
                  not sorted by a user defined sorting.
   */
  explicit
  WRATHLayerBase(const WRATHTripleBufferEnabler::handle &tr,
                 const WRATHDrawOrderComparer::handle sorter
                 =WRATHDrawOrderComparer::handle());

  
  virtual
  ~WRATHLayerBase();

  /*!\fn T* root_node
    Returns a root node of the specified type.
    The type T MUST be derived from 
    WRATHLayerItemNodeBase. In addition the
    the type must be constructable from 
    a handle to \ref WRATHTripleBufferEnabler
   */
  template<typename T>
  T*
  root_node(void);

  /*!\fn void for_each_root_node
    Performs an action for each root
    node that is derived from a specified type.
    The type T MUST be derived from 
    WRATHLayerItemNodeBase.
    \tparam T type to request the root node of
    \tparam F functor action taking a pointer to a type T
    \param action functor to apply to root node of type T
   */
  template<typename T, typename F>
  void
  for_each_root_node(type_tag<T>, F action);


  using WRATHCanvas::create;

  /*!\fn DataHandle create(const WRATHAttributeStore::handle &,
                           const WRATHCompiledItemDrawStateCollection &,
                           WRATHLayerItemNodeBase*, int)
    Provided as a conveniance, equivalent
    \code
    SubKey K(pNode);
    return create(attrib_store, pkey, K, sub_slot);
    \endcode
    \param attrib_store handle to attribute store from which
                        the returned DataHandle will allocate
                        attributes
    \param pkey WRATHItemDrawState specifying the GL state vector
    \param pNode item node
    \param sub_slot Specifies what value to pass to
                    WRATHAttributeStore::implicit_attribute_data()
                    where the implicit attribute data is to
                    be stored.
   */
  DataHandle
  create(const WRATHAttributeStore::handle &attrib_store,
         const WRATHCompiledItemDrawStateCollection &pkey,
         WRATHLayerItemNodeBase *pNode,
         unsigned int sub_slot=0);

  /*!\fn WRATHDrawOrderComparer::handle& sorter
    Returns the sort object used for elements of this
    WRATHLayerBase, see \ref WRATHItemDrawState::m_force_draw_order.
   */
  const WRATHDrawOrderComparer::handle&
  sorter(void)
  {
    return m_sorter;
  }
  
  virtual
  bool
  accepts_subkey(const SubKeyBase &psubkey)
  {
    return dynamic_cast<const SubKey*>(&psubkey)!=NULL;
  }
  
  virtual
  void
  add_raw_draw_command(WRATHDrawType pass,
                       WRATHRawDrawDataElement *b);

  virtual
  void
  release_group(DataHandle &g);

protected:

  virtual
  void
  on_place_on_deletion_list(void);

  /*!\fn const vecN<std::map<int, WRATHRawDrawData*>, 4>& render_raw_datas(void) const
    Returns the WRATHRawDrawData keyed by WRATHDrawType.
    The returned map is only modified from the rendering thread
    and should only be used from the rendering thread.
   */
  const vecN<std::map<int, WRATHRawDrawData*>, 4>&
  render_raw_datas(void) const
  {
    return m_render_raw_datas;
  }

  /*!\fn const std::map<int, WRATHRawDrawData*>& render_raw_datas(enum WRATHDrawType::draw_type_t) const
    Provided as a conveniance, equivalent to
    \code
    render_raw_datas()[tp]
    \endcode
    \param tp WRATHDrawType::draw_type_t selection                                                
   */
  const std::map<int, WRATHRawDrawData*>&
  render_raw_datas(enum WRATHDrawType::draw_type_t tp) const
  {
    return m_render_raw_datas[tp];
  }

  virtual 
  DataHandle
  create_implement(const WRATHAttributeStore::handle &attrib_store,
                   const WRATHCompiledItemDrawStateCollection &pkey,
                   const SubKeyBase &psubkey,
                   unsigned int sub_slot)
  {
    const SubKey *sub_key;
    
    sub_key=dynamic_cast<const SubKey*>(&psubkey);
    WRATHassert(sub_key!=NULL);
    return create(attrib_store, pkey,
                  sub_key->m_node, sub_slot);
                        
  }

private:

  
  class value_type;
  class MetaGroupBase;
  class MetaGroup;

  
  typedef WRATHInterleavedAttributes<GLubyte> NodeIndexAttribute;

  class CustomData:public CustomDataBaseT<NodeIndexAttribute>
  {
  public:
    CustomData(GLubyte pslot, 
               WRATHLayerItemNodeBase *p,
               MetaGroup *mg):
      m_subkey(p),
      m_meta(mg)
    {
      slot()=pslot;
    }

    CustomData(const CustomData &obj):
      m_subkey(obj.m_subkey.m_node),
      m_meta(obj.m_meta)
    {
      slot()=obj.slot();
    }

    GLubyte&
    slot(void)
    {
      return m_value.get<0>();
    }

    GLubyte
    slot(void) const
    {
      return m_value.get<0>();
    }

    virtual
    const SubKeyBase*
    subkey(void) const
    {
      return &m_subkey;
    }

    virtual
    void
    set_implicit_attribute_data(const_c_array< c_array<NodeIndexAttribute> > R) const;

    bool
    operator<(const CustomData &obj) const;

    NodeIndexAttribute m_value;
    SubKey m_subkey;
    MetaGroup *m_meta;
  };


  class MetaGroupBase:boost::noncopyable
  {
  public:
    MetaGroupBase(const WRATHAttributeStore::handle &attr_store, 
                  unsigned int implicit_slot,
                  const_c_array<WRATHCompiledItemDrawState> draw_state,
                  WRATHLayerBase *player);

    virtual
    ~MetaGroupBase();
    
    WRATHItemGroup *m_main_group;
    std::vector<GLStateOfNodeCollection*> m_node_gl;
    int m_use_count;
    unsigned int m_number_slots;
    std::vector<WRATHItemGroup::DrawCall> m_main_group_specs;
  };

  /*
    main purpose of MetaGroupBase is so that 
    the number of slots can be "computed" in a ctor
   */
  class MetaGroup:private MetaGroupBase
  {
  public:
    MetaGroup(const WRATHAttributeStore::handle &attr_store, 
              unsigned int implicit_slot,
              const std::vector<WRATHCompiledItemDrawState> &draw_state,
              value_type *v, WRATHLayerBase *player,
              WRATHLayerItemNodeBase *non_visible_node);

    ~MetaGroup();

    WRATHItemGroup*
    item_group(const std::vector<WRATHDrawOrder::const_handle> &force_draw_orders);

    int
    slot_location(WRATHLayerItemNodeBase *v);

    bool
    slot_allocated_for_node(WRATHLayerItemNodeBase *v);

    bool
    has_slots_available(void);

    int
    add_element(WRATHLayerItemNodeBase*);

    void
    remove_element(WRATHLayerItemNodeBase*);

    void
    increment_use_count(void)
    {
      ++m_use_count;
    }

    void
    decrement_use_count(void)
    {
      --m_use_count;
      WRATHassert(m_use_count>=0);
    }

    bool
    in_use(void)
    {
      WRATHassert(m_use_count>=0);
      return m_use_count>0;
    }

    int
    number_slots(void)
    {
      return m_number_slots;
    }

    void
    skip_bookkeeping_cleanup(void);
    
  private:
    typedef std::vector<WRATHDrawOrder::const_handle> item_key;
    typedef std::map<item_key, WRATHItemGroup*> item_map;

    WRATHItemGroup*
    fetch_item_group(const std::vector<WRATHDrawOrder::const_handle> &force_draw_orders);

    WRATHSlotAllocator<WRATHLayerItemNodeBase*> m_slot_allocator;
    value_type *m_value;
    WRATHLayerItemNodeBase *m_non_visible_node;
    item_map m_item_groups; 
    WRATHBufferAllocator *m_shared_index_buffer;
  };

  

  class value_type
  {
  public:
    std::map<WRATHLayerItemNodeBase*, MetaGroup*> m_has;
    std::set<MetaGroup*> m_has_free_slots;

    void
    purge_meta_group_nolock(MetaGroup*,
                            const WRATHSlotAllocator<WRATHLayerItemNodeBase*>::map_type&,
                            WRATHLayerItemNodeBase *exclude);
  };



  typedef boost::tuple<WRATHAttributeStore::handle, 
                       unsigned int, 
                       std::vector<WRATHCompiledItemDrawState> > key_type;
  typedef std::map<key_type, value_type> map_type;
                       
  DataHandle
  create_no_lock(const WRATHAttributeStore::handle &attr_store, 
                 unsigned int implicit_slot,
                 const WRATHCompiledItemDrawStateCollection &draw_state,
                 WRATHLayerItemNodeBase *node);

  void
  release_group_no_lock(DataHandle &g);

  WRATHRawDrawData*
  fetch_raw_data_nolock(WRATHDrawType);

  void
  add_raw_draw_data_to_array(WRATHDrawType, WRATHRawDrawData*);

  typedef WRATHUtil::TypeInfoSortable type_info_key;

  WRATHDrawOrderComparer::handle m_sorter;

  WRATHMutex m_mutex;
  map_type m_map;
  std::set<MetaGroup*> m_meta_groups;
  vecN<std::map<int, WRATHRawDrawData*>, WRATHDrawType::number_draw_types> m_raw_datas;
  std::map<CustomData, int> m_custom_data_objs;

  /*
    this map is "essentially" the same as m_raw_datas
    but is only accessed and modified in the rendering thread.
   */
  vecN<std::map<int, WRATHRawDrawData*>, WRATHDrawType::number_draw_types> m_render_raw_datas;

  /*
   */
  WRATHMutex m_roots_mutex;
  std::map<type_info_key, WRATHLayerItemNodeBase*> m_roots;
};


/*! @} */

template<typename T, typename F>
void
WRATHLayerBase::
for_each_root_node(type_tag<T>, F action)
{
  WRATHAutoLockMutex(m_roots_mutex);

  for(std::map<type_info_key, WRATHLayerItemNodeBase*>::iterator iter=m_roots.begin(),
        end=m_roots.end(); iter!=end; ++iter)
    {
      T *ptr;
      ptr=dynamic_cast<T*>(iter->second);
      if(ptr!=NULL)
        {
          action(ptr);
        }
    }
}

template<typename T>
T*
WRATHLayerBase::
root_node(void)
{
  WRATHAutoLockMutex(m_roots_mutex);

  std::map<type_info_key, WRATHLayerItemNodeBase*>::iterator iter;

  iter=m_roots.find(typeid(T));
  if(iter!=m_roots.end())
    {
      T *v;

      v=dynamic_cast<T*>(iter->second);
      WRATHassert(v!=NULL);

      return v;
    }

  T *v;

  v=WRATHNew T(this->triple_buffer_enabler());
  m_roots[typeid(T)]=v;

  return v;
  
}


#endif
