/*! 
 * \file FileData.hpp
 * \brief file FileData.hpp
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


#ifndef FILE_DATA_HPP
#define FILE_DATA_HPP

#include "WRATHConfig.hpp"
#include <string>
#include "WRATHTextDataStream.hpp"
#include "TextChunk.hpp"
#include "vectorGL.hpp"
#include "WRATHRectItem.hpp"
#include "FileType.hpp"
#include "WRATHShapeItem.hpp"
#include "WRATHBBox.hpp"
#include "WRATHShapeAttributePacker.hpp"
#include "WRATHShaderSpecifier.hpp"
#include "WRATHLayer.hpp"
#include "WRATHLayerItemNodeRotateTranslate.hpp"

class FilePacket;
class FileData
{
public:
  typedef FileType::file_fetch_type file_fetch_type;

  class LinkAtResult
  {
  public:    
    /*!
      What file the link is to,
      NULL indicates a "back" link,
      i.e. display previous page.
     */
    FileData *m_link_file;

    /*!
      if .first is true, then .second
      is the name of the tag to jump to.
     */
    std::pair<bool, std::string> m_jump_tag;

    /*!
      If true, means that the link is a 
      QUIT link and on hit should terminate.
     */
    bool m_is_quit_link;

  private:
    friend class FileData;

    LinkAtResult(FileData *pfile, 
                 const std::pair<bool,std::string> &pjump_tag):
      m_link_file(pfile), m_jump_tag(pjump_tag),
      m_is_quit_link(false)
    {}

    LinkAtResult(void):
      m_is_quit_link(true)
    {}

  }; 

  FileData(FilePacket *pparent, 
           const std::string &pfilename,
           file_fetch_type pfile_type);

  ~FileData();

  const std::string&
  source_file(void)
  {
    return m_filename;
  }
  
  WRATHLayer&
  container(void)
  {
    load_file();
    WRATHassert(m_container!=NULL);
    return *m_container;
  }

  WRATHLayerItemNodeRotateTranslate&
  transformation_node(void)
  {
    load_file();
    WRATHassert(m_tr!=NULL);
    return *m_tr;
  }

  void
  update_culling(const ivec2 &window_sz,
                 bool disable_culling);

  int
  number_chars(void)
  {
    return m_number_chars;
  }

  int
  number_streams(void)
  {
    return m_number_streams;
  }

  int
  number_chunks(void)
  {
    return m_text_chunks.size();
  }

  const LinkAtResult*
  link_at(int x, int y);
  
  std::pair<bool, vec2>
  jump_tag(const std::string &ptag_name);

  /*!
    if pfile is NULL, then link
    is a "back link" 
   */
  void
  add_link(FileData *pfile,
           const WRATHTextAttributePacker::BBox &bbox,
           const std::pair<bool,std::string> &jump_location);

  /*!
    Add a quit link
   */
  void
  add_quit_link(const WRATHTextAttributePacker::BBox &bbox);
  
  /*!
    Add a jump "tag" at the position.
   */
  void
  add_jump_tag(const std::string &tag_name, 
               const vec2 &plocation); 
      
  void
  add_image(WRATHImage *im, 
            WRATHShaderSpecifier *image_drawer,
            const WRATHSubItemDrawState &image_extra_state,
            vec2 bl, vec2 tr, vec4 color);

  void
  add_shape(WRATHShapeF *shape,
            WRATHShapeAttributePackerF *packer,
            WRATHShapeProcessorPayload payload,
            const WRATHShapeAttributePackerBase::PackingParametersBase &additional_packing_params,
            WRATHShaderSpecifier *drawer,
            const WRATHSubItemDrawState &extra_state,
            const vec2 &pos, const vec4 &color,
            WRATHBBox<2> shapebounds);


  

  void
  reload_file(void);

  void
  add_text(const WRATHFormattedTextStream &ptext, 
           const WRATHStateStream &pstate_stream);

  const std::string&
  filename(void) const
  {
    return m_filename;
  }

  
  const vec4&
  background_color(void) const
  {
    return m_background_color;
  }

  void
  background_color(const vec4 &v) 
  {
    m_background_color=v;
  }

  bool
  file_loaded(void)
  {
    return m_container!=NULL;
  }

  const WRATHTextAttributePacker::BBox&
  bbox(void)
  {
    return m_bbox;
  }

private:

  void
  load_file(void);

  class PerLink
  {
  public:
    PerLink(const LinkAtResult &L, 
            const WRATHTextAttributePacker::BBox &b):
      m_file(L),
      m_bbox(b)
    {}

    LinkAtResult m_file;
    WRATHTextAttributePacker::BBox m_bbox;
  };


  std::string m_filename;
  file_fetch_type m_file_type;
  vec4 m_background_color;

  FilePacket *m_parent;
  WRATHLayer *m_container;
  WRATHLayerItemNodeRotateTranslate *m_tr;
  std::vector<TextChunk*> m_text_chunks;
  WRATHTextAttributePacker::BBox m_bbox;
  int m_number_chars, m_number_streams;

  std::vector<PerLink> m_links;
  std::vector<WRATHRectItem*> m_images;
  std::vector<WRATHShapeItem*> m_shapes;
  std::map<std::string, vec2> m_jump_tags;
};


#endif
