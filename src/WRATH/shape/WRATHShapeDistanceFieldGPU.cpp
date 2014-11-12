/*! 
 * \file WRATHShapeDistanceFieldGPU.cpp
 * \brief file WRATHShapeDistanceFieldGPU.cpp
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
#include <math.h>
#include <complex>
#include "WRATHShapeDistanceFieldGPU.hpp"
#include "WRATHGLProgram.hpp"
#include "matrixGL.hpp"
#include "WRATHglGet.hpp"
#include "WRATHgluniform.hpp"
#include "WRATHGLExtensionList.hpp"
#include "WRATHGLStateStack.hpp"
#include "WRATHStaticInit.hpp"
#include "WRATHStaticInit.hpp"

namespace
{
 
   
  class DrawerCommon:public WRATHGLProgram
  {
  public:
    DrawerCommon(const std::string &pname,
                 const std::string &vs_shader_src,
                 const std::string &fs_shader_src,
                 DrawerCommon*&ptr,
                 bool requires_draw_positive_distances);
    
    ~DrawerCommon();

    void
    bind_and_set_uniforms(const float4x4 &pvm);

    void
    bind_and_set_uniforms(const float4x4 &pvm,
                          bool draw_positive_distances);

  private:
    GLint m_pvm, m_distance_sign;
    DrawerCommon* &m_ptr;
  };

  static
  DrawerCommon*
  simple_drawer(void)
  {
    WRATHStaticInit();
    static DrawerCommon *R(NULL);
    if(R==NULL)
      {
        WRATHNew DrawerCommon("distance_field_simple_renderer",
                              "distance_field_simple_shader.vert.wrath-shader.glsl",
                              "distance_field_simple_shader.frag.wrath-shader.glsl",
                              R, false);
      }
    WRATHassert(R!=NULL);
    return R;
  }

  static
  DrawerCommon*
  primitive_drawer(void)
  {
    WRATHStaticInit();
    static DrawerCommon *R(NULL);
    if(R==NULL)
      {
        WRATHNew DrawerCommon("distance_field_primitive_renderer",
                              "distance_field_draw_distance_rects.vert.wrath-shader.glsl",
                              "distance_field_draw_distance_rects.frag.wrath-shader.glsl",
                              R, true);
      }
    WRATHassert(R!=NULL);
    return R;
  }

  static
  DrawerCommon*
  point_drawer(void)
  {
    WRATHStaticInit();
    static DrawerCommon *R(NULL);
    if(R==NULL)
      {
        WRATHNew DrawerCommon("distance_field_point_renderer",
                              "distance_field_draw_distance_points.vert.wrath-shader.glsl",
                              "distance_field_draw_distance_points.frag.wrath-shader.glsl",
                              R, true);
      }
    WRATHassert(R!=NULL);
    return R;
  }
    
  class FillRenderer:boost::noncopyable
  {
  public:
    explicit
    FillRenderer(const WRATHShapeSimpleTessellatorPayload::handle &h);

    void
    draw_to_stencil(const float4x4 &pvm) const;

    void
    draw_fans(const float4x4 &pvm) const;

  private:

    std::vector< std::vector<vec2> > m_fan_pts;
  };


  class EdgeRects:boost::noncopyable
  {
  public:
    EdgeRects(const WRATHShapeSimpleTessellatorPayload::handle &h, 
              float geometry_dist);

    void
    draw(const float4x4 &pvm,
         bool positive_distances) const;

  private:    

    std::vector<vec3> m_verts;
    std::vector<GLushort> m_inds;
  };

  class PointRects:boost::noncopyable
  {
  public:
    PointRects(bool draw_pts_as_fans, float geometry_dist,
               const WRATHShapeSimpleTessellatorPayload::handle &h,
               float pixel_dist);
    void
    draw(const float4x4 &pvm,
         bool positive_distances) const;

  private:
    class packet_of_fans
    {
    public:
      std::vector<vec3> m_draw_as_fans_attrs;
      std::vector<GLushort> m_draw_as_fans_indices;
    };

    class packet_of_rects
    {
    public:
      std::vector<vec4> m_draw_as_rects_attrs;
      std::vector<GLushort> m_draw_as_rects_indices;
    };

    void
    build_fans(const std::vector<vec2> &pts, 
               float geometry_dist, float pixel_dist);

    void
    build_rects(const std::vector<vec2> &pts, float geometry_dist);

    bool m_draw_pts_as_fans;
    float m_sprite_radius;
    const WRATHShapeSimpleTessellatorPayload::handle m_src;

    std::list<packet_of_rects> m_rects;
    std::list<packet_of_fans> m_fans;
  };

  class DistanceRenderer:boost::noncopyable
  {
  public:
    DistanceRenderer(bool draw_pts_as_sprites, 
                     bool bother_with_pts,
                     float geometry_dist,
                     const WRATHShapeSimpleTessellatorPayload::handle &h,
                     float pixel_dist):
      m_edges(h, geometry_dist),
      m_points(NULL)
    {
      if(bother_with_pts)
        {
          m_points=WRATHNew PointRects(!draw_pts_as_sprites, geometry_dist, h, pixel_dist);
        }
    }

    ~DistanceRenderer()
    {
      if(m_points!=NULL)
        {
          WRATHDelete(m_points);
        }
    }

    void
    draw(const float4x4 &pvm, const FillRenderer &f, 
         bool use_depth_buffer) const;

  private:
    EdgeRects m_edges;
    PointRects *m_points;

  };


}


/////////////////////////////////
// DrawerCommon methods
DrawerCommon::
DrawerCommon(const std::string &pname,
             const std::string &vs_shader_src,
             const std::string &fs_shader_src,
             DrawerCommon* &ptr,
             bool requires_draw_positive_distances):
  WRATHGLProgram(pname,
                 WRATHGLShader::shader_source()
                 .add_source(vs_shader_src, WRATHGLShader::from_resource),
                 WRATHGLShader::shader_source()
                 .add_source(fs_shader_src, WRATHGLShader::from_resource)),
  m_ptr(ptr)
{
  WRATHassert(m_ptr==NULL);

  /*
    ok to grab uniforms immediately because these are
    only consructed just befor gettin used.
   */
  m_ptr=this;
  m_pvm=uniform_location("pvm");
  WRATHassert(m_pvm!=-1);

  if(requires_draw_positive_distances)
    {
      m_distance_sign=uniform_location("distance_sign");
      WRATHassert(m_distance_sign!=-1);
    }
}


