/*! 
 * \file sdl_demo.hpp
 * \brief file sdl_demo.hpp
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


#ifndef SDL_DEMO_HPP
#define SDL_DEMO_HPP


#include "WRATHConfig.hpp"
#include "generic_command_line.hpp"
#include "FURYSDLEvent.hpp"
#include "WRATHgl.hpp"
#include "ngl_backend.hpp"

#include <iostream>
#include <sys/time.h>
#include <vector>
#include <SDL/SDL.h>

#ifndef WRATH_GL_VERSION
#include <EGL/egl.h>
#endif

class DemoKernel;
class DemoKernelMaker;

class DemoKernel
{
public:
  DemoKernel(DemoKernelMaker *q):
    m_q(q)
  {}

  virtual
  ~DemoKernel()
  {}

  /*
    implement to draw the contents.
   */
  virtual
  void
  paint(void)=0;

  /*
    implement to handle an event.
   */
  virtual
  void
  handle_event(FURYEvent::handle)=0;

protected:

  /*!
    Returns true if the demo is "ended".
   */
  bool
  demo_ended(void);
  
  /*!
    Signal to end the demo, closing
    the DemoWidget as well.
   */
  void
  end_demo(void);

  /*
    "signal" that widget needs to be repainted
   */
  void
  update_widget(void);

  /*
    return the size of the window
   */
  ivec2
  size(void);

  /*
    same as size().x()
   */
  int
  width(void);

  /*
    same as size().y()
   */
  int
  height(void);

  /*
    set the title bar
   */
  void
  titlebar(const std::string &title);

  /*
    "grab the mouse", 
    \param v v=true grab the mouse, v=false release the mouse
   */
  void 
  grab_mouse(bool v);

  /*
    "grab the keyboard", 
    \param v v=true grab the keyboard, v=false release the keyboard
   */
  void 
  grab_keyboard(bool v);

  /*
    TODO:
    - enable/disable grab all Qt events
   */

  /*
    Enable key repeat, i.e. holding key
    generates lots of key events.
   */
  void
  enable_key_repeat(bool v);


  /*
    interpret key events as text events.
   */
  void
  enable_text_event(bool v);

private:

  DemoKernelMaker *m_q;
};

class DemoKernelMaker:public command_line_register
{
public:
  command_line_argument_value<int> m_red_bits;
  command_line_argument_value<int> m_green_bits;
  command_line_argument_value<int> m_blue_bits;
  command_line_argument_value<int> m_alpha_bits;
  command_line_argument_value<int> m_depth_bits;
  command_line_argument_value<int> m_stencil_bits;
  command_line_argument_value<bool> m_fullscreen;
  command_line_argument_value<bool> m_hide_cursor;
  command_line_argument_value<bool> m_use_msaa;
  command_line_argument_value<int> m_msaa;
  command_line_argument_value<int> m_width;
  command_line_argument_value<int> m_height;
  command_line_argument_value<int> m_bpp;
  command_line_argument_value<std::string> m_libGL;

  DemoKernelMaker(void);

  virtual
  ~DemoKernelMaker();

  virtual
  DemoKernel*
  make_demo(void)=0;

  virtual
  void
  delete_demo(DemoKernel*)=0;

  /*
    call this as your main.
   */
  int
  main(int argc, char **argv);

private:

  friend class DemoKernel;

  void
  pre_handle_event(FURYEvent::handle);

  enum return_code
  init_sdl(void);

#ifndef WRATH_GL_VERSION
  enum return_code
  get_egl_configs(std::vector<EGLConfig>* p_egl_configs);

  unsigned int
  get_egl_compatible_bpp(std::vector<EGLConfig>* target);

  int
  choose_egl_config(std::vector<EGLConfig>* configs);

  enum return_code
  create_and_bind_context(EGLConfig* config);
#endif

  bool m_end_demo_flag;
  bool m_call_update;

  DemoKernel *m_d;
  FURYSDL::EventProducer *m_ep;
  FURYSDL::EventProducer::connect_t m_connect;
  SDL_Surface *m_window;
};


#endif
