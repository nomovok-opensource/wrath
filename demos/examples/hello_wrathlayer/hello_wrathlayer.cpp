/*! 
 * \file hello_wrathlayer.cpp
 * \brief file hello_wrathlayer.cpp
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
#include <fstream>
#include <iomanip>
#include <limits>
#include "WRATHRectItem.hpp"
#include "WRATHDefaultRectAttributePacker.hpp"
#include "WRATHLayer.hpp"
#include "WRATHLayerItemNodeTranslate.hpp"
#include "WRATHTextureFontFreeType_Mix.hpp"
#include "WRATHTextureFontFreeType_CurveAnalytic.hpp"
#include "WRATHTextItem.hpp"
#include "WRATHTextDataStream.hpp"
#include "WRATHDefaultTextAttributePacker.hpp"
#include "wrath_demo_image_support.hpp"
#include "WRATHFontFetch.hpp"
#include "wrath_demo.hpp"
#include "WRATHFontShaderSpecifier.hpp"
#include "WRATHLayerItemWidgetsTranslate.hpp"

/*!\details
The demo creates the widget directly and places them
onto a WRATHLayer. The main purpose of the demo
is to show the use pre-made widget types.
*/



class wrathlayer_example:public DemoKernel
{
public:
  wrathlayer_example(DemoKernelMaker *parent);
  ~wrathlayer_example();

  virtual
  void
  paint(void);

  virtual
  void
  handle_event(FURYEvent::handle);

private:

  void
  set_text(void);


  /*
    The font we will use. Font's are viewed as
    shared resources, and their cleanup is handled
    via WRATHResourceManagerBase::clear_all_resource_managers().
    Font's derived from WRATHTextureFont are stored
    in textures (but what is stored in the textures
    typically allow for the font to be zoomed in
    but still maintain good render results).

    Selecting a font can be done via filename, face-index
    pair or using WRATHFontFetch to select the
    font.
   */
  typedef WRATHMixFontTypes<WRATHTextureFontFreeType_Analytic>::mix FontType;
  WRATHTextureFont *m_font;
  
  /*
    A Triple buffer enabler is required. It's main
    purpose is to allow for updating the data
    presented in a seperate thread than the data
    is drawn. 
   */
  WRATHTripleBufferEnabler::handle m_tr;

  /*
    A WRATHLayer is an object that holds
    (all) the data to draw. A WRATHLayer
    derives from WRATHCanvas so that
    one can create draw groups from it and a
    WRATHLayer has a draw method to
    draw it's draw groups. Additionally, a 
    WRATHLayer may have child 
    WRATHLayer objects.
   */
  WRATHLayer *m_contents;

  /*
    Use the family set defined by \ref WRATHLayerTranslateFamilySet
    to define the family's we wish to use. Recall
    that WRATHLayerTranslateFamilySet is just a typedef
    of \ref WRATHFamilySet setting the node type, etc
    correctly.
   */
  typedef WRATHLayerTranslateFamilySet::PlainFamily Basic;
  typedef WRATHLayerTranslateFamilySet::LinearGradientFamily LinearGradient;
  typedef WRATHLayerTranslateFamilySet::ColorFamily Color;
  typedef WRATHLayerTranslateFamilySet::ImageFamily ImageFamily;
  
  /*
    The text widget
   */
  Basic::TextWidget *m_text;

  /*
    The image widget
   */
  ImageFamily::RectWidget *m_image;

  /*
    The shape widget, note that the Shape
    type is from WRATHLayerTranslateLinearGradientWidget,
    that type provides a per-item gradient value.
   */
  LinearGradient::ShapeWidget *m_shape;


  /*
    a solid color shape item
   */
  Color::ShapeWidget *m_solid_shape;
  
  /*
    some stroking with const color,
    with color stored in the item
   */
  Color::ShapeWidget *m_stroke_shape;
  

