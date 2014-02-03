/*! 
 * \file WRATHGLProgram.cpp
 * \brief file WRATHGLProgram.cpp
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
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <cstring>
#include "WRATHglShaderBits.hpp"
#include "WRATHGLProgram.hpp"
#include "WRATHassert.hpp" 
#include "WRATHUtil.hpp"
#include "WRATHShaderSourceResource.hpp"
#include "WRATHGPUConfig.hpp"

namespace
{
  std::string
  get_path(const std::string &S)
  {
    std::string::size_type last_at;
        
    last_at=S.find_last_of("/\\");
    return S.substr(0, last_at);
  }

  std::string
  strip_leading_white_spaces(const std::string &S)
  {
    std::string::const_iterator iter, end;
    for(iter=S.begin(), end=S.end(); iter!=end and isspace(*iter); ++iter)
      {
      }
    return (iter!=end and *iter=='#')?
      std::string(iter, end):
      S;
  }

  void
  emit_source_line(std::ostream &output_stream,
                   const std::string &source, 
                   int line_number, const std::string &label)
  {
    std::string S;
    S=strip_leading_white_spaces(source);
    output_stream << S << "\n";
    //                  << std::setw(80-S.length()) << "  //LOCATION(" 
      //            << std::setw(3) << line_number
    //          << ", " << label 
    //            << ")\n";

  }

  std::pair<bool, std::string>
  includes_file(const std::string &S, const std::string &path)
  {
    std::pair<bool, std::string> R(false, "");
    if(!S.empty() and *S.begin()=='@')
      {
        /*
          find the last "@" symbol in S
        */
        std::string::size_type last_at;
        
        last_at=S.find_last_of('@');
        if(last_at==0)
          {
            last_at=std::string::npos;
          }
        else
          {
            --last_at;
          }
        R.second=WRATHUtil::filename_fullpath(path + "/" + S.substr(1, last_at));
        R.first=true;
      }
    return R;
  }

  
  std::pair<bool, std::string>
  includes_resource(const std::string &S)
  {
    std::pair<bool, std::string> R(false, "");
    if(!S.empty() and *S.begin()=='@')
      {
        std::string::size_type last_at;
        last_at=S.find_last_of('@');
        if(last_at==0)
          {
            last_at=std::string::npos;
          }
        else
          {
            --last_at;
          }
        R.second=S.substr(1, last_at);
        R.first=true;
      }
    return R;
  }

  void
  add_source_code_from_stream(const std::string &label,
                              std::istream &istr, 
                              std::ostream &output_stream,
                              const std::string &path,
                              std::set<std::string> &filelist)
  {
    std::string S;
    std::pair<bool, std::string> R;
    int line_number(1);


    while(getline(istr, S))
      {
        R=includes_file(S, path);
        if(R.first)
          {
            if(filelist.find(R.second)==filelist.end())
              {
                std::ifstream included_file(R.second.c_str());

                filelist.insert(R.second);
                add_source_code_from_stream(R.second,
                                            included_file, 
                                            output_stream,
                                            get_path(R.second),
                                            filelist);
              }
            else
              {
                output_stream << "// drop repeated file inclusion of \""
                              << R.second << "\" LOCATION(" 
                              << std::setw(3) << line_number
                              << ", " << label 
                              << ")\n";
              }
          }
        else
          {
            emit_source_line(output_stream, S, line_number, label);
          }

        ++line_number;
        S.clear();
      }

    if(line_number==1)
      {
        WRATHwarning("WARNING!! Empty file \""<< label << "\"");
      }
  }


  void
  add_source_code_from_stream(const std::string &label,
                              std::istream &istr, 
                              std::ostream &output_stream,
                              std::set<std::string> &filelist)
  {
    std::string S;
    std::pair<bool, std::string> R;
    int line_number(1);

    while(getline(istr, S))
      {

        R=includes_resource(S);
        if(R.first)
          {
            if(filelist.find(R.second)==filelist.end())
              {
                std::istringstream istr;
                istr.str(WRATHShaderSourceResource::retrieve_value(R.second));

                filelist.insert(R.second);
                add_source_code_from_stream(R.second,
                                            istr, 
                                            output_stream,
                                            filelist);
              }
            else
              {
                output_stream << "// drop repeated file inclusion of \""
                              << R.second << "\" LOCATION(" 
                              << std::setw(3) << line_number
                              << ", " << label 
                              << ")\n";
              }
          }
        else
          {
            emit_source_line(output_stream, S, line_number, label);
          }
        
        ++line_number;
        S.clear();
      }

    if(line_number==1)
      {
        WRATHwarning("WARNING!! Resource \"" << label << "\"");
      }
  }

  

  void
  add_source_entry(const WRATHGLShader::shader_source::source_code_type &v, 
                   std::ostream &output_stream)
  {
    //if from a file we need to add the file contents one line at a time:
    if(v.second==WRATHGLShader::from_file)
      {
        std::string fullpath(WRATHUtil::filename_fullpath(v.first));
        std::ifstream file(fullpath.c_str());

        if(file)
          {
            std::set<std::string> filelist;
            filelist.insert(fullpath);
            
            add_source_code_from_stream(v.first, file, output_stream, get_path(fullpath), filelist); 
          }
        else
          {
            output_stream << "\n//WARNING: Could not open file \"" 
                          << v.first << "\"\n";
          }
      }
    else
      {
        std::istringstream istr;
        std::set<std::string> filelist;
        std::string label;

        if(v.second==WRATHGLShader::from_string)
          {
            istr.str(v.first);
            label="raw string";
          }
        else
          {
            istr.str(WRATHShaderSourceResource::retrieve_value(v.first));
            label=v.first;
          }

        add_source_code_from_stream(label, istr, output_stream, filelist); 
      }
  }

  /*
    removes ending array index of name and sets that 
    value to array_index. If it does not name an array,
    array_index is set as 0. Note that if can name an
    array value with index 0, we do this delibertly.
   */
  template<typename iterator>
  std::string
  filter_name(iterator begin, iterator end, int &array_index)
  {
    std::string return_value;

    /*
      Firstly, strip out all white spaces:
     */
    return_value.reserve(std::distance(begin,end));
    for(iterator iter=begin; iter!=end; ++iter)
      {
        if(!std::isspace(*iter))
          {
            return_value.append(1, *iter);
          }
      }

    /*
      now check if the last character is a ']'
      and if it is remove characters until
      a '[' is encountered.
     */
    if(!return_value.empty() and *return_value.rbegin()==']')
      {
        std::string::size_type loc;

        loc=return_value.find_last_of('[');
        WRATHassert(loc!=std::string::npos);

        std::istringstream array_value(return_value.substr(loc+1, return_value.size()-1));
        array_value >> array_index;

        //also resize return_value to remove
        //the [array_index]:
        return_value.resize(loc);
      }
    else
      {
        array_index=0;
      }
    return return_value;
  }

  template<typename F, typename G>
  void
  get_details(GLuint programHandle,
              GLenum count_enum, GLenum length_enum,
              F fptr,
              std::map<std::string, WRATHGLProgram::parameter_info> &output,
              G gptr)
  {
    GLint count, largest_length;
    std::vector<char> pname;

    glGetProgramiv(programHandle, count_enum, &count);

    if(count>0)
      {
        glGetProgramiv(programHandle, length_enum, &largest_length);
        
        ++largest_length;
        pname.resize(largest_length, '\0');
        
        for(int i=0;i!=count;++i)
          {
            
            GLsizei name_length, psize;
            GLenum ptype;
            int array_index;
            WRATHGLProgram::parameter_info v;

            std::memset(&pname[0], 0, largest_length);
            
            fptr(programHandle, i, largest_length,
                 &name_length, &psize,
                 &ptype, &pname[0]);
                        
            v.m_type=ptype;
            v.m_count=psize;
            v.m_name=filter_name(pname.begin(), 
                                 pname.begin()+name_length,
                                 array_index);
            if(array_index!=0)
              {
                /*
                  crazy GL... it lists an element
                  from an array as a unique location,
                  chicken out and add it with the
                  array index:
                 */
                v.m_name=std::string(pname.begin(), 
                                     pname.begin()+name_length);
              }

            v.m_index=i;
            v.m_location=gptr(programHandle, &pname[0]);

            output[v.m_name]=v;
          }
      }
  }

  std::ostream&
  operator<<(std::ostream &ostr, 
             enum WRATHGLShader::shader_extension_enable_type tp)
  {
    switch(tp)
      {
      default:
      case WRATHGLShader::enable_extension:
        ostr << "enable";
        break;

      case WRATHGLShader::require_extension:
        ostr << "require";
        break;

      case WRATHGLShader::warn_extension:
        ostr << "warn";
        break;

      case WRATHGLShader::disable_extension:
        ostr << "disable";
        break;
      }
    return ostr;
  }

  const WRATHGLProgram::attribute_uniform_query_result
  find_worker(const std::map<std::string, WRATHGLProgram::parameter_info> &pmap,
              const std::string &pname)
  {
    std::map<std::string, WRATHGLProgram::parameter_info>::const_iterator iter;

    iter=pmap.find(pname);
    if(iter!=pmap.end())
      {
        return
          WRATHGLProgram::attribute_uniform_query_result(iter->second.m_location,
                                                         &iter->second);
      }

    std::string filtered_name;
    int array_index;
    filtered_name=filter_name(pname.begin(), pname.end(),
                              array_index);
    
    iter=pmap.find(filtered_name);
    if(iter!=pmap.end() and array_index<iter->second.m_count)
      {
        return 
          WRATHGLProgram::attribute_uniform_query_result(iter->second.m_location+array_index,
                                                         &iter->second);
      }
    
    return WRATHGLProgram::attribute_uniform_query_result();
    
  }

}


