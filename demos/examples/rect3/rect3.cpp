/*! 
 * \file rect3.cpp
 * \brief file rect3.cpp
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


#include "WRATHConfig.hpp"
#include "WRATHNew.hpp"
#include "vecN.hpp"
#include "WRATHNew.hpp"
#include "WRATHmalloc.hpp"
#include "WRATHUtil.hpp"
#include "WRATHWidget.hpp"
#include "WRATHTime.hpp"
#include "WRATHLayerItemNodeTranslate.hpp"
#include "WRATHLayerItemWidgets.hpp"

#include "wrath_demo.hpp"
#include "wrath_demo_image_support.hpp"

#include "augmented_node.hpp"
#include "wobbly_node.hpp"
#include "rect_attribute_packer.hpp"

/*!\details
  In this example we will create a custom shader
  compatible with \ref WRATHDefaultRectAttributePacker.
  The shader will be equipped to do non-linear brush
  remapping to make the image and gradient wobble.
*/

class cmd_line_type:public DemoKernelMaker
{
public:
  command_line_argument_value<std::string> m_image;

  cmd_line_type(void):
    m_image("images/eye.jpg", "image", "Image to use for demo", *this)
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

class RectExample:public DemoKernel
{
public:
  RectExample(cmd_line_type *cmd_line);
  ~RectExample();
  
  void resize(int width, int height);
  virtual void handle_event(FURYEvent::handle ev);
  virtual void paint(void);

private:
  typedef WRATHLayerItemNodeTranslate BaseNode;
  typedef RingNode<BaseNode> BaseRingNode;
  typedef WobblyNode<BaseRingNode> Node;

  typedef WRATHLayerItemWidget<Node>::FamilySet FamilySet; 
  typedef FamilySet::ColorFamily ColorFamily;
  typedef FamilySet::LinearGradientFamily ColorLinearGradientFamily;
  typedef FamilySet::RadialGradientFamily ColorRadialGradientFamily;
  typedef FamilySet::RepeatXRepeatYImageFamily ImageFamily;
  typedef FamilySet::RadialGradientRepeatXRepeatYImageFamily RadialGradientImageFamily;

  //conveniance template function to make widgets
  template<typename T>
  T*
  make_widget(WRATHGradient *grad, WRATHImage *image)
  {
    T *return_value;
    
    //define the WRATHBrush to apply to the returned widget
    WRATHBrush brush(grad, image);
    brush.flip_image_y(true); 
    
    //set the shaders for the brush from the node type
    T::Node::set_shader_brush(brush);

    //use the WRATHShaderBrushSourceHoard, m_shader_hoard,
    //to fetch/get the shader for the brush. 
    //we also specify that the brush mapping is non-linear
    //so that we can specify the brush coordinates in the
    //fragment shader.
    const WRATHShaderSpecifier *sp;
    sp=&m_shader_hoard.fetch(brush, 
                             WRATHBaseSource::mediump_precision,
                             WRATHShaderBrushSourceHoard::nonlinear_brush_mapping);

    //now pass that as the drawer for the rectwidget,
    //use the shader of sp and augment the GL state
    //with the brush
    WRATHRectItemTypes::Drawer drawer;

    drawer=WRATHRectItemTypes::Drawer(sp,
                                      ExampleRectAttributePacker::fetch(),
                                      WRATHDrawType::opaque_pass());
    m_shader_hoard.add_state(brush, drawer.m_draw_passes[0].m_draw_state);
    
    //create the widget
    return_value=WRATHNew T(m_layer, drawer);
    
    //set the values of the node from the brush.
    return_value->set_from_brush(brush);
    
    if(image!=NULL)
      {
         return_value->m_outer_radius=0.5f*std::min(image->size().x(),
						    image->size().y());
      }
    else
      {
        return_value->m_outer_radius=100.0f;
      }

    /*
      packing of attribute data takes place on set_parameters,
      until this is called the rect does not have it's
      attribute data set.
     */
    return_value->set_parameters(ExampleRectAttributePacker::rect_properties());    

    
    return_value->m_inner_radius=0.4f*return_value->m_outer_radius;
    return_value->z_order(m_widget_count);
    return_value->position(vec2(m_widget_count*10, m_widget_count*10));
    ++m_widget_count;
    
    return return_value;
  }
    
  
  WRATHGradient*
  make_gradient(void);

