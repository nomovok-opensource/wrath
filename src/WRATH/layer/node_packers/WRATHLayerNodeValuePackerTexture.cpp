/*! 
 * \file WRATHLayerNodeValuePackerTexture.cpp
 * \brief file WRATHLayerNodeValuePackerTexture.cpp
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
#include "WRATHLayerNodeValuePackerTexture.hpp"

/*
  Implementation overview:

  0) Elements are packed into vec4 residing on a texture
     The access is texture(node_uniform, node), i.e. the y-coordinate
     gives the node. Doing so allows that the texture upload from
     the nodes is pack_by_node
  1) For each entry in the passed ActiveNodeValuesCollection,
     we have a float (non-uniform) that fetches the 
     value from the massive uniform array. This fetching
     is done in the GLSL function WRATH_LAYER_UNIFORM_PACKER_TEXTURE_fetch_values()

  2) We generate the main which is just WRATH_LAYER_UNIFORM_PACKER_TEXTURE_fetch_values()
     followed by WRATH_LAYER_UNIFORM_PACKER_TEXTURE_real_main()

  3) The GLSL code then #define's main as WRATH_LAYER_UNIFORM_PACKER_TEXTURE_real_main 
     so that subsequent code that has a main actually defines 
     WRATH_LAYER_UNIFORM_PACKER_TEXTURE_real_main
   

 */

namespace
{
  const char *texture_name="WRATH_LAYER_UNIFORM_PACKER_TEXTURE_sampler";

  int
  compute_channel_count(enum WRATHLayerNodeValuePackerTexture::texture_channel_type tp)
  {
    static int channel_cnt[]=
    {
      /*[four_channel_texture]=*/ 4,
      /*[two_channel_texture]=*/ 2,
      /*[one_channel_texture]=*/ 1,
    };
    WRATHassert(tp<3);
    return channel_cnt[tp];
  }
  
  class Payload:public WRATHLayerNodeValuePackerBase::SpecDataProcessedPayload
  {
  public:
    typedef handle_t<Payload> handle;
    typedef const_handle_t<Payload> const_handle;

    explicit
    Payload(bool is_fp16, 
            enum WRATHLayerNodeValuePackerTexture::texture_channel_type ch):
      WRATHLayerNodeValuePackerBase::SpecDataProcessedPayload(),
      m_texture_unit(GL_INVALID_ENUM),
      m_texture_width(0),
      m_is_fp16(is_fp16),
      m_channel_format(ch)
    {
      WRATHLayerNodeValuePackerBase::NodeDataPackParametersCollection::packing_group pk_group(m_packer_parameters.default_packing_group());
      WRATHLayerNodeValuePackerBase::NodeDataPackParameters &pack_params(m_packer_parameters.packer_set_parameters(pk_group));

      pack_params.m_float_alignment=compute_channel_count(ch);
    }
         
    GLenum m_texture_unit;
    int m_texture_width;
    bool m_is_fp16;
    enum WRATHLayerNodeValuePackerTexture::texture_channel_type m_channel_format;
  };

  class NodePackerTextureFunctions:public WRATHLayerNodeValuePackerBase::function_packet
  {
  public:


    NodePackerTextureFunctions(const std::string &prec_string, 
                               enum WRATHLayerNodeValuePackerTexture::texture_channel_type ch);

    virtual
    SpecDataProcessedPayload::handle
    create_handle(const ActiveNodeValuesCollection &) const;

    virtual
    void
    append_fetch_code(WRATHGLShader::shader_source &src,
                      GLenum shader_stage,
                      const ActiveNodeValues &spec,
                      const SpecDataProcessedPayload::handle &hnd,
                      const std::string &index_name) const;

    virtual
    void
    add_actions(const SpecDataProcessedPayload::handle& /*payload*/,
                const ProcessedActiveNodeValuesCollection&,
                WRATHShaderSpecifier::ReservedBindings& /*reserved_bindings*/,
                WRATHGLProgramOnBindActionArray& /*actions*/,
                WRATHGLProgramInitializerArray& /*initers*/) const;

    virtual
    bool
    supports_per_node_value(GLenum) const
    {
      return true;
    }

  private:

