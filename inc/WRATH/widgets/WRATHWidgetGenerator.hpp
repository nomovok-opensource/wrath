/*! 
 * \file WRATHWidgetGenerator.hpp
 * \brief file WRATHWidgetGenerator.hpp
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


#ifndef __WRATH_WIDGET_GENERATOR_HPP__
#define __WRATH_WIDGET_GENERATOR_HPP__

#include "WRATHConfig.hpp"
#include <boost/type_traits.hpp>
#include <boost/static_assert.hpp>
#include <boost/optional.hpp>
#include "WRATHWidget.hpp"
#include "WRATHRectAttributePacker.hpp"
#include "WRATHDefaultRectAttributePacker.hpp"
#include "WRATHCanvasHandle.hpp"
#include "WRATHWidgetHandle.hpp"
#include "WRATHWidgetEnums.hpp"
#include "WRATHFamily.hpp"
#include "WRATHFamilySet.hpp"
#include "WRATHDefaultShapeShader.hpp"
#include "WRATHDefaultRectShader.hpp"
#include "WRATHLayerItemNodeDepthOrder.hpp"

/*! \addtogroup Widgets
 * @{
 */

/*!\namespace WRATHWidgetGenerator
  Namespace to define classes that set parameters
  of Items. The classes themselves only hold
  const references and/or pointers, so generally speaking
  don't save the values of these classes for later use.
 */
namespace WRATHWidgetGenerator
{
  /*!
    Bring into scope the enumerations from WRATHWidgetEnums.
   */
  using namespace WRATHWidgetEnums;
  
  /*!
    Bring into scope the stroking types from WRATHDefaultStrokeAttributePacker
   */
  using namespace WRATHDefaultStrokeAttributePacker::StrokingTypes;

  /*!
    Bring into scope the filling types from WRATHDefaultFillShapeAttributePacker
   */
  using namespace WRATHDefaultFillShapeAttributePacker::FillingTypes;

  /*!
    Bring into scope the types from WRATHShapeItemTypes
   */
  using namespace WRATHShapeItemTypes;

  /*!
    Bring into scope the types from WRATHTextItemTypes
   */
  using namespace WRATHTextItemTypes;

  /*!\typedef RectDrawer
    explicitely typedef types from WRATHRectItemTypes 
   */
  typedef WRATHRectItemTypes::Drawer RectDrawer;

  /*!
    Bring into scope enumerations from WRATHBrushEnums
   */
  using namespace WRATHBrushBits;

  /*!\class Brush
    Specifies what image and gradient to
    apply to an item. Note that the image
    and gradient applied to an item
    cannot be changed for the lifetime 
    of the item.
   */
  class Brush:public BrushBits<Brush>
  {
  public:
    /*!\fn Brush(WRATHImage*, WRATHGradient*)
      Ctor.
      \param pImage value to which to initialize \ref m_image
      \param pGradient value to which to initialize \ref m_gradient
     */
    Brush(WRATHImage *pImage=NULL,
          WRATHGradient *pGradient=NULL):
      m_image(pImage),
      m_gradient(pGradient)
    {}

    /*!\fn Brush(WRATHGradient*, WRATHImage*)
      Ctor.
      \param pGradient value to which to initialize \ref m_gradient
      \param pImage value to which to initialize \ref m_image
     */    
    Brush(WRATHGradient *pGradient,
          WRATHImage *pImage=NULL):
      m_image(pImage),
      m_gradient(pGradient)
    {}

    /*!\var m_image
      WRATHImage to use for image data
     */
    WRATHImage *m_image;

    /*!\var m_gradient
      WRATHGradient to use for gradient color data
     */
    WRATHGradient *m_gradient;  

    /*!\var m_draw_state
      Additional item draw state to apply to the brush
     */  
    WRATHSubItemDrawState m_draw_state; 
  };

  /*!\class TextWidgetCreator
    Class to specify how to create a TextWidget.
    The class does NOT store it's values passed to its
    ctor, rather it saves the refernces. Thus one 
    should not "save" these objects unless the objects
    passed at the ctor will stay in scope.
    \tparam TextWidget must be for some class T, \ref WRATHTextWidget\<T\>
   */
  template<typename TextWidget>
  class TextWidgetCreator
  {
  public:
    /*!\fn TextWidgetCreator
      Ctor.
      \param opacity value to which to initalize \ref m_opacity
      \param pdrawer value to which to initalize \ref m_drawer, value is not copied, only the reference is saved
      \param pdraw_order value to which to initalize \ref m_draw_order, value is not copied, only the reference is saved
      \param extra_state value to which to initalize \ref m_extra_state, value is not copied, only the reference is saved
     */
    TextWidgetCreator(enum text_opacity_t opacity,
                      const TextDrawerPacker &pdrawer,
                      const TextDrawOrder &pdraw_order,
                      const TextExtraDrawState &extra_state):
      m_opacity(opacity),
      m_drawer(pdrawer),
      m_draw_order(pdraw_order),
      m_extra_state(extra_state)
    {}
    
    /*!\fn TextWidget* operator()(ParentWidget*) const
      Create a new TextWidget whose parent is the passed
      parent widget. 
      \param c parent widget to be of the returned new TextWidget
     */
    template<typename ParentWidget>
    TextWidget*
    operator()(ParentWidget *c) const
    {
      return WRATHNew TextWidget(c, m_opacity, m_drawer, 
                                 m_draw_order, m_extra_state);
    }

    /*!\var m_opacity
      Opacity to use for TextWidget objects constructed
      by TextWidget* operator()(ParentWidget*) const
     */
    enum text_opacity_t m_opacity;

    /*!\var m_drawer
      Reference to \ref WRATHTextItemTypes::TextDrawerPacker to use for TextWidget objects constructed
      by operator()(ParentWidget*) const
     */
    const TextDrawerPacker &m_drawer;

    /*!\var m_draw_order
      Reference to \ref WRATHTextItemTypes::TextDrawOrder to use for TextWidget objects constructed
      by operator()(ParentWidget*) const
     */
    const TextDrawOrder &m_draw_order;

    /*!\var m_extra_state
      Reference to \ref WRATHTextItemTypes::TextExtraDrawState to use for TextWidget objects constructed
      by operator()(ParentWidget*) const
     */
    const TextExtraDrawState &m_extra_state;
  };

  /*!\class RectWidgetCreator
    Class to specify how to create a \ref WRATHRectWidget\<T\>.
    \tparam RectWidget must be for some class T, \ref WRATHRectWidget\<T\> 
   */
  template<typename RectWidget>
  class RectWidgetCreator
  {
  public:
    /*!\fn RectWidgetCreator
      Ctor 
      \param drawer value to which to initialize \ref m_drawer
     */
    explicit
    RectWidgetCreator(const WRATHRectItem::Drawer &drawer):
      m_drawer(drawer)
    {}

    /*!\fn RectWidget* operator()(ParentWidget*) const
      Create a new Rect whose parent is the passed parent widget. 
      \param c parent widget to be of the returned new RectWidget
     */
    template<typename ParentWidget>
    RectWidget*
    operator()(ParentWidget *c) const
    {
      return WRATHNew RectWidget(c, m_drawer);
    }

    /*!\var m_drawer
      Reference to \ref WRATHRectItem::Drawer to use 
      for RectWidget objects constructed
      by RectWidget* operator()(ParentWidget*) const
     */
    const WRATHRectItem::Drawer &m_drawer;
  };

  /*!\class ShapeWidgetCreator
    Class to specify how to create a \ref WRATHShapeWidget\<WidgetBase\>.    
    The class does NOT store it's values passed to its
    ctor, rather it saves the refernces. Thus one 
    should not "save" these objects unless the objects
    passed at the ctor will stay in scope.
    \tparam ShapeWidget must be for some class WidgetBase, 
            \ref WRATHShapeWidget\<WidgetBase\>
    \tparam T the type for the WRATHShape\<T\> 
   */
  template<typename ShapeWidget, typename T>
  class ShapeWidgetCreator
  {
  public:
    /*!\fn ShapeWidgetCreator
      Ctor.
      \param pShape value to which to initialize \ref m_shape
      \param drawer value to which to initialize \ref m_drawer
      \param P value to which to initialize \ref m_P
     */
    ShapeWidgetCreator(const shape_valueT<T> &pShape,
                       const ShapeDrawer<T> &drawer,
                       const WRATHShapeAttributePackerBase::PackingParametersBase &P):
      m_shape(pShape),
      m_drawer(drawer),
      m_P(P)
    {}

    /*!\fn ShapeWidget* operator()(ParentWidget*) const
      Create a new ShapeWidget whose parent is the passed parent widget. 
      \param c parent widget to be of the returned new ShapeWidget
     */
    template<typename ParentWidget>
    ShapeWidget*
    operator()(ParentWidget *c) const
    {
      return WRATHNew ShapeWidget(c, m_shape, m_drawer, m_P);
    }
    
    /*!\var m_shape
      Reference to shape value passed to ctor of ShapeWidget in  
      ShapeWidget* operator()(ParentWidget*) const
     */
    const shape_valueT<T> &m_shape;
    
    /*!\var m_drawer
      Reference to drawer value passed to ctor of ShapeWidget in  
      ShapeWidget* operator()(ParentWidget*) const
     */
    const ShapeDrawer<T> &m_drawer;
    
    /*!\var m_P
      Reference to packing parameters value passed to ctor of ShapeWidget in  
      ShapeWidget* operator()(ParentWidget*) const
     */
    const WRATHShapeAttributePackerBase::PackingParametersBase &m_P;
  };
  
