/*! 
 * \file clip.cpp
 * \brief file clip.cpp
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
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdlib.h>
#include "vecN.hpp"
#include "WRATHNew.hpp"
#include "WRATHUtil.hpp"
#include "WRATHLayerItemWidgetsRotateTranslate.hpp"
#include "WRATHLayerItemWidgets.hpp"
#include "WRATHGenericWidget.hpp"
#include "WRATHTime.hpp"

#include "wrath_demo.hpp"
#include "wrath_demo_image_support.hpp"

#include "item_packer.hpp"
#include "item.hpp"
#include "augmented_node.hpp"

/*!\details
  In this example we build from the node
  type we made in \ref augmented_node_example
  to demo clipping with our custom node type.
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

class ClipExample:public DemoKernel
{
public:
  ClipExample(cmd_line_type *cmd_line);
  ~ClipExample();
  
  void resize(ivec2 new_size, ivec2 old_size);
  virtual void handle_event(FURYEvent::handle ev);
  virtual void paint(void);

private:

  //define the node type to use
  typedef WRATHLayerItemNodeRotateTranslate BaseNode;
  typedef RingNode<BaseNode> Node;

  //define the family types
  typedef WRATHLayerItemWidget<Node>::FamilySet FamilySet;
  typedef FamilySet::CPlainFamily PlainFamily;
  typedef FamilySet::CSimpleXSimpleYImageFamily ImageFamily;
  typedef FamilySet::CColorFamily ColorFamily;

  //define the widget types
  typedef WRATHGenericWidget<item, PlainFamily::WidgetBase> PlainWidget;
  typedef WRATHGenericWidget<item, ImageFamily::WidgetBase> ImageWidget;
  typedef WRATHGenericWidget<item, ColorFamily::WidgetBase> ColorWidget;

  template<typename T>
  T*
  make_widget(WRATHLayer *layer,
              int N, 
              WRATHDrawType tp,
              WRATHImage *image)
  {
    T *widget;
    WRATHBrush brush(image);

    brush.flip_image_y(true);

    //the Node class has the static method set_shader_brush 
    //to set the shader code associated for the brush
    //for details, see \ref WRATHLayerItemNodeBase::set_shader_brush
    T::Node::set_shader_brush(brush);

    item::parameters params;
    params.m_polygon_spec.m_number_sides=N;

    //use the WRATHShaderBrushSourceHoard, m_shader_hoard,
    //to fetch/get the shader for the brush.
    const WRATHShaderSpecifier *sp;
    sp=&m_shader_hoard.fetch(brush, WRATHBaseSource::mediump_precision);
    params.m_drawer=item::Drawer(sp,
                                 ItemAttributePacker::example_packer(),
                                 tp);

    //we need to add the brush draw state (i.e. what gradient) 
    //to the draw state of the item.
    m_shader_hoard.add_state(brush, params.m_drawer.m_draw_passes[0].m_draw_state);

    widget=WRATHNew T(layer, params);

    //some properties of the brush need to be transmitted
    //to the node of the widget, set_from_brush() does that
    widget->set_from_brush(brush);

    return widget;
  }

  void
  load_images(void);

  void
  set_border_and_clip(float clip_radius, 
                      float border_thickness,
                      vec2 pt);

  void
  move_widget(Node *n, vec2 delta);

  
  WRATHShaderBrushSourceHoard m_shader_hoard;

  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_layer;
  ColorWidget *m_border_widget;

  WRATHLayer *m_child_layer;
  PlainWidget *m_clip_in_widget;
  std::vector<ImageWidget*> m_widgets;
  std::vector<WRATHImage*> m_images;

  WRATHTime m_time;
  bool m_first_frame;
};



DemoKernel* 
cmd_line_type::
make_demo(void)
{
  return WRATHNew ClipExample(this);
}
  

ClipExample::
ClipExample(cmd_line_type *cmd_line):
  DemoKernel(cmd_line),
  m_shader_hoard(WRATHGLShader::shader_source()
                 .add_source("item.vert.glsl", WRATHGLShader::from_resource),
                 WRATHGLShader::shader_source()
                 .add_source("item.frag.glsl", WRATHGLShader::from_resource)),
  m_first_frame(true)
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

  m_layer->simulation_matrix(WRATHLayer::projection_matrix, float4x4(proj_params));
  
  const int num_sides_on_clip=40;
  m_border_widget=make_widget<ColorWidget>(m_layer, num_sides_on_clip, 
                                           WRATHDrawType::opaque_pass(),
                                           NULL);
  m_border_widget->color( vec4(0.0f, 0.0f, 0.0f, 1.0f)); //black border.
  m_border_widget->z_order(0);


  /*
    now we make a child layer of m_layer 
   */
  m_child_layer=WRATHNew WRATHLayer(m_layer);

  /*
    create a clipping inside item to m_child_layer.
    the clipping applied to a WRATHLayer L is
    pixels within RegionIn(L) which is the
    intersection of RegionIn(L->parent) with
    LocalRegionIn(L) where LocalRegionIn(L)  
    is defined as the UNION of all items of 
    L which  have their type as 
    WRATHDrawType::clip_inside_draw.
    If there are no such items, then 
    LocalRegionIn(L) is the entire 
    screen space.
   */
  m_clip_in_widget=make_widget<PlainWidget>(m_child_layer, num_sides_on_clip, 
                                            WRATHDrawType(0, WRATHDrawType::clip_inside_draw),
                                            NULL);
  m_clip_in_widget->z_order(1); //make m_clip_in_widget just below the border

  set_border_and_clip(width()/4.0, 50.0f, vec2(width()/2, height()/2));

  load_images();

  /*
    now create the widgets
   */
  const int num_polys = 15;
  for(int i=0; i<num_polys; ++i)
    {
      ImageWidget *widget;
      WRATHImage *img;
      int num_sides;

      img=m_images[rand()%m_images.size()];
      num_sides=rand()%5 + 3;
      widget=make_widget<ImageWidget>(m_child_layer, num_sides, 
                                      WRATHDrawType::opaque_pass(),
                                      img);

      widget->translation(vec2(rand()%width(), rand()%height()) );
      widget->rotation( static_cast<float>(rand())/RAND_MAX*M_PI);

      widget->m_inner_radius=rand()%100;
      widget->m_outer_radius=100 + rand()%100;

      //make the z-order of widget decrease from -1,
      //thus layer widgets are drawn on top.
      widget->z_order(-1-i);

      m_widgets.push_back(widget);
    }
                                                     
  
                  
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

