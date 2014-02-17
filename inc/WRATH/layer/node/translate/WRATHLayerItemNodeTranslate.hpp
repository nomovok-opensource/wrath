/*! 
 * \file WRATHLayerItemNodeTranslate.hpp
 * \brief file WRATHLayerItemNodeTranslate.hpp
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


#ifndef WRATH_HEADER_LAYER_ITEM_NODE_TRANSLATE_HPP_
#define WRATH_HEADER_LAYER_ITEM_NODE_TRANSLATE_HPP_

#include "WRATHConfig.hpp"
#include <limits>
#include "vectorGL.hpp"
#include "WRATHBBox.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHLayerItemNodeDepthOrder.hpp"
#include "WRATHScaleTranslate.hpp"
#include "WRATHLayer.hpp"
#include "WRATHLayerIntermediateTransformation.hpp"

/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerItemNodeTranslateValues
  The class data_type holds the parameters
  of a WRATHLayerItemNodeTranslate:
  - transformation (see \ref m_transformation)
  - clipping (see \ref m_clipping_active and \ref m_clip_rect)
  - visibility (see \ref m_visible)
 */
class WRATHLayerItemNodeTranslateValues
{
public:
  /*!\typedef layer_transformer_handle
    Conveniance typedef to a handle to a WRATHLayerIntermediateTransformation
   */
  typedef WRATHLayerIntermediateTransformation::handle layer_transformer_handle;

  /*!\typedef clip_drawer_handle 
    Conveniance typedef to a handle to a WRATHLayerClipDrawer
   */
  typedef WRATHLayerClipDrawer::handle clip_drawer_handle;

  /*!\fn WRATHLayerItemNodeTranslateValues
    Ctor, initializes
    \ref m_visible as true
    \ref m_clipping_active as false
    \ref m_clip_rect as the set \f$[0,1]X[0,1]\f$
    and \ref m_transformation as identity.
   */
  WRATHLayerItemNodeTranslateValues(void):
    m_visible(true),
    m_clip_rect( vec2(0.0f, 0.0f), vec2(1.0f, 1.0f)),
    m_clipping_active(false)
  {}
  
  /*!\var m_transformation
    Holds the transformation of the node,
    from local coordiantes to root or
    from local coordinate to parent coordinates.
  */
  WRATHScaleTranslate m_transformation;
  
  /*!\var m_visible
    Holds if the node is visible relative
    to the root or parent.
  */
  bool m_visible;
  
  /*!\var m_clip_rect
    For \ref WRATHLayerItemNodeTranslate::values(),
    holds the clipping rectangle in local coordinates,
    i.e. coordinates before any transformations are 
    applied. For example if one wishes to clip contents
    of a widget to [0,w]x[0,h] (i.e. a widget of width
    w and height h), then \ref m_clip_rect is then 
    set as WRATHBBox<2>( vec2(0,0), vec2(w,h)).    
  */
  WRATHBBox<2> m_clip_rect;
  
  /*!\var m_clipping_active
    Holds if clipping is active relative
    to the parent. 
  */
  bool m_clipping_active;

  /*!\fn void compose
    Sets the values of *this WRATHLayerItemNodeTranslateValues
    as "parent * local". The transformation and clipping are composed.
    However, the clipping window is in _GLOBAL_ coordinates, this
    is so that repeated composition is made easier.
    \param parent WRATHLayerItemNodeTranslateValues LHS of composition operation
    \param local WRATHLayerItemNodeTranslateValues RHS of composition operation
   */
  void
  compose(const WRATHLayerItemNodeTranslateValues &parent,
          const WRATHLayerItemNodeTranslateValues &local);

  /*!\fn void extract_values
    Extracts values from this WRATHLayerItemNodeTranslateValues
    and places them into an array. The values are extracted as follows:
    - WRATH_LAYER_TRANSLATE_X holds the translation in x
    - WRATH_LAYER_TRANSLATE_Y holds the translation in y
    - WRATH_LAYER_TRANSLATE_SCALE holds the scaling factor, if clipping is active it is made negative
    - WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MIN_X holds the min-x of the clipping window, transformed to _LOCAL_ coords
    - WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MIN_Y holds the min-y of the clipping window, transformed to _LOCAL_ coords
    - WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MAX_X holds the max-x of the clipping window, transformed to _LOCAL_ coords
    - WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MAX_Y holds the max-y of the clipping window, transformed to _LOCAL_ coords
    - WRATH_LAYER_TRANSLATE_Z holds the value z-value passed. If \ref m_visible is false, then the value stored 
                              is instead extreme negative.
    \param out_value location to which to extract values
    \param z_order value to pack for WRATH_LAYER_TRANSLATE_Z
  */
  void
  extract_values(reorder_c_array<float> out_value, float z_order);