    void
    append_extract_code(const ActiveNodeValues &spec,
                        std::ostream &ostr) const;

    

    std::string m_temp_type;
    const_c_array<std::string> m_member_swizzle;
    const_c_array<std::string> m_member_names;
    const_c_array<std::string> m_texture_swizzle;
    enum WRATHLayerNodeValuePackerTexture::texture_channel_type m_channel_format;
    unsigned int m_channel_count;
    std::string m_prec_string; 
    bool m_is_fp16;
  };

  class NodePackerTextureFunctionsPacket 
  {
  public:
    NodePackerTextureFunctionsPacket(enum WRATHLayerNodeValuePackerTexture::texture_channel_type ch):
      m_mediump("mediump", ch),
      m_highp("highp", ch)
    {}

    const NodePackerTextureFunctions&
    functions(enum WRATHLayerNodeValuePackerTexture::texture_packing_type type)
    {
      return (type==WRATHLayerNodeValuePackerTexture::fp16_texture)?
        m_mediump:
        m_highp;
    }

  private:
    NodePackerTextureFunctions m_mediump, m_highp;
  };

  class TextureForNodeBase:public WRATHTextureChoice::texture_base
  {
  public:
    typedef handle_t<TextureForNodeBase> handle;

    TextureForNodeBase(WRATHLayerNodeValuePackerBase::DataToGL q,
                       const Payload::const_handle &hnd);

    void
    bind_texture(GLenum ptexture_unit);

    int
    texture_width(void)
    {
      return m_texture_width; 
    }

    GLuint
    texture_name(void)
    {
      return m_texture_name;
    }

    GLenum
    texture_unit(void)
    {
      return m_texture_unit;
    }

    void
    deactivate(void)
    {
      if(m_texture_name!=0)
        {
          glDeleteTextures(1, &m_texture_name);
          m_texture_name=0;
        }
      m_active=false;
    }
    
    int
    num_channels(void) const
    {
      return m_num_channels;
    }

    enum WRATHLayerNodeValuePackerTexture::texture_channel_type 
    channel_format(void) const
    {
      return m_channel_format;
    }

  protected:
        
    virtual
    void
    upload_texture_data(const_c_array<float> input, int number_rows)=0;

    virtual
    void
    create_texture(void)=0;

  private:
    bool m_active;
    WRATHLayerNodeValuePackerBase::DataToGL m_source;
    GLenum m_texture_unit;
    GLuint m_texture_name;
    int m_texture_width;
    enum WRATHLayerNodeValuePackerTexture::texture_channel_type m_channel_format;
    int m_num_channels;
  };

  class TextureForNodeFP16:public TextureForNodeBase
  {
  public:
    TextureForNodeFP16(WRATHLayerNodeValuePackerBase::DataToGL q, 
                       const Payload::const_handle &h);

  protected:

    virtual
    void
    create_texture(void);

    virtual
    void
    upload_texture_data(const_c_array<float> input, int number_rows);

  private:
    GLenum m_texture_format;
    GLenum m_pixel_format;
    GLenum m_pixel_type;
    std::vector<uint16_t> m_fp16_data;
  };

  class TextureForNodeFP32:public TextureForNodeBase
  {
  public:
    TextureForNodeFP32(WRATHLayerNodeValuePackerBase::DataToGL q, 
                       const Payload::const_handle &h);

  protected:

    virtual
    void
    create_texture(void);

    virtual
    void
    upload_texture_data(const_c_array<float> input, int number_rows);

  private:
    GLenum m_texture_format;
    GLenum m_pixel_format;
    GLenum m_pixel_type;
  };
}

