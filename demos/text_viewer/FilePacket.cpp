/*! 
 * \file FilePacket.cpp
 * \brief file FilePacket.cpp
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
#include <stdio.h>
#include <limits>
#include <boost/algorithm/string/predicate.hpp>
#include <unistd.h>
#include "FilePacket.hpp"
#include "FileData.hpp"
#include "WRATHUTF8.hpp"
#include "WRATHUTF16.hpp"
#include "WRATHFreeTypeSupport.hpp"
#include "generic_command_line.hpp"
#include "WRATHTextureFontFreeType_Mix.hpp"
#include "WRATHFontFetch.hpp"
#include "WRATHDefaultStrokeAttributePacker.hpp"
#include "WRATHShapeDistanceFieldGPU.hpp"
#include "WRATHShapeDistanceFieldGPUutil.hpp"
#include "WRATHDefaultFillShapeAttributePacker.hpp"
#include "WRATHShapePreStroker.hpp"

#include "wrath_demo_image_support.hpp"

#ifdef WRATH_QT
#include <QFont>
#endif

template<>
void
readvalue_from_string(enum WRATHText::letter_spacing_e &value, const std::string &v)
{
  if(boost::iequals(v, "absolute") or boost::iequals(v,"abs"))
    {
      value=WRATHText::letter_spacing_absolute;
    }
  else
    {
      value=WRATHText::letter_spacing_relative;
    }
}

template<>
void
readvalue_from_string(enum WRATHText::capitalization_e &value, const std::string &v)
{
  if(boost::iequals(v, "lower") or boost::iequals(v,"lower_case"))
    {
      value=WRATHText::capitalization_all_lower_case;
    } 
  else if(boost::iequals(v, "upper") or boost::iequals(v,"upper_case"))
    {
      value=WRATHText::capitalization_all_upper_case;
    }
  else if(boost::iequals(v, "title") or boost::iequals(v,"title_case"))
    {
      value=WRATHText::capitalization_title_case;
    }
  else
    {
      value=WRATHText::capitalization_as_in_stream;
    }
}

template<>
void
readvalue_from_string(std::locale &value, const std::string &v)
{
  value=WRATHTextDataStream::create_locale(v.c_str());
}



namespace
{
  bool
  zero_fill(int winding_number, void *)
  {
    return winding_number==0;
  }

  template<typename iterator>
  void
  or_points(WRATHBBox<2> &pbox, iterator begin, iterator end)
  {
    for(;begin!=end;++begin)
      {
        pbox.set_or(begin->position());
      }
  }

  std::string
  convert_percent_to_spaces(const std::string &input)
  {
    std::string R(input);

    std::replace(R.begin(), R.end(), '%', ' ');
    return R;                 
  }

  std::map<int, WRATHTextureFont*>&
  int_map_font(void)
  {
    static std::map<int, WRATHTextureFont*> R;
    return R;
  }

  std::map<WRATHTextureFont*, int>&
  font_map_int(void)
  {
    static std::map<WRATHTextureFont*, int> R;
    return R;
  }

  int
  get_font_id(WRATHTextureFont *p)
  {
    std::map<WRATHTextureFont*, int>::iterator iter;
    static int C(0);

    iter=font_map_int().find(p);
    if(iter==font_map_int().end())
      {
        font_map_int()[p]=C;
        int_map_font()[C]=p;
        ++C;
        iter=font_map_int().find(p);
      }
    WRATHassert(iter!=font_map_int().end());
    return iter->second;
  }

  WRATHTextureFont*
  get_font(int id)
  {
    std::map<int, WRATHTextureFont*>::iterator iter;

    iter=int_map_font().find(id);

    return (iter!=int_map_font().end())?
      iter->second:NULL;

  }


  enum return_code
  get_show_font_subrange_arguments(const std::string &argument,
                                   WRATHTextureFont* &pfont, int &face_index,
                                   range_type<int> &range, std::string &font_name)
  {
    std::istringstream istr(argument);
    int id;

    istr >> id >> face_index 
         >> range.m_begin >> range.m_end 
         >> font_name;
    
    if(istr.fail())
      {
        std::cout << "\nFailed to get values string \""
                  << argument << "\"" << std::flush;
        return routine_fail;
      }

    pfont=get_font(id);

    return (pfont==NULL)?
      routine_fail:
      routine_success;
  }

  std::string
  set_show_font_subrange_arguments(WRATHTextureFont *pfont, int face_index,
                                   const range_type<int> &range, const std::string &font_name)
  {
    std::ostringstream ostr;
    ostr << get_font_id(pfont) 
         << " " << face_index << " "
         << range.m_begin << " " << range.m_end
         << " " << font_name;
    return ostr.str();
  }


  enum FileType::file_fetch_type
  file_type_from_file_ext(const std::string &ext)
  {
    typedef std::pair<std::string, enum FileType::file_fetch_type> local_type;
    static bool ready(false);
    static std::vector<local_type> R;
    static WRATHMutex mutex;

    WRATHLockMutex(mutex);
    if(!ready)
      {
        ready=true;
        R.push_back( std::make_pair(std::string("txt"), FileType::load_interpreted));
        R.push_back( std::make_pair(std::string("wutf8"), FileType::load_utf8));
        R.push_back( std::make_pair(std::string("wutf16"), FileType::load_utf16));
        R.push_back( std::make_pair(std::string("utf8"), FileType::load_raw_utf8));
        R.push_back( std::make_pair(std::string("utf16"), FileType::load_raw_utf16));
        R.push_back( std::make_pair(std::string("BMP"), FileType::load_image));
        R.push_back( std::make_pair(std::string("GIF"), FileType::load_image));
        R.push_back( std::make_pair(std::string("JPG"), FileType::load_image));
        R.push_back( std::make_pair(std::string("JPEG"), FileType::load_image));
        R.push_back( std::make_pair(std::string("PNG"), FileType::load_image));
        R.push_back( std::make_pair(std::string("PBM"), FileType::load_image));
        R.push_back( std::make_pair(std::string("PGM"), FileType::load_image));
        R.push_back( std::make_pair(std::string("PPM"), FileType::load_image));
        R.push_back( std::make_pair(std::string("TIFF"), FileType::load_image));
        R.push_back( std::make_pair(std::string("XBM"), FileType::load_image));
        R.push_back( std::make_pair(std::string("XPM"), FileType::load_image));

        R.push_back( std::make_pair(std::string("TTF"), FileType::load_font));
        R.push_back( std::make_pair(std::string("TTC"), FileType::load_font));
        R.push_back( std::make_pair(std::string("OTF"), FileType::load_font));
        R.push_back( std::make_pair(std::string("PFB"), FileType::load_font));
      }
    WRATHUnlockMutex(mutex);

    for(std::vector<local_type>::const_iterator
          iter=R.begin(), end=R.end(); iter!=end; ++iter)
      {
        if(boost::iequals(iter->first, ext))
          {
            return iter->second;
          }
      }
    return FileType::load_raw;
  }

  WRATHText::color_type
  link_color_for_file_browser(int type)
  {
    switch(type)
      {
      case FileType::load_image:
        return WRATHText::color_type(127, 127, 32, 255);
        break;

      case FileType::load_directory:
        return WRATHText::color_type(0, 187, 187, 255);
        break;

      case FileType::load_font:
        return WRATHText::color_type(255, 155, 0, 255);
        break;

      default:
        return WRATHText::color_type(155, 127, 255, 255);
      }
  };


  template<typename T>
  enum return_code
  load_file_contents(const std::string &pfilename,
                     std::vector<T> &out_data)
  {
    FILE *pFile;
    long file_size;

    pFile=fopen(pfilename.c_str(), "rb");
    if(pFile==NULL)
      {
        return routine_fail;
      }

    fseek(pFile, 0, SEEK_END);
    file_size=ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    out_data.resize(file_size/sizeof(T), 0);
    if(!out_data.empty())
      {
        size_t num_read;
        num_read=fread(&out_data[0], sizeof(T), out_data.size(), pFile);

        
        WRATHunused(num_read);
        WRATHassert(num_read==out_data.size());
      }
    fclose(pFile);
    return routine_success;
  }

  

  template<typename T>
  class CommandsForStreamProperty
  {
  public:
    typedef T StreamProperty;
    typedef typename T::type StreamPropertyType;

    typedef void (*fptr)(WRATHTextDataStream&, 
                         const std::string &original_cmd_string,
                         const std::vector<std::string> &cmd_string_tokenized);

    typedef std::pair<std::string, fptr> entry;

    static
    void
    push_property(WRATHTextDataStream &stream, 
                  const std::string &,
                  const std::vector<std::string> &cmd_string_tokenized)
    {
      StreamPropertyType v;

      readvalue_from_string<StreamPropertyType>(v, cmd_string_tokenized[1]);
      stream.stream() << typename StreamProperty::push_type(StreamProperty(v), -1);
    }

    static
    void
    set_property(WRATHTextDataStream &stream, 
                  const std::string &,
                  const std::vector<std::string> &cmd_string_tokenized)
    {
      StreamPropertyType v;

      readvalue_from_string<StreamPropertyType>(v, cmd_string_tokenized[1]);
      stream.stream() << typename StreamProperty::set_type(StreamProperty(v), -1);
    }

    static
    void
    pop_property(WRATHTextDataStream &stream, 
                 const std::string &,
                 const std::vector<std::string> &)
    {
      
      stream.stream() << typename StreamProperty::pop_type(-1);
    }

    static
    void
    add_command(std::vector<entry> &c, const std::string &pname)
    {
      c.push_back( entry(std::string("push_") + pname, &CommandsForStreamProperty::push_property) );
      c.push_back( entry(std::string("pop_") + pname, &CommandsForStreamProperty::pop_property) );
      c.push_back( entry(std::string("set_") + pname, &CommandsForStreamProperty::set_property) );
    } 
  };


  template<typename T>
  class command_arg:public command_line_argument_value<T>
  {
  public:
    command_arg(T v, const std::string &arg, 
                command_line_register &ptr):
      command_line_argument_value<T>(v, arg, "", ptr, false)
    {}
  };

  class image_argumnets:public command_line_register
  {
  public:
    image_argumnets(void):
      m_image("", "name", *this),
      m_w(0.0f, "w", *this),
      m_h(0.0f, "h", *this),
      m_r(1.0f, "red", *this),
      m_g(1.0f, "green", *this),
      m_b(1.0f, "blue", *this),
      m_a(1.0f, "alpha", *this)
    {}

    command_arg<std::string> m_image;
    command_arg<float> m_w, m_h;
    command_arg<float> m_r, m_g, m_b, m_a;
  };

  class font_config_arguments:public command_line_register
  {
  public:
    font_config_arguments(void):
      m_family("", "family", *this),
      m_foundry("", "foundry", *this),
      m_style("", "style", *this),
      m_italic(false, "italic", *this),
      m_bold(false, "bold", *this)
    {}

    command_arg<std::string> m_family, m_foundry, m_style;
    command_arg<bool> m_italic, m_bold;

    void
    generate_font_properties(WRATHFontFetch::FontProperties &output)
    {
      if(m_family.set_by_command_line())
        {
          output.family_name(convert_percent_to_spaces(m_family.m_value));
        }

      if(m_foundry.set_by_command_line())
        {
          output.foundry_name(convert_percent_to_spaces(m_foundry.m_value));
        }

      if(m_style.set_by_command_line())
        {
          output.style_name(convert_percent_to_spaces(m_style.m_value));
        }

      output.italic(m_italic.m_value);
      output.bold(m_bold.m_value);
    }
  };

  
  class create_distance_field_arguments:public command_line_register
  {
  public:
    command_arg<int> m_width, m_height;
    command_arg<std::string> m_shape, m_name;
    command_arg<float> m_pixel_dist;
    command_arg<bool> m_use_point_sprites;
    command_arg<bool> m_skip_corners;

    create_distance_field_arguments(void):
      m_width(256, "width", *this),
      m_height(256, "height", *this),
      m_shape("", "shape", *this),
      m_name("", "name", *this),
      m_pixel_dist(1.0f, "pixel_dist", *this),
      m_use_point_sprites(true, "use_point_sprites", *this),
      m_skip_corners(false, "skip_corners", *this)
    {}
      
  };

  class glyph_dump_arguments:public command_line_register
  {
  public:
    command_arg<int> m_start, m_end;
    command_arg<bool> m_use_character_codes;

    glyph_dump_arguments(void):
      m_start(0, "start", *this),
      m_end(std::numeric_limits<int>::max(), "end", *this),
      m_use_character_codes(false, "character_codes", *this)
    {}
  };
  
  class change_formatting_type:public command_line_register
  {
  public:
    command_arg<bool> m_raw;
    command_arg<float> m_width;
    command_arg<float> m_left;
    command_arg<std::string> m_alignment;
    command_arg<bool> m_break_words;

    change_formatting_type(void):
      m_raw(false, "raw", *this),
      m_width(-100.0f, "width", *this),
      m_left(0.0, "left", *this),
      m_alignment("left", "alignment", *this),
      m_break_words(false, "break_words", *this)
    {}
  };

  class color_arguments:public command_line_register
  {
  public:
    command_arg<int> m_r, m_g, m_b, m_a;
    
    explicit
    color_arguments(WRATHText::color_type D):
      m_r(D.x(), "r", *this),
      m_g(D.y(), "g", *this),
      m_b(D.z(), "b", *this),
      m_a(D.w(), "a", *this)
    {}
  };

  class color_argumentsf:public command_line_register
  {
  public:
    command_arg<float> m_r, m_g, m_b, m_a;
    
    explicit
    color_argumentsf(const vec4 &D):
      m_r(D.x(), "r", *this),
      m_g(D.y(), "g", *this),
      m_b(D.z(), "b", *this),
      m_a(D.w(), "a", *this)
    {}
  };

  class sub_super_scrips_arguments:public command_line_register
  {
  public:
    command_arg<float> m_offset;
    command_arg<bool> m_offset_is_relative;
    command_arg<float> m_scale_font_factor;
    command_arg<bool> m_use_previous_char_info;
    
    explicit
    sub_super_scrips_arguments(float initial_offset_value):
      m_offset(initial_offset_value, "offset", *this),
      m_offset_is_relative(true, "relative", *this),
      m_scale_font_factor(0.5f, "scale_font_factor", *this),
      m_use_previous_char_info(true, "use_prev", *this)
    {}
  };

  class named_link_arguments:public color_arguments
  {
  public:
    command_arg<std::string> m_filename;
    command_arg<std::string> m_linkname;
    command_arg<std::string> m_tagname;
    command_arg<bool> m_underline;

    named_link_arguments(WRATHText::color_type D):
      color_arguments(D),
      m_filename("", "target", *this),
      m_linkname("", "label", *this),
      m_tagname("", "tag", *this),
      m_underline(true, "underline", *this)
    {}
  };

  class per_column_data
  {
  public:
    command_arg<int> m_width;
    command_arg<std::string> m_file;
    command_arg<std::string> m_type;
    command_arg<float> m_space;
    command_arg<bool> m_reset;
    command_arg<std::string> m_alignment;
    command_arg<bool> m_break_words;

    explicit
    per_column_data(const std::string &postfix, command_line_register *ptr):
      m_width(100, std::string("width")+postfix, *ptr),
      m_file("nofile", std::string("file")+postfix, *ptr),
      m_type("include_file", std::string("type")+postfix, *ptr),
      m_space(0.0f, std::string("space")+postfix, *ptr),
      m_reset(true, std::string("reset")+postfix, *ptr),
      m_alignment("left", std::string("alignment")+postfix, *ptr),
      m_break_words(false, std::string("break_words")+postfix, *ptr)
    {}
  };

  class column_count_type:public command_arg<int>
  {
  public:
    std::vector<per_column_data*> m_col_data;
    
    column_count_type(command_line_register *ptr):
      command_arg<int>(0, "count", *ptr)
    {}
    
    ~column_count_type()
    {
      for(std::vector<per_column_data*>::iterator 
            iter=m_col_data.begin(); iter!=m_col_data.end(); ++iter)
        {
          WRATHPhasedDelete(*iter);
        }
    }
    
    virtual
    int
    check_arg(const std::vector<std::string> &argv, int location)
    {
      int old_value(m_value);
      int return_value;

      return_value=command_arg<int>::check_arg(argv, location);
      
      if(old_value<m_value)
        {
          m_col_data.resize(m_value);
          for(int i=old_value; i<m_value; ++i)
            {
              std::ostringstream i_as_str;
              i_as_str << i;
              m_col_data[i]=WRATHNew per_column_data(i_as_str.str(), parent());
            }
        }
      return return_value;
    }
  };

  class column_format_arguments:public command_line_register
  {
  private:
    column_count_type m_data;

  public:
    command_arg<float> m_spacing;

    column_format_arguments(void):
      m_data(this),
      m_spacing(1.0f, "space", *this)
    {}

    int
    number_columns(void)
    {
      return m_data.m_value;
    }

    const per_column_data&
    column_data(int i)
    {
      WRATHassert(0<=i and i<number_columns());
      return *m_data.m_col_data[i];
    }

  };

  
  
  class point_packet
  {
  public:
    enum arc_type
      {
        no_arc,
        cw_arc,
        ccw_arc
      };

    point_packet(const vec2 &pt):
      m_pt(pt),
      m_arc_mode(no_arc),
      m_angle(0.0f)
    {}

    vec2 m_pt;
    std::vector<vec2> m_control_points;

    enum arc_type m_arc_mode;
    float m_angle;

  };

  class shape_params
  {
  public:
    std::list< std::vector<point_packet> > m_outlines;

    void
    generate(const_c_array<std::string> in_values);
  };

  void
  shape_params::
  generate(const_c_array<std::string> in_values)
  {
    if(in_values.empty())
      {
        return;
      }

    const_c_array<std::string>::iterator current_iter, end_iter;
    int parity;
    vec2 current_vec2;
    bool adding_ctr_points(false);
    enum point_packet::arc_type doing_arc(point_packet::no_arc);

    /*
      Expected format:
      [ (p00x p00y) (p01x p01y) ... ] 
      [ (p10x p10y) (p11x p11y) ... ]

      the nutty way we parse it
      if we see a '[' we start a new outline
      Control points are signaled to begin with '[[' and end with ']]' 
      Arcs are signaled by having CCW or CW, in that case the next
      value indicates an angle.
      Since we are displaying so that y-increases
      downwards, we invert CW vs CCW.
     */
    
    current_iter=in_values.begin();
    end_iter=in_values.end();
    for(parity=0; current_iter!=end_iter; ++current_iter)
      {

        if(*current_iter=="[")
          {
            adding_ctr_points=false;
            m_outlines.push_back(std::vector<point_packet>());
          }
        else if(*current_iter=="[[")
          {
            adding_ctr_points=true;
          }
        else if(*current_iter=="]]")
          {
            adding_ctr_points=false;
          }
        else if(*current_iter=="CCW")
          {
            doing_arc=point_packet::cw_arc;
          }
        else if(*current_iter=="CW")
          {
            doing_arc=point_packet::ccw_arc;
          }
        else
          {
            float float_value;

            std::istringstream istr(*current_iter);
            istr >> float_value;
            if(!istr.fail())
              {
                if(doing_arc!=point_packet::no_arc)
                  {
                    m_outlines.back().back().m_arc_mode=doing_arc;
                    m_outlines.back().back().m_angle=float_value;

                    doing_arc=point_packet::no_arc;
                    
                  }
                else
                  {
                    current_vec2[parity]=float_value;
                    if(parity==1)
                      {
                        if(!adding_ctr_points)
                          {
                            m_outlines.back().push_back(current_vec2);
                          }
                        else if(!m_outlines.empty() and !m_outlines.back().empty())
                          {
                            m_outlines.back().back().m_control_points.push_back(current_vec2);
                          }
                        parity=0;
                      }
                    else
                      {
                        parity=1;
                      }
                  }
              }
          }
      }
  }

  class tess_params_argc:public command_line_register
  {
  public:
    tess_params_argc(WRATHShapeSimpleTessellatorPayload::PayloadParams &pp):
      m_curve_tessellation(pp.m_curve_tessellation, "curve_tess", *this),
      m_max_recurse(pp.m_max_recurse, "max_recurse", *this)
    {}
    
    command_arg<unsigned int&> m_curve_tessellation;
    command_arg<int&> m_max_recurse;
  };

  class stroke_params_args:public command_line_register
  {
  public:
    stroke_params_args(void):
      m_params(),
      m_shape("", "shape", *this),
      m_join_style("", "join_style", *this),
      m_cap_style("", "cap_style", *this),
      m_close_outline(m_params.m_close_outline, "close", *this),
      m_width(m_params.m_radius*2.0f, "width", *this),
      m_miter_limit(m_params.m_miter_limit, "miter_limit", *this),
      m_stroke_curves(true, "stroke_curves", *this)
    {}

    void
    set_params(void);

    WRATHDefaultStrokeAttributePacker::StrokingParameters m_params;

    command_arg<std::string> m_shape;
    command_arg<std::string> m_join_style;
    command_arg<std::string> m_cap_style;
    command_arg<bool> m_close_outline;
    command_arg<float> m_width;
    command_arg<float> m_miter_limit;
    command_arg<bool> m_stroke_curves;
  };

  void
  stroke_params_args::
  set_params(void)
  {
    m_params.m_close_outline=m_close_outline.m_value;
    m_params.m_miter_limit=m_miter_limit.m_value;
    m_params.m_radius=0.5f*m_width.m_value;
    m_params.m_stroke_curves=m_stroke_curves.m_value?
      WRATHDefaultStrokeAttributePacker::solid_stroke:
      WRATHDefaultStrokeAttributePacker::no_stroke;

    if(m_cap_style.set_by_command_line())
      {
        if(m_cap_style.m_value=="square")
          {
            m_params.m_cap_style=WRATHDefaultStrokeAttributePacker::square_cap;
          }
        else if(m_cap_style.m_value=="flat")
          {
            m_params.m_cap_style=WRATHDefaultStrokeAttributePacker::flat_cap;
          }
        else if(m_cap_style.m_value=="rounded")
          {
            m_params.m_cap_style=WRATHDefaultStrokeAttributePacker::rounded_cap;
          }
      }

    if(m_join_style.set_by_command_line())
      {
        if(m_join_style.m_value=="bevel")
          {
            m_params.m_join_style=WRATHDefaultStrokeAttributePacker::bevel_join;
          }
        else if(m_join_style.m_value=="miter")
          {
            m_params.m_join_style=WRATHDefaultStrokeAttributePacker::miter_join;
          }
        else if(m_join_style.m_value=="round")
          {
            m_params.m_join_style=WRATHDefaultStrokeAttributePacker::round_join;
          }
        else if(m_join_style.m_value=="none")
          {
            m_params.m_join_style=WRATHDefaultStrokeAttributePacker::no_join;
          }
      }
  }

}

