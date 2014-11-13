/*! 
 * \file sdl_demo.cpp
 * \brief file sdl_demo.cpp
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
#include <typeinfo>
#include <fstream>
#include "vectorGL.hpp"
#include "WRATHglGet.hpp"
#include "sdl_demo.hpp"

#ifdef HARMATTAN
#include <policy/resource-set.h>
#endif


DemoKernelMaker::
DemoKernelMaker(void):
  m_red_bits(-1, "red_bits", 
             "Bpp of red channel, non-positive values mean use SDL defaults",
             *this),
  m_green_bits(-1, "green_bits", 
               "Bpp of green channel, non-positive values mean use SDL defaults",
               *this),
  m_blue_bits(-1, "blue_bits", 
              "Bpp of blue channel, non-positive values mean use SDL defaults",
              *this),
  m_alpha_bits(-1, "alpha_bits", 
               "Bpp of alpha channel, non-positive values mean use SDL defaults",
               *this),
  m_depth_bits(24, "depth_bits", 
               "Bpp of depth buffer, non-positive values mean use SDL defaults",
               *this),
  m_stencil_bits(8, "stencil_bits", 
                 "Bpp of stencil buffer, non-positive values mean use SDL defaults",
                 *this),
  m_fullscreen(false, "fullscreen", "fullscreen mode", *this),
  m_hide_cursor(false, "hide_cursor", "If true, hide the mouse cursor with a SDL call", *this),
  m_use_msaa(false, "enable_msaa", "If true enables MSAA", *this),
  m_msaa(4, "msaa_samples", 
         "If greater than 0, specifies the number of samples "
         "to request for MSAA. If not, SDL will choose the "
         "sample count as the highest available value",
         *this),
  m_width(800, "width", "window width", *this),
  m_height(480, "height", "window height", *this),
  m_bpp(32, "bpp", "bits per pixel", *this),
  m_libGL("", "libGL", "if non-empty use a custom libGL.so", *this),

  #ifdef WRATH_GLES_VERSION
  m_gl_major(2, "gles_major", "GLES major version", *this),
  m_gl_minor(0, "gles_minor", "GLEs minor version", *this),
  #else
  m_gl_major(3, "gl_major", "GL major version", *this),
  m_gl_minor(3, "gl_minor", "GL minor version", *this),
  #endif

  m_gl_forward_compatible_context(false, "foward_context", "if true request forward compatible context", *this),
  m_gl_debug_context(false, "debug_context", "if true request a context with debug", *this),
  m_gl_core_profile(true, "core_context", "if true request a context which is core profile", *this), 

  m_log_all_gl(false, "log_gl", "if true all GL commands are logged, otherwise only errors are logged", *this),
  m_log_gl_file("", "log_gl_file", "GL commands/errors are logged to the named file. Default is errors are logged to stderr."
		    "If value is stderr then logged to stderr, if value is stdout logged to stdout", *this),
  m_log_alloc_commands("", "log_alloc", "If non empty, logs allocs and deallocs to the named file", *this),
  m_print_gl_info(false, "print_gl_info", "If true print to stdout GL information", *this),

  m_gl_log(NULL),
  m_alloc_log(NULL),
  m_end_demo_flag(false),
  m_vao(0),
  m_d(NULL),
  m_ep(NULL),
  m_window(NULL)
{}

enum return_code
DemoKernelMaker::   
init_sdl(void)
{
  // Init SDL
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)<0)
    {
      /*
        abort!
       */
      std::cerr << "\nFailed on SDL_Init\n";
      return routine_fail;
    }

  int video_flags;

  if(m_fullscreen.m_value)
    {
      video_flags=SDL_WINDOW_FULLSCREEN;
    }
  else
    {
      video_flags = SDL_WINDOW_RESIZABLE;
    }


  video_flags|=SDL_WINDOW_OPENGL;
  if(m_libGL.set_by_command_line())
    {
      SDL_GL_LoadLibrary(m_libGL.m_value.c_str());
    }
  
  /*
    set GL attributes:
  */
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
  if(m_stencil_bits.m_value>=0)
    {
      SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, m_stencil_bits.m_value);
    }
  
  if(m_depth_bits.m_value>=0)
    {
      SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, m_depth_bits.m_value);
    }
  
  if(m_red_bits.m_value>=0)
    {
      SDL_GL_SetAttribute( SDL_GL_RED_SIZE, m_red_bits.m_value);
    }
  
  if(m_green_bits.m_value>=0)
    {
      SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, m_green_bits.m_value);
    }
  
  if(m_blue_bits.m_value>=0)
    {
      SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, m_blue_bits.m_value);
    }
  
  if(m_alpha_bits.m_value>=0)
    {
      SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, m_alpha_bits.m_value);
    }

  #ifdef WRATH_GLES_VERSION
  {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, m_gl_major.m_value);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, m_gl_minor.m_value);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  }
  #else
  {
    if(m_gl_major.m_value>=3)
      {
        int context_flags(0);
        int profile_mask(0);
        
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, m_gl_major.m_value);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, m_gl_minor.m_value);
        
        if(m_gl_forward_compatible_context.m_value)
          {
            context_flags|=SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
          }
        
        if(m_gl_debug_context.m_value)
          {
            context_flags|=SDL_GL_CONTEXT_DEBUG_FLAG;
          }
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, context_flags);
        
        if(m_gl_core_profile.m_value)
          {
            profile_mask=SDL_GL_CONTEXT_PROFILE_CORE;
          }
        else
          {
            profile_mask=SDL_GL_CONTEXT_PROFILE_COMPATIBILITY;
          }
        
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile_mask);
      }
  }
  #endif


  // Create the SDL window
  m_window = SDL_CreateWindow("",
                              0, 0,
                              m_width.m_value,
                              m_height.m_value,
                              video_flags);

  if(m_window==NULL)
    {
      /*
        abort
      */
      std::cerr << "\nFailed on SDL_CreateWindow: "<< SDL_GetError() << "\n";
      return routine_fail;
    }
  
  m_ctx=SDL_GL_CreateContext(m_window);
  if(m_ctx==NULL)
    {
      std::cerr << "Unable to create GL context: " << SDL_GetError() << "\n";
      return routine_fail;
    }
  SDL_GL_MakeCurrent(m_window, m_ctx);

  int w, h;
  SDL_GetWindowSize(m_window, &w, &h);
  m_ep=WRATHNew FURYSDL::EventProducer(w, h);
  m_connect=m_ep->connect( boost::bind(&DemoKernelMaker::pre_handle_event,
                                       this, _1));
  if(m_hide_cursor.m_value)
    {
      SDL_ShowCursor(SDL_DISABLE);
    }

  if(!m_log_gl_file.m_value.empty())
    {
      std::ostream *ostr;
      if(m_log_gl_file.m_value=="stderr")
	{
	  ostr=&std::cerr;
	}
      else if(m_log_gl_file.m_value=="stdout")
	{
	  ostr=&std::cout;
	} 
      else
	{
	  m_gl_log=WRATHNew std::ofstream(m_log_gl_file.m_value.c_str());
	  ostr=m_gl_log;
	}
      
      ngl_LogStream(ostr);
    }

  ngl_log_gl_commands(m_log_all_gl.m_value);
        

  if(!m_log_alloc_commands.m_value.empty())
    {
      std::ostream *ostr;
      if(m_log_alloc_commands.m_value=="stderr")
	{
	  ostr=&std::cerr;
	}
      else if(m_log_alloc_commands.m_value=="stdout")
	{
	  ostr=&std::cout;
	} 
      else
	{
          if(m_log_alloc_commands.m_value!=m_log_gl_file.m_value)
            {
              m_alloc_log=WRATHNew std::ofstream(m_log_alloc_commands.m_value.c_str());
              ostr=m_alloc_log;
            }
          else
            {
              ostr=m_gl_log;
            }
	}
      WRATHMemory::set_new_log(ostr);
    }

  

  if(m_print_gl_info.m_value)
    {
      std::cout << "\nGL_VERSION:" << glGetString(GL_VERSION)
                << "\nGL_VENDOR:" << glGetString(GL_VENDOR)
                << "\nGL_RENDERER:" << glGetString(GL_RENDERER)
                << "\nGL_SHADING_LANGUAGE_VERSION:" << glGetString(GL_SHADING_LANGUAGE_VERSION)
                << "\nGL_MAX_VERTEX_ATTRIBS:" << WRATHglGet<GLint>(GL_MAX_VERTEX_ATTRIBS)
                << "\nGL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:" << WRATHglGet<GLint>(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS);

      #ifdef WRATH_GL_VERSION
      {
        std::cout << "\nGL_MAX_CLIP_DISTANCES:" << WRATHglGet<GLint>(GL_MAX_CLIP_DISTANCES);

        if(ngl_functionExists(glGetStringi))
          {
            int cnt;
            
            cnt=WRATHglGet<GLint>(GL_NUM_EXTENSIONS);
            std::cout << "\nGL_EXTENSIONS(" << cnt << "):";
            for(int i=0; i<cnt; ++i)
              {
                std::cout << "\n\t" << glGetStringi(GL_EXTENSIONS, i);
              }
          }
        else
          {
            std::cout << "\nGL_EXTENSIONS:" << glGetString(GL_EXTENSIONS);
          }
        
      }
      #else
      {
        std::cout << "\nGL_EXTENSIONS:" << glGetString(GL_EXTENSIONS);
      }
      #endif 
      std::cout << "\n";
    }

  /*
    this is GL-lame. GL core profiles starting in version 3.1
    require that a VAO is bound, so we just generate one, leave
    it bound and delete it later.
    
    Comment: NGL system creates a macro for each GL entry point,
    thus to see if it is defined in the header just an #ifdef
    is neeed. If the function is implemented by GL/GLES,
    that requires the function-macro ngl_functionExists  
   */
  #ifdef glBindVertexArray
  {
    if(ngl_functionExists(glBindVertexArray))
      {
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
      }
  }
  #endif


  return routine_success;
}