  /*!\fn const WRATHLayerItemNodeBase::node_function_packet& functions
    Function packet for the transformation node values
    in a WRATHLayerItemNodeTranslateValues
   */
  static
  const WRATHLayerItemNodeBase::node_function_packet&
  functions(void);

  /*!\fn layer_transformer_handle create_pre_transformer
    Create a WRATHLayerIntermediateTransformation object
    that pre-applies the transformation of this 
    WRATHLayerItemNodeTranslateValues. It is required
    that this WRATHLayerItemNodeTranslateValues stays
    in scope for as long as the returned object has an
    active reference or until unhook_transformer() is called on it.
    \param tr handle to WRATHTripleBufferEnabler for syncing
   */
  layer_transformer_handle
  create_pre_transformer(const WRATHTripleBufferEnabler::handle &tr);

  /*!\fn void unhook_transformer(const layer_transformer_handle&)
    If the WRATHLayerItemNodeTranslateValues object
    that created a WRATHLayerIntermediateTransformation
    object via create_pre_transformer() goes out of scope
    before the WRATHLayerIntermediateTransformation does,
    call unhook_transformer() to make the transformation be the identity
    and to not refer to the WRATHLayerItemNodeTranslateValues
    that created it.
    \param h handle to WRATHLayerIntermediateTransformation 
             created with create_pre_transformer()
   */
  static
  void
  unhook_transformer(const layer_transformer_handle &h);

  /*!\fn clip_drawer_handle create_clip_drawer
    Create a WRATHLayerClipDrawer object
    that specifies the clipping of this
    WRATHLayerItemNodeTranslateValues. It is required
    that this WRATHLayerItemNodeTranslateValues stays
    in scope for as long as the returned object has an
    active reference or until unhook_clip_drawer() is called on it.
    \param tr handle to WRATHTripleBufferEnabler for syncing
   */
  clip_drawer_handle
  create_clip_drawer(const WRATHTripleBufferEnabler::handle &tr);

  /*!\fn void unhook_clip_drawer(const clip_drawer_handle&)
    If the WRATHLayerItemNodeTranslateValues object
    that created a WRATHLayerIntermediateTransformation
    object via create_clip_drawer() goes out of scope
    before the WRATHLayerIntermediateTransformation does,
    call unhook_clip_drawer() to make the clip drawer specify no clipping
    and to not refer to the WRATHLayerItemNodeTranslateValues
    that created it.
    \param h handle to WRATHLayerClipDrawer created with create_clip_drawer()
   */
  static
  void
  unhook_clip_drawer(const clip_drawer_handle &h);
};


/*!\class WRATHLayerItemNodeTranslateT
  A WRATHLayerItemNodeTranslate provides a translation
  and clipping against coordinate aligned rectangles
  hierarchy. The clip coordinates are relative to
  the translation (i.e. clip coordinates are local).
  The data of a WRATHLayerItemNodeTranslate 
  is extracted to GLSL by \ref WRATHLayerItemNodeTranslateValues::extract_values().
  \tparam pz_order_type enumeration dictating how global z-order is computed
  \tparam normalizer_type normalizer type providing the integer type for the z-order values
                          and normalization functions. The template class \ref
                          WRATHUtil::normalizer provides exactly what is required
                          for pnormalizer_type.
 */
template<enum WRATHLayerItemNodeDepthType::depth_order_t pz_order_type, 
         typename pnormalizer_type=WRATHUtil::normalizer<int16_t> >
