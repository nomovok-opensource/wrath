/*! 
 * \file hello_widget_generator2.cpp
 * \brief file hello_widget_generator2.cpp
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
#include "WRATHFontFetch.hpp"
#include "wrath_demo.hpp"
#include "wrath_demo_image_support.hpp"
#include "WRATHFontShaderSpecifier.hpp"
#include "WRATHLayerNodeValuePackerTexture.hpp"
#include "WRATHLayerNodeValuePackerHybrid.hpp"
#include "WRATHLayerNodeValuePackerUniformArrays.hpp"
#include "WRATHLayerItemWidgetsTranslate.hpp"


#include "WRATHWidgetGenerator.hpp"

typedef WRATHLayerNodeValuePackerTextureT<WRATHLayerNodeValuePackerTexture::fp32_texture,
					  WRATHLayerNodeValuePackerTexture::two_channel_texture> two_channel_fp32; 

typedef WRATHLayerNodeValuePackerUniformArrays NodePacker;
//typedef WRATHLayerNodeValuePackerTextureT<WRATHLayerNodeValuePackerTexture::fp32_texture,
//					  WRATHLayerNodeValuePackerTexture::two_channel_texture> NodePacker;

//typedef WRATHLayerNodeValuePackerTextureFP32 NodePacker;
//typedef WRATHLayerNodeValuePackerHybrid<WRATHLayerNodeValuePackerUniformArrays, WRATHLayerNodeValuePackerTextureFP16> NodePacker;
//typedef WRATHLayerNodeValuePackerHybrid<WRATHLayerNodeValuePackerUniformArrays, two_channel_fp32> NodePacker;

/*
   WRATHLayerItemNodeTranslate --> node type
   NodePacker --> packer of per node values
   WRATHLayer --> canvas type
 */
typedef WRATHLayerItemWidget<WRATHLayerItemNodeTranslate, NodePacker, WRATHLayer>::Generator WidgetGenerator;


WRATHImage*
safe_load_image(const std::string &pname)
{
  WRATHImage *R;
  WRATHImage::ImageFormat fmt;

  fmt
    .internal_format(GL_RGBA)
    .pixel_data_format(GL_RGBA)
    .pixel_type(GL_UNSIGNED_BYTE)
    .magnification_filter(GL_LINEAR)
    .minification_filter(GL_NEAREST)
    .automatic_mipmap_generation(false);

  R=WRATHDemo::fetch_image(pname, fmt);

  if(R==NULL)
    {
      /*
        unable to load that image, darn.. just make a new one...
       */
      R=WRATHNew WRATHImage(std::string("failed to load\"") + pname + "\"",
                            ivec2(2,2), fmt);
      int num_pixels(R->size().x()*R->size().y());
      int num_bytes(num_pixels*R->image_format(0).m_pixel_format.bytes_per_pixel());
      std::vector<uint8_t> pixels(num_bytes, 255);

      R->respecify_sub_image(0, //layer,
                             0, //LOD
                             R->image_format(0).m_pixel_format, //pixel format
                             pixels, //pixel data
                             ivec2(0,0), //bottom left corner
                             R->size());
    }

  return R;
}


/*!\details
 This demo gives an example of using \ref WRATHWidgetGeneratorT
 to create text so that the text has a brush applied to it.
*/



class cmd_line_type:public DemoKernelMaker
{
public:

  command_line_argument_value<bool> m_use_aa;

  cmd_line_type(void):
    m_use_aa(true, "use_aa", "Use AntiAliasing on stroking of boundary of shape", *this)
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



class widget_generator_example:public DemoKernel
{
public:
  widget_generator_example(cmd_line_type *parent);
  ~widget_generator_example();

  virtual
  void
  paint(void);

  virtual
  void
  handle_event(FURYEvent::handle);

private:

  /*
    a silly class to make changing position and scale easier...
   */
  class ConvenianceScaleTranslate
  {
  public:
    ConvenianceScaleTranslate(void):
      m_position(0.0f, 0.0f),
      m_scale(1.0f)
    {}

    WRATHScaleTranslate
    as_scale_translate(void) const
    {
      return WRATHScaleTranslate(m_position, m_scale);
    }

    vec2 m_position;
    float m_scale;
  };

  void
  paint(WidgetGenerator *painter);

