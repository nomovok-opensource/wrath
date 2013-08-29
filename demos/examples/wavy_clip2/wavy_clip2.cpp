/*! 
 * \file wavy_clip2.cpp
 * \brief file wavy_clip2.cpp
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
#include "WRATHLayerItemWidgetsTranslate.hpp"
#include "WRATHLayerItemWidgets.hpp"
#include "WRATHGenericWidget.hpp"
#include "WRATHTime.hpp"

#include "wrath_demo.hpp"
#include "wrath_demo_image_support.hpp"

#include "rect_attribute_packer.hpp"
#include "wobbly_node.hpp"
#include "augmented_node.hpp"

/*!\details
  In this example we use the wobbly rings
  defined in \ref rect3_example onto the 
  effect made in \ref clip2_example.
 */

class cmd_line_type:public DemoKernelMaker
{
public:
  command_line_argument_value<int> m_num_clip_widgets;
  command_line_argument_value<int> m_num_widgets;

  cmd_line_type(void):
    m_num_clip_widgets(3, "num_clip_widgets", "number of clip widgets", *this),
    m_num_widgets(165, "num_widgets", "number widgets", *this)
  {}

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

void
bound_and_v(float &in, 
            float &speed, 
            float delta_t,
            range_type<float> bounds)
{
  float delta, p;

  delta=delta_t*speed;
  p=in + delta;

  if(p<bounds.m_begin)
    {
      p = bounds.m_begin + std::abs(bounds.m_begin-p);
      speed=std::abs(speed);
    }
  else if(p>bounds.m_end)
    {
      p = bounds.m_end - std::abs(p-bounds.m_end);
      speed=-std::abs(speed);
    }
  in=p;
}






//define the node type to use
typedef WRATHLayerItemNodeTranslate BaseNode;
typedef RingNode<BaseNode> BaseRingNode;
typedef WobblyNode<BaseRingNode> Node;

//define the family types
typedef WRATHLayerItemWidget<Node>::FamilySet FamilySet;
typedef FamilySet::PlainFamily PlainFamily;
typedef FamilySet::RepeatXRepeatYImageFamily ImageFamily;
typedef FamilySet::ColorFamily ColorFamily;

//define the widget types
typedef PlainFamily::RectWidget PlainWidget;
typedef ImageFamily::RectWidget ImageWidget;
typedef ColorFamily::RectWidget ColorWidget;

/*
  conveniance function to make our ring widgets
 */
template<typename T>
T*
make_widget(WRATHLayer *layer,
            WRATHShaderBrushSourceHoard &hoard,
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
  
    
  //use the WRATHShaderBrushSourceHoard, m_shader_hoard,
  //to fetch/get the shader for the brush.
  const WRATHShaderSpecifier *sp;
  sp=&hoard.fetch(brush, 
                  WRATHBaseSource::mediump_precision,
                  WRATHShaderBrushSourceHoard::linear_brush_mapping);
 
  //now pass that as the drawer for the rectwidget,
  //use the shader of sp and augment the GL state
  //with the brush
  WRATHRectItemTypes::Drawer drawer(sp, ExampleRectAttributePacker::fetch(), tp);
  
  //we need to add the brush draw state (i.e. what image, gradient, etc) 
  //to the draw state of the item.
  hoard.add_state(brush, drawer.m_draw_passes[0].m_draw_state);
  
  widget=WRATHNew T(layer, drawer);
  widget->set_parameters(WRATHReferenceCountedObject::handle());   
  
  //some properties of the brush need to be transmitted
  //to the node of the widget, set_from_brush() does that
  widget->set_from_brush(brush);
  
  return widget;
}


/*
  ClipWidget represents 3 widgets in total:
  - an outer ring at the bottom
  - an inner ring for clip out
  - an inner ring for clip in, that widget is on the
    child layer.
 */
class ClipWidget:boost::noncopyable
{
public:
  ClipWidget(vec2 pos, 
             WRATHShaderBrushSourceHoard &shader,
             WRATHLayer *layer,
             WRATHLayer *child_layer,
             vec4 ring_color):
    m_position(pos),
    m_velocity((float)rand()/RAND_MAX*180.0f - 70.0f, 
               (float)rand()/RAND_MAX*180.0f - 70.0f),
    m_wobble_freq(100.0f),
    m_wobble_magnitude(1.0f),
    m_wobble_phase(0.0f)
  {
    m_clip_out_widget=make_widget<PlainWidget>(layer, 
                                               shader, 
                                               WRATHDrawType(0, WRATHDrawType::clip_outside_draw),
                                               NULL);

    m_clip_in_widget=make_widget<PlainWidget>(child_layer, 
                                              shader, 
                                              WRATHDrawType(0, WRATHDrawType::clip_inside_draw),
                                              NULL);

    m_ring_widget=make_widget<ColorWidget>(layer, 
                                           shader, 
                                           WRATHDrawType(0, WRATHDrawType::opaque_draw),
                                           NULL);
    m_ring_widget->color(ring_color);
  }