/////////////////////////////////
// TextureForNodeFP16 methods
TextureForNodeFP16::
TextureForNodeFP16(WRATHLayerNodeValuePackerBase::DataToGL q, 
                   const Payload::const_handle &h):
  TextureForNodeBase(q, h),
  m_fp16_data( num_channels()*texture_width()*256, 0)
{
  #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
  {
    static GLenum pixel_format[]=
      {
        /*[four_channel_texture]=*/ GL_RGBA,
        /*[two_channel_texture]= */ GL_LUMINANCE_ALPHA, 
        /*[one_channel_texture]= */ GL_LUMINANCE,
      };

    m_pixel_format=pixel_format[channel_format()];
    m_texture_format=m_pixel_format;
    m_pixel_type=GL_HALF_FLOAT_OES;
  }
  #else
  {
    static GLenum pixel_format[]=
      {
        /*[four_channel_texture]=*/ GL_RGBA,
        /*[two_channel_texture]= */ GL_RG, 
        /*[one_channel_texture]= */ GL_RED,
      };

    static GLenum tex_format[]=
      {
        /*[four_channel_texture]=*/ GL_RGBA16F,
        /*[two_channel_texture]= */ GL_RG16F,
        /*[one_channel_texture]= */ GL_R16F,
      };
    
    m_pixel_format=pixel_format[channel_format()];
    m_texture_format=tex_format[channel_format()];
    m_pixel_type=GL_HALF_FLOAT;
  }
  #endif


  WRATHassert(h->m_is_fp16);
}


void
TextureForNodeFP16::
create_texture(void)
{
  glTexImage2D(GL_TEXTURE_2D,
               0,
               m_texture_format,
               texture_width(), 256, 0,
               m_pixel_format, m_pixel_type, NULL);  
}


void
TextureForNodeFP16::
upload_texture_data(const_c_array<float> input, int number_rows)
{
  c_array<uint16_t> all_of_it(m_fp16_data);
  c_array<uint16_t> output(all_of_it.sub_array(0, input.size()));
  
  WRATHassert(static_cast<int>(input.size())==number_rows*num_channels()*texture_width());
  
  WRATHUtil::convert_to_halfp_from_float(output, input);

  if(texture_width()&1 and num_channels()==1)
    {
      /*
        if the texture width is odd and if the
        number of channels is 1, then there are
        an odd number of half floats per row,
        thus the alignment is 2, the size in
        bytes of a half float. All other situations
        have that the alignmnet is a multiple of 4
        anyways.
       */
      glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    }

  glTexSubImage2D(GL_TEXTURE_2D,
                  0, //LOD
                  0, 0, //bottom left corner
                  texture_width(), number_rows, //texture size
                  m_pixel_format, // format
                  m_pixel_type, //type
                  output.c_ptr());


}

//////////////////////////////////////
// TextureForNodeFP32 methods
TextureForNodeFP32::
TextureForNodeFP32(WRATHLayerNodeValuePackerBase::DataToGL q, 
                   const Payload::const_handle &h):
  TextureForNodeBase(q, h)
{
  #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
  {
    static GLenum pixel_format[]=
      {
        /*[four_channel_texture]=*/ GL_RGBA,
        /*[two_channel_texture]= */ GL_LUMINANCE_ALPHA, 
        /*[one_channel_texture]= */ GL_LUMINANCE,
      };
    m_pixel_format=pixel_format[channel_format()];
    m_texture_format=m_pixel_format;
  }
  #else
  {
    static GLenum pixel_format[]=
      {
        /*[four_channel_texture]=*/ GL_RGBA,
        /*[two_channel_texture]= */ GL_RG, 
        /*[one_channel_texture]= */ GL_RED,
      };

    static GLenum tex_format[]=
      {
        /*[four_channel_texture]=*/ GL_RGBA32F,
        /*[two_channel_texture]= */ GL_RG32F,
        /*[one_channel_texture]= */ GL_R32F,
      };
    m_pixel_format=pixel_format[channel_format()];
    m_texture_format=tex_format[channel_format()];
  }
  #endif

  WRATHassert(!h->m_is_fp16);
}

void
TextureForNodeFP32::
create_texture(void)
{
  glTexImage2D(GL_TEXTURE_2D,
               0,
               m_texture_format,
               texture_width(), 256, 0,
               m_pixel_format, GL_FLOAT, NULL);
  
}


void
TextureForNodeFP32::
upload_texture_data(const_c_array<float> input, int number_rows)
{
  WRATHassert(static_cast<int>(input.size())==number_rows*num_channels()*texture_width());
  glTexSubImage2D(GL_TEXTURE_2D,
                  0, //LOD
                  0, 0, //bottom left corner
                  texture_width(), number_rows, //size
                  m_pixel_format, // format
                  GL_FLOAT, //type
                  input.c_ptr());
}



