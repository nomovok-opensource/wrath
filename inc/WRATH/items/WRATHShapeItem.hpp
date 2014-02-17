/*! 
 * \file WRATHShapeItem.hpp
 * \brief file WRATHShapeItem.hpp
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


/*! \addtogroup Items
 * @{
 */


#ifndef WRATH_HEADER_SHAPE_ITEM_HPP_
#define WRATH_HEADER_SHAPE_ITEM_HPP_

#include "WRATHConfig.hpp"
#include "WRATHBaseItem.hpp"
#include "WRATHShape.hpp"
#include "WRATHShapeAttributePacker.hpp"
#include "WRATHShaderSpecifier.hpp"
#include "WRATHDefaultFillShapeAttributePacker.hpp"
#include "WRATHDefaultStrokeAttributePacker.hpp"
#include "WRATHBrush.hpp"
#include "WRATHItemTypes.hpp"
#include "WRATHDefaultShapeShader.hpp"

/*!\namespace WRATHShapeItemTypes
  Namespace to encapsulate the types used by
  WRATHShapeItem to construct a WRATHShapeItem
 */
namespace WRATHShapeItemTypes
{
  /*!\enum fill_shape_t  
    Enumeration to indicate to draw the
    WRATHShape filled for use with built-in
    WRATHShaderSpecifier and WRATHShapeAttributePacker
    objects.
   */
  enum fill_shape_t
    {
      fill_shape
    };

  /*!\enum stroke_shape_t 
    Enumeration to indicate to draw the
    WRATHShape stroked for use with built-in
    WRATHShaderSpecifier and WRATHShapeAttributePacker
    objects.
   */
  enum stroke_shape_t
    {
      stroke_shape
    };
  
  /*!\enum shape_opacity_t
    Enumeration to describe if a WRATHShapeItem
    is to be drawn as opaque or transparent.
    This enumeration is used in conveniance
    ctor's of \ref ShapeDrawer objects.
   */
  enum shape_opacity_t
    {
      /*!
        WRATHShapeItem is drawn as transparent
       */
      shape_transparent,

      /*!
        WRATHShapeItem is drawn as opaque
        with anti-aliasing, thus needing
        two passes.
       */
      shape_opaque,

      /*!
        WRATHShapeItem is opaque AND is 
        not anti-aliased, thus the shape
        is drawn in one (opaque) pass
       */
      shape_opaque_non_aa,
    };

  /*!\class ShapeDrawerPass
    A ShapeDrawerPass represents a single drawing
    pass of drawing a WRATHShape by a WRATHShapeItem.
   */
  class ShapeDrawerPass:public WRATHItemTypes::DrawerPass
  {
  public:
    /*!\fn ShapeDrawerPass
      Ctor.
      \param sh echoed to WRATHItemTypes::DrawerPass::DrawerPass(const WRATHShaderSpecifier*, WRATHDrawType)
      \param pdraw_type echoed to WRATHItemTypes::DrawerPass::DrawerPass(const WRATHShaderSpecifier*, WRATHDrawType)
    */
    ShapeDrawerPass(const WRATHShaderSpecifier *sh=NULL,
                    WRATHDrawType pdraw_type=WRATHDrawType::opaque_pass()):
      WRATHItemTypes::DrawerPass(sh, pdraw_type),
      m_use_secondary_indices(false)
    {}

    /*!\var m_use_secondary_indices
      If true, indicates that the pass uses the
      seconday indices packed by WRATHShapeAttributePacker.
      In the case that the WRATHShapeAttributePacker
      has no secondary indices, then will use the primary
      indices.
     */
    bool m_use_secondary_indices;
  };
  
