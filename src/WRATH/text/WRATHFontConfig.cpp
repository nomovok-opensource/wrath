/*! 
 * \file WRATHFontConfig.cpp
 * \brief file WRATHFontConfig.cpp
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
#include "WRATHFontConfig.hpp"
#include "ostream_utility.hpp"
#include "WRATHStaticInit.hpp"

namespace
{

  template<typename T>
  class FcMagicValue
  {
  public:
    FcMagicValue(void):
      m_value(),
      m_exists(false)
    {}

    T m_value;
    bool m_exists;
  };

  class FontEntry
  {
  public:
    FontEntry(void):
      m_face_index(0)
    {}

    std::string m_filename;
    int m_face_index;
  };

  void
  FcGetMagicValue(FcPattern*p, const char *label, FcMagicValue<FcChar8*> &v, int n=0)
  {
    v.m_exists= (FcResultMatch==FcPatternGetString(p, label, n, &v.m_value) );
  }
  
  void
  FcGetMagicValue(FcPattern*p, const char *label, FcMagicValue<bool> &v, int n=0)
  {
    FcBool b;

    v.m_exists= (FcResultMatch==FcPatternGetBool(p, label, n, &b) );
    v.m_value=(b==FcTrue);
  }
  
  void
  FcGetMagicValue(FcPattern *p, const char *label, FcMagicValue<int> &v, int n=0)
  {
    v.m_exists= (FcResultMatch==FcPatternGetInteger(p, label, n, &v.m_value) );
  }

  void
  FcGetMagicValue(FcPattern *p, const char *label, FcMagicValue<FcLangSet*> &v, int n=0)
  {
    v.m_exists= (FcResultMatch==FcPatternGetLangSet(p, label, n, &v.m_value) );
  }

  
  template<typename T, typename S>
  void
  GetPairFromFcMagicValue(S &out_value,
                          const FcMagicValue<T> &in_value)
  {
    if(in_value.m_exists)
      {
        out_value=S(in_value.m_value);
      }
  }

  void
  GetPairFromFcMagicValue(std::string &out_value,
                          const FcMagicValue<FcChar8*> &in_value)
  {
    if(in_value.m_exists)
      {
        const char *str(reinterpret_cast<const char*>(in_value.m_value));
        out_value=str;
      }
  }

  template<typename T, typename S>
  void
  GetPairFromFcMagicValue(std::pair<bool, S> &out_value,
                          const FcMagicValue<T> &in_value)
  {
    out_value.first=in_value.m_exists;
    GetPairFromFcMagicValue(out_value.second, in_value);
  }


  template<typename T>
  std::ostream&
  operator<<(std::ostream &ostr, const FcMagicValue<T> &obj)
  {
    if(obj.m_exists)
      {
        ostr << ":" << obj.m_value;
      }
    else
      {
        ostr << ":InvalidFetch";
      }
    return ostr;
  }

  
  void
  FcPatternHelper(FcPattern *p, const std::pair<bool, std::string> &v, const char *label)
  {
    if(v.first)
      {
        FcPatternAddString(p, label,
                           reinterpret_cast<const FcChar8*>(v.second.c_str()));
      }
  }

  void
  FcPatternHelper(FcPattern *p, const std::pair<bool, int> &v, const char *label)
  {
    if(v.first)
      {
        FcPatternAddInteger(p, label, v.second);
      }
  }

  template<typename T>
  class infont_print_helper
  {
  public:
    const std::pair<bool, T> &m_v;
    const char *m_label;

    infont_print_helper(const std::pair<bool, T> &v, const char *l):
      m_v(v),
      m_label(l)
    {}

    friend
    std::ostream&
    operator<<(std::ostream &ostr, const infont_print_helper &obj)
    {
      if(obj.m_v.first)
        {
          ostr << obj.m_label << ":\"" << obj.m_v.second << "\" ";  
        }
      return ostr;
    }
    
  };

  void
  print_pretty_formatted(std::ostream &ostr, const WRATHFontConfig::FontSpecification &v)
  {
    const WRATHFontConfig::InFontSpecification &obj(v.m_fontconfig_details);
    std::ostringstream oprefix;
    std::string prefix;

    oprefix << "\n[FontConfig] ("
            << v->name()
            << ", " 
            << v->face_index()
            << "):";
    prefix=oprefix.str();

    ostr << prefix << infont_print_helper<std::string>(obj.m_family_name, FC_FAMILY)
         << prefix << infont_print_helper<std::string>(obj.m_foundary_name, FC_FOUNDRY)
         << prefix << infont_print_helper<std::string>(obj.m_style, FC_STYLE)
         << prefix << infont_print_helper<int>(obj.m_weight, FC_WEIGHT)
         << prefix << infont_print_helper<int>(obj.m_slant, FC_SLANT)
         << prefix << " font-family: \"" << v->properties().m_family_name << "\""
         << prefix << " font-foundry: \"" << v->properties().m_foundry_name << "\""
         << prefix << " font-style: \"" << v->properties().m_style_name << "\""
         << prefix << " bold-italic: \"" << vec2(v->properties().m_bold, v->properties().m_italic)<< "\"";
    
    if(!obj.m_languages.empty())
      {
        ostr << prefix << FC_LANG << ":" << "{";
        for(std::set<std::string>::iterator iter=obj.m_languages.begin(),
              end=obj.m_languages.end(); iter!=end; ++iter)
          {
            if(iter!=obj.m_languages.begin())
              {
                ostr << ", ";
              }
            ostr << "\"" << *iter << "\"";
          }
        ostr << "}";
      }
    ostr << "\n[FontConfig]\n";
  }

  class font_config_magic_class
  {
  public:
    typedef WRATHFontConfig::FontList FontList;

    font_config_magic_class(void);
    ~font_config_magic_class();

    WRATHFontFetch::font_handle
    fetch_font_entry(const WRATHFontConfig::InFontSpecification &spec);

    const FontList&
    font_list(void) const
    {
      return m_font_list;
    }

  private:
    void
    add_entry(FcPattern *p);

    WRATHMutex m_fc_mutex;
    FcFontSet *m_fc_font_list;
    FontList m_font_list;
  };


  font_config_magic_class&
  font_config_magic(void)
  {
    WRATHStaticInit();
    static font_config_magic_class R;
    return R;
  }

}

/////////////////////////////////
// font_config_magic_class methods
font_config_magic_class::
font_config_magic_class(void)
{
  FcPattern *fc_pattern; 
  FcObjectSet *fc_object_set; 
  

  /*
    require those fonts that are both scalable
    and outline fonts.
  */
  fc_pattern=FcPatternCreate();
  
  FcPatternAddBool(fc_pattern, FC_OUTLINE, FcTrue);
  FcPatternAddBool(fc_pattern, FC_SCALABLE, FcTrue);
  
  
  /*
    add the properties we care about to fc_object_set
  */
  const char *fc_properties[]=
    {
      FC_FAMILY, FC_WEIGHT, FC_SLANT,
      FC_FILE, FC_INDEX, FC_FOUNDRY, 
      FC_SCALABLE, FC_OUTLINE,
      FC_LANG, FC_STYLE,
      NULL
    };
  
  fc_object_set=FcObjectSetCreate();
  for(const char **iter=fc_properties; *iter!=NULL; ++iter)
    {
      FcObjectSetAdd(fc_object_set, *iter);
    } 
  
  /*
    get a list of fonts from the default
    font configuration which are scalable
    outline fonts.
  */
  m_fc_font_list=FcFontList(NULL, fc_pattern, fc_object_set);
  
  for(int i=0; i<m_fc_font_list->nfont; ++i)
    {
      add_entry(m_fc_font_list->fonts[i]);
    }
  FcPatternDestroy(fc_pattern);
  FcObjectSetDestroy(fc_object_set);

  /*
  for(FontList::const_iterator iter=m_font_list.begin(),
        end=m_font_list.end(); iter!=end; ++iter)
    {
      print_pretty_formatted(std::cout, iter->second);
    }
  */
}

