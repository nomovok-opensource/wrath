/*! 
 * \file WRATHFamilySet.hpp
 * \brief file WRATHFamilySet.hpp
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


#ifndef __WRATH_FAMILY_SET_HPP__
#define __WRATH_FAMILY_SET_HPP__

#include "WRATHConfig.hpp"
#include "WRATHWidgetEnums.hpp"
#include "WRATHTextureCoordinate.hpp"
#include "WRATHWidgetHandle.hpp"
#include "WRATHFamily.hpp"

/*! \addtogroup Widgets
 * @{
 */

/*!\class WRATHFamilySet

  A template typedef machine to define node types from a transformation
  node type. In addition defines typedef mapping to \ref WRATHFamily
  for these augmented node types:\n

  [C][Color][ Linear[Repeat]Gradient | Radial[Repeat]Gradient ][Image]Family \n\n

  each value in [] can be included or discluded, the meanings are:
  - C : generic clipping, only necessary for shapes (and any user defined item type that does draw coordinate alinged rectangles)
  - Color: includes a color value in the node
  - LinearGradient: includes linear gradient value in the node
  - RadialGradient: includes radial gradient value in the node
  - Repeat: includes a window to repeat the gradient
  - Image: includes texture coordinate and repeat mode values in the node
  
  In addition also defines the set of families, (each is a typedef to WRATHFamily with the correctly augmented node type):\n
 
  [C][Color][Linear[Repeat]Gradient|Radial[Repeat]Gradient]{ModeX}X{ModeY}YImageFamily\n

  where ModeX names the repeat mode in X and ModeY names the repeat mode in Y
  where the modes are one of:
  - Repeat 
  - Simple 
  - Clamp  
  - MirrorRepeat 

  Also defines the two family types:\n
  
  [C]PlainFamily\n\n
  
  corresponding to using the base node type.

  For example, the family set ColorImageFamily represents
  using a node type that stores color, texture coordinate 
  and texture repeat mode information. Another example,
  LinearGradientRepeatXMirrorRepeatYImageFamily is
  a family with image and linear gradient information
  where the image is sampled with x-direction repeat,
  and y-direction mirror repeat. Another example,
  RadialRepeatGradientFamily is a node holding data
  for repeat radial gradient.

  As a conveniance, the enumeration values
  \ref WRATHWidgetEnums::widget_clipping_t and
  \ref WRATHWidgetEnums::node_type_bits are
  copied into the scope of WRATHFamilySet.

  \tparam pBaseNodeType base node type
  \tparam NodeTypeDefiner class that defines Node<N, uint32_t>::type
          that reflects a node type with base properties from node type N
          and _the_ added property as indicated by
          which bit is up from \ref WRATHWidgetEnums::node_type_bits.
          At most only one bit will be up, possibly even 0.
  \tparam pImageConstantRepeatMode class that given a node type Node,
          and repeat modes in X and Y evaluates to a node type
          of the original node type N with texture coordinates
          that will when it uses image data will sample the image
          data via the specified repeat modes
  \tparam pCanvasType specifies the canvas type
  \tparam pDrawerFactoryTypeDefiner class that defines the member type
          DrawerFactory<N> type given a node type N 
          to be a type derived from WRATHItemDrawerFactory suitable for the
          passed node type and pCanvasType
  \tparam SubDrawerID template type that takes a WRATHWidgetGenerator::widget_clipping_t
          value and defines the enumeration subdrawer_id that corresponds
          correctly with the clipping style when fed into the factory type.
 */
template<typename pBaseNodeType,
         template<typename Node, uint32_t bit> class pNodeTypeDefiner,
         template<typename Node, 
                  enum WRATHTextureCoordinate::repeat_mode_type,
                  enum WRATHTextureCoordinate::repeat_mode_type> class pImageConstantRepeatMode,
         typename pCanvasType,
         typename pDrawerFactoryTypeDefiner,
         template <enum WRATHWidgetEnums::widget_clipping_t> class pSubDrawerID>
class WRATHFamilySet
{
public:

  enum
    {
      /*!
        SubDrawer ID to pass a factory type for relying
        on quad-quad per-item clipping
       */
      SubDrawerIDQuadClipping=pSubDrawerID<WRATHWidgetEnums::widget_quad_clipping>::subdrawer_id,

      /*!
        SubDrawer ID to pass a factory type for general clipping
       */
      SubDrawerIDGenericClipping=pSubDrawerID<WRATHWidgetEnums::widget_generic_clipping>::subdrawer_id,
    };
  

  /*
    bring into scope the enums of WRATHWidgetEnums
   */
  /// @cond
  #define COPY_ENUM(X) enum { X=WRATHWidgetEnums::X }
  COPY_ENUM(linear_gradient);
  COPY_ENUM(radial_gradient);
  COPY_ENUM(gradient_repeat);
  COPY_ENUM(linear_repeat_gradient);
  COPY_ENUM(radial_repeat_gradient);
  COPY_ENUM(color);
  COPY_ENUM(image);
  #undef COPY_ENUM
  
  #define COPY_ENUM(X) enum { X=WRATHTextureCoordinate::X }
  COPY_ENUM(simple);
  COPY_ENUM(clamp);
  COPY_ENUM(repeat);
  COPY_ENUM(mirror_repeat);
  #undef COPY_ENUM
  /// @endcond


  /*!\typedef Canvas
    The WRATHCanvas derived type that all items
    of all families are drawn to.
   */
  typedef pCanvasType Canvas;

  /*!\typedef Node
    Base class node type
   */
  typedef pBaseNodeType Node;
      
  
  /*!\class selector
    Implementation class detail to define
    a node type from a set of bits up from
    the enumeration \ref WRATHWidgetEnums::node_type_bits.
    Likely one will not use directly.
    \tparam bits bit field taking value froms from 
            \ref WRATHWidgetEnums::node_type_bits indicating
            what data to add to the base node type, \ref
            Node
  */
  template<uint32_t bits>
  class selector
  {
  public:
    enum
      {
        /*!
          Localize template parameter bits;
          is the a bitfield from values taken
          from \ref WRATHWidgetEnums::node_type_bits
          indicating what data to augment \ref Node 
         */
        selector_bits=bits
      };

    /*!\typedef node
      Defines the node type of the \ref Node augmented by
      the data specified by \ref selector_bits
     */
    typedef typename WRATHWidgetEnums::node_type<pBaseNodeType, pNodeTypeDefiner, bits>::type node;  

    /*!\class const_repeat_node
      Defines the node type of the \ref Node augmented by:
      - the data required by the template parameter bits
      - the data required for texture mapping from an image where the repeat mode is constant

      This class-type is only used when the bit \ref WRATHWidgetEnums::image
      is not up in \ref selector_bits
      \tparam X repeat mode in the x-direction
      \tparam Y repeat mode in the y-direction
     */
    template<enum WRATHTextureCoordinate::repeat_mode_type X,
             enum WRATHTextureCoordinate::repeat_mode_type Y>
    class const_repeat_node
    {
    public:
      /*!\typedef type
        The node type augmented to allow to sample image
        data with a fixed repeat mode
       */
      typedef pImageConstantRepeatMode<node, X, Y> type;
    };
  };
  
  /*!\class WidgetBaseDefiner
    Implementation class detail to fill the parameters 
    of \ref WRATHWidgetBase for a node type N.
    Likely one will not use directly.
    \tparam N node type to generate a \ref WRATHWidgetBase
              created type. The node type N typically is generated
              by \ref selector using either \ref selector::node
              or \ref selector::const_repeat_node::type
   */
  template<typename N>
  class WidgetBaseDefiner
  {
  public:    
    /*!\typedef quad_clipped
      \ref WRATHWidgetBase generated type for using quad-quad per-item clipping
     */
    typedef WRATHWidgetBase<N, pCanvasType, 
                            typename pDrawerFactoryTypeDefiner::template DrawerFactory<N>, 
                            SubDrawerIDQuadClipping> quad_clipped;   