  ~ClipWidget()
  {
    WRATHDelete(m_clip_out_widget);
    WRATHDelete(m_clip_in_widget);
    WRATHDelete(m_ring_widget);
  }

  PlainWidget *m_clip_out_widget;
  PlainWidget *m_clip_in_widget;
  ColorWidget *m_ring_widget;

  vec2 m_position, m_velocity;
  float m_inner, m_outer;                       
  float m_wobble_freq, m_wobble_magnitude, m_wobble_phase;
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

  typedef WRATHLayerTranslateFamilySet::SimpleXSimpleYImageFamily::RectWidget BackgroundWidget;

  BackgroundWidget*
  make_image_widget(WRATHLayer *layer, WRATHImage *img);

  void
  load_images(void);

  void
  animate_ring(int i,
               float delta_t, 
               vec2 &velocity,
               float &inner_radius,
               float &outer_radius,
               vec2 &position,
               float &phase, float &freq, float &amplitude);

  void
  animate_ring(Node *n, int i, float delta_t);
  
  void
  animate_ring(ClipWidget *n, int i, float delta_t);


  WRATHShaderBrushSourceHoard m_shader_hoard;

  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_layer;

  WRATHLayer *m_child_layer;
  BackgroundWidget *m_background_widget, *m_background_widget2;
  std::vector<ImageWidget*> m_widgets;
  std::vector<WRATHImage*> m_images;
  std::vector<ClipWidget*> m_clip_widgets;

  WRATHTime m_time, m_total_time;
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
                 .add_source("wobbly.vert.glsl", WRATHGLShader::from_resource),
                 WRATHGLShader::shader_source()
                 .add_source("wobbly.frag.glsl", WRATHGLShader::from_resource)),
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

  /*
    now we make a child layer of m_layer 
   */
  m_child_layer=WRATHNew WRATHLayer(m_layer);
  
  
  const int num_clip_widgets=cmd_line->m_num_clip_widgets.m_value;
  const int num_widgets=cmd_line->m_num_widgets.m_value;

  /*
    create our clip widgets, we will set their z_order values
    so that:
     - m_ring_widget is in order of ctor 
     - m_clip_out_widget of each is above all m_ring_widget
       AND below all elements in m_widgets
     - m_clip_in_widget is above all m_clip_out_widget 
    
    So what happens is that when 2 clipwidget intesect
    their rings are not drawn over the clip inside region
    of either one.
  */
  for(int i=0; i<num_clip_widgets; ++i)
    {
      ClipWidget *N;
      vec2 pos;
      float r1, r2;

      r1=static_cast<float>(rand())/static_cast<float>(RAND_MAX);
      r2=static_cast<float>(rand())/static_cast<float>(RAND_MAX);
      
      pos=vec2( r1*width(), r2*height());

      N=WRATHNew ClipWidget(pos,
                            m_shader_hoard,
                            m_layer, m_child_layer,
                            vec4(0.0f, 0.0f, 0.0f, 1.0f));

      N->m_ring_widget->z_order(-i);
      N->m_clip_out_widget->z_order(-i-num_clip_widgets);
      N->m_clip_in_widget->z_order(-i-2*num_clip_widgets);

      m_clip_widgets.push_back(N);
    }
  