////////////////////////////////////
//WRATHGLShader::shader_source methods
void
WRATHGLShader::shader_source::
build_source_code(std::ostream &output_glsl_source_code, GLenum shader_type) const
{
  if(!m_version.empty())
    {
      output_glsl_source_code <<"\n#version " << m_version << "\n";
    }

  for(std::map<std::string, enum shader_extension_enable_type>::const_iterator 
        iter=m_extensions.begin(), end=m_extensions.end(); iter!=end; ++iter)
    {
      output_glsl_source_code << "\n#extension " << iter->first << ": " << iter->second;
    }

  /*
    GL core profile does not define texture2D, rather
    all texture lookup function names are overloaded
    by the sampler type. Rather than make everyone
    go nuts and have all the GLES2/GL2 shaders break,
    provide the #define from *2D / *3D etc to without
   */
  if(WRATHGPUConfig::old_glsl_texture_functions_deprecated())
    {
      output_glsl_source_code << "\n#define texture1D texture"
                              << "\n#define texture1DLod textureLod"
                              << "\n#define texture1DProj textureProj"
                              << "\n#define texture1DProjLod textureProjLod"   
                              
                              << "\n#define texture2D texture"
                              << "\n#define texture2DLod textureLod"
                              << "\n#define texture2DProj textureProj"
                              << "\n#define texture2DProjLod textureProjLod" 

                              << "\n#define texture3D texture"
                              << "\n#define texture3DLod textureLod"
                              << "\n#define texture3DProj textureProj"
                              << "\n#define texture3DProjLod textureProjLod" 

                              << "\n#define shadow1D texture"
                              << "\n#define shadow1DLod textureLod"
                              << "\n#define shadow1DProj textureProj"
                              << "\n#define shadow1DProjLod textureProjLod" 

                              << "\n#define shadow2D texture"
                              << "\n#define shadow2DLod textureLod"
                              << "\n#define shadow2DProj textureProj"
                              << "\n#define shadow2DProjLod textureProjLod" 

                              << "\n#define textureCube texture"
                              << "\n#define textureCubeLod textureLod"

        /*
          also for symbols coming from GL_EXT_shader_texture_lod
         */
                              << "\n#define texture2DLodEXT texture2DLod"
                              << "\n#define texture2DProjLodEXT texture2DProjLod"
                              << "\n#define textureCubeLodEXT textureCubeLod"
                              << "\n#define texture2DGradEXT textureGrad"
                              << "\n#define texture2DProjGradEXT textureProjGrad"
                              << "\n#define textureCubeGradEXT textureGrad"
                              << "\n";

    }

  if(shader_type==GL_FRAGMENT_SHADER and m_wrath_FragColor)
    {
      #if WRATH_GL_GLES_VERSION>=3
      {
        output_glsl_source_code << "\nout mediump vec4 wrath_FragColor;\n";
      }
      #else
      {
        output_glsl_source_code << "\n#define wrath_FragColor gl_FragColor \n";
      }
      #endif
    }

  if(!WRATHGPUConfig::use_in_out_in_shaders())
    {
      if(shader_type==GL_VERTEX_SHADER)
        {
          output_glsl_source_code << "\n#define shader_in attribute";
        }
      else
        {
          output_glsl_source_code << "\n#define shader_in varying";
        }
      
      if(shader_type!=GL_FRAGMENT_SHADER)
        {
          output_glsl_source_code << "\n#define shader_out varying";
        }
      else
        {
          output_glsl_source_code << "\n#define shader_out out";
        }
    }
  else
    {
      output_glsl_source_code << "\n#define shader_in in"
                              << "\n#define shader_out out";
    }

  


  if(shader_type==GL_FRAGMENT_SHADER)
    {
      if(WRATHGPUConfig::unextended_shader_support_derivatives())
        {
          output_glsl_source_code << "\n#define WRATH_DERIVATIVES_SUPPORTED\n";
        }
      else
        {
          output_glsl_source_code << "\n#extension GL_OES_standard_derivatives: enable" 
                                  << "\n#if defined(GL_OES_standard_derivatives)"
                                  << "\n#define WRATH_DERIVATIVES_SUPPORTED"
                                  << "\n#endif\n";
        }
    }

  
      
  /*
    //this block of code is evil, it fakes support for the LOD functions
    //by just mapping them to texture2D
    //also should we add something equally evil for the case
    //of on desktop and adding defines to map EXT function names
    //to non-EXT function names?? or visa-versa?
  if(!WRATHGPUConfig::unextended_shader_support_texture_lod())
    {
      output_glsl_source_code << "\nextension GL_EXT_shader_texture_lod:enable"
                              << "\n#ifndef GL_EXT_shader_texture_lod"
                              << "\n#define texture2DLodEXT(A,B,C) texture2D(A, B)"
                              << "\n#define texture2DProjLodEXT(A,B,C) texture2DProj(A, B)"
                              << "\n#define textureCubeLodEXT(A,B,C) textureCube(A, B)"
                              << "\n#define texture2DGradEXT(A,B,C,D) texture2D(A, B)"
                              << "\n#define texture2DProjGradEXT(A,B,C,D) texture2D(A, B)"
                              << "\n#define textureCubeGradEXT(A,B,C,D) texture2D(A, B)"
                              << "\n#endif\n";
    }
  */

  output_glsl_source_code << "\n#define WRATH_" << gl_shader_type_label(shader_type) << "\n";




  #if defined(WRATH_TEGRA_PREC_HACK)
    {
      /*
        some older TEGRA2 GL drivers have the precision
        qualifier order messed up, they expect 
        mediump in vec2 instead of the correct
        in mediump vec2. The work around is a hack:
        we set the defaults precision qualifiers for 
        all types and then add defines removing the 
        precision  qualifier symbols.
      */
      
      if(shader_type==GL_FRAGMENT_SHADER)
        {
          output_glsl_source_code << "\nprecision mediump float;"
                                  << "\nprecision mediump int;"
                                  << "\nprecision mediump sampler2D;";
        }
      else
        {
          output_glsl_source_code << "\nprecision highp float;"
                                  << "\nprecision highp int;";
        }


      output_glsl_source_code << "\n#define lowp"
                              << "\n#define highp"
                              << "\n#define mediump\n\n";
    }
  #elif defined(WRATH_GL_VERSION) or defined(WRATH_REMOVE_PRECISION_QUALIFIERS)
    {
      output_glsl_source_code << "\n#define lowp"
                              << "\n#define highp"
                              << "\n#define mediump\n\n";
    }
  #else
    {
      if(m_force_highp)
        {
          output_glsl_source_code << "\n#define lowp highp"
                                  << "\n#define mediump highp\n";
        }
      else if(shader_type==GL_FRAGMENT_SHADER)
        {
          /*
            code hackery to force highp to mediump 
            for platforms that do not support
            highp in the fragment shader
          */
          output_glsl_source_code << "\n#ifdef GL_ES"
                                  << "\n#ifndef GL_FRAGMENT_PRECISION_HIGH"
                                  << "\n#define highp mediump"
                                  << "\n#endif"
                                  << "\n#endif\n\n";
        }
    }
  #endif

  if(WRATHGPUConfig::dependent_texture_lookup_requires_LOD())
    {
      output_glsl_source_code << "\n#define WRATH_GPU_CONFIG_DEPENDENT_TEXTURE_LOOKUP_REQUIRES_LOD\n";
    }

  if(WRATHGPUConfig::fragment_shader_poor_branching())
    {
      output_glsl_source_code << "\n#define WRATH_GPU_CONFIG_FRAGMENT_SHADER_POOR_BRANCHING\n";
    }

  if(WRATHGPUConfig::fragment_shader_texture_LOD_supported())
    {
      output_glsl_source_code << "\n#define WRATH_GPU_CONFIG_FRAGMENT_SHADER_TEXTURE_LOD\n";
    }

  for(std::list< source_code_type>::const_iterator 
        iter=m_values.begin(), end=m_values.end(); iter!=end; ++iter)
    {      
      add_source_entry(*iter, output_glsl_source_code);               
    }

  /*
    some GLSL pre-processors do not like to end on a
    comment or other certain tokens, to make them
    less grouchy, we emit a few extra \n's
   */
  output_glsl_source_code << "\n\n\n"
                          << "#define WRATH_GL_SOURCE_END\n\n";
}