  /*!\class NullItemProperties
    Template conveniance class for when
    an item does not have properties to 
    be set.
   */
  class NullItemProperties
  {
  public:
    /*!\fn void operator()(Widget*) const
      Does nothing, i.e. the function
      of the functor NullItemProperties
      is a no-op
     */
    template<typename Widget>
    void
    operator()(Widget *) const
    {}
  };

  /*!\class TextItemProperties
    A class to hold the parameter values
    to set the text of a \ref WRATHTextItem.
    The parameters themselves are stored
    via const-references.
   */
  class TextItemProperties
  {
  public:
    /*!\fn TextItemProperties
      Ctor.
      \param R range specifying what range within
               a stream of text to print
      \param ptext formatted text
      \param state_stream associated steate stream for ptext
     */
    TextItemProperties(range_type<int> R,
                       const WRATHFormattedTextStream &ptext,
                       const WRATHStateStream &state_stream):
      m_R(R),
      m_text(ptext),
      m_state_stream(state_stream)
    {}      

    /*!\fn void operator()(TextWidget *)
      Template-conveniance function for setting
      the text of a TextWidget as according to
      this TextItemProperties object.
      \param p pointer to widget whose properties() is a WRATHTextItem
               to set
     */
    template<typename TextWidget>
    void
    operator()(TextWidget *p) const
    {
      p->properties()->clear();
      p->properties()->add_text(m_R, m_text, m_state_stream);
    }

    /*!\var m_R
      Range specifying what range within
      a stream of text to print
    */
    range_type<int> m_R;

    /*!\var m_text
      Stream of formatted text. Note that
      \ref m_text is a _reference_.
     */
    const WRATHFormattedTextStream &m_text;

    /*!\var m_state_stream
      State stream associated to \ref m_text.
      Note that \ref m_state_stream is a 
      _reference_.
     */
    const WRATHStateStream &m_state_stream;
  };

  /*!\class RectItemProperties
    Wrapper over WRATHReferenceCountedObject::handle
    for the purpose of providing operator().
   */
  class RectItemProperties:
    public WRATHReferenceCountedObject::handle
  {
  public:
    /*!\fn RectItemProperties
      Ctor.
      \param rect rectange parameters passed to \ref WRATHRectItem::set_parameters()
     */
    explicit
    RectItemProperties(const WRATHReferenceCountedObject::handle &rect):
      WRATHReferenceCountedObject::handle(rect)
    {}

    /*!\fn void operator()(Widget*) const
      Template-conveniance function for setting
      the parameters of a WRATHRectItem of a widget type.
      \param p pointer to widget whose properties() is a WRATHRectItem
     */
    template<typename Widget>
    void
    operator()(Widget *p) const
    {
      p->properties()->set_parameters(*this);
    }
  };

  /*!\class LinearGradientProperties
    Class to hold the position values for a
    linear gradient whose positional values
    are stored on an item node. The contents of a
    LinearGradientProperties are not references
    but actual vec2 objects.
   */
  class LinearGradientProperties
  {
  public:
    /*!\fn LinearGradientProperties
      Ctor.
      \param pstart value to which to assign to \ref m_start
      \param pend value to which to assign to \ref m_end
     */
    LinearGradientProperties(const vec2 &pstart=vec2(0.0f, 0.0f),
                             const vec2 &pend=vec2(1.0f, 1.0f)):
      m_start(pstart),
      m_end(pend)
    {}

    /*!\fn void operator()(LinearGradientWidget *) const
      Template-conveniance function for setting
      the position of the linear gradient
      \param p pointer to widget whose node() has the method 
               set_gradient(const vec2 &start, const vec2 &end).
     */
    template<typename LinearGradientWidget>
    void
    operator()(LinearGradientWidget *p) const
    {
      p->node()->set_gradient(m_start, m_end);
    }

    /*!\var m_start
      Specifies the start position
      of the linear gradient.
     */
    vec2 m_start;

    /*!\var m_end
      Specifies the end position
      of the linear gradient.
     */
    vec2 m_end;
  };

  /*!\class RadialGradientProperties
    Class to hold the position values for a
    radial gradient whose positional values
    are stored on an item node. The contents of a
    RadialGradientProperties are not references
    but actual float and vec2 objects.
   */
  class RadialGradientProperties
  {
  public:
    /*!\fn RadialGradientProperties
      Ctor.
      \param pstart value to which to assign to \ref m_start
      \param pend value to which to assign to \ref m_end
      \param pstart_r value to which to assign to \ref m_start_r
      \param pend_r value to which to assign to \ref m_end_r
     */
    RadialGradientProperties(const vec2 &pstart=vec2(0.0f, 0.0f),
                             float pstart_r=0.0f,
                             const vec2 &pend=vec2(1.0f, 1.0f),
                             float pend_r=1.0f):
      m_start(pstart),
      m_start_r(pstart_r),
      m_end(pend),
      m_end_r(pend_r)
    {}

    /*!\fn void operator()(RadialGradientWidget *) const
      Template-conveniance function for setting
      the position of the linear gradient
      \param p pointer to widget whose node() has the method 
               set_gradient(const vec2 &start, float start_r, const vec2 &end, float end_r).
     */
    template<typename RadialGradientWidget>
    void
    operator()(RadialGradientWidget *p) const
    {
      p->node()->set_gradient(m_start, m_start_r, m_end, m_end_r);
    }

    /*!\var m_start
      Specifies the start position
      of the radial gradient.
     */
    vec2 m_start;

    /*!\var m_start_r
      Specifies the start radius
      of the radial gradient.
     */
    float m_start_r;

    /*!\var m_end
      Specifies the end position
      of the radial gradient.
     */
    vec2 m_end;

    /*!\var m_end_r
      Specifies the end radius
      of the radial gradient.
     */
    float m_end_r;
  };

  /*!\class ColorProperties
    Class to hold a color value stored
    on an item node.
   */
  class ColorProperties
  {
  public:
    /*!\typedef color_type
      Local typedef to \ref WRATHGradient::color
     */
    typedef WRATHGradient::color color_type;

    /*!\fn ColorProperties
      Ctor.
      \param c value to which to initialize \ref m_value
     */
    explicit
    ColorProperties(const color_type &c=vec4(1.0f, 1.0f, 1.0f, 1.0f)):
      m_value(c)
    {}

    /*!\fn void operator()(ColorValueWidget *) const
      Template-conveniance function for setting
      the color stored at a widget's node.
      \param p pointer to widget whose node() has the method color().
     */
    template<typename ColorValueWidget>
    void
    operator()(ColorValueWidget *p) const
    {
      p->node()->color(m_value);
    }

    /*!\var m_value
      Value to set the color as in the void operator()(ColorValueWidget *) const
     */
    color_type m_value;
  };

  /*!\class CompositeProperties
    A Compositve represents 2 item property
    setter objects that are applied to an item.
   */
  template<typename T1, typename T2>
  class CompositeProperties
  {
  public:
    /*!\fn CompositeProperties
      Ctor.
      \param t1 first properties functor to apply to widget in void operator()(Widget*) const
      \param t2 second properties functor to apply to widget in void operator()(Widget*) const
     */ 
    CompositeProperties(const T1 &t1, const T2 &t2):
      m_t1(t1), m_t2(t2)
    {}

    /*!\fn void operator()(Widget*) const
      Apply properties functors passed in ctor to widget.
      \param w widget to which to apply properties functors
     */
    template<typename Widget>
    void
    operator()(Widget *w) const
    {
      m_t1(w);
      m_t2(w);
    }

  private:
    const T1 m_t1;
    const T2 m_t2;
  };

  /*!\fn Composite(const T1 &a, const T1 &b)
    Provided as a template programming conveniance,
    equivalent to
    \code
    CompositeProperties<T1, T2>(a,b);
    \endcode
    \param a first item property setter to pass along
    \param b second item property setter to pass along
   */
  template<typename T1, typename T2>
  CompositeProperties<T1, T2>
  Composite(const T1 &a, const T1 &b)
  {
    return CompositeProperties<T1, T2>(a,b);
  }

  /*!\fn Text(range_type<int>,
              const WRATHFormattedTextStream &,
              const WRATHStateStream &)
    Overload function for generating a 
    parameter values for setting text.
    \param R range of characters in text stream to use
    \param ptext foramtted text stream 
    \param state_stream state changes associated to text stream
   */
  inline
  TextItemProperties
  Text(range_type<int> R,
       const WRATHFormattedTextStream &ptext,
       const WRATHStateStream &state_stream)
  {
    return TextItemProperties(R, ptext, state_stream);
  }

  /*!\fn Text(const WRATHFormattedTextStream &,
              const WRATHStateStream &)
    Overload function for generating a 
    parameter values for setting text.
    \param ptext foramtted text stream 
    \param state_stream state changes associated to text stream
   */
  inline
  TextItemProperties
  Text(const WRATHFormattedTextStream &ptext,
       const WRATHStateStream &state_stream)
  {
    return TextItemProperties(range_type<int>(0, ptext.data_stream().size()), 
                              ptext, state_stream);
  }

  /*!\fn Text(const WRATHTextDataStream &)
    Overload function for generating a 
    parameter values for setting text.
    \param ptext text stream 
   */
  inline
  TextItemProperties
  Text(const WRATHTextDataStream &ptext)
  {
    return TextItemProperties(range_type<int>(0, ptext.formatted_text().data_stream().size()),
                              ptext.formatted_text(), ptext.state_stream());
  }