  load_images();

  //make a background widget for m_layer:
  m_background_widget=make_image_widget(m_layer, m_images[0]);
  m_background_widget->z_order(1); //below everything in m_layer.

  //make a background widget for m_child_layer
  m_background_widget2=make_image_widget(m_child_layer, m_images[1]);
  m_background_widget2->z_order(-3*num_clip_widgets); //below everything in m_child_layer.


  for(int i=0, zi=-3*num_clip_widgets-1; i<num_widgets; ++i, --zi)
    {
      ImageWidget *widget;
      WRATHImage *img;

      img=m_images[rand()%m_images.size()];
      widget=make_widget<ImageWidget>(m_child_layer,  
                                      m_shader_hoard,
                                      WRATHDrawType::opaque_pass(),
                                      img);

      widget->translation(vec2(rand()%width(), rand()%height()) );
      //      widget->rotation( static_cast<float>(rand())/RAND_MAX*M_PI);

      widget->m_velocity=vec2( (float)rand()/RAND_MAX*190.0f - 95.0f, 
                               (float)rand()/RAND_MAX*190.0f - 95.0f);

      widget->m_inner_radius=rand()%100;
      widget->m_outer_radius=100 + rand()%100;

      //make the z-order of widget decrease from -1,
      //thus layer widgets are drawn on top.
      widget->z_order(zi);
      m_widgets.push_back(widget);
    }
                                                     
  
                  
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

ClipExample::
~ClipExample()
{
  /*
    delete m_clip_widgets before m_layer
    because their dtor will delete 
    their widgets
   */
  for(unsigned int i=0, endi=m_clip_widgets.size(); i<endi; ++i)
    {
      WRATHDelete(m_clip_widgets[i]);
    }

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

ClipExample::BackgroundWidget*
ClipExample::
make_image_widget(WRATHLayer *layer, WRATHImage *img)
{
  BackgroundWidget *R;
  WRATHBrush brush(img);
  brush.flip_image_y(true); 
    
  BackgroundWidget::set_shader_brush(brush);
  
  R=WRATHNew BackgroundWidget(layer, brush);
  R->set_from_brush(brush);
  
  WRATHDefaultRectAttributePacker::Rect::handle rect;    
  rect=WRATHNew WRATHDefaultRectAttributePacker::Rect(width(), height());
  rect->m_brush_stretch=vec2(m_images[0]->size())/vec2(size());

  R->set_parameters(rect);
  R->position(vec2(0.0f, 0.0f));
  return R;
}

void
ClipExample::
load_images(void)
{
  WRATHImage::ImageFormat fmt;
  
  std::vector<std::string> name;
  name.push_back("images/hands.jpg");
  name.push_back("images/image1.jpg");
  name.push_back("images/light5.jpg");
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
resize(ivec2 new_size, ivec2 old_size)
{
  float_orthogonal_projection_params proj_params(0, new_size.x(), new_size.y(), 0);
  m_layer->simulation_matrix(WRATHLayer::projection_matrix, float4x4(proj_params));
  glViewport(0, 0, new_size.x(), new_size.y());

  WRATHDefaultRectAttributePacker::Rect::handle rect;  
  
  rect=WRATHNew WRATHDefaultRectAttributePacker::Rect(new_size.x(), new_size.y());
  rect->m_brush_stretch=vec2(m_images[0]->size())/vec2(new_size);
  m_background_widget->set_parameters(rect); 

  rect->m_brush_stretch=vec2(m_images[1]->size())/vec2(new_size);
  m_background_widget2->set_parameters(rect);

  
  for(unsigned int i=0, endi=m_widgets.size(); i<endi; ++i)
    {
      vec2 pos;
      pos=m_widgets[i]->position();
      pos.x()*=static_cast<float>(new_size.x())/static_cast<float>(old_size.x());
      pos.y()*=static_cast<float>(new_size.y())/static_cast<float>(old_size.y());
      m_widgets[i]->position(pos);
    }
}


void
ClipExample::
animate_ring(int i,
             float delta_t, 
             vec2 &velocity,
             float &inner_radius,
             float &outer_radius,
             vec2 &position,
             float &out_phase, float &out_freq, float &out_amplitude)
{
  float cycle, radius, phase;
  uint32_t modulas, total_time;
  uint32_t period_in_ms(1500);
  float freq( 2.0f*M_PI/ static_cast<float>(period_in_ms));

  phase= static_cast<float>(i) * M_PI/12.0f;
  /*
    we want it cyclic; first we take the modulas in integer
    arithmatic.
   */
  total_time=m_total_time.elapsed();
  modulas=total_time%period_in_ms;
  cycle=std::sin( freq*static_cast<float>(modulas) + phase);
  cycle = (cycle + 1.0)/2.0;


  radius = cycle*5.0*(i%10 + 1) + 5.0*(i%10 + 1);
  outer_radius=radius + 2.0*(i%20);
  inner_radius=radius - 2.0*(i%20);

  //set phase, freq and amplitide of wobble:
  int phase_offset(30*i);
  period_in_ms=1000;
  modulas=(total_time+phase_offset) % period_in_ms;
  cycle=static_cast<float>(modulas)/static_cast<float>(period_in_ms);

  out_phase=cycle*M_PI*2.0f;
  out_amplitude= radius/2.0f;
  out_freq=outer_radius;

   
  bound_and_v(position.x(), 
              velocity.x(), 
              delta_t, 
              range_type<float>(0.0f, static_cast<float>(width()) ) );

  bound_and_v(position.y(), 
              velocity.y(), 
              delta_t, 
              range_type<float>(0.0f, static_cast<float>(height()) ) );
}

void
ClipExample::
animate_ring(Node *n, int i, float delta_t)
{
  vec2 pos(n->position());
  animate_ring(i, delta_t, n->m_velocity, 
               n->m_inner_radius, n->m_outer_radius,
               pos,
               n->m_wobble_phase, n->m_wobble_freq, n->m_wobble_magnitude);
  n->position(pos);
}

void
ClipExample::
animate_ring(ClipWidget *n, int i, float delta_t)
{
  animate_ring(i, delta_t, n->m_velocity, 
               n->m_inner, n->m_outer,
               n->m_position,
               n->m_wobble_phase, n->m_wobble_freq, n->m_wobble_magnitude);


  n->m_inner=std::abs(n->m_inner*2.0f);
  n->m_outer=n->m_inner + 30.0f;
  
  n->m_clip_in_widget->m_inner_radius=0.0f;
  n->m_clip_in_widget->m_outer_radius=n->m_inner;
  n->m_clip_in_widget->m_wobble_phase=n->m_wobble_phase;
  n->m_clip_in_widget->m_wobble_freq=n->m_wobble_freq;
  n->m_clip_in_widget->m_wobble_magnitude=n->m_wobble_magnitude;
  n->m_clip_in_widget->position(n->m_position);
  
  n->m_clip_out_widget->m_inner_radius=0.0f;
  n->m_clip_out_widget->m_outer_radius=n->m_inner;
  n->m_clip_out_widget->m_wobble_phase=n->m_wobble_phase;
  n->m_clip_out_widget->m_wobble_freq=n->m_wobble_freq;
  n->m_clip_out_widget->m_wobble_magnitude=n->m_wobble_magnitude;
  n->m_clip_out_widget->position(n->m_position);
  
  n->m_ring_widget->m_inner_radius=n->m_inner;
  n->m_ring_widget->m_outer_radius=n->m_outer;
  n->m_ring_widget->m_wobble_phase=n->m_wobble_phase;
  n->m_ring_widget->m_wobble_freq=n->m_wobble_freq;
  n->m_ring_widget->m_wobble_magnitude=n->m_wobble_magnitude;
  n->m_ring_widget->position(n->m_position);
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
      animate_ring(m_widgets[i], i, delta_t);
    }

  for(unsigned int i=0, endi=m_clip_widgets.size(); i<endi; ++i)
    {
      animate_ring(m_clip_widgets[i], i+m_widgets.size(), delta_t);
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