WRATHGLShader::shader_source&
WRATHGLShader::shader_source::
absorb(const shader_source &obj)
{
  std::list<source_code_type> vs(obj.m_values);
  
  m_values.splice(m_values.end(), vs);
  std::copy(obj.m_extensions.begin(), obj.m_extensions.end(),
            std::inserter(m_extensions, m_extensions.end()) );
  
  m_force_highp=m_force_highp or obj.m_force_highp;
  if(!obj.m_version.empty())
    {
      m_version=obj.m_version;
    }
  return *this;
}

////////////////////////////////////////////////
//WRATHGLShader methods
WRATH_RESOURCE_MANAGER_IMPLEMENT(WRATHGLShader, std::string)

WRATHGLShader::
WRATHGLShader(const std::string &presource_name,
              const shader_source &src, 
              GLenum pshader_type):
  m_shader_ready(false),
  m_name(0),
  m_shader_type(pshader_type),
  m_resource_name(presource_name),
  m_compile_success(false)
{
 
  std::ostringstream contents;
  
  src.build_source_code(contents, pshader_type);
  m_source_code=contents.str();
  resource_manager().add_resource(presource_name, this);
}

WRATHGLShader::
~WRATHGLShader()
{
  resource_manager().remove_resource(this);
 
  /*
    TODO: deletion of a shader should not be
    required to be done with a GL context
    current.
   */
  if(m_name)
    {
      glDeleteShader(m_name);
    }
}