    /*!\typedef generic_clipped
      \ref WRATHWidgetBase generated type for using generic per-item clipping
     */
    typedef WRATHWidgetBase<N, pCanvasType, 
                            typename pDrawerFactoryTypeDefiner::template DrawerFactory<N>, 
                            SubDrawerIDGenericClipping> generic_clipped;    
  };
 
  /*!\class Family
    Conveniance typedef filling in parameters to 
    \ref WRATHFamily from a bitfield indicating to 
    how to augment the node type \ref Node so
    that clipping is quad-quad per-item clipping.
    Likely one will not use directly.
    \tparam bits bitfield using the values from \ref WRATHWidgetEnums::node_type_bits
   */
  template<uint32_t bits>
  class Family:
    public WRATHFamily<typename WidgetBaseDefiner<typename selector<bits>::node>::quad_clipped>
  {};
  
  /*!\class CFamily
    Conveniance typedef filling in parameters to 
    \ref WRATHFamily from a bitfield indicating to 
    how to augment the node type \ref Node so
    that clipping is generic per-item clipping.
    Likely one will not use directly.
    \tparam bits bitfield using the values from \ref WRATHWidgetEnums::node_type_bits
   */
  template<uint32_t bits>
  class CFamily:
    public WRATHFamily<typename WidgetBaseDefiner<typename selector<bits>::node>::generic_clipped>
  {};

  /*!\class selector_chooser
    Conveniance class to define typedefs to generate
    a node type from a bitfield an const repeat mode
    values.
    \tparam bits bitfield using the values from \ref WRATHWidgetEnums::node_type_bits
    \tparam X repeat mode in the x-direction
    \tparam Y repeat mode in the y-direction
   */
  template<uint32_t bits,
           enum WRATHTextureCoordinate::repeat_mode_type X,
           enum WRATHTextureCoordinate::repeat_mode_type Y>
  class selector_chooser
  {
  public:
    /*!\typedef the_selector
      Conveniance typedef for \ref selector,
      note that \ref WRATHWidgetEnums::image is masked out
      from bits.
     */
    typedef selector<bits&~image> the_selector;

    /*!\typedef type
      Conveniance typedef mapping to \ref selector::const_repeat_node::type
      with the correct template parameter values.
     */
    typedef typename the_selector::template const_repeat_node<X, Y>::type type;
  };

  /*!\class FamilyConstRepeatMode
    Anologue of \ref Family with node type additionally
    agumented by node values for texture mapping
    with a constant repeat mode.
    Likely one will not use directly.
    \tparam bits bitfield using the values from \ref WRATHWidgetEnums::node_type_bits
    \tparam X repeat mode in the x-direction
    \tparam Y repeat mode in the y-direction
   */
  template<uint32_t bits,
           enum WRATHTextureCoordinate::repeat_mode_type X,
           enum WRATHTextureCoordinate::repeat_mode_type Y>
  class FamilyConstRepeatMode:
    public WRATHFamily<typename WidgetBaseDefiner<typename selector_chooser<bits, X, Y>::type >::quad_clipped>
  {};

  /*!\class CFamilyConstRepeatMode
    Anologue of \ref CFamily with node type additionally
    agumented by node values for texture mapping
    with a constant repeat mode.
    Likely one will not use directly.
    \tparam bits bitfield using the values from \ref WRATHWidgetEnums::node_type_bits
    \tparam X repeat mode in the x-direction
    \tparam Y repeat mode in the y-direction
   */
  template<uint32_t bits,
           enum WRATHTextureCoordinate::repeat_mode_type X,
           enum WRATHTextureCoordinate::repeat_mode_type Y>
  class CFamilyConstRepeatMode:
    public WRATHFamily<typename WidgetBaseDefiner<typename selector_chooser<bits, X, Y>::type>::generic_clipped>
  {};

  /*
    Define the massive matrix of family sets,
    the convention is as follows:
     - linear --> LinearGradient
     - image ---> Image
     - color ---> Color
     - radial --> RadialGradient
     - linear_repeat_gradient --> LinearRepeatGradient
     - radial_repeat_gradient --> RadialRepeatGradient
   */
  /// @cond