  void
  ready_brush(void);

  /*
    The font type we will use. Font's are viewed as
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
  typedef WRATHMixFontTypes<WRATHTextureFontFreeType_Distance>::mix FontType;
  
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
    a flag to indicate that resize of window
    happend.
   */
  bool m_resized;
  
  /*
    the root widget, which is the parent
    to the following widgets.
   */
  WidgetGenerator::NodeHandle::AutoDelete m_root_widget;
  

  /*
    node widget to demo transformation
    heirarchy of widget generator
   */
  WidgetGenerator::NodeHandle::AutoDelete m_empty_widget;
  ConvenianceScaleTranslate m_empty_position;
  
  WRATHBrush m_brush;
  WRATHWidgetGenerator::LinearGradientProperties m_gradient_position_values;
  /*
    The text widget with
    - const color modulated
    - linear gradient modulated
    - image modulated
   */
  typedef WidgetGenerator::ColorLinearGradientRepeatXRepeatYImageFamily Family;

  Family::DrawnText::AutoDelete m_text_widget;
  ConvenianceScaleTranslate m_text_position;
  WRATHTextDataStream m_text;
  bool m_text_dirty;

  enum WRATHWidgetGenerator::shape_opacity_t m_use_aa;
};

widget_generator_example::
widget_generator_example(cmd_line_type *parent):
  DemoKernel(parent),
  m_contents(NULL),
  m_resized(false),
  m_gradient_position_values(vec2(0.0f, 0.0f), vec2(100.0f, 100.0f)),
  m_text_dirty(true),
  m_use_aa(parent->m_use_aa.m_value?
           WRATHWidgetGenerator::shape_opaque:
           WRATHWidgetGenerator::shape_opaque_non_aa)
{

  /*
    enable key repeat..
   */
  enable_key_repeat(true);

  /*
    specify to WRATHFontFetch with what font type
    to use for creating fonts. Usually only called
    once for the lifetime of a program.
  */
  WRATHFontFetch::font_fetcher(type_tag<FontType>());
    
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
  
  ready_brush();
  update_widget();
}



widget_generator_example::
~widget_generator_example()
{
  if(m_contents==NULL)
    {
      return;
    }

  /*
    clean up:

    The underlying Widget object (via the method widget())
    of m_root is owned by m_contents hence will be delete
    when m_contents is. The other underlying widget objects
    have as an ancestor the widget of m_root, hence they 
    will be delete when the widget of m_root is.

    The SmartWidget types catch a signal when their
    widget object is deleted, hence for example deleting
    m_root.widget() will have that m_root.widget() is
    NULL on return of WRATHPhasedDelete().
   */
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
widget_generator_example::
ready_brush(void)
{
  if(m_brush.m_image==NULL)
    {
      m_brush.m_image=safe_load_image("images/1024x1024.png");;
    }

  
  if(m_brush.m_gradient==NULL)
    {
      /*
        create the WRATHGradient object which specifies
        the actual colors of the gradient filling the 
        shape. Internally, a WRATHGradient is a portion 
        of a texture WRATHGradient objects are resource 
        managed if passed a name in their ctor.
      */
      m_brush.m_gradient=WRATHNew WRATHGradient("my_gradient_is_resource_managed",
                                                WRATHGradient::MirrorRepeat);
      //add/set color stops of m_gradient.
      m_brush.m_gradient->set_color(0.0f, vec4(1.0f, 1.0f, 1.0f, 1.0f));
      m_brush.m_gradient->set_color(0.25f, vec4(1.0f, 1.0f, 0.0f, 1.0f));
      m_brush.m_gradient->set_color(0.75f, vec4(1.0f, 0.0f, 0.0f, 1.0f));
      m_brush.m_gradient->set_color(1.0f, vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }  

  /*
    this is an API-ickiness, we need to make the
    shaders of the brush ready.
   */
  Family::Node::set_shader_brush(m_brush);
}

void
widget_generator_example::
paint(WidgetGenerator *painter)
{
  if(m_text_dirty)
    {
      m_text_dirty=false;
      m_text.clear();
      m_text.stream() << WRATHText::set_font(WRATHFontFetch::FontProperties()
                                             .family_name("DejaVuSerif") //select Serif font
                                             .bold(true) //bold
                                             .italic(false)) //no slanting
                      << WRATHText::set_color( WRATHText::color_type(0xFF, 0xFF, 0xFF, 0xFF));

      if(m_use_aa)
        {
          m_text.stream() << WRATHText::set_font_brush_letter_aa(m_brush);
        }
      else
        {
          m_text.stream() << WRATHText::set_font_brush_letter_non_aa(m_brush);
        }
      m_text.stream() << "Brush From Letter ZAZAZAZAZAZAZAZAZAZAZ\n\n\n\n\n";

      if(m_use_aa)
        {
          m_text.stream() << WRATHText::set_font_brush_item_aa(m_brush);
        }
      else
        {
          m_text.stream() << WRATHText::set_font_brush_item_non_aa(m_brush);
        }
      m_text.stream() << "Brush From Item ZAZAZAZAZAZAZAZAZAZAZ\n"
                      << "Brush From Item ZAZAZAZAZAZAZAZAZAZAZ\n"
                      << "Brush From Item ZAZAZAZAZAZAZAZAZAZAZ\n"
                      << "Brush From Item ZAZAZAZAZAZAZAZAZAZAZ\n";
      
      for(int i=0;i<100;++i)
        {
          m_text.stream() << "\nRepeated Text, lots of repeated text, going on and on and on and on..";
        }
    }

  painter->push_node(m_empty_widget);
  m_empty_widget.widget()->transformation(m_empty_position.as_scale_translate());

  /*
    A WRATHWidgetGenerator's purpose is to give
    a procedurally oriented drawer interface
    for creating and modifying widgets.

    When one calls an add_ method, the widget is
    recontructed and it's z-value is set.

    In contrast, update_generic() only sets the
    z-value for the widget.
  */

  if(!m_text_widget.widget() or m_text_dirty)
    {
      painter->add_text(m_text_widget, 
                        WRATHWidgetGenerator::Text(m_text), 
                        WRATHWidgetGenerator::text_opaque);

      //awkward API issue: the node type needs to know about the
      //brush to work some of it's magicks: specifially, it needs
      //to take the image and gradient values of m_brush to set 
      //some texture coordinate values internal to the node type.
      m_text_widget.widget()->node()->set_from_brush(m_brush);
    }
  else
    {
      painter->update_generic(m_text_widget);
    }
  m_gradient_position_values(m_text_widget.widget());
  m_text_widget.widget()->transformation(m_text_position.as_scale_translate());
}



void
widget_generator_example::
paint(void)
{
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


  /*
    The WRATHWidgetGenerator interface provides a
    procedural like interface to create and update
    widgets.
   */
  int z(0);
  WidgetGenerator painter(m_contents, m_root_widget, z);
  paint(&painter);
 

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

   WRATHLayer::draw_information draw_counts;

   m_contents->clear_and_draw(&draw_counts);
   /*
   std::cout << draw_counts.m_draw_count 
             << " draw calls "
             << draw_counts.m_layer_count
             << " layers\n";
   */
}

  
void
widget_generator_example::
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

          case FURYKey_Space:
            m_text_dirty=true;  
            if(m_root_widget.widget()!=NULL)
              {
                WRATHPhasedDelete(m_root_widget.widget());
              }
            break;

          case FURYKey_F:
            m_text_dirty=true;
            m_empty_position.m_position.x()-=10.0f;
            ev->accept();
            break;

          case FURYKey_H:
            m_text_dirty=true;
            m_empty_position.m_position.x()+=10.0f;
            ev->accept();
            break;

          case FURYKey_T:
            m_text_dirty=true;
            m_empty_position.m_position.y()-=10.0f;
            ev->accept();
            break;

          case FURYKey_G:
            m_text_dirty=true;
            m_empty_position.m_position.y()+=10.0f;
            ev->accept();
            break;

          case FURYKey_Y:
            m_text_dirty=true;
            m_empty_position.m_scale*=1.1f;
            ev->accept();
            break;

          case FURYKey_R:
            m_text_dirty=true;
            m_empty_position.m_scale/=1.1f;
            ev->accept();
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



  
DemoKernel*
cmd_line_type::
make_demo(void)
{
  return WRATHNew widget_generator_example(this);
}

int
main(int argc, char **argv)
{
  cmd_line_type cmd_line;
  return cmd_line.main(argc, argv);
}





