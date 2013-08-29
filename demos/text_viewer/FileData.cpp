/*! 
 * \file FileData.cpp
 * \brief file FileData.cpp
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
#include "FileData.hpp"
#include "FilePacket.hpp"
#include "WRATHDefaultRectAttributePacker.hpp"
#include "WRATHLayerItemDrawerFactory.hpp"
#include "WRATHLayerItemWidgets.hpp"
#include "NodePacker.hpp"

namespace
{
  template<typename T>
  class NodeWithColor:public T
  {
  public:
    enum
      {
        base_number_per_node_values=T::number_per_node_values,
        number_per_node_values=4+base_number_per_node_values,
      };

    typedef WRATHLayerItemDrawerFactory<NodeWithColor, NodePacker> Factory;

    class Functions:public WRATHLayerItemNodeBase::node_function_packet
    {
    public:
      virtual
      WRATHLayerItemNodeBase*
      create_completely_clipped_node(const WRATHTripleBufferEnabler::handle &tr) const
      {
        return T::functions().create_completely_clipped_node(tr);
      }
      
      virtual
      void
      append_shader_source(std::map<unsigned int, WRATHGLShader::shader_source> &src,
                           const WRATHLayerNodeValuePackerBase::function_packet &available) const
      {
        T::functions().append_shader_source(src, available);
      }
      
      virtual
      void
      add_per_node_values(WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                            const WRATHLayerNodeValuePackerBase::function_packet &available) const
      {
        T::functions().add_per_node_values(spec, available);
        spec
          .add_source(T::number_per_node_values+0, "color_red", GL_VERTEX_SHADER)
          .add_source(T::number_per_node_values+1, "color_green", GL_VERTEX_SHADER)
          .add_source(T::number_per_node_values+2, "color_blue", GL_VERTEX_SHADER)
          .add_source(T::number_per_node_values+3, "color_alpha", GL_VERTEX_SHADER);
      }
    };

    template<typename S>
    NodeWithColor(S *tr):
      T(tr),
      m_color(1.0f, 0.0f, 1.0f, 1.0f)
    {}

    virtual
    const WRATHLayerItemNodeBase::node_function_packet&
    node_functions(void) const
    {
      return functions();
    }

    static
    const WRATHLayerItemNodeBase::node_function_packet&
    functions(void)
    {
      static Functions R;
      return R;
    }

    virtual
    void
    extract_values(reorder_c_array<float> out_value)
    {
      reorder_c_array<float> sub_array(out_value.sub_array(0, base_number_per_node_values));
      
      T::extract_values(sub_array);
      out_value[base_number_per_node_values+0]=m_color.x();
      out_value[base_number_per_node_values+1]=m_color.y();
      out_value[base_number_per_node_values+2]=m_color.z();
      out_value[base_number_per_node_values+3]=m_color.w();
    }
    vec4 m_color;
  };

  template<typename T>
  class NodeWithImage:public T
  {
  public:
    enum
      {
        base_number_per_node_values=T::number_per_node_values,
        number_per_node_values=4+base_number_per_node_values,
      };

    typedef WRATHLayerItemDrawerFactory<NodeWithImage, NodePacker> Factory;

    class Functions:public WRATHLayerItemNodeBase::node_function_packet
    {
    public:
      virtual
      WRATHLayerItemNodeBase*
      create_completely_clipped_node(const WRATHTripleBufferEnabler::handle &tr) const
      {
        return T::functions().create_completely_clipped_node(tr);
      }
      
      virtual
      void
      append_shader_source(std::map<unsigned int, WRATHGLShader::shader_source> &src,
                           const WRATHLayerNodeValuePackerBase::function_packet &available) const
      {
        T::functions().append_shader_source(src, available);
      }
      
      virtual
      void
      add_per_node_values(WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                            const WRATHLayerNodeValuePackerBase::function_packet &available) const
      {
        T::functions().add_per_node_values(spec, available);
        spec
          .add_source(T::number_per_node_values+0, "tex_x", GL_VERTEX_SHADER)
          .add_source(T::number_per_node_values+1, "tex_y", GL_VERTEX_SHADER)
          .add_source(T::number_per_node_values+2, "tex_w", GL_VERTEX_SHADER)
          .add_source(T::number_per_node_values+3, "tex_h", GL_VERTEX_SHADER);
      }
    };

    template<typename S>
    NodeWithImage(S *tr):
      T(tr),
      m_tex_xy(0.0f, 0.0f),
      m_tex_wh(1.0f, 1.0f)
    {}

    virtual
    const WRATHLayerItemNodeBase::node_function_packet&
    node_functions(void) const
    {
      return functions();
    }

    static
    const WRATHLayerItemNodeBase::node_function_packet&
    functions(void)
    {
      static Functions R;
      return R;
    }

    virtual
    void
    extract_values(reorder_c_array<float> out_value)
    {
      reorder_c_array<float> sub_array(out_value.sub_array(0, base_number_per_node_values));
      
      T::extract_values(sub_array);
      out_value[base_number_per_node_values+0]=m_tex_xy.x();
      out_value[base_number_per_node_values+1]=m_tex_xy.y();
      out_value[base_number_per_node_values+2]=m_tex_wh.x();
      out_value[base_number_per_node_values+3]=m_tex_wh.y();
    }

    void
    set(WRATHImage *im)
    {
      m_tex_xy=im->minX_minY_texture_coordinate();
      m_tex_wh=im->maxX_maxY_texture_coordinate() - m_tex_xy;
    }

    vec2 m_tex_xy, m_tex_wh;
  };

  typedef NodeWithColor<WRATHLayerItemNodeRotateTranslate> ShapeNode;
  typedef NodeWithImage<NodeWithColor<WRATHLayerItemNodeRotateTranslate> > ImageNode;
}


FileData::
FileData(FilePacket *pparent, const std::string &pfilename,
         file_fetch_type tp):
  m_filename(pfilename),
  m_file_type(tp),
  m_parent(pparent),
  m_container(NULL),
  m_tr(NULL),
  m_number_chars(0),
  m_number_streams(0)
{
}

FileData::
~FileData()
{
  for(std::vector<TextChunk*>::iterator 
        iter=m_text_chunks.begin(), end=m_text_chunks.end();
      iter!=end; ++iter)
    {
      WRATHPhasedDelete(*iter);
    }

  for(std::vector<WRATHRectItem*>::iterator 
        iter=m_images.begin(), end=m_images.end();
      iter!=end; ++iter)
    {
      WRATHPhasedDelete(*iter);
    }

   for(std::vector<WRATHShapeItem*>::iterator 
        iter=m_shapes.begin(), end=m_shapes.end();
      iter!=end; ++iter)
    {
      WRATHPhasedDelete(*iter);
    }


  if(m_container!=NULL)
    {
      WRATHPhasedDelete(m_container);
    }
}

void
FileData::
load_file(void)
{
  if(m_container!=NULL)
    {
      WRATHassert(m_tr!=NULL);
      return;
    }

  WRATHassert(m_tr==NULL);

  m_container=WRATHNew WRATHLayer(&m_parent->root_container());
  m_container->simulation_matrix(WRATHLayer::modelview_matrix, float4x4());
  m_container->simulation_composition_mode(WRATHLayer::modelview_matrix, WRATHLayer::compose_matrix);
  m_container->simulation_matrix(WRATHLayer::projection_matrix, float4x4());
  m_container->simulation_composition_mode(WRATHLayer::projection_matrix, WRATHLayer::compose_matrix);
  m_container->visible(false);

  m_tr=WRATHNew WRATHLayerItemNodeRotateTranslate(m_container->root_node<WRATHLayerItemNodeRotateTranslate>());

  //NOTE: this operation can be threaded:
  m_parent->load_file(m_filename, this, m_file_type);
}

void
FileData::
reload_file(void)
{
  if(m_container==NULL)
    {
      load_file();
      return;
    }

  m_bbox.clear();

  for(std::vector<TextChunk*>::iterator 
        iter=m_text_chunks.begin(), end=m_text_chunks.end();
      iter!=end; ++iter)
    {
      WRATHPhasedDelete(*iter);
    }

  for(std::vector<WRATHRectItem*>::iterator 
        iter=m_images.begin(), end=m_images.end();
      iter!=end; ++iter)
    {
      WRATHPhasedDelete(*iter);
    }

   for(std::vector<WRATHShapeItem*>::iterator 
        iter=m_shapes.begin(), end=m_shapes.end();
      iter!=end; ++iter)
    {
      WRATHPhasedDelete(*iter);
    }

  m_text_chunks.clear();
  m_images.clear();
  m_links.clear();
  m_shapes.clear();

  m_number_streams=0;
  m_number_chars=0;

  m_parent->load_file(m_filename, this, m_file_type);
  
}



void
FileData::
add_text(const WRATHFormattedTextStream &ptext, 
         const WRATHStateStream &pstate_stream)
{
  int total_size, I, ch_size;

  total_size=ptext.data_stream().size();
  ch_size=m_parent->text_chunk_size();

  for(I=0; I<total_size; I+=ch_size) 
    {
      TextChunk *ptr;
      int end_index;

      end_index=std::min(total_size, I+ch_size);
      ptr=WRATHNew TextChunk(range_type<int>(I,end_index),
                           ptext, pstate_stream,
                           m_container, m_parent, m_tr);
      m_text_chunks.push_back(ptr);

      m_bbox.set_or(ptr->bbox());
    }
  m_number_chars+=total_size;
  ++m_number_streams;
}  

void
FileData::
update_culling(const ivec2 &window_size, bool disable_culling)
{
  WRATH2DRigidTransformation R;
  WRATHTextAttributePacker::BBox window_box;

  load_file();

  R=m_tr->global_values().m_transformation.inverse();
    
  window_box.set_or(R.apply_to_point(vec2(0,0)));
  window_box.set_or(R.apply_to_point(vec2(0, window_size.y())));
  window_box.set_or(R.apply_to_point(vec2(window_size.x(), 0)));
  window_box.set_or(R.apply_to_point(vec2(window_size.x(), window_size.y())));
  for(std::vector<TextChunk*>::iterator 
        iter=m_text_chunks.begin(), 
        end=m_text_chunks.end();
      iter!=end; ++iter)
    {
      TextChunk *ptr(*iter);
      ptr->visible(disable_culling or window_box.intersects(ptr->bbox()));
    }
}

const FileData::LinkAtResult*
FileData::
link_at(int x, int y)
{
  vec2 pos(x, y);

  pos=m_tr->global_values().m_transformation.inverse().apply_to_point(pos);

  //TODO: quad tree search of link:  
  for(std::vector<PerLink>::iterator iter=m_links.begin(),
        end=m_links.end(); iter!=end; ++iter)
    {
      if(iter->m_bbox.intersects(pos))
        {
          return &iter->m_file;
        }
    }

  return NULL;
}


void
FileData::
add_jump_tag(const std::string &tag_name, 
             const vec2 &plocation)
{
  m_jump_tags[tag_name]=plocation;
}

std::pair<bool, vec2>
FileData::
jump_tag(const std::string &ptag_name)
{
  std::map<std::string, vec2>::const_iterator iter;
  std::pair<bool, vec2> return_value(false, vec2());

  iter=m_jump_tags.find(ptag_name);
  if(iter!=m_jump_tags.end())
    {
      return_value.first=true;
      return_value.second=iter->second;
    }
  return return_value;
}


void
FileData::
add_link(FileData *pfile,
         const WRATHTextAttributePacker::BBox &bbox,
         const std::pair<bool, std::string> &jump_location)
{
  //TODO: create quad search tree for fast
  //intersection test:
  m_links.push_back(PerLink(LinkAtResult(pfile, jump_location), bbox));
}

void
FileData::
add_quit_link(const WRATHTextAttributePacker::BBox &bbox)
{
  //TODO: create quad search tree for fast
  //intersection test:
  m_links.push_back(PerLink(LinkAtResult(), bbox));
}




void
FileData::
add_shape(WRATHShapeF *shape,
          WRATHShapeAttributePackerF *packer,
          WRATHShapeProcessorPayload payload,
          const WRATHShapeAttributePackerBase::PackingParametersBase &additional_packing_params,
          WRATHShaderSpecifier *shader,
          const WRATHSubItemDrawState &extra_state,
          const vec2 &pos, const vec4 &color,
          WRATHBBox<2> shapebounds)
{
  ShapeNode *new_node;
  WRATHShapeItem *shape_item;
  WRATHShapeItemTypes::ShapeDrawerF drawer(shader, packer);

  new_node=WRATHNew ShapeNode(m_tr);
  new_node->translation(pos);
  new_node->m_color=color;
  new_node->visible(true);

  drawer.m_draw_passes[0].m_draw_state=extra_state;
  drawer.m_draw_passes[0].m_draw_type=WRATHDrawType::transparent_pass();


  shape_item=WRATHNew WRATHShapeItem(ShapeNode::Factory(), 
                                     0,
                                     m_container, 
                                     WRATHLayer::SubKey(new_node),
                                     WRATHShapeItemTypes::shape_value(*shape, payload),
                                     drawer,
                                     additional_packing_params);
  m_shapes.push_back(shape_item);

  shapebounds.translate(pos);
  m_bbox.set_or(shapebounds);
}

void
FileData::
add_image(WRATHImage *im, 
          WRATHShaderSpecifier *image_spec_drawer,
          const WRATHSubItemDrawState &image_extra_state,
          vec2 bl, vec2 tr, vec4 color)
{
  WRATHRectItem *ptr;
  ImageNode *tr_node;
  WRATHRectItem::Drawer image_drawer(image_spec_drawer);

  tr_node=WRATHNew ImageNode(m_tr);
  tr_node->translation(bl);
  tr_node->m_color=color;
  tr_node->set(im);

  image_drawer.m_draw_passes[0].m_draw_state
    .absorb(image_extra_state)
    .add_texture(GL_TEXTURE0, im->texture_binder(0));
  image_drawer.m_draw_passes[0].m_draw_type=WRATHDrawType::transparent_pass();

  ptr=WRATHNew WRATHRectItem(ImageNode::Factory(), 0,
                             m_container, 
                             WRATHLayer::SubKey(tr_node),
                             image_drawer);

  m_bbox.set_or(bl);
  m_bbox.set_or(tr);
  ptr->set_parameters(WRATHNew WRATHDefaultRectAttributePacker::Rect(tr-bl));
  m_images.push_back(ptr);

}

 