DrawerCommon::
~DrawerCommon()
{
  WRATHassert(m_ptr==this);
  m_ptr=NULL;
}

void
DrawerCommon::
bind_and_set_uniforms(const float4x4 &pvm)
{
  use_program();
  WRATHglUniform(m_pvm, pvm);
}

void
DrawerCommon::
bind_and_set_uniforms(const float4x4 &pvm,
                      bool draw_positive_distances)
{
  float dis(draw_positive_distances?1.0f:-1.0f);

  bind_and_set_uniforms(pvm);
  
  WRATHassert(m_distance_sign!=-1);
  WRATHglUniform(m_distance_sign, dis); 
}


/////////////////////////////
//FillRenderer  methods
FillRenderer::
FillRenderer(const WRATHShapeSimpleTessellatorPayload::handle &h)
{
  /*
    Our primitives are the following:
    for each edge [e0,e1], we create the
    triangles

    [e0,e1,f0] and [f0,e0,f1]

    where 
    f0.x()=f1.x()=min_x,
    f0.y()=e0.y() and f1.y()=e1.y().
   */

  m_fan_pts.resize(h->tessellation().size());
  
  for(unsigned int o=0, endo=h->tessellation().size(); o<endo; ++o)
    {
      WRATHShapeSimpleTessellatorPayload::TessellatedOutline::handle O;
      std::vector<vec2> &current(m_fan_pts[o]);
      int count(0);

      O=h->tessellation()[o];
      current.push_back(vec2(0.0f, 0.0f)); //current[0]=center of fan.

      for(unsigned int e=0, ende=O->edges().size(); e<ende; ++e)
        {
          WRATHShapeSimpleTessellatorPayload::TessellatedEdge::handle E;

          E=O->edges()[e];
          /*
            Note that we also have what should be degenerate
            triangles of the last point of an edge to
            the first of the next edge, that coordinate
            _should_ be the same, but round off error might
            not make that true!
           */
          for(unsigned int i=0, endi=E->curve_points().size(); i<endi; ++i, ++count)
            {
              current.push_back(E->curve_points()[i].position());
              current[0]+=current.back();
            }
        }
      if(count!=0)
        {
          current[0]/=static_cast<float>(count);
          current.push_back(current[1]);
        }
      else
        {
          WRATHassert(current.size()==1);
          current.clear();
        }
    }


}

