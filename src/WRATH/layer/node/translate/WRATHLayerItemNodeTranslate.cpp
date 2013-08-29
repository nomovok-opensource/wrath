/*! 
 * \file WRATHLayerItemNodeTranslate.cpp
 * \brief file WRATHLayerItemNodeTranslate.cpp
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
#include "WRATHLayerItemNodeTranslate.hpp"
#include "WRATHLayer.hpp"
#include "WRATHStaticInit.hpp"
#include "WRATHUtil.hpp"


namespace
{
  const char quad_draw_program_name[]="WRATHLayerItemNodeTranslate:ClipMeshNodeDrawer@"__FILE__;

  WRATHGLProgram*
  quad_drawer(void)
  {
    WRATHGLProgram *q;

    q=WRATHGLProgram::retrieve_resource(quad_draw_program_name);
    if(q==NULL)
      {
        q=WRATHNew WRATHGLProgram(quad_draw_program_name,

                                  WRATHGLShader::shader_source()
                                  .add_source("layer_translate_clip_rect.vert.wrath-shader.glsl", 
                                              WRATHGLShader::from_resource),

                                  WRATHGLShader::shader_source()
                                  .add_source("layer_translate_clip_rect.frag.wrath-shader.glsl", 
                                              WRATHGLShader::from_resource),

                                  WRATHGLPreLinkActionArray()
                                  .add_binding("in_normalized_pts", 0) );

        WRATHassert(q==WRATHGLProgram::retrieve_resource(quad_draw_program_name));
      }

    return q;
  }

  class QuadDrawer
  {
  public:

    QuadDrawer(void);

    void
    draw(const float4x4 &pvm,
         const vec2 &p, const vec2 &q);

  private:
    GLint m_pvm;
    GLint m_p, m_q;    
    WRATHGLProgram *m_gl_program;
  };


  class FromNodeValues
  {
  public:
    bool m_visible;
    bool m_clipped;
    vec2 m_p, m_q;
  };

  class Transformer:
    public WRATHLayerIntermediateTransformation
  {
  public:
    explicit
    Transformer(const WRATHTripleBufferEnabler::handle &tr,
                WRATHLayerItemNodeTranslateValues *q):
      m_node(q),
      m_tr(tr)
    {
      m_sig=m_tr->connect(WRATHTripleBufferEnabler::on_complete_simulation_frame,
                          WRATHTripleBufferEnabler::pre_update_no_lock,
                          boost::bind(&Transformer::on_complete_simulation_frame, this));
    }

    ~Transformer()
    {
      m_sig.disconnect();
    }

    void
    unhook(void)
    {
      m_sig.disconnect();
      m_node=NULL;
    }

    virtual
    void
    modify_matrix(float4x4& in_out_matrix);

  private:

    void
    on_complete_simulation_frame(void)
    {
      if(m_node!=NULL)
        {
          m_values[m_tr->current_simulation_ID()]=m_node->m_transformation;
        }
      else
        {
          m_values[m_tr->current_simulation_ID()]=WRATHScaleTranslate();
        }
    }

    WRATHLayerItemNodeTranslateValues *m_node;
    WRATHTripleBufferEnabler::handle m_tr;
    vecN<WRATHScaleTranslate, 3> m_values;
    WRATHTripleBufferEnabler::connect_t m_sig;

  };

  class NodeMagic:public WRATHLayerClipDrawer
  {
  public:
    explicit
    NodeMagic(const WRATHTripleBufferEnabler::handle &tr,
              WRATHLayerItemNodeTranslateValues *q):
      m_node(q),
      m_tr(tr)
    {
      /*
        note:  pre_update_no_lock, i.e. use the values of the just completed
        simulation frame!
       */
      m_sig=m_tr->connect(WRATHTripleBufferEnabler::on_complete_simulation_frame,
                          WRATHTripleBufferEnabler::pre_update_no_lock,
                          boost::bind(&NodeMagic::on_complete_simulation_frame, this));
    }

    ~NodeMagic()
    {
      m_sig.disconnect();
      
    }

    void
    unhook(void)
    {
      m_sig.disconnect();
      m_node=NULL;
    }

    virtual
    DrawStateElementClipping
    clip_mode(WRATHLayer *layer,
              const DrawStateElementTransformations &layer_transformations,
              const_c_array<DrawStateElement> draw_state_stack) const;

    virtual
    void
    draw_region(bool clear_z, 
                const DrawStateElement &layer,
                const_c_array<DrawStateElement> draw_stack) const;

  private:
    
    void
    on_complete_simulation_frame(void);

    void
    set_clipping(void);

    WRATHLayerItemNodeTranslateValues *m_node;
    WRATHTripleBufferEnabler::handle m_tr;
    vecN<FromNodeValues, 3> m_values;
    WRATHTripleBufferEnabler::connect_t m_sig;

   
    mutable QuadDrawer m_quad_drawer;
  };


  class NodeTranslateFunctions:public WRATHLayerItemNodeBase::node_function_packet
  {
  public:
    virtual
    WRATHLayerItemNodeBase*
    create_completely_clipped_node(const WRATHTripleBufferEnabler::handle &r) const
    {
      WRATHLayerItemNodeTranslate *return_value;

      return_value=WRATHNew WRATHLayerItemNodeTranslate(r);
      return_value->visible(false);
      return return_value;
    }

    virtual
    void
    add_per_node_values(WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                          const WRATHLayerNodeValuePackerBase::function_packet &) const
    {
      spec
        .add_source(0, "WRATH_LAYER_TRANSLATE_X", GL_VERTEX_SHADER)
        .add_source(1, "WRATH_LAYER_TRANSLATE_Y", GL_VERTEX_SHADER)
        .add_source(2, "WRATH_LAYER_TRANSLATE_Z", GL_VERTEX_SHADER)
        .add_source(3, "WRATH_LAYER_TRANSLATE_SCALE", GL_VERTEX_SHADER)
        .add_source(4, "WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MIN_X", GL_VERTEX_SHADER)
        .add_source(5, "WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MAX_X", GL_VERTEX_SHADER)
        .add_source(6, "WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MIN_Y", GL_VERTEX_SHADER)
        .add_source(7, "WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MAX_Y", GL_VERTEX_SHADER);
    }

    virtual
    void
    append_shader_source(std::map<GLenum, WRATHGLShader::shader_source> &src,
                         const WRATHLayerNodeValuePackerBase::function_packet &) const
    {
      src[GL_VERTEX_SHADER].add_source("transformation_layer_translate.vert.wrath-shader.glsl", 
                                       WRATHGLShader::from_resource);

      src[GL_FRAGMENT_SHADER].add_source("transformation_layer_translate.frag.wrath-shader.glsl", 
                                         WRATHGLShader::from_resource);
    }
  };

  

}