  /*!\class ShapeDrawer
    Named parameter list for a specifying 
    how to draw a WRATHShapeItem.
   */
  template<typename T>
  class ShapeDrawer:
    public WRATHItemTypes::Drawer<WRATHShapeAttributePacker<T>, ShapeDrawerPass>
  {
  public:
    /*!\typedef base_class
      Conveniance local typedef
     */ 
    typedef WRATHItemTypes::Drawer<WRATHShapeAttributePacker<T>, ShapeDrawerPass> base_class;

    /*!\fn ShapeDrawer(void)
      Ctor. Empty initializer.
     */
    ShapeDrawer(void)
    {}

    /*!\fn ShapeDrawer(const WRATHShaderSpecifier*, const WRATHShapeAttributePacker<T>*,
                       enum shape_opacity_t, const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle&,
                       int)
      Ctor. Initialize \ref m_packer as the passed
      WRATHShapeAttributePacker<T>, depending on the
      opacity type, initialize \ref m_draw_passes
      as having one or two passes using the sub shaders
      from sh.
      \param sh value to which to set \ref m_draw_passes[0].m_shader
      \param p WRATHShapeAttribute<T> packer to generate attribute data
      \param aa specifies opacity for drawing the shape
      \param h handle to specify which passes are drawn where
      \param pitem_pass the "item" pass for the shape item, value is passed
                        to the WRATHTextureFont::DrawTypeSpecifier object 
                        to help determine the WRATHDrawType value
                        to use for the shape item
     */
    ShapeDrawer(const WRATHShaderSpecifier *sh,
                const WRATHShapeAttributePacker<T> *p,
                enum shape_opacity_t aa,
                const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h
                =WRATHTwoPassDrawer::default_pass_specifier(),
                int pitem_pass=0);

    /*!\fn ShapeDrawer(const WRATHShaderSpecifier*, 
                       const WRATHShapeAttributePacker<T>*, 
                       WRATHDrawType)   
      Ctor. Initialize \ref m_packer as the passed
      WRATHShapeAttributePacker<T>,
      \ref m_draw_passes as an array of length 1
      with ShapeDrawerPass::m_shader set.
      \param sh value to which to set \ref m_draw_passes[0].m_shader
      \param p WRATHShapeAttribute<T> packer to generate attribute data
      \param ppass WRATHDrawType specifying at which pass to draw
     */
    ShapeDrawer(const WRATHShaderSpecifier *sh,
                const WRATHShapeAttributePacker<T> *p,
                WRATHDrawType ppass=WRATHDrawType::opaque_pass()):
      base_class(sh, p, ppass)
    {}
    
    /*!\fn ShapeDrawer(enum fill_shape_t,
                       const WRATHBrush&,
                       enum shape_opacity_t,
                       const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle&,
                       int,
                       enum WRATHBaseSource::precision_t)
      Initialize ShapeDrawer to use WRATH's built in shaders
      for drawing a shape filled.
      \param ignore actual value ignored, enumeration type tags the ctor to use
      \param brush \ref WRATHBrush specifying if/how to apply image, gradient and const-color
      \param aa specifies opacity for drawing the shape
      \param h handle to specify which passes are drawn where
      \param pitem_pass the "item" pass for the shape item, value is passed
                        to the WRATHTextureFont::DrawTypeSpecifier object 
                        to help determine the WRATHDrawType value
                        to use for the shape item
      \param v precision qualifiers to use in computing the gradient
               interpolate.
     */
    ShapeDrawer(enum fill_shape_t ignore,
                const WRATHBrush &brush=WRATHBrush(),
                enum shape_opacity_t aa=shape_opaque_non_aa,
                const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h
                =WRATHTwoPassDrawer::default_pass_specifier(),
                int pitem_pass=0,
                enum WRATHBaseSource::precision_t v
                =WRATHBaseSource::mediump_precision);
    
    /*!\fn ShapeDrawer(enum stroke_shape_t,
                       const WRATHBrush &,
                       enum shape_opacity_t,
                       const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle&,
                       int,
                       enum WRATHBaseSource::precision_t)
      Initialize ShapeDrawer to use WRATH's built in shaders
      for drawing a shape stroked.   
      \param ignore actual value ignored, enumeration type tags the ctor to use
      \param brush \ref WRATHBrush specifying if/how to apply image, gradient and const-color
      \param aa specifies opacity for drawing the shape
      \param h handle to specify which passes are drawn where
      \param pitem_pass the "item" pass for the shape item, value is passed
                        to the WRATHTextureFont::DrawTypeSpecifier object 
                        to help determine the WRATHDrawType value
                        to use for the shape item
      \param v precision qualifiers to use in computing the gradient
               interpolate.
     */
    ShapeDrawer(enum stroke_shape_t ignore,
                const WRATHBrush &brush=WRATHBrush(),
                enum shape_opacity_t aa=shape_opaque_non_aa,
                const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h
                =WRATHTwoPassDrawer::default_pass_specifier(),
                int pitem_pass=0,
                enum WRATHBaseSource::precision_t v
                =WRATHBaseSource::mediump_precision);
  };

  /*!\class shape_valueT
    Class to specify the WRATHShape and 
    WRATHShapeProcessorPayload that a 
    WRATHShapeItem is to draw.
   */
  template<typename T>
  class shape_valueT
  {
  public:
    /*!\fn shape_valueT(const WRATHShape<T>&) 
      WRATHShapeItem will have the WRATHShapeAttributePacker
      select what payload to draw from the WRATHShape 
      \param s WRATHShape to draw
     */
    shape_valueT(const WRATHShape<T> &s):
      m_shape(s)
    {}

