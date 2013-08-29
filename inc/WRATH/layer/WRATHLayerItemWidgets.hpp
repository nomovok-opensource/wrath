/*! 
 * \file WRATHLayerItemWidgets.hpp
 * \brief file WRATHLayerItemWidgets.hpp
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


#ifndef __WRATH_LAYER_ITEM_WIDGET_HPP__
#define __WRATH_LAYER_ITEM_WIDGET_HPP__

#include "WRATHWidget.hpp"
#include "WRATHWidgetGenerator.hpp"
#include "WRATHLayer.hpp"
#include "WRATHLayerItemDrawerFactory.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHLayerItemNodeLinearGradient.hpp"
#include "WRATHLayerItemNodeRepeatGradient.hpp"
#include "WRATHLayerItemNodeRadialGradient.hpp"
#include "WRATHLayerItemNodeColorValue.hpp"
#include "WRATHLayerNodeValuePackerUniformArrays.hpp"
#include "WRATHLayerNodeValuePackerTexture.hpp"
#include "WRATHLayerItemNodeTexture.hpp"


/*! \addtogroup Layer
 * @{
 */

/*!\namespace WRATHLayerItemWidgetSupport
  Supporting class definitions used by 
  template class WRATHLayerItemWidget
 */
namespace WRATHLayerItemWidgetSupport
{
  /*!\typedef DefaultNodePacker
    A Conveniance typedef to specify the default
    method to pack per-node values.
   */
  typedef WRATHLayerNodeValuePackerUniformArrays DefaultNodePacker;
  //typedef WRATHLayerNodeValuePackerTextureFP32 DefaultNodePacker;
  //typedef WRATHLayerNodeValuePackerTextureFP16 DefaultNodePacker;

  /*!\class NodeSelector 
    Template class for defining a typedef named type
    that corresponds to using a "base" node type
    with the data needed to supplement it with
    values defined by \ref WRATHWidgetEnums::node_type_bits,
    i.e. does:\n
    \n WRATHWidgetGenerator::const --> type is \ref WRATHLayerItemNodeColorValue
    \n WRATHWidgetGenerator::linear_gradient --> type is \ref WRATHLayerItemNodeLinearGradient
    \n WRATHWidgetGenerator::gradient_repeat --> type is \ref WRATHLayerItemNodeRepeatGradient 
    \n WRATHWidgetEnums::radial_gradient --> type is \ref WRATHLayerItemNodeRadialGradient 
  */
  template<typename N, uint32_t>
  class NodeSelector
  {
  };
  
  ///@cond
  template<typename N>
  class NodeSelector<N, 0>
  {
  public:
    typedef N type;
  };

  template<typename N>
  class NodeSelector<N, WRATHWidgetEnums::color>
  {
  public:
    typedef WRATHLayerItemNodeColorValue<N> type;
  };
  
  template<typename N>
  class NodeSelector<N, WRATHWidgetEnums::linear_gradient>
  {
  public:
    typedef WRATHLayerItemNodeLinearGradient<N> type;
  };
  
  template<typename N>
  class NodeSelector<N, WRATHWidgetEnums::gradient_repeat>
  {
  public:
    typedef WRATHLayerItemNodeRepeatGradient<N> type;
  };
  
  template<typename N>
  class NodeSelector<N, WRATHWidgetEnums::radial_gradient>
  {
  public:
    typedef WRATHLayerItemNodeRadialGradient<N> type;
  };

  template<typename N>
  class NodeSelector<N, WRATHWidgetEnums::image>
  {
  public:
    typedef WRATHLayerItemNodeTextureDynamic<N> type;
  };
  /// @endcond

};


/*!\class WRATHLayerItemWidget
  Class to define template types
  for widgets whose nodes derive from
  WRATHLayerItemNodeBase
  \tparam BaseNodeType base node type providing transformation, derived from \ref WRATHLayerItemNodeBase
  \tparam NodePackerType node packing type, derived from  \ref WRATHLayerNodeValuePackerBase
  \tparam CanvasType canvas type, derived from  \ref WRATHLayerBase
 */ 
template<typename BaseNodeType,
         typename NodePackerType=WRATHLayerItemWidgetSupport::DefaultNodePacker,
         typename CanvasType=WRATHLayer>
class WRATHLayerItemWidget
{
public:   
  /*!\typedef FamilySet
    Conveniance class that fill in the template arguments
    of WRATHFamilySet into a local typedef FamilySet
   */
  typedef WRATHFamilySet<BaseNodeType,
                         WRATHLayerItemWidgetSupport::NodeSelector,
                         WRATHLayerItemNodeTexture,
                         CanvasType,
                         WRATHLayerItemDrawerFactoryWrapper<NodePackerType>,
                         WRATHLayerItemDrawerFactoryCommon::SubDrawerID> FamilySet;


  /*!\typedef Generator
    Conveniance class that fill in the template arguments
    of WRATHWidgetGeneratorT into a local typedef Generator
  */
  typedef WRATHWidgetGeneratorT<FamilySet> Generator;
};

/*! @} */

#endif