//////////////////////////
// Transformer
void
Transformer::
modify_matrix(float4x4& in_out_matrix)
{
  const WRATHScaleTranslate &value(m_values[m_tr->present_ID()]);  
  /*
    insert the transformation 
    between the parent and the layer,
    that is why we multiply by on the left.
  */
  vec2 v(value.translation());
  float f(value.scale());
  float4x4 M;
  
  M(0,0)=M(1,1)=f;      
  M(0,3)=v.x();
  M(1,3)=v.y();
      
  in_out_matrix=M*in_out_matrix;
}

////////////////////////////////////////
// QuadDrawer
QuadDrawer::
QuadDrawer(void):
  m_gl_program(NULL)
{}

void
QuadDrawer::
draw(const float4x4 &pvm,
     const vec2 &p, const vec2 &q)
{
  if(m_gl_program==NULL)
    {
      m_gl_program=quad_drawer();

      m_pvm=glGetUniformLocation(m_gl_program->name(), "pvm");
      WRATHassert(m_pvm!=-1);

      m_p=glGetUniformLocation(m_gl_program->name(), "p");
      WRATHassert(m_p!=-1);

      m_q=glGetUniformLocation(m_gl_program->name(), "q");
      WRATHassert(m_q!=-1);
    }


  m_gl_program->use_program();

  WRATHglUniform(m_pvm, pvm);
  WRATHglUniform(m_p, p);
  WRATHglUniform(m_q, q);

  const GLbyte corners_as_01[]=
    {
      0, 0,
      1, 0,
      1, 1,

      0, 0,
      1, 1,
      0, 1,

    };

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glVertexAttribPointer(0, //index
                        2, //count 
                        opengl_trait<GLbyte>::type, //type
                        GL_FALSE, //normalize
                        0, //stride
                        corners_as_01); //offset/ptr

  for(int i=1;i<WRATHDrawCallSpec::attribute_count; ++i)
    {
      glDisableVertexAttribArray(i);
    }

  glDrawArrays(GL_TRIANGLES, 0, 6);
  
}