    /*!\fn shape_valueT(const WRATHShape<T>&, const WRATHShapeProcessorPayload&)   
      WRATHShapeItem will use the passed payload.
      If the passed payload is invalid, then
      the WRATHShapeItem will have the WRATHShapeAttributePacker
      select what payload to draw from the WRATHShape s
      \param s WRATHShape to draw
      \param p WRATHShapeProcessorPayload of s to draw
     */
    shape_valueT(const WRATHShape<T> &s,
                 const WRATHShapeProcessorPayload &p):
      m_shape(s),
      m_payload(p)
    {}

    /*!\fn shape_valueT(const WRATHShape<T>&, const PayloadParams&)   
      WRATHShapeItem will get a payload constructed from
      the passed paramaters as the payload to draw
      (\ref WRATHShape::fetch_matching_payload()).
      \param s WRATHShape to draw
      \param params parameters of the WRATHShapeProcessorPayload to draw 
     */
    template<typename PayloadParams>
    shape_valueT(const WRATHShape<T> &s, const PayloadParams &params):
      m_shape(s)
    {
      m_payload
        =m_shape.template fetch_matching_payload<typename PayloadParams::PayloadType>(params);
    }

    /*!\var m_shape  
      Reference to WRATHShape<T> to draw,
      set at ctor.
     */
    const WRATHShape<T> &m_shape;

    /*!\var m_payload   
      Payload of \ref m_shape to use for drawing.
      Set at ctor and can also be changed.
     */
    WRATHShapeProcessorPayload m_payload;
  };

  /*!\fn shape_value(const WRATHShape<T>&)
    Template conveniance function to return
    a shape_valueT<T> object from a 
    WRATHShape<T>.
    \param s WRATHShape<T> from which to contruct the shape_valueT<T>
   */
  template<typename T>
  static
  shape_valueT<T>
  shape_value(const WRATHShape<T> &s)
  {
    return shape_valueT<T>(s);
  }

  /*!\fn shape_value(const WRATHShape<T>&, const WRATHShapeProcessorPayload&)  
    Template conveniance function to return
    a shape_valueT<T> object from a 
    WRATHShape<T> and WRATHShapeProcessorPayload
    pair.
    \param s WRATHShape<T> from which to contruct the shape_valueT<T>
    \param p WRATHShapeProcessorPayload from which to contruct the shape_valueT<T>
   */
  template<typename T>
  static
  shape_valueT<T>
  shape_value(const WRATHShape<T> &s,
              const WRATHShapeProcessorPayload &p)
  {
    return shape_valueT<T>(s, p);
  }

  /*!\fn shape_value(const WRATHShape<T>&, const PayloadParams&)
    Template conveniance function to return
    a shape_valueT<T> object from a 
    WRATHShape<T> and PayloadParams
    pair.
    \param s WRATHShape<T> from which to contruct the shape_valueT<T>
    \param params PayloadParams from which to contruct the shape_valueT<T>
   */
  template<typename T, typename PayloadParams>
  static
  shape_valueT<T>
  shape_value(const WRATHShape<T> &s, const PayloadParams &params)
  {
    return shape_valueT<T>(s, params);
  }

  /*!\typedef ShapeDrawerF
    Conveniance typedef for \ref ShapeDrawer using float
   */
  typedef ShapeDrawer<float> ShapeDrawerF;

  /*!\typedef ShapeDrawerI 
    Conveniance typedef for \ref ShapeDrawer using int
   */
  typedef ShapeDrawer<int> ShapeDrawerI;
  
  /*!\typedef shape_valueF
    Conveniance typedef for \ref shape_valueT using float.
   */
  typedef shape_valueT<float> shape_valueF;

  /*!\typedef shape_valueI  
    Conveniance typedef for \ref shape_valueT using int
   */
  typedef shape_valueT<int> shape_valueI;
};


/*!\class WRATHShapeItem
  A WRATHShapeItem represents drawing a WRATHShape.
  The WRATHShape can be drawn stroked or filled.  
 */
