/*! 
 * \file WRATHLayerItemDrawerFactory.hpp
 * \brief file WRATHLayerItemDrawerFactory.hpp
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


#ifndef WRATH_HEADER_LAYER_ITEM_DRAWER_FACTORY_HPP_
#define WRATH_HEADER_LAYER_ITEM_DRAWER_FACTORY_HPP_

#include "WRATHConfig.hpp"
#include "WRATHLayer.hpp"
#include "WRATHShaderSpecifier.hpp"
#include "WRATHItemDrawerFactory.hpp"
#include "WRATHLayerNodeValuePackerBase.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHWidgetEnums.hpp"


/*! \addtogroup Layer
 * @{
 */

/*!\namespace WRATHLayerItemDrawerFactoryCommon
  Namespace to encapsulate various types used
  by \ref WRATHLayerItemDrawerFactory
 */ 
namespace WRATHLayerItemDrawerFactoryCommon
{
  
  /*!\enum clipping_implementation_type
    Enumeration to describe the clipping
    used by a GLSL program.
   */
  enum clipping_implementation_type
    {
      /*!
        GLSL program will only clip coordinate
        aligned quads and line segments
       */
      quad_clipping,

      /*!
        GLSL program will implement clipping
        using gl_ClipVertex. Clipping works
        for arbitary primitive types
       */
      clip_vertex_clipping,

      /*!
        GLSL program will implement clipping
        using gl_ClipDistance[]. Clipping works
        for arbitary primitive types
       */
      clip_distance_clipping,
      
      /*!
        GLSL program will implement clipping
        via a discard in the fragment shader
       */
      clip_discard_clipping,
    };

  /*!\class SubDrawerID
    Template class to produce a value (subdrawer_id) from 
    WRATHLayerItemDrawerFactoryCommon::clipping_implementation_type
    enumeration from a WRATHWidgetEnums::widget_quad_clipping enumeration,
    for the purpose of feeding as subdrawer_id to a WRATHLayerItemDrawerFactory.
  */
  template<enum WRATHWidgetEnums::widget_clipping_t C>
  class SubDrawerID 
  {
  public:
    enum 
      {
        /*!
          SubDrawerID taking as value a value from the enumeration
          \ref WRATHLayerItemDrawerFactoryCommon
         */
        subdrawer_id=WRATHLayerItemDrawerFactoryCommon::quad_clipping
      };
  };
  
  
  /// @cond
  template<>
  class SubDrawerID<WRATHWidgetEnums::widget_quad_clipping>
  {
  public:
    enum 
      {
        subdrawer_id=WRATHLayerItemDrawerFactoryCommon::quad_clipping
      };
  };
  
  template<>
  class SubDrawerID<WRATHWidgetEnums::widget_generic_clipping>
  {
  public:
    enum
      {
        /*
          TODO: if an environment that supports GL_CLIP_DISTANCE or GL_CLIP_VERTEX,
          use clip_distance_clipping or clip_vertex_clipping respectively
        */
        subdrawer_id=WRATHLayerItemDrawerFactoryCommon::clip_discard_clipping
      };
  };
  /// @endcond

  
  /*!\fn WRATHMultiGLProgram* generate_multi_glsl_program(const WRATHShaderSpecifier *,
                                                          const WRATHAttributePacker *,
                                                          enum clipping_implementation_type ,
                                                          const WRATHLayerItemNodeBase::node_function_packet &,
                                                          const WRATHLayerNodeValuePackerBase::function_packet &,
                                                          WRATHLayerNodeValuePackerBase::ProcessedActiveNodeValuesCollection &,
                                                          WRATHLayerNodeValuePackerBase::SpecDataProcessedPayload::handle &)
    Generates a GLSL program.
    Adds symbols based off of the clipping type:
    - clip_vertex_clipping --> WRATH_CLIP_VIA_CLIP_VERTEX
    - clip_distance_clipping --> WRATH_CLIP_VIA_CLIP_DISTANCE
    - clip_discard_clipping --> WRATH_CLIP_VIA_DISCARD

    Adds a single attribute, "transf_index" which names the
    attribute that specifies the node index.
    
    Additionally, adds for each non-empty shader stage S,
    in the passed WRATHShaderSpecifier the macro
    \code
      WRATH_XXX_ITEM_VALUE_FETCH_OK
    \endcode
    where XXX=WRATHGLShader::gl_shader_type_label(S)
    if the named shader shage S support fetching per-item
    value. These macros are ADDED to each shader stage.
    For example the macro WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
    is added to BOTH the vertex and fragment shaders so
    that both stages know that the node value fetching
    is possible from the fragment shader.

    \param shader_specifier specifies the "meat" of the shader code, i.e. the main of a GLSL shader
    \param attribute_packer specifies the attributes: their types and names
    \param tp specifies the macros defined to define clipping
    \param node_functions specifies the GLSL code to perform per-item transformations
    \param uniform_packer_functions specifies the GLSL code to perform fethcing per-node values
    \param spec [output] returns the WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection that the node demands
    \param payload [output] returns a handle to a payload object that is used to create
                   instances of objects derived from WRATHLayerNodeValuePackerBase
   */
  WRATHMultiGLProgram*
  generate_multi_glsl_program(const WRATHShaderSpecifier *shader_specifier,
                              const WRATHAttributePacker *attribute_packer,
                              enum clipping_implementation_type tp,
                              const WRATHLayerItemNodeBase::node_function_packet &node_functions,
                              const WRATHLayerNodeValuePackerBase::function_packet &uniform_packer_functions,
                              WRATHLayerNodeValuePackerBase::ProcessedActiveNodeValuesCollection &spec,
                              WRATHLayerNodeValuePackerBase::SpecDataProcessedPayload::handle &payload);
};