  /*
    stroking with gradient madness
   */
  LinearGradient::ShapeWidget *m_stroke_with_gradient_shape;
  

  /*
    Gradient object, stores the colors of a gradient
   */
  WRATHGradient *m_gradient;


  bool m_text_dirty;

  /*
    a flag to indicate that resize of window
    happend.
   */
  bool m_resized;

  /*
   */
  vec3 m_pre_translate;
};

wrathlayer_example::
wrathlayer_example(DemoKernelMaker *parent):
  DemoKernel(parent),
  m_font(NULL),
  m_contents(NULL),
  m_text(NULL),
  m_image(NULL),
  m_shape(NULL),
  m_solid_shape(NULL),
  m_stroke_shape(NULL),
  m_stroke_with_gradient_shape(NULL),
  m_resized(false),
  m_pre_translate(0.0f)
{

  /*
    enable key repeat..
   */
  enable_key_repeat(true);
  
  /*
    We need to create font. Font's are viewed as
    shared resources, and their cleanup is handled
    via WRATHResourceManagerBase::clear_all_resource_managers().
    Font's derived from WRATHTextureFont are stored
    in textures (but what is stored in the textures
    typically allow for the font to be zoomed in
    but still maintain good render results).

    Selecting a font can be done via filename, face-index
    pair or using WRATHFontFetch to select the
    font.
  */
  //FontType::creation_texture_mode(FontType::local_pixel_coordinates);
  

  /*
    specify to WRATHFontFetch with what font type
    to use for creating fonts. Usually only called
    once for the lifetime of a program.
  */
  WRATHFontFetch::font_fetcher(type_tag<FontType>());


  m_font=WRATHFontFetch::fetch_font(48, /*pixel size for texture data of font */
                                     WRATHFontFetch::FontProperties()
                                     .family_name("Sans") //select Serif font
                                     .bold(true) //bold
                                     .italic(false) //no slanting
                                     );

  
  WRATHassert(m_font!=NULL);
  
    
  /*
    Create the triple buffer enabler which coordinates
    in an almost lock free fashion changing and drawing
    data. In the vast super-majority of cases, one needs
    only one triple buffer enabler per window.
   */
  m_tr=WRATHNew WRATHTripleBufferEnabler();

  /*
    Create the WRATHLayer which will
    draw our text item
   */
  m_contents=WRATHNew WRATHLayer(m_tr);

  /*
    Set the projection matrix for m_contents,
    a float_orthogonal_projection_params is a simple POD
    that provides arguments for creating a matrix
    representing an orthogonal matrix.
    The variable m_q comes from DemoKernel, it is
    a pointer to the QGLWidget to which we
    display.
   */
  float_orthogonal_projection_params proj_params(0, width(),
                                                 height(), 0);
  
  m_contents->simulation_matrix(WRATHLayer::projection_matrix,
                                float4x4(proj_params));

  /*
    Create our text widget, to be drawn
    on our canvas, m_contents.

    Once placed on m_contents, m_contents
    will delete m_text when m_contents
    is deleted. It is still legal to
    delete m_text before m_contents
    (for example if you want to just
    stop displaying the text of m_text)
   */
  m_text=WRATHNew 
    Basic::TextWidget(m_contents, //"canvas" were widget is placed 
                      WRATHTextItemTypes::text_transparent); 
  /*
    set positional properties of the text widget
   */
  m_text->z_order(0);

 

  


  WRATHImage *im;
  WRATHImage::ImageFormat fmt;
  fmt
    .internal_format(GL_RGBA)
    .pixel_data_format(GL_RGBA)
    .pixel_type(GL_UNSIGNED_BYTE)
    .magnification_filter(GL_LINEAR)
    .minification_filter(GL_LINEAR_MIPMAP_NEAREST)
    .automatic_mipmap_generation(true);
  im=WRATHDemo::fetch_image("images/1024x1024.png", fmt);
  if(im==NULL)
    {
      im=WRATHNew WRATHImage("backupimage",
                             ivec2(2,2),
                             fmt);
      int num_bytes;
      num_bytes=4*fmt.m_pixel_format.bytes_per_pixel();
      std::vector<uint8_t> pixels(num_bytes, 127);
      im->respecify_sub_image(0, //layer,
                              0, //LOD
                              fmt.m_pixel_format, //pixel format
                              pixels, //pixel data
                              ivec2(0,0), //bottom left corner
                              ivec2(2,2)); //dimensions
                             
    }
  
  /*
    create our image widget and 
    make it a child of the text 
    widget (thus it goes on the 
    same canvas)
  */
  WRATHBrush brush(im);
  brush.flip_image_y(true);
  ImageFamily::Node::set_shader_brush(brush);
  m_image=WRATHNew ImageFamily::RectWidget(m_text, brush);
  m_image->set_from_brush(brush);
  m_image->set(WRATHTextureCoordinate::mirror_repeat,
               WRATHTextureCoordinate::mirror_repeat);
  WRATHDefaultRectAttributePacker::Rect::handle rect;
  rect=WRATHNew WRATHDefaultRectAttributePacker::Rect(vec2(im->size()));
  m_image->properties()->set_parameters(rect);

  /*
    set clipping and positional properties of 
    image widget
  */
  m_image->clip_rect(WRATHBBox<2>(vec2(0,0), vec2(width(), height())));
  m_image->z_order(1);


  /*
    make a gradient:
    A Gradient holds the "color" stops.
    Internally, a WRATHGradient is a portion of a texture
    WRATHGradient objects are resource managed if passed
    a name in their ctor.
   */
  m_gradient=WRATHNew WRATHGradient("my_gradient_is_resource_managed",
                                    WRATHGradient::MirrorRepeat);

  //add/set color stops of m_gradient.
  m_gradient->set_color(0.0f, vec4(0.0f, 0.0f, 1.0f, 1.0f));
  m_gradient->set_color(0.25f, vec4(0.0f, 1.0f, 0.0f, 1.0f));
  m_gradient->set_color(0.75f, vec4(1.0f, 0.0f, 0.0f, 1.0f));
  m_gradient->set_color(1.0f, vec4(1.0f, 1.0f, 1.0f, 1.0f));
  

   

  /*
    Create a shape.
   */
  WRATHShapeF pshape;
  
  pshape.current_outline() << WRATHOutline<float>::position_type(10.0f, 10.0f)
                           << WRATHOutline<float>::control_point(300.0f, 500.0f)
                           << WRATHOutline<float>::position_type(0.0f, 1000.0f)
                           << WRATHOutline<float>::position_type(1000.0f, 1000.0f)
                           << WRATHOutline<float>::position_type(1000.0f, 0.0f);
  

  typedef WRATHShapeItemTypes::ShapeDrawerF ShapeDrawer;

  /*
    Specify how to draw our shape, in this case we
    specify that the shape is to be filled with
    the gradient m_gradient using the Node type
    from WRATHLayerTranslateLinearGradientWidget
    to specify how to compute the gradient interpolate.
   */
  WRATHBrush gradient_brush, color_brush;

  gradient_brush.m_gradient=m_gradient;
  gradient_brush.gradient_source(LinearGradient::Node::gradient_source());
  color_brush.color_value_source(Color::Node::color_source());



  ShapeDrawer shape_drawer(WRATHShapeItemTypes::fill_shape,
                           gradient_brush);


  m_shape=WRATHNew LinearGradient::ShapeWidget(m_text,
                                               WRATHShapeItemTypes::shape_value(pshape),
                                               shape_drawer);
  m_shape->node()->set_gradient(vec2(0.0f, 0.0f), 
                                vec2(100.0f, 100.0f));
  m_shape->z_order(2);
  m_shape->node()->scaling_factor(0.25f);
  


  

  ShapeDrawer solid_color_stroked(WRATHShapeItemTypes::stroke_shape, 
                                  color_brush);
  m_stroke_shape=
    WRATHNew Color::ShapeWidget(m_text,
                                     WRATHShapeItemTypes::shape_value(pshape),
                                     solid_color_stroked);
  m_stroke_shape->z_order(3);
  m_stroke_shape->node()->color(vec4(1.0f, 1.0f, 1.0f, 1.0f));
  m_stroke_shape->node()->scaling_factor(0.5f);

  
  ShapeDrawer stroked_with_gradient(WRATHShapeItemTypes::stroke_shape,
                                    gradient_brush);
  m_stroke_with_gradient_shape=
    WRATHNew LinearGradient::ShapeWidget(m_text,
                                         WRATHShapeItemTypes::shape_value(pshape),
                                         stroked_with_gradient);
  m_stroke_with_gradient_shape->z_order(4);
  m_stroke_with_gradient_shape->node()->scaling_factor(0.75f);
  m_stroke_with_gradient_shape->node()->set_gradient(vec2(0.0f, 0.0f), 
                                                     vec2(100.0f, 100.0f));


  
  ShapeDrawer solid_color_fill(WRATHShapeItemTypes::fill_shape, 
                               color_brush);
  m_solid_shape=WRATHNew Color::ShapeWidget(m_text,
                                            WRATHShapeItemTypes::shape_value(pshape),
                                            solid_color_fill);
  m_solid_shape->z_order(5);
  m_solid_shape->color(vec4(1.0f, 1.0f, 0.0f, 1.0f));
  m_solid_shape->scaling_factor(0.5f);
  

  //set text of m_text in seperate function, we do this
  //after making the image because we set the text 
  //depending on the position, etc of the image widget.
  set_text(); 
}


