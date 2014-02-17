/*! 
 * \file WRATHLayerItemNodeDepthOrder.hpp
 * \brief file WRATHLayerItemNodeDepthOrder.hpp
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


#ifndef WRATH_HEADER_LAYER_ITEM_NODE_DEPTH_ORDER_HPP_
#define WRATH_HEADER_LAYER_ITEM_NODE_DEPTH_ORDER_HPP_

#include "WRATHConfig.hpp"
#include "WRATHLayerItemNodeBase.hpp"


/*! \addtogroup Layer
 * @{
 */

/*!\namespace WRATHLayerItemNodeDepthType
  Namespace to encapsulate global depth ordering
  from local depth ordering for \ref WRATHLayerItemNodeDepthOrder
 */
namespace WRATHLayerItemNodeDepthType
{
  /*!\enum depth_order_t
    Enumeration to specify the global depth ordering
    from local depth ordering for \ref WRATHLayerItemNodeDepthOrder
   */
  enum depth_order_t
    {
      /*!
        The global z-order value is the same
        as the local z-order vlaue, i.e.
        "flat"
       */
      flat_ordering,

      /*!
        The global z-order is computed so that:
        - parent is always under (i.e. comes before) each of its children
        - for each node N which has that WRATHLayerItemNodeT::previous_sibling() is non-NULL,
           \code
             N->previous_sibling()->global_z_order() < N->global_z_order() 
           \endcode
          is true
       */
      hierarchical_ordering
    };

  
}


#include "WRATHLayerItemNodeDepthOrderPrivate.tcc"


/*!\class WRATHLayerItemNodeDepthOrder
  A WRATHLayerItemNodeDepthOrder provides implementing z-order
  values for both flat and hierarchical z-ordering.
  \tparam pz_order_type enumeration dictating how global z-order is computed
  \tparam T a derived class S of WRATHLayerItemNodeDepthOrder should make the value of T
            S so that the parent() and root() methods return an S*
  \tparam pnormalizer_type normalizer type defining type for the underlying integer type
                           to hold the z-order value along with normalization functions
                           and range values. The template class \ref WRATHUtil::normalizer
                           provides exactly what is required.
 */
template<enum WRATHLayerItemNodeDepthType::depth_order_t pz_order_type, 
         typename T, 
         typename pnormalizer_type>
class WRATHLayerItemNodeDepthOrder:public WRATHLayerItemNodeBaseT<T>
{
public:
  /*!\var z_order_type
    Localize the z-ordering type template parameter
   */
  static const enum WRATHLayerItemNodeDepthType::depth_order_t z_order_type=pz_order_type;

  /*!\typedef normalizer_type
    Localize the normalize type template parameter
   */
  typedef pnormalizer_type normalizer_type;

  /*!\typedef global_z_order_type
    The type for the global z-order value. If \ref z_order_type 
    equals \ref WRATHLayerItemNodeDepthType::flat_ordering
    then is int, otherwise is normalizer_type::type
   */
  typedef typename WRATHLayerItemNodeDepthOrderPrivate::z_order_helper<z_order_type, T, normalizer_type>::global_z_order_type global_z_order_type;

  /*!\fn WRATHLayerItemNodeDepthOrder(T*)
    Ctor. 
    \param pparent pointer to parent of the created 
                   WRATHLayerItemNodeDepthOrder. The parent
                   owns the created object, must
                   NOT be NULL.
   */
  explicit
  WRATHLayerItemNodeDepthOrder(T* pparent):
    WRATHLayerItemNodeBaseT<T>(pparent),
    m_local_z(0)
  {
    m_z_order_helper.register_parent_changes(this);
  }

  /*!\fn WRATHLayerItemNodeDepthOrder(const WRATHTripleBufferEnabler::handle&)
    Ctor. Creates a root WRATHLayerItemNodeDepthOrder.
    \param r handle to a WRATHTripleBufferEnabler to
             which the users of the created object will
             sync.
   */
  explicit
  WRATHLayerItemNodeDepthOrder(const WRATHTripleBufferEnabler::handle &r):
    WRATHLayerItemNodeBaseT<T>(r),
    m_local_z(0)
  {
    m_z_order_helper.register_parent_changes(this);
  }