  /*!\fn Rect(const WRATHReferenceCountedObject::handle &)
    Overload function for generating a 
    parameter values. 
    \param rect rectangle properties to pass along
   */
  inline
  RectItemProperties
  Rect(const WRATHReferenceCountedObject::handle &rect)
  {
    return RectItemProperties(rect);
  }

  /*!\fn Rect(const vec2 &, float z)
    Overload function for generating a 
    parameter values. 
    \param width_height dimenstions of rectangle
    \param z geometric-z used in perspective transformation 
   */
  inline
  RectItemProperties
  Rect(const vec2 &width_height, float z=-1.0f)
  {
    return RectItemProperties(WRATHNew WRATHDefaultRectAttributePacker::Rect(width_height, z));
  }

  /*!\fn Rect(float, float, float)
    Overload function for generating a 
    parameter values. 
    \param width width of rectangle
    \param height height of rectangle
    \param z geometric-z used in perspective transformation 
   */
  inline
  RectItemProperties
  Rect(float width, float height, float z=-1.0f)
  {
    return Rect(vec2(width, height), z);
  }
  
  /*!\class WidgetCounter
    A WidgetCounter stores counts generated when using
    \ref WRATHWidgetGeneratorT to add or manipulate
    widgets
   */
  class WidgetCounter
  {
  public:
    /*!\fn WidgetCounter
      Ctor.
     */
    WidgetCounter(void):
      m_number_nodes(0),
      m_number_items(0),
      m_number_canvases(1), //init as one since at ctor WRATHWidgetGeneratorT needs a canvas.
      m_number_contructed_items(0)
    {}

    /*!\var m_number_nodes
      Number of node widgets noted by WRATHWidgetGeneratorT
     */
    int m_number_nodes;

    /*!\var m_number_items
      Number of node items noted by WRATHWidgetGeneratorT
     */
    int m_number_items;

    /*!\var m_number_canvases
      Number of canvas items noted by WRATHWidgetGeneratorT
     */
    int m_number_canvases;

    /*!\var m_number_contructed_items
      Number of constructed items noted by WRATHWidgetGeneratorT
     */
    int m_number_contructed_items;
  };
};


/*!\class WRATHWidgetGeneratorT
  A WRATHWidgetGeneratorT provides an imperative 
  interface to create widgets. Widgets are either
  created or modified in each call. A WRATHWidgetGeneratorT
  maintains a stack of \ref WRATHEmptyWidget pointers.
  For reference we will call this stack the NodeStack.
  That stack represents a heierarchy transformation 
  applied to created/modified widgets. In addition the
  widget at the top of the stack will be the parent
  of created/modified widgets. The stack can be pushed
  and popped. 

  The add_ methods of a WRATHWidgetGeneratorT add
  a widget to the canvas that is in use by the
  node at the top of it's NodeStack.
  A given item has state. 

  The core add_ method from which all other add_ methods
  rely upon is add_generic():

  \code   
  add_generic(WidgetHandle &widget,
              const WidgetPropertySetter &P,
              const WidgetCreator &C)
  \endcode

  Acts on widget_handle W, node values taken from N
  (if N is non-NULL).  The mutable item properties are 
  specified with WidgetPropertySetter P and the immutable 
  item properties are specified with WidgetCreator C.
  
  Examples of WidgetCreator are found in WRATHWidgetGenerator
  namespace:
  - WRATHWidgetGenerator::TextWidgetCreator
  - WRATHWidgetGenerator::RectWidgetCreator
  - WRATHWidgetGenerator::ShapeWidgetCreator

  Examples of WidgetPropertySetter are found in WRATHWidgetGenerator
  namespace:
  - WRATHWidgetGenerator::RectItemProperties,
  - WRATHWidgetGenerator::LinearGradientProperties,
  - WRATHWidgetGenerator::ColorProperties,
  - WRATHWidgetGenerator::CompositeProperties,
  - WRATHWidgetGenerator::TextItemProperties
  
  All other add_foo method of WRATHWidgetGeneratorT rely on
  add_generic passing a WidgetCreator and WidgetPropertySetter
  typed based upon the types passed to the add_foo method.

  WRATHWidgetGeneratorT is to inherit from a type defined
  by the template class \ref WRATHFamilySet. The expectation
  is that the widget handle types degined by the families defined 
  by WRATHFamilySet are used for each of the add_* methods.
  (For example add_text()'s first argument should be of a
  type pFamilySet::FooFamily::DrawnText where FooFamily
  is one of the many family sets defined in \ref WRATHFamilySet).
  
  \tparam pFamilySet type defined using the template type \ref WRATHFamilySet

*/
template<typename pFamilySet>
class WRATHWidgetGeneratorT: public pFamilySet
{
public:  

  /*!\typedef FamilySet
    Conveniance typedef to cut down on typing.
   */
  typedef pFamilySet FamilySet; 

  /*!\class WidgetCreators
    Conveniance typedef machine, view this class 
    as an implementation detail.
    \param W \ref WRATHWidgetBase created type 
   */
  template<typename W>
  class WidgetCreators:public WRATHFamily<W>
  {
  public:
    /*!\typedef TextCreator
      typedef for \ref WRATHWidgetGenerator::TextWidgetCreator
      giving the correct widget type from the \ref WRATHWidgetBase 
      created type W
     */ 
    typedef WRATHWidgetGenerator::TextWidgetCreator<typename WRATHFamily<W>::TextWidget> TextCreator;

    /*!\typedef RectCreator
      typedef for \ref WRATHWidgetGenerator::RectWidgetCreator
      giving the correct widget type from the \ref WRATHWidgetBase 
      created type W
     */ 
    typedef WRATHWidgetGenerator::RectWidgetCreator<typename WRATHFamily<W>::RectWidget> RectCreator;

    /*!\class ShapeCreator
      typedef machine for using \ref WRATHWidgetGenerator::ShapeWidgetCreator
      \tparam T template parameter to \ref WRATHShape that creator consumes
     */ 
    template<typename T>
    class ShapeCreator
    {
    public:
      /*!\typedef Creator
        Typedef to \ref WRATHWidgetGenerator::ShapeWidgetCreator
        for the shape type and WRATHWidgetBase type W
       */
      typedef WRATHWidgetGenerator::ShapeWidgetCreator<typename WRATHFamily<W>::ShapeWidget, T> Creator;
    };
  };

  /*!\class ShapeCreator
    Template class defining type to make shape
    item creation easier to type, view this class 
    as an implementation detail.
    \tparam W WidgetType (not \ref WRATHWidgetBase derived type).
    \tparam T template parameter to \ref WRATHShape that creator consumes
   */
  template<typename W, typename T>
  class ShapeCreator
  {
  public:
    /*!\typedef WidgetBase
      Conveniance local typedef 
     */
    typedef typename W::WidgetBase WidgetBase;

    /*!\typedef Intermediate
      Conveniance local typedef 
     */
    typedef typename WidgetCreators<WidgetBase>::template ShapeCreator<T> Intermediate;

    /*!\typedef type
      typedef to \ref WRATHWidgetGenerator::ShapeWidgetCreator
     */
    typedef typename Intermediate::Creator type;
  };

  /*!\typedef Node 
    The underlying node type for the WRATHWidgetGeneratorT 's
    transformation/clipping stack. The type itself should
    represent a transformation and rectangular clipping
    of a hierarchy of such nodes.
   */
  typedef typename FamilySet::Node Node;

  /*!\typedef Canvas
    The WRATHCanvas derived type that all drawn items
    are to be attached to. 
   */
  typedef typename FamilySet::Canvas Canvas;

  /*!\typedef NodeWidget
    Typedef of widget type that has an empty item.
    Such a widget can be used at the ctor of
    WRATHWidgetGeneratorT to specify the ancestor
    of all widgets created/modified by the 
    WRATHWidgetGeneratorT.
  */
  typedef typename FamilySet::PlainFamily::NodeWidget NodeWidget;

  /*!\typedef NodeHandle
    Non-copyable handle representing a transformation/clipping
    node entry on a WRATHWidgetGeneratorT 's node stack.
   */
  typedef typename FamilySet::PlainFamily::NodeHandle NodeHandle;

  /*!\typedef DrawnCanvas
    Typedef for Canvas widgets (i.e. those that represent
    drawing a Canvas).
   */
  typedef typename FamilySet::PlainFamily::DrawnCanvas DrawnCanvas; 

  /*!\typedef CanvasHandle 
    Handle to a Canvas.
   */
  typedef WRATHCanvasHandleT<Canvas> CanvasHandle;
  
  /*!\class CanvasClipper
    A CanvasClipper is a proxy with which one
    adds clipping to a canvas (removing clipping
    from a canvas is accomplished by simply deleting
    \ref WRATHWidgetHandle::widget() of the item
    added for clipping).
   */
  class CanvasClipper
  {
  public:
    /*!\fn CanvasClipper 
      Default ctor creates an invalid CanvasClipper
     */
    CanvasClipper(void):
      m_w(NULL)
    {}
    