//////////////////////////////////////
// TextureForNodeBase methods
TextureForNodeBase::
TextureForNodeBase(WRATHLayerNodeValuePackerBase::DataToGL q,
                   const Payload::const_handle &hnd):
  m_active(true),
  m_source(q),
  m_texture_unit(hnd->m_texture_unit),
  m_texture_name(0),
  m_texture_width(hnd->m_texture_width),
  m_channel_format(hnd->m_channel_format),
  m_num_channels(compute_channel_count(m_channel_format))
{
  

}


void
TextureForNodeBase::
bind_texture(GLenum ptexture_unit)
{
  WRATHassert(ptexture_unit==texture_unit());
  WRATHunused(ptexture_unit);

  if(!m_active)
    {
      return;
    }

  if(m_texture_name==0)
    {
      glGenTextures(1, &m_texture_name);
      WRATHassert(m_texture_name!=0);

      glBindTexture(GL_TEXTURE_2D, m_texture_name);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);      
      create_texture();
    }
  else
    {
      glBindTexture(GL_TEXTURE_2D, m_texture_name);
    }
  upload_texture_data(m_source.data_to_pack_to_GL_restrict(),
                      m_source.number_slots_to_pack_to_GL());
}

//////////////////////////////////////////////
// NodePackerTextureFunctions methods
NodePackerTextureFunctions::
NodePackerTextureFunctions(const std::string &prec_string, 
                           enum WRATHLayerNodeValuePackerTexture::texture_channel_type ch):
  m_channel_format(ch),
  m_prec_string(prec_string),
  m_is_fp16(prec_string==std::string("mediump"))
{
  m_channel_count=compute_channel_count(ch);
  WRATHStaticInit();

  /*
    the temporaray has the type m_temp_type
    m_member_names gives the member to take from the temporary, 
                   [i] = i'th component, 0<=i<m_channel_count

    m_texture_swizzle_values gives the swizzle from the texture lookup
                   [0] = all, 
                   [i] (for i!=0) grab only i node values, 1<=i<m_channel_count

    m_member_swizzle left hand side of taking value from texture lookup,
                   [0] = all, 
                   [i] (for i!=0) grab only i node values, 1<=i<m_channel_count

   */

  switch(m_channel_format)
    {
    case WRATHLayerNodeValuePackerTexture::four_channel_texture :
      {
        static vecN<std::string, 4> members(".x", ".y", ".z", ".w");
        static vecN<std::string, 4> tex_swi(".xyzw", ".x", ".xy", ".xyz");

        m_member_names=members;
        m_texture_swizzle=tex_swi;
        m_member_swizzle=m_texture_swizzle;
        m_temp_type="vec4";
      }
      break;

    case WRATHLayerNodeValuePackerTexture::two_channel_texture :
      {
        static vecN<std::string, 2> members(".x", ".y");
        
        #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
          static vecN<std::string, 2> tex_swi(".xw", ".x");
        #else
          static vecN<std::string, 2> tex_swi(".xy", ".x");
        #endif
        
        static vecN<std::string, 2> mem_swi(".xy", ".x");

        m_member_names=members;
        m_texture_swizzle=tex_swi;
        m_member_swizzle=mem_swi;
        m_temp_type="vec2";
      }
      break;

    case WRATHLayerNodeValuePackerTexture::one_channel_texture :
      {
        static vecN<std::string, 1> members("");
        static vecN<std::string, 1> tex_swi(".x");
        static vecN<std::string, 1> mem_swi("");

        m_member_names=members;
        m_texture_swizzle=tex_swi;
        m_member_swizzle=mem_swi;
        m_temp_type="float";
      }
      break;
    }
}

NodePackerTextureFunctions::SpecDataProcessedPayload::handle
NodePackerTextureFunctions::
create_handle(const ActiveNodeValuesCollection &) const
{
  return WRATHNew Payload(m_is_fp16, m_channel_format); 
}