bool
WRATHGLShader::
compile_success(void) 
{
  compile();
  return m_compile_success;
}

const std::string&
WRATHGLShader::
compile_log(void) 
{
  compile();
  return m_compile_log;
}

GLuint
WRATHGLShader::
name(void) 
{
  compile();
  return m_name;
}


void
WRATHGLShader::
compile(void)
{
  if(m_shader_ready) 
    {
      return;
    }


  
  //now do the GL work, create a name and compile the source code:
  WRATHassert(m_name==0);

  m_shader_ready=true;
  m_name=glCreateShader(m_shader_type);

  //std::cout << "Compile shader:\"" << resource_name() << "\n";
  
  const char *sourceString[1];
  sourceString[0]=m_source_code.c_str();

  
  glShaderSource(m_name, //shader handle
                 1, //number strings
                 sourceString, //array of strings
                 NULL); //lengths of each string or NULL implies each is 0-terminated
  
  glCompileShader(m_name);

  GLint logSize(0), shaderOK;
  std::vector<char> raw_log;

  //get shader compile status and log length.
  glGetShaderiv(m_name, GL_COMPILE_STATUS, &shaderOK); 
  glGetShaderiv(m_name, GL_INFO_LOG_LENGTH, &logSize);
  
  //retrieve the compile log string, eh gross.
  raw_log.resize(logSize+2,'\0');
  glGetShaderInfoLog(m_name, //shader handle 
                     logSize+1, //maximum size of string
                     NULL, //GLint* return length of string
                     &raw_log[0]); //char* to write log to.

  m_compile_log=&raw_log[0];
  m_compile_success=(shaderOK==GL_TRUE);

  if(!m_compile_success)
    {
      std::ostringstream oo;

      oo << "bad_shader_" << m_name << ".glsl";

      std::ofstream eek(oo.str().c_str());
      eek << m_source_code
          << "\n\n"
          << m_compile_log;
    }
  
}