    /*!\fn CanvasClipper push_node
      Functionally, a CanvasClipper has associated to it
      a stack of _node_ transformations. One can push
      and pop that stack of node transformations so
      that clipping items added via CanvasClipper
      can have a transformation heirarchy specified
      by being added. The stack is _reset_ whenever
      \ref WRATHWidgetGeneratorT::push_canvas_node(DrawnCanvas&)
      or WRATHWidgetGeneratorT::canvas_clipping(void) is called.
      \param smart_widget \ref WRATHWidgetHandle object
                          holding the underlying WRATHNodeWidget
                          which represents the node pushed
     */
    CanvasClipper
    push_node(NodeHandle &smart_widget)
    {
      NodeWidget *p(smart_widget.widget());
      pre_treat_widget_implement(p, m_w->m_clip_stack.back());
  
      if(p==NULL)
        { 
          p=WRATHNew NodeWidget(m_w->m_clip_stack.back());
        }
      
      m_w->m_clip_stack.push_back(p);
      smart_widget.widget(p);

      if(Node::z_order_type==WRATHLayerItemNodeDepthType::hierarchical_ordering)
        {
          /*
            only needed when node type's z-ordering is hierarchical,
            it needs to be infront of any siblings.
          */
          m_w->m_stack.back().m_canvas->add_clip_out_item(p);
          p->global_z_order_consumes_slot(false);
        }

      ++m_w->m_counters.m_number_nodes;

      return *this;
    }

    /*!\fn pop_node
      Pop the transformation stack for
      adding clipping items, see also
      \ref push_node().
     */
    CanvasClipper
    pop_node(void)
    {
      WRATHassert(m_w!=NULL);
      WRATHassert(m_w->m_clip_stack.size()>1);
      m_w->m_clip_stack.pop_back();
      return *this;
    }

    /*!\fn CanvasClipper clip_generic(WidgetHandle &,
                                      const WidgetPropertySetter &,
                                      const WidgetCreator &,
                                      bool)
       Add a generic item for clipping.
       \tparam WidgetHandle widget _handle_ type, i.e one of
                            \ref WRATHWidgetHandle\<T\>
                            or \ref WRATHWidgetHandleAutoDelete\<T\>
                            for some widget type T.
       \tparam WidgetCreator type that creates a new Widget
                             with operator()(NodeWidget*)
                             with the new widget's parent
                             being the argument to operator().
       \tparam WidgetPropertySetter type that sets a Widget
                                    item properties via the
                                    method operator()(W::Widget*).

       \param widget widget handle to handle the new widget; if the handle already 
                     has a widget that widget is deleted
       \param P functor object to set the widget item properties
       \param C functor object to create a the widget
       \param is_clip_out if true item specifies clip out region, otherwise
                          item specifies a clip in region
     */
    template<typename WidgetHandle,
             typename WidgetPropertySetter,
             typename WidgetCreator>
    CanvasClipper
    clip_generic(WidgetHandle &widget,
                 const WidgetPropertySetter &P,
                 const WidgetCreator &C,
                 bool is_clip_out) const
    {
      WRATHassert(m_w!=NULL);
      int z(0);

      WRATHassert(m_w->m_stack.back().m_canvas!=NULL);
      m_w->add_generic(widget, P, C, m_w->m_clip_stack.back(), z);

      if(is_clip_out)
        {
          m_w->m_stack.back().m_canvas->add_clip_out_item(widget.widget());
        }
      return *this;
    }

    /*!\fn CanvasClipper clip_text(enum WRATHWidgetGenerator::canvas_clip_t,
                                   W &smart_widget, 
                                   WRATHWidgetGenerator::TextItemProperties ptext,
                                   const WRATHWidgetGenerator::TextDrawerPacker&,
                                   const WRATHWidgetGenerator::TextExtraDrawState &) const
       Add a text item for clipping.
       \tparam W handle type to a text widget, for example WRATHWidgetHandle\<WRATHTextWidget\<N\> \>
                 and WRATHWidgetHandleAutoDelete\<WRATHTextWidget\<N\> \> for
                 some node type N where N is derived from \ref Node.
                 The most common types being \ref WRATHFamily::DrawnText
                 and \ref WRATHFamily::DrawnText::AutoDelete
       \param smart_widget widget handle to handle the new widget; if the handle already 
                           has a widget that widget is deleted
       \param wtype specifies if text item is to be clip inside or clip outside
       \param ptext \ref WRATHWidgetGenerator::TextItemProperties specifies what text to draw
       \param pdrawer specifies the default drawer of the text (i.e. how to draw the text
                      for when those characaters of the \ref WRATHTextDataStream don't specify
                      the text).
       \param extra_state specifies additional draw state of the text item of the widget
     */
    template<typename W>
    CanvasClipper
    clip_text(enum WRATHWidgetGenerator::canvas_clip_t wtype,
              W &smart_widget, 
              WRATHWidgetGenerator::TextItemProperties ptext,
              const WRATHWidgetGenerator::TextDrawerPacker &pdrawer
              =&WRATHFontShaderSpecifier::default_non_aa(),
              const WRATHWidgetGenerator::TextExtraDrawState &extra_state
              =WRATHWidgetGenerator::TextExtraDrawState()) const
    {      
      WRATHWidgetGenerator::TextDrawOrder pdraw_order;
      enum WRATHDrawType::draw_type_t ptype;

      ptype=WRATHWidgetGenerator::convert_type(wtype);
      pdraw_order.m_pass_specifier=WRATHTextureFontDrawer::clip_pass_specifier(ptype);

      typename WidgetCreators<typename W::WidgetBase>::TextCreator cr(WRATHWidgetGenerator::text_opaque_non_aa, 
                                                                      pdrawer,  pdraw_order, extra_state);
      return clip_generic(smart_widget, ptext, cr, wtype==WRATHWidgetGenerator::clip_outside);
    }    

    /*!\fn CanvasClipper clip_shape(enum WRATHWidgetGenerator::canvas_clip_t,
                                    W &,
                                    const WRATHWidgetGenerator::shape_valueT<T> &,
                                    const WRATHShaderSpecifier*,
                                    const WRATHShapeAttributePacker<T>*,
                                    const WRATHShapeAttributePackerBase::PackingParametersBase &P) const
        Add a shape widget to clipping.
        \tparam W handle type to a shape widget, for example WRATHWidgetHandle\<WRATHShapeWidget\<N\> \>
                  and WRATHWidgetHandleAutoDelete\<WRATHShapeWidget\<N\> \> for
                  some node type N where N is derived from \ref Node.
                  The most common types being \ref WRATHFamily::DrawnShape
                  and \ref WRATHFamily::DrawnShape::AutoDelete
        \tparam T the type for the coordinates of a WRATHShape\<T\>
        \param smart_widget widget handle to handle the new widget; if the handle already 
                            has a widget that widget is deleted
        \param wtype specifies if text item is to be clip inside or clip outside
        \param pShape shape that the widget is to be
        \param shader specifies how the shape is drawn
        \param ppacker attribute packer with which to pack the attributes
        \param P additional attribute packing parameters to create the shape item
        \param pstate additional draw state to apply to the clip shape item
     */
    template<typename W, typename T>
    CanvasClipper
    clip_shape(enum WRATHWidgetGenerator::canvas_clip_t wtype,
               W &smart_widget,
               const WRATHWidgetGenerator::shape_valueT<T> &pShape,
               const WRATHShaderSpecifier *shader,
               const WRATHShapeAttributePacker<T> *ppacker,
               const WRATHShapeAttributePackerBase::PackingParametersBase &P,
               const WRATHSubItemDrawState &pstate=WRATHSubItemDrawState()) const
    {
      WRATHWidgetGenerator::ShapeDrawer<T> drawer(shader, ppacker); 
      typename WidgetCreators<typename W::WidgetBase>::template ShapeCreator<T>::Creator cr(pShape, drawer, P);
      enum WRATHDrawType::draw_type_t ptype;

      ptype=WRATHWidgetGenerator::convert_type(wtype);
      drawer.m_draw_passes[0].m_draw_type.m_type=ptype;
      drawer.m_draw_passes[0].m_draw_state=pstate;
      return clip_generic(smart_widget, WRATHWidgetGenerator::NullItemProperties(), cr, 
                          wtype==WRATHWidgetGenerator::clip_outside);
    }


    /*!\fn CanvasClipper clip_stroked_shape(enum WRATHWidgetGenerator::canvas_clip_t,
                                            W &,
                                            const WRATHWidgetGenerator::shape_valueT<T>&,
                                            const WRATHWidgetGenerator::StrokingParameters &)
        Add a stroked shape widget to clipping.
        \tparam W handle type to a shape widget, for example WRATHWidgetHandle\<WRATHShapeWidget\<N\> \>
                  and WRATHWidgetHandleAutoDelete\<WRATHShapeWidget\<N\> \> for
                  some node type N where N is derived from \ref Node.
                  The most common types being \ref WRATHFamily::DrawnShape
                  and \ref WRATHFamily::DrawnShape::AutoDelete
        \tparam T the type for the coordinates of a WRATHShape\<T\>
        \param smart_widget widget handle to handle the new widget; if the handle already 
                            has a widget that widget is deleted
        \param wtype specifies if text item is to be clip inside or clip outside
        \param pShape shape that the widget is to be
        \param P stroking parameters of the clipping shape (line width, join style, etc)
     */
    template<typename W, typename T>
    CanvasClipper
    clip_stroked_shape(enum WRATHWidgetGenerator::canvas_clip_t wtype,
                       W &smart_widget,
                       const WRATHWidgetGenerator::shape_valueT<T> &pShape,
                       const WRATHWidgetGenerator::StrokingParameters &P
                       =WRATHWidgetGenerator::StrokingParameters()) const
    {
      return clip_shape(wtype, smart_widget, pShape,
                        &WRATHDefaultShapeShader::shader_simple(),
                        WRATHDefaultStrokeAttributePackerT<T>::fetch(), 
                        P);
                       
    }

