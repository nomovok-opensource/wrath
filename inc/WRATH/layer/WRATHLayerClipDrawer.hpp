/*! 
 * \file WRATHLayerClipDrawer.hpp
 * \brief file WRATHLayerClipDrawer.hpp
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


#ifndef __WRATH_LAYER_CLIP_DRAWER_HPP__
#define __WRATH_LAYER_CLIP_DRAWER_HPP__

#include "WRATHConfig.hpp"
#include "WRATHReferenceCountedObject.hpp"
#include "WRATHLayerBase.hpp"
#include "WRATHBBox.hpp"

/*! \addtogroup Layer
 * @{
 */

class WRATHLayer;

/*!\class WRATHLayerClipDrawer
  A WRATHLayerClipDrawer is an interface to specify
  a clipping region that clips a WRATHLayer.  
 */
class WRATHLayerClipDrawer:
  public WRATHReferenceCountedObjectT<WRATHLayerClipDrawer>
{
public:
  /*!\enum clip_mode_type
    Enumeration of the clipping mode active
    on a WRATHLayer passed to clip_mode().
   */
  enum clip_mode_type
    {
      /*!
        Indicates that the WRATHLayer is
        completely clipped, i.e. to skip
        drawing the elements of the WRATHLayer.
       */
      skip_layer,

      /*!
        Indicates that the WRATHLayer is 
        clipped against a shape that will
        be drawn by draw_region() as follows:
        - Clipping region is drawn with depth and stencil
          test on to stencil buffer
        - Pixels of clipping region has it's depth
          value reset
        - Then draw WRATHLayer, stencil test pass only
          within the region drawn.
        - Clipping region is reset with depth
          value as from the GLSL shader of
          draw_region().
        
        The use case is that one has a WRATHLayer
        and on wishes to assoicate a common z-value
        for the contents WRATHLayer. The above has
        that the WRATHLayer is drawn only over those
        pixels where the _WRATHLayer_'s z allows
        and the contents of the WRATHLayer have that
        the z-buffer is reset. This has the same function
        as if the WRATHLayer is drawn to a texture and
        that texture is drawn with z-test enabled.
       */
      layer_clipped_hierarchy,

      /*!
       Indicates that the WRATHLayer is 
       clipped against a shape that will
       be drawn by draw_region() as follows:
       - Clipping region is drawn to only the stencil
         buffer regardless of z-values
       - Draw WRATHLayer, stencil test pass only
         within the region drawn.
  
       The use case is that one just wishes
       for the contents of the WRATHLayer to
       be clipped against a shape but the
       nature of z-values (i.e. back/front) is
       not affected
       */
      layer_clipped_sibling,

      /*!
        Indicates that the WRATHLayer is
        not clipped against any shape.
       */
      layer_unclipped
    };

  /*!\typedef ClipState
    A WRATHLayerClipDrawer object is stateless
    (or all it's state is immutable with respect
    to drawing and clipping).
    In order to track state of a drawing/clipping operation,
    a  WRATHLayerClipDrawer derived class
    will pack such data into a WRATHReferenceCountedObject
    derived object.
   */
  typedef WRATHReferenceCountedObject::handle ClipState;

  /*!\class DrawStateElementTransformations
    A DrawStateElementTransformations represents
    the transformation of an element of the 
    "drawing state" stack of drawing a WRATHLayer 
    heierarhy.
   */
  class DrawStateElementTransformations
  {
  public:
    /*!\var m_composed_modelview
      The composed modelview matrix applied
      to \ref DrawStateElement::m_layer
     */
    float4x4 m_composed_modelview;

    /*!\var m_composed_projection
      The composed projection matrix applied
      to \ref DrawStateElement::m_layer
     */
    float4x4 m_composed_projection;

    /*!\var m_composed_pvm;
      The product of \ref m_composed_projection
      and \ref m_composed_projection
     */
    float4x4 m_composed_pvm;
  };


  /*!\class DrawStateElementClipping
    A DrawStateElementClipping represents the clipping 
    state of an element of the "drawing state" stack of 
    drawing a WRATHLayer heierarhy.
   */
  class DrawStateElementClipping
  {
  public:

    /*!\fn DrawStateElementClipping
      Ctor. Initializes \ref m_device_bbox as the entire
      normalized space, i.e. \f$[-1,1]x[-1,1]\f$
      \param c value to which to initialize \ref m_clip_mode 
     */
    DrawStateElementClipping(enum clip_mode_type c=layer_unclipped):
      m_device_bbox(vec2(-1.0f, -1.0f), vec2(1.0f, 1.0f)),
      m_clip_mode(c)
    {}

    /*!\var m_device_bbox
      The bounding box of a WRATHLayer
      on the screen in normalized device
      coordinates
     */
    WRATHBBox<2> m_device_bbox;
    
    /*!\var m_clip_mode
      The clip mode type applied a WRATHLayer
     */
    enum clip_mode_type m_clip_mode;

    /*!\var m_clip_state
      The clipping state from the WRATHLayerClipDrawer
      that applied clipping to the WRATHLayer
     */
    ClipState m_clip_state;
    
  };

  /*!\class DrawStateElement
    A DrawStateElement represents and element
    of the "drawing state" stack of drawing 
    a WRATHLayer heierarhy.
   */
  class DrawStateElement
  {
  public:
    /*!\fn DrawStateElement
      Ctor
      \param p value to which to initalize \ref m_layer
     */ 
    DrawStateElement(WRATHLayer *p=NULL):
      m_layer(p)
    {}

    /*!\var m_layer
      The WRATHLayer of the element
     */
    WRATHLayer *m_layer;

    /*!\var m_transformations
      Represents the transformations applied 
      to \ref m_layer.
     */
    DrawStateElementTransformations m_transformations;

    /*!\var m_clipping
      Represents the clipping applied 
      to \ref m_layer.
     */
    DrawStateElementClipping m_clipping;
  };

  
  virtual
  ~WRATHLayerClipDrawer()
  {}

  /*!\fn clip_mode
    To be implemented by a derived class
    to return if and how clipping is to be applied
    to a WRATHLayer.
    \param layer to which to apply clipping
    \param layer_transformations the transformation applied to the layer
    \param draw_state_stack the draw stack _below_ the layer, i.e.
                            draw_state_stack.back() represents the 
                            parent of layer in the draw state.
   */
  virtual
  DrawStateElementClipping
  clip_mode(WRATHLayer *layer,
            const DrawStateElementTransformations &layer_transformations,
            const_c_array<DrawStateElement> draw_state_stack) const=0;

  /*!\fn draw_region
    To be implemented by a derived class
    to draw the clipping regionto be applied
    to a WRATHLayer. Do NOT make any assumptions
    about GL state when implementing draw_region().
    Indeed it is undefined what GLSL program is
    active, what attributes are enabled and
    what buffer objects are currently bound.
    If an implementation use VAO (glBindVertexArray)
    to packet up the attribute state, an implementation
    MUST call glBindVertexArray(0) at the end of the routine
    to make sure that WRATH does not affect one's private VAO.
    Additionally, an implementation of draw_region()
    MUST not change any of:
    - Any of the write masks (glColorMask, glStencilMask, glDepthMask)
    - Stencil and depth tests (glDepthFunc, glStencilFunc[Separate], glStencilOp[Separate])
    - Current bound framebuffer object (glBindFramebuffer)
    - Drawing target (glDrawBuffer and glDrawBuffers)

    \param layer The DrawStateElement holding the data needed to
                 draw the clipping region
    \param draw_stack the draw state stack, note that the top of the
                      statck is the same object as layer                             
    \param clear_z if clear_z is true, then draw_region() is expected
                   to set the normalized device z-coordinate as 1.0
                   over the clipping region (when being done so, the 
                   depth test is GL_ALWAYS, but the stencil test is
                   active with the stencil buffer set so that exactly
                   those pixels of the clipping resgion pass the stencil
                   test.
   */
  virtual
  void
  draw_region(bool clear_z, 
              const DrawStateElement &layer,
              const_c_array<DrawStateElement> draw_stack) const=0;

};


/*! @} */

#endif