void
FillRenderer::
draw_to_stencil(const float4x4 &pvm) const
{
  /*
    Observation:

     given a set of outlines, S={O_i}, the winding number
     at a point p for that set of outlines is 

     winding(p,S)= sum_i winding(p,O_i)
 
     where winding(p, O_i) is the winding number of p
     within the single outline O_i.

     For any direction r, that winding number is given by
     the sum of a(r, p, e) over all edges e=[e0,e1] of
     the outline O_i where

     a(r, p, e) is zero if the ray from p in the direction
                r does not intersect e.
   
                is sign( "r cross (e1-e0)" if the ray does 
                intersect.
     
     Let c=center of outline O_i. We let r=p-c,
     and then 

     a(r, p, e) is zero if p is not within the triangle
                [c,e0,e1]

                otherwise if -1 if [c,e0,e1] is CW
                and 1 if it is CCW.
         

     Thus we can use the stencil buffer as follows:
     1) set the stencil test to always pass
     2) set the stencil to increment (with wrapping)
        for clockwise triangles
     3) set the stencil to decerement (with wrapping)
        for counterclockwise triangles

     For each outline, draw a triangle fan.

     If the complexity of the shape is no more
     than the starting stencil value, then
     the ending_stencil_value - starting_stencil_value
     is the winding numbers.
   */


  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glStencilMask(~0);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_STENCIL_TEST);
  glStencilFuncSeparate(GL_FRONT, GL_ALWAYS, 0, ~0);
  glStencilFuncSeparate(GL_BACK, GL_ALWAYS, 0, ~0);
  glStencilOpSeparate(GL_FRONT, GL_INCR_WRAP, GL_INCR_WRAP, GL_INCR_WRAP);
  glStencilOpSeparate(GL_BACK, GL_DECR_WRAP, GL_DECR_WRAP, GL_DECR_WRAP);
  draw_fans(pvm);
}

void
FillRenderer::
draw_fans(const float4x4 &pvm) const
{
  simple_drawer()->bind_and_set_uniforms(pvm);

  for(std::vector< std::vector<vec2> >::const_iterator
        iter=m_fan_pts.begin(), end=m_fan_pts.end();
      iter!=end; ++iter)
    {

      if(!iter->empty())
        {
          glVertexAttribPointer(0, //index
                                2, //number of coordinates
                                GL_FLOAT, //floats
                                GL_FALSE, //ignored
                                sizeof(vec2), //stride
                                &iter->operator[](0));
          
          glDrawArrays(GL_TRIANGLE_FAN, 0, iter->size());
        }
    }
}

////////////////////////////////
// EdgeRects methods
EdgeRects::
EdgeRects(const WRATHShapeSimpleTessellatorPayload::handle &h, 
          float geometry_dist)
{
  
  for(unsigned int o=0, endo=h->tessellation().size(); o<endo; ++o)
    {
      WRATHShapeSimpleTessellatorPayload::TessellatedOutline::handle O;

      O=h->tessellation()[o];
      for(unsigned int e=0, ende=O->edges().size(); e<ende; ++e)
        {
          WRATHShapeSimpleTessellatorPayload::TessellatedEdge::handle E;

          E=O->edges()[e];
          for(unsigned int i=0, endi=E->curve_points().size(); i<endi; ++i)
            {
              vec2 pt, n;
              GLushort loc;
              
              loc=m_verts.size();
              pt=E->curve_points()[i].position();
              n=E->curve_points()[i].normal();

              m_verts.push_back(vec3(pt, 0.0f));
              m_verts.push_back(vec3(pt + geometry_dist*n, 1.0));
              m_verts.push_back(vec3(pt - geometry_dist*n, 1.0));

              if(i!=0)
                {
                  GLushort a(loc), b(loc-3);

                  m_inds.push_back(a);
                  m_inds.push_back(b);
                  m_inds.push_back(a+1);
                  
                  m_inds.push_back(b);
                  m_inds.push_back(a+1);
                  m_inds.push_back(b+1);
      
                  m_inds.push_back(a);
                  m_inds.push_back(b);
                  m_inds.push_back(a+2);
                  
                  m_inds.push_back(b);
                  m_inds.push_back(a+2);
                  m_inds.push_back(b+2);
                }
            }
        }
    }
}