    /*!\fn CanvasClipper clip_filled_shape(enum WRATHWidgetGenerator::canvas_clip_t,
                                           W &,
                                           const WRATHWidgetGenerator::shape_valueT<T>&,
                                           const WRATHWidgetGenerator::StrokingParameters &)
        Add a filled shape widget to clipping.
        \tparam W handle type to a shape widget, for example WRATHWidgetHandle\<WRATHShapeWidget\<N\> \>
                  and WRATHWidgetHandleAutoDelete\<WRATHShapeWidget\<N\> \> for
                  some node type N where N is derived from \ref Node.
                  The most common types being \ref WRATHFamily::DrawnShape
                  and \ref WRATHFamily::DrawnShape::AutoDelete
        \tparam T the type for the coordinates of a WRATHShape\<T\>
        \param smart_widget widget handle to handle the new widget; if the handle already 
                            has a widget that widget is deleted
        \param wtype specifies if text item is to be clip inside or clip outside
        \param pShape shape that the widget is to be
        \param P filling parameters of the clipping shape (including fill rule)
     */
    template<typename W, typename T>
    CanvasClipper
    clip_filled_shape(enum WRATHWidgetGenerator::canvas_clip_t wtype,
                      W &smart_widget,
                      const WRATHWidgetGenerator::shape_valueT<T> &pShape,
                      const WRATHWidgetGenerator::FillingParameters &P
                      =WRATHWidgetGenerator::FillingParameters()) const
    {
      return clip_shape(wtype, smart_widget, pShape,
                        &WRATHDefaultShapeShader::shader_simple(),
                        WRATHDefaultFillShapeAttributePackerT<T>::fetch(), 
                        P);
                       
    }

    /*!\fn CanvasClipper clip_rect(enum WRATHWidgetGenerator::canvas_clip_t,
                                   W&,
                                   const vec2&) const
        Add a rect widget to clipping.
        \tparam W handle type to a rect widget, for example WRATHWidgetHandle\<WRATHRectWidget\<N\> \>
                  and WRATHWidgetHandleAutoDelete\<WRATHRectWidget\<N\> \> for
                  some node type N where N is derived from \ref Node.
                  The most common types being \ref WRATHFamily::DrawnShape
                  and \ref WRATHFamily::DrawnShape::Auto
        \param smart_widget widget handle to handle the new widget; if the handle already 
                            has a widget that widget is deleted
        \param wtype specifies if text item is to be clip inside or clip outside
        \param width_height the width and height of the clipping rect                          
     */
    template<typename W>
    CanvasClipper
    clip_rect(enum WRATHWidgetGenerator::canvas_clip_t wtype,
              W &smart_widget,
              const vec2 &width_height) const
    {
      enum WRATHDrawType::draw_type_t ptype;

      ptype=WRATHWidgetGenerator::convert_type(wtype); 
      WRATHWidgetGenerator::RectDrawer dr(&WRATHDefaultRectShader::shader_simple(),
                                          WRATHDefaultRectAttributePacker::fetch(),
                                          WRATHDrawType(0, ptype));
      typename WidgetCreators<typename FamilySet::PlainFamily::WidgetBase>::RectCreator C(dr);
      

      return clip_generic(smart_widget, 
                          WRATHWidgetGenerator::Rect(width_height), 
                          C, wtype==WRATHWidgetGenerator::clip_outside);
    }
           
    /*
      TODO:
       - clip_shape using brush so that alpha test with discard do interesting things
       - clip_rect interface too
     */

  private:

    friend class WRATHWidgetGeneratorT;

    CanvasClipper&
    operator=(const CanvasClipper &obj)
    {
      m_w=obj.m_w;
      return *this;
    }

    CanvasClipper(const CanvasClipper &obj):
      m_w(obj.m_w)
    {}

    explicit
    CanvasClipper(WRATHWidgetGeneratorT *w):
      m_w(w)
    {
      WRATHassert(m_w!=NULL);
      m_w->m_clip_stack.clear();
      m_w->m_clip_stack.push_back(m_w->current());
    }

    WRATHWidgetGeneratorT *m_w;
  };

  /*!\class AutoPushNode
    Class that calls \ref WRATHWidgetGeneratorT::push_node() at ctor
    and WRATHWidgetGeneratorT::pop_node() at dtor.
    Debug build adds a check to make sure it pops
    the same node it pushed
   */
  class AutoPushNode:boost::noncopyable
  {
  public:
    /*!\fn AutoPushNode
      \param p WRATHWidgetGeneratorT onto which to push
      \param smart_widget node widget to push
     */ 
    AutoPushNode(WRATHWidgetGeneratorT *p,
                 NodeHandle &smart_widget):
      m_p(p)
    {
      m_p->push_node(smart_widget);
      debug_track();
    }

    ~AutoPushNode()
    {
      WRATHassert(m_pushed_node==m_p->stack_top());
      m_p->pop_node();
    }

  private:
    WRATHWidgetGeneratorT *m_p;

#ifdef WRATHDEBUG
    NodeWidget *m_pushed_node;

    void
    debug_track(void) 
    {
      m_pushed_node=m_p->m_stack.back().m_node_widget;
    }

#else
    void
    debug_track(void) {}
#endif
  };

  /*!\class AutoPushCanvasNode
    Class that calls \ref WRATHWidgetGeneratorT::push_canvas_node() at ctor
    and WRATHWidgetGeneratorT::pop_node() at dtor.
    Debug build adds a check to make sure it pops
    the same node it pushed
   */
  class AutoPushCanvasNode:boost::noncopyable
  {
  public:
    /*!\fn AutoPushCanvasNode
      \param p WRATHWidgetGeneratorT onto which to push
      \param canvas canvas widget to push
     */ 
    AutoPushCanvasNode(WRATHWidgetGeneratorT *p,
                       DrawnCanvas &canvas):
      m_p(p)
    {
      m_p->push_canvas_node(canvas);
      debug_track();
    }

    ~AutoPushCanvasNode()
    {
      WRATHassert(m_pushed_node==m_p->stack_top());
      m_p->pop_node();
    }

  private:
    WRATHWidgetGeneratorT *m_p;

#ifdef WRATHDEBUG
    NodeWidget *m_pushed_node;

    void
    debug_track(void) 
    {
      m_pushed_node=m_p->m_stack.back().m_node_widget;
    }

#else
    void
    debug_track(void) {}
#endif
  };

  
  /*!\fn WRATHWidgetGeneratorT(NodeWidget*, int&)
    Ctor.
    \param proot_widget will be the ancestor for all
                        widgets made with the WRATHWidgetGeneratorT
    \param pz _reference_ to an integer that is _decremented_
              whenever a widget is set/created with the
              WRATHWidgetGeneratorT
   */
  WRATHWidgetGeneratorT(NodeWidget *proot_widget, int &pz):
    m_z(pz),
    m_stack(1, proot_widget)
  {
    WRATHassert(m_stack.back().m_node_widget!=NULL);
  }

  /*!\fn WRATHWidgetGeneratorT(Canvas*, NodeHandle&, int&)
    Ctor
    \param pCanvas canvas on which to place widgets
    \param proot_widget reference to a NodeHandle which
                 holds the root widget of the created WidgetGeneratorT
    \param pz _reference_ to an integer that is _decremented_
              whenever a widget is set/created with the
              WRATHWidgetGeneratorT
   */
  WRATHWidgetGeneratorT(Canvas *pCanvas,
                        NodeHandle &proot_widget, int &pz);
  
  
  /*!\fn const WRATHWidgetGenerator::WidgetCounter& counters
    Returns the current count of items, nodes, etc, 
    added via this WRATHWidgetGeneratorT.
   */
  const WRATHWidgetGenerator::WidgetCounter&
  counters(void) { return m_counters; }

  /*!\fn int current_z
    Returns the current value of the z-index;
    this value is decremented when items are
    added, i.e. items on top are those with
    smaller z (remember that -100000 is smaller than -1)
   */
  int
  current_z(void) const { return m_z; }

  /*!\fn void push_node
    Pushes node values (i.e. transformation, clipping, visibility, etc)
    onto the node stack. Subsequent created/modified
    widgets become child widget's of smart_widget.widget().
    \param smart_widget \ref WRATHWidgetHandle object
                        holding the underlying WRATHNodeWidget
                        which represents the node pushed
  */
  void
  push_node(NodeHandle &smart_widget);

  /*!\fn void pop_node
    Pop the top of the widget transformation stack.
   */
  void
  pop_node(void);

  /*!\fn CanvasClipper push_canvas_node
    \param canvas holds the Canvas on which to change drawing to
   */
  CanvasClipper
  push_canvas_node(DrawnCanvas &canvas)
  {
    push_canvas_node_implement(canvas);
    return CanvasClipper(this);
  }

  /*!\fn NodeWidget* stack_top
    Returns a _pointer_ to the node widget at the
    top of the node stack.
   */
  NodeWidget*
  stack_top(void)
  {
    return current();
  }

  /*!\fn Canvas* canvas
    Returns the Canvas to which items added
    are added. This value changes when push_canvas_node()
    and pop_node() are called. Equivalent to:
    \code
    stack_top()->canvas()
    \endcode
   */
  Canvas*
  canvas(void)
  {
    return stack_top()->canvas();
  }

  /*!\fn CanvasClipper canvas_clipping
    Returns a CanvasClipper to canvas()
    for the purpose of adding clipping items
    to canvas().
   */
  CanvasClipper
  canvas_clipping(void)
  {
    return CanvasClipper(this);
  }

  /*!\fn int default_text_item_pass(void)
    Returns the draw order pass value 
    (as found in WRATHDrawType::m_value)
    for those text items added without 
    explictily specifying the 
    WRATHWidgetGenerator::TextDrawOrder object.
    Default value is 0.
   */
  int
  default_text_item_pass(void)
  {
    return m_default_text_item_pass;
  }

