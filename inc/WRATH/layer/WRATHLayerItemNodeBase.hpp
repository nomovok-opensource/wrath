/*! 
 * \file WRATHLayerItemNodeBase.hpp
 * \brief file WRATHLayerItemNodeBase.hpp
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


#ifndef WRATH_HEADER_LAYER_ITEM_NODE_BASE_HPP_
#define WRATH_HEADER_LAYER_ITEM_NODE_BASE_HPP_

#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include <stdint.h>
#include <vector>
#include "reorder_c_array.hpp"
#include "WRATHTripleBufferEnabler.hpp"
#include "WRATHLayerNodeValuePackerBase.hpp"
#include "WRATHBrush.hpp"

/*! \addtogroup Layer
 * @{
 */


/*!\class WRATHLayerItemNodeBase
  A WRATHLayerItemNodeBase is a base class to hold 
  "any" kind of per item node data an implementation
  of WRATHLayerBase requires. Implementations of
  WRATHLayerBase will expect a class derived from
  WRATHLayerItemNodeBase for their subkey values.

  The class WRATHLayerItemNodeBase is not thread
  safe! Manipulation or querying of any data of
  a WRATHLayerItemNodeBase must be done in the 
  simulation thread. In particular, deletion and 
  construction of objects may only occur from the 
  simulation thread. The structure of WRATHLayerBase
  and WRATHLayerItemNodeBase are so that one can
  freely change the local values of a WRATHLayerItemNodeBase
  from the simulation thread, and those values will
  find themselves safely triple buffer copied for
  the rendering thread to consume. This triple buffer
  copying is done by extract_values() which is only
  executed from the simulation thread.

  A WRATHLayerItemNodeBase knows it's root node,
  and that root node knows if some portion of
  the heirarchy is dirty (values within nodes
  or child-parent relationships). Walking the
  heirarchy each time one needs a value relative
  to the root, even lazily (i.e checking if the
  heirarchy is dirty and only walking if it dirty)
  is still quite slow and quite heavy in cache misses.
  To combat this, whenever a WRATHLayerItemNodeBase is
  parentless it connects to the WRATHTripleBufferEnabler
  signal of when a simulation frame ends (i.e.
  see WRATHTripleBufferEnabler::connect(), with
  parameters WRATHTripleBufferEnabler::on_complete_simulation_frame
  and WRATHTripleBufferEnabler::pre_update_lock. Since
  the global values are needed by other functions
  the gp_order field is set to \ref HierarchyNodeWalk
  which is a large negative value.  
 */
class WRATHLayerItemNodeBase
{
public:
  /*!
    Because checking if the hierarhy is dirty at each
    access of values that are affected by the hierarchy
    can trigger cache misses, each root WRATHLayerItemNodeBase
    is connected to walk the hierarchy in the simulation thread
    just before the simulation ID is updated (i.e. the arguments
    to WRATHTripleBufferEnabler::connect() are \ref
    WRATHTripleBufferEnabler::on_complete_simulation_frame
    and \ref WRATHTripleBufferEnabler::pre_update_no_lock).

    However, that hierarchy walk must happen before trying to use
    those values. The default group order to passed
    to \ref WRATHTripleBufferEnabler::connect() is 
    \ref HierarchyNodeWalk; this value however can be changed
    by the method hierarchy_walk_group_order().
  */
  static const int HierarchyNodeWalk=-32000;

  enum
    {
      /*!
        Enumeration indicating number of per-node values
        consumed. All classes derived from WRATHLayerItemNodeBase
        must redefine this enumeration to indicate how many
        per-node values they send to OpenGL.
       */
      number_per_node_values=0
    };

  /*!\typedef parent_changed_signal_t
    Signal type of signal emitted when the parent changes,
    signal is emitted AFTER the parent has changed
    and the argument to the signal is the OLD parent.
   */
  typedef boost::signals2::signal<void (WRATHLayerItemNodeBase*)> parent_changed_signal_t;
  
  /*!\class node_function_packet
    A provides an interface for functions that depend
    on the node type but not on a node itself.
   */
  class node_function_packet:boost::noncopyable
  {
  public:
    virtual
    ~node_function_packet() {}
    
