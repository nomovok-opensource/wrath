/*! 
 * \file WRATHLayerItemNodeRotateTranslate.hpp
 * \brief file WRATHLayerItemNodeRotateTranslate.hpp
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


#ifndef WRATH_HEADER_LAYER_ITEM_NODE_ROTATE_TRANSLATE_HPP_
#define WRATH_HEADER_LAYER_ITEM_NODE_ROTATE_TRANSLATE_HPP_

#include "WRATHConfig.hpp"
#include <limits>
#include "vectorGL.hpp"
#include "WRATH2DRigidTransformation.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHLayerItemNodeDepthOrder.hpp"
#include "WRATHLayer.hpp"
#include "WRATHLayerIntermediateTransformation.hpp"



/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerItemNodeRotateTranslateValues
    The class data_type holds the parameters
    of a WRATHLayerItemNodeRotateTranslateT:
    - transformation (see \ref m_transformation)
    - visibility (see \ref m_visible)  
 */
class WRATHLayerItemNodeRotateTranslateValues
{
public:
  /*!\fn WRATHLayerItemNodeRotateTranslateValues
    Ctor, initializes \ref m_visible as true
    and \ref m_transformation as identity.
   */
  WRATHLayerItemNodeRotateTranslateValues(void):
    m_visible(true)
  {}
  
  /*!\var m_transformation
    Holds the transformation
  */
  WRATH2DRigidTransformation m_transformation;
  
  /*!\var m_visible
    Holds visibility
  */
  bool m_visible;

  /*!\fn void compose
    Sets the values of *this WRATHLayerItemNodeRotateTranslateValues
    as "parent * local".
    \param parent WRATHLayerItemNodeRotateTranslateValues LHS of composition operation
    \param local WRATHLayerItemNodeRotateTranslateValues RHS of composition operation
   */
  void
  compose(const WRATHLayerItemNodeRotateTranslateValues &parent,
          const WRATHLayerItemNodeRotateTranslateValues &local);

  /*!\fn void extract_values
    Extracts values from this WRATHLayerItemNodeRotateTranslateValues
    and places them into an array. The values are extracted as follows:
     - The tuple (WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_RX,
               WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_RY,
               WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_TX,
               WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_TY)
       holds the transformation from local to global coordinates packed 
       as WRATH2DRigidTransformation::value_as_vec4()

    - WRATH_LAYER_ROTATE_TRANSLATE_Z holds the value z-value passed. If \ref 
                                     m_visible is false, then the value  
                                     stored is instead extreme negative.
    \param out_value location to which to extract values
    \param z_order value to pack for WRATH_LAYER_ROTATE_TRANSLATE_Z
   */
  void
  extract_values(reorder_c_array<float> out_value, float z_order);

  /*!\fn const WRATHLayerItemNodeBase::node_function_packet& functions
    Function packet for the transformation node values
    in a WRATHLayerItemNodeRotateTranslateValues
   */
  static
  const WRATHLayerItemNodeBase::node_function_packet&
  functions(void);

  /*!\fn WRATHLayerIntermediateTransformation::handle create_pre_transformer
    Create a WRATHLayerIntermediateTransformation object
    that pre-applies the transformation of this 
    WRATHLayerItemNodeRotateTranslateValues. It is required
    that this WRATHLayerItemNodeRotateTranslateValues stays
    in scope for as long as the returned object has an
    active reference or until unhook() is called on it.
   */
  WRATHLayerIntermediateTransformation::handle
  create_pre_transformer(const WRATHTripleBufferEnabler::handle &tr);

  /*!\fn void unhook
    If the WRATHLayerItemNodeRotateTranslateValues object
    that created a WRATHLayerIntermediateTransformation
    object via create_pre_transformer() goes out of scope
    before the WRATHLayerIntermediateTransformation does,
    call unhook() to make the transformation be the identity
    and to not refer to the WRATHLayerItemNodeRotateTranslateValues
    that created it.
   */
  static
  void
  unhook(const WRATHLayerIntermediateTransformation::handle &h);
};




/*!\class WRATHLayerItemNodeRotateTranslateT
  A WRATHLayerItemNodeRotateTranslate implements
  WRATHLayerItemNodeBase by providing a translation
  and rotation data for a node. 
  \tparam pz_order_type enumeration dictating how global z-order is computed
  \tparam pnormalizer_type normalizer type providing the integer type for the z-order values
                          and normalization functions. The template class \ref
                          WRATHUtil::normalizer provides exactly what is required
                          for pnormalizer_type.
 */
template<enum WRATHLayerItemNodeDepthType::depth_order_t pz_order_type, 
         typename pnormalizer_type=WRATHUtil::normalizer<int16_t> >