void
EdgeRects::
draw(const float4x4 &pvm,
     bool positive_distances) const
{
  /*
    bind the GLSLProgram and
    set shader state to reflect the parameters:
   */
  primitive_drawer()->bind_and_set_uniforms(pvm, positive_distances);
  
  glVertexAttribPointer(0, //index
                        3, //number of coordinates
                        GL_FLOAT, //floats
                        GL_FALSE, //ignored
                        sizeof(vec3), //stride
                        &m_verts[0]);

  glDrawElements(GL_TRIANGLES, m_inds.size(), 
                 GL_UNSIGNED_SHORT, &m_inds[0]);
}

//////////////////////////////////////
//PointRects methods
PointRects::
PointRects(bool draw_pts_as_fans, float geometry_dist,
           const WRATHShapeSimpleTessellatorPayload::handle &h,
           float pixel_dist):
  m_draw_pts_as_fans(draw_pts_as_fans),
  m_sprite_radius(pixel_dist),
  m_src(h)
{
  std::vector<vec2> pts;

  for(unsigned int o=0, endo=h->tessellation().size(); o<endo; ++o)
    {
      WRATHShapeSimpleTessellatorPayload::TessellatedOutline::handle O;
      
      O=h->tessellation()[o];
      for(unsigned int e=0, ende=O->edges().size(); e<ende; ++e)
        {
          WRATHShapeSimpleTessellatorPayload::TessellatedEdge::handle E;
          E=O->edges()[e];
          pts.push_back(E->curve_points().front().position());
        }
    }

  if(m_draw_pts_as_fans)
    {
      build_fans(pts, geometry_dist, pixel_dist);
    }
  else
    {
      build_rects(pts, geometry_dist);
    }
}

void
PointRects::
build_fans(const std::vector<vec2> &pts, 
           float geometry_dist, float pixel_dist)
{
  
  /*
    Tessellate a circle, The real
    descision is to know the radius and to
    essentially make one point per pixel
    on the rectangle that bounds the circle,
    this is kind of brute force-ish, but sighs,
    what else can we do?
  */   
  int number_tris_per_fan;
  std::complex<float> delta_z_theta;
  std::vector< std::complex<float> > z_thetas;
  float theta;
  
  number_tris_per_fan=std::max(4, static_cast<int>(7*pixel_dist));
  theta=2.0f*float(M_PI)/static_cast<float>(number_tris_per_fan);
  
  {
    float imag, real;
    
    real=cosf(theta);
    imag=sinf(theta);
    delta_z_theta=std::complex<float>(real, imag);
  }

  /*
    z_theta[i]= geometry_dist*( cos(i*theta), sin(i*theta))
  */
  z_thetas.reserve(number_tris_per_fan);
  z_thetas.push_back(geometry_dist);
  for(int t=1;t<number_tris_per_fan; ++t)
    {
      z_thetas.push_back( z_thetas.back()*delta_z_theta);
    }
  
  for(int i=0, endi=pts.size(); i<endi; ++i)
    {
      if(m_fans.empty() or 
         m_fans.back().m_draw_as_fans_attrs.size()>64000)
        {
          m_fans.push_back(packet_of_fans());
        }
      
      GLushort center_indx;
      
      center_indx=m_fans.back().m_draw_as_fans_attrs.size();
      m_fans.back().m_draw_as_fans_attrs.push_back(vec3( pts[i], 0.0f));
      
      for(int t=0;t<number_tris_per_fan; ++t)
        {
          vec2 p;
          std::complex<float> z(z_thetas[t]);
              
          p=pts[i] + vec2(z.real(), z.imag());
          m_fans.back().m_draw_as_fans_attrs.push_back( vec3(p, 1.0f));
          
          int next_t( (t+1)%number_tris_per_fan);
          m_fans.back().m_draw_as_fans_indices.push_back(center_indx);
          m_fans.back().m_draw_as_fans_indices.push_back(center_indx+1+t);
          m_fans.back().m_draw_as_fans_indices.push_back(center_indx+1+next_t);
        }
    }
  
  if(!m_fans.empty() and m_fans.back().m_draw_as_fans_attrs.empty())
    {
      m_fans.pop_back();
    }
}