  #define MAKE_REPEAT_FAMILY_TYPES_WORKER_Y_PP(bits, Mode, Arg) \
    typedef FamilyConstRepeatMode<bits, WRATHTextureCoordinate::Mode, WRATHTextureCoordinate::repeat> Arg##RepeatYImageFamily; \
    typedef FamilyConstRepeatMode<bits, WRATHTextureCoordinate::Mode, WRATHTextureCoordinate::simple> Arg##SimpleYImageFamily; \
    typedef FamilyConstRepeatMode<bits, WRATHTextureCoordinate::Mode, WRATHTextureCoordinate::clamp> Arg##ClampYImageFamily; \
    typedef FamilyConstRepeatMode<bits, WRATHTextureCoordinate::Mode, WRATHTextureCoordinate::mirror_repeat> Arg##MirrorRepeatYImageFamily; 

  #define MAKE_REPEAT_FAMILY_TYPES_WORKER_Y(bits, Mode, Arg)   \
    MAKE_REPEAT_FAMILY_TYPES_WORKER_Y_PP(bits, Mode, Arg)

  #define MAKE_REPEAT_FAMILY_TYPES_PP(bits, Arg) \
    MAKE_REPEAT_FAMILY_TYPES_WORKER_Y(bits, repeat, Arg##RepeatX); \
    MAKE_REPEAT_FAMILY_TYPES_WORKER_Y(bits, simple, Arg##SimpleX); \
    MAKE_REPEAT_FAMILY_TYPES_WORKER_Y(bits, clamp, Arg##ClampX); \
    MAKE_REPEAT_FAMILY_TYPES_WORKER_Y(bits, mirror_repeat, Arg##MirrorRepeatX); 

  #define MAKE_REPEAT_FAMILY_TYPES(bits, Arg) \
    MAKE_REPEAT_FAMILY_TYPES_PP(bits, Arg)    

  #define MAKE_FAMILY_TYPES(bits, family_name)                          \
    typedef Family<bits> family_name##Family;                           \
    typedef CFamily<bits> C##family_name##Family;                       \
    typedef Family<bits|image> family_name##ImageFamily;              \
    typedef CFamily<bits|image> C##family_name##ImageFamily;            \
    MAKE_REPEAT_FAMILY_TYPES(bits, family_name);                 \
    MAKE_REPEAT_FAMILY_TYPES(bits, C##family_name)
    

  typedef Family<0> PlainFamily;                             
  typedef CFamily<0> CPlainFamily;                         
  typedef Family<image> ImageFamily;                
  typedef CFamily<image> CImageFamily;              
  MAKE_REPEAT_FAMILY_TYPES(0, );                  
  MAKE_REPEAT_FAMILY_TYPES(0, C);

  MAKE_FAMILY_TYPES(color, Color);

  MAKE_FAMILY_TYPES(linear_gradient, LinearGradient);  
  MAKE_FAMILY_TYPES(linear_repeat_gradient, LinearRepeatGradient);
  MAKE_FAMILY_TYPES(linear_gradient|color, ColorLinearGradient);  
  MAKE_FAMILY_TYPES(linear_repeat_gradient|color, ColorLinearRepeatGradient);
  
  MAKE_FAMILY_TYPES(radial_gradient, RadialGradient);
  MAKE_FAMILY_TYPES(radial_repeat_gradient, RadialRepeatGradient);
  MAKE_FAMILY_TYPES(radial_gradient|color, ColorRadialGradient);
  MAKE_FAMILY_TYPES(radial_repeat_gradient|color, ColorRadialRepeatGradient);
  


  #undef MAKE_FAMILY_TYPES
  #undef MAKE_REPEAT_FAMILY_TYPES
  #undef MAKE_REPEAT_FAMILY_TYPES_PP
  #undef MAKE_REPEAT_FAMILY_TYPES_WORKER_Y
  #undef MAKE_REPEAT_FAMILY_TYPES_WORKER_Y_PP

  /// @endcond
};




/*! @} */

#endif