///////////////////////////////////
// FilePacket::stack_data methods
FilePacket::stack_data::
stack_data(const stack_data &parent, const std::string &pfilename):
  m_line(1),
  m_actual_file(!pfilename.empty())
{
  if(m_actual_file)
    {
      std::string::size_type iter;

      iter=pfilename.find_last_of("/\\");

      //get the path, if the leading character is
      //a \ or / then the path is absolute:
      if(pfilename.at(0)=='/'
         or pfilename.at(0)=='\'')
        {
          m_file_with_path=pfilename;
          m_file_path=pfilename.substr(0,iter+1); //include trailing '/'
          m_file_without_path=pfilename.substr(1+iter);
        }
      else
        {
          m_file_path=parent.m_file_path + pfilename.substr(0,iter+1);
          m_file_without_path=pfilename.substr(iter+1);
          m_file_with_path=m_file_path+m_file_without_path;
        }
    }
}

/////////////////////////////////////////
// FilePacket::font_glyph_generator methods
FilePacket::font_glyph_generator::
font_glyph_generator(FilePacket *pparent, WRATHTextureFont *pfont):
  m_font(pfont),
  m_parent(pparent),
  m_current(0),
  m_abort(false),
  m_done(false)
{
  std::string::size_type pos;

  pos=pfont->simple_name().find_last_of('/');

  if(pos!=std::string::npos)
    {
      m_label=pfont->simple_name().substr(pos+1);
    }
  else
    {
      m_label=pfont->simple_name();
    }
}

FilePacket::font_glyph_generator::
~font_glyph_generator(void)
{}

void
FilePacket::font_glyph_generator::
abort(void)
{
  WRATHLockMutex(m_mutex);
  m_abort=true;
  WRATHUnlockMutex(m_mutex);
}


bool
FilePacket::font_glyph_generator::
complete(float &v)
{
  int C;
  bool r;

  WRATHLockMutex(m_mutex);
  C=m_current;
  r=m_done;
  WRATHUnlockMutex(m_mutex);
  
  v=static_cast<float>(C)/static_cast<float>(m_font->number_glyphs());

  return r;
}



void*
FilePacket::font_glyph_generator::
thread_function(void *obj)
{
  font_glyph_generator *ptr(reinterpret_cast<font_glyph_generator*>(obj));
  bool aborted(false);

  for(int C=0, endC=ptr->m_font->number_glyphs(); C<endC and !aborted; ++C)
    {
      WRATHTextureFont::glyph_index_type G(static_cast<uint32_t>(C));
      ptr->m_font->glyph_data(G);

      WRATHLockMutex(ptr->m_mutex);
      ptr->m_current=C+1;
      if(ptr->m_abort)
        {
          aborted=true;
        }
      WRATHUnlockMutex(ptr->m_mutex);
    }

  WRATHLockMutex(ptr->m_mutex);
  ptr->m_done=true;
  WRATHUnlockMutex(ptr->m_mutex);

  return NULL;
}

FilePacket::font_glyph_generator*
FilePacket::font_glyph_generator::
create(FilePacket *pparent, WRATHTextureFont *pfont)
{
  font_glyph_generator *obj;

  obj=WRATHNew font_glyph_generator(pparent, pfont);
  obj->m_thread_id=WRATHThreadID::create_thread(thread_function, obj);
  return obj;
}

WRATHThreadID
FilePacket::font_glyph_generator::
thread_id(void)
{
  return m_thread_id;
}
  

//////////////////////////////////////////
// FilePacket::per_shape_data methods
FilePacket::per_shape_data::
per_shape_data(WRATHShapeF *pshape, 
               const WRATHShapeSimpleTessellatorPayload::PayloadParams &tess_params):
  m_shape(pshape),
  m_tess_params(tess_params),
  m_pre_stroke_parameters(tess_params)
{}

FilePacket::per_shape_data::
~per_shape_data(void)
{
  WRATHDelete(m_shape);
}

WRATHShapeSimpleTessellatorPayload::handle
FilePacket::per_shape_data::
tessellated_data(void)
{
  return m_shape->fetch_matching_payload<WRATHShapeSimpleTessellatorPayload>(m_tess_params);
}

WRATHShapePreStrokerPayload::handle
FilePacket::per_shape_data::
pre_stroke_data(void)
{
  return m_shape->fetch_matching_payload<WRATHShapePreStrokerPayload>(m_pre_stroke_parameters);
}
     
WRATHShapeTriangulatorPayload::handle
FilePacket::per_shape_data::
fill_data(void)
{
  return m_shape->fetch_matching_payload<WRATHShapeTriangulatorPayload>(m_tess_params);
}


//////////////////////////////////////////
// FilePacket::CommandData methods
FilePacket::CommandData::
CommandData(FileData *p):
  m_current(p),
  m_left(0.0f),
  m_width(false, 0.0f),
  m_parent(NULL), 
  m_root(this),
  m_current_location(1),
  m_is_spill(false)
{
  init_stream();
}

FilePacket::CommandData::
CommandData(const vec2 &pos, 
            CommandData *parent, 
            float left, 
            float width,
            enum WRATHFormatter::alignment_type palignment,
            bool pbreak_words):
  m_included_files(parent->m_included_files),
  m_current(parent->m_current),
  m_left(left+parent->m_left),
  m_width(true, width),
  m_parent(parent),
  m_root(parent->m_root),
  m_current_location(1, m_parent->m_current_location.back()),
  m_is_spill(false)
{
  m_parent->m_children.push_back(this);
  m_layout
    .start_position(pos)
    .alignment(palignment)
    .break_words(pbreak_words)
    .clear_end_line_constraints()
    .clear_begin_line_constraints();

  init_stream();

}

bool
FilePacket::CommandData::
circular_inclusion(const std::string &pfile)
{
  return m_included_files.find(WRATHUtil::filename_fullpath(pfile))!=m_included_files.end();
}