std::string
WRATHGLShader::
gl_shader_type_label(GLenum shader_type)
{
  #define CASE(X) case X: return #X
  
  switch(shader_type)
    {
    default:
      {
        std::ostringstream ostr;
        ostr << "UNKNOWN_SHADER_STAGE_" << std::hex << shader_type;
        return ostr.str();
      }

      CASE(GL_FRAGMENT_SHADER);
      CASE(GL_VERTEX_SHADER);

      #ifdef GL_GEOMETRY_SHADER
        CASE(GL_GEOMETRY_SHADER);
      #endif

      #ifdef GL_TESS_EVALUATION_SHADER
        CASE(GL_TESS_EVALUATION_SHADER);
      #endif

      #ifdef GL_TESS_CONTROL_SHADER
        CASE(GL_TESS_CONTROL_SHADER);
      #endif
    }
  
  #undef CASE
}

uint32_t
WRATHGLShader::
gl_shader_bit(GLenum shader_type)
{
  #define CASE(X) case X: return X##_BIT;
  
  switch(shader_type)
    {
    default:
        return 0;

      CASE(GL_FRAGMENT_SHADER);
      CASE(GL_VERTEX_SHADER);

      #ifdef GL_GEOMETRY_SHADER
        CASE(GL_GEOMETRY_SHADER);
      #endif

      #ifdef GL_TESS_EVALUATION_SHADER
        CASE(GL_TESS_EVALUATION_SHADER);
      #endif

      #ifdef GL_TESS_CONTROL_SHADER
        CASE(GL_TESS_CONTROL_SHADER);
      #endif
    }
  
  #undef CASE
}