class WRATHLayerItemNodeTranslateT:
  public WRATHLayerItemNodeDepthOrder<pz_order_type, WRATHLayerItemNodeTranslateT<pz_order_type, pnormalizer_type>, pnormalizer_type>
{
public:
  enum
    {
      /*!
       */
      number_per_node_values=8
    };  
  
  /*!\fn WRATHLayerItemNodeTranslateT(const WRATHTripleBufferEnabler::handle &)
    Ctor. Creates a root WRATHLayerItemNodeTranslateT.
    \param r handle to a WRATHTripleBufferEnabler to
             which the users of the created object will
             sync.
   */
  explicit
  WRATHLayerItemNodeTranslateT(const WRATHTripleBufferEnabler::handle &r):
    WRATHLayerItemNodeDepthOrder<pz_order_type, WRATHLayerItemNodeTranslateT<pz_order_type, pnormalizer_type>, pnormalizer_type>(r),
    m_compose_transformation_with_parent(true)
  {}

  /*!\fn WRATHLayerItemNodeTranslateT(WRATHLayerItemNodeTranslateT*)
    Ctor. 
    \param pparent pointer to parent of the created 
                   WRATHLayerItemNodeTranslate. The parent
                   owns the created object, must
                   NOT be NULL.
   */
  explicit
  WRATHLayerItemNodeTranslateT(WRATHLayerItemNodeTranslateT *pparent):
    WRATHLayerItemNodeDepthOrder<pz_order_type, WRATHLayerItemNodeTranslateT<pz_order_type, pnormalizer_type>, pnormalizer_type>(pparent),
    m_compose_transformation_with_parent(true)
  {}

  ~WRATHLayerItemNodeTranslateT()
  { 
    WRATHLayerItemNodeTranslateValues::unhook_clip_drawer(m_clipper);
    WRATHLayerItemNodeTranslateValues::unhook_transformer(m_transformer);
  }

  /*!\fn bool visible(void) const
    Returns true if and only if this WRATHLayerItemNodeTranslate
    is visible relative to it's parent, see \ref WRATHLayerItemNodeTranslateValues::m_visible.
   */
  bool 
  visible(void) const
  {
    return m_values.m_visible;
  }

  /*!\fn void visible(bool)
    Set if this WRATHLayerItemNodeTranslate
    is visible relative to it's parent, see \ref WRATHLayerItemNodeTranslateValues::m_visible.
    \param v value to use
   */
  void
  visible(bool v)
  {
    this->mark_dirty(v!=m_values.m_visible);
    m_values.m_visible=v;
  }  

  /*!\fn bool compose_transformation_with_parent
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
   */
  void
  compose_transformation_with_parent(bool v)
  {
    this->mark_dirty(v!=m_compose_transformation_with_parent);
    m_compose_transformation_with_parent=v;
  }

  /*!\fn const WRATHScaleTranslate& transformation(void) const
    Returns the node's transformation, 
    see \ref WRATHLayerItemNodeTranslateValues::m_transformation
   */
  const WRATHScaleTranslate&
  transformation(void) const
  {
    return m_values.m_transformation;
  }

  /*!\fn void transformation(const WRATHScaleTranslate&)
    Set the node's transformation directly
   */
  void
  transformation(const WRATHScaleTranslate &v)
  {
    m_values.m_transformation=v;
    this->mark_dirty();
  }

  /*!\fn const vec2& translation(void) const
    Returns the translation of the node's transformation, 
    see \ref WRATHLayerItemNodeTranslateValues::m_transformation and
    \ref WRATHScaleTranslate::translation().
   */
  const vec2&
  translation(void) const
  {
    return m_values.m_transformation.translation();
  }

  /*!\fn void translation(const vec2&)
    Sets the translation of the node's transformation, 
    see \ref WRATHLayerItemNodeTranslateValues::m_transformation and
    \ref WRATHScaleTranslate::translation(const vec2 &).
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
   */
  void
  position(const vec2 &v) 
  {
    translation(v);
  }

  /*!\fn float scaling_factor(void) const
    Returns the scaling factor of the node's transformation, 
    see \ref WRATHLayerItemNodeTranslateValues::m_transformation and
    \ref WRATHScaleTranslate::scale().
   */
  float
  scaling_factor(void) const
  {
    return m_values.m_transformation.scale();
  }

  /*!\fn void scaling_factor(float)
    Sets the scaling factor of the node's transformation, 
    see \ref WRATHLayerItemNodeTranslateValues::m_transformation and
    \ref WRATHScaleTranslate::scale(float).
   */
  void
  scaling_factor(float v) 
  {
    m_values.m_transformation.scale(v);
    this->mark_dirty();
  }
  
  /*!\fn const WRATHBBox<2>& clip_rect(void) const
    Returns the node's clipping rectangle,
    see \ref WRATHLayerItemNodeTranslateValues::m_clip_rect
   */
  const WRATHBBox<2>&
  clip_rect(void) const
  {
    return m_values.m_clip_rect;
  }

  /*!\fn void clip_rect(const WRATHBBox<2> &)
    Sets the node's clipping rectangle,
    see \ref WRATHLayerItemNodeTranslateValues::m_clip_rect
   */
  void
  clip_rect(const WRATHBBox<2> &v)
  {
    m_values.m_clip_rect=v;
    this->mark_dirty();
  }

  /*!\fn bool clipping_active(void) const
    Returns true if the node's clipping is active,
    see \ref WRATHLayerItemNodeTranslateValues::m_clipping_active
   */
  bool
  clipping_active(void) const
  {
    return m_values.m_clipping_active;
  }

  /*!\fn void clipping_active(bool)
    Set if the node's clipping is active,
    see \ref WRATHLayerItemNodeTranslateValues::m_clipping_active
    \param v value to which to set \ref WRATHLayerItemNodeTranslateValues::m_clipping_active
   */
  void
  clipping_active(bool v) 
  {
    this->mark_dirty(v!=m_values.m_clipping_active);
    m_values.m_clipping_active=v;
  }

  /*!\fn const WRATHLayerItemNodeTranslateValues& values(void) const
    Returns the values of this node 
    (clipping, translation, etc)
    relative to it's parent.
   */
  const WRATHLayerItemNodeTranslateValues&
  values(void) const
  {
    return m_values;
  }

  /*!\fn const WRATHLayerItemNodeTranslateValues& global_values(void) const
    Returns the values of this node 
    (clipping, translation, etc)
    relative to _ROOT_. Note if the
    hierarhcy is considered dirty, 
    then these values may or may not
    be accurate.
   */
  const WRATHLayerItemNodeTranslateValues&
  global_values(void) const
  {
    return m_global_values;
  }

  /*!\fn void canvas_as_child_of_node(WRATHLayer*)
    Set so that the indicated WRATHLayer is
    drawn as if it was a child of this, i.e.
    when the WRATHLayer is drawn it has the
    clipping and transformation of this
    node pre-applied to it's contents.
    \param c WRATHLayer to which to apply
   */
  void
  canvas_as_child_of_node(WRATHLayer *c)
  {
    if(!m_transformer.valid())
      {
        m_transformer=m_global_values.create_pre_transformer(this->triple_buffer_enabler());
        m_clipper=m_global_values.create_clip_drawer(this->triple_buffer_enabler());
      }
    c->simulation_transformation_modifier(WRATHLayer::modelview_matrix, m_transformer);
    c->simulation_clip_drawer(m_clipper);
  }

  /*!\fn void extract_values(reorder_c_array<float>)
    Implements WRATHLayerItemNodeBase::extract_values(reorder_c_array<float>)
    \param out_value location to which to write per-node values
   */
  virtual
  void
  extract_values(reorder_c_array<float> out_value)
  {
    m_global_values.extract_values(out_value, this->normalized_z());
  }

  /*!\fn const WRATHLayerItemNodeBase::node_function_packet& node_functions()
    Implements WRATHLayerItemNodeBase::node_functions() 
   */
  virtual
  const WRATHLayerItemNodeBase::node_function_packet&
  node_functions(void) const
  {
    return WRATHLayerItemNodeTranslateValues::functions();
  }

  /*!\fn const WRATHLayerItemNodeBase::node_function_packet& functions()
    Returns same value as node_functions()
   */
  static
  const WRATHLayerItemNodeBase::node_function_packet&
  functions(void) 
  {
    return WRATHLayerItemNodeTranslateValues::functions();
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
        //we need the clippig window in global coordinates though:
        m_global_values.m_clip_rect.scale(m_global_values.m_transformation.scale());
        m_global_values.m_clip_rect.translate(m_global_values.m_transformation.translation());
      }
  }

private:  
  bool m_compose_transformation_with_parent;
  WRATHLayerItemNodeTranslateValues m_values;
  WRATHLayerItemNodeTranslateValues m_global_values;
  WRATHLayerClipDrawer::handle m_clipper;
  WRATHLayerIntermediateTransformation::handle m_transformer;
};

/*!\typedef WRATHLayerItemNodeTranslate
  Conveniance typedef to WRATHLayerItemNodeTranslateT\<WRATHLayerItemNodeDepthType::flat_ordering\>,
  i.e. transformation as translate and scaling, with clipping
  window and flat z-ordering.
 */ 
typedef WRATHLayerItemNodeTranslateT<WRATHLayerItemNodeDepthType::flat_ordering> WRATHLayerItemNodeTranslate;


/*! @} */

#endif