void
FilePacket::CommandData::
init_stream(void)
{
  m_layout
    .eat_white_spaces(true)
    .add_begin_line_constraint( WRATHColumnFormatter::Constraint()
                                .constraint(m_left))
    .line_spacing(2.0f);

  if(m_width.first)
    {
      m_layout
        .add_end_line_constraint( WRATHColumnFormatter::Constraint()
                                  .constraint(m_left+m_width.second));
    }

  m_streams.push_back(WRATHNew WRATHTextDataStream());
  m_streams.front()->format(m_layout);
                            
  m_streams.front()->stream() 
    << WRATHText::set_state<line_stream_type>(false, underline_stream_id)
    << WRATHText::set_state<line_stream_type>(false, strikethrough_stream_id);
}

FilePacket::CommandData::
~CommandData()
{
  for(std::vector<WRATHTextDataStream*>::iterator
        iter=m_streams.begin(), end=m_streams.end();
      iter!=end; ++iter)
    {
      WRATHTextDataStream *ptr(*iter);
      WRATHPhasedDelete(ptr);
    }

  for(std::list<CommandData*>::iterator
        iter=m_children.begin(), end=m_children.end();
      iter!=end; ++iter)
    {
      CommandData *ptr(*iter);
      WRATHPhasedDelete(ptr);
    }

  
}

void
FilePacket::CommandData::
add_shape(const std::string &pname, per_shape_data *pshape)
{
  if(pshape!=NULL)
    {
      m_root->m_shapes[pname]=pshape;
    }
}

void
FilePacket::CommandData::
add_distance_field(const std::string &pname, WRATHImage *im)
{
  if(im!=NULL)
    {
      m_root->m_shape_distance_images[pname]=im;
    }
}

FilePacket::per_shape_data*
FilePacket::CommandData::
get_shape(const std::string &pname)
{
  std::map<std::string, per_shape_data*>::iterator iter;

  iter=m_root->m_shapes.find(pname);
  return (iter!=m_root->m_shapes.end())?
    iter->second:
    NULL;
}

WRATHImage*
FilePacket::CommandData::
get_distance_field(const std::string &pname)
{
  std::map<std::string, WRATHImage*>::iterator iter;
  WRATHImage *R;

  iter=m_root->m_shape_distance_images.find(pname);
  R=(iter!=m_root->m_shape_distance_images.end())?
    iter->second:
    NULL;


  return R;
}


void
FilePacket::CommandData::
place_text(void)
{
  for(std::vector<WRATHTextDataStream*>::iterator
        iter=m_streams.begin(), end=m_streams.end();
      iter!=end; ++iter)
    {
      WRATHTextDataStream *ptr(*iter);
      m_current->add_text(ptr->formatted_text(), 
                          ptr->state_stream());
    }

  for(std::list<CommandData*>::iterator
        iter=m_children.begin(), end=m_children.end();
      iter!=end; ++iter)
    {
      CommandData *ptr(*iter);
      ptr->place_text();
    }
}

vec2
FilePacket::CommandData::
new_stream(int flags)
{
  WRATHTextDataStream *ptr;
  vec2 pos;

  ptr=WRATHNew WRATHTextDataStream();
  ptr->set_state(m_streams.back()->state_stream(),
                 (copy_stacks&flags)!=0);

  if(reset_lining&flags)
    {
      current_stream().stream() 
        << WRATHText::set_state<line_stream_type>(false, underline_stream_id)
        << WRATHText::set_state<line_stream_type>(false, strikethrough_stream_id);
    }
  pos=m_streams.back()->end_text_pen_position().m_descend_start_pen_position;
  m_streams.push_back(ptr);

  if(m_is_spill)
    {
      //if the formatting was a spilling formatting
      //then we need to set the y-position to end of the 
      //requirements of column formatting.
      m_is_spill=false;

      for(std::vector<WRATHColumnFormatter::Constraint>::const_iterator
            iter=m_layout.m_begin_line_constraints.begin(),
            end=m_layout.m_begin_line_constraints.end();
          iter!=end; ++iter)
        {
          pos.y()=std::max(pos.y(), iter->m_begin);
        }   

      for(std::vector<WRATHColumnFormatter::Constraint>::const_iterator
            iter=m_layout.m_end_line_constraints.begin(),
            end=m_layout.m_end_line_constraints.end();
          iter!=end; ++iter)
        {
          pos.y()=std::max(pos.y(), iter->m_begin);
        }   
    }

  return pos;
}


//////////////////////////////////////////
// FilePacket methods
FilePacket::
FilePacket(WRATHLayer *proot_container,
           WRATHTextItem::Drawer pfont_drawer,
           misc_drawers_type pmisc_drawers,
           float default_pt_size, 
           WRATHTextureFont *default_font,
           vecN<GLubyte,4> default_color,
           const vec4 &pdefault_background_color,
           int chunk_size, bool lazyz,
           const ExtraDrawState &pextra_state,
           Loader pfetcher,
           bool generate_font_threaded_on_load,
           bool manual_mipmap_generation):
  m_minification_image_filter(GL_LINEAR_MIPMAP_NEAREST),
  m_magnification_image_filter(GL_LINEAR),
  m_font_drawer(pfont_drawer),
  m_misc_drawers(pmisc_drawers),
  m_chunk_size(chunk_size),
  m_default_pt_size(default_pt_size),
  m_default_font(default_font),
  m_default_color(default_color),
  m_default_background_color(pdefault_background_color),
  m_lazy_z(lazyz),
  m_extra_state(pextra_state),
  m_fetcher(pfetcher),
  m_generate_font_threaded_on_load(generate_font_threaded_on_load),
  m_manual_mipmap_generation(manual_mipmap_generation)
{
  m_accepted_commands.push_back( file_cmd("include_file", &FilePacket::include_file));
  m_accepted_commands.push_back( file_cmd("include_raw_file", &FilePacket::include_raw_file));
  m_accepted_commands.push_back( file_cmd("include_utf8", &FilePacket::include_utf8));
  m_accepted_commands.push_back( file_cmd("include_utf16", &FilePacket::include_utf16));
  m_accepted_commands.push_back( file_cmd("include_raw_utf8", &FilePacket::include_raw_utf8));
  m_accepted_commands.push_back( file_cmd("include_raw_utf16", &FilePacket::include_raw_utf16));
  m_accepted_commands.push_back( file_cmd("glyph_dump", &FilePacket::glyph_dump));
  m_accepted_commands.push_back( file_cmd("image", &FilePacket::add_image));

  m_accepted_commands.push_back( file_cmd("PP", &FilePacket::change_formatting));
  m_accepted_commands.push_back( file_cmd("COL", &FilePacket::column_format));
  m_accepted_commands.push_back( file_cmd("bgcolor", &FilePacket::set_background_color));
  

  m_accepted_commands.push_back( file_cmd("link", &FilePacket::add_link));
  m_accepted_commands.push_back( file_cmd("back_link", &FilePacket::add_back_link));
  m_accepted_commands.push_back( file_cmd("tag", &FilePacket::add_tag));
  m_accepted_commands.push_back( file_cmd("named_link", &FilePacket::add_named_link));

  m_accepted_commands.push_back( file_cmd("color", &FilePacket::change_color));
  m_accepted_commands.push_back( file_cmd("font_size", &FilePacket::change_font_pixel_size));
  m_accepted_commands.push_back( file_cmd("font_qt", &FilePacket::change_font_qt));
  m_accepted_commands.push_back( file_cmd("font_file", &FilePacket::change_font_file));
  m_accepted_commands.push_back( file_cmd("font", &FilePacket::change_font));  
  m_accepted_commands.push_back( file_cmd("underline", &FilePacket::change_underlining));
  m_accepted_commands.push_back( file_cmd("strike", &FilePacket::change_strikethrough));
  
  m_accepted_commands.push_back( file_cmd("push_color", &FilePacket::push_color));
  m_accepted_commands.push_back( file_cmd("push_font_size", &FilePacket::push_font_pixel_size));
  m_accepted_commands.push_back( file_cmd("push_font_qt", &FilePacket::push_font_qt));
  m_accepted_commands.push_back( file_cmd("push_font_file", &FilePacket::push_font_file));
  m_accepted_commands.push_back( file_cmd("push_font", &FilePacket::push_font));
  m_accepted_commands.push_back( file_cmd("push_underline", &FilePacket::push_underlining));
  m_accepted_commands.push_back( file_cmd("push_strike", &FilePacket::push_strikethrough));
  
  m_accepted_commands.push_back( file_cmd("pop_color", &FilePacket::pop_color));
  m_accepted_commands.push_back( file_cmd("pop_font_size", &FilePacket::pop_font_pixel_size));
  m_accepted_commands.push_back( file_cmd("pop_font", &FilePacket::pop_font));
  m_accepted_commands.push_back( file_cmd("pop_underline", &FilePacket::pop_underlining));
  m_accepted_commands.push_back( file_cmd("pop_strike", &FilePacket::pop_strikethrough));

  m_accepted_commands.push_back( file_cmd("begin_sub_script", &FilePacket::begin_sub_script));
  m_accepted_commands.push_back( file_cmd("end_sub_script", &FilePacket::end_sub_super_script));
  m_accepted_commands.push_back( file_cmd("begin_super_script", &FilePacket::begin_super_script));
  m_accepted_commands.push_back( file_cmd("end_super_script", &FilePacket::end_sub_super_script));

  m_accepted_commands.push_back( file_cmd("directory_listing", &FilePacket::include_dir) );

  m_accepted_commands.push_back( file_cmd("set_tess_params", &FilePacket::set_tess_params) );
  m_accepted_commands.push_back( file_cmd("create_shape", &FilePacket::create_shape) );
  m_accepted_commands.push_back( file_cmd("stroke", &FilePacket::add_stroked_shape) );
  m_accepted_commands.push_back( file_cmd("fill", &FilePacket::add_filled_shape) );
  m_accepted_commands.push_back( file_cmd("create_distance_field", 
                                          &FilePacket::create_distance_field) );
  m_accepted_commands.push_back( file_cmd("dist_image", 
                                          &FilePacket::add_distance_image) );

  CommandsForStreamProperty<WRATHText::kerning>::add_command(m_accepted_stream_commands, "kerning");
  CommandsForStreamProperty<WRATHText::horizontal_stretching>::add_command(m_accepted_stream_commands, 
                                                                           "horizontal_stretch");
  CommandsForStreamProperty<WRATHText::vertical_stretching>::add_command(m_accepted_stream_commands, 
                                                                         "vertical_stretch");
  CommandsForStreamProperty<WRATHText::word_spacing>::add_command(m_accepted_stream_commands, 
                                                                  "word_spacing");
  CommandsForStreamProperty<WRATHText::letter_spacing>::add_command(m_accepted_stream_commands, 
                                                                    "letter_spacing");
  CommandsForStreamProperty<WRATHText::letter_spacing_type>::add_command(m_accepted_stream_commands, 
                                                                         "letter_spacing_type");
  CommandsForStreamProperty<WRATHText::capitalization>::add_command(m_accepted_stream_commands,
                                                                    "capitalization");
  CommandsForStreamProperty<WRATHText::localization>::add_command(m_accepted_stream_commands,
                                                                  "localization");
  
  m_accepted_column_commands.push_back( file_cmd("file", &FilePacket::include_file));
  m_accepted_column_commands.push_back( file_cmd("raw_file", &FilePacket::include_raw_file));
  m_accepted_column_commands.push_back( file_cmd("utf8", &FilePacket::include_utf8));
  m_accepted_column_commands.push_back( file_cmd("utf16", &FilePacket::include_utf16));
  m_accepted_column_commands.push_back( file_cmd("raw_utf8", &FilePacket::include_raw_utf8));
  m_accepted_column_commands.push_back( file_cmd("raw_utf16", &FilePacket::include_raw_utf16));
  m_accepted_column_commands.push_back( file_cmd("image", &FilePacket::add_image_column));

  m_spill_column_commands.push_back( file_cmd("spill", &FilePacket::spill));
  m_spill_column_commands.push_back( file_cmd("spill_file", &FilePacket::include_file));
  m_spill_column_commands.push_back( file_cmd("spill_utf8", &FilePacket::include_utf8));
  m_spill_column_commands.push_back( file_cmd("spill_utf16", &FilePacket::include_utf16));
  m_spill_column_commands.push_back( file_cmd("spill_raw_utf8", &FilePacket::include_raw_utf8));
  m_spill_column_commands.push_back( file_cmd("spill_raw_utf16", &FilePacket::include_raw_utf16));

  //application will set transformation and project matrices
  //by modifying proot_container.
  m_root_container=WRATHNew WRATHLayer(proot_container);
  m_root_container->simulation_composition_mode(WRATHLayer::modelview_matrix, WRATHLayer::compose_matrix);
  m_root_container->simulation_composition_mode(WRATHLayer::projection_matrix, WRATHLayer::compose_matrix);


  m_stroked_shape_packer=WRATHDefaultStrokeAttributePackerF::fetch();
  m_scratch=WRATHNew WRATHShapeGPUDistanceFieldCreator::ScratchPadFBO(m_root_container->triple_buffer_enabler());

  m_filled_shape_packer=WRATHDefaultFillShapeAttributePackerF::fetch();

  execute_on_change_font(m_default_font);
}

FilePacket::
~FilePacket()
{
  for(std::map<file_key, FileData*>::iterator 
        iter=m_files.begin(), end=m_files.end();
      iter!=end; ++iter)
    {
      WRATHPhasedDelete(iter->second);
    }

  for(std::list<per_shape_data*>::iterator
        iter=m_shapes.begin(), end=m_shapes.end();
      iter!=end; ++iter)
    {
      per_shape_data *ptr(*iter);
      WRATHPhasedDelete(ptr);
    }


  /* abort any remaining font generations */
  WRATHLockMutex(m_font_generation_list_mutex);
  for(std::map<WRATHTextureFont*, font_glyph_generator*>::iterator
        iter=m_in_progress.begin(), end=m_in_progress.end(); iter!=end; ++iter)
    {
      iter->second->abort();
      WRATHThreadID::wait_thread(iter->second->thread_id());
      WRATHPhasedDelete(iter->second);
    }


  WRATHUnlockMutex(m_font_generation_list_mutex);

  m_scratch=NULL;
}

void
FilePacket::
execute_on_change_font(WRATHTextureFont *pfont)
{
  if(m_generate_font_threaded_on_load and m_all_loaded_fonts.find(pfont)==m_all_loaded_fonts.end())
    {
      WRATHLockMutex(m_font_generation_list_mutex);
      m_all_loaded_fonts.insert(pfont);
      m_in_progress[pfont]=font_glyph_generator::create(this, pfont);

      WRATHUnlockMutex(m_font_generation_list_mutex);
    }
}