/////////////////////////////////
//WRATHGLPreLinkAction methods
void
WRATHGLPreLinkAction::
action(WRATHGLProgram*) const
{}

bool
WRATHGLPreLinkAction::
post_action(std::ostream&, WRATHGLProgram*) const
{
  return false;
}

////////////////////////////////////////
// WRATHGLBindAttribute methods
void
WRATHGLBindAttribute::
action(WRATHGLProgram* glsl_program) const
{
  glBindAttribLocation(glsl_program->name(), m_location, m_label.c_str());  
}

bool
WRATHGLBindAttribute::
post_action(std::ostream &str, WRATHGLProgram *program) const
{
  std::map<std::string, int> check_list;
  bool return_value(false);

  std::map<std::string, WRATHGLProgram::parameter_info>::const_iterator iter;

  iter=program->active_attributes().find(m_label);
  if(iter==program->active_attributes().end())
    {
      return_value=true;
      str << "\nAttribute \""
          << m_label
          << "\" not present in shader, but location specified by binder"; 
    }
  else if(iter->second.m_location!=m_location)
    {
      return_value=true;
      str << "\nAttribute \""
          << m_label
          << "\" has different location than of binder";
    }

  program->m_binded_attributes.insert(m_label);

  

  return return_value;
  
}


////////////////////////////////////////////////////////
//WRATHGLProgram methods

WRATH_RESOURCE_MANAGER_IMPLEMENT(WRATHGLProgram, std::string)

WRATHGLProgram::
WRATHGLProgram(const std::string &presource_name,
               const std::map<GLenum, WRATHGLShader::shader_source> &shaders,
               const WRATHGLPreLinkActionArray &action,
               const WRATHGLProgramInitializerArray &initers,
               const WRATHGLProgramOnBindActionArray &bind_actions):
  m_initializers(initers.m_values),
  m_bind_actions(bind_actions)
{
  for(std::map<GLenum, WRATHGLShader::shader_source>::const_iterator 
        iter=shaders.begin(), end=shaders.end(); iter!=end; ++iter)
    {
      std::ostringstream shader_name;

      shader_name << presource_name << "." << WRATHGLShader::gl_shader_type_label(iter->first);
      m_shaders.push_back(WRATHNew WRATHGLShader(shader_name.str(), iter->second, iter->first) );
    }

  pre_assemble(presource_name, action);
}


WRATHGLProgram::
~WRATHGLProgram()
{
  if(m_name)
    {
      glDeleteProgram(m_name);
    }
  m_dtor_signal();
  resource_manager().remove_resource(this);
  //std::cout << "~WRATHGLProgram(" << this << "): " << m_resource_name << "\n";
}


void
WRATHGLProgram::
pre_assemble(const std::string &presource_name, const WRATHGLPreLinkActionArray &action)
{
  resource_manager().add_resource(presource_name, this);
  m_resource_name=presource_name;
  m_name=0;
  m_assembled=false;
  m_pre_link_actions=action;
}