void
NodePackerTextureFunctions::
append_extract_code(const ActiveNodeValues &input, std::ostream &ostr) const
{
  if(input.entries().empty())
    {
      return;
    }

  unsigned int size(input.number_active());
  unsigned int size_divCount(size/m_channel_count);
  unsigned int count, idx;
  static const char *temp_label="WRATH_LAYER_UNIFORM_TEXTURE_temp";
  int div_factor;
  float factor;
  std::vector<std::string> entries_ordered_by_source_index; 
  

  ostr << "\n\t" << m_prec_string << " "
       << m_temp_type << " " << temp_label << ";";

  /*
    we want to minimize the number of times we need 
    to call texture2DLod(). So we make several loops,
    first using 4-coordinate look up as much as possible,
    then a 1, 2, or 3 look up.
   */
  div_factor=size_divCount;
  if(m_channel_count!=1 and size%m_channel_count)
    {
      ++div_factor;
    }
  factor=1.0f/static_cast<float>(div_factor);

  /*
    we need the entries sorted by ActiveNodeValue::m_offset
   */
  entries_ordered_by_source_index.resize(size);
  for(ActiveNodeValues::map_type::const_iterator 
        iter=input.entries().begin(),
        end=input.entries().end(); 
      iter!=end; ++iter)
    {
      entries_ordered_by_source_index[iter->second.m_offset]=iter->second.label();
    }

  for(idx=0, count=0; idx<size_divCount; ++idx, count+=m_channel_count)
    {
      float x_tex;

      x_tex=(0.5f + static_cast<float>(idx) ) * factor;
      ostr << "\n\t" << temp_label << m_member_swizzle[0]
           << "=texture2DLod("
           << texture_name << ", vec2("
           << x_tex << ", node_texel), 0.0)"
           << m_texture_swizzle[0] << ";"
           << " // idx=" << idx << ", div_factor=" << div_factor
           << ", factor=" << factor;

      for(unsigned int i=0; i<m_channel_count; ++i)
        {
          ostr << "\n\t" << entries_ordered_by_source_index[i+count]
               << "=" << temp_label << m_member_names[i] << ";";
        }
    }

  if(size>count)
    {
      unsigned int remaining(size-count);

      WRATHassert(remaining>0);
      WRATHassert(remaining+1<=m_channel_count);

      ostr << "\n\t" << temp_label << m_member_swizzle[remaining]
           << "=texture2DLod("
           << texture_name << ", vec2(1.0, node_texel), 0.0)"
           << m_texture_swizzle[remaining] << ";";

      for(unsigned int i=0; i<remaining; ++i)
        {
          ostr << "\n\t" << entries_ordered_by_source_index[i+count]
               << "=" << temp_label << m_member_names[i] << ";";
        }
    }

  ostr << "\n\n";
}


void
NodePackerTextureFunctions::
append_fetch_code(WRATHGLShader::shader_source &src,
                  GLenum /*shader_stage*/,
                  const ActiveNodeValues &spec,
                  const SpecDataProcessedPayload::handle& /*hnd*/,
                  const std::string &index_name) const
{
  std::ostringstream ostr;

      
  ostr << "\n\n#define fetch_node_value(X) X\n";
  for(ActiveNodeValues::map_type::const_iterator iter=spec.entries().begin(),
        end=spec.entries().end(); iter!=end; ++iter)
    {
      ostr << "\n" << m_prec_string << " float " 
           << iter->second.label() << ";"
           << " // source index=" << iter->second.m_source_index
           << " = " << iter->first
           << ", offset=" << iter->second.m_offset;
      /*
        TODO: add #define statements for labels beyond the first label.
       */
    }
  
  
  ostr << "\nuniform "
       << m_prec_string << " sampler2D "
       << texture_name << ";\n";
    
  /*
    255 or 256 ?
   */
  ostr << "void pre_fetch_node_values(void)"
       << "\n{"
       << "\n\tmediump float node_texel;"
       << "\n\tnode_texel=("
       << index_name << " + 0.5) / 255.0;";
  
  
  append_extract_code(spec, ostr);  
  ostr << "\n}\n\n";
  
  
  src.add_source(ostr.str(), WRATHGLShader::from_string);
}