    /*!\fn create_completely_clipped_node
      To be implemented by a derived class to 
      create and return a WRATHLayerItemNodeBase
      object which will have an item's contents
      clipped completely. In implementing 
      create_completely_clipped_node() for a 
      node class, it is NOT required that the
      node type returned is the same node type.
      In particular it is not necessary that
      the return value of its functions() method
      (see WRATHLayerItemNodeBase::functions()).
      This is because the returned node has none
      of it's methods called. What is required 
      is that the node works with the same 
      shaders in such a way to make sure that
      primitives are completely clipped.
      They typical pattern is that node types
      defining the transformation type will
      implement this method and derived types
      that augment a transformation node type
      will rely on the transformation node
      types implementation of 
      create_completely_clipped_node().
    */
    virtual
    WRATHLayerItemNodeBase*
    create_completely_clipped_node(const WRATHTripleBufferEnabler::handle&) const=0;

    /*!\fn add_per_node_values
      To be implemented by a derived class to append to
      the needed per-node values.
      \param spec [output] location to which to appen per-item uniform
      \param available [input] functor object that specifies for each shader
                       stage if per-item value fetching is possible
    */
    virtual
    void
    add_per_node_values(WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                          const WRATHLayerNodeValuePackerBase::function_packet &available) const=0;

    /*!\fn append_shader_source
      To be implemented by a derived class to append
      to different shaders stages the GLSL code
      associated to the node type (typically 
      the transformation and clipping code).
      \param src [output] location to which to place shader source code, keyed by shader type
      \param available [input] functor object that specifies for each shader
                       stage if per-item value fetching is possible
     */
    virtual
    void
    append_shader_source(std::map<GLenum, WRATHGLShader::shader_source> &src,
                         const WRATHLayerNodeValuePackerBase::function_packet &available) const=0;
    
  };

  /*!\fn WRATHLayerItemNodeBase(WRATHLayerItemNodeBase*)
    Ctor. 
    \param parent pointer to parent of the created 
                  WRATHLayerItemNodeBase. The parent
                  owns the created object, must
                  NOT be NULL.
   */
  explicit
  WRATHLayerItemNodeBase(WRATHLayerItemNodeBase* parent);

  /*!\fn WRATHLayerItemNodeBase(const WRATHTripleBufferEnabler::handle&)
    Ctor. Creates a root WRATHLayerItemNodeBase.
    \param r handle to a WRATHTripleBufferEnabler to
             which the users of the created object will
             sync.
   */
  explicit
  WRATHLayerItemNodeBase(const WRATHTripleBufferEnabler::handle &r);

  virtual
  ~WRATHLayerItemNodeBase();

  /*!\fn const WRATHTripleBufferEnabler::handle& triple_buffer_enabler  
    Returns a handle to the WRATHTripleBufferEnabler that this
    object uses for triple buffer coordinating.
  */
  const WRATHTripleBufferEnabler::handle& 
  triple_buffer_enabler(void) const
  {
    return m_tr;
  }
  
  /*!\fn WRATHTripleBufferEnabler::connect_t connect  
    Provided as a conveniance, equivalent to
    \code
    triple_buffer_enabler()->connect(tp, tm, subscriber, gp_order)
    \endcode
    see \ref WRATHTripleBufferEnabler::connect()
  */
  WRATHTripleBufferEnabler::connect_t
  connect(enum WRATHTripleBufferEnabler::signal_type tp,
          enum WRATHTripleBufferEnabler::signal_time tm,
          const WRATHTripleBufferEnabler::signal_t::slot_type &subscriber,
          int gp_order=0) const
  {
    return m_tr->connect(tp, tm, subscriber, gp_order);
  }

  /*!\fn void schedule_rendering_action   
    Provided as a conveniance, equivalent to
    \code
    triple_buffer_enabler()->schedule_rendering_action(v)
    \endcode
    see \ref WRATHTripleBufferEnabler::schedule_rendering_action>()
    \tparam T functor object providing operator() to execute its action
    \param v action to schedule
  */
  template<typename T>
  void
  schedule_rendering_action(const T &v) const
  {
    m_tr->schedule_rendering_action<T>(v);
  }
  
  /*!\fn void schedule_simulation_action  
    Provided as a conveniance, equivalent to
    \code
    triple_buffer_enabler()->schedule_simulation_action(v)
    \endcode
    see \ref WRATHTripleBufferEnabler::schedule_simulation_action()
    \tparam T functor object providing operator() to execute its action
    \param v action to schedule
  */
  template<typename T>
  void
  schedule_simulation_action(const T &v) const
  {
    m_tr->schedule_simulation_action<T>(v);
  }
  
  /*!\fn WRATHLayerItemNodeBase* parent_base(void) const
    Returns the parent of this object
   */
  WRATHLayerItemNodeBase*
  parent_base(void) const
  {
    return m_parent;
  }
  
  /*!\fn WRATHLayerItemNodeBase* root_base
    Returns the root node of this object
   */
  WRATHLayerItemNodeBase*
  root_base(void) const
  {
    return m_root;
  }