/////////////////////////////////////////
// NodeMagic methods
void
NodeMagic::
on_complete_simulation_frame(void)
{
  FromNodeValues &v(m_values[m_tr->current_simulation_ID()]);
  if(m_node!=NULL)
    {
      WRATHBBox<2> clip_box;

      v.m_clipped=m_node->m_clipping_active;
      clip_box=m_node->m_clip_rect;

      v.m_visible=m_node->m_visible
        and !(v.m_clipped and clip_box.empty());

      if(v.m_visible and v.m_clipped)
        {
          /*
            we want m_p and m_q is GLOBAL coordinates
            of the node because the vertex shader's pvm matrix
            comes from the _parent_ render layer pvm matrix.
           */
          v.m_p=clip_box.min_corner();
          v.m_q=clip_box.max_corner();
        }
    }
  else
    {
      v.m_visible=false;
    }
}



WRATHLayerClipDrawer::DrawStateElementClipping
NodeMagic::
clip_mode(WRATHLayer*, 
          const DrawStateElementTransformations &layer_transformation, 
          const_c_array<DrawStateElement> state_stack) const
{
  /*
    TODO: use the bounding rect of value to fill 
    the return values bounding box values better
   */
  const FromNodeValues &value(m_values[m_tr->present_ID()]);

  if(!value.m_visible)
    {
      return skip_layer; 
    }
  
  WRATHLayerClipDrawer::DrawStateElementClipping return_value;
  if(!value.m_clipped)
    {
      return_value.m_clip_mode=layer_unclipped;
      return_value.m_device_bbox=state_stack.back().m_clipping.m_device_bbox;
    }
  else
    {
      float4x4 pvm;
      WRATHLayer *p(state_stack.back().m_layer);

      if(p!=NULL)
        {
          pvm=state_stack.back().m_transformations.m_composed_pvm;
        }
      else
        {
          /*
            NOTE: if the layer has no parent,
            then we view the node as giving a transformation 
            after projection, but before the layers modelview...
            is this correct? should we instead have m_composed_pvm?
            or just the identity?
           */
          pvm=layer_transformation.m_composed_projection;
        }

      vecN<vec4,4> proj_points;
      vecN<vec2,4> screen_pts;

      proj_points[0]=pvm*vec4(value.m_p.x(), value.m_p.y(), -1.0f, 1.0f);
      proj_points[2]=pvm*vec4(value.m_p.x(), value.m_q.y(), -1.0f, 1.0f);
      proj_points[1]=pvm*vec4(value.m_q.x(), value.m_q.y(), -1.0f, 1.0f);
      proj_points[3]=pvm*vec4(value.m_q.x(), value.m_p.y(), -1.0f, 1.0f);
      /*
        TODO: clip on w<0 as well, i.e. if w<0,
        then we need to realize the portion of the
        rectangle that is infrom of w>0.
       */
      WRATHBBox<2> pbox;
      for(int i=0;i<4;++i)
        {
          screen_pts[i]=vec2(proj_points[i].x(), proj_points[i].y())/proj_points[i].w();
          pbox.set_or(screen_pts[i]);
        }
     

      /**/
      return_value.m_device_bbox=pbox.intersection(state_stack.back().m_clipping.m_device_bbox);
      return_value.m_clip_mode=return_value.m_device_bbox.empty()?
        skip_layer:
        layer_clipped_sibling;

      /*
      return_value.m_device_bbox=state_stack.back().m_clipping.m_device_bbox;
      return_value.m_clip_mode=layer_clipped_sibling;
      */
    }

  return return_value;
        
}




void
NodeMagic::
draw_region(bool clear_z, 
            const DrawStateElement &player,
            const_c_array<DrawStateElement> /*draw_stack*/) const
{
  /*
    use draw_statck to get values about layer and it's render parent...
   */
  WRATHLayer *layer(player.m_layer);
  const FromNodeValues &value(m_values[m_tr->present_ID()]);

  WRATHassert(value.m_visible);
  WRATHassert(value.m_clipped); 

  float4x4 pvm;
  WRATHLayer *p(layer->current_render_parent());

  if(p!=NULL)
    {
      pvm=p->current_render_transformation().m_composed_pvm;
    }
  else
    {
      pvm=layer->current_render_transformation().m_composed_projection;
    }

  if(clear_z)
    {
      /*
        set pvm so that it makes z_clip=w_clip,
        which is done simply by having the
        3rd row identical to the 4'th row
       */
      pvm(2,0)=pvm(3,0);
      pvm(2,1)=pvm(3,1);
      pvm(2,2)=pvm(3,2);
      pvm(2,3)=pvm(3,3);
    }
  m_quad_drawer.draw(pvm, value.m_p, value.m_q);
}