  /*!\fn int z_order(void) const
    Return the local z-order for this node.
    The local z-order is always stored as an
    int. Default value is 0.
   */
  int
  z_order(void) const
  {
    return m_local_z;
  }

  /*!\fn void z_order(int)
    Sets the local z-order for this node.
    The local z-order is always stored as an
    int. Default value is 0.
    \param v value to which to set the z-order
   */
  void
  z_order(int v)
  {
    if(v!=m_local_z)
      {
        m_local_z=v;
        m_z_order_helper.note_order_change(this);
      }
  }
  
  /*!\fn global_z_order_type global_z_order
    Returns the global z-order of the node.
    If the node z-ordering is flat, type is an
    int, otherwise type is normalizer_type::type.
   */
  global_z_order_type
  global_z_order(void) const
  {
    return m_global_z;
  }
  
  /*!\fn normalized_z
    Returns global_z_order() normalized
    by \ref normalizer_type to [-1,1].
   */
  float
  normalized_z(void) const
  {
    return m_normalized_z;
  }

  /*!\fn void global_z_order_consumes_slot
    Sets if this node "consumes" (see below)
    a slot in the global z-value assignment.
    For flat-z this function has no affect.
    Default value is true.

    For non-flat z-ordering, one can choose
    weather or not a node "consumes", i.e.
    decrements a counter when having its
    global z-value assigned. This is required
    whenever the node is used directly by an 
    item. However, for nodes that are used as 
    only transformation purposes (i.e. no item
    is used by them), they do not need to consume
    a z-value. 
    \param v if false node will not consume, if true
                node will consume. Default value
                is true
   */
  void
  global_z_order_consumes_slot(bool v)
  {
    m_z_order_helper.global_z_order_consumes_slot(v);
  }

  /*!\fn void global_start_z
    Sets the starting z-order value to use 
    for the hierarchy to which this node belongs.
    For flat-z this function has no affect.
    Recall that the back most node is to have
    the largest value for global_z_order().
    Default value is given by normalizer_type::max_value - 1,
    see also \ref WRATHUtil::normalizer::max_value.    
   */ 
  void
  global_start_z(global_z_order_type v)
  {
    this->mark_dirty(this->root()->m_z_order_helper.global_start_z(v));
  }

  /*!\fn bool compare_children
    Implement WRATHLayerItemNodeBase::compare_children()
    \param lhs left side of comparison operation 
    \param rhs right side of comparison operation 
   */
  virtual
  bool
  compare_children(const WRATHLayerItemNodeBase *lhs, 
                   const WRATHLayerItemNodeBase *rhs) const
  {
    /*
      the global z_order is more negative values infront, 
      for example: -100 infront of 0 which is infront of 100.
      We also want that the children of a node are drawn infront
      of the node too. The walk of the node structure is the following:
      1) First the parent
      2) Then the children in the order they are stored.
      So we store the children in "draw" order, thus more negative
      z-order is last, hence it is reverse sorted:
     */
    const WRATHLayerItemNodeDepthOrder *plhs(static_cast<const WRATHLayerItemNodeDepthOrder*>(lhs));
    const WRATHLayerItemNodeDepthOrder *prhs(static_cast<const WRATHLayerItemNodeDepthOrder*>(rhs));
    return plhs->z_order() > prhs->z_order();
  }

protected:

  /*!\fn compute_z_value
    A class that implements WRATHLayerItemNodeDepthOrder
    must call compute_z_value() in it's implementation
    of WRATHLayerItemNodeBase::compute_values() to
    properly set the global z-value.
   */
  void
  compute_z_value(void)
  {
    m_z_order_helper.compute_z_value(this);
    m_normalized_z=normalizer_type::signed_normalize(m_global_z);
  }

private:
  friend class WRATHLayerItemNodeDepthOrderPrivate::z_order_helper<z_order_type, T, normalizer_type>;

  int m_local_z;
  global_z_order_type m_global_z;
  float m_normalized_z;
  WRATHLayerItemNodeDepthOrderPrivate::z_order_helper<z_order_type, T, normalizer_type> m_z_order_helper;
};

/*! @} */


#endif