  /*!\fn bool have_children
    Returns true if and only if this
    node has any child nodes.
   */
  bool
  have_children(void) const
  {
    return !m_children.empty();
  }

  /*!\fn WRATHLayerItemNodeBase* previous_sibling_base
    Returns the sibling previous to this
    in it's parent listing. If this is the
    first child of it's parent or if it
    has no parent, returns NULL.
   */
  WRATHLayerItemNodeBase*
  previous_sibling_base(void) const;

  /*!\fn WRATHLayerItemNodeBase* next_sibling_base
    Returns the sibling following to this
    in it's parent listing. If this is the
    last child of it's parent or if it
    has no parent, returns NULL.
   */
  WRATHLayerItemNodeBase*
  next_sibling_base(void) const;

  /*!\fn void call_recurse_base
    Given a functor F that takes as argument
    a pointer to a T*, call that
    functor F on this WRATHLayerItemNodeBase
    and every _descendant_ WRATHLayerItemNodeBase.
    It is an error if this WRATHLayerItemNodeBase
    cannot be dynamic casted to T or if any
    child of this WRATHLayerItemNodeBase 
    cannot be dynamic casted to T.
    \tparam F functor object providing operator()(T*) method to execute its action
    \tparam T WRATHLayerItemNodeBase derived type
    \param f Functor to act on this and every 
             descendant of this
   */
  template<typename F, typename T>
  void
  call_recurse_base(const F &f)
  {
    WRATHassert(dynamic_cast<T*>(this)!=NULL);

    f(dynamic_cast<T*>(this));
    for(std::list<WRATHLayerItemNodeBase*>::iterator 
          iter=m_children.begin(), end=m_children.end(); 
        iter!=end; ++iter)
      {
        (*iter)->call_recurse_base<F, T>(f);
      }
  }

  /*!\fn void call_for_each_child
    Given a functor F that takes as argument
    a pointer to a T*, call that
    functor F on this WRATHLayerItemNodeBase
    and every child WRATHLayerItemNodeBase.
    It is an error if this WRATHLayerItemNodeBase
    cannot be dynamic casted to T or if any
    child of this WRATHLayerItemNodeBase 
    cannot be dynamic casted to T.
    \tparam F functor object providing operator()(T*) method to execute its action
    \tparam T WRATHLayerItemNodeBase derived type
    \param f Functor to act on this and every 
             child of this
   */
  template<typename F, typename T>
  void
  call_for_each_child(const F &f)
  {
    WRATHassert(dynamic_cast<T*>(this)!=NULL);

    f(dynamic_cast<T*>(this));
    for(std::list<WRATHLayerItemNodeBase*>::iterator 
          iter=m_children.begin(), end=m_children.end(); 
        iter!=end; ++iter)
      {
        WRATHLayerItemNodeBase *C;
        C=*iter;
        WRATHassert(dynamic_cast<T*>(C)!=NULL);
        f(dynamic_cast<T*>(C));
      }
  }

  /*!\fn enum return_code parent_base(WRATHLayerItemNodeBase *)
    Sets the parent of this object.
    Will fail if the new parent is a 
    child of this or if p's 
    WRATHTripleBufferEnabler object
    is different than this's 
    WRATHTripleBufferEnabler object
    (see \ref WRATHTripleBufferEnabler::PhasedDeletedObject::triple_buffer_enabler)
    \param p pointer to new parent node. A value of NULL
             is acceptable and indicates that this node
             will then become a root node.
   */
  enum return_code
  parent_base(WRATHLayerItemNodeBase *p);

  /*!\fn boost::signals2::connection connect_parent_changed
    Connect to the signal fired when the parent changes.
    The signal is fired _AFTER_ the parent is changed.
    Note that removal of a child from a node P does NOT
    break the sorting of the internal child list P
    (this is because the child list is a linked list).
    \param slot action to execute each sime signal is fired
   */
  boost::signals2::connection
  connect_parent_changed(const parent_changed_signal_t::slot_type &slot)
  {
    return m_parent_changed_signal.connect(slot);
  }

  /*!\fn void extract_values
    To be implemented by a derived class to
    extract the values to be fed to GL associated
    to the node. The values are to be an
    array of floating numbers. The function
    is only called from the simulation thread, thus
    it is safe to walk the hierarhy, etc.
    What values to write and where are determined
    by the \ref WRATHLayerItemNodeBase::node_function_packet object
    returned by node_functions().
    \param out_value location to which to write the values
                     for extraction.
   */
  virtual
  void
  extract_values(reorder_c_array<float> out_value)=0;