bool
FilePacket::
update_threaded_font_load_progress(WRATHTextDataStream &ostr)
{
  bool return_value;
  
  WRATHLockMutex(m_font_generation_list_mutex);
  
  std::list<std::map<WRATHTextureFont*, font_glyph_generator*>::iterator> done_iters;
  for(std::map<WRATHTextureFont*, font_glyph_generator*>::iterator
        iter=m_in_progress.begin(), end=m_in_progress.end(); iter!=end; ++iter)
    {
      float percentage_done;
      if(iter->second->complete(percentage_done))
        {
          WRATHThreadID::wait_thread(iter->second->thread_id());
          WRATHPhasedDelete(iter->second);
          done_iters.push_back(iter);
        }
      else
        {
          ostr.stream() << "\n" << iter->second->label()
                        << ": " << std::setw(3) << static_cast<int>(100.0f*percentage_done)
                        << "%";
        }
    }
  
  for(std::list<std::map<WRATHTextureFont*, font_glyph_generator*>::iterator>::iterator
        iter=done_iters.begin(), end=done_iters.end(); iter!=end; ++iter)
    {
      m_in_progress.erase(*iter);
    }
  
  return_value=!m_in_progress.empty();

  WRATHUnlockMutex(m_font_generation_list_mutex);
  return return_value;
}

FileData*
FilePacket::
fetch_file(const std::string &rawfilename, file_fetch_type ptype)
{
  std::map<file_key, FileData*>::iterator iter;
  std::string pfilename;
  
  if(ptype!=FileType::load_font_subrange)
    {
      pfilename=WRATHUtil::filename_fullpath(rawfilename);
    }
  else
    {
      pfilename=rawfilename;
    }
  iter=m_files.find(file_key(pfilename,ptype) );

  if(iter==m_files.end())
    {
      FileData *fptr;

      fptr=WRATHNew FileData(this, pfilename, ptype);
      fptr->background_color(m_default_background_color);

      m_files[file_key(pfilename, ptype)]=fptr;
      return fptr;
    }
  else
    {
      return iter->second;
    }
}


void
FilePacket::
create_command_from_string(const std::string &in_string,
                           Command &out_command)
{
  std::istringstream istr(in_string);


  out_command.clear();
  out_command.original_string(in_string);

  istr >> out_command.command();
  while(istr)
    {
      std::string arg;
      istr >> arg;

      if(!istr.fail())
        {
          out_command.add_argument(arg);
        }
    }
}



void
FilePacket::
handle_command(const FilePacket::Command &cmd, 
               FilePacket::CommandData &cmd_data)
{
  for(std::vector<file_cmd>::iterator 
        iter=m_accepted_commands.begin(), 
        end=m_accepted_commands.end();
      iter!=end; ++iter)
    {
      if(iter->first==cmd.command())
        {
          (this->*iter->second)(cmd, cmd_data);
          return;
        }
    }

  for(std::vector<stream_cmd>::iterator 
        iter=m_accepted_stream_commands.begin(), 
        end=m_accepted_stream_commands.end();
      iter!=end; ++iter)
    {
      if(iter->first==cmd.command())
        {
          iter->second(cmd_data.current_stream(),
                       cmd.original_string(),
                       cmd.string_tokenized());
          return;
        }
    }

  //unknown command, oh well write in red:
  cmd_data.current_stream().stream()
    << push_default_state(this)
    << WRATHText::set_color(0xff, 0x33, 0x33, 0xff)
    << "\nUnknown command: " << cmd.command()
    << " (at " << cmd_data.m_current_location.back() << ")"
    << pop_default_state();
}

void
FilePacket::
load_file(const std::string &pfilename, FileData *file_data, 
          file_fetch_type type)
{
  Command cmd;
  bool raw_formatting(false);
  cmd_fptr fptr(NULL);
  CommandData cmd_data(file_data);


  
  cmd_data.current_stream().stream() 
    << WRATHText::set_font(m_default_font)
    << WRATHText::set_color(m_default_color)
    << WRATHText::set_pixel_size(m_default_pt_size);

  cmd_data.m_layout
    .alignment(WRATHFormatter::align_center)
    .add_begin_line_constraint( WRATHColumnFormatter::Constraint()
                                .constraint(0))
    
    .add_end_line_constraint( WRATHColumnFormatter::Constraint()
                              .constraint(800));

  cmd_data.current_stream().format(cmd_data.m_layout);
  add_quit_link(Command(), cmd_data);
  

  switch(type)
    {
    default:
    case FileType::load_interpreted:
      cmd.command()="include_file";
      cmd.add_argument(pfilename);
      fptr=&FilePacket::include_file;
      break;

    case FileType::load_utf8:
      cmd.command()="include_utf8";
      cmd.add_argument(pfilename);
      fptr=&FilePacket::include_utf8;
      break;

    case FileType::load_utf16:
      cmd.command()="include_utf16";
      cmd.add_argument(pfilename);
      fptr=&FilePacket::include_utf16;
      break;

    case FileType::load_raw:
      cmd.command()="include_raw_file";
      cmd.add_argument(pfilename);
      fptr=&FilePacket::include_raw_file;  
      raw_formatting=true;
      break;

    case FileType::load_raw_utf8:
      cmd.command()="include_raw_utf8";
      cmd.add_argument(pfilename);
      fptr=&FilePacket::include_raw_utf8;
      raw_formatting=true;
      break;

    case FileType::load_raw_utf16:
      cmd.command()="include_raw_utf16";
      cmd.add_argument(pfilename);
      fptr=&FilePacket::include_raw_utf16;
      raw_formatting=true;
      break;

    case FileType::load_image:
      {
        cmd.command()="image";
        cmd.add_argument(std::string("name:")+pfilename);
        fptr=&FilePacket::add_image;
        raw_formatting=true;
      }
      break;

    case FileType::load_font:
      cmd.command()="show_font";
      cmd.add_argument(pfilename);
      fptr=&FilePacket::show_font;
      break;

    case FileType::load_font_subrange:
      cmd.command()="show_font_subrange";
      cmd.add_argument(pfilename);
      fptr=&FilePacket::show_font_subrange;
      break;
      
    case FileType::load_directory:
      fptr=NULL;
      raw_formatting=true;
      cmd.add_argument(pfilename);
      break;
    }
  

  

  vec2 pos(cmd_data.new_stream());
  cmd_data.m_layout
    .alignment(WRATHFormatter::align_text_begin)
    .start_position(pos)
    .line_spacing(2.0f)
    .clear_end_line_constraints();
 
  cmd_data.current_stream().format(cmd_data.m_layout);
  add_back_link(Command(), cmd_data);

  if(type!=FileType::load_interpreted and type!=FileType::load_font_subrange)
    {
      cmd_data.current_stream().stream()
        << "\n[File:\"" << pfilename << "\"]";
    }

  pos=cmd_data.new_stream();
  cmd_data.m_layout
    .alignment(WRATHFormatter::align_text_begin)
    .start_position(pos)
    .line_spacing(2.0f);

  if(raw_formatting)
    {
      cmd_data.m_layout
        .clear_end_line_constraints()
        .eat_white_spaces(false);
    }
  else
    {
      cmd_data.m_layout
        .eat_white_spaces(true)
        .add_end_line_constraint( WRATHColumnFormatter::Constraint()
                                  .constraint(800));
    } 
  cmd_data.current_stream().format(cmd_data.m_layout); 

  
  if(type==FileType::load_directory)
    {
      ::DIR *ptr;
      std::string pp(WRATHUtil::filename_fullpath(pfilename));
      
      fptr=NULL;
      ptr=::opendir(pp.c_str());
      if(ptr!=NULL)
        {
          WRATHassert(*pp.rbegin()=='/');
          
          include_dir(cmd_data, ptr, pp);
          ::closedir(ptr);
        }
    }
  else if(fptr!=NULL)
    {
      (this->*fptr)(cmd, cmd_data);
    }


  

  post_process(cmd_data);
  cmd_data.place_text();

  // execute_on_change_font(m_default_font);
}

template<typename iterator>
void
FilePacket::
include_file(iterator begin, iterator end, 
             bool process_commands, CommandData &cmd_data)
{
  uint32_t last_char(0);
  bool line_commented(false);

  //this is kind of an embarassing mess, but it is demo code.
  for(iterator iter=begin; iter!=end; ++iter)
    {
      uint32_t ch(*iter);

      if(ch=='\n')
        {
          ++cmd_data.m_current_location.back().m_line;
          line_commented=false;
        }

      if(last_char=='\n' and ch=='%')
        {
          line_commented=true;
        }

      if(line_commented)
        {
          last_char=ch;
        }
      else
        {
          if(ch=='\\' and last_char!='\\')
            {
              const char cmd_string[]="\\cmd{";
              int current_letter(0);
              iterator restore_iter(iter);
              bool is_command(process_commands);
              
              //read forward to see if this is a command:              
              for(;iter!=end and is_command and current_letter<5; ++current_letter, ++iter)
                {
                  is_command=( *iter==static_cast<uint32_t>(cmd_string[current_letter]) );
                }
              
              is_command=is_command and (current_letter==5);
              if(is_command)
                {
                  //is a command: find the next close brace in the file
                  iterator command_start(iter);
                  int line_count(0);
                  
                  for(;iter!=end and *iter!='}'; ++iter)
                    {
                      if(*iter=='\n')
                        {
                          ++line_count;
                        }
                      last_char=*iter;
                    }
                  
                  is_command=(iter!=end and *iter=='}');
                  
                  if(is_command)
                    {
                      //we have a command.
                      Command command_parsed;
                      
                      cmd_data.m_current_location.back().m_line+=line_count;
                      
                      create_command_from_string(std::string(command_start, iter),
                                                 command_parsed);
                      
                      handle_command(command_parsed, cmd_data);
                    }
                }
              
              if(!is_command)
                {
                  iter=restore_iter;
                  last_char=ch;
                }
            }
          else if(ch=='n' and last_char=='\\')
            {
              cmd_data.current_stream().append(WRATHTextureFont::character_code_type('\n'));
              last_char=0;
            }
          else if(ch=='t' and last_char=='\\')
            {
              cmd_data.current_stream().append(WRATHTextureFont::character_code_type('\t'));
              last_char=0;
            }
          else if(ch=='\\' and last_char=='\\')
            {
              cmd_data.current_stream().append(WRATHTextureFont::character_code_type(ch));
              last_char=0;
            }
          else if(!cmd_data.m_layout.m_eat_white_spaces and cmd_data.m_parent==NULL)
            {
              cmd_data.current_stream().append(WRATHTextureFont::character_code_type(ch));
              last_char=ch;
            }
          else if(ch=='\n')
            {
              last_char=ch;
            }
          else
            {
              if(last_char=='\n' and ch!=' ')
                {
                  cmd_data.current_stream().append(WRATHTextureFont::character_code_type(' '));
                }
              
              cmd_data.current_stream().append(WRATHTextureFont::character_code_type(ch));
              last_char=ch;
            }
        }
    }
  
  cmd_data.current_stream().stream() 
    << WRATHText::set_state<line_stream_type>(false, underline_stream_id)
    << WRATHText::set_state<line_stream_type>(false, strikethrough_stream_id);
  
}

void
FilePacket::
include_file_general(const Command &cmd, CommandData &cmd_data, 
                     bool process_commands)
{
  std::vector<uint8_t> raw_data;

  if(cmd.arguments_empty())
    {
      cmd_data.current_stream().stream() 
        << push_default_state(this)
        << WRATHText::set_color(0xff, 0x33, 0x33, 0xff)
        << "\nNo file specified"
        << " (at " << cmd_data.m_current_location.back() << ")\n"
        << pop_default_state();
      return;
    }

  stack_data pfile(cmd_data.m_current_location.back(),
                   cmd.argument(0));

  if(process_commands and
     cmd_data.circular_inclusion(pfile.m_file_with_path))
    {
      cmd_data.current_stream().stream() 
        << push_default_state(this)
        << WRATHText::set_color(0xff, 0x33, 0x33, 0xff)
        << "\nCircular inclusion of file \"" 
        << cmd.argument(0) << "\""
        << " (at " << cmd_data.m_current_location.back() << ")\n"
        << pop_default_state();
      return;
    }
     

  if(routine_fail==load_file_contents(pfile.m_file_with_path, raw_data))
    {
      cmd_data.current_stream().stream() 
        << push_default_state(this)
        << WRATHText::set_color(0xff, 0x33, 0x33, 0xff)
        << "\nUnable to open file \"" 
        << cmd.argument(0) << " for reading"
        << " (at " << cmd_data.m_current_location.back() << ")\n" 
        << pop_default_state();
      return;
    }
     
  if(process_commands)
    {
      cmd_data.m_included_files.insert(pfile.m_file_with_path);
      cmd_data.m_current_location.push_back(pfile);
  
      include_file(raw_data.begin(), raw_data.end(), 
                   true, cmd_data);

      cmd_data.m_current_location.pop_back();
      cmd_data.m_included_files.erase(pfile.m_file_with_path);
    }
  else
    {
      cmd_data.current_stream().append(raw_data.begin(), raw_data.end());
    }
  


}

void
FilePacket::
post_process(const CommandData &cmd_data)
{

  typedef const_c_array<std::pair<int, WRATHFormatter::LineData> > eol_array_type;
  typedef eol_array_type::iterator eol_iter_type;


  for(std::vector<LinkEntry>::const_iterator
        iter=cmd_data.m_links.begin(), end=cmd_data.m_links.end();
      iter!=end; ++iter)
    {
      FileData *linkFile;
      range_type<int> R(iter->m_range.m_begin, iter->m_range.m_begin);
      eol_array_type eols(iter->m_stream->formatted_text().eols());
      range_type<eol_iter_type> eol_iter_pair(eols.begin(), eols.end());
      WRATHFormatter::LineData L;

      if(!iter->m_filename.empty())
        {
          linkFile=fetch_file(iter->m_filename, iter->m_type);
        }
      else
        {
          linkFile=NULL;
        }

      L=WRATHStateStream::sub_range(R.m_begin, L, eol_iter_pair);
      WRATHTextAttributePacker::BBox bb;

      for(int i=iter->m_range.m_begin; i<iter->m_range.m_end; ++i)
        {
          if(WRATHStateStream::update_value_from_change(i, L, eol_iter_pair))
            {
              m_font_drawer
                .m_attribute_packer->compute_bounding_box(R,
                                                          iter->m_stream->formatted_text(),
                                                          iter->m_stream->state_stream(),
                                                          bb);
              if(!iter->m_is_quit_link)
                {
                  cmd_data.m_current->add_link(linkFile, bb, iter->m_tag);
                }
              else
                {
                  cmd_data.m_current->add_quit_link(bb);
                }
              R.m_begin=i;
              bb.clear();
            }

          R.m_end=i;
        }
      
      m_font_drawer
        .m_attribute_packer->compute_bounding_box(R,
                                                  iter->m_stream->formatted_text(),
                                                  iter->m_stream->state_stream(),
                                                  bb);
      if(!iter->m_is_quit_link)
        {
          cmd_data.m_current->add_link(linkFile, bb, iter->m_tag);
        }
      else
        {
          cmd_data.m_current->add_quit_link(bb);
        }
    }

  for(std::vector<TagEntry>::const_iterator 
        iter=cmd_data.m_tags.begin(), end=cmd_data.m_tags.end();
      iter!=end; ++iter)
    {
      vec2 pt;

      if(iter->m_stream->formatted_text().data_stream().empty())
        {
          unsigned int L;

          L=std::max(0, iter->m_location);
          L=std::min(L, 
                     static_cast<unsigned int>(iter->m_stream->formatted_text().data_stream().size()-1));

          pt=iter->m_stream->formatted_text().data(L).m_position;
        }
      else
        {
          pt=iter->m_fallback_position;
        }
      cmd_data.m_current->add_jump_tag(iter->m_tag_name, pt);
    }

  for(std::list<CommandData*>::const_iterator
        iter=cmd_data.m_children.begin(), 
        end=cmd_data.m_children.end();
      iter!=end; ++iter)
    {
      CommandData *ptr(*iter);
      post_process(*ptr);
    }
}