DemoKernelMaker::
~DemoKernelMaker()
{
  
}

void
DemoKernelMaker::
pre_handle_event(FURYEvent::handle ev)
{
  if(!m_end_demo_flag and m_d!=NULL 
     and ev.valid())
    {
      m_d->handle_event(ev);
      switch(ev->type())
        {
        case FURYEvent::Resize:
          {
            m_call_update=true;
          }
          break;
         
        case FURYEvent::Quit:
        case FURYEvent::Close:
          {
            m_end_demo_flag=true;
          }
          break;

	case FURYEvent::KeyUp:
	  {
	    FURYKeyEvent::handle qe(ev.static_cast_handle<FURYKeyEvent>());
	    if(qe->key().m_value == FURYKey_Escape)
	      {
		m_end_demo_flag=true;
	      }
	  }
	  break;

        default:
          break;
        }

    }
}


int
DemoKernelMaker::
main(int argc, char **argv)
{

  if(argc==2 and std::string(argv[1])==std::string("-help"))
    {
      std::cout << "\n\nUsage: " << argv[0];
      print_help(std::cout);
      print_detailed_help(std::cout);
      return 0;
    }


  std::cout << "\n\nRunning: \"";
  for(int i=0;i<argc;++i)
    {
      std::cout << argv[i] << " ";
    }

  parse_command_line(argc, argv);
  std::cout << "\n\n" << std::flush;

  enum return_code R;
  R=init_sdl();

  if(R==routine_fail)
    {
      return -1;
    }

  m_d=make_demo();
  m_call_update=true;
  m_end_demo_flag=false;

  while(!m_end_demo_flag)
    {
      SDL_Event ev;
      while(SDL_PollEvent(&ev))
        {
          if(ev.type==SDL_WINDOWEVENT and 
             (ev.window.type==SDL_WINDOWEVENT_EXPOSED or ev.window.type==SDL_WINDOWEVENT_SHOWN))
            {
              m_call_update=true;
            }
          m_ep->feed_event(&ev);
        }

      if(m_call_update and !m_end_demo_flag)
        {
          m_call_update=false;
          m_d->paint();
          SDL_GL_SwapWindow(m_window);
        }
      else
        {
          /*
            take a short nap if we do not draw..
           */
          SDL_Delay(1);
        }
    }

  if(m_d!=NULL)
    {
      m_connect.disconnect();
      delete_demo(m_d);
    }

  WRATHDelete(m_ep);
  m_ep=NULL;

  #ifdef glBindVertexArray
  {
    if(m_vao!=0)
      {
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &m_vao);
      }
  }
  #endif
  
  SDL_ShowCursor(SDL_ENABLE);
  SDL_SetWindowGrab(m_window, SDL_FALSE);
      
  SDL_GL_MakeCurrent(m_window, NULL);
  SDL_GL_DeleteContext(m_ctx);
  
  SDL_DestroyWindow(m_window);
  SDL_Quit();

  ngl_LogStream(NULL);
  ngl_log_gl_commands(false);
  WRATHMemory::set_new_log(NULL);

  if(m_gl_log!=NULL and m_gl_log!=m_alloc_log)
    {
      WRATHDelete(m_gl_log);
    }
  if(m_alloc_log!=NULL)
    {
      WRATHDelete(m_alloc_log);
    }

  return 0;
}



