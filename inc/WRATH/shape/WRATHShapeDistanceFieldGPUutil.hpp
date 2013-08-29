/*! 
 * \file WRATHShapeDistanceFieldGPUutil.hpp
 * \brief file WRATHShapeDistanceFieldGPUutil.hpp
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


#ifndef __WRATH_SHAPE_DISTANCE_FIELD_GPU_UTIL_HPP__
#define __WRATH_SHAPE_DISTANCE_FIELD_GPU_UTIL_HPP__

#include "WRATHConfig.hpp"
#include "WRATHShapeDistanceFieldGPU.hpp" 
#include "WRATHTripleBufferEnabler.hpp"
#include "WRATHImage.hpp"


/*! \addtogroup Shape
 * @{
 */


namespace WRATHShapeGPUDistanceFieldCreator
{
  /*!\class ScratchPadFBO
    Quick and dirty ScratchPad implementation
    that creates an FBO and deletes it on
    dtor.
   */
  class ScratchPadFBO:public ScratchPad
  {
  public:
    /*!\fn ScratchPadFBO
      Ctor
      \param tr handle to WRATHTripleBufferEnabler to sync GL operations
      \param texture_format texture format of texture used by FBO
     */ 
    explicit
    ScratchPadFBO(const WRATHTripleBufferEnabler::handle &tr,
                  GLenum texture_format=GL_RGBA);
    ~ScratchPadFBO();

    virtual
    enum return_code
    init_and_bind_fbo(ivec2 dims, bool requires_depth_buffer);

    virtual
    ivec4
    viewport_parameters(void);

  private:

    WRATHTripleBufferEnabler::handle m_tr;
    GLenum m_format;
    ivec2 m_max_dim, m_current_dim;
    GLuint m_fbo;
    GLuint m_texture;
    GLuint m_depth;
    GLuint m_stencil;
    bool m_depth_is_renderbuffer;
  };

  /*!\class DistanceFieldTarget_WRATHImage
    A simple barbaric DistanceFieldTarget implementation
    that performs glCopyTexSubImage2D to place the contents
    into a WRATHImage.
   */
  class DistanceFieldTarget_WRATHImage:public DistanceFieldTarget
  {
  public:
    /*!\fn DistanceFieldTarget_WRATHImage
      Ctor.
      \param pImage WRATHImage to which to place results
      \param offset offset into WRATHImage to place results
     */
    explicit
    DistanceFieldTarget_WRATHImage(WRATHImage *pImage,
                                   ivec2 offset=ivec2(0,0));

    virtual
    enum return_code
    copy_results(const ScratchPad::handle &H);

  private:
    WRATHImage *m_image;
    ivec2 m_offset;
  };
}


/*! @} */

#endif
