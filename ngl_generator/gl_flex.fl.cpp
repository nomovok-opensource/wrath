/*! 
 * \file gl_flex.fl.cpp
 * \brief file gl_flex.fl.cpp
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

%option noyywrap


%{ 
#include "HeaderCreator.hpp"


%}
/* definitions */
space [ \t]+ 
allSpace [ \t\n]  
anychar .|{allSpace}
const "const"{allSpace}+
GLTYPEARB GLchar|GLcharARB|GLintptr|GLintpreARB|GLsizeiptr|GLsizeiptrARB|GLhandleARB|GLhalfARB|GLhalfNV
GLTYPESIMPLE GLenum|GLbitfield|GLboolean|GLsizei|GLvoid|GLuint64EXT|GLuint64
GLTYPEBYTE GLbyte|GLubyte
GLTYPESHORT GLshort|GLushort
GLTYPEINT GLint|GLuint|int
GLTYPEFLOAT GLfloat|GLdouble|GLclampf|GLclampd|float|double
GLTYE {GLTYPEARB}|{GLTYPESIMPLE}|{GLTYPEBYTE}|{GLTYPESHORT}|{GLTYPEINT}|{GLTYPEFLOAT}|void|wchar_t
GLPTR {GLTYE}{allSpace}*"*"
GLTYPE {GLTYE}|{GLPTR}
CGLTYPE {const}{GLTYPE}
CGLGLTYPE {CGLTYPE}|{GLTYPE}

%%

%{
 /*rules*/
%}



extern{space}+{CGLGLTYPE}{space}*+APIENTRY{space}+gl[^\n]*\n  { 
  openGL_function_info *ptr;
  ptr=new openGL_function_info(yytext,"extern", "APIENTRY");
  openGL_functionList.push_back(ptr); 
}


WINGDIAPI{space}+{CGLGLTYPE}{space}*+APIENTRY{space}+gl[^\n]*\n  { 
  openGL_function_info *ptr;
  ptr=new openGL_function_info(yytext,"WINGDIAPI", "APIENTRY");
  openGL_functionList.push_back(ptr); 
}


GLAPI{space}+{CGLGLTYPE}{space}*+APIENTRY{space}+gl[^\n]*\n  { 
  openGL_function_info *ptr;
  ptr=new openGL_function_info(yytext, "GLAPI", "APIENTRY");
  openGL_functionList.push_back(ptr); 
}

extern{space}+{CGLGLTYPE}{space}*+GLAPIENTRY{space}+gl[^\n]*\n  { 
  openGL_function_info *ptr;
  ptr=new openGL_function_info(yytext,"extern", "GLAPIENTRY");
  openGL_functionList.push_back(ptr); 
}

GLAPI{space}+{CGLGLTYPE}{space}*+GLAPIENTRY{space}+gl[^\n]*\n  { 
  openGL_function_info *ptr;
  ptr=new openGL_function_info(yytext, "GLAPI", "GLAPIENTRY");
  openGL_functionList.push_back(ptr); 
}

GL_APICALL{space}+{CGLGLTYPE}{space}*+GL_APIENTRY{space}+gl[^\n]*\n  { 
  openGL_function_info *ptr;
  ptr=new openGL_function_info(yytext,"GL_APICALL", "GL_APIENTRY");
  openGL_functionList.push_back(ptr); 
}

FUNCTIONPOINTERMODE {
  openGL_function_info::sm_use_function_pointer_mode=true;
}

NONFUNCTIONPOINTERMODE {
  openGL_function_info::sm_use_function_pointer_mode=false;
}


[ \t\n]+  { /* do nothing */  }
. { /* do nothing */  }
%%

/*code*/


int main(int argc, char **argv)
{
  //read input from stdin!
  //arguments affect behavior.

  string headerName, sourceName, current, baseName("kgl"), path, outputpath;
  ofstream sourceFile, headerFile;
  list<string> fileNames;
  int num;
  map<string,openGL_function_info*>::iterator gg;

  for(int i=1;i<argc;++i)
    {
      string::iterator iter;

      current=argv[i];
      iter=find(current.begin(), current.end(), '=');
      if(iter!=current.end() and iter+1!=current.end())
        {
          if(string(current.begin(),iter)==string("name"))
            {
              baseName=string(iter+1,current.end());
            }

         if(string(current.begin(),iter)==string("path"))
            {
              path=string(iter+1,current.end());
            }
         if(string(current.begin(),iter)==string("outputpath"))
           {
             outputpath=string(iter+1,current.end());
           }
        }
      else //not a command parameter, then is a file. 
        {
          fileNames.push_back(current);
        }
    }

  if(!outputpath.empty())
    {
      outputpath += "/";
    }
  
  headerName=outputpath + baseName + ".hpp";
  sourceName=outputpath + baseName + ".cpp";

  sourceFile.open(sourceName.c_str());
  headerFile.open(headerName.c_str());
  if(!sourceFile || !headerFile)
    {
      cerr << "Failed to open one file for writing!\n";
      exit(-1);
    }

  //strip path from base name first:
  openGL_function_info::SetPrefix("ngl");

  //read from stdin the function prototypes
  YY_FLUSH_BUFFER;
  yyrestart(stdin);
  yylex();


  ofstream detailFile((outputpath + baseName+"detailed_output.txt").c_str());
  ofstream functionList((outputpath + baseName+"function_list_output.txt").c_str());

  openGL_function_info::HeaderStart(headerFile, fileNames);
  openGL_function_info::SourceStart(sourceFile, fileNames);

  for(num=0,gg=openGL_function_info::sm_lookUp.begin(); gg!=openGL_function_info::sm_lookUp.end(); ++gg, ++num)
    {
      
      gg->second->output_to_header(headerFile);
      gg->second->output_to_source(sourceFile);
      
      gg->second->GetInfo(detailFile);
      
      functionList << gg->second->function_name() << "\n";
      
    }

  openGL_function_info::HeaderEnd(headerFile, fileNames);
  openGL_function_info::SourceEnd(sourceFile, fileNames);

  cout << "\nGL functions counted=" << openGL_function_info::sm_numberFunctions
       << "\n";
  
}