void
wrathlayer_example::
set_text(void)
{
  m_text_dirty=false;
  /*
    Now we can set the text of the item. Setting the text
    should be done via a WRATHTextDataStream. A
    WRATHTextDataStream can be used in the same way
    as std::ostream, in addition it also supports a
    variety of state set via manipulators defined in
    namespace WRATHText 
   */
  WRATHTextDataStream ostr;

  ostr.stream() << WRATHText::set_font(m_font)
                << WRATHText::set_color( WRATHText::color_type(0xFF, 0xFF, 0x44, 0xFF),
                                         WRATHText::top_corner_bits)
                << WRATHText::set_color( WRATHText::color_type(0x00, 0x00, 0xFF, 0xFF),
                                         WRATHText::bottom_corner_bits)
                << "\nPosition: " << m_image->position()
                << "\nScaling: " << m_image->scaling_factor()
                << "\nLinearGradient: " << m_shape->node()->start_gradient()
                << ": " << m_shape->node()->end_gradient()
                << WRATHText::set_color( WRATHText::color_type(0xFF, 0x44, 0x00, 0xFF),
                                         WRATHText::top_corner_bits)
                << WRATHText::set_color( WRATHText::color_type(0x44, 0xFF, 0x00, 0xFF),
                                         WRATHText::bottom_corner_bits)
                << "\narrow keys:move item"
                << "\nz/x:zoom in and out"
                << "\nC:toggle clipping"
                << "\nt,g,f,h:move clipping window"
                << "\ny/r:scale up/down clipping window"
                << "\na,w,s,d:move pt0 of gradient"
                << "\np:reset clipping window size";
  
  if(m_image->clipping_active())
    {
      ostr.stream() << "\nclipping: " << m_image->clip_rect();
    }
  else
    {
      ostr.stream() << "\nclipping: off";
    }
       
  for(int i=0;i<100;++i)
    {
      GLubyte R[2]={0x77,0xFF};;
      GLubyte G[3]={0xFF, 0x44, 0x77};
      GLubyte B[5]={0x22, 0x55, 0x88, 0xff};

      ostr.stream() 
                << WRATHText::set_color(WRATHText::color_type(R[i%2],
                                                              G[i%3],
                                                              B[i%5],
                                                              0xFF),
                                        WRATHText::top_corner_bits)
                << WRATHText::set_color( WRATHText::color_type(R[(1+i)%2],
                                                               G[(1+i)%3],
                                                               B[(1+i)%5],
                                                               0xFF),
                                         WRATHText::bottom_corner_bits)
                << "\nRepeated Text, lots of repeated text, going on and on and on and on..";
    }

  /*
    Set the text of m_text to the contents of ostr:
   */
  m_text->properties()->clear();
  m_text->properties()->add_text(ostr);
      
  //get to call paint().
  update_widget();

}