void
WRATHGLProgram::
assemble(void)
{
  if(m_assembled)
    {
      return;
    }

  std::ostringstream error_ostr;
  std::ostringstream str_action_log;
  bool post_action_warning;

  m_assembled=true;
  WRATHassert(m_name==0);
  m_name=glCreateProgram();

      
  m_link_success=true;

  //attatch the shaders, attaching a bad shader makes 
  //m_link_success become false
  for(std::vector<WRATHGLShader*>::iterator iter=
        m_shaders.begin(), end=m_shaders.end(); iter!=end; ++iter)
    {
      if((*iter)->compile_success())
        {
          glAttachShader(m_name, (*iter)->name());
        }
      else
        {
          m_link_success=false;
        }
    }
  
  //perform any pre-link actions.
  m_pre_link_actions.execute_actions(this);
  
  //now finally link!
  glLinkProgram(m_name);
  
  //retrieve the log fun
  std::vector<char> raw_log;
  GLint logSize, linkOK;
  
  glGetProgramiv(m_name, GL_LINK_STATUS, &linkOK); 
  glGetProgramiv(m_name, GL_INFO_LOG_LENGTH, &logSize);
  
  raw_log.resize(logSize+2);
  glGetProgramInfoLog(m_name, logSize+1, NULL , &raw_log[0]); 
  
  error_ostr << "\n-----------------------\n" 
             << &raw_log[0];

  m_link_log=error_ostr.str();
  m_link_success=m_link_success and (linkOK==GL_TRUE);

  if(m_link_success)
    {
      int e1, e2, e3, e4;

      e1=ngl_functionExists(glGetActiveAttrib);
      e2=ngl_functionExists(glGetAttribLocation);
      e3=ngl_functionExists(glGetUniformLocation);
      e4=ngl_functionExists(glGetUniformLocation);
      WRATHassert(e1 and e2 and e3 and e4);
      WRATHunused(e1);
      WRATHunused(e2);
      WRATHunused(e3);
      WRATHunused(e4);

      get_details(m_name,
                  GL_ACTIVE_ATTRIBUTES,
                  GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
                  ngl_functionPointer(glGetActiveAttrib),
                  m_attribute_list, 
                  ngl_functionPointer(glGetAttribLocation) );

      get_details(m_name,
                  GL_ACTIVE_UNIFORMS,
                  GL_ACTIVE_UNIFORM_MAX_LENGTH,
                  ngl_functionPointer(glGetActiveUniform),
                  m_uniform_list,
                  ngl_functionPointer(glGetUniformLocation));

      post_action_warning=m_pre_link_actions.execute_post_actions(str_action_log, this);
      /*
        check that all attributes in the shader
        are explicitely bounded.
       */
      for(std::map<std::string, parameter_info>::const_iterator
            iter=m_attribute_list.begin(), end=m_attribute_list.end();
          iter!=end; ++iter)
        {
          if(m_binded_attributes.find(iter->first)==m_binded_attributes.end())
            {
              post_action_warning=true;
              str_action_log << "\nAttribute \"" << iter->first 
                             << "\" present in shader, but location not specified by binder";
            }
        }

      m_action_log=str_action_log.str();

      #if defined(WRATHDEBUG)
      {
        if(post_action_warning)
          {
            WRATHwarning("\nAction warning log for \"" 
                         << resource_name() << "\":\n"
                         << m_action_log << "\n");
          }
        
        if(post_action_warning)
          {
            std::ostringstream oo;
            
            oo << "good_program_post_action_warning" << m_name << ".glsl";
            std::ofstream eek(oo.str().c_str());
          
            log_contents(eek);
            eek << "\n\nWarning" << m_action_log << "\n";
          }
      }
      #endif
      
      WRATHunused(post_action_warning);
    }
  else
    {
      //since the program cannot be used,
      //clear it's initers..
      m_initializers.clear();


      std::ostringstream oo;
      oo << "bad_program_" << m_name << ".glsl";
      std::ofstream eek(oo.str().c_str());
      
      for(std::vector<WRATHGLShader*>::iterator iter=m_shaders.begin(),
            end=m_shaders.end(); iter!=end; ++iter)
        {
          WRATHGLShader *sh(*iter);

          eek << "\n\nshader: " << sh->name()
              << "[" << WRATHGLShader::gl_shader_type_label(sh->shader_type()) << "]\n"
              << "shader_source:\n"
              << sh->source_code()
              << "compile log:\n"
              << sh->compile_log();
        }

      eek << "\n\nLink Log: "
          << link_log();
    }
}


const std::string&
WRATHGLProgram::
link_log(void) 
{
  assemble();
  return m_link_log;
}

const std::string&
WRATHGLProgram::
action_log(void) 
{
  assemble();
  return m_action_log;
}

bool
WRATHGLProgram::
link_success(void) 
{
  assemble();
  return m_link_success;
}


GLuint 
WRATHGLProgram::
name(void) 
{
  assemble();
  return m_name;
}

const std::map<std::string, WRATHGLProgram::parameter_info>&
WRATHGLProgram::
active_uniforms(void) 
{
  assemble();
  return m_uniform_list;
}

const std::map<std::string, WRATHGLProgram::parameter_info>&
WRATHGLProgram::
active_attributes(void) 
{
  assemble();
  return m_attribute_list;
}


void
WRATHGLProgram::
log_contents(std::ostream &ostr) 
{
  assemble();

  ostr << "WRATHGLProgram: " << resource_name()
       << "[GLname: " << m_name << "]:\tShaders:";

  for(std::vector<WRATHGLShader*>::const_iterator 
        iter=m_shaders.begin(), end=m_shaders.end();
      iter!=end; ++iter)
    {
      ostr << "\n\nGLSL name=" << (*iter)->name() 
           << ", type=" << WRATHGLShader::gl_shader_type_label((*iter)->shader_type())
           << "\nSource:\n" << (*iter)->source_code()
           << "\nCompileLog:\n" << (*iter)->compile_log();
    }

  ostr << "\nLink Log:\n" << link_log()
       << "\nAction Log:\n" << action_log();

  if(m_link_success)
    {

            
      ostr << "\n\nUniforms:";
      for(std::map<std::string, WRATHGLProgram::parameter_info>::const_iterator
            iter=m_uniform_list.begin(), end=m_uniform_list.end();
          iter!=end;++iter)
        {
          ostr << "\n\t"
               << iter->second.m_name 
               << "\n\t\ttype=0x" 
               << std::hex << iter->second.m_type
               << "\n\t\tcount=" << std::dec << iter->second.m_count
               << "\n\t\tindex=" << std::dec << iter->second.m_index
               << "\n\t\tlocation=" << iter->second.m_location;
        }
      
      ostr << "\n\nAttributes:";
      for(std::map<std::string, WRATHGLProgram::parameter_info>::const_iterator
            iter=m_attribute_list.begin(), end=m_attribute_list.end();
          iter!=end;++iter)
        {
          ostr << "\n\t"
               << iter->second.m_name 
               << "\n\t\ttype=0x" 
               << std::hex << iter->second.m_type
               << "\n\t\tcount=" << std::dec << iter->second.m_count
               << "\n\t\tindex=" << std::dec << iter->second.m_index
               << "\n\t\tlocation=" << iter->second.m_location;
        }
    }

}