void
NodePackerTextureFunctions::
add_actions(const SpecDataProcessedPayload::handle& h,
            const ProcessedActiveNodeValuesCollection &pr,
            WRATHShaderSpecifier::ReservedBindings& reserved_bindings,
            WRATHGLProgramOnBindActionArray&,
            WRATHGLProgramInitializerArray& initers) const
{
  Payload::handle payload;
  WRATHassert(h.dynamic_cast_handle<Payload>().valid());
  payload=h.static_cast_handle<Payload>();

  /*
    set the texture width now, there should
    be one packing way (or zero if there are none-to pack)
   */
  WRATHassert(pr.number_indices()<=1);
  if(pr.number_indices()>0)
    {
      int number_active(0);
      int tex_width;

      number_active=pr.active_node_values(0).number_active();
      tex_width=number_active/m_channel_count;
      if(m_channel_count!=1 and (number_active%m_channel_count)!=0 )
        {
          ++tex_width;
        }

      payload->m_texture_width=tex_width;
    }
  

  if(payload->m_texture_width==0)
    {
      return;
    }

  /* 
     find texture unit to use for the lookup.
   */
  GLenum tex_unit(GL_TEXTURE0);

  /*
    find the first texture unit not used by 
    reserved_bindings.m_texture_binding_points
   */
  for(std::set<GLenum>::const_iterator current=reserved_bindings.m_texture_binding_points.begin(),
        end=reserved_bindings.m_texture_binding_points.end(); current!=end; ++current)
    {
      if(tex_unit<*current)
        {
          break;
        }
      else
        {
          tex_unit=std::max(tex_unit, *current + 1);
        }
    }
  

  payload->m_texture_unit=tex_unit;
  initers
    .add_sampler_initializer(texture_name, tex_unit-GL_TEXTURE0);

  reserved_bindings.add_texture_binding(tex_unit);
  
}


//////////////////////////////////////////////////
// WRATHLayerNodeValuePackerTexture methods
WRATHLayerNodeValuePackerTexture::
WRATHLayerNodeValuePackerTexture(WRATHLayerBase *layer,
                                 const SpecDataProcessedPayload::const_handle &h,
                                 const ProcessedActiveNodeValuesCollection &spec):
  WRATHLayerNodeValuePackerBase(layer, h, spec)
{  
  Payload::const_handle payload;



  WRATHassert(h.dynamic_cast_handle<Payload>().valid());
  payload=h.static_cast_handle<Payload>();
  
  if(payload->m_texture_width>0)
    {
      DataToGL datum(data_to_gl_indexed(0));
      if(payload->m_is_fp16)
        {
          m_texture=WRATHNew TextureForNodeFP16(datum, payload);
        }
      else
        {
          m_texture=WRATHNew TextureForNodeFP32(datum, payload);
        }
    }
}

WRATHLayerNodeValuePackerTexture::
~WRATHLayerNodeValuePackerTexture()
{}

void
WRATHLayerNodeValuePackerTexture::
phase_render_deletion(void)
{
  if(m_texture.valid())
    {
      TextureForNodeBase::handle H;
      
      WRATHassert(m_texture.static_cast_handle<TextureForNodeBase>().valid());
      H=m_texture.static_cast_handle<TextureForNodeBase>();
      
      /*
        deletes the texture and makes the uniform inactive
      */
      H->deactivate();
      m_texture=NULL;
    }

  WRATHLayerNodeValuePackerBase::phase_render_deletion();
}
  
void
WRATHLayerNodeValuePackerTexture::
append_state(WRATHSubItemDrawState &skey)
{
  if(m_texture.valid())
    {
      TextureForNodeBase::handle H;
      
      WRATHassert(m_texture.static_cast_handle<TextureForNodeBase>().valid());
      H=m_texture.static_cast_handle<TextureForNodeBase>();
      
      skey.add_texture(H->texture_unit(), H);
    }
}

const WRATHLayerNodeValuePackerBase::function_packet&
WRATHLayerNodeValuePackerTexture::
functions(enum texture_packing_type type, enum texture_channel_type ch)
{
  WRATHStaticInit();
  
  static NodePackerTextureFunctionsPacket one(one_channel_texture);
  static NodePackerTextureFunctionsPacket two(two_channel_texture);
  static NodePackerTextureFunctionsPacket four(four_channel_texture);

  switch(ch)
    {
    default:
    case four_channel_texture:
      return four.functions(type);

    case two_channel_texture:
      return two.functions(type);

    case one_channel_texture:
      return one.functions(type);
    }
}