void
PointRects::
build_rects(const std::vector<vec2> &pts, float d)
{
  for(int i=0, endi=pts.size(); i<endi; ++i)
    {
      if(m_rects.empty() or 
         m_rects.back().m_draw_as_rects_attrs.size()>64000)
        {
          m_rects.push_back(packet_of_rects());
        }
      
      GLushort center_indx;   
      const vec2 rel_offsets[]=
        {
          vec2(-1.0f,  1.0f),
          vec2( 1.0f,  1.0f),
          vec2( 1.0f, -1.0f),
          vec2(-1.0f, -1.0f)
        };

      center_indx=m_rects.back().m_draw_as_rects_attrs.size();

      for(int a=0;a<4;++a)
        {
          vec2 q;

          q=pts[i]+d*rel_offsets[a];

          vec4 qq(q.x(), q.y(), rel_offsets[a].x(), rel_offsets[a].y());
          m_rects.back().m_draw_as_rects_attrs.push_back(qq);
        }

      m_rects.back().m_draw_as_rects_indices.push_back(center_indx+0);
      m_rects.back().m_draw_as_rects_indices.push_back(center_indx+1);
      m_rects.back().m_draw_as_rects_indices.push_back(center_indx+2);

      m_rects.back().m_draw_as_rects_indices.push_back(center_indx+0);
      m_rects.back().m_draw_as_rects_indices.push_back(center_indx+2);
      m_rects.back().m_draw_as_rects_indices.push_back(center_indx+3);

    }
  
  if(!m_rects.empty() and m_rects.back().m_draw_as_rects_attrs.empty())
    {
      m_rects.pop_back();
    }
}

  
void
PointRects::
draw(const float4x4 &pvm, bool positive_distances) const
{
  if(m_draw_pts_as_fans)
    {
      primitive_drawer()->bind_and_set_uniforms(pvm, positive_distances);
  
      for(std::list<packet_of_fans>::const_iterator iter=m_fans.begin(),
            end=m_fans.end(); iter!=end; ++iter)
        {
          glVertexAttribPointer(0, //index
                                3, //number of coordinates
                                GL_FLOAT, //floats
                                GL_FALSE, //ignored
                                sizeof(vec3), //stride
                                &iter->m_draw_as_fans_attrs[0]);
          
          glDrawElements(GL_TRIANGLES, iter->m_draw_as_fans_indices.size(), 
                         GL_UNSIGNED_SHORT, &iter->m_draw_as_fans_indices[0]);
        }
    }
  else
    {
      point_drawer()->bind_and_set_uniforms(pvm, positive_distances);

      for(std::list<packet_of_rects>::const_iterator iter=m_rects.begin(),
            end=m_rects.end(); iter!=end; ++iter)
        {
          glVertexAttribPointer(0, //index
                                4, //number of coordinates
                                GL_FLOAT, //floats
                                GL_FALSE, //ignored
                                sizeof(vec2), //stride
                                &iter->m_draw_as_rects_attrs[0]);
          
          glDrawElements(GL_TRIANGLES, iter->m_draw_as_rects_indices.size(), 
                         GL_UNSIGNED_SHORT, &iter->m_draw_as_rects_indices[0]);
        }
    }
}

/////////////////////////////////////
// DistanceRenderer methods
void
DistanceRenderer::
draw(const float4x4 &pvm, const FillRenderer &f, bool use_depth_buffer) const
{
  
  //first in shape distance values using non-zero winding rule:
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_STENCIL_TEST);
  
  //draw whenever the winding rule is non-zero, note the +128
  glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_NOTEQUAL, 128, ~0); 
  glStencilOpSeparate(GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP);

  f.draw_fans(pvm); //draw first to where primitive is value (1,1,1,1)

  //use depth buffer if should:
  if(use_depth_buffer)
    {
      glDepthMask(GL_TRUE);
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS);
    }
  m_edges.draw(pvm, true);

  if(m_points!=NULL)
    {
      m_points->draw(pvm, true);
    }

  //then negative distance values, i.e. those outside of shape:
  glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_EQUAL, 128, ~0); 
  m_edges.draw(pvm, false);

  if(m_points!=NULL)
    {
      m_points->draw(pvm, false);
    }
}



////////////////////////////////
//WRATHShapeGPUDistanceFieldCreator  methods
enum return_code
WRATHShapeGPUDistanceFieldCreator::
generate_distance_field(const WRATHShapeSimpleTessellatorPayload::handle &h,
                        ivec2 dims, float pixel_dist,
                        const ScratchPad::handle &scratch,
                        const DistanceFieldTarget::handle &dest,
                        enum corner_point_handling_type ct)
{
  if(!h.valid()
     or h->tessellation().empty()
     or !scratch.valid()
     or !dest.valid()
     or dims.x()==0 or dims.y()==0)
    {
      /*
        empty WRATHShape, no image to generate.
       */
      return routine_fail;
    }

  
  WRATHBBox<2> bbox;
  bbox=h->bounding_box();

  if(bbox.empty())
    {
      return routine_fail;
    }


