/*! 
 * \file FilePacket.hpp
 * \brief file FilePacket.hpp
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


#ifndef __FILE_PACKET_HPP__
#define __FILE_PACKET_HPP__

#include "WRATHConfig.hpp"
#include <vector>
#include <string>
#include <map>
#include <dirent.h>
#include <boost/utility.hpp>

#ifdef WRATH_QT
#include <QFont>
#endif

#include "generic_command_line.hpp"
#include "WRATHTextDataStream.hpp"
#include "WRATHLayer.hpp"
#include "WRATHLayerItemNodeRotateTranslate.hpp"
#include "WRATHTextureFontDrawer.hpp"
#include "WRATHTextItem.hpp"
#include "WRATHImage.hpp"
#include "TextChunk.hpp"
#include "FileType.hpp"
#include "WRATHShapeAttributePacker.hpp"
#include "WRATHShapeDistanceFieldGPU.hpp"
#include "WRATHShapeSimpleTessellator.hpp"
#include "WRATHShapePreStroker.hpp"
#include "WRATHShapeTriangulator.hpp"
#include "WRATHShaderSpecifier.hpp"

class FileData;
class FilePacket:boost::noncopyable
{
public:

  typedef FileType::file_fetch_type file_fetch_type;

  class misc_drawers_type
  {
  public:
    misc_drawers_type(void):
      m_line_drawer(NULL),
      m_image_drawer(NULL),
      m_stroked_shape_drawer(NULL),
      m_distance_field_drawer(NULL),
      m_filled_shape_drawer(NULL)
    {}
   

    WRATHShaderSpecifier *m_line_drawer;
    WRATHShaderSpecifier *m_image_drawer;
    WRATHShaderSpecifier *m_stroked_shape_drawer;
    WRATHShaderSpecifier *m_distance_field_drawer;
    WRATHShaderSpecifier *m_filled_shape_drawer;
  };

  class ExtraDrawState
  {
  public:
    WRATHTextItem::ExtraDrawState m_text_extra_state;
    WRATHSubItemDrawState m_line_extra_state;
    WRATHSubItemDrawState m_image_extra_state;
    WRATHSubItemDrawState m_stroked_shape_extra_state;
    WRATHSubItemDrawState m_distance_field_extra_state;
    WRATHSubItemDrawState m_filled_shape_extra_state;
  };

  class Loader
  {
  public:
    typedef WRATHTextureFont* (*fetch_font)(int psize, 
                                            const std::string &pfilename, 
                                            int face_index);

    
    #ifdef WRATH_QT
      typedef WRATHTextureFont* (*fetch_qt_font)(const QFont &fnt, int pixel_height);
      fetch_qt_font m_font_via_qt;
    #endif

    
    fetch_font m_font_via_resource;
    
    Loader(void):
    #ifdef WRATH_QT
      m_font_via_qt(NULL),
    #endif
      m_font_via_resource(NULL)
    {}
  };

  FilePacket(WRATHLayer *proot_container,
             WRATHTextItem::Drawer pfont_drawer,
             misc_drawers_type pmisc_drawers,
             float default_pt_size, 
             WRATHTextureFont *default_font,
             vecN<GLubyte,4> default_text_color,
             const vec4 &pdefault_background_color,
             int chunk_size, bool lazyz,
             const ExtraDrawState &pextra_state,
             Loader pfont_fetcher,
             bool generate_font_threaded_on_load,
             bool manual_mipmap_generation);

  ~FilePacket();

  WRATHTextItem::Drawer
  texture_font_drawer(void)
  {
    return m_font_drawer;
  }
  
  int
  text_chunk_size(void)
  {
    return m_chunk_size;
  }

  WRATHLayer&
  root_container(void)
  {
    return *m_root_container;
  }

  const misc_drawers_type&
  misc_drawers(void)
  {
    return m_misc_drawers;
  }

  const ExtraDrawState&
  extra_state(void)
  {
    return m_extra_state;
  }

  bool
  lazy_z(void)
  {
    return m_lazy_z;
  }

  enum WRATHTextItemTypes::text_opacity_t
  text_item_opacity_type(void)
  {
    return m_lazy_z?
      WRATHTextItemTypes::text_transparent:
      WRATHTextItemTypes::text_opaque;
  }
  

  FileData*
  fetch_file(const std::string &pfilename, 
             file_fetch_type type=FileType::load_interpreted);

  void
  load_file(const std::string &pfilename, 
            FileData *file_data_target, 
            file_fetch_type type);

   
  static
  vecN<GLubyte,4>
  link_color(void)
  {
    return vecN<GLubyte,4>(0x40, 0x99, 0xFF, 0xFF);
  }
  
  bool //returns true if there are fonts being generated.
  update_threaded_font_load_progress(WRATHTextDataStream &ostr);

  GLenum m_minification_image_filter;
  GLenum m_magnification_image_filter;


private:
  class Command;
  class LinkEntry;
  class stack_data;
  class TagEntry;
  class CommandData;

  typedef void (FilePacket::*cmd_fptr)(const Command &cmd, CommandData &cmd_data);
  typedef void (*stream_cmd_fptr)(WRATHTextDataStream&, 
                                  const std::string &original_cmd_string,
                                  const std::vector<std::string> &cmd_string_tokenized);

  typedef std::pair<std::string, cmd_fptr> file_cmd;
  typedef std::pair<std::string, stream_cmd_fptr> stream_cmd;
  typedef std::pair<std::string, file_fetch_type> file_key;
  
  class per_shape_data
  {
  public:
    per_shape_data(WRATHShapeF *pshape, 
                   const WRATHShapeSimpleTessellatorPayload::PayloadParams &tess_params);

    ~per_shape_data(void);

    WRATHShapeSimpleTessellatorPayload::handle
    tessellated_data(void);

    WRATHShapePreStrokerPayload::handle
    pre_stroke_data(void);

    WRATHShapeTriangulatorPayload::handle
    fill_data(void);

    WRATHShapeF*
    shape(void) 
    {
      return m_shape;
    }

  private:
    WRATHShapeF *m_shape;
    WRATHShapeSimpleTessellatorPayload::PayloadParams m_tess_params;
    WRATHShapePreStrokerPayload::PayloadParams m_pre_stroke_parameters;
  };

  class CommandData:boost::noncopyable
  {
  public:
    explicit
    CommandData(FileData *p);
 
    
    CommandData(const vec2 &pos,
                CommandData *parent, 
                float left, 
                float width,
                enum WRATHFormatter::alignment_type palignment,
                bool pbreak_words);

    ~CommandData();

    void
    place_text(void);

    enum
      {
        reset_lining=1,
        copy_stacks=2
      };

    void
    init_stream(void);

    vec2
    new_stream(int flags=reset_lining);

    WRATHTextDataStream&
    current_stream(void) 
    {
      WRATHassert(!m_streams.empty());
      return *m_streams.back();
    }

    const WRATHTextDataStream&
    current_stream(void) const
    {
      WRATHassert(!m_streams.empty());
      return *m_streams.back();
    }

    bool
    circular_inclusion(const std::string &pfile);

    void
    add_shape(const std::string &pname, per_shape_data *pshape);

    void
    add_distance_field(const std::string &pname,
                       WRATHImage *im);

    per_shape_data*
    get_shape(const std::string &pname);

    WRATHImage*
    get_distance_field(const std::string &pname);

    WRATHColumnFormatter::LayoutSpecification m_layout;
    std::set<std::string> m_included_files;
    std::vector<WRATHTextDataStream*> m_streams;
    std::vector<LinkEntry> m_links;
    std::vector<TagEntry> m_tags;
    FileData *m_current;
    float m_left;
    std::pair<bool, float> m_width;
    CommandData *m_parent, *m_root;
    std::list<CommandData*> m_children;
    std::vector<stack_data> m_current_location;
    bool m_is_spill;
    
    WRATHShapeSimpleTessellatorPayload::PayloadParams m_tess_params;

  private:
    std::map<std::string, per_shape_data*> m_shapes;
    std::map<std::string, WRATHImage*> m_shape_distance_images;
  };


  class Command
  {
  public:

    Command(void):
      m_values(1)
    {}

    const std::string&
    command(void) const
    {
      return m_values[0];
    }

    std::string&
    command(void)
    {
      return m_values[0];
    }

    const std::string&
    argument(int i) const
    {
      return m_values[i+1];
    }

    std::string&
    argument(int i) 
    {
      return m_values[i+1];
    }

    void
    add_argument(const std::string &v)
    {
      m_values.push_back(v);
    }

    int
    number_arguments(void) const
    {
      return m_values.size() - 1;
    }

    void
    number_arguments(int i)
    {
      m_values.resize(i+1);
    }

    void
    parse_arguments(command_line_register &parser) const
    {
      parser.parse_command_line(m_values);
    }

    void
    clear(void)
    {
      m_values.resize(1);
      m_values[0].clear();
    }

    bool
    arguments_empty(void) const
    {
      return m_values.size()<=1;
    }

    void
    original_string(const std::string &v)
    {
      m_original_string=v;
    }
    
    const std::string&
    original_string(void) const
    {
      return m_original_string;
    }

    const std::vector<std::string>&
    string_tokenized(void) const
    {
      return m_values;
    }

  private:
    std::string m_original_string;
    std::vector<std::string> m_values;
  };

  class TagEntry
  {
  public:
    TagEntry(void):
      m_tag_name(),
      m_location(0),
      m_stream(NULL),
      m_fallback_position(0.0f, 0.0f)
    {}

    TagEntry&
    tag_name(const std::string &v)
    {
      m_tag_name=v;
      return *this;
    }

    TagEntry&
    location(int &v)
    {
      m_location=v;
      return *this;
    }

    TagEntry&
    stream(const WRATHTextDataStream &v)
    {
      m_stream=&v;
      return *this;
    }

    TagEntry&
    fallback_position(const vec2 &v)
    {
      m_fallback_position=v;
      return *this;
    }

    std::string m_tag_name;
    int m_location;
    const WRATHTextDataStream *m_stream;
    vec2 m_fallback_position;
  };

  class LinkEntry
  {
  public:
    std::string m_filename;
    file_fetch_type m_type;
    std::pair<bool, std::string> m_tag;
    range_type<int> m_range;
    const WRATHTextDataStream *m_stream;
    bool m_is_quit_link;

    LinkEntry(void):
      m_type(FileType::load_interpreted),
      m_range(0,0),
      m_stream(NULL),
      m_is_quit_link(false)
    {}

    LinkEntry&
    is_quit_link(bool v)
    {
      m_is_quit_link=v;
      return *this;
    }

    LinkEntry&
    tag_name(const std::string &v)
    {
      m_tag.second=v;
      m_tag.first=true;
      return *this;
    }

    LinkEntry&
    filename(const std::string &v)
    {
      m_filename=v;
      return *this;
    }

    LinkEntry&
    range(const range_type<int> &v)
    {
      m_range=v;
      return *this;
    }

    LinkEntry&
    range(int a, int b)
    {
      m_range.m_begin=a;
      m_range.m_end=b;
      return *this;
    }

    LinkEntry&
    stream(const WRATHTextDataStream &str)
    {
      m_stream=&str;
      return *this;
    }

    LinkEntry&
    type(file_fetch_type v)
    {
      m_type=v;
      return *this;
    }
  };

  class push_default_state
  {
  public:
    FilePacket *m_this;

    explicit
    push_default_state(FilePacket *p):
      m_this(p)
    {}
  };

  class pop_default_state
  {
  public:

    explicit
    pop_default_state(void)
    {}
  };

  class stack_data
  {
  public:
    stack_data(void):
      m_line(1),
      m_file_with_path("stdin"),
      m_file_without_path("stdin"),
      m_file_path(""),
      m_actual_file(false)
    {}
    
    stack_data(const stack_data &parent, const std::string &pfilename);

    int m_line;
    std::string m_file_with_path;
    std::string m_file_without_path;
    std::string m_file_path;
    bool m_actual_file;
  };
  
  class font_glyph_generator:boost::noncopyable
  {
  public:
    
    ~font_glyph_generator();

    static
    font_glyph_generator*
    create(FilePacket*, WRATHTextureFont*);

    WRATHThreadID
    thread_id(void);

    bool
    complete(float &percentage_done);

    void
    abort(void);

    const std::string&
    label(void)
    {
      return m_label;
    }

  private:
    font_glyph_generator(FilePacket*, WRATHTextureFont*);

    

    static
    void*
    thread_function(void*);

    WRATHMutex m_mutex;
    WRATHTextureFont *m_font;
    FilePacket *m_parent;
    int m_current;
    bool m_abort, m_done;
    WRATHThreadID m_thread_id;
    std::string m_label;
  };


  template<typename T>
  friend 
  WRATHTextDataStream::stream_type<T>
  operator<<(WRATHTextDataStream::stream_type<T> str,
             const stack_data &obj)
  {
    if(obj.m_actual_file)
      {
        str << "(" << obj.m_line
            << ", " << obj.m_file_without_path
            << ")";
      }
    else
      {
        str << "NULL-file";
      }
    return str;
  }
  
  template<typename T>
  friend
  WRATHTextDataStream::stream_type<T>
  operator<<(WRATHTextDataStream::stream_type<T> str, push_default_state obj)
  {
    str << WRATHText::push_pixel_size(obj.m_this->m_default_pt_size)
        << WRATHText::push_scale(1.0f)
        << WRATHText::push_font(obj.m_this->m_default_font)
        << WRATHText::push_color(obj.m_this->m_default_color)
        << WRATHText::push_state<line_stream_type>(false, underline_stream_id)
        << WRATHText::push_state<line_stream_type>(false, strikethrough_stream_id);
    return str;
  }

  template<typename T>
  friend
  WRATHTextDataStream::stream_type<T>
  operator<<(WRATHTextDataStream::stream_type<T> str, pop_default_state)
  {
    str << WRATHText::pop_state<line_stream_type>(strikethrough_stream_id)
        << WRATHText::pop_state<line_stream_type>(underline_stream_id)
        << WRATHText::pop_color()
        << WRATHText::pop_font()
        << WRATHText::pop_pixel_size()
        << WRATHText::pop_scale();
    return str;
  }

  

    
  static
  void
  create_command_from_string(const std::string &in_string,
                             Command &out_command);
  
  void
  handle_command(const Command &cmd, 
                 CommandData &cmd_data);

  template<typename iterator>
  void
  include_file(iterator begin, iterator end, 
               bool process_commands, CommandData &cmd_data);

  void
  post_process(const CommandData &cmd_data);

  void
  change_color(const Command &cmd, CommandData &cmd_data, bool push);

  void
  change_font_pixel_size(const Command &cmd, CommandData &cmd_data, bool push);

  void
  change_font_qt(const Command &cmd, CommandData &cmd_data, bool push);

  void
  change_font_file(const Command &cmd, CommandData &cmd_data, bool push);

  void
  change_font(const Command &cmd, CommandData &cmd_data, bool push);

  void
  include_file(const Command &cmd, CommandData &cmd_data)
  {
    include_file_general(cmd, cmd_data, true);
  }

  void
  include_dir(const Command &cmd, CommandData &cmd_data);

  void
  include_utf8(const Command &cmd, CommandData &cmd_data)
  {
    include_utf8_general(cmd, cmd_data, true);
  }

  void
  include_utf16(const Command &cmd, CommandData &cmd_data)
  {
    include_utf16_general(cmd, cmd_data, true);
  }

  void
  include_raw_file(const Command &cmd, CommandData &cmd_data)
  {
    include_file_general(cmd, cmd_data, false);
  }

  void
  include_raw_utf8(const Command &cmd, CommandData &cmd_data)
  {
    include_utf8_general(cmd, cmd_data, false);
  }

  void
  include_raw_utf16(const Command &cmd, CommandData &cmd_data)
  {
    include_utf16_general(cmd, cmd_data, false);
  }

  void
  include_utf8_general(const Command &cmd, CommandData &cmd_data, 
                       bool process_commands);

  void
  include_utf16_general(const Command &cmd, CommandData &cmd_data, 
                        bool process_commands);

  void
  include_file_general(const Command &cmd, CommandData &cmd_data, 
                       bool process_commands);

  void
  add_link(const Command &cmd, CommandData &cmd_data);

  void
  add_named_link(const Command &cmd, CommandData &cmd_data);

  void
  add_back_link(const Command &cmd, CommandData &cmd_data);

  void
  add_quit_link(const Command &cmd, CommandData &cmd_data);

  void
  add_tag(const Command &cmd, CommandData &cmd_data);

  void
  change_color(const Command &cmd, CommandData &cmd_data)
  {
    change_color(cmd, cmd_data, false);
  }

  void
  change_font_pixel_size(const Command &cmd, CommandData &cmd_data)
  {
    change_font_pixel_size(cmd, cmd_data, false);
  }

  void
  change_underlining(const Command &cmd, CommandData &cmd_data)
  {
    change_line_generic(underline_stream_id, cmd, cmd_data, false);
  }

  void
  change_strikethrough(const Command &cmd, CommandData &cmd_data)
  {
    change_line_generic(strikethrough_stream_id, cmd, cmd_data, false);
  }

  void
  change_font_qt(const Command &cmd, CommandData &cmd_data)
  {
    change_font_qt(cmd, cmd_data, false);
  }

  void
  change_font_file(const Command &cmd, CommandData &cmd_data)
  {
    change_font_file(cmd, cmd_data, false);
  }

  void
  change_font(const Command &cmd, CommandData &cmd_data)
  {
    change_font(cmd, cmd_data, false);
  }

  void
  push_color(const Command &cmd, CommandData &cmd_data)
  {
    change_color(cmd, cmd_data, true);
  }

  void
  push_font_pixel_size(const Command &cmd, CommandData &cmd_data)
  {
    change_font_pixel_size(cmd, cmd_data, true);
  }

  void
  push_underlining(const Command &cmd, CommandData &cmd_data)
  {
    change_line_generic(underline_stream_id, cmd, cmd_data, true);
  }

  void
  push_strikethrough(const Command &cmd, CommandData &cmd_data)
  {
    change_line_generic(strikethrough_stream_id, cmd, cmd_data, true);
  }

  void
  push_font_qt(const Command &cmd, CommandData &cmd_data)
  {
    change_font_qt(cmd, cmd_data, true);
  }

  void
  push_font_file(const Command &cmd, CommandData &cmd_data)
  {
    change_font_file(cmd, cmd_data, true);
  }

  void
  push_font(const Command &cmd, CommandData &cmd_data)
  {
    change_font(cmd, cmd_data, true);
  }

  void
  pop_color(const Command &cmd, CommandData &cmd_data);

  void
  pop_font_pixel_size(const Command &cmd, CommandData &cmd_data);

  void
  pop_underlining(const Command &cmd, CommandData &cmd_data);

  void
  pop_strikethrough(const Command &cmd, CommandData &cmd_data);

  void
  pop_font(const Command &cmd, CommandData &cmd_data);


  void
  begin_sub_super_script(const Command &cmd, CommandData &cmd_data, 
                         bool negate, float initial_offset_value);

  void
  end_sub_super_script(const Command &cmd, CommandData &cmd_data);

  void
  begin_sub_script(const Command &cmd, CommandData &cmd_data);

  void
  begin_super_script(const Command &cmd, CommandData &cmd_data);

  void
  change_formatting(const Command &cmd, CommandData &cmd_data);

  void
  set_background_color(const Command &cmd, CommandData &cmd_data);

  void
  glyph_dump(const Command &cmd, CommandData &cmd_data);

  void
  glyph_dump(int begin, int end, bool character_codes, 
             CommandData &cmd_data);

  void
  show_font(const Command &cmd, CommandData &cmd_data);

  void
  show_font_subrange(const Command &cmd, CommandData &cmd_data);

  void
  change_line_generic(int stream_id, 
                      const Command &cmd, CommandData &cmd_data,
                      bool push);

  void
  add_image(const Command &cmd, CommandData &cmd_data);

  void
  add_distance_image(const Command &cmd, CommandData &cmd_data);

  
  void
  set_tess_params(const Command &cmd, CommandData &cmd_data);

  void
  create_shape(const Command &cmd, CommandData &cmd_data);

  void
  create_distance_field(const Command &cmd, CommandData &cmd_data);

  void
  add_stroked_shape(const Command &cmd, CommandData &cmd_data);

  void
  add_filled_shape(const Command &cmd, CommandData &cmd_data);

  void
  column_format(const Command &cmd, CommandData &cmd_data);

  void
  add_image_column(const Command &cmd, CommandData &cmd_data);
  
  void
  spill(const Command &cmd, CommandData &cmd_data);

  void
  include_dir(CommandData &cmd_data,
              ::DIR*, const std::string &filename);

  void
  generic_include_file(const Command &cmd, CommandData &cmd_data);

  void
  execute_on_change_font(WRATHTextureFont *pfont);

  void
  actual_distance_field_generation(per_shape_data *pshape,
                                   ivec2 dims, float pixel_dist,
                                   WRATHImage *pImage,
                                   bool skip_corners, bool use_point_sprites);

  std::vector<file_cmd> m_accepted_commands;
  std::vector<stream_cmd> m_accepted_stream_commands;
  std::map<file_key, FileData*> m_files;

  std::vector<file_cmd> m_accepted_column_commands;
  std::vector<file_cmd> m_spill_column_commands;

  WRATHTextItem::Drawer m_font_drawer;
  misc_drawers_type m_misc_drawers;
  int m_chunk_size;

  float m_default_pt_size;
  WRATHTextureFont *m_default_font;
  vecN<GLubyte,4> m_default_color;
  vec4 m_default_background_color;

  WRATHLayer *m_root_container;
  bool m_lazy_z;

  ExtraDrawState m_extra_state;
  Loader m_fetcher;
  bool m_generate_font_threaded_on_load;
  bool m_manual_mipmap_generation;

  WRATHMutex m_font_generation_list_mutex;
  std::map<WRATHTextureFont*, font_glyph_generator*> m_in_progress;

  std::set<WRATHTextureFont*> m_all_loaded_fonts;
  std::list<per_shape_data*> m_shapes;

  WRATHShapeAttributePackerF *m_stroked_shape_packer;
  WRATHShapeAttributePackerF *m_filled_shape_packer;
  WRATHShapeGPUDistanceFieldCreator::ScratchPad::handle m_scratch;
};




#endif
