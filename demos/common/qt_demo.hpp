/*! 
 * \file qt_demo.hpp
 * \brief file qt_demo.hpp
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


#ifndef QT_DEMO_HPP
#define QT_DEMO_HPP


#include "WRATHConfig.hpp"
#include "WRATHgl.hpp"
#include "generic_command_line.hpp"
#include "FURYQtEvent.hpp"

class DemoWidget;
class DemoKernel;
class DemoKernelMaker;

class DemoKernel:boost::noncopyable
{
public:
  DemoKernel(DemoKernelMaker *q):
    m_q(q)
  {}

  virtual
  ~DemoKernel()
  {}

  /*
    implement to draw the contents
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
  friend class DemoWidget;
  friend class DemoKernelMaker;
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

  command_line_argument_value<int> m_gl_major, m_gl_minor;
  command_line_argument_value<bool> m_gl_forward_compatible_context;
  command_line_argument_value<bool> m_gl_debug_context;
  command_line_argument_value<bool> m_gl_core_profile;

  command_line_argument_value<std::string> m_log_gl_commands;
  command_line_argument_value<std::string> m_log_alloc_commands;
  command_line_argument_value<bool> m_print_gl_info;

  DemoKernelMaker(void):
    m_red_bits(-1, "red_bits", 
               "Bpp of red channel, non-positive values mean use Qt defaults",
               *this),
    m_green_bits(-1, "green_bits", 
                 "Bpp of green channel, non-positive values mean use Qt defaults",
                 *this),
    m_blue_bits(-1, "blue_bits", 
                "Bpp of blue channel, non-positive values mean use Qt defaults",
                *this),
    m_alpha_bits(-1, "alpha_bits", 
                 "Bpp of alpha channel, non-positive values mean use Qt defaults",
                 *this),
    m_depth_bits(-1, "depth_bits", 
                 "Bpp of depth buffer, non-positive values mean use Qt defaults",
                 *this),
    m_stencil_bits(-1, "stencil_bits", 
                 "Bpp of stencil buffer, non-positive values mean use Qt defaults",
                 *this),
    m_fullscreen(false, "fullscreen", "fullscreen mode", *this),
    m_hide_cursor(false, "hide_cursor", "If true, hide the mouse cursor with a Qt call", *this),
    m_use_msaa(false, "enable_msaa", "If true enables MSAA", *this),
    m_msaa(4, "msaa_samples", 
           "If greater than 0, specifies the number of samples "
           "to request for MSAA. If not, Qt will choose the "
           "sample count as the highest available value",
           *this),

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
    
    m_log_gl_commands("", "log_gl", "if non-empty, GL commands are logged to the named file. "
                      "If value is stderr then logged to stderr, if value is stdout logged to stdout", *this),
    m_log_alloc_commands("", "log_alloc", "If non empty, logs allocs and deallocs to the named file", *this),
    m_print_gl_info(false, "print_gl_info", "If true print to stdout GL information", *this),

    m_w(NULL)
  {}

  virtual
  ~DemoKernelMaker()
  {}

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
  friend class DemoWidget;
  friend class DemoKernel;

  DemoWidget *m_w;

};



#endif