  bool nv_framebuffer_fetch_supported;
  bool write_frag_depth_supported;
  bool need_depth_buffer;
  bool can_use_point_sprites;
  WRATHGLStateStack GLstate_magic;


  #if defined(WRATH_GL_VERSION)
    {
      nv_framebuffer_fetch_supported=false;
      write_frag_depth_supported=true;
    }
  #else
    {
      WRATHGLExtensionList R;
      /*
        The GL_OES_frag_depth and GL_ARB_frag_depth
        actually do NOT exist, but it would not be a 
        surprise if a GLES2 implementation gave that
        string when it should have given GL_EXT_frag_depth.
      */
      write_frag_depth_supported=
        R.extension_supported("GL_EXT_frag_depth")
        or R.extension_supported("GL_OES_frag_depth")
        or R.extension_supported("GL_ARB_frag_depth");

      nv_framebuffer_fetch_supported=
        R.extension_supported("GL_NV_shader_framebuffer_fetch");
    }
  #endif

  /*
    TODO: tweak shaders to use GL_NV_shader_framebuffer_fetch
    to skip using depth buffer..
   */
  need_depth_buffer=true;

  can_use_point_sprites=nv_framebuffer_fetch_supported 
    or write_frag_depth_supported;


  GLstate_magic.push(WRATHGLStateStack::rendering_target_bit);
  if(routine_fail==scratch->init_and_bind_fbo(dims, need_depth_buffer))
    {
      return routine_fail;
    }
  
  GLstate_magic.push(WRATHGLStateStack::color_buffer_bit
                     |WRATHGLStateStack::depth_buffer_bit
                     |WRATHGLStateStack::stencil_buffer_bit
                     |WRATHGLStateStack::rendering_action_bit);
  
  /*
    Basic idea: 
    0) bind the scratches FBO    
    1) render to stencil buffer the winding and or even-odd number
    2) draw unsigned distance values with stencil test to pass
       only if in shape, these values will be normalized to [0.5,1.0]
    3) draw unsigned distance values with stencil test to pass
       only if not in shape, these values will be normalized to [0, 0.5]
    4) Let dest "get" the values from scratch.
   */
  
   
  /*
    compute pvm. The pvm should be so that
    it maps the bounding box bbox to [-1,1]x[1,1]
    and be an orthogonal projection too
  */
  float4x4 pvm;
  float_orthogonal_projection_params F(bbox.min_corner().x(),
                                       bbox.max_corner().x(),
                                       bbox.min_corner().y(),
                                       bbox.max_corner().y(),
                                       -1.0f,
                                       1.0f);
  pvm.orthogonal_projection_matrix(F);


  float geometry_dist;
  vec2 rel_bounds;

  rel_bounds=0.5*( bbox.max_corner() - bbox.min_corner() )/vec2(dims.x(), dims.y());

  geometry_dist=std::max(rel_bounds.x(), rel_bounds.y())*pixel_dist;

  /*
    common attribute and element array state for all drawing.
   */
  glEnableVertexAttribArray(0);
  for(int i=1, endi=WRATHglGet<GLint>(GL_MAX_VERTEX_ATTRIBS); i<endi; ++i)
    {
      glDisableVertexAttribArray(i);
    }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  //note that 128 for the stencil clear value
  glClearStencil(128);  
  glStencilMask(~0);
  glEnable(GL_STENCIL_TEST);

  glDisable(GL_BLEND);
  glDisable(GL_CULL_FACE);

  if(need_depth_buffer)
    {
      #if defined(WRATH_GL_VERSION)
        glDepthRange(0.0, 1.0);
        glClearDepth(1.0);
      #else
        glDepthRangef(0.0f, 1.0f);
        glClearDepthf(1.0f);
      #endif
      
      glDepthMask(GL_TRUE);
      glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    }
  else
    {
      glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    }

  /*
    Render to the stencil buffer the fill rule:
   */
  FillRenderer fill_rule_renderer(h);

  fill_rule_renderer.draw_to_stencil(pvm);

  /*
    Render distance values
   */
  

  DistanceRenderer distance_renderer(can_use_point_sprites and ct==use_point_sprites,
                                     ct!=skip_points,
                                     geometry_dist, h, pixel_dist);
  
  
  distance_renderer.draw(pvm, fill_rule_renderer, need_depth_buffer);

 
  /*
    now copy (or fake copy) the contents
    of the current FBO to dest:
   */
  return dest->copy_results(scratch);
                                   
}
