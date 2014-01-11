/*! 
 * \file hello_widget_generator.cpp
 * \brief file hello_widget_generator.cpp
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
//                                        WRATHLayerNodeValuePackerTexture::two_channel_texture> NodePacker;

//typedef WRATHLayerNodeValuePackerTextureFP32 NodePacker;
//typedef WRATHLayerNodeValuePackerHybrid<WRATHLayerNodeValuePackerUniformArrays, WRATHLayerNodeValuePackerTextureFP16> NodePacker;
//typedef WRATHLayerNodeValuePackerHybrid<WRATHLayerNodeValuePackerUniformArrays, two_channel_fp32> NodePacker;

//typedef WRATHLayerItemNodeTranslate Node;


typedef WRATHLayerItemNodeTranslateT<WRATHLayerItemNodeDepthType::hierarchical_ordering> Node;

/*
   Node --> node type
   NodePacker --> packer of per node values
   WRATHLayer --> canvas type
 */
typedef WRATHLayerItemWidget<Node, 
                             NodePacker, 
                             WRATHLayer>::Generator WidgetGenerator;

class print_z_values
{
public:
  unsigned int
  compute_generation_count(Node *pn) const
  {
    unsigned int R(0);

    for(;pn!=NULL; pn=pn->parent(), ++R)
      {}

    return R;
  }

  void
  operator()(Node *pn) const
  {
    std::cout << WRATHUtil::format_tabbing(compute_generation_count(pn))
      //<< typeid(*pn).name()
              << "local(type=" << typeid(pn->z_order()).name() << ")=" << pn->z_order()
              << ", global(type=" << typeid(pn->global_z_order()).name() << ")=" << pn->global_z_order()
              << ", dz=" << Node::normalizer_type::signed_normalize(pn->global_z_order())
              << "\n";
  }
};



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
  This demo gives an example of using the \ref WRATHWidgetGeneratorT
  class to generate widgets in a more procedural fashion.
  This example shows making of widgets: rectangles, shapes 
  and text widgets together with applying a brush against
  shape and rect widgets.
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
  create_images_and_gradients_as_needed(void);

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

  /*
    The text widget
   */
  WidgetGenerator::PlainFamily::DrawnText::AutoDelete m_text_widget;
  ConvenianceScaleTranslate m_text_position;
  WRATHTextDataStream m_text;
  bool m_text_dirty;

  /*
    The image widget
   */
  WidgetGenerator::LinearGradientRepeatXRepeatYImageFamily::DrawnRect::AutoDelete m_image_widget;
  //WidgetGenerator::DrawnImage::AutoDelete m_image_widget;
  ConvenianceScaleTranslate m_image_position;
  WRATHImage *m_src_image;
  WRATHGradient *m_gradient_on_image;
  WRATHWidgetGenerator::LinearGradientProperties m_image_gradient_position_values;

  /*
    the rect widget which is drawn on a seperate canvas clipped.
   */
  WidgetGenerator::DrawnCanvas::AutoDelete m_rect_clipped_canvas;
  ConvenianceScaleTranslate m_clipper_mover_position;
  WidgetGenerator::NodeHandle::AutoDelete m_clipper_mover;
  WidgetGenerator::PlainFamily::DrawnShape::AutoDelete m_shape_clipper;
  WidgetGenerator::PlainFamily::DrawnText::AutoDelete m_text_clipper;
  WRATHTextDataStream m_text_for_clipping;
  WidgetGenerator::PlainFamily::DrawnRect::AutoDelete m_rect_clipper;
  ConvenianceScaleTranslate m_rect_clipper_position;
  bool m_rect_clipper_visible;

  WidgetGenerator::RadialRepeatGradientFamily::DrawnRect::AutoDelete m_rect_widget;
  ConvenianceScaleTranslate m_rect_clipped_canvas_position;
  WRATHWidgetGenerator::RadialGradientProperties m_rect_gradient_position_values;
  /*
    the parent shape widget,
    holding the shape stroked and filled.
   */
  WidgetGenerator::NodeHandle::AutoDelete m_parent_shape_widget;
  ConvenianceScaleTranslate m_shape_position; 
  vecN<WRATHShapeF, 2> m_shapes;
  

  /*
    The fill-shape widget
   */
  //WidgetGenerator::DrawnShapeLinearGradient::AutoDelete m_shape_widget;
  WidgetGenerator::LinearGradientImageFamily::DrawnShape::AutoDelete m_shape_widget;
  WRATHWidgetGenerator::LinearGradientProperties m_gradient_position_values;
  WRATHGradient *m_gradient;
  WRATHImage *m_small_image;
  /*
    The stroke-shape widget
   */
  WidgetGenerator::CColorFamily::DrawnShape::AutoDelete m_shape_outline;
  WRATHWidgetGenerator::ColorProperties m_shape_outline_color;

  /*
   */
  enum WRATHWidgetGenerator::shape_opacity_t m_use_aa;

  int m_shape_should_use, m_shape_currently_in_use;
  bool m_first_run;
};