  /*!\fn void default_text_item_pass(int)
    Sets the draw order pass value 
    (as found in WRATHDrawType::m_value)
    for those text items added without 
    explicitly specifying the 
    WRATHWidgetGenerator::TextDrawOrder object.
    Default value is 0.
    \param v new value to use
   */
  void
  default_text_item_pass(int v)
  {
    m_default_text_item_pass=v;
  }

  /*!\fn const WRATHDrawType& default_rect_item_pass(void)
    Returns the the draw order pass value 
    (as found in WRATHDrawType::m_value)
    for those items added with add_rect()
    or add_rect() which do no explictily
    specify the WRATHDrawType values
    (such items are also drawn opaque).
   */
  const WRATHDrawType&
  default_rect_item_pass(void)
  {
    return m_default_rect_item_pass;
  }

  /*!\fn void default_rect_item_pass(const WRATHDrawType &)
    Sets the the draw order pass value 
    (as found in WRATHDrawType::m_value)
    for those items added with add_rect()
    or add_rect() which do no explictily
    specify the WRATHDrawType values
    (such items are also drawn opaque).
    \param v new value to use
   */
  void
  default_rect_item_pass(const WRATHDrawType &v)
  {
    m_default_rect_item_pass=v;
  }

  /*!\fn int default_stroke_item_pass(void)
    Returns the the draw order pass value 
    (as found in WRATHDrawType::m_value)
    for add stroked items (i.e. add_stroked_shape())
    that do not explicity specify the value.
    Default value is 0.
   */
  int
  default_stroke_item_pass(void)
  {
    return m_default_stroke_item_pass;
  }

  /*!\fn void default_stroke_item_pass(int)
    Sets the the draw order pass value 
    (as found in WRATHDrawType::m_value)
    for add stroked items (i.e. add_stroked_shape())
    that do not explicity specify the value.
    Default value is 0.
    \param v new value to use
   */
  void
  default_stroke_item_pass(int v)
  {
    m_default_stroke_item_pass=v;
  }

  /*!\fn int default_fill_item_pass(void)
    Returns the the draw order pass value 
    (as found in WRATHDrawType::m_value)
    for add filled items  (i.e. add_filled_shape())
    that do not explicity specify the value.
    Default value is 0.
   */
  int
  default_fill_item_pass(void)
  {
    return m_default_fill_item_pass;
  }

  /*!\fn void default_fill_item_pass(int)
    Sets the the draw order pass value 
    (as found in WRATHDrawType::m_value)
    for add filled items (i.e. add_filled_shape())
    that do not explicity specify the value.
    Default value is 0.
    \param v new value to use
   */
  void
  default_fill_item_pass(int v)
  {
    m_default_fill_item_pass=v;
  }

  /*!\fn enum WRATHWidgetGenerator::shape_opacity_t default_stroke_item_aa(void)
    Returns the default anti-aliasing applied
    for added stroked items (i.e. add_stroked_shape())
    that do not explicity specify the value.
    Default value is WRATHWidgetGenerator::shape_opaque_non_aa
   */
  enum WRATHWidgetGenerator::shape_opacity_t 
  default_stroke_item_aa(void)
  {
    return m_default_stroke_item_aa;
  }

  /*!\fn void default_stroke_item_aa(enum WRATHWidgetGenerator::shape_opacity_t)
    Sets the default anti-aliasing applied
    for added stroked items (i.e. add_stroked_shape())
    that do not explicity specify the value.
    Default value is WRATHWidgetGenerator::shape_opaque_non_aa
    \param v new value to use
   */
  void
  default_stroke_item_aa(enum WRATHWidgetGenerator::shape_opacity_t v)
  {
    m_default_stroke_item_aa=v;
  }

  /*!\fn enum WRATHWidgetGenerator::shape_opacity_t default_fill_item_aa(void)
    Returns the default anti-aliasing applied
    for added filled items (i.e. add_filled_shape())
    that do not explicity specify the value.
    Default value is WRATHWidgetGenerator::shape_opaque_non_aa
   */
  enum WRATHWidgetGenerator::shape_opacity_t 
  default_fill_item_aa(void)
  {
    return m_default_fill_item_aa;
  }

  /*!\fn void default_fill_item_aa(enum WRATHWidgetGenerator::shape_opacity_t)
    Sets the default anti-aliasing applied
    for added filled items (i.e. add_filled_shape())
    that do not explicity specify the value.
    Default value is WRATHWidgetGenerator::shape_opaque_non_aa
    \param v new value to use
   */
  void
  default_fill_item_aa(enum WRATHWidgetGenerator::shape_opacity_t v)
  {
    m_default_fill_item_aa=v;
  }

  /*!\fn void add_generic(WidgetHandle &,
                          const WidgetPropertySetter &,
                          const WidgetCreator &)
    Generic add routine. 
    \tparam WidgetHandle widget _handle_ type, i.e one of
                         \ref WRATHWidgetHandle\<T>
                         or \ref WRATHWidgetHandleAutoDelete\<T\>
                         for some widget type T.
    \tparam WidgetCreator type that creates a new Widget
                          with operator()(NodeWidget*)
                          with the new widget's parent
                          being the argument to operator().
    \tparam WidgetPropertySetter type that sets a Widget
                                 item properties via the
                                 method operator()(W::Widget*).

    \param widget widget handle to handle the new widget; if the handle already 
                  has a widget that widget is deleted
    \param P functor object to set the widget item properties
    \param C functor object to create a the widget
   */
  template<typename WidgetHandle,
           typename WidgetPropertySetter,
           typename WidgetCreator>
  void
  add_generic(WidgetHandle &widget,
              const WidgetPropertySetter &P,
              const WidgetCreator &C)
  {
    add_generic(widget, P, C, current(), m_z);
  }

  /*!\fn update_generic(WidgetHandle&)
    Assumes that the underlying widget of the
    widget handle is already created. Only sets
    the node data and z-value of the underlying widget.
    \param widget handle to widget to update the z-value
   */
  template<typename WidgetHandle>
  void
  update_generic(WidgetHandle &widget)
  {
    update_generic(widget, current(), m_z);
  }

 
  /*!\fn void add_text
    Add a text widget.
    \tparam W handle type to a text widget, for example WRATHWidgetHandle\<WRATHTextWidget\<N\> \>
              and WRATHWidgetHandleAutoDelete\<WRATHTextWidget\<N\> \> for
              some node type N where N is derived from \ref Node.
              The most common types being \ref WRATHFamily::DrawnText
              and \ref WRATHFamily::DrawnText::AutoDelete
    \param smart_widget widget handle to handle the new widget; if the handle already 
                        has a widget that widget is deleted
    \param ptext \ref WRATHWidgetGenerator::TextItemProperties specifies what text to draw
    \param opacity specifies opacity type of text
    \param pdrawer specifies the default drawer of the text (i.e. how to draw the text
                   for when those characaters of the \ref WRATHTextDataStream don't specify
                   the text).
    \param pdraw_order specifies the draw order of the text item of the widget
    \param extra_state specifies additional draw state of the text item of the widget
   */
  template<typename W>
  void
  add_text(W &smart_widget,
           WRATHWidgetGenerator::TextItemProperties ptext,
           enum WRATHWidgetGenerator::text_opacity_t opacity,
           const WRATHWidgetGenerator::TextDrawerPacker &pdrawer=WRATHWidgetGenerator::TextDrawerPacker(),
           boost::optional<const WRATHWidgetGenerator::TextDrawOrder&> pdraw_order=
           boost::optional<const WRATHWidgetGenerator::TextDrawOrder&>(),
           const WRATHWidgetGenerator:: TextExtraDrawState &extra_state=WRATHWidgetGenerator::TextExtraDrawState())
  { 
    WRATHWidgetGenerator::TextDrawOrder v(default_fill_item_pass());
    if(!pdraw_order)
      {
        const WRATHWidgetGenerator::TextDrawOrder& vv(v);
        pdraw_order=vv;
      }
    typename WidgetCreators<typename W::WidgetBase>::TextCreator cr(opacity, pdrawer, 
                                                                    pdraw_order.get(), extra_state);
    add_generic(smart_widget, ptext, cr);
  }

 
  /*!\fn void add_rect(W&, const RectPropertyType &, const WRATHWidgetGenerator::RectDrawer &)
    Add a rect widget.
    \tparam W widget handle type to a rect widget, for example WRATHWidgetHandle\<WRATHRectWidget\<N\> \>
              and WRATHWidgetHandleAutoDelete\<WRATHRectWidget\<N\> \> for
              some node type N where N is derived from \ref Node.
              The most common types being \ref WRATHFamily::DrawnRect 
              and \ref WRATHFamily::DrawnRect::AutoDelete
    \tparam RectPropertyType type that sets a Widget
                             rect-item properties via the
                             method operator()(W::Widget*).
    \param smart_widget widget handle to handle the new widget; if the handle already 
                        has a widget that widget is deleted
    \param params specifies the properties of the rect item of the widget 
    \param drawer specifies the drawer of the rect item of the widget
   */
  template<typename W, typename RectPropertyType>
  void
  add_rect(W &smart_widget,
           const RectPropertyType &params,
           const WRATHWidgetGenerator::RectDrawer &drawer)
  {
    typename WidgetCreators<typename W::WidgetBase>::RectCreator cr(drawer);
    add_generic(smart_widget, params, cr);
  }