ClipExample::
~ClipExample()
{
  if(m_layer!=NULL)
    {
      WRATHPhasedDelete(m_layer);
    }
  
  /*
    Delete all resources
   */
  WRATHResourceManagerBase::clear_all_resource_managers();

  /*
    purge cleanup to perform post processing cleanup
    tasks(typically deletion of GL objects).
   */
  m_tr->purge_cleanup();
  m_tr=NULL;
}


void
ClipExample::
load_images(void)
{
  WRATHImage::ImageFormat fmt;
  
  std::vector<std::string> name;
  name.push_back("images/eye.jpg");
  name.push_back("images/hands.jpg");
  name.push_back("images/light5.jpg");
  name.push_back("images/image1.jpg");
  name.push_back("images/image.png");
  
  for(std::vector<std::string>::const_iterator
        iter=name.begin(), end=name.end();
      iter!=end; ++iter)
    {
      WRATHImage::WRATHImageID id(*iter);
      //helper function
      WRATHImage* im = WRATHDemo::fetch_image(id, fmt);
      m_images.push_back(im);
    }
}


void
ClipExample::
set_border_and_clip(float clip_radius, float border_thickness, vec2 pt)
{
  m_clip_in_widget->m_inner_radius=0.0f;
  m_clip_in_widget->m_outer_radius=clip_radius;
  m_clip_in_widget->position(pt);

  m_border_widget->m_inner_radius=clip_radius;
  m_border_widget->m_outer_radius=clip_radius+border_thickness;
  m_border_widget->position(pt);
}


void
ClipExample::
resize(ivec2 new_size, ivec2 old_size)
{
  float_orthogonal_projection_params proj_params(0, new_size.x(), new_size.y(), 0);
  m_layer->simulation_matrix(WRATHLayer::projection_matrix, float4x4(proj_params));
  glViewport(0, 0, new_size.x(), new_size.y());

  set_border_and_clip(new_size.x()/4.0, 50.0f, vec2(new_size.x()/2, new_size.y()/2));
  for(unsigned int i=0, endi=m_widgets.size(); i<endi; ++i)
    {
      vec2 pos;

      pos=m_widgets[i]->position();
      pos.y()*=static_cast<float>(new_size.y())/static_cast<float>(old_size.y());
      m_widgets[i]->position(pos);
    }
}

void
ClipExample::
move_widget(Node *n, vec2 delta)
{
  vec2 pos;
	
  pos=n->position() + delta;
  pos=vec2(fmod(pos.x(), width()), fmod(pos.y(), height()));
  n->position(pos);
}

void 
ClipExample::
paint(void)
{
  /*
    on the first frame, we make delta_t 0.0
    but we also need to restart m_time
    because it started at its construction.
   */
  float delta_t;
  delta_t=static_cast<float>(m_time.restart())/1000.0f;
  delta_t=m_first_frame?
    0.0f:
    delta_t;

  float angle(delta_t*2.5f);
  std::complex<float> rot( cosf(angle), sinf(angle));
  vecN<std::complex<float>, 2> rots( rot, std::conj(rot));
  
  for(unsigned int i=0, endi=m_widgets.size(); i<endi; ++i)
    {
      float delta_x;

      delta_x= static_cast<float>(i+1) * delta_t*1000.0f/15.0f;

      move_widget(m_widgets[i], vec2(delta_x, 0.0));
      m_widgets[i]->rotation( m_widgets[i]->rotation()*rots[i&1]);
    }
  
  m_tr->signal_complete_simulation_frame();
  m_tr->signal_begin_presentation_frame();
  m_layer->clear_and_draw();

  update_widget();
  m_first_frame=false;
}

void 
ClipExample::
handle_event(FURYEvent::handle ev)
{
  if(ev->type()==FURYEvent::Resize)
    {
      FURYResizeEvent::handle rev(ev.static_cast_handle<FURYResizeEvent>());
      resize(rev->new_size(), rev->old_size());
    }
}

int 
main(int argc, char **argv)
{
    cmd_line_type cmd_line;
    return cmd_line.main(argc, argv);
}