  WRATHImage*
  make_image(const std::string&);

  void
  move_node(int phase_offset, 
	    Node *pnode, float delta_t);

  WRATHShaderBrushSourceHoard m_shader_hoard;

  WRATHGradient *m_gradient;
  WRATHImage *m_image;
  int m_widget_count;

  ColorFamily::RectWidget *m_colored_widget;
  ColorLinearGradientFamily::RectWidget *m_lin_gr_widget;
  ColorRadialGradientFamily::RectWidget *m_rad_gr_widget;
  ImageFamily::RectWidget *m_image_widget;
  RadialGradientImageFamily::RectWidget *m_image_rad_gr_widget;


  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_layer;
  WRATHTime m_time, m_total_time;
  bool m_first_frame;
};



WRATHGradient*
RectExample::
make_gradient(void)
{
  WRATHGradient *R;
  R=WRATHNew WRATHGradient("my gradient");
  R->set_color(0.00f, WRATHGradient::color(1.0f, 0.0f, 0.0f, 1.0f));
  R->set_color(0.25f, WRATHGradient::color(0.0f, 1.0f, 0.0f, 1.0f));
  R->set_color(0.50f, WRATHGradient::color(0.0f, 0.0f, 1.0f, 1.0f));
  R->set_color(0.75f, WRATHGradient::color(1.0f, 1.0f, 1.0f, 1.0f));
  return R;
}

WRATHImage*
RectExample::
make_image(const std::string &pname)
{
  WRATHImage *R;
  WRATHImage::ImageFormat fmt;

  fmt
    .internal_format(GL_RGBA)
    .pixel_data_format(GL_RGBA)
    .pixel_type(GL_UNSIGNED_BYTE)
    .magnification_filter(GL_LINEAR)
    .minification_filter(GL_LINEAR)
    .automatic_mipmap_generation(false);
    

  R=WRATHDemo::fetch_image(pname, fmt);
  if(R==NULL)
    {
      R=WRATHNew WRATHImage(std::string("failed to load\"") + pname + "\"",
                            ivec2(2,2), 
                            fmt);
      
      int num_pixels(R->size().x()*R->size().y());
      int num_bytes(num_pixels*R->image_format(0).m_pixel_format.bytes_per_pixel());
      std::vector<uint8_t> pixels(num_bytes);
      c_array<uint8_t> raw_pixels(pixels);
      c_array<vecN<uint8_t,4> > pixels_vs;
      pixels_vs=raw_pixels.reinterpret_pointer<vecN<uint8_t,4> >();
      
      pixels_vs[0]=vecN<uint8_t,4>(0, 0, 0, 255);
      pixels_vs[1]=vecN<uint8_t,4>(255, 255, 255, 255);
      pixels_vs[2]=vecN<uint8_t,4>(255, 255, 255, 255);
      pixels_vs[3]=vecN<uint8_t,4>(0, 0, 0, 255);
      

      R->respecify_sub_image(0, //layer,
                             0, //LOD
                             R->image_format(0).m_pixel_format, //pixel format
                             pixels, //pixel data
                             ivec2(0,0), //bottom left corner
                             R->size());
    }
  
  return R;
}

RectExample::
RectExample(cmd_line_type *cmd_line):
  DemoKernel(cmd_line),
  m_shader_hoard(WRATHGLShader::shader_source()
                 .add_source("wobbly.vert.glsl", WRATHGLShader::from_resource),
                 WRATHGLShader::shader_source()
                 .add_source("wobbly.frag.glsl", WRATHGLShader::from_resource)),
  m_widget_count(0),
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

  m_image=make_image(cmd_line->m_image.m_value);
  m_gradient=make_gradient();

  m_colored_widget=make_widget<ColorFamily::RectWidget>(NULL, NULL);
  m_colored_widget->m_velocity=vec2(300.0f, 100.0f);

  m_lin_gr_widget=make_widget<ColorLinearGradientFamily::RectWidget>(m_gradient, NULL);
  m_lin_gr_widget->m_velocity=vec2(120.0f, -155.0f);

  m_rad_gr_widget=make_widget<ColorRadialGradientFamily::RectWidget>(m_gradient, NULL);
  m_rad_gr_widget->m_velocity=vec2(-34.0f, 133.0f);

