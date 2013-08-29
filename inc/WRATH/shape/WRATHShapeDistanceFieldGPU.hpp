/*! 
 * \file WRATHShapeDistanceFieldGPU.hpp
 * \brief file WRATHShapeDistanceFieldGPU.hpp
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


#ifndef __WRATH_SHAPE_DISTANCE_FIELD_GPU_HPP__
#define __WRATH_SHAPE_DISTANCE_FIELD_GPU_HPP__

#include "WRATHConfig.hpp"
#include "WRATHShapeSimpleTessellator.hpp"

/*! \addtogroup Shape
 * @{
 */

/*!\namespace WRATHShapeGPUDistanceFieldCreator
  Namespace to encapsulate support types 
  and fucntions for using a GPU to create 
  a distance field stored in a texture
 */
namespace WRATHShapeGPUDistanceFieldCreator
{
 
  /*!\enum corner_point_handling_type
    Enumeration describing how to generate distance
    values near corners.
   */
  enum corner_point_handling_type
    {
      /*!
        Handle corners by drawing point
        sprites (requires writing to
        gl_FragDepth/gl_FragDepthEXT and
        or GL_NV_framebuffer_fetch)
       */
      use_point_sprites,

      /*!
        Draw a triangle fan for each corner point
       */
      use_triangle_fans,

      /*!
        Don't worry about distances
        from corners
       */
      skip_points
    };

  /*!\class ScratchPad
    Rendering the DistanceField is performed in two steps:
    \n 1) Render to stencil buffer the winding or even/odd rule
    \n 2) Render the distance values (in two passes, one pass
          for inside the shape and another pass for outside the 
          shape). This rendering is performed by a shader that
          will write to each RGBA channel the (normalized)
          distance value.    

    A ScratchPad is where the rendering takes place.
    As a side note, one ScratchPad can be used over and
    over again (for example by making the underlying 
    image buffers large and using glViewport appropiately).  
   */
  class ScratchPad:
    public WRATHReferenceCountedObjectT<ScratchPad>
  {
  public:
    /*!\fn enum return_code init_and_bind_fbo
      To be implemented by a derived class
      to create and bind an FBO to where
      the distance feild will be rendered.
      The FBO _must_ have a stencil buffer.

      The shader for the distance field 
      writes to only one texture (i.e. not MRT) 
      and will write to each RGBA channel 
      the distance value as a normalized value 
      [0,1] with (0.5, 1.0] meaning inside the 
      shape, [0.0, 0.5) outside the shape and
      0.5 on the boundary of the shape.
      In addition to binding the FBO, the
      routine is expected to correctly set
      glViewport and potentially glScissor
      tests.
      \param dims 2D-dimensions of the distance field scratchpad
      \param requires_depth_buffer if true requires a 
                                   depth buffer as well

     */
    virtual
    enum return_code
    init_and_bind_fbo(ivec2 dims, bool requires_depth_buffer)=0;

    /*!\fn ivec4 viewport_parameters
      To be implemented to return the parameters to be
      fed to glViewport to render to the ScratchPad.
      The format of the parameters is:
      - [0] --> x-coordinate of bottom left
      - [1] --> y-cooridnate of top right
      - [2] --> width
      - [3] --> height
      Remember that GL has bottom left means (0,0)
     */
    virtual
    ivec4
    viewport_parameters(void)=0;

  };

  /*!\class DistanceFieldTarget
    A DistanceFieldTarget represents the location
    to which to copy the rendered distance field.
    It is feasible that a DistanceFieldTarget is
    a no-op and no operation is performed 
    (for example if the ScratchPad is used).
   */
  class DistanceFieldTarget:
    public WRATHReferenceCountedObjectT<DistanceFieldTarget>
  {
  public:
    /*!\fn enum return_code copy_results
      To be implemented by the derived class to perform
      any post processing, such as for example copying
      from the texture of the FBO to the final destination
      of the Distance Field results.
      \param H ScratchPad that holds the distance field texture render.
     */
    virtual
    enum return_code
    copy_results(const ScratchPad::handle &H)=0;
  };

  

  /*!\fn enum return_code generate_distance_field(const WRATHShapeSimpleTessellatorPayload::handle&,
                                                  ivec2, float,
                                                  const ScratchPad::handle&,
                                                  const DistanceFieldTarget::handle&,
                                                  enum corner_point_handling_type)
  
    Generate a distance field from a WRATHShapeSimpleTessellatorPayload.
    Generation of the distance field is realized by rasterizing
    quads generated from each edge of the WRATHShapeSimpleTessellatorPayload,
    in addition to extra rasterization at the corners.
    \param h handle to a WRATHShapeSimpleTessellatorPayload holding the
             tessellation of a WRATHShape<T>
    \param dims Dimension of distance field to generate
    \param pixel_dist distance in pixels to which the distance values
                      are saturated, this value determines the size of the
                      quads rasterized, a good value is 1.5
    \param scratch handle to a ScratchPad to draw intermediate results
    \param dest handle to a DistanceFieldTarget to where the distance field
                is rendered
    \param ct specifies if and how to handle distances from corners
   */
  enum return_code
  generate_distance_field(const WRATHShapeSimpleTessellatorPayload::handle &h,
                          ivec2 dims, float pixel_dist,
                          const ScratchPad::handle &scratch,
                          const DistanceFieldTarget::handle &dest,
                          enum corner_point_handling_type ct=skip_points);
  
};




/*! @} */

#endif
