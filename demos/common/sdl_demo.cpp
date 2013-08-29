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
#include "SDL_syswm.h"
#include "vectorGL.hpp"
#include "sdl_demo.hpp"

#ifdef HARMATTAN
#include <policy/resource-set.h>
#endif

#ifndef WRATH_GL_VERSION
#include <EGL/egl.h>
#endif

namespace
{
#ifndef WRATH_GL_VERSION
  class egl_data_type
  {
  public:
    EGLContext m_egl_context;
    EGLSurface m_egl_surface;
    EGLDisplay m_egl_display;
    EGLNativeWindowType m_egl_window;
    EGLConfig m_egl_cfg;
    EGLint m_egl_major, m_egl_minor;
    bool m_egl_ready;
    int m_egl_frame_count;

      struct timeval m_start_time;

      egl_data_type(void):
        m_egl_context(EGL_NO_CONTEXT),
        m_egl_surface(EGL_NO_SURFACE),
        m_egl_display(EGL_NO_DISPLAY),
        m_egl_major(0),
        m_egl_minor(0),
        m_egl_ready(false),
        m_egl_frame_count(0)
      {}
  };

  egl_data_type&
  egl_data(void)
  {
    static egl_data_type e;
    return e;
  }
#endif
}

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
  m_depth_bits(16, "depth_bits", 
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
  m_end_demo_flag(false),
  m_d(NULL),
  m_ep(NULL),
  m_window(NULL)
{}

enum return_code
DemoKernelMaker::   
init_sdl(void)
{
#if !defined(WRATH_GL_VERSION)
  // Get the EGL configs and the compatible bpp

  // Get the display using the DISPLAY environment variable
  // similar to the SDL implementation.
  Display *x_display = XOpenDisplay(NULL);
  egl_data().m_egl_display = eglGetDisplay(x_display);
 
  // Check if we have a display - choose default if not
  if(egl_data().m_egl_display == EGL_NO_DISPLAY)
    {
      std::cerr << "No EGL display. Using EGL_DEFAULT_DISPLAY.\n";
      egl_data().m_egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }

  // Get the configs
  std::vector<EGLConfig> egl_configs;

  if(get_egl_configs(&egl_configs) != routine_success)
    {
      std::cerr << "\nFailed to get EGL configuration\n";
      return routine_fail;
    }

  // Fetch the bpp
  m_bpp.m_value = get_egl_compatible_bpp(&egl_configs);

  // Terminate the display connection
  eglTerminate(egl_data().m_egl_display);

  // Set the egl_data config variable
  egl_data().m_egl_display = EGL_NO_DISPLAY;

  // Close the display (SDL will open it's own)
  XCloseDisplay(x_display);

#endif

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
  const SDL_VideoInfo *video_info = SDL_GetVideoInfo();

  video_flags = SDL_RESIZABLE;

  if(video_info == NULL)
    {
      /*
        abort
      */
      std::cerr << "\nFailed on SDL_GetVideoInfo\n";
      return routine_fail;
    }

  if(m_fullscreen.m_value)
    {
      video_flags=video_flags | SDL_FULLSCREEN;

      // Setting w/h to 0 will create a full screen window size
      m_width.m_value = 0;
      m_height.m_value = 0;
    }


#if defined(WRATH_GL_VERSION)
  {
    video_flags|=SDL_OPENGL;

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
  }

#endif

  

  // Create the SDL window
  m_window = SDL_SetVideoMode(m_width.m_value,
                              m_height.m_value,
                              m_bpp.m_value, 
                              video_flags);

  if(m_window==NULL)
    {
      /*
        abort
      */
      std::cerr << "\nFailed on SDL_SetVideoMode\n";
      return routine_fail;
    }
  
  
  m_ep=WRATHNew FURYSDL::EventProducer(m_window->w, m_window->h);
  m_connect=m_ep->connect( boost::bind(&DemoKernelMaker::pre_handle_event,
                                       this, _1));


#if !defined(WRATH_GL_VERSION)
  // Set the window and display to the ones created by SDL
  SDL_SysWMinfo x11info;

  memset(&x11info, 0, sizeof(x11info));
  SDL_GetWMInfo(&x11info);

  // Get the display from SDL
  egl_data().m_egl_display = eglGetDisplay((EGLNativeDisplayType)x11info.info.x11.display);
  egl_data().m_egl_window = (EGLNativeWindowType)x11info.info.x11.window;

  // Reget the configs with the display used by SDL
  if(get_egl_configs(&egl_configs) != routine_success)
    {
      std::cerr << "\nFailed to get EGL configuration\n";
      return routine_fail;
    }

  int config = choose_egl_config(&egl_configs);


  if(egl_data().m_egl_display == EGL_NO_DISPLAY)
    {
      std::cerr << "Error fetching display from SDL for context creation.\n";
      return routine_fail;
    }

  if(config >= 0)
    {
      std::cout << "Config: " << config << std::endl;
      create_and_bind_context(&egl_configs[config]);
    }
#endif

  if(m_hide_cursor.m_value)
    {
      SDL_ShowCursor(SDL_DISABLE);
    }

  return routine_success;
}

DemoKernelMaker::
~DemoKernelMaker()
{
  
}

#if !defined(WRATH_GL_VERSION)
enum return_code
DemoKernelMaker::
create_and_bind_context(EGLConfig* egl_config)
{
  /*
    Create the EGL Surface: 
   */
  egl_data().m_egl_surface = eglCreateWindowSurface(egl_data().m_egl_display, (*egl_config),
                                                  egl_data().m_egl_window, NULL);
  
  EGLint error_code;

  if(egl_data().m_egl_surface == EGL_NO_SURFACE)
    {
      error_code = eglGetError();
      std::cerr << "Unable to create EGLSurface (" << error_code << ")\n";
      return routine_fail;
    }

  // Generate the config list for egl context
  EGLint ctx_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  egl_data().m_egl_context = eglCreateContext(egl_data().m_egl_display,
                                              (*egl_config),
                                              EGL_NO_CONTEXT, 
                                              &ctx_attribs[0]);

  if(egl_data().m_egl_context == EGL_NO_CONTEXT)
    {
      error_code = eglGetError();
      std::cerr << "Error creating context (" << error_code << ")\n";
      return routine_fail;
    }

  // Make current context
  eglMakeCurrent(egl_data().m_egl_display,
                 egl_data().m_egl_surface,
                 egl_data().m_egl_surface,
                 egl_data().m_egl_context);

  gettimeofday(&egl_data().m_start_time, NULL);

  return routine_success;
}

enum return_code
DemoKernelMaker::
get_egl_configs(std::vector<EGLConfig>* p_egl_configs)
{
  EGLint number_egl_configs(0);
  EGLint error_code;

  // Initialize EGL
  if(eglInitialize(egl_data().m_egl_display, &egl_data().m_egl_major, &egl_data().m_egl_minor) != EGL_TRUE) 
    {
      error_code = eglGetError();
      std::cerr << "Could not initialize egl (" << error_code << ")\n";
      return routine_fail;
    }

  // Generate the config list for egl
  EGLint basic_attribs[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_RED_SIZE, m_red_bits.m_value < 0 ? 0 : m_red_bits.m_value,
    EGL_GREEN_SIZE, m_green_bits.m_value < 0 ? 0 : m_green_bits.m_value,
    EGL_BLUE_SIZE, m_blue_bits.m_value < 0 ? 0 : m_green_bits.m_value,
    EGL_ALPHA_SIZE, m_alpha_bits.m_value < 0 ? 0 : m_alpha_bits.m_value,
    EGL_DEPTH_SIZE, m_depth_bits.m_value < 0 ? 0 : m_depth_bits.m_value,
    EGL_STENCIL_SIZE, m_stencil_bits.m_value < 0 ? 0 : m_stencil_bits.m_value,
  };

  std::vector<EGLint> config_attribs(
      basic_attribs, basic_attribs + sizeof(basic_attribs) / sizeof(basic_attribs[0]));

  // Check for MSAA
  if(m_use_msaa.m_value && m_msaa.m_value > 0)
    {
      config_attribs.push_back(EGL_SAMPLE_BUFFERS);
      config_attribs.push_back(1);

      config_attribs.push_back(EGL_SAMPLES);
      config_attribs.push_back(m_msaa.m_value);
    }

  // End the configs
  config_attribs.push_back(EGL_NONE);

  // See how many configs the system would return
  if(EGL_FALSE==eglGetConfigs(egl_data().m_egl_display, NULL, 0, &number_egl_configs))
    {
      error_code = eglGetError();
      std::cerr << "Unable to get configuration (" << error_code << ")\n";
      return routine_fail;
    }

  if(number_egl_configs==0)
    {
      std::cerr << "EGL said there are not any configs!\n";
      return routine_fail;
    }

  p_egl_configs->resize(number_egl_configs);

  // Fetch the config(s) that EGL says are ok
  if(EGL_FALSE == eglChooseConfig(egl_data().m_egl_display, &config_attribs[0],
                                  &(*p_egl_configs)[0], p_egl_configs->size(), &number_egl_configs))
    {
      error_code = eglGetError();
      std::cerr << "Unable to choose configuration (" << error_code << ")\n";
      return routine_fail;
    }

  return routine_success;
}

unsigned int
DemoKernelMaker::
get_egl_compatible_bpp(std::vector<EGLConfig> *egl_configs)
{
  // Choose the first matching bpp
  EGLint value;

  for(unsigned int i = 0; i < egl_configs->size(); i++)
    {
      eglGetConfigAttrib(egl_data().m_egl_display, (*egl_configs)[i], EGL_BUFFER_SIZE, &value);

      if(value == m_bpp.m_value)
        {
          return value;
        }
    }

  std::cerr << "Unable to find compatible bpp!\n";
  return 0;
}

int
DemoKernelMaker::choose_egl_config(std::vector<EGLConfig>* egl_configs)
{
  EGLint r_value, g_value, b_value, alpha_value, samples_value, sample_buffers_value;
#define GET_ATTRIB(name,value) eglGetConfigAttrib(egl_data().m_egl_display,(*egl_configs)[i],name,value);
  for(unsigned int i = 0; i < egl_configs->size(); i++)
    {
      // Color depth
      GET_ATTRIB(EGL_RED_SIZE, &r_value);
      if(m_red_bits.m_value >= 0 && m_red_bits.m_value != r_value) continue;

      GET_ATTRIB(EGL_GREEN_SIZE, &g_value);
      if(m_green_bits.m_value >= 0 && m_green_bits.m_value != g_value) continue;

      GET_ATTRIB(EGL_BLUE_SIZE, &b_value);
      if(m_blue_bits.m_value >= 0 && m_blue_bits.m_value != b_value) continue;

      GET_ATTRIB(EGL_ALPHA_SIZE, &alpha_value);
      if(m_alpha_bits.m_value >= 0 && m_alpha_bits.m_value != alpha_value) continue;

      // MSAA checks
      if(m_use_msaa.m_value && m_msaa.m_value > 0) 
        {
          GET_ATTRIB(EGL_SAMPLES, &samples_value);
          if(samples_value != m_msaa.m_value) continue;

          GET_ATTRIB(EGL_SAMPLE_BUFFERS, &sample_buffers_value);
          if(sample_buffers_value == 0) continue;
        }

      return i;
    }
  // Return -1 if no configuration was found
  return -1;
#undef GET_ATTRIB
}

#endif

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
            int bpp;
            Uint32 flags;
            
            bpp=m_window->format->BitsPerPixel;
            flags=m_window->flags;
            
            FURYResizeEvent::handle rev(ev.static_cast_handle<FURYResizeEvent>());
            m_window=SDL_SetVideoMode(rev->new_size().x(),
                                      rev->new_size().y(),
                                      bpp, flags);
            m_call_update=true;
          }
          break;
         
        case FURYEvent::Quit:
        case FURYEvent::Close:
          {
            m_end_demo_flag=true;
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
          if(ev.type==SDL_ACTIVEEVENT or ev.type==SDL_VIDEOEXPOSE)
            {
              m_call_update=true;
            }
          m_ep->feed_event(&ev);
        }

      if(m_call_update and !m_end_demo_flag)
        {
          m_call_update=false;
          m_d->paint();
          #if defined(WRATH_GL_VERSION)
          {
            SDL_GL_SwapBuffers();
          }
          #else
          {
            eglSwapBuffers(egl_data().m_egl_display, egl_data().m_egl_surface);
          }
          #endif
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

  SDL_ShowCursor(SDL_ENABLE);
  SDL_WM_GrabInput(SDL_GRAB_OFF);

#if !defined(WRATH_GL_VERSION)
  // Destroy surface if it exists
  if(egl_data().m_egl_surface != EGL_NO_SURFACE)
    {
      // Remove context from use
      eglMakeCurrent(egl_data().m_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      // Destroy surface
      eglDestroySurface(egl_data().m_egl_display, egl_data().m_egl_surface);
      // Set the egl_data config variable
      egl_data().m_egl_surface = EGL_NO_SURFACE;
    }

  // Destroy the context if it exists
  if(egl_data().m_egl_context != EGL_NO_CONTEXT)
    {
      // Destroy context
      eglDestroyContext(egl_data().m_egl_display, egl_data().m_egl_context);
      // Set the egl_data config variable
      egl_data().m_egl_context = EGL_NO_CONTEXT;
    }

  // Terminate the display if it exists
  if(egl_data().m_egl_display != EGL_NO_DISPLAY)
    {
      // Terminate the display
      eglTerminate(egl_data().m_egl_display);
      // Set the egl_data config variable
      egl_data().m_egl_display = EGL_NO_DISPLAY;
    }

  egl_data().m_egl_ready = false;
#endif

  SDL_Quit();


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
      SDL_WM_GrabInput(SDL_GRAB_OFF);

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
  WRATHassert(m_q!=NULL and m_q->m_window!=NULL);
  return ivec2(m_q->m_window->w,
               m_q->m_window->h);
}

int
DemoKernel::
width(void)
{
  WRATHassert(m_q!=NULL and m_q->m_window!=NULL);
  return m_q->m_window->w;
}

int
DemoKernel::
height(void)
{
  WRATHassert(m_q!=NULL and m_q->m_window!=NULL);
  return m_q->m_window->h;
}

void
DemoKernel::
titlebar(const std::string &title)
{
  SDL_WM_SetCaption(title.c_str(), NULL);
}

void //true=grab
DemoKernel::
grab_mouse(bool v)
{
  if(v)
    {
      SDL_WM_GrabInput(SDL_GRAB_ON);
    }
  else
    {
      SDL_WM_GrabInput(SDL_GRAB_OFF);      
    }
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