/*!\class WRATHLayerItemDrawerFactory
  The template class WRATHLayerItemDrawerFactory provides the C++
  machinery to generate WRATHItemDrawerFactory derived types
  that generate WRATHItemDrawer derived objects for consumption
  by a WRATHLayerBase derived object. The type created is stateless
  and is meant to be used with WRATHWidgetBase and WRATHWidgetGeneratorT.
  \tparam NodeType the node type for the drawers created by the factory.
                   Must derived from WRATHLayerItemNodeBase and provide
                   the static function const WRATHLayerItemNodeBase::node_function_packet &functions(void)
                   which returns a WRATHLayerItemNodeBase::node_function_packet object
                   suitable for NodeType.

  \tparam NodePackerType the packer type that is used to pack uniforms. 
                         Must be derived from WRATHLayerNodeValuePackerBase and
                         provide the static method 
                         const WRATHLayerNodeValuePackerBase::function_packet &functions(void)
                         which returns a WRATHLayerNodeValuePackerBase::function_packet
                         for that packer type.

  \tparam DrawerType (defaults to WRATHLayerItemDrawer\<NodePackerType\>) is the 
                     draw group drawer type that must derive from WRATHLayerItemDrawerBase
                     and have a constructor with the signature 
                     DrawerType(WRATHMultiGLProgram*, 
                                WRATHLayerNodeValuePackerBase::SpecDataProcessedPayload::handle, 
                                const WRATHLayerNodeValuePackerBase::ProcessedActiveNodeValuesCollection&);
                     (for example as seen in WRATHLayerItemDrawer\<T\>) where pr
                     is the WRATHMultiGLProgram the drawer is to use, pnumber_slots
                     is the number of unique nodes the drawer supports in a single call
                     and spec specifies the per-node values.
 */
template<typename NodeType,
         typename NodePackerType,
         typename DrawerType=WRATHLayerItemDrawer<NodePackerType> >
class WRATHLayerItemDrawerFactory:public WRATHItemDrawerFactory
{
public:
  
  /*!\fn WRATHItemDrawer* generate_drawer
    Implementation of generate_drawer suitable for
    use with \ref WRATHShaderSpecifier::fetch_drawer(),
    the sub_drawer_id is used to identify which clipping
    implementation to use.
    \param shader_specifier source code for item drawing
    \param attribute_packer attribute packer naming the attributes
    \param clipping_implementation must have an enumeration value 
                                   from \ref WRATHLayerItemDrawerFactoryCommon::clipping_implementation_type
   */
  virtual
  WRATHItemDrawer*
  generate_drawer(const WRATHShaderSpecifier *shader_specifier,
                  const WRATHAttributePacker *attribute_packer,
                  int clipping_implementation) const
  {
    WRATHMultiGLProgram *pr;
    enum WRATHLayerItemDrawerFactoryCommon::clipping_implementation_type cp;
    WRATHLayerNodeValuePackerBase::SpecDataProcessedPayload::handle payload;
    WRATHLayerNodeValuePackerBase::ProcessedActiveNodeValuesCollection spec;

    cp=static_cast<enum WRATHLayerItemDrawerFactoryCommon::clipping_implementation_type>(clipping_implementation);
  
      
    pr=WRATHLayerItemDrawerFactoryCommon::generate_multi_glsl_program(shader_specifier,
                                                                      attribute_packer,
                                                                      cp,
                                                                      NodeType::functions(),
                                                                      NodePackerType::functions(),
                                                                      spec, payload);
    
    return WRATHNew DrawerType(pr, payload, spec);
  }

  
  virtual
  WRATHItemDrawerFactory*
  copy(void) const
  {
    return WRATHNew WRATHLayerItemDrawerFactory();
  }

};
         
/*!\class WRATHLayerItemDrawerFactoryWrapper
  The purpose of this template class is mostly 
  for template meta-programming to allow
  for creating a factory class that only
  depends on the NodeType. 
  \tparam NodePackerType \ref WRATHLayerNodeValuePackerBase derived type to pack per-node values
  \tparam DrawerType \ref WRATHLayerNodeValuePackerBase::Drawer\<NodePackerType\> derived type,
                     consumable as the last template argument to \ref WRATHLayerItemDrawerFactory
 */
template<typename NodePackerType,
         typename DrawerType=WRATHLayerItemDrawer<NodePackerType> >
class WRATHLayerItemDrawerFactoryWrapper
{
public:
  /*!\class DrawerFactory
    Class definition to consume a \ref WRATHLayerItemNodeBase 
    derived type and produce a WRATHLayerItemDrawerFactory
    type.
    \tparam NodeType node type, must be derived from \ref WRATHLayerItemNodeBase 
   */
  template<typename NodeType>
  class DrawerFactory:public WRATHLayerItemDrawerFactory<NodeType, NodePackerType, DrawerType>
  {};
};


/*! @} */

#endif