  /*!\fn void add_rect(W&, const RectPropertyType&,
                       const WRATHWidgetGenerator::Brush &,
                       boost::optional<WRATHDrawType> pitem_pass,
                       enum WRATHBaseSource::precision_t)
    Add a rect widget.
    \tparam W handle type to a rect widget, for example WRATHWidgetHandle\<WRATHRectWidget\<N\> \>
              and WRATHWidgetHandleAutoDelete\<WRATHRectWidget\<N\> \> for
              some node type N where N is derived from \ref Node.
              The most common types being \ref WRATHFamily::DrawnRect 
              and \ref WRATHFamily::DrawnRect::AutoDelete
    \tparam RectPropertyType type that sets a Widget
                             rect-item properties via the
                             method operator()(W::Widget*).
    \param smart_widget widget handle to handle the new widget; if the handle already 
                        has a widget that widget is deleted
    \param params specifies the properties of the rect item of the widget                      
    \param pbrush specifies the brush to apply to the rect item of the widget
    \param pitem_pass specifies the item pass of the rect item of the widget                         
    \param v specifies the precision of the brush color computation                         
   */
  template<typename W, typename RectPropertyType>
  void
  add_rect(W &smart_widget,
           const RectPropertyType &params,
           const WRATHWidgetGenerator::Brush &pbrush=WRATHWidgetGenerator::Brush(),
           boost::optional<WRATHDrawType> pitem_pass
           =boost::optional<WRATHDrawType>(),
           enum WRATHBaseSource::precision_t v
           =WRATHBaseSource::mediump_precision)
  {
    WRATHBrush brush(pbrush.m_image, pbrush.m_gradient, pbrush.m_bits);
    brush.m_draw_state.absorb(pbrush.m_draw_state);
    W::Node::set_shader_brush(brush);
    WRATHWidgetGenerator::RectDrawer drawer(brush, 
                                            pitem_pass.get_value_or(m_default_rect_item_pass),
                                            v);
    add_rect(smart_widget, params, drawer);
    smart_widget.widget()->node()->set_from_brush(brush);
  }
    
  /*!\fn void add_shape(W&,
                        const WRATHWidgetGenerator::shape_valueT<T>&,
                        const WRATHWidgetGenerator::ShapeDrawer<T>&,
                        const WRATHShapeAttributePackerBase::PackingParametersBase&)
     Add a shape widget
     \tparam W handle type to a shape widget, for example WRATHWidgetHandle\<WRATHShapeWidget\<N\> \>
               and WRATHWidgetHandleAutoDelete\<WRATHShapeWidget\<N\> \> for
               some node type N where N is derived from \ref Node.
               The most common types being \ref WRATHFamily::DrawnShape
               and \ref WRATHFamily::DrawnShape::AutoDelete
     \tparam T the type for the coordinates of a WRATHShape\<T\>
     \param smart_widget widget handle to handle the new widget; if the handle already 
                        has a widget that widget is deleted
     \param pShape shape that the widget is to be
     \param drawer drawer with which to draw the shape item of the widget
     \param P additional attribute packing parameters to create the shape item
   */
  template<typename T, typename W>
  void
  add_shape(W &smart_widget,
            const WRATHWidgetGenerator::shape_valueT<T> &pShape,
            const WRATHWidgetGenerator::ShapeDrawer<T> &drawer,
            const WRATHShapeAttributePackerBase::PackingParametersBase &P
            =WRATHShapeAttributePackerBase::PackingParametersBase())
  {
    typename WidgetCreators<typename W::WidgetBase>::template ShapeCreator<T>::Creator cr(pShape, drawer, P);
    add_generic(smart_widget,  
                WRATHWidgetGenerator::NullItemProperties(), cr);
  }
  
  
  /*!\fn void add_shape(W&,
                        const Setter&,
                        const WRATHWidgetGenerator::shape_valueT<T>&,
                        const WRATHWidgetGenerator::ShapeDrawer<T>&,
                        const WRATHShapeAttributePackerBase::PackingParametersBase&)
     add a shape widget
     \tparam W handle type to a shape widget, for example WRATHWidgetHandle\<WRATHShapeWidget\<N\> \>
               and WRATHWidgetHandleAutoDelete\<WRATHShapeWidget\<N\> \> for
               some node type N where N is derived from \ref Node.
               The most common types being \ref WRATHFamily::DrawnShape
               and \ref WRATHFamily::DrawnShape::AutoDelete
     \tparam T the type for the coordinates of a WRATHShape<T>
     \tparam Setter type that sets a Widget item properties via the method operator()(W::Widget*).
     \param smart_widget widget handle to handle the new widget; if the handle already 
                        has a widget that widget is deleted
     \param setter sets the properties of the item of the widget
     \param pShape shape that the widget is to be
     \param drawer drawer with which to draw the shape item of the widget
     \param P additional attribute packing parameters to create the shape item
   */
  template<typename T, typename W, typename Setter>
  void
  add_shape(W &smart_widget,
            const Setter &setter,
            const WRATHWidgetGenerator::shape_valueT<T> &pShape,
            const WRATHWidgetGenerator::ShapeDrawer<T> &drawer,
            const WRATHShapeAttributePackerBase::PackingParametersBase &P=
            WRATHShapeAttributePackerBase::PackingParametersBase())
  {
    
    typename ShapeCreator<W, T>::type cr(pShape, drawer, P);
    add_generic(smart_widget, setter, cr);
  }

  /*!\fn void add_filled_shape(W&,
                               const Setter&,
                               const WRATHWidgetGenerator::shape_valueT<T>&,
                               const WRATHWidgetGenerator::Brush&,
                               const WRATHWidgetGenerator::FillingParameters&,
                               boost::optional<enum WRATHWidgetGenerator::shape_opacity_t>,
                               const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle&,
                               boost::optional<int>,
                               enum WRATHBaseSource::precision_t)
     Add a shape widget drawn filled.
     \tparam W handle type to a shape widget, for example WRATHWidgetHandle\<WRATHShapeWidget\<N\> \>
               and WRATHWidgetHandleAutoDelete\<WRATHShapeWidget\<N\> \> for
               some node type N where N is derived from \ref Node.
               The most common types being \ref WRATHFamily::DrawnShape
               and \ref WRATHFamily::DrawnShape::AutoDelete
     \tparam T the type for the coordinates of a WRATHShape<T>
     \tparam Setter type that sets a Widget item properties via the method operator()(W::Widget*).
     \param smart_widget widget handle to handle the new widget; if the handle already 
                        has a widget that widget is deleted
     \param setter sets the properties of the item of the widget 
     \param pShape shape that the widget is to be
     \param pbrush brush applied to shape (i.e. \ref WRATHImage and \ref WRATHGradient)
     \param P filling parameters for shape, including fill rule
     \param aa opacity type of item of widget
     \param h draw type specifier for item of widget
     \param pitem_pass item pass value for item of widget
     \param v precision to compute shader brush color values.
   */
  template<typename T, typename W, typename Setter>
  void
  add_filled_shape(W &smart_widget,
                   const Setter &setter,
                   const WRATHWidgetGenerator::shape_valueT<T> &pShape,
                   const WRATHWidgetGenerator::Brush &pbrush,
                   const WRATHWidgetGenerator::FillingParameters &P
                   =WRATHWidgetGenerator::FillingParameters(),
                   boost::optional<enum WRATHWidgetGenerator::shape_opacity_t> aa
                   =boost::optional<enum WRATHWidgetGenerator::shape_opacity_t>(),
                   const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h
                   =WRATHTwoPassDrawer::default_pass_specifier(),
                   boost::optional<int> pitem_pass=boost::optional<int>(),
                   enum WRATHBaseSource::precision_t v
                   =WRATHBaseSource::mediump_precision)
  {
    WRATHBrush brush(pbrush.m_image, pbrush.m_gradient, pbrush.m_bits);
    brush.m_draw_state.absorb(pbrush.m_draw_state);
    W::Node::set_shader_brush(brush);
    WRATHWidgetGenerator::ShapeDrawer<T> drawer(WRATHWidgetGenerator::fill_shape,
                                                brush, 
                                                aa.get_value_or(default_fill_item_aa()), 
                                                h, pitem_pass.get_value_or(default_fill_item_pass()), v);
    add_shape(smart_widget, setter, pShape, drawer, P);
    smart_widget.widget()->node()->set_from_brush(brush);
  }