class WRATHShapeItem:public WRATHBaseItem
{
public:  
  /*!\fn WRATHShapeItem 
    Ctor.
    \param fact WRATHItemDrawerFactory responsible for fetching/creating
                the WRATHItemDrawer used by the item
    \param subdrawer_id SubDrawer Id passed to \ref WRATHItemDrawerFactory::generate_drawer()
    \param pcanvas WRATHCanvas where created item is placed, Canvas does NOT own the item.
    \param subkey SubKey used by pcanvas, typically holds "what" transformation/clipping node to use
    \param shape shape for the WRATHShapeItem to draw, the current state of the WRATHShape is
                 taken at ctor. Further changes of the WRATHShape after the WRATHShapeItem
                 is contructed do NOT change the display of the WRATHShapeItem
    \param drawer specifies how the WRATHShapeItem is to be drawn
    \param additional_packing_params additional attribute packing parameters
                                     that WRATHShapeItem::Drawer::m_packer
                                     (see \ref WRATHItemTypes::Drawer::m_packer)
                                     may use when creating the attribute data
                                     for the WRATHShapeItem.
   */
  template<typename T>
  WRATHShapeItem(const WRATHItemDrawerFactory &fact, int subdrawer_id,
                 WRATHCanvas *pcanvas,
                 const WRATHCanvas::SubKeyBase &subkey,
                 const WRATHShapeItemTypes::shape_valueT<T> &shape,
                 const WRATHShapeItemTypes::ShapeDrawer<T> &drawer,
                 const WRATHShapeAttributePackerBase::PackingParametersBase &additional_packing_params=
                 WRATHShapeAttributePackerBase::PackingParametersBase())
  {
    WRATHShapeProcessorPayload payload(shape.m_payload);

    if(!payload.valid())
      {
        drawer.m_packer->default_payload(&shape.m_shape);
      }

    construct<T>(fact, subdrawer_id,
                 pcanvas, subkey,
                 shape.m_shape, 
                 drawer,  
                 payload,
                 additional_packing_params);
  }

  ~WRATHShapeItem();

  virtual
  WRATHCanvas*
  canvas_base(void) const;

  virtual
  void
  canvas_base(WRATHCanvas *c);

  /*!\fn change_shape
    Change the shape that this WRATHShapeItem draws. It is imperiative
    the the template type T is the _exact_ same type used in the
    ctor to specify the original shape this WRATHShapeItem drew.
    Method will assert if not under debug and silently do nothing
    on release. Note that a WRATHShapeItem does NOT track what
    shape (or for that matter what payload) it is drawing, hence
    changing the shape to the same shape will still force a regeneration
    of attribute data. You have been warned.
    \param shape shape for the WRATHShapeItem to draw, the current state of the WRATHShape is
                 taken at ctor. Further changes of the WRATHShape after the WRATHShapeItem
                 is contructed do NOT change the display of the WRATHShapeItem
    \param additional_packing_params additional attribute packing parameters
                                     that attribute packer specified in the ctor
                                     may use when creating the attribute data
                                     for the WRATHShapeItem.
   */
  template<typename T>
  void
  change_shape(const WRATHShapeItemTypes::shape_valueT<T> &shape,
               const WRATHShapeAttributePackerBase::PackingParametersBase &additional_packing_params=
               WRATHShapeAttributePackerBase::PackingParametersBase());

private:

  template<typename T>
  void
  construct(const WRATHItemDrawerFactory &fact, int subdrawer_id,
            WRATHCanvas *pcanvas,
            const WRATHCanvas::SubKeyBase &subkey,
            const WRATHShape<T> &pshape,
            const WRATHShapeItemTypes::ShapeDrawer<T> &drawer,
            WRATHShapeProcessorPayload payload,
            const WRATHShapeAttributePackerBase::PackingParametersBase &additional_packing_params);

  void
  init_key_and_allocate(WRATHShapeAttributePackerBase::allocation_requirement_type reqs,
                        const WRATHItemDrawerFactory &factory, int subdrawer_id,
                        WRATHCanvas *canvas,
                        const WRATHCanvas::SubKeyBase &subkey,
                        const WRATHShapeAttributePackerBase *packer,
                        GLenum buffer_object_hint,
                        const std::vector<WRATHShapeItemTypes::ShapeDrawerPass> &draw_passes);

  void
  allocate_indices_and_attributes(WRATHShapeAttributePackerBase::allocation_requirement_type);
  

  WRATHCanvas::DataHandle m_primary_item_group;
  WRATHCanvas::DataHandle m_secondary_item_group;
  std::vector<range_type<int> > m_attribute_data_location;
  WRATHIndexGroupAllocator::index_group<GLushort> m_primary_index_data_location;
  WRATHIndexGroupAllocator::index_group<GLushort> m_secondary_index_data_location;

  /*
    tracking to allow for chaning WRATHShape data...
   */
  const WRATHShapeAttributePackerBase *m_packer;
  int m_allocated_number_attributes;
  WRATHStateBasedPackingData::handle m_immutable_packing_data;
};

#include "WRATHShapeItemImplement.tcc"


/*! @} */

#endif