WRATHGLProgram::attribute_uniform_query_result
WRATHGLProgram::
find_uniform(const std::string &pname) 
{
  return find_worker(active_uniforms(), pname);
}

WRATHGLProgram::attribute_uniform_query_result
WRATHGLProgram::
find_attribute(const std::string &pname) 
{
  return find_worker(active_attributes(), pname);
}

void
WRATHGLProgram::
use_program(void) 
{
  assemble();

  WRATHassert(m_name!=0);

  if(!m_link_success)
    {
      WRATHwarning("\nAttempt to use ill-formed GLProgram" << resource_name() << "\n");
      return;
    }

  glUseProgram(m_name);
  for(std::vector<WRATHGLProgramInitializer::const_handle>::const_iterator
        iter=m_initializers.begin(), end=m_initializers.end();
      iter!=end; ++iter)
    {
      const WRATHGLProgramInitializer::const_handle &v(*iter);
      if(v.valid())
        {
          v->perform_initialization(this);
        }
    }
  m_initializers.clear();
  m_bind_actions.execute_actions(this);
}

///////////////////////////////////////////
//WRATHGLProgramOnBindActionArray methods
void
WRATHGLProgramOnBindActionArray::
execute_actions(WRATHGLProgram *pr) const
{
  for(std::vector<WRATHGLProgramOnBindAction::const_handle>::const_iterator
        iter=m_values.begin(), end=m_values.end(); iter!=end; ++iter)
    {
      if(iter->valid())
        {
          (*iter)->perform_action(pr);
        }
    }
}

WRATHGLProgramOnBindActionArray&
WRATHGLProgramOnBindActionArray::
absorb(const WRATHGLProgramOnBindActionArray &obj)
{
  unsigned int sz(m_values.size());

  m_values.resize(sz+obj.m_values.size());
  std::copy(obj.m_values.begin(), obj.m_values.end(),
            m_values.begin()+sz);

  return *this;
}

////////////////////////////////////////////
// WRATHGLPreLinkActionArray methods
void
WRATHGLPreLinkActionArray::
execute_actions(WRATHGLProgram *pr) const
{
  for(std::vector<WRATHGLPreLinkAction::const_handle>::const_iterator
        iter=m_values.begin(), end=m_values.end(); iter!=end; ++iter)
    {
      if(iter->valid())
        {
          (*iter)->action(pr);
        }
    }
}

bool
WRATHGLPreLinkActionArray::
execute_post_actions(std::ostream &ostr, WRATHGLProgram *pr) const
{
  bool return_value(false);

  for(std::vector<WRATHGLPreLinkAction::const_handle>::const_iterator
        iter=m_values.begin(), end=m_values.end(); iter!=end; ++iter)
    {
      if(iter->valid())
        {
          bool r;

          r=(*iter)->post_action(ostr, pr);
          return_value=return_value or r;
        }
    }
  return return_value;
}


WRATHGLPreLinkActionArray&
WRATHGLPreLinkActionArray::
absorb(const WRATHGLPreLinkActionArray &obj)
{
  unsigned int sz(m_values.size());

  m_values.resize(sz+obj.m_values.size());
  std::copy(obj.m_values.begin(), obj.m_values.end(),
            m_values.begin()+sz);

  return *this;
}

//////////////////////////////////////
//WRATHGLProgramInitializerArray methods
WRATHGLProgramInitializerArray&
WRATHGLProgramInitializerArray::
absorb(const WRATHGLProgramInitializerArray &obj)
{
  unsigned int sz(m_values.size());

  m_values.resize(sz+obj.m_values.size());
  std::copy(obj.m_values.begin(), obj.m_values.end(),
            m_values.begin()+sz);

  return *this;
}