void
FilePacket::
include_utf8_general(const Command &cmd, CommandData &cmd_data, 
                     bool process_commands)
{
  std::vector<uint8_t> raw_bytes;

  if(cmd.arguments_empty())
    {
      cmd_data.current_stream().stream() 
        << push_default_state(this)
        << WRATHText::set_color(0xFF, 0x33, 0x33, 0xFF)
        << "\nNo utf8 file specified"
        << " (at " << cmd_data.m_current_location.back() << ")\n" 
        << pop_default_state();
      return;
    }

  stack_data pfile(cmd_data.m_current_location.back(),
                   cmd.argument(0));

  if(process_commands and 
     cmd_data.circular_inclusion(pfile.m_file_with_path))
    {
      cmd_data.current_stream().stream() 
        << push_default_state(this)
        << WRATHText::set_color(0xFF, 0x33, 0x33, 0xFF)
        << "\nCircular inclusion of file \"" 
        << cmd.argument(0) << " detected "
        << " (at " << cmd_data.m_current_location.back() << ")\n" 
        << pop_default_state();
      return;
    }


  if(routine_success==load_file_contents(pfile.m_file_with_path,
                                         raw_bytes))
    {
      //check for BOM marker:
      std::vector<uint8_t>::iterator beg(raw_bytes.begin()), end(raw_bytes.end());
      if(raw_bytes.size()>=3 
         and raw_bytes[0]==0xEF
         and raw_bytes[1]==0xBB
         and raw_bytes[2]==0xBF)
        {
          beg+=3;
        }

      WRATHUTF8<std::vector<uint8_t>::iterator> UTF8(beg, end);
      if(process_commands)
        {  
          cmd_data.m_included_files.insert(pfile.m_file_with_path);
          cmd_data.m_current_location.push_back(pfile);
          
          include_file(UTF8.begin(), UTF8.end(), process_commands, cmd_data);

          cmd_data.m_current_location.pop_back();
          cmd_data.m_included_files.erase(pfile.m_file_with_path);
        }
      else
        {
          cmd_data.current_stream().append(UTF8.begin(), UTF8.end()); 
        }
    }
  else
    {
      cmd_data.current_stream().stream() 
        << push_default_state(this)
        << WRATHText::set_color(0xFF, 0x33, 0x33, 0xFF)
        << "\nUnable to open utf8 file \"" 
        << cmd.argument(0) << " for reading "
        << " (at " << cmd_data.m_current_location.back() << ")\n" 
        << pop_default_state();
    }
}



void
FilePacket::
include_utf16_general(const Command &cmd, CommandData &cmd_data, 
                      bool process_commands)
{

  std::vector<uint16_t> raw_bytes;;

  if(cmd.arguments_empty())
    {
      cmd_data.current_stream().stream() 
        << push_default_state(this)
        << WRATHText::set_color(0xFF, 0x33, 0x33, 0xFF)
        << "\nNo utf16 file specified"
        << " (at " << cmd_data.m_current_location.back() << ")\n" 
        << pop_default_state();
      return;
    }

  stack_data pfile(cmd_data.m_current_location.back(),
                   cmd.argument(0));
  
  if(process_commands and 
     cmd_data.circular_inclusion(pfile.m_file_with_path))
    {
      cmd_data.current_stream().stream() 
        << push_default_state(this)
        << WRATHText::set_color(0xFF, 0x33, 0x33, 0xFF)
        << "\nCircular inclusion of file \"" 
        << cmd.argument(0) << " detected "
        << " (at " << cmd_data.m_current_location.back() << ")\n" 
        << pop_default_state();
      return;
    }


  if(routine_success==load_file_contents(pfile.m_file_with_path, raw_bytes))
    {
      std::vector<uint16_t>::iterator beg(raw_bytes.begin()), end(raw_bytes.end());
      if(!raw_bytes.empty() 
         and (raw_bytes[0]==0xFFFE or raw_bytes[0]==0xFEFF))
        {
          ++beg;
          if(raw_bytes[0]==0xFFFE)
            {
              for(std::vector<uint16_t>::iterator iter=beg; iter!=end; ++iter)
                {
                  uint16_t A, B;
                  A= (*iter) & 0xFF;
                  B= (*iter) >> 8;
                  *iter= (A<<8)|B;
                }
            }
        }

      WRATHUTF16<std::vector<uint16_t>::iterator> UTF16(beg, end);
      if(process_commands)
        {  
          cmd_data.m_included_files.insert(pfile.m_file_with_path);
          cmd_data.m_current_location.push_back(pfile);

          include_file(UTF16.begin(), UTF16.end(), process_commands, cmd_data);

          cmd_data.m_current_location.pop_back();
          cmd_data.m_included_files.erase(pfile.m_file_with_path);
        }
      else
        {
          cmd_data.current_stream().append(UTF16.begin(), UTF16.end());   
        }   
    }
  else
    {
      cmd_data.current_stream().stream() 
        << push_default_state(this)
        << WRATHText::set_color(0xFF, 0x33, 0x33, 0xFF)
        << "\nUnable to open utf16 file \"" 
        << cmd.argument(0) << " for reading "
        << " (at " << cmd_data.m_current_location.back() << ")\n" 
        << pop_default_state();
    }
}

void
FilePacket::
pop_color(const Command&, CommandData &cmd_data)
{
  cmd_data.current_stream().stream() 
    << WRATHText::pop_color();
}

void
FilePacket::
pop_font(const Command&, CommandData &cmd_data)
{
  cmd_data.current_stream().stream() 
    << WRATHText::pop_font();
}

void
FilePacket::
set_background_color(const Command &cmd, CommandData &cmd_data)
{
  
  color_argumentsf value(cmd_data.m_current->background_color()*255.0f);
  cmd.parse_arguments(value);

  cmd_data.m_current->background_color( vec4(value.m_r.m_value,
                                             value.m_g.m_value,
                                             value.m_b.m_value,
                                             value.m_a.m_value)/255.0f);
                                          
}

void
FilePacket::
change_color(const Command &cmd, CommandData &cmd_data, bool push)
{
  WRATHText::color_type current_color;

  cmd_data.current_stream().stream() 
    << WRATHText::get_color(current_color);

  color_arguments value(current_color);
  cmd.parse_arguments(value);

  if(!push)
    {
      cmd_data.current_stream().stream() 
        << WRATHText::set_color(value.m_r.m_value,
                              value.m_g.m_value,
                              value.m_b.m_value,
                              value.m_a.m_value);
    }
  else
    {
      cmd_data.current_stream().stream() 
        << WRATHText::push_color(value.m_r.m_value,
                               value.m_g.m_value,
                               value.m_b.m_value,
                               value.m_a.m_value);
    }
}

void
FilePacket::
pop_font_pixel_size(const Command&, CommandData &cmd_data)
{
  cmd_data.current_stream().stream() 
    << WRATHText::pop_pixel_size();
}


void
FilePacket::
change_font_pixel_size(const Command &cmd, CommandData &cmd_data, bool push)
{
  if(!cmd.arguments_empty())
    {
      float v;

      std::istringstream istr(cmd.argument(0));
      istr >> v;
      if(!istr.fail())
        {
          if(!push)
            {
              cmd_data.current_stream().stream() 
                << WRATHText::set_pixel_size(v);
            }
          else
            {
              cmd_data.current_stream().stream() 
                << WRATHText::push_pixel_size(v);
            }
        }
    }
}


void
FilePacket::
change_font_qt(const Command &cmd, CommandData &cmd_data, bool push)
{
  if(!cmd.arguments_empty())
    {
      /*
        convert '%' to ' ':
       */
      WRATHTextureFont *new_font(NULL);
      int pix_sz(m_default_font->pixel_size());
 
      #ifdef WRATH_QT
        QString fnt_name(convert_percent_to_spaces(cmd.argument(0)).c_str());
        QFont fnt(fnt_name);

        if(cmd.number_arguments()>=2
           and cmd.argument(1)=="italic")
          {
            fnt.setItalic(true);
          }

        new_font=m_fetcher.m_font_via_qt(fnt, pix_sz);
      #else
	WRATHFontFetch::FontProperties in_spec;
	WRATHFontFetch::font_handle out_spec;

        in_spec.family_name(convert_percent_to_spaces(cmd.argument(0)));
        if(cmd.number_arguments()>=2
           and cmd.argument(1)=="italic")
          {
            in_spec.italic(true);
          }

        out_spec=WRATHFontFetch::fetch_font_entry(in_spec);
	if(out_spec.valid())
          {
            new_font=m_fetcher.m_font_via_resource(pix_sz,
                                               out_spec->name(), 
                                               out_spec->face_index());
          }
      #endif

      if(new_font==NULL)
        {
          cmd_data.current_stream().stream() 
            << WRATHText::get_font(new_font);
        }
      else
        {
          execute_on_change_font(new_font);
        }

      if(!push)
        {
          cmd_data.current_stream().stream() 
            << WRATHText::set_font(new_font);
        }
      else
        {
          cmd_data.current_stream().stream() 
            << WRATHText::push_font(new_font);
        }
        
    }
}


void
FilePacket::
change_font_file(const Command &cmd, CommandData &cmd_data, bool push)
{
  if(!cmd.arguments_empty())
    {
      WRATHTextureFont *new_font(NULL);
      int pix_sz, face_index(0);

      pix_sz=m_default_font->pixel_size();

      if(cmd.number_arguments()>2)
        {
          std::istringstream istr(cmd.argument(1));
          istr >> face_index;

          if(istr.fail())
            {
              face_index=0;
            }
        }

      stack_data pfile(cmd_data.m_current_location.back(),
                       convert_percent_to_spaces(cmd.argument(0)));
      std::string filename(WRATHUtil::filename_fullpath(pfile.m_file_with_path));

      new_font=m_fetcher.m_font_via_resource(pix_sz, filename, face_index);

      if(new_font==NULL)
        {
          cmd_data.current_stream().stream() 
            << WRATHText::get_font(new_font);
        }
      else
        {
          execute_on_change_font(new_font);
        }

      if(!push)
        {
          cmd_data.current_stream().stream() 
            << WRATHText::set_font(new_font);
        }
      else
        {
          cmd_data.current_stream().stream() 
            << WRATHText::push_font(new_font);
        }
    }
}

void
FilePacket::
change_font(const Command &cmd, CommandData &cmd_data, bool push)
{
  font_config_arguments font_args;
  WRATHFontFetch::FontProperties in_spec;
  WRATHFontFetch::font_handle out_spec;
  
  
  cmd.parse_arguments(font_args);
  font_args.generate_font_properties(in_spec);

  out_spec=WRATHFontFetch::fetch_font_entry(in_spec);
  if(out_spec.valid()) 
    {
      WRATHTextureFont *new_font(NULL);
      int pix_sz;

      pix_sz=m_default_font->pixel_size();

      stack_data pfile(cmd_data.m_current_location.back(),
                       convert_percent_to_spaces(cmd.argument(0)));
      std::string filename(WRATHUtil::filename_fullpath(out_spec->name()));

      new_font=m_fetcher.m_font_via_resource(pix_sz, filename, out_spec->face_index());

      if(new_font==NULL)
        {
          cmd_data.current_stream().stream() 
            << WRATHText::get_font(new_font);
        }
      else
        {
          execute_on_change_font(new_font);
        }

      if(!push)
        {
          cmd_data.current_stream().stream() 
            << WRATHText::set_font(new_font);
        }
      else
        {
          cmd_data.current_stream().stream() 
            << WRATHText::push_font(new_font);
        }
    }

}



void
FilePacket::
change_formatting(const Command &cmd, CommandData &cmd_data)
{
  vec2 pos;
  WRATHColumnFormatter::LayoutSpecification L;
  change_formatting_type arg_values;
  float r(cmd_data.m_width.second);
  
  pos=cmd_data.new_stream();
  cmd.parse_arguments(arg_values);

  
  if(arg_values.m_width.set_by_command_line())
    {
      r=arg_values.m_width.m_value;
      if(cmd_data.m_width.first)
        {
          r=std::min(r, cmd_data.m_width.second-arg_values.m_left.m_value);
        }
    }
  
  arg_values.m_left.m_value+=cmd_data.m_left;
  r+=arg_values.m_left.m_value;
  
  if(arg_values.m_width.set_by_command_line() or cmd_data.m_width.first)
    {
      L.add_end_line_constraint( WRATHColumnFormatter::Constraint()
                                 .constraint(r));
    }

  enum WRATHFormatter::alignment_type alignment(WRATHFormatter::align_text_begin);
  if(arg_values.m_alignment.m_value=="right")
    {
      alignment=WRATHFormatter::align_text_end;
    }
  else if(arg_values.m_alignment.m_value=="center")
    {
      alignment=WRATHFormatter::align_center;
    }

  L.add_begin_line_constraint( WRATHColumnFormatter::Constraint()
                               .constraint(arg_values.m_left.m_value))
    .start_position(pos)
    .line_spacing(2.0f)
    .alignment(alignment)
    .break_words(arg_values.m_break_words.m_value)
    .eat_white_spaces(!arg_values.m_raw.m_value or cmd_data.m_parent!=NULL);

  cmd_data.current_stream().format(L);
  cmd_data.m_layout=L;
  
}