void
font_config_magic_class::
add_entry(FcPattern *p)
{
  FcMagicValue<FcChar8*> family_name, file_name, foundary_name, font_style;
  FcMagicValue<int> font_index, font_weight, font_slant;
  FcMagicValue<FcLangSet*> lang;
  
  FcGetMagicValue(p, FC_FILE, file_name);
  if(file_name.m_exists)
    {
      FontEntry entry_key;
      WRATHFontConfig::FontSpecification entry;
      
      GetPairFromFcMagicValue(entry_key.m_filename, file_name);
      
      FcGetMagicValue(p, FC_INDEX, font_index);
      GetPairFromFcMagicValue(entry_key.m_face_index, font_index);
      
      entry.font()=WRATHFontDatabase::fetch_font_entry(entry_key.m_filename, entry_key.m_face_index);
      
      FcGetMagicValue(p, FC_FAMILY, family_name);
      GetPairFromFcMagicValue(entry.m_fontconfig_details.m_family_name, family_name);
      
      FcGetMagicValue(p, FC_FOUNDRY, foundary_name);
      GetPairFromFcMagicValue(entry.m_fontconfig_details.m_foundary_name, foundary_name);
      
      FcGetMagicValue(p, FC_WEIGHT, font_weight);
      GetPairFromFcMagicValue(entry.m_fontconfig_details.m_weight, font_weight);
      
      FcGetMagicValue(p, FC_SLANT, font_slant);
      GetPairFromFcMagicValue(entry.m_fontconfig_details.m_slant, font_slant);
      
      FcGetMagicValue(p, FC_STYLE, font_style);
      GetPairFromFcMagicValue(entry.m_fontconfig_details.m_style, font_style);
      
      #if (FC_MAJOR>=2 && FC_MINOR>=7) || (FC_MAJOR>=3)   
      {        
        FcGetMagicValue(p, FC_LANG, lang);
        if(lang.m_exists)
          {
            FcStrSet *langs_as_strs;
            FcStrList *iter;
            FcChar8 *current;
            
            langs_as_strs=FcLangSetGetLangs(lang.m_value);
            iter=FcStrListCreate(langs_as_strs);
            
            for(current=FcStrListNext(iter); current; current=FcStrListNext(iter))
              {
                const char *str(reinterpret_cast<const char*>(current));
                entry.m_fontconfig_details.m_languages.insert(std::string(str));
              }
            FcStrListDone(iter);
          }
      }
      #endif
              
      WRATHassert(m_font_list.find(entry.font())==m_font_list.end());
      m_font_list[entry.font()]=entry;
    }
}