class WRATHLayerItemNodeRotateTranslateT:
  public WRATHLayerItemNodeDepthOrder<pz_order_type, WRATHLayerItemNodeRotateTranslateT<pz_order_type, pnormalizer_type>, pnormalizer_type>
{
public:
  
  enum
    {
      /*!
        Enumeration indicating number of per-node values the node type has
       */
      number_per_node_values=5
    };
     
  /*!\fn WRATHLayerItemNodeRotateTranslateT(const WRATHTripleBufferEnabler::handle &)
    Ctor. Creates a root WRATHLayerItemNodeRotateTranslate.
    \param r handle to a WRATHTripleBufferEnabler to
             which the users of the created object will
             sync.
   */
  explicit
  WRATHLayerItemNodeRotateTranslateT(const WRATHTripleBufferEnabler::handle &r):
    WRATHLayerItemNodeDepthOrder<pz_order_type, WRATHLayerItemNodeRotateTranslateT<pz_order_type, pnormalizer_type>, pnormalizer_type >(r),
    m_compose_transformation_with_parent(true)
  {}

  /*!\fn WRATHLayerItemNodeRotateTranslateT(WRATHLayerItemNodeRotateTranslateT*)
    Ctor. 
    \param pparent pointer to parent of the created 
                   WRATHLayerItemNodeRotateTranslate. The parent
                   owns the created object, must
                   NOT be NULL.
   */
  explicit
  WRATHLayerItemNodeRotateTranslateT(WRATHLayerItemNodeRotateTranslateT *pparent):
    WRATHLayerItemNodeDepthOrder<pz_order_type, WRATHLayerItemNodeRotateTranslateT<pz_order_type, pnormalizer_type>, pnormalizer_type>(pparent),
    m_compose_transformation_with_parent(true)
  {}

  ~WRATHLayerItemNodeRotateTranslateT()
  {
    WRATHLayerItemNodeRotateTranslateValues::unhook(m_transformer);
  }

  /*!\fn bool visible(void) const
    Returns true if and only if this WRATHLayerItemNodeRotateTranslate
    is visible relative to it's parent, see \ref WRATHLayerItemNodeRotateTranslateValues::m_visible.
   */
  bool 
  visible(void) const
  {
    return m_values.m_visible;
  }

  /*!\fn void visible(bool)
    Set if this WRATHLayerItemNodeRotateTranslate
    is visible relative to it's parent, see 
    \ref WRATHLayerItemNodeRotateTranslateValues::m_visible.
   */
  void
  visible(bool v)
  {
    this->mark_dirty(v!=m_values.m_visible);
    m_values.m_visible=v;
  }

  /*!\fn bool compose_transformation_with_parent(void)
    Returns true if the transformation returned in
    \ref global_values() is composed with the 
    transformation of global_values() of parent().
    Default value is true.
   */
  bool
  compose_transformation_with_parent(void)
  {
    return m_compose_transformation_with_parent;
  }

  /*!\fn void compose_transformation_with_parent(bool)
    Sets if the transformation returned in
    \ref global_values() is composed with the 
    transformation of global_values() of parent().
    Default value is true.
    \param v new value to use
   */
  void
  compose_transformation_with_parent(bool v)
  {
    this->mark_dirty(v!=m_compose_transformation_with_parent);
    m_compose_transformation_with_parent=v;
  }

  /*!\fn const WRATH2DRigidTransformation& transformation(void) const
    Returns the node's transformation, 
    see \ref WRATHLayerItemNodeRotateTranslateValues::m_transformation
   */
  const WRATH2DRigidTransformation&
  transformation(void) const
  {
    return m_values.m_transformation;
  }

  /*!\fn void transformation(const WRATH2DRigidTransformation&)
    Set the node's transformation directly
    \param v new value to use
   */
  void
  transformation(const WRATH2DRigidTransformation &v)
  {
    m_values.m_transformation=v;
    this->mark_dirty();
  }

  /*!\fn const vec2& translation(void) const
    Returns the translation of the node's transformation, 
    see \ref WRATHLayerItemNodeRotateTranslateValues::m_transformation and
    \ref WRATH2DRigidTransformation::translation().
   */
  const vec2&
  translation(void) const
  {
    return m_values.m_transformation.translation();
  }

  /*!\fn void translation(const vec2&)
    Sets the translation of the node's transformation, 
    see \ref WRATHLayerItemNodeRotateTranslateValues::m_transformation and
    \ref WRATH2DRigidTransformation::translation(const vec2 &).
    \param v new value to use
   */
  void
  translation(const vec2 &v)
  {
    m_values.m_transformation.translation(v);
    this->mark_dirty();
  }

  /*!\fn const vec2& position(void) const
    Provided for readability, equivalent to
    \code
    translation()
    \endcode
   */
  const vec2&
  position(void) const
  {
    return translation();
  }

  /*!\fn void position(const vec2&)
    Provided for readability, equivalent to
    \code
    translation(v)
    \endcode
    \param v new value to use
   */
  void
  position(const vec2 &v) 
  {
    translation(v);
  }

  /*!\fn float scaling_factor(void) const
    Returns the scaling factor of the node's transformation, 
    see \ref WRATHLayerItemNodeRotateTranslateValues::m_transformation and
    \ref WRATH2DRigidTransformation::scale().
   */
  float
  scaling_factor(void) const
  {
    return m_values.m_transformation.scale();
  }

  /*!\fn void scaling_factor(float)
    Sets the scaling factor of the node's transformation, 
    see \ref WRATHLayerItemNodeRotateTranslateValues::m_transformation and
    \ref WRATH2DRigidTransformation::scale(float).
    \param v new value to use
   */
  void
  scaling_factor(float v) 
  {
    m_values.m_transformation.scale(v);
    this->mark_dirty();
  }

  /*!\fn void rotation(float)
    Sets the rotation of the node's transformation, 
    see \ref WRATHLayerItemNodeRotateTranslateValues::m_transformation and
    \ref WRATH2DRigidTransformation::rotation(float).
    \param angle_in_radians rotation angle in _RADIANS_
   */
  void
  rotation(float angle_in_radians) 
  {
    m_values.m_transformation.rotation(angle_in_radians);
    this->mark_dirty();
  }

  /*!\fn enum return_code rotation(const std::complex<float> &)
    Sets the rotation of the node's transformation, 
    see \ref WRATHLayerItemNodeRotateTranslateValues::m_transformation and
    \ref WRATH2DRigidTransformation::rotation(const std::complex<float> &).
    \param r rotation given as complex number, i.e. r=exp(i a) where a=angle in radians 
   */
  enum return_code
  rotation(const std::complex<float> &r) 
  {
    return m_values.m_transformation.rotation(r);
    this->mark_dirty();
  }

  /*!\fn const std::complex<float>& rotation(void) const
    Returns the rotation of the node's transformation, 
    see \ref WRATHLayerItemNodeRotateTranslateValues::m_transformation and
    \ref WRATH2DRigidTransformation::rotation() const.
   */
  const std::complex<float>&
  rotation(void) const
  {
    return m_values.m_transformation.rotation();
  }
  
  /*!\fn const WRATHLayerItemNodeRotateTranslateValues& values(void) const
    Returns the values of this node 
    (transformation, visibility, etc)
    relative to it's parent.
   */
  const WRATHLayerItemNodeRotateTranslateValues&
  values(void) const
  {
    return m_values;
  }

  /*!\fn const WRATHLayerItemNodeRotateTranslateValues& global_values(void) const
    Returns the values of this node 
    (visibility, translation, etc)
    relative to _ROOT_. Note if the
    hierarhcy is considered dirty, 
    then these values may or may not
    be accurate.
   */
  const WRATHLayerItemNodeRotateTranslateValues&
  global_values(void) const
  {
    return m_global_values;
  }

  /*!\fn void canvas_as_child_of_node(WRATHLayer*)
    Set so that the indicated WRATHLayer is
    drawn as if it was a child of this, i.e.
    when the WRATHLayer is drawn it has the
    rotation and transformation of this
    node pre-applied to it's contents.
    \param c WRATHLayer to which to apply
   */
  void
  canvas_as_child_of_node(WRATHLayer *c)
  {
    if(!m_transformer.valid())
      {
        m_transformer=m_global_values.create_pre_transformer(this->triple_buffer_enabler());
      }
    c->simulation_transformation_modifier(WRATHLayer::modelview_matrix, m_transformer);
  }

  virtual
  void
  extract_values(reorder_c_array<float> out_value)
  {
    m_global_values.extract_values(out_value, this->normalized_z());
  }

  /*!\fn const WRATHLayerItemNodeBase::node_function_packet& functions
    Returns same value as \ref node_functions().
   */
  static
  const WRATHLayerItemNodeBase::node_function_packet&
  functions(void) 
  {
    return WRATHLayerItemNodeRotateTranslateValues::functions();
  }

  virtual
  const WRATHLayerItemNodeBase::node_function_packet&
  node_functions(void) const
  {
    return WRATHLayerItemNodeRotateTranslateValues::functions();
  }

protected:

  virtual
  void
  compute_values(void)
  {
    this->compute_z_value();
    if(this->parent()!=NULL and m_compose_transformation_with_parent)
      {
        m_global_values.compose(this->parent()->m_global_values, m_values);
      }
    else
      {
        m_global_values=m_values;
      }
  }

private:
  bool m_compose_transformation_with_parent;
  WRATHLayerItemNodeRotateTranslateValues m_values;
  WRATHLayerItemNodeRotateTranslateValues m_global_values;
  WRATHLayerIntermediateTransformation::handle m_transformer;
};


/*!\typedef WRATHLayerItemNodeRotateTranslate
  Conveniance typedef to WRATHLayerItemNodeRotateTranslateT\<WRATHLayerItemNodeDepthType::flat_ordering\>,
  i.e. transformation as translate and scaling, with clipping
  window and flat z-ordering.
 */ 
typedef WRATHLayerItemNodeRotateTranslateT<WRATHLayerItemNodeDepthType::flat_ordering> WRATHLayerItemNodeRotateTranslate;


/*! @} */

#endif