void
FilePacket::
add_quit_link(const Command&, CommandData &cmd_data)
{
  int mark_begin(0), mark_end(0);

  cmd_data.current_stream().stream() 
    << WRATHText::stream_size(mark_begin)
    << WRATHText::push_state<line_stream_type>(true, underline_stream_id)
    << WRATHText::push_state<line_stream_type>(false, strikethrough_stream_id)
    << WRATHText::push_color(255, 100, 100, 255)
    << "Quit"  
    << WRATHText::pop_color()
    << WRATHText::pop_state<line_stream_type>(strikethrough_stream_id)
    << WRATHText::pop_state<line_stream_type>(underline_stream_id)
    << WRATHText::stream_size(mark_end);
  
  cmd_data.m_links.push_back(LinkEntry()
                             .range(mark_begin, mark_end+1)
                             .stream(cmd_data.current_stream())
                             .is_quit_link(true) );
}

void
FilePacket::
add_back_link(const Command &cmd, CommandData &cmd_data)
{
  std::string linkname;
  int mark_begin(0), mark_end(0);
  

  if(!cmd.arguments_empty())
    {
      linkname=cmd.argument(0);
    }
  else
    {
      linkname="Back";
    }

  cmd_data.current_stream().stream() 
    << WRATHText::stream_size(mark_begin)
    << WRATHText::push_state<line_stream_type>(true, underline_stream_id)
    << WRATHText::push_state<line_stream_type>(false, strikethrough_stream_id)
    << WRATHText::push_color(link_color())
    << linkname  
    << WRATHText::pop_color()
    << WRATHText::pop_state<line_stream_type>(strikethrough_stream_id)
    << WRATHText::pop_state<line_stream_type>(underline_stream_id)
    << WRATHText::stream_size(mark_end);
  
  cmd_data.m_links.push_back(LinkEntry()
                             .filename("")
                             .range(mark_begin, mark_end+1)
                             .stream(cmd_data.current_stream()) );
  
}

void
FilePacket::
add_named_link(const Command &cmd, CommandData &cmd_data)
{
  named_link_arguments args(link_color());
  cmd.parse_arguments(args);

  if(!args.m_filename.set_by_command_line())
    {
      args.m_filename.m_value=cmd_data.m_current_location.back().m_file_without_path;
    }

  if(!args.m_linkname.set_by_command_line())
    {
      args.m_linkname.m_value=args.m_filename.m_value;
    }
  
  LinkEntry link_entry;
  int mark_begin(0), mark_end(0);
  stack_data filename(cmd_data.m_current_location.back(), 
                      args.m_filename.m_value);
  
  cmd_data.current_stream().stream() 
    << WRATHText::stream_size(mark_begin)
    << WRATHText::push_state<line_stream_type>(args.m_underline.m_value, underline_stream_id)
    << WRATHText::push_state<line_stream_type>(false, strikethrough_stream_id)
    << WRATHText::push_color(args.m_r.m_value, args.m_g.m_value,
                           args.m_b.m_value, args.m_a.m_value)
    << args.m_linkname.m_value   
    << WRATHText::pop_color()
    << WRATHText::pop_state<line_stream_type>(strikethrough_stream_id)
    << WRATHText::pop_state<line_stream_type>(underline_stream_id)
    << WRATHText::stream_size(mark_end);
  
  link_entry
    .filename(WRATHUtil::filename_fullpath(filename.m_file_with_path))
    .range(mark_begin, mark_end+1)
    .stream(cmd_data.current_stream());
  
  if(args.m_tagname.set_by_command_line())
    {
      link_entry
        .tag_name(args.m_tagname.m_value);
    }
  
  cmd_data.m_links.push_back(link_entry);
}

void
FilePacket::
add_link(const Command &cmd, CommandData &cmd_data)
{
  
  /*
    Link command is name of file followed 
    by name of link, if no link name follows,
    then use the name of the file as the name
    of the link.
  */
  if(!cmd.arguments_empty())
    {
      std::string linkname;
      int mark_begin(0), mark_end(0);
      stack_data filename(cmd_data.m_current_location.back(), 
                          cmd.argument(0));


      if(cmd.number_arguments()>=2)
        {
          linkname=cmd.argument(1);
        }
      else
        {
          linkname=filename.m_file_without_path;
        }

      cmd_data.current_stream().stream() 
        << WRATHText::stream_size(mark_begin)
        << WRATHText::push_state<line_stream_type>(true, underline_stream_id)
        << WRATHText::push_state<line_stream_type>(false, strikethrough_stream_id)
        << WRATHText::push_color(link_color())
        << linkname  
        << WRATHText::pop_color()
        << WRATHText::pop_state<line_stream_type>(strikethrough_stream_id)
        << WRATHText::pop_state<line_stream_type>(underline_stream_id)
        << WRATHText::stream_size(mark_end);
      
      cmd_data.m_links.push_back(LinkEntry()
                                 .filename(WRATHUtil::filename_fullpath(filename.m_file_with_path))
                                 .range(mark_begin, mark_end+1)
                                 .stream(cmd_data.current_stream()) );
    }
}

void
FilePacket::
add_tag(const Command &cmd, CommandData &cmd_data)
{
  if(cmd.number_arguments()>0)
    {
      int L;
      cmd_data.current_stream().stream() 
        << WRATHText::stream_size(L);

      cmd_data.m_tags.push_back(TagEntry()
                                .tag_name(cmd.argument(0))
                                .location(L)
                                .stream(cmd_data.current_stream())
                                .fallback_position(cmd_data.m_layout.m_start_position));
    }
}

void
FilePacket::
change_line_generic(int stream_id, const Command &cmd, 
                    CommandData &cmd_data, bool push)
{
  bool value(false);

  //get the current value.
  cmd_data.current_stream().stream()
    << WRATHText::get_state<line_stream_type>(value, stream_id);

  value=!value;

  if(!cmd.arguments_empty())
    {
      if(cmd.argument(0)=="on")
        {
          value=true;
        }
      else if(cmd.argument(0)=="off")
        {
          value=false;
        }
    }

  if(!push)
    {
      cmd_data.current_stream().stream()
        << WRATHText::set_state<line_stream_type>(value, stream_id);
    }
  else
    {
      cmd_data.current_stream().stream()
        << WRATHText::push_state<line_stream_type>(value, stream_id);
    }
}

void
FilePacket::
pop_underlining(const Command&, CommandData &cmd_data)
{
  cmd_data.current_stream().stream()
    << WRATHText::pop_state<line_stream_type>(underline_stream_id);
}

void
FilePacket::
pop_strikethrough(const Command&, CommandData &cmd_data)
{
  cmd_data.current_stream().stream()
    << WRATHText::pop_state<line_stream_type>(strikethrough_stream_id);
}


void
FilePacket::
add_image_column(const Command &cmd, CommandData &cmd_data)
{
  WRATHassert(!cmd.arguments_empty());
  WRATHassert(cmd_data.m_width.first);

  WRATHImage *im;

  stack_data pfile(cmd_data.m_current_location.back(),
                   cmd.argument(0));

  std::string filename(WRATHUtil::filename_fullpath(pfile.m_file_with_path));
      
  im=WRATHDemo::fetch_image(filename,
			    WRATHImage::ImageFormat()
			    .internal_format(GL_RGBA)
			    .pixel_data_format(GL_RGBA)
			    .pixel_type(GL_UNSIGNED_BYTE)
			    .magnification_filter(m_magnification_image_filter)
			    .minification_filter(m_minification_image_filter)
			    .automatic_mipmap_generation(!m_manual_mipmap_generation),
			    false);

  if(im==NULL or im->size().x()<=0 or im->size().y()<=0)
    {
      cmd_data.current_stream().stream() 
        << push_default_state(this)
        << WRATHText::set_color(0xFF, 0x33, 0x33, 0xFF)
        << "\nUnable to load image file \"" 
        << cmd.argument(0) << "\""
        << " (at " << cmd_data.m_current_location.back() << ")\n" 
        << pop_default_state();
      return;
        
    }

  //draw the image so that it takes the entire width
  //and the aspect ratio is preserved.
  float aspect, w, h;
  
  w=cmd_data.m_width.second;
  aspect=static_cast<float>(im->size().y())/static_cast<float>(im->size().x());
  h=aspect*w;
  vec2 pos;
  
  pos=cmd_data.m_layout.m_start_position;
  cmd_data.m_layout.start_position(pos + vec2(0,h));
  cmd_data.current_stream().format(cmd_data.m_layout);
  
  vec2 bl(pos.x(), pos.y()+h);
  vec2 tr(pos.x()+w, pos.y());
  vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
  
  cmd_data.m_current->add_image(im, 
                                misc_drawers().m_image_drawer,
                                extra_state().m_image_extra_state,
                                bl, tr,
                                color);
}




void
FilePacket::
add_image(const Command &cmd, CommandData &cmd_data)
{
  WRATHImage *im;
  
  image_argumnets im_args;
  cmd.parse_arguments(im_args);
  
  stack_data pfile(cmd_data.m_current_location.back(),
                   im_args.m_image.m_value);
  
  std::string filename(WRATHUtil::filename_fullpath(pfile.m_file_with_path));
  
  im=WRATHDemo::fetch_image(filename,
			    WRATHImage::ImageFormat()
			    .internal_format(GL_RGBA)
			    .pixel_data_format(GL_RGBA)
			    .pixel_type(GL_UNSIGNED_BYTE)
			    .magnification_filter(m_magnification_image_filter)
			    .minification_filter(m_minification_image_filter)
			    .automatic_mipmap_generation(!m_manual_mipmap_generation),
			    false);
  
  
  if(im==NULL or im->size().x()<1 or im->size().y()<1)
    {
      cmd_data.current_stream().stream() << push_default_state(this)
                                         << WRATHText::set_color(0xFF, 0x33, 0x33, 0xFF)
                                         << "\nUnable to load image file \"" 
                                         << im_args.m_image.m_value << "\""
                                         << " (at " << cmd_data.m_current_location.back() << ")\n" 
                                         << pop_default_state();
      return;
    }
  
  float w(im->size().x()), h(im->size().y());
  
  if(im_args.m_w.set_by_command_line())
    {
      w=im_args.m_w.m_value;
      if(im_args.m_h.set_by_command_line())
        {
          h=im_args.m_h.m_value;
        }
      else
        {
          float aspect;
          
          aspect=static_cast<float>(im->size().y())/static_cast<float>(im->size().x());
          h=w*aspect;
        }
    }

  //change the formatting placing the "pen"
  //position below the image:
  vec2 pos;
  
  pos=cmd_data.new_stream(CommandData::copy_stacks);
  cmd_data.m_layout.start_position(pos + vec2(0,h));
  cmd_data.current_stream().format(cmd_data.m_layout);
  cmd_data.current_stream().stream() << "\n";
  
  vec2 bl(pos.x(), pos.y()+h);
  vec2 tr(pos.x()+w, pos.y());
  vec4 color(im_args.m_r.m_value,
             im_args.m_g.m_value,
             im_args.m_b.m_value,
             im_args.m_a.m_value);
  
  cmd_data.m_current->add_image(im, 
                                misc_drawers().m_image_drawer,
                                extra_state().m_image_extra_state,
                                bl, tr, color);
}



