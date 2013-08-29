/*! 
 * \file shape.cpp
 * \brief file shape.cpp
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
 * \author Jean Fairlie <jean.fairlie@nomovok.com>
 * 
 */


#include "WRATHConfig.hpp"
#include "WRATHNew.hpp"
#include <fstream>
#include <iomanip>
#include <limits>
#include "vecN.hpp"
#include "WRATHNew.hpp"
#include "WRATHmalloc.hpp"
#include "WRATHUtil.hpp"
#include <dirent.h>
#include "ngl_backend.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <WRATHShapeItem.hpp>
#include <WRATHShapeSimpleTessellator.hpp>
#include <WRATHDefaultFillShapeAttributePacker.hpp>
#include <WRATHDefaultStrokeAttributePacker.hpp>
#include <WRATHLayerItemWidgetsTranslate.hpp>
#include "wrath_demo.hpp"

/*!
\details
This example demonstrates the basic usage of the WRATHShape API and the
Shape widget.
 */


class cmd_line_type:public DemoKernelMaker
{
public:
  virtual
  DemoKernel* 
  make_demo(void);
  
  virtual
  void
  delete_demo(DemoKernel *k)
  {
    if(k!=NULL)
      {
        WRATHDelete(k);
      }
  }
};


class ShapeExample:public DemoKernel
{
public:
  ShapeExample(cmd_line_type *cmd_line);
  ~ShapeExample();
  /*
    called when window resizes
    */
  void 
  resize(int width, int height);
  
  virtual
  void
  handle_event(FURYEvent::handle ev);
  
  virtual
  void
  paint(void);

private:
  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_layer;
  /*
    High level widget that takes care of rendering the shape
    */
  typedef WRATHLayerTranslateFamilySet::ColorFamily::ShapeWidget ShapeWidget;
  ShapeWidget *m_shape_widget;

};


DemoKernel* 
cmd_line_type::
make_demo(void)
{
  return WRATHNew ShapeExample(this);
}



ShapeExample::
ShapeExample(cmd_line_type *cmd_line):
  DemoKernel(cmd_line)
{
  /*
    Create the WRATHTripleBufferEnabler object
    which our visual items will use to sync
   */
  m_tr=WRATHNew WRATHTripleBufferEnabler();

  /*
    Create the WRATHLayer which will contain and
    draw our shape
   */
  m_layer=WRATHNew WRATHLayer(m_tr);

  /*
    these are the transforms that will be
    applied to all elements contained in the
    WRATHLayer:

    + a 3D transform
    + a Projection transform

    For the purpose of this example the 3D transform
    will be the identity matrix. In other words no
    transform will be applied to the vertex data.

    Projection will be orthographic
    */

  float_orthogonal_projection_params proj_params(0, width(),
                                                 height(), 0);

  m_layer->simulation_matrix(WRATHLayer::projection_matrix,
                                float4x4(proj_params));


  WRATHShapeF shape;
  /*
    A WRATHShape is basically a list of WRATHOutline.
    Get a new one.
    */
  shape.new_outline();

  /*
    A WRATHOutline is basically a list of curve segments
    defined by a list of points and interpolators.
    Interpolators define how to calculate the intermediate
    points between two consecutive points in a WRATHOutline .
    */
  WRATHOutline<float>& outline = shape.current_outline();

  /* Add two points and an bezier control point between them.
    Adding the points like this automatically defines bezier
    interpolators for them.
    Since only one control point is added the bezier defined
    is cuadratic. Adding more control points would define
    cubic, quartic and so on.
    */

  //these are screen coords
  //add first point at top left of screen
  outline<<WRATHOutline<float>::position_type(0.0, 0.0)
  //add control point at bottom middle of screen
        <<WRATHOutline<float>::control_point(WRATHOutline<float>::position_type(width()/2.0, height()))
  //add second point to top right of screen
         <<WRATHOutline<float>::position_type(width(), 0.0);


  //specify the draw for m_shape_widget
  // - apply a brush, the shaders of the brush come from the node type of ShapeWidget 
  // - specify to stroke the shape
  WRATHBrush brush;
  ShapeWidget::set_shader_brush(brush);
  WRATHShapeItemTypes::ShapeDrawerF drawer(WRATHShapeItemTypes::stroke_shape, brush);
  
  m_shape_widget = WRATHNew ShapeWidget(m_layer,
                                        WRATHShapeItemTypes::shape_valueT<float>(shape),
                                        drawer);
  m_shape_widget->color(vec4(0.0f, 0.0f, 0.0f, 0.0f));
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

ShapeExample::
~ShapeExample()
{
  if(m_layer!=NULL)
    {
      WRATHPhasedDelete(m_layer);
    }
  WRATHResourceManagerBase::clear_all_resource_managers();
  m_tr->purge_cleanup();
  m_tr=NULL;
}

void 
ShapeExample::
resize(int width, int height)
{
    float_orthogonal_projection_params proj_params(0, width, height, 0);

    m_layer->simulation_matrix(WRATHLayer::projection_matrix, float4x4(proj_params));
    glViewport(0, 0, width, height);
}

void 
ShapeExample::
handle_event(FURYEvent::handle ev)
{
  if(ev->type()==FURYEvent::Resize)
    {
      FURYResizeEvent::handle rev(ev.static_cast_handle<FURYResizeEvent>());
      resize(rev->new_size().x(), rev->new_size().y());
    }
}

void 
ShapeExample::
paint(void)
{
  m_tr->signal_complete_simulation_frame();
  m_tr->signal_begin_presentation_frame();
  m_layer->clear_and_draw();
}


int
main(int argc, char **argv)
{
    cmd_line_type cmd_line;
    return cmd_line.main(argc, argv);
}