font_config_magic_class::
~font_config_magic_class()
{
  FcFontSetDestroy(m_fc_font_list);
}

WRATHFontFetch::font_handle
font_config_magic_class::
fetch_font_entry(const WRATHFontConfig::InFontSpecification &in_spec)
{
  WRATHAutoLockMutex(m_fc_mutex);
  
  /*
    Create an FcPattern from in_spec:
  */
  FcPattern *fc_filter; 
  FcLangSet *fc_langs(NULL);
  fc_filter=FcPatternCreate();
  
  /*
    Awkward moments in documentation:
    the desription of FcFontSetMatch at
    http://www.freedesktop.org/software/fontconfig/fontconfig-devel/fcfontsetmatch.html
    states that:
    
    " This function should be called only after FcConfigSubstitute and FcDefaultSubstitute 
    have been called for pattern; otherwise the results will not be correct"
    
    where the documentation for FcConfigSubstitute is a what the heck moment.
    Worse, calling FcDefaultSubstitute initially gives incorrect results
    in tests, for now we do NOT call FcDefaultSubstitute and we do NOT call
    FcConfigSubstitute.
  */
  //FcDefaultSubstitute(fc_filter);
  //FcConfigSubstitute(NULL, fc_filter, FcMatchPattern);
  
  
  FcPatternHelper(fc_filter, in_spec.m_family_name, FC_FAMILY);
  FcPatternHelper(fc_filter, in_spec.m_foundary_name, FC_FOUNDRY);
  FcPatternHelper(fc_filter, in_spec.m_style, FC_STYLE);
  FcPatternHelper(fc_filter, in_spec.m_weight, FC_WEIGHT);
  FcPatternHelper(fc_filter, in_spec.m_slant, FC_SLANT);
  
  if(!in_spec.m_languages.empty())
    {
      fc_langs=FcLangSetCreate();
      for(std::set<std::string>::iterator iter=in_spec.m_languages.begin(),
            end=in_spec.m_languages.end(); iter!=end; ++iter)
        {
          FcLangSetAdd(fc_langs,
                       reinterpret_cast<const FcChar8*>(iter->c_str()));
        }
      FcPatternAddLangSet(fc_filter, FC_LANG, fc_langs);
    }
  
  FcPattern *fc_font_choice(NULL);
  FcResult fc_result;
  WRATHFontFetch::font_handle R;

  fc_font_choice=FcFontSetMatch(NULL, &m_fc_font_list, 1, fc_filter, &fc_result);
  if(fc_font_choice!=NULL)
    {
      FcMagicValue<FcChar8*> file_name;
      FcMagicValue<int> font_index;
      
      FcGetMagicValue(fc_font_choice, FC_FILE, file_name);
      FcGetMagicValue(fc_font_choice, FC_INDEX, font_index);
      
      if(file_name.m_exists)
        {
          const char *fname(reinterpret_cast<const char*>(file_name.m_value));
          
          if(!font_index.m_exists)
            {
              font_index.m_value=0;
            }
          R=WRATHFontDatabase::fetch_font_entry(fname, font_index.m_value);
        }
      FcPatternDestroy(fc_font_choice);
    }
  
  if(!in_spec.m_languages.empty())
    {
      FcLangSetDestroy(fc_langs);
    }
  
  FcPatternDestroy(fc_filter);
  return R;
}