  m_image_rad_gr_widget=make_widget<RadialGradientImageFamily::RectWidget>(m_gradient, m_image);  
  m_image_rad_gr_widget->m_velocity=vec2(130.0f, -220.0f);

  m_image_widget=make_widget<ImageFamily::RectWidget>(NULL, m_image);
  m_image_widget->m_velocity=vec2(80.0f, 60.0f);

                    
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

RectExample::
~RectExample()
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
RectExample::
resize(int width, int height)
{
  float_orthogonal_projection_params proj_params(0, width, height, 0);
  m_layer->simulation_matrix(WRATHLayer::projection_matrix, float4x4(proj_params));
  glViewport(0, 0, width, height);
}

void
RectExample::
move_node(int phase_offset, Node *pnode, float delta_t)
{
  vec2 oldp, newp;

  oldp=pnode->position();
  
  newp=oldp + delta_t*pnode->m_velocity;
  pnode->position(newp);

  //make the center within the screen.
  newp+=pnode->m_outer_radius;

  if(newp.x()<0.0f or newp.x()>static_cast<float>(width()) )
    {
      pnode->m_velocity.x()=-pnode->m_velocity.x();
    }

  if(newp.y()<0.0f or newp.y()>static_cast<float>(height()) )
    {
      pnode->m_velocity.y()=-pnode->m_velocity.y();
    }



  uint32_t modulas, period_in_ms(1000);
  float cycle;

  modulas=( phase_offset+m_total_time.elapsed() ) % period_in_ms;
  cycle=static_cast<float>(modulas)/static_cast<float>(period_in_ms);

  pnode->m_wobble_phase=cycle*M_PI*2.0f;

  float mm(pnode->m_inner_radius*0.5f);
  pnode->m_wobble_magnitude= mm + 35.0f;



  pnode->m_wobble_freq=pnode->m_outer_radius;
}

void 
RectExample::
paint(void)
{
  /*
    on the first frame, we make delta_t
    but we also need to restart m_time
    because it started at its construction.
   */
  float secs;
  secs=static_cast<float>(m_time.restart())/1000.0f;
  secs=m_first_frame?
    0.0f:secs;

  //move them around
  move_node(0, m_colored_widget, secs);
  move_node(200, m_lin_gr_widget, secs);
  move_node(300, m_rad_gr_widget, secs);
  move_node(400, m_image_widget, secs);
  move_node(500, m_image_rad_gr_widget, secs);

  //make the colors and gradient move around
  int32_t e;
  float s, c;
  vec2 r;
  e=m_total_time.elapsed()%4000; //4 second cycle

  sincosf( static_cast<float>(e)*2.0f*M_PI/4000.0f, &s, &c);
  m_colored_widget->color( WRATHGradient::color(0.5f + 0.5f*s,
                                                0.5f + 0.5f*c,
                                                (c+s+2.0f)/4.0f,
                                                1.0f));
  m_lin_gr_widget->set_gradient( vec2(100.0f*c, 100.0f*s),
                                 vec2(0.0f, c*c));

  r=vec2(m_rad_gr_widget->m_outer_radius);
  m_rad_gr_widget->set_gradient(r - vec2(s,c)*r, 0.0f,
                                r - vec2(s,c)*r, (c+2.0f)*std::max(r.x(), r.y()));

  r=vec2(m_image_rad_gr_widget->m_outer_radius);
  m_image_rad_gr_widget->set_gradient(r + vec2(s,c)*r, 0.0f,
                                      r + vec2(s,c)*r, (s+2.0f)*std::max(r.x(), r.y()));


  m_tr->signal_complete_simulation_frame();
  m_tr->signal_begin_presentation_frame();
  m_layer->clear_and_draw();
  m_first_frame=false;

  update_widget();
}

void 
RectExample::
handle_event(FURYEvent::handle ev)
{
  if(ev->type()==FURYEvent::Resize)
    {
      FURYResizeEvent::handle rev(ev.static_cast_handle<FURYResizeEvent>());
      resize(rev->new_size().x(), rev->new_size().y());
    }
}



DemoKernel* 
cmd_line_type::
make_demo(void)
{
  return WRATHNew RectExample(this);
}
  

int 
main(int argc, char **argv)
{
    cmd_line_type cmd_line;
    return cmd_line.main(argc, argv);
}