void
FilePacket::
column_format(const Command &cmd, CommandData &cmd_data)
{
  column_format_arguments parsed_args;

  cmd.parse_arguments(parsed_args);

  //end the formatting:
  vec2 pos;
  float y_column_end;

  pos=cmd_data.new_stream(CommandData::copy_stacks);
  y_column_end=pos.y();

  int i, endi, spill_index=-1;
  cmd_fptr spill_column_command(NULL);
  float width_sum;
  enum WRATHFormatter::alignment_type spill_alignment(WRATHFormatter::align_text_begin);

  //find if there is a spill column, only one spill
  //spill column is supported
  for(i=0, endi=parsed_args.number_columns(); 
      i<endi and spill_index==-1; ++i)
    {
      for(std::vector<file_cmd>::iterator 
            iter=m_spill_column_commands.begin(),
            end=m_spill_column_commands.end();
          iter!=end and spill_index==-1; ++iter)
        {
          if(iter->first==parsed_args.column_data(i).m_type.m_value)
            {
              spill_index=i;
              spill_column_command=iter->second;
            }
        }
    }

  std::vector<vec2> column_ends(parsed_args.number_columns(), vec2(0.0f,0.0f));
  std::vector<float> column_begins(parsed_args.number_columns(), 0.0f);

  for(i=0, endi=parsed_args.number_columns(), width_sum=0; i<endi; ++i)
    {
      enum WRATHFormatter::alignment_type alignment(WRATHFormatter::align_text_begin);

      column_begins[i]=width_sum+pos.x(); 
      column_ends[i].x()=column_begins[i]+parsed_args.column_data(i).m_width.m_value;

      if(parsed_args.column_data(i).m_alignment.m_value=="right")
        {
          alignment=WRATHFormatter::align_text_end;
        }
      else if(parsed_args.column_data(i).m_alignment.m_value=="center")
        {
          alignment=WRATHFormatter::align_center;
        }

      //if the column is the spill column
      //then do not handle it here.
      if(i!=spill_index)
        {
          Command child_cmd;
          float w(parsed_args.column_data(i).m_width.m_value);
          CommandData *child_cmd_data;
          vec2 loc(pos);
          bool found_command(false);

          child_cmd.command()=parsed_args.column_data(i).m_type.m_value;
          child_cmd.add_argument(parsed_args.column_data(i).m_file.m_value);

          loc.x()+=width_sum;
          
          child_cmd_data=WRATHNew CommandData(pos, &cmd_data, 
                                            width_sum, w, alignment,
                                            parsed_args.column_data(i).m_break_words.m_value);   

          if(parsed_args.column_data(i).m_reset.m_value)
            {
              child_cmd_data->current_stream().stream() 
                << WRATHText::set_font(m_default_font)
                << WRATHText::set_color(m_default_color)
                << WRATHText::set_pixel_size(m_default_pt_size)
                << WRATHText::set_state<line_stream_type>(false, underline_stream_id)
                << WRATHText::set_state<line_stream_type>(false, strikethrough_stream_id);
            }
          else
            {
              child_cmd_data->current_stream()
                .set_state(cmd_data.current_stream().state_stream());
            }
          child_cmd_data->current_stream().stream() << "\n";
          
          //now see of the type agrees with a known command:
          for(std::vector<file_cmd>::iterator 
                iter=m_accepted_column_commands.begin(),
                end=m_accepted_column_commands.end();
              iter!=end and !found_command; ++iter)
            {
              if(iter->first==parsed_args.column_data(i).m_type.m_value)
                {
                  (this->*iter->second)(child_cmd, *child_cmd_data);
                  found_command=true;
                }
            }
          
          if(!found_command)
            {
              child_cmd_data->current_stream().stream() 
                << "Unknown Column command: " 
                << parsed_args.column_data(i).m_type.m_value;
            }
          
          column_ends[i].y()=child_cmd_data->new_stream().y();

          y_column_end=std::max(y_column_end, column_ends[i].y());
        } //of if(i!=spill_index)
      else
        {
          spill_alignment=alignment;
        }

      width_sum+=parsed_args.column_data(i).m_width.m_value;
      width_sum+=parsed_args.column_data(i).m_space.m_value;
      width_sum+=parsed_args.m_spacing.m_value;
    }
    
  

  if(spill_column_command!=NULL)
    {
      cmd_data.m_layout.m_end_line_constraints.clear();
      cmd_data.m_layout.m_begin_line_constraints.clear();

      /*
        modify column_y_ends[] so that
        they only increase in value as one
        moves away from spill_index.
      */
      for(int k=spill_index-1; k>=0; --k)
        {
          if(k+1!=spill_index)
            {
              column_ends[k].y()=std::max(column_ends[k].y(),
                                          column_ends[k+1].y());
            }

          //also add the restriction:
          cmd_data.m_layout
            .add_begin_line_constraint(WRATHColumnFormatter::Constraint()
                                       .constraint(column_begins[k])
                                       .begin(column_ends[k].y()) );
        }

      for(int k=spill_index+1; k<parsed_args.number_columns(); ++k)
        {
          if(k-1!=spill_index)
            {
              column_ends[k].y()=std::max(column_ends[k].y(),
                                          column_ends[k-1].y());
            }

          //also add the restriction:
          cmd_data.m_layout.add_end_line_constraint(WRATHColumnFormatter::Constraint()
                                                    .constraint(column_ends[k].x())
                                                    .begin(column_ends[k].y()) );
        }

      cmd_data.m_layout
        .add_begin_line_constraint(WRATHColumnFormatter::Constraint()
                                   .constraint(column_begins[spill_index]));

      cmd_data.m_layout
        .add_end_line_constraint(WRATHColumnFormatter::Constraint()
                                 .constraint(column_ends[spill_index].x()) );

  
      

      //now set the formatting to start at pos:  
      cmd_data.m_layout
        .alignment(spill_alignment)
        .break_words(parsed_args.column_data(spill_index).m_break_words.m_value)
        .start_position(column_begins[spill_index], pos.y());
      cmd_data.current_stream().format(cmd_data.m_layout);

      if(parsed_args.column_data(spill_index).m_reset.m_value)
        {
          cmd_data.current_stream().stream() 
            << WRATHText::set_font(m_default_font)
            << WRATHText::set_color(m_default_color)
            << WRATHText::set_pixel_size(m_default_pt_size)
            << WRATHText::set_state<line_stream_type>(false, underline_stream_id)
            << WRATHText::set_state<line_stream_type>(false, strikethrough_stream_id);
        }

      Command spill_cmd;
      spill_cmd.command()=parsed_args.column_data(spill_index).m_type.m_value;
      spill_cmd.add_argument(parsed_args.column_data(spill_index).m_file.m_value);
      (this->*spill_column_command)(spill_cmd, cmd_data);
    }
  else
    {
      pos.y()=y_column_end;
  
      //now set the formatting to start at pos:  
      cmd_data.m_layout.start_position(pos);
      cmd_data.current_stream().format(cmd_data.m_layout);
    }

}


void
FilePacket::
show_font_subrange(const Command &cmd, CommandData &cmd_data)
{
  enum return_code R;
  
  int face_index;
  range_type<int> range;
  std::string font_name;
  WRATHTextureFont *new_font(NULL);

  R=get_show_font_subrange_arguments(cmd.argument(0),
                                     new_font, face_index,
                                     range, font_name);

  if(R!=routine_success)
    {
      return;
    }

  cmd_data.current_stream().stream()
    << "\nFont: " << font_name << "\nface_index=" << face_index
    << " glyphs [" << range.m_begin << ", " << range.m_end << ")\n\n"
    << WRATHText::push_font(new_font);

  glyph_dump(range.m_begin, range.m_end, false, cmd_data);
  
  cmd_data.current_stream().stream() << WRATHText::pop_font();
}

void
FilePacket::
show_font(const Command &cmd, CommandData &cmd_data)
{
  if(cmd.number_arguments()>0)
    {
      WRATHTextureFont *new_font(NULL);
      int pix_sz;
      

      pix_sz=m_default_font->pixel_size();

      stack_data pfile(cmd_data.m_current_location.back(),
                       cmd.argument(0));
      std::string filename(WRATHUtil::filename_fullpath(pfile.m_file_with_path));

      new_font=m_fetcher.m_font_via_resource(pix_sz, filename, 0);
      if(new_font!=NULL)
        {

          WRATHFreeTypeSupport::LockableFace::handle temp_face;
          int num_faces(1);

          temp_face=WRATHFreeTypeSupport::load_face(filename, 0);
          execute_on_change_font(new_font);

          if(temp_face.valid())
            {
              num_faces=temp_face->face()->num_faces;
              temp_face=WRATHFreeTypeSupport::LockableFace::handle();
            }

          cmd_data.current_stream().stream() << WRATHText::push_font(new_font);
          glyph_dump(32, 127, true, cmd_data);            
          cmd_data.current_stream().stream() << WRATHText::pop_font();

          for(int i=0; i<num_faces; ++i)
            {
              new_font=m_fetcher.m_font_via_resource(pix_sz, filename, i);
              if(new_font!=NULL)
                {
                  if(num_faces>1)
                    {
                      cmd_data.current_stream().stream() << "\n\nFace #" << i;
                    }

                  for(int L=0, endL=new_font->number_glyphs(); L<endL; L+=128)
                    {
                      range_type<int> range(L, std::min(L+128, endL));
                      int mark_begin(0), mark_end(0);
                 
                      cmd_data.current_stream().stream() 
                        << WRATHText::stream_size(mark_begin)
                        << WRATHText::push_state<line_stream_type>(true, underline_stream_id)
                        << WRATHText::push_state<line_stream_type>(false, strikethrough_stream_id)
                        << WRATHText::push_color(link_color()) << "\n\t";
                      
                      if(num_faces>1)
                        {
                          cmd_data.current_stream().stream() << "\t";
                        }

                      cmd_data.current_stream().stream() 
                        << "Glyphs[" << range.m_begin << ","
                        << range.m_end << ")"
                        << WRATHText::pop_color()
                        << WRATHText::pop_state<line_stream_type>(strikethrough_stream_id)
                        << WRATHText::pop_state<line_stream_type>(underline_stream_id)
                        << WRATHText::stream_size(mark_end);
                      
                      std::string pname;
                      pname=set_show_font_subrange_arguments(new_font, i, range, filename);

                      cmd_data.m_links.push_back(LinkEntry()
                                                 .filename(pname)
                                                 .range(mark_begin, mark_end+1)
                                                 .stream(cmd_data.current_stream())
                                                 .type(FileType::load_font_subrange));
                    }
                }
            }
        }
      else
        {
          cmd_data.current_stream().stream() 
            << push_default_state(this)
            << WRATHText::set_color(0xff, 0x33, 0x33, 0xff)
            << "\n\"" << cmd.argument(0) <<"\" is not a font file format supported\n"
            << pop_default_state();
        }
    }
}

void
FilePacket::
glyph_dump(int begin, int end, bool character_codes, CommandData &cmd_data)
{
  WRATHTextureFont *font(NULL);  

  cmd_data.current_stream().stream()
    << WRATHText::get_font(font);

  if(font==NULL)
    {
      return;
    }

  for(int I=begin; I<end; ++I)
    {
      WRATHTextureFont::glyph_index_type gl;
      
      if(character_codes)
        {
          WRATHTextureFont::character_code_type cl(I);
          gl=font->glyph_index(cl);
        }
      else
        {
          float current_scale(1.0f);

          gl=WRATHTextureFont::glyph_index_type(I);
          cmd_data.current_stream().stream() 
            << WRATHText::get_scale(current_scale);
          
          cmd_data.current_stream().stream()
            << WRATHText::push_font(m_default_font)
            << WRATHText::push_color(link_color_for_file_browser(FileType::load_font))
            << WRATHText::push_scale(current_scale*0.5)
            << gl.value() << ":"
            << WRATHText::pop_scale()
            << WRATHText::pop_color()
            << WRATHText::pop_font();
        }
      
      const WRATHTextureFont::glyph_data_type &ch(font->glyph_data(gl));
      
      if(ch.glyph_index().valid() and ch.texel_size()!=ivec2(0,0))
        {
          cmd_data.current_stream().stream() << gl << " ";
          if(ch.texel_size().x() < ch.advance().x())
            {
              cmd_data.current_stream().stream() << " ";
            }
        }
    }
}


void
FilePacket::
glyph_dump(const Command &cmd, CommandData &cmd_data)
{
  WRATHTextureFont *font(NULL);  

  cmd_data.current_stream().stream()
    << WRATHText::get_font(font);

  
  if(font!=NULL)
    {
      glyph_dump_arguments args;

      cmd.parse_arguments(args);

      if(!args.m_use_character_codes.m_value)
        {
          args.m_end.m_value=std::min(args.m_end.m_value,
                                      font->number_glyphs());
        }
     
      glyph_dump(args.m_start.m_value, args.m_end.m_value,
                 args.m_use_character_codes.m_value, cmd_data);
      
     
    }
}


void
FilePacket::
spill(const Command&, CommandData &cmd_data)
{
  cmd_data.m_is_spill=true;
}


void
FilePacket::
begin_sub_super_script(const Command &cmd, CommandData &cmd_data, 
                       bool negate, float initial_offset_value)
{
  sub_super_scrips_arguments args(initial_offset_value);
  float current_scale(1.0f);

  cmd.parse_arguments(args);
  
  cmd_data.current_stream().stream() 
    << WRATHText::get_scale(current_scale);

  if(args.m_offset_is_relative.m_value)
    {
      float font_height(0.0f);
      WRATHTextureFont *font(NULL);
      
      cmd_data.current_stream().stream() 
        << WRATHText::get_font(font);

      if(font!=NULL)
        {
          //get the last character, if the last
          //character is a space then use 
          //font->new_line_height() to guess the offset
          //otherwise use the height of the last character.

          font_height=font->new_line_height();
          if(args.m_use_previous_char_info.m_value
             and !cmd_data.current_stream().raw_text().character_data().empty())
            {
              WRATHTextData::character ch;
              WRATHTextureFont::glyph_index_type gl;

              ch=cmd_data.current_stream().raw_text().character_data().back();
              gl=ch.glyph_index();
              if(!gl.valid())
                {
                  gl=font->glyph_index(ch.character_code());
                }
              const WRATHTextureFont::glyph_data_type &gl_data(font->glyph_data(gl));
              if(gl_data.glyph_index().valid() and
                 gl_data.texel_size()!=ivec2(0,0))
                {
                  font_height=gl_data.bounding_box_size().y() + gl_data.origin().y();
                }
            }
        }
      
     
      args.m_offset.m_value*=font_height*current_scale;

      if(!args.m_use_previous_char_info.m_value)
        {
          args.m_offset.m_value*=args.m_scale_font_factor.m_value;
        }
    }

  if(negate)
    {
      args.m_offset.m_value=-args.m_offset.m_value;
    }

  float previous_value;
  cmd_data.current_stream().stream() 
    << WRATHText::get_baseline_shift_y(previous_value);

  cmd_data.current_stream().stream() 
    << WRATHText::push_scale(current_scale*args.m_scale_font_factor.m_value)
    << WRATHText::push_baseline_shift_y(previous_value+args.m_offset.m_value);
}

void
FilePacket::
begin_sub_script(const Command &cmd, CommandData &cmd_data)
{
  begin_sub_super_script(cmd, cmd_data, false, 0.4f);
}

void
FilePacket::
begin_super_script(const Command &cmd, CommandData &cmd_data)
{
  begin_sub_super_script(cmd, cmd_data, true, 0.75f);
}

void
FilePacket::
end_sub_super_script(const Command&, CommandData &cmd_data)
{

  cmd_data.current_stream().stream() 
    << WRATHText::pop_scale()
    << WRATHText::pop_baseline_shift_y();
}

void
FilePacket::
include_dir(const Command &cmd, CommandData &cmd_data)
{

  if(cmd.number_arguments()>0 and !cmd.argument(0).empty())
    {
      DIR *ptr;
      std::string filename(cmd.argument(0));

      if(*filename.rbegin()!='/')
        {
          filename.push_back('/');
        }

      if(filename[0]!='/')
        {
          if(cmd_data.m_current_location.back().m_file_path.empty()
             or *cmd_data.m_current_location.back().m_file_path.rbegin()=='/')
            {
              filename=cmd_data.m_current_location.back().m_file_path
                + filename;
            }
          else
            {
              filename=cmd_data.m_current_location.back().m_file_path
                + "/" + filename;
            }
          filename=WRATHUtil::filename_fullpath(filename);
        }

      ptr=::opendir(filename.c_str());
      if(ptr!=NULL)
        {
          include_dir(cmd_data, ptr, filename);
          ::closedir(ptr);
        }
      else
        {
          cmd_data.current_stream().stream() 
            << push_default_state(this)
            << WRATHText::set_color(0xff, 0x33, 0x33, 0xff)
            << "\nUnable to open directory \"" 
            << cmd.argument(0) << " for reading"
            << " (at " << cmd_data.m_current_location.back() << ")\n" 
            << pop_default_state();
        }
      
      cmd_data.current_stream().stream() << "\n";
    }
  else
    {
      cmd_data.current_stream().stream() 
        << push_default_state(this)
        << WRATHText::set_color(0xff, 0x33, 0x33, 0xff)
        << "directory_listing command with no path given "
        <<  " (at " << cmd_data.m_current_location.back() << ")\n" 
        << pop_default_state();
    }
}