//////////////////////////////////////////////
// WRATHLayerItemNodeTranslateValues methods
void
WRATHLayerItemNodeTranslateValues::
compose(const WRATHLayerItemNodeTranslateValues &parent_value,
        const WRATHLayerItemNodeTranslateValues &local)
{
  
  m_visible=parent_value.m_visible and local.m_visible;
  m_clipping_active=parent_value.m_clipping_active or local.m_clipping_active;
  m_transformation=parent_value.m_transformation*local.m_transformation;
  
  /*
    compute the clipping if needed:
  */
  if(m_clipping_active)
    {
      /*
        parent_value.m_x_clipping gives clipping in global coordinates,
        and we want ours there too
      */
      if(local.m_clipping_active)
        {
          m_clip_rect=local.m_clip_rect;
          m_clip_rect.scale(m_transformation.scale());
          m_clip_rect.translate(m_transformation.translation());
        }
      else
        {
          m_clip_rect=parent_value.m_clip_rect;
        }
      
      if(parent_value.m_clipping_active and local.m_clipping_active)
        {
          m_clip_rect=m_clip_rect.intersection(parent_value.m_clip_rect);
        } 
      
      m_visible=m_visible and !m_clip_rect.empty();         
    }
}


WRATHLayerIntermediateTransformation::handle
WRATHLayerItemNodeTranslateValues::
create_pre_transformer(const WRATHTripleBufferEnabler::handle &tr)
{
  return WRATHNew Transformer(tr, this);
}

void
WRATHLayerItemNodeTranslateValues::
unhook_transformer(const WRATHLayerIntermediateTransformation::handle &h)
{
  if(h.valid())
    {
      WRATHassert(dynamic_cast<Transformer*>(h.raw_pointer())!=NULL);
      static_cast<Transformer*>(h.raw_pointer())->unhook();
    }
}

WRATHLayerClipDrawer::handle
WRATHLayerItemNodeTranslateValues::
create_clip_drawer(const WRATHTripleBufferEnabler::handle &tr)
{
  return WRATHNew NodeMagic(tr, this);
}

void
WRATHLayerItemNodeTranslateValues::
unhook_clip_drawer(const WRATHLayerClipDrawer::handle &h)
{
  if(h.valid())
    {
      WRATHassert(dynamic_cast<NodeMagic*>(h.raw_pointer())!=NULL);
      static_cast<NodeMagic*>(h.raw_pointer())->unhook();
    }
}



const WRATHLayerItemNodeBase::node_function_packet&
WRATHLayerItemNodeTranslateValues::
functions(void) 
{
  WRATHStaticInit();
  static NodeTranslateFunctions return_value;

  return return_value;
}

void
WRATHLayerItemNodeTranslateValues::
extract_values(reorder_c_array<float> out_values, float z_order)
{  
  bool visible;

  visible=m_visible and !(m_clipping_active and m_clip_rect.empty());
  out_values[0]=m_transformation.translation().x();
  out_values[1]=m_transformation.translation().y();  
  out_values[2]=(visible)?
    z_order:
    -100.0f;

  out_values[3]=m_clipping_active?
    m_transformation.scale():
    -m_transformation.scale();

  if(m_clipping_active and visible)
    {
      vec2 p(m_clip_rect.min_corner());
      vec2 q(m_clip_rect.max_corner());
      
      WRATHScaleTranslate tr(m_transformation.inverse());
      p=tr.apply_to_point(p);
      q=tr.apply_to_point(q);
      
      /*
        NOTE: we are storing in the vertex shader
        the clipping rectangle relative to local
        coordinates! This is because it is easier
        to clip before rather than after!
      */
      out_values[4]=std::min(p.x(), q.x());
      out_values[5]=std::max(p.x(), q.x());
      out_values[6]=std::min(p.y(), q.y());
      out_values[7]=std::max(p.y(), q.y());
    }
  else
    {
      out_values[4]=-1.0f;
      out_values[5]=+1.0f;
      out_values[6]=-1.0f;
      out_values[7]=+1.0f;
    }
}