const WRATHFontConfig::FontList&
WRATHFontConfig::
font_list(void)
{
  return font_config_magic().font_list();
}

WRATHFontDatabase::Font::const_handle
WRATHFontConfig::
fetch_font_entry(const InFontSpecification &spec)
{
  return font_config_magic().fetch_font_entry(spec);
}

const WRATHFontConfig::FontSpecification&
WRATHFontConfig::
fetch_font_entry_detailed(const InFontSpecification &spec)
{
  WRATHStaticInit();

  static FontSpecification null_value;
  WRATHFontFetch::font_handle hnd;
  FontList::const_iterator iter;

  hnd=fetch_font_entry(spec);
  iter=font_list().find(hnd);

  return (iter!=font_list().end())?
    iter->second:
    null_value;
}

std::ostream&
operator<<(std::ostream &ostr, const WRATHFontConfig::InFontSpecification &obj)
{
  ostr << "[: " 
       << infont_print_helper<std::string>(obj.m_family_name, FC_FAMILY)
       << infont_print_helper<std::string>(obj.m_foundary_name, FC_FOUNDRY)
       << infont_print_helper<std::string>(obj.m_style, FC_STYLE)
       << infont_print_helper<int>(obj.m_weight, FC_WEIGHT)
       << infont_print_helper<int>(obj.m_slant, FC_SLANT);

  if(!obj.m_languages.empty())
    {
      ostr << " " << FC_LANG << ":" << "{";
      for(std::set<std::string>::iterator iter=obj.m_languages.begin(),
            end=obj.m_languages.end(); iter!=end; ++iter)
        {
          if(iter!=obj.m_languages.begin())
            {
              ostr << ", ";
            }
          ostr << "\"" << *iter << "\"";
        }
      ostr << "}";
    }
  ostr << "] ";
  return ostr;
}


std::ostream&
operator<<(std::ostream &ostr, const WRATHFontConfig::FontSpecification &obj)
{
  if(obj.valid())
    {
      ostr << "(" << obj->name() << ", " 
           << obj->face_index() << ")";
    }
  else
    {
      ostr << "(NULL)";
    }

  ostr << "\n" << obj.m_fontconfig_details << "\n\n"; 
  return ostr;
}


namespace WRATHFontDatabase
{

  void
  populate_database(void)
  {
    font_config_magic();
  }
  
  Font::const_handle
  fetch_font_entry(const FontProperties &properties)
  {
    WRATHFontConfig::InFontSpecification spec;

    if(!properties.m_style_name.empty())
      {
        spec.style(properties.m_style_name);
      }
    
    if(!properties.m_family_name.empty())
      {
        spec.family_name(properties.m_family_name);
      }
    
    if(!properties.m_foundry_name.empty())
      {
        spec.foundry_name(properties.m_foundry_name);
      }
    
    
    spec.weight(properties.m_bold?
                FC_WEIGHT_BOLD:
                FC_WEIGHT_NORMAL);
    
    spec.slant(properties.m_italic?
               FC_SLANT_ITALIC: //or should it be FC_SLANT_OBLIQUE?
               FC_SLANT_ROMAN);

    return WRATHFontConfig::fetch_font_entry(spec);
  }
  

}