///////////////////////////
// DemoKernel methods 
void
DemoKernel::
end_demo(void)
{
  WRATHassert(m_q!=NULL);
  if(m_q!=NULL and !m_q->m_end_demo_flag)
    {
      SDL_SetWindowGrab(m_q->m_window, SDL_FALSE);

      m_q->m_end_demo_flag=true;
      m_q->m_connect.disconnect();
    }
}

bool
DemoKernel::
demo_ended(void)
{
  WRATHassert(m_q!=NULL);
  return m_q->m_end_demo_flag;
}


void
DemoKernel::
update_widget(void)
{
  WRATHassert(m_q!=NULL);
  m_q->m_call_update=true;    
}


ivec2
DemoKernel::
size(void)
{
  int w, h;
  WRATHassert(m_q!=NULL and m_q->m_window!=NULL);
  SDL_GetWindowSize(m_q->m_window, &w, &h);
  return ivec2(w,h);
}

int
DemoKernel::
width(void)
{
  return size().x();
}

int
DemoKernel::
height(void)
{
  return size().y();
}

void
DemoKernel::
titlebar(const std::string &title)
{
  SDL_SetWindowTitle(m_q->m_window, title.c_str());
}

void //true=grab
DemoKernel::
grab_mouse(bool v)
{
  SDL_SetWindowGrab(m_q->m_window, (v) ? SDL_TRUE : SDL_FALSE);
}

void //true=grab
DemoKernel::
grab_keyboard(bool v)
{
  grab_mouse(v);
}

void
DemoKernel::
enable_key_repeat(bool v)
{
  m_q->m_ep->enable_key_repeat(v);
}

void
DemoKernel::
enable_text_event(bool v)
{
  m_q->m_ep->enable_text_mode(v);
}