  /*!\fn void add_filled_shape(W&,
                               const Setter&,
                               const WRATHWidgetGenerator::shape_valueT<T>&,
                               const WRATHWidgetGenerator::FillingParameters&,
                               boost::optional<enum WRATHWidgetGenerator::shape_opacity_t>,
                               const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle&,
                               boost::optional<int>,
                               enum WRATHBaseSource::precision_t)
     Add a shape widget drawn filled.
     \tparam W handle type to a shape widget, for example WRATHWidgetHandle\<WRATHShapeWidget\<N\> \>
               and WRATHWidgetHandleAutoDelete\<WRATHShapeWidget\<N\> \> for
               some node type N where N is derived from \ref Node.
               The most common types being \ref WRATHFamily::DrawnShape
               and \ref WRATHFamily::DrawnShape::AutoDelete
     \tparam T the type for the coordinates of a WRATHShape<T>
     \tparam Setter type that sets a Widget item properties via the method operator()(W::Widget*).
     \param smart_widget widget handle to handle the new widget; if the handle already 
                        has a widget that widget is deleted
     \param setter sets the properties of the item of the widget 
     \param pShape shape that the widget is to be
     \param P filling parameters for shape, including fill rule
     \param aa opacity type of item of widget
     \param h draw type specifier for item of widget
     \param pitem_pass item pass value for item of widget
     \param v precision to compute shader brush color values.
   */
  template<typename T, typename W, typename Setter>
  void
  add_filled_shape(W &smart_widget,
                   const Setter &setter,
                   const WRATHWidgetGenerator::shape_valueT<T> &pShape,
                   const WRATHWidgetGenerator::FillingParameters &P
                   =WRATHWidgetGenerator::FillingParameters(),
                   boost::optional<enum WRATHWidgetGenerator::shape_opacity_t> aa
                   =boost::optional<enum WRATHWidgetGenerator::shape_opacity_t>(),
                   const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h
                   =WRATHTwoPassDrawer::default_pass_specifier(),
                   boost::optional<int> pitem_pass=boost::optional<int>(),
                   enum WRATHBaseSource::precision_t v
                   =WRATHBaseSource::mediump_precision)
  {
    WRATHBrush brush;
    W::Node::set_shader_brush(brush);
    WRATHWidgetGenerator::ShapeDrawer<T> drawer(WRATHWidgetGenerator::fill_shape,
                                                brush, 
                                                aa.get_value_or(default_fill_item_aa()), 
                                                h, pitem_pass.get_value_or(default_fill_item_pass()), v);
    add_shape(smart_widget, setter, pShape, drawer, P);
    smart_widget.widget()->node()->set_from_brush(brush);
  }
  
  /*!\fn void add_stroked_shape(W&,
                               const Setter&,
                               const WRATHWidgetGenerator::shape_valueT<T>&,
                               const WRATHWidgetGenerator::Brush&,
                               const WRATHWidgetGenerator::StrokingParameters&,
                               boost::optional<enum WRATHWidgetGenerator::shape_opacity_t>,
                               const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle&,
                               boost::optional<int>,
                               enum WRATHBaseSource::precision_t)
     Add a shape widget drawn stroked.
     \tparam W handle type to a shape widget, for example WRATHWidgetHandle\<WRATHShapeWidget\<N\> \>
               and WRATHWidgetHandleAutoDelete\<WRATHShapeWidget\<N\> \> for
               some node type N where N is derived from \ref Node.
               The most common types being \ref WRATHFamily::DrawnShape
               and \ref WRATHFamily::DrawnShape::AutoDelete
     \tparam T the type for the coordinates of a WRATHShape\<T\>
     \tparam Setter type that sets a Widget item properties via the method operator()(W::Widget*).
     \param smart_widget widget handle to handle the new widget; if the handle already 
                        has a widget that widget is deleted
     \param setter sets the properties of the item of the widget 
     \param pShape shape that the widget is to be
     \param pbrush brush applied to shape (i.e. \ref WRATHImage and \ref WRATHGradient)
     \param P stroking parameters for shape, including width, joint style, etc
     \param aa opacity type of item of widget
     \param h draw type specifier for item of widget
     \param pitem_pass item pass value for item of widget
     \param v precision to compute shader brush color values.
   */
  template<typename T, typename W, typename Setter>
  void
  add_stroked_shape(W &smart_widget,
                    const Setter &setter,
                    const WRATHWidgetGenerator::shape_valueT<T> &pShape,
                    const WRATHWidgetGenerator::Brush &pbrush,
                    const WRATHWidgetGenerator::StrokingParameters &P
                    =WRATHWidgetGenerator::StrokingParameters(),
                    boost::optional<enum WRATHWidgetGenerator::shape_opacity_t> aa
                    =boost::optional<enum WRATHWidgetGenerator::shape_opacity_t>(),
                    const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h
                    =WRATHTwoPassDrawer::default_pass_specifier(),
                    boost::optional<int> pitem_pass=boost::optional<int>(),
                    enum WRATHBaseSource::precision_t v
                    =WRATHBaseSource::mediump_precision)
  {
    WRATHBrush brush(pbrush.m_image, pbrush.m_gradient, pbrush.m_bits);
    brush.m_draw_state.absorb(pbrush.m_draw_state);
    W::Node::set_shader_brush(brush);
    WRATHWidgetGenerator::ShapeDrawer<T> drawer(WRATHWidgetGenerator::stroke_shape,
                                                brush, 
                                                aa.get_value_or(default_stroke_item_aa()), 
                                                h, pitem_pass.get_value_or(default_stroke_item_pass()), v);
    add_shape(smart_widget, setter, pShape, drawer, P);
    smart_widget.widget()->node()->set_from_brush(brush);
  } 

  /*!\fn void add_stroked_shape(W&,
                               const Setter&,
                               const WRATHWidgetGenerator::shape_valueT<T>&,
                               const WRATHWidgetGenerator::StrokingParameters&,
                               boost::optional<enum WRATHWidgetGenerator::shape_opacity_t>,
                               const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle&,
                               boost::optional<int>,
                               enum WRATHBaseSource::precision_t)
     Add a shape widget drawn stroked.
     \tparam W handle type to a shape widget, for example WRATHWidgetHandle\<WRATHShapeWidget\<N\> \>
               and WRATHWidgetHandleAutoDelete\<WRATHShapeWidget\<N\> \> for
               some node type N where N is derived from \ref Node.
               The most common types being \ref WRATHFamily::DrawnShape
               and \ref WRATHFamily::DrawnShape::AutoDelete
     \tparam T the type for the coordinates of a WRATHShape\<T\>
     \tparam Setter type that sets a Widget item properties via the method operator()(W::Widget*).
     \param smart_widget widget handle to handle the new widget; if the handle already 
                        has a widget that widget is deleted
     \param setter sets the properties of the item of the widget 
     \param pShape shape that the widget is to be
     \param P stroking parameters for shape, including width, joint style, etc
     \param aa opacity type of item of widget
     \param h draw type specifier for item of widget
     \param pitem_pass item pass value for item of widget
     \param v precision to compute shader brush color values.
   */
  template<typename T, typename W, typename Setter>
  void
  add_stroked_shape(W &smart_widget,
                    const Setter &setter,
                    const WRATHWidgetGenerator::shape_valueT<T> &pShape,
                    const WRATHWidgetGenerator::StrokingParameters &P
                    =WRATHWidgetGenerator::StrokingParameters(),
                    boost::optional<enum WRATHWidgetGenerator::shape_opacity_t> aa
                    =boost::optional<enum WRATHWidgetGenerator::shape_opacity_t>(),
                    const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h
                    =WRATHTwoPassDrawer::default_pass_specifier(),
                    boost::optional<int> pitem_pass=boost::optional<int>(),
                    enum WRATHBaseSource::precision_t v
                    =WRATHBaseSource::mediump_precision)
  {
    WRATHBrush brush;
    W::Node::set_shader_brush(brush);
    WRATHWidgetGenerator::ShapeDrawer<T> drawer(WRATHWidgetGenerator::stroke_shape,
                                                brush, 
                                                aa.get_value_or(default_stroke_item_aa()), 
                                                h, pitem_pass.get_value_or(default_stroke_item_pass()), v);
    add_shape(smart_widget, setter, pShape, drawer, P);
    smart_widget.widget()->node()->set_from_brush(brush);
  }

protected:

  /*!\var m_z
    _reference_ to the integer that is decremented
    to which the z-index values items are set as.
   */
  int &m_z;

private:
  
  class stack_entry
  {
  public:
    stack_entry(NodeWidget *w):
      m_node_widget(w),
      m_number_child_canvases(0),
      m_canvas(NULL)
    {}

    NodeWidget *m_node_widget;
    int m_number_child_canvases;

    /*
      non-NULL if the node represents pushing 
      a canvas node.
     */
    typename DrawnCanvas::Widget *m_canvas;
  };

  NodeWidget*
  current(void)
  {
    return m_stack.back().m_node_widget;
  }

  void
  push_widget(NodeWidget *p)
  {
    WRATHassert(p!=NULL);
    m_stack.push_back(p);
  }

  void
  push_widget_create_if_needed(NodeWidget *&widget_ptr);

  
    
  void
  push_canvas_node_implement(DrawnCanvas &canvas);
  
  
  template<typename Widget,
           typename WidgetPropertySetter,
           typename WidgetCreator>
  
  void
  add_generic_implement(Widget *&widget_ptr,
                        const WidgetPropertySetter &P,
                        const WidgetCreator &C,
                        NodeWidget *n,
                        int &z);

  template<typename Widget,
           typename WidgetPropertySetter,
           typename WidgetCreator>
  
  void
  add_generic(Widget &smart_widget,
              const WidgetPropertySetter &P,
              const WidgetCreator &C,
              NodeWidget *n,
              int &z);

  template<typename WidgetHandle>
  void
  update_generic(WidgetHandle &widget, NodeWidget *n, int &z);
   

  template<typename T>
  static
  void
  pre_treat_widget_implement(T* &q, NodeWidget *n);


  template<typename T>
  void
  pre_treat_widget(T* &q)
  {
    pre_treat_widget_implement(q, current());
  }


  int m_default_text_item_pass;
  WRATHDrawType m_default_rect_item_pass;
  int m_default_stroke_item_pass, m_default_fill_item_pass;
  enum WRATHWidgetGenerator::shape_opacity_t m_default_stroke_item_aa;
  enum WRATHWidgetGenerator::shape_opacity_t m_default_fill_item_aa;
  std::vector<stack_entry> m_stack;


  WRATHWidgetGenerator::WidgetCounter m_counters;

  /*
    We keep a seperate node stack for the adding of items 
    that do clipping
   */
  std::vector<NodeWidget*> m_clip_stack;
};


/*! @} */

#include "WRATHWidgetGeneratorImplement.tcc"


#endif