widget_generator_example::
widget_generator_example(cmd_line_type *parent):
  DemoKernel(parent),

  m_contents(NULL),
  m_resized(false),


  m_text_dirty(true),

  m_src_image(NULL),
  m_gradient_on_image(NULL),
  m_image_gradient_position_values(vec2(0.0f, 0.0f), vec2(100.0f, 100.0f)),

  
  m_rect_clipper_visible(false),
  m_rect_gradient_position_values(vec2(0.0f, 0.0f), 0.0f, vec2(0.0f, 0.0f), 70.0f),
  //m_rect_gradient_position_values(vec2(0.0f, 0.0f), vec2(-30.0f, 50.0f)),

  /*
    initialize the position values of the linear 
    gradiint to be applied to m_shape_widget
   */
  m_gradient_position_values(vec2(0.0f, 0.0f), vec2(100.0f, 100.0f)),
  m_gradient(NULL),
  m_small_image(NULL),
  m_use_aa(parent->m_use_aa.m_value?
           WRATHWidgetGenerator::shape_opaque:
           WRATHWidgetGenerator::shape_opaque_non_aa),

  m_shape_should_use(0),
  m_shape_currently_in_use(0),
  m_first_run(true)
           
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

  /*
    set our WRATHShape geometry
   */
  m_shapes[0].current_outline() << WRATHOutline<float>::position_type(0.0f, 0.0f)
                                << WRATHOutline<float>::control_point(150.0f, 250.0f)
                                << WRATHOutline<float>::position_type(0.0f, 500.0f)
                                << WRATHOutline<float>::position_type(500.0f, 500.0f)
                                << WRATHOutline<float>::position_type(500.0f, 0.0f);
  
  m_shapes[1].current_outline() << WRATHOutline<float>::position_type(0.0f, 0.0f)
                                << WRATHOutline<float>::control_point(-150.0f, 250.0f)
                                << WRATHOutline<float>::position_type(0.0f, 500.0f)
                                << WRATHOutline<float>::position_type(500.0f, 500.0f)
                                << WRATHOutline<float>::position_type(500.0f, 0.0f)
                                << WRATHOutline<float>::control_point(250.0f, 100.0f);

  m_shape_position.m_scale=0.5f;
  m_rect_clipper_position.m_position=vec2(200.0f, 200.0f);

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

    It is however safe to explictely delete m_text_item.widget() 
    and/or m_image_item.widget() before m_root.widget() 
    is deleted, which is turn is safe to explictely delete
    before m_contents.
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
create_images_and_gradients_as_needed(void)
{
  if(m_src_image==NULL)
    {
      m_src_image=safe_load_image("images/1024x1024.png");;
    }

  if(m_gradient_on_image==NULL)
    {
      m_gradient_on_image=WRATHNew WRATHGradient("my_second_gradient_is_also_resource_managed",
                                             WRATHGradient::Repeat);
  
      m_gradient_on_image->set_color(0.0f, vec4(1.0f, 0.0f, 0.0f, 1.0f));
      m_gradient_on_image->set_color(0.25f, vec4(1.0f, 1.0f, 1.0f, 1.0f));
      m_gradient_on_image->set_color(0.50f, vec4(0.0f, 1.0f, 0.0f, 1.0f));
      m_gradient_on_image->set_color(0.76f, vec4(0.0f, 0.0f, 1.0f, 1.0f));
    }

  if(m_gradient==NULL)
    {
      /*
        create the WRATHGradient object which specifies
        the actual colors of the gradient filling the 
        shape. Internally, a WRATHGradient is a portion 
        of a texture WRATHGradient objects are resource 
        managed if passed a name in their ctor.
      */
      m_gradient=WRATHNew WRATHGradient("my_gradient_is_resource_managed",
                                        WRATHGradient::MirrorRepeat);
      //add/set color stops of m_gradient.
      m_gradient->set_color(0.0f, vec4(1.0f, 1.0f, 1.0f, 1.0f));
      m_gradient->set_color(0.25f, vec4(1.0f, 1.0f, 0.0f, 1.0f));
      m_gradient->set_color(0.75f, vec4(1.0f, 0.0f, 0.0f, 1.0f));
      m_gradient->set_color(1.0f, vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

  if(m_small_image==NULL)
    {
      m_small_image=safe_load_image("images/512.512.png");
    }
  
}

void
widget_generator_example::
paint(WidgetGenerator *painter)
{
  
  create_images_and_gradients_as_needed();
  
  if(m_text_dirty)
    {
      m_text_for_clipping.clear();
      m_text_for_clipping.stream() << WRATHText::set_font(WRATHFontFetch::FontProperties()
                                                          .family_name("DejaVuSerif") //select Serif font
                                                          .bold(true) //bold
                                                          .italic(false)) //no slanting
                                   << WRATHText::set_pixel_size(80)
                                   << WRATHText::set_color(vec4(0.3f, 0.6f, 0.6f, 1.0f))
                                   << "\nSome funky\nfunky clipping\nto letters";


      m_text.clear();
      m_text.stream() << WRATHText::set_font(WRATHFontFetch::FontProperties()
                                             .family_name("DejaVuSerif") //select Serif font
                                             .bold(true) //bold
                                             .italic(false)) //no slanting
                      << WRATHText::set_color( WRATHText::color_type(0xFF, 0xFF, 0x44, 0xFF),
                                               WRATHText::top_corner_bits)
                      << WRATHText::set_color( WRATHText::color_type(0x00, 0x00, 0xFF, 0xFF),
                                               WRATHText::bottom_corner_bits)
                      << "\nParent: " << m_empty_position.m_position << "@" << m_empty_position.m_scale
                      << WRATHText::set_font(WRATHFontFetch::FontProperties()
                                             .family_name("DejaVuSans") //select Sans font
                                             .bold(false) //not bold
                                             .italic(false)) //no slanting
                      << "\nImage: " << m_image_position.m_position << "@" << m_image_position.m_scale
                      << "\nShape: " << m_shape_position.m_position << "@" << m_shape_position.m_scale
                      << WRATHText::set_font(WRATHFontFetch::FontProperties()
                                             .family_name("DejaVuSans") //select Sans font
                                             .bold(false) //not bold
                                             .italic(true)) //slanted
                      << "\narrow keys, z/x: move image, zoom out/in"
                      << "\nw,a,s,d, q/e: move shape, zoom out/in"
                      << "\nt,f,g,h, r,y: move parental widget, zoom out/in"
                      << "\ni,k,j,l, u,o: move clipped blue-green rect, zoom out/in"
                      << "\n8,5,4,6, 7,9: move clip out text, zoom out/in"
                      << "\nv,b,n,m, 1,3, 2:move clip out rect, zoom out/in, toggle active"
                      << "\nSpace: reconstruct all";
      
      for(int i=0;i<100;++i)
        {
          GLubyte R[2]={0x77,0xFF};;
          GLubyte G[3]={0xFF, 0x44, 0x77};
          GLubyte B[5]={0x22, 0x55, 0x88, 0xff};
          
          m_text.stream() 
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

  if(!m_image_widget.widget()) 
    {
      m_text_dirty=false;
      painter->add_rect(m_image_widget, 
                        WRATHWidgetGenerator::Rect(1800.0f, 1800.0f),
                        WRATHWidgetGenerator::Brush(m_src_image, m_gradient_on_image)
                        .flip_image_y(true));
      m_image_gradient_position_values(m_image_widget.widget());
      m_image_widget.widget()->full_image();
      //m_image_widget.widget()->sub_image(ivec2(300,300), ivec2(100,100));
      //m_image_widget.widget()->set(WRATHTextureCoordinate::repeat, WRATHTextureCoordinate::mirror_repeat);
    }
  else
    {
      painter->update_generic(m_image_widget);
    }
  m_image_widget.widget()->transformation(m_image_position.as_scale_translate());


  /*
    pushing a canvas node allows one to specify
    clipping via widgets: rect, text, shapes, etc.
    
    the clip_ methods from the return value
    of push_canvas_node() are the same in spirit
    as add_* calls. In particular, you should not
    call clip_ unless you wish to either change
    the item that performs the clipping and/or
    create the item. 
   */
  if(!m_rect_clipped_canvas.widget())
    {
      painter->push_canvas_node(m_rect_clipped_canvas)
        .clip_rect(WRATHWidgetGenerator::clip_outside,
                   m_rect_clipper,
                   vec2(100.0f, 100.0f))
        
        .clip_filled_shape(WRATHWidgetGenerator::clip_inside,
                           m_shape_clipper, 
                           WRATHWidgetGenerator::shape_value(m_shapes[0]))
        .push_node(m_clipper_mover)
        .clip_text(WRATHWidgetGenerator::clip_outside,
                   m_text_clipper,
                   WRATHWidgetGenerator::Text(m_text_for_clipping))
        .pop_node();
    }
  else
    {
      painter->push_canvas_node(m_rect_clipped_canvas);
    }

  m_rect_clipped_canvas.widget()->transformation(m_rect_clipped_canvas_position.as_scale_translate());
  m_clipper_mover.widget()->transformation(m_clipper_mover_position.as_scale_translate());
  m_rect_clipper.widget()->transformation(m_rect_clipper_position.as_scale_translate());
  m_rect_clipper.widget()->visible(m_rect_clipper_visible);

  if(!m_rect_widget.widget())
    {
      painter->add_rect(m_rect_widget,
                        WRATHWidgetGenerator::Rect(500.0f, 500.0f), 
                        WRATHWidgetGenerator::Brush(m_gradient_on_image));
    }
  else
    {
      painter->update_generic(m_rect_widget);
    }
  m_rect_widget.widget()->node()->set_window(vec2(-50.0f, -50.0f), vec2(50.0f, 50.0f));
  m_rect_gradient_position_values(m_rect_widget.widget());
  
  painter->pop_node();



  painter->push_node(m_parent_shape_widget);
  m_parent_shape_widget.widget()->transformation(m_shape_position.as_scale_translate());
  
  if(!m_shape_widget.widget())
    {
      painter->add_filled_shape(m_shape_widget, //widget
                                m_gradient_position_values, //values for linear gradeint
                                WRATHWidgetGenerator::shape_value(m_shapes[m_shape_currently_in_use]), //shape to draw
                                WRATHWidgetGenerator::Brush(m_gradient, m_small_image)
                                .flip_image_y(true)); //colors of gradient applied to filling
    }
  else
    {
      painter->update_generic(m_shape_widget);
    }
  

  m_shape_outline_color=WRATHWidgetGenerator::ColorProperties(vec4(1.0f, 1.0f, 0.5f, 1.0f));

  if(!m_shape_outline.widget())
    {
      painter->add_stroked_shape(m_shape_outline,
                                 m_shape_outline_color,
                                 WRATHWidgetGenerator::shape_value(m_shapes[m_shape_currently_in_use]),
                                 WRATHWidgetGenerator::StrokingParameters()
                                 .close_outline(true)
                                 .join_style(WRATHWidgetGenerator::round_join),
                                 m_use_aa);
    }
  else
    {
      painter->update_generic(m_shape_outline);
    }
  
  
  painter->pop_node();

  if(!m_text_widget.widget() or m_text_dirty)
    {
      painter->add_text(m_text_widget, 
                        WRATHWidgetGenerator::Text(m_text), 
                        WRATHWidgetGenerator::text_opaque);
    }
  else
    {
      painter->update_generic(m_text_widget);
    }
  m_text_widget.widget()->transformation(m_text_position.as_scale_translate());

  if(!m_first_run and m_shape_should_use!=m_shape_currently_in_use)
    {
      m_shape_currently_in_use=m_shape_should_use;


      m_shape_outline.widget()->properties()
        ->change_shape(WRATHWidgetGenerator::shape_value(m_shapes[m_shape_currently_in_use]),
                       WRATHWidgetGenerator::StrokingParameters()
                       .close_outline(true)
                       .join_style(WRATHWidgetGenerator::round_join));

      m_shape_widget.widget()->properties()
        ->change_shape(WRATHWidgetGenerator::shape_value(m_shapes[m_shape_currently_in_use]));
    }
  m_first_run=false;


  //make m_image_widget in front
  //painter->update_generic(m_image_widget);
  /*
  std::cout << "\n\t#m_number_nodes=" << painter->counters().m_number_nodes
            << "\n\t#m_number_items=" << painter->counters().m_number_items
            << "\n\t#m_number_canvases=" << painter->counters().m_number_canvases
            << "\n\t#m_number_contructed_items=" << painter->counters().m_number_contructed_items
            << "\n\n";
  */
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
    Use WRATHWidgetGenerator to create/update the widgets
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

          case FURYKey_P:
            if(m_root_widget.widget()!=NULL)
              {
                m_root_widget.widget()->call_recurse_base<print_z_values, Node>(print_z_values());
              }
            break;

          case FURYKey_Tab:
            m_shape_should_use=1-m_shape_should_use;
            break;

          case FURYKey_Space:
            m_text_dirty=true;  
            if(m_root_widget.widget()!=NULL)
              {
                WRATHPhasedDelete(m_root_widget.widget());
              }
            /*
              deletion of m_root.widget() triggers
              deletion of all it's child widgets.
             */
            WRATHassert(m_root_widget.widget()==NULL);
            WRATHassert(m_empty_widget.widget()==NULL);
            WRATHassert(m_text_widget.widget()==NULL);
            WRATHassert(m_image_widget.widget()==NULL);
            WRATHassert(m_parent_shape_widget.widget()==NULL);
            WRATHassert(m_shape_widget.widget()==NULL);
            WRATHassert(m_shape_outline.widget()==NULL);    
            
            #if 0 //test recreate resources too.
            {
              m_src_image=NULL;
              m_gradient_on_image=NULL;
              m_gradient=NULL;
              m_small_image=NULL;
              WRATHResourceManagerBase::clear_all_resource_managers();
              m_tr->purge_cleanup();
            }
            #endif
            break;

          case FURYKey_Left:
            m_text_dirty=true;
            m_image_position.m_position.x()-=10.0f;
            ev->accept();
            break;

          case FURYKey_Right:
            m_text_dirty=true;
            m_image_position.m_position.x()+=10.0f;
            ev->accept();
            break;

          case FURYKey_Up:
            m_text_dirty=true;
            m_image_position.m_position.y()-=10.0f;
            ev->accept();
            break;

          case FURYKey_Down:
            m_text_dirty=true;
            m_image_position.m_position.y()+=10.0f;
            ev->accept();
            break;

          case FURYKey_X:
            m_text_dirty=true;
            m_image_position.m_scale*=1.1f;
            ev->accept();
            break;

          case FURYKey_Z:
            m_text_dirty=true;
            m_image_position.m_scale/=1.1f;
            ev->accept();
            break;

            
          case FURYKey_A:
            m_text_dirty=true;
            m_shape_position.m_position.x()-=10.0f;
            ev->accept();
            break;

          case FURYKey_D:
            m_text_dirty=true;
            m_shape_position.m_position.x()+=10.0f;
            ev->accept();
            break;

          case FURYKey_W:
            m_text_dirty=true;
            m_shape_position.m_position.y()-=10.0f;
            ev->accept();
            break;

          case FURYKey_S:
            m_text_dirty=true;
            m_shape_position.m_position.y()+=10.0f;
            ev->accept();
            break;

          case FURYKey_E:
            m_text_dirty=true;
            m_shape_position.m_scale*=1.1f;
            ev->accept();
            break;

          case FURYKey_Q:
            m_text_dirty=true;
            m_shape_position.m_scale/=1.1f;
            ev->accept();
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

          case FURYKey_I:
            m_rect_clipped_canvas_position.m_position.y()-=10.0f;
            ev->accept();
            break;

          case FURYKey_K:
            m_rect_clipped_canvas_position.m_position.y()+=10.0f;
            ev->accept();
            break;

          case FURYKey_J:
            m_rect_clipped_canvas_position.m_position.x()-=10.0f;
            ev->accept();
            break;

          case FURYKey_L:
            m_rect_clipped_canvas_position.m_position.x()+=10.0f;
            ev->accept();
            break;

          case FURYKey_O:
            m_rect_clipped_canvas_position.m_scale*=1.1f;
            ev->accept();
            break;

          case FURYKey_U:
            m_rect_clipped_canvas_position.m_scale/=1.1f;
            ev->accept();
            break;


            //m_clipper_mover_position m_clipper_mover
          case FURYKey_8:
            m_clipper_mover_position.m_position.y()-=10.0f;
            ev->accept();
            break;

          case FURYKey_5:
            m_clipper_mover_position.m_position.y()+=10.0f;
            ev->accept();
            break;

          case FURYKey_4:
            m_clipper_mover_position.m_position.x()-=10.0f;
            ev->accept();
            break;

          case FURYKey_6:
            m_clipper_mover_position.m_position.x()+=10.0f;
            ev->accept();
            break;

          case FURYKey_9:
            m_clipper_mover_position.m_scale*=1.1f;
            ev->accept();
            break;

          case FURYKey_7:
            m_clipper_mover_position.m_scale/=1.1f;
            ev->accept();
            break;

          case FURYKey_N:
            m_rect_clipper_position.m_position.x()+=5.0f;
            ev->accept();
            break;

          case FURYKey_B:
            m_rect_clipper_position.m_position.x()-=5.0f;
            ev->accept();
            break;

          case FURYKey_M:
            m_rect_clipper_position.m_position.y()+=5.0f;
            ev->accept();
            break;

          case FURYKey_V:
            m_rect_clipper_position.m_position.y()-=5.0f;
            ev->accept();
            break;

          case FURYKey_1:
            m_rect_clipper_position.m_scale/=1.1f;
            ev->accept();
            break;

          case FURYKey_3:
            m_rect_clipper_position.m_scale*=1.1f;
            ev->accept();
            break;

          case FURYKey_2:
            m_rect_clipper_visible=!m_rect_clipper_visible;
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