  /*!\fn bool compare_children
    To be optionally implemented to compare
    child object to sort the order in which
    the children are traversed. An implementation
    can assume that the both pointers passed
    are non-NULL. The default implementation 
    compares the passed pointer values.
    \param lhs left side of comparison
    \param rhs right side of comparison
   */
  virtual
  bool
  compare_children(const WRATHLayerItemNodeBase *lhs, 
                   const WRATHLayerItemNodeBase *rhs) const
  {
    return lhs<rhs;
  }
  
  /*!\fn const node_function_packet& functions
    An implementation of WRATHLayerItemNodeBase
    MUST implement a -static- function
    with the signature
    \code
    const node_function_packet& functions(void);
    \endcode
    Additionally, the implementation of
    \ref node_functions() must return the exact
    same reference value as the static function
    functions(). The function declared in the
    header file DOES not have an implementation
    and thus calling WRATHLayerItemNodeBase::functions()
    results in a link error.
   */
  static
  const node_function_packet&
  functions(void);

  /*!\fn const node_function_packet& node_functions()
    To be implemented by a derived class to 
    return the \ref WRATHLayerItemNodeBase::node_function_packet object
    associated to the node type. An implementation
    must also implement a _static_ method
    functions(void) which returns the same
    reference value as node_functions().
   */
  virtual
  const node_function_packet&
  node_functions(void) const=0;

  /*!\fn void hierarchy_walk_group_order(int).
    Sets the group order of when the
    heirarchy tree of which this
    WRATHLayerItemNodeBase belongs to 
    be walked. The value is clamped
    to be negative, i.e. passing a non-negative
    values sets it as -1. The default
    value is \ref HierarchyNodeWalk
    \param v group order for the heirarchy walk.
   */
  void
  hierarchy_walk_group_order(int v)
  {
    m_root->hierarchy_walk_group_order_implement(std::min(-1, v));
  }
 
  /*!\fn void walk_hierarchy_if_necessary(void) const
    Walks the hierarchy starting at the root node 
    if there is an element of the hierarchy 
    marked as dirty.
   */
  void
  walk_hierarchy_if_necessary(void) const
  {
    m_root->root_walk();
  }

  /*!\fn bool hierarchy_dirty
    Returns true if and only if the hierarchy
    is marked dirty. Calling walk_hierarchy_if_necessary()
    walks the hierarchy and clears the flag.
    However, one should rely on that the 
    hierarchy is walked at the end of each 
    simulation frame.
   */
  bool
  hierarchy_dirty(void) const
  {
    return m_root->m_is_dirty;
  }

  /*!\fn void set_shader_brush
    For those node types that carry with their
    _type_ additional shader information
    (for example a \ref WRATHGradientSourceBase)
    that affects how to draw through a \ref
    WRATHShaderBrush, that type should re-implement
    set_brush() as a static method to do it's job
    and that implementation should call it's base
    class set_brush() method. The default implementation
    is to do nothing.
   */
  static
  void
  set_shader_brush(WRATHShaderBrush &)
  {}

  /*!\fn void set_from_brush
    For those nodes which have values that
    depend on the value in a brush,
    they can set those value in
    \ref set_from_brush(). Default implementation
    is to do nothing.
   */
  virtual
  void
  set_from_brush(const WRATHBrush&)
  {}

protected:

  /*!\fn compute_values
    To be implemented by a derived class to compute
    values that depend on internal state of the
    node and the state of it's parent node. The most
    common use case is computing hierarchical
    transformation values between node and root for a 
    node. It is an error to call mark_dirty() or to 
    change the hierarchy during the call to compute_values().
   */
  virtual
  void
  compute_values(void)=0;

  /*!\fn mark_dirty
    To be used by an implementation of WRATHLayerItemNodeBase
    to indicate that an internal state value has changed
    and that a heirarchy walk is necessary to get its 
    local values correct of the local values of any child.
    \param v if true, marks as dirty, if false does not change dirty flag
   */
  void
  mark_dirty(bool v=true)
  {
    WRATHassert(m_root!=NULL);
    m_root->m_is_dirty=m_root->m_is_dirty or v;
  }

  /*!\fn mark_child_ordering_dirty
    To be used by an implementation of WRATHLayerItemNodeBase
    to signal that the children need to be re-ordered.
    \param v if true, marks as dirty, if false does not change dirty flag
   */
  void
  mark_child_ordering_dirty(bool v=true)
  {
    m_child_order_is_dirty=m_child_order_is_dirty or v;
  }