wrathlayer_example::
~wrathlayer_example()
{
  if(m_contents==NULL)
    {
      return;
    }

  /*
    clean up:
      the parent of m_image is m_text,
      and m_text is on the canvas
      m_contents, so all get deleted
      by deleting m_contents.
      It is safe though to delete
      m_text before m_contents
      and it is safe to delete
      m_image before m_text.
   */
  //WRATHPhasedDelete(m_image);
  //WRATHPhasedDelete(m_text);
  WRATHPhasedDelete(m_contents);

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
wrathlayer_example::
paint(void)
{
  glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

   /*
    resize happend, a resize of the window triggers that we need
    to call glViewport on the GL context and
    we also need to update the projection matrix
    of m_contents:
   */
  if(m_resized)
    {
      glViewport(0, 0, width(), height());
      float_orthogonal_projection_params proj_params(0, width(),
                                                     height(), 0);
      m_contents->simulation_matrix(WRATHLayer::projection_matrix,
                                    float4x4(proj_params));
      m_resized=false;
    }

  if(m_text_dirty)
    {
      set_text();
    }

  /*
    in this example simulation and drawing are
    done in paint, a more advanced application
    would spawn another thread which would
    at regular intervals update the simulation
    data (in this case the contents of m_text,
    the transformation value of m_text_transformation)
    and call m_tr->signal_complete_simulation_frame()
    to indicate that a new frame of simulation
    data is ready.
   */
   m_tr->signal_complete_simulation_frame();

   /*
     Now we draw, before drawing we need to coordinate
     by calling m_tr->signal_begin_presentation_frame();
    */
   m_tr->signal_begin_presentation_frame();
   
   /*
     now finally draw, we need to restore the GL state
     as well since WRATH does not make any guarantees
     on what the GL state is after drawing. 
   */
   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   /*
     try out that pre-matrix stuff:
    */
   float4x4 M;
   M.translate_matrix(m_pre_translate);
   m_contents->clear_and_draw(&M);
   

}

  
void
wrathlayer_example::
handle_event(FURYEvent::handle ev)
{
  switch(ev->type())
    {
    case FURYEvent::KeyDown:
      {
        FURYKeyEvent::handle qe(ev.static_cast_handle<FURYKeyEvent>());
        ev->accept();
        switch(qe->key().m_value)
          {
          case FURYKey_Escape:
            /*
              end_demo is a method inherited from DemoKernel
              to end the program and close the window
             */
            end_demo();
            break;

          case FURYKey_K:
            m_pre_translate.x()+=10.0f;
            break;

          case FURYKey_L:
            m_pre_translate.x()-=10.0f;
            break;

          case FURYKey_Left:
            m_text_dirty=true;
            m_image->position(m_image->position()+vec2(-10.0f, 0.0f));
            ev->accept();
            break;

          case FURYKey_Right:
            m_text_dirty=true;
            m_image->position(m_image->position()+vec2( 10.0f, 0.0f));
            ev->accept();
            break;

          case FURYKey_Up:
            m_text_dirty=true;
            m_image->position(m_image->position()+vec2(0.0f, -10.0f));
            ev->accept();
            break;

          case FURYKey_Down:
            m_text_dirty=true;
            m_image->position(m_image->position()+vec2(0.0f,  10.0f));
            ev->accept();
            break;

          case FURYKey_Z:
            m_text_dirty=true;
            m_image->scaling_factor(m_image->scaling_factor()*1.1f);
            ev->accept();
            break;

          case FURYKey_X:
            m_text_dirty=true;
            m_image->scaling_factor(m_image->scaling_factor()/1.1f);
            ev->accept();
            break;

          case FURYKey_C:
            m_text_dirty=true;
            m_image->clipping_active(!m_image->clipping_active());
            ev->accept();
            break;

          case FURYKey_T:
            {
              WRATHBBox<2> B(m_image->clip_rect());
              m_text_dirty=true;
              B.translate(vec2(0.0f, -10.0f));
              m_image->clip_rect(B);
              ev->accept();
            }
            break;

          case FURYKey_G:
            {
              WRATHBBox<2> B(m_image->clip_rect());
              m_text_dirty=true;
              B.translate(vec2(0.0f, 10.0f));
              m_image->clip_rect(B);
              ev->accept();
            }
            break;

          case FURYKey_F:
            {
              WRATHBBox<2> B(m_image->clip_rect());
              m_text_dirty=true;
              B.translate(vec2(-10.0f, 0.0f));
              m_image->clip_rect(B);
              ev->accept();
            }
            break;

          case FURYKey_H:
            {
              WRATHBBox<2> B(m_image->clip_rect());
              m_text_dirty=true;
              B.translate(vec2(10.0f, 0.0f));
              m_image->clip_rect(B);
              ev->accept();
            }
            break;

          case FURYKey_Y:
            {
              WRATHBBox<2> B(m_image->clip_rect());
              m_text_dirty=true;
              B.scale(1.1f);
              m_image->clip_rect(B);
              ev->accept();
            }
            break;

          case FURYKey_R:
            {
              WRATHBBox<2> B(m_image->clip_rect());
              m_text_dirty=true;
              B.scale(1.0f/1.1f);
              m_image->clip_rect(B);
              ev->accept();
            }
            break;

          case FURYKey_P:
            {
              WRATHBBox<2> B(vec2(0,0), vec2(800,800));
              m_text_dirty=true;
              m_image->clip_rect(B);
              ev->accept();
            }
            break;

          case FURYKey_W:
            {
              vec2 p(m_shape->node()->start_gradient());
              p.y()-=10.0f;
              m_shape->node()->start_gradient(p);
              m_text_dirty=true;
            }
            break;

          case FURYKey_S:
            {
              vec2 p(m_shape->node()->start_gradient());
              p.y()+=10.0f;
              m_shape->node()->start_gradient(p);
              m_text_dirty=true;
            }
            break;

          case FURYKey_A:
            {
              vec2 p(m_shape->node()->start_gradient());
              p.x()-=10.0f;
              m_shape->node()->start_gradient(p);
              m_text_dirty=true;
            }
            break;

          case FURYKey_D:
            {
              vec2 p(m_shape->node()->start_gradient());
              p.x()+=10.0f;
              m_shape->node()->start_gradient(p);
              m_text_dirty=true;
            }
            break;

            

          }
      }
      break;

    case FURYEvent::Resize:
      {
        m_resized=true;
        ev->accept();
      }

    default:
      break;
    }


  /*
    redraw the screen since it's contents may 
    have changed from handling the event
  */
  update_widget();

}


//////////////////////////////////////////
// support methods...

/*
  More boilerplate code, a DemoKernelMaker
  needs to implement a method to create
  a DemoKernel and a method to delete it.
 */
class cmd_line_type:public DemoKernelMaker
{
public:
  virtual
  DemoKernel*
  make_demo(void)
  {
    return WRATHNew wrathlayer_example(this);
  }
  
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


int
main(int argc, char **argv)
{
  cmd_line_type cmd_line;
  return cmd_line.main(argc, argv);
}