void
FilePacket::
include_dir(CommandData &cmd_data, 
            ::DIR *in_ptr, 
            const std::string &path)
{
  if(in_ptr==NULL)
    {
      return;
    }

  std::vector< std::pair<bool, std::pair<std::string, std::string> > > files;
  bool is_root(path=="/");

  for(struct dirent *current=::readdir(in_ptr);
      current!=NULL; current=::readdir(in_ptr))
    {
      if(!std::strcmp(current->d_name,"."))
        {
          continue;
        }

      if(is_root and !std::strcmp(current->d_name,".."))
        {
          continue;
        }

      DIR *ptr(NULL);
      std::string absolute_filename, relative_filename;
      bool is_directory(false);

      relative_filename=current->d_name;
      absolute_filename=path+relative_filename;

      ptr=::opendir(absolute_filename.c_str());
      if(ptr!=NULL)
        {
          is_directory=true;
          ::closedir(ptr);
          absolute_filename.push_back('/');
          relative_filename.push_back('/');
        }

      files.push_back(std::make_pair(!is_directory,
                                     std::make_pair(relative_filename, absolute_filename))); 

    }

  std::sort(files.begin(), files.end());

  for(std::vector< std::pair<bool, std::pair<std::string, std::string> > >::iterator 
        iter=files.begin(), end=files.end(); iter!=end; ++iter)
    {
      std::string &absolute_filename(iter->second.second);
      std::string &relative_filename(iter->second.first);

      file_fetch_type file_type(FileType::load_raw);
      int mark_begin(0), mark_end(0);

      if(!iter->first)
        {
          file_type=FileType::load_directory;
        }
      else
        {
          std::string ext(WRATHUtil::filename_extension(absolute_filename));
          file_type=file_type_from_file_ext(ext);                          
        }

      cmd_data.current_stream().stream() 
        << "\n"
        << WRATHText::stream_size(mark_begin)
        << WRATHText::push_state<line_stream_type>(false, underline_stream_id)
        << WRATHText::push_state<line_stream_type>(false, strikethrough_stream_id)
        << WRATHText::push_color(link_color_for_file_browser(file_type))
        << relative_filename 
        << WRATHText::pop_color()
        << WRATHText::pop_state<line_stream_type>(strikethrough_stream_id)
        << WRATHText::pop_state<line_stream_type>(underline_stream_id)
        << WRATHText::stream_size(mark_end);

      cmd_data.m_links.push_back(LinkEntry()
                                 .filename(absolute_filename)
                                 .range(mark_begin, mark_end+1)
                                 .type(file_type)
                                 .stream(cmd_data.current_stream()) );

    }
} 

void
FilePacket::
set_tess_params(const Command &cmd, CommandData &cmd_data)
{
  tess_params_argc args(cmd_data.m_tess_params);
  cmd.parse_arguments(args);
}


void
FilePacket::
create_shape(const Command &in_cmd, CommandData &cmd_data)
{
  shape_params S;
  std::string filtered_string(in_cmd.original_string());

  std::replace(filtered_string.begin(), filtered_string.end(), '(', ' ');
  std::replace(filtered_string.begin(), filtered_string.end(), ')', ' ');
  std::replace(filtered_string.begin(), filtered_string.end(), ',', ' ');
  
  Command cmd;
  create_command_from_string(filtered_string, cmd);

  if(cmd.number_arguments()==0)
    {
      return;
    }

  /*
    read geometry data from cmd
   */
  if(cmd.number_arguments()>1)
    {
      const_c_array<std::string> str_ptr;
      
      str_ptr=const_c_array<std::string>(&cmd.argument(1), cmd.number_arguments()-1);
      S.generate(str_ptr);
    }
  

  WRATHShapeF *pnewShape=WRATHNew WRATHShapeF();
  pnewShape->label(cmd.argument(0));

  for(std::list<std::vector<point_packet> >::const_iterator iter=S.m_outlines.begin(), 
        end=S.m_outlines.end(); iter!=end; ++iter)
    {
      pnewShape->new_outline();       
      for(std::vector<point_packet>::const_iterator i=iter->begin(),
            e=iter->end(); i!=e; ++i)
        {
          pnewShape->current_outline() << i->m_pt;

          if(i->m_arc_mode!=point_packet::no_arc)
            {
              bool is_ccw(i->m_arc_mode==point_packet::ccw_arc);
              pnewShape->current_outline().to_arc(i->m_angle*M_PI/180.0f, is_ccw);
            }
          else
            {

              for(std::vector<vec2>::const_iterator c=i->m_control_points.begin(),
                    ce=i->m_control_points.end(); c!=ce; ++c)
                {
                  pnewShape->current_outline() << WRATHOutlineF::control_point(*c);
                }
            }
        }
    }

  per_shape_data *pShapeData;
  
  pShapeData=WRATHNew per_shape_data(pnewShape, cmd_data.m_tess_params);
  cmd_data.add_shape(cmd.argument(0), pShapeData);
  m_shapes.push_back(pShapeData);
}

void
FilePacket::
add_filled_shape(const Command &cmd, CommandData &cmd_data)
{
  per_shape_data *shape;
  WRATHShapeTriangulatorPayload::handle payload;
  vec2 pos;

  stroke_params_args args;

  cmd.parse_arguments(args);
  args.set_params();

  shape=cmd_data.get_shape(args.m_shape.m_value);

 
  if(shape==NULL)
    {
      return;
    }
  

  vec2 center, sz;
  WRATHBBox<2> pbox;
  WRATHShapeSimpleTessellatorPayload::handle tessed;
  
  tessed=shape->tessellated_data();
  WRATHassert(tessed.valid());
  
  pbox=tessed->bounding_box();
  if(pbox.empty())
    {
      return;
    }

  center=0.5f*(pbox.min_corner() + pbox.max_corner());
  sz=pbox.max_corner() - pbox.min_corner();
  
  payload=shape->fill_data();
  WRATHassert(payload.valid());

  pos=cmd_data.new_stream(CommandData::copy_stacks);
  cmd_data.m_layout.start_position(pos + sz);
  cmd_data.current_stream().format(cmd_data.m_layout);
  cmd_data.current_stream().stream() << "\n";

  
  vecN<GLubyte,4> current_color(m_default_color);
  cmd_data.current_stream().stream() << WRATHText::get_color(current_color);
  vec4 color(static_cast<float>(current_color.x())/255.0f,
             static_cast<float>(current_color.y())/255.0f,
             static_cast<float>(current_color.z())/255.0f,
             static_cast<float>(current_color.w())/255.0f);
  
  pbox.translate(-center);
  cmd_data.m_current->add_shape(shape->shape(), 
                                m_filled_shape_packer, 
                                payload, 
                                WRATHDefaultFillShapeAttributePacker::FillingParameters(-center),
                                misc_drawers().m_filled_shape_drawer, 
                                extra_state().m_filled_shape_extra_state,
                                pos+0.5f*sz, color, pbox);

  /*
    
    // For kicks, show the zero_fill rule below

  pos=cmd_data.new_stream(CommandData::copy_stacks);
  cmd_data.m_layout.start_position(pos + sz);
  cmd_data.current_stream().format(cmd_data.m_layout);
  cmd_data.current_stream().stream() << "\n";
  cmd_data.m_current->add_shape(shape->shape(), 
                                m_filled_shape_packer, 
                                payload, 
                                WRATHDefaultFillShapeAttributePacker::FillingParameters(-center,
                                                                                        zero_fill),
                                misc_drawers().m_filled_shape_drawer, 
                                extra_state().m_filled_shape_extra_state,
                                pos+0.5f*sz, color, pbox);


    // For kicks, show both the zero_fill and non_zero fill             
    // at the same place with different colors
  
  pos=cmd_data.new_stream(CommandData::copy_stacks);
  cmd_data.m_layout.start_position(pos + sz);
  cmd_data.current_stream().format(cmd_data.m_layout);
  cmd_data.current_stream().stream() << "\n";
  cmd_data.m_current->add_shape(shape->shape(), 
                                m_filled_shape_packer, 
                                payload, 
                                WRATHDefaultFillShapeAttributePacker::FillingParameters(-center),
                                misc_drawers().m_filled_shape_drawer, 
                                extra_state().m_filled_shape_extra_state,
                                pos+0.5f*sz, vec3(1.0f, 0.0f, 1.0f), pbox);
  cmd_data.m_current->add_shape(shape->shape(), 
                                m_filled_shape_packer, 
                                payload, 
                                WRATHDefaultFillShapeAttributePacker::FillingParameters(-center,
                                                                                        zero_fill),
                                misc_drawers().m_filled_shape_drawer, 
                                extra_state().m_filled_shape_extra_state,
                                pos+0.5f*sz, vec3(0.0f, 1.0f, 1.0f), pbox);
  */
  

}

void
FilePacket::
add_stroked_shape(const Command &cmd, CommandData &cmd_data)
{
  
  per_shape_data *shape;
  WRATHShapePreStrokerPayload::handle payload;
  vec2 pos;

  stroke_params_args args;

  cmd.parse_arguments(args);
  args.set_params();

  shape=cmd_data.get_shape(args.m_shape.m_value);
  if(shape==NULL)
    {
      return;
    }
  

  vec2 center, sz;
  WRATHBBox<2> pbox;
  WRATHShapeSimpleTessellatorPayload::handle tessed;

  tessed=shape->tessellated_data();
  WRATHassert(tessed.valid());

  
  pbox=tessed->bounding_box();
  if(pbox.empty())
    {
      return;
    }
  center=0.5f*(pbox.min_corner() + pbox.max_corner());
  sz=pbox.max_corner() - pbox.min_corner();
  
  args.m_params.m_translate=-center;

  payload=shape->pre_stroke_data();
  WRATHassert(payload.valid());

  pos=cmd_data.new_stream(CommandData::copy_stacks);
  cmd_data.m_layout.start_position(pos + sz);
  cmd_data.current_stream().format(cmd_data.m_layout);
  cmd_data.current_stream().stream() << "\n";

  
  vecN<GLubyte,4> current_color(m_default_color);
  cmd_data.current_stream().stream() << WRATHText::get_color(current_color);
  vec4 color(static_cast<float>(current_color.x())/255.0f,
             static_cast<float>(current_color.y())/255.0f,
             static_cast<float>(current_color.z())/255.0f,
             static_cast<float>(current_color.w())/255.0f);

  pbox.translate(-center);
  cmd_data.m_current->add_shape(shape->shape(), 
                                m_stroked_shape_packer, 
                                payload, 
                                args.m_params,
                                misc_drawers().m_stroked_shape_drawer, 
                                extra_state().m_stroked_shape_extra_state,
                                pos+0.5f*sz, color, pbox);
}

void
FilePacket::
add_distance_image(const Command &cmd, CommandData &cmd_data)
{
  image_argumnets im_args;
  WRATHImage *im;

  cmd.parse_arguments(im_args);
  im=cmd_data.get_distance_field(im_args.m_image.m_value);

  if(im==NULL)
    {
      cmd_data.current_stream().stream() 
                << push_default_state(this)
                << WRATHText::set_color(0xFF, 0x33, 0x33, 0xFF)
                << "\nNo distance field with name \"" 
                << im_args.m_image.m_value << "\""
                << " (at " << cmd_data.m_current_location.back() << ")\n" 
                << pop_default_state();
      return;
    }

  float w(im->size().x()), h(im->size().y());

  if(im_args.m_w.set_by_command_line())
    {
      w=im_args.m_w.m_value;
      if(im_args.m_h.set_by_command_line())
        {
          h=im_args.m_h.m_value;
        }
      else
        {
          float aspect;
          
          aspect=static_cast<float>(im->size().y())/static_cast<float>(im->size().x());
          h=w*aspect;
        }
    }

  //change the formatting placing the "pen"
  //position below the image:
  vec2 pos;
  
  pos=cmd_data.new_stream(CommandData::copy_stacks);
  cmd_data.m_layout.start_position(pos + vec2(0,h));
  cmd_data.current_stream().format(cmd_data.m_layout);
  cmd_data.current_stream().stream() << "\n";
  
  vec2 bl(pos.x(), pos.y());
  vec2 tr(pos.x()+w, pos.y()+h);
  vec4 color(im_args.m_r.m_value,
             im_args.m_g.m_value,
             im_args.m_b.m_value,
             im_args.m_a.m_value);
  
  cmd_data.m_current->add_image(im, 
                                misc_drawers().m_distance_field_drawer,
                                extra_state().m_distance_field_extra_state,
                                bl, tr, color);


}


void
FilePacket::
create_distance_field(const Command &cmd, CommandData &cmd_data)
{
  per_shape_data *shape;
  create_distance_field_arguments args;

  cmd.parse_arguments(args);

  shape=cmd_data.get_shape(args.m_shape.m_value);
  if(shape==NULL)
    {
      return;
    }

  WRATHImage *pImage;
  std::string name("DIST::??"+args.m_name.m_value);
  ivec2 dims(args.m_width.m_value,
             args.m_height.m_value);

  pImage=WRATHNew WRATHImage(name, dims,
                             WRATHImage::ImageFormat()
                             .internal_format(GL_ALPHA)
                             .pixel_data_format(GL_ALPHA)
                             .pixel_type(GL_UNSIGNED_BYTE)
                             .magnification_filter(GL_LINEAR)
                             .minification_filter(GL_LINEAR));

  cmd_data.add_distance_field(args.m_name.m_value, pImage);

  m_root_container->triple_buffer_enabler()->
    schedule_rendering_action(boost::bind(&FilePacket::actual_distance_field_generation, this,
                                          shape, dims, args.m_pixel_dist.m_value,
                                          pImage,
                                          args.m_skip_corners.m_value,
                                          args.m_use_point_sprites.m_value));
}

void
FilePacket::
actual_distance_field_generation(per_shape_data *shape,
                                 ivec2 dims, float pixel_dist,
                                 WRATHImage *pImage,
                                 bool skip_corners, bool use_point_sprites)
{
  
  
  WRATHShapeGPUDistanceFieldCreator::DistanceFieldTarget::handle dest;

  
  dest=WRATHNew WRATHShapeGPUDistanceFieldCreator::DistanceFieldTarget_WRATHImage(pImage);

  
  enum WRATHShapeGPUDistanceFieldCreator::corner_point_handling_type pp(WRATHShapeGPUDistanceFieldCreator::skip_points);

  if(!skip_corners)
    {
      pp=use_point_sprites?
        WRATHShapeGPUDistanceFieldCreator::use_point_sprites:
        WRATHShapeGPUDistanceFieldCreator::use_triangle_fans;
    }
  
  WRATHShapeGPUDistanceFieldCreator::generate_distance_field(shape->tessellated_data(), 
                                                             dims, pixel_dist,
                                                             m_scratch, dest, pp);

}