  /*!\fn mark_dirty_and_child_ordering_dirty
    Provided as a conveniance, equivalent to
    \code
    mark_dirty(v);
    mark_child_ordering_dirty(v);
    \endcode
    \param v if true, marks as dirty, if false does not change dirty flags
   */
  void
  mark_dirty_and_child_ordering_dirty(bool v=true)
  {
    mark_dirty(v);
    mark_child_ordering_dirty(v);
  }

private:

  void
  root_walk(void);

  void
  walk_hierarchy(void);

  void
  add_child(WRATHLayerItemNodeBase*);

  void
  remove_child(WRATHLayerItemNodeBase*);

  void
  recurse_set_root(WRATHLayerItemNodeBase*);

  void
  hierarchy_walk_group_order_implement(int);

  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayerItemNodeBase *m_parent, *m_root;
  std::list<WRATHLayerItemNodeBase*> m_children;
  bool m_is_dirty, m_child_order_is_dirty;
  std::list<WRATHLayerItemNodeBase*>::iterator m_slot;

  WRATHTripleBufferEnabler::connect_t m_sig_walk;
  parent_changed_signal_t m_parent_changed_signal;
  int m_hierarchy_walk_group_order;
};



/*!\class WRATHLayerItemNodeBaseT
  When implementing WRATHLayerItemNodeBase derived
  class T, one should inherit from WRATHLayerItemNodeBaseT<T>.
  Class provides member function parent(T*), root(),
  previous_sibling() and next_sibling() correctly casting to T*
 */
template<typename T>
class WRATHLayerItemNodeBaseT:public WRATHLayerItemNodeBase
{
public:
  /*!\fn WRATHLayerItemNodeBaseT(T*)
    Ctor. 
    \param pparent pointer to parent of the created 
                   WRATHLayerItemNodeBaseT. The parent
                   owns the created object, must
                   NOT be NULL.
   */
  explicit
  WRATHLayerItemNodeBaseT(T* pparent):
    WRATHLayerItemNodeBase(pparent)
  {}

  /*!\fn WRATHLayerItemNodeBaseT(const WRATHTripleBufferEnabler::handle&)
    Ctor. Creates a root WRATHLayerItemNodeBaseT.
    \param r handle to a WRATHTripleBufferEnabler to
             which the users of the created object will
             sync.
   */
  explicit
  WRATHLayerItemNodeBaseT(const WRATHTripleBufferEnabler::handle &r):
    WRATHLayerItemNodeBase(r)
  {}

  /*!\fn T* parent(void) const
    Returns the parent of this object
   */
  T*
  parent(void) const
  {
    WRATHLayerItemNodeBase *q(this->parent_base());
    WRATHassert( (q!=NULL) xor (dynamic_cast<T*>(q)==NULL) );    
    return static_cast<T*>(q);
  }

  /*!\fn T* previous_sibling
    Returns the sibling previous to this
    in it's parent listing. If this is the
    first child of it's parent or if it
    has no parent, returns NULL.
   */
  T*
  previous_sibling(void) const
  {
    WRATHLayerItemNodeBase *q(this->previous_sibling_base());
    WRATHassert( (q!=NULL) xor (dynamic_cast<T*>(q)==NULL) );    
    return static_cast<T*>(q);
  }

  /*!\fn T* next_sibling
    Returns the sibling following to this
    in it's parent listing. If this is the
    last child of it's parent or if it
    has no parent, returns NULL.
   */
  T*
  next_sibling(void) const
  {
    WRATHLayerItemNodeBase *q(this->next_sibling_base());
    WRATHassert( (q!=NULL) xor (dynamic_cast<T*>(q)==NULL) );    
    return static_cast<T*>(q);
  }
  
  /*!\fn T* root
    Returns the root node of this object
   */
  T*
  root(void) const
  {
    WRATHLayerItemNodeBase *q(this->root_base());
    WRATHassert(dynamic_cast<T*>(q)!=NULL);
    return static_cast<T*>(q);
  }

  /*!\fn enum return_code parent(T*)
    Sets the parent of this object.
    Will fail if the new parent is a 
    child of this or if p's 
    WRATHTripleBufferEnabler object
    is different than this's 
    WRATHTripleBufferEnabler object
    (see \ref WRATHTripleBufferEnabler::PhasedDeletedObject::triple_buffer_enabler)
    \param p pointer to new parent node. A value of NULL
             is acceptable and indicates that this node
             will then become a root node.
   */
  enum return_code
  parent(T *p)
  {
    return this->parent_base(p);
  }
};


/*! @} */

#endif
