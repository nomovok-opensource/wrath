/*! 
 * \file NodePacker.hpp
 * \brief file NodePacker.hpp
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


#ifndef UNIFORM_PACKER_HPP_
#define UNIFORM_PACKER_HPP_


#include "WRATHLayerItemDrawerFactory.hpp"
#include "WRATHLayerNodeValuePackerUniformArrays.hpp"
#include "WRATHLayerNodeValuePackerHybrid.hpp"
#include "WRATHLayerNodeValuePackerTexture.hpp"
#include "WRATHLayerItemNodeRotateTranslate.hpp"
#include "WRATHLayerItemWidgets.hpp"

typedef WRATHLayerNodeValuePackerUniformArrays NodePackerBase; 
//typedef WRATHLayerNodeValuePackerTextureFP16 NodePackerBase; 
//typedef WRATHLayerNodeValuePackerTextureFP32 NodePackerBase; 
//typedef WRATHLayerNodeValuePackerHybrid<WRATHLayerNodeValuePackerUniformArrays, WRATHLayerNodeValuePackerTextureFP32> NodePackerBase;

class NodePacker:public NodePackerBase
{
public:

  typedef WRATHLayerItemDrawerFactory<WRATHLayerItemNodeRotateTranslate, NodePacker> Factory;
  typedef WRATHLayerItemWidget<WRATHLayerItemNodeRotateTranslate, NodePacker>::Generator Generator;
  typedef WRATHLayerItemWidget<WRATHLayerItemNodeRotateTranslate, NodePacker>::FamilySet FamilySet;

  NodePacker(WRATHLayerBase *layer,
                const SpecDataProcessedPayload::const_handle &payload,
                const ProcessedActiveNodeValuesCollection &spec):
    NodePackerBase(layer, payload, spec)
  {}

  static
  const WRATHLayerNodeValuePackerBase::function_packet&
  functions(void);

  static
  int&
  max_node_count(void);

  
};



#endif
