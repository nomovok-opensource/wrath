/*! 
 * \file main.cpp
 * \brief file main.cpp
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


#include <QtOpenGL>
#include <fstream>
#include <iomanip>
#include <limits>
#include "transformation_node.hpp"
#include "test_widget.hpp"
#include "image_item.hpp"
#include "draw_item.hpp"
#include "text_item.hpp"
#include "generic_command_line.hpp"
#include "test_widget.hpp"
#include "test_widgetGL.hpp"




class cmd_line_type:public main_widget_command_line
{
public:
  /* config bits */
  command_line_argument_value<int> m_red_bits;
  command_line_argument_value<int> m_green_bits;
  command_line_argument_value<int> m_blue_bits;
  command_line_argument_value<int> m_alpha_bits;
  command_line_argument_value<bool> m_gl_widget;

  cmd_line_type(void):
    main_widget_command_line(),

    m_red_bits(0, "red_bits", "Red bit depth", *this),
    m_green_bits(0, "green_bits", "Green bit depth", *this),
    m_blue_bits(0, "blue_bits", "Blue bit depth", *this),
    m_alpha_bits(0, "alpha_bits", "Alpha bit depth", *this),
    m_gl_widget(false, "gl", "Use GL widget", *this)
  {}

    

};      
        




int 
main(int argc, char **argv)
{
  cmd_line_type cmd_line;

  if(argc==2 and std::string(argv[1])==std::string("-help"))
    {
      std::cout << "\n\nUsage: " << argv[0];
      cmd_line.print_help(std::cout);
      
      cmd_line.print_detailed_help(std::cout);
      std::cout << "\n\n"
                << "Also don't forget -graphicssystem opengl/raster/native for Qt"
                << " to override rendering system of widget";
      return 0;
    }

  
  cmd_line.m_text_red.m_value=std::min(std::max(0, cmd_line.m_text_red.m_value), 255);
  cmd_line.m_text_green.m_value=std::min(std::max(0, cmd_line.m_text_green.m_value), 255);
  cmd_line.m_text_blue.m_value=std::min(std::max(0, cmd_line.m_text_blue.m_value), 255);

  
  //create widget and application:
  QApplication qapp(argc, argv);

  std::cout << "\n\nRunning: \"";
  for(int i=0;i<argc;++i)
    {
      std::cout << argv[i] << " ";
    }

  cmd_line.parse_command_line(argc, argv);
  std::cout << "\n\n";
  

  QWidget *widget;

  if(cmd_line.m_gl_widget.m_value)
    {
      QGLFormat fmt;
      
      fmt.setDoubleBuffer(true);

      if(cmd_line.m_red_bits.set_by_command_line())
        {
          fmt.setRedBufferSize(cmd_line.m_red_bits.m_value);
        }
      
      if(cmd_line.m_green_bits.set_by_command_line())
        {
          fmt.setGreenBufferSize(cmd_line.m_green_bits.m_value);
        }
      
      if(cmd_line.m_blue_bits.set_by_command_line())
        {
          fmt.setBlueBufferSize(cmd_line.m_blue_bits.m_value);
        }
      
      if(cmd_line.m_alpha_bits.set_by_command_line())
        {
          fmt.setAlphaBufferSize(cmd_line.m_alpha_bits.m_value);
        }

      widget=new TestWidgetGL(cmd_line, fmt);
    }
  else
    {
      widget=new TestWidget(cmd_line);      
    }

  
  widget->show();
  QTimer *timer;

  timer=new QTimer();
  timer->setSingleShot(false);
  timer->setInterval(cmd_line.m_animation_timer.m_value);
  QObject::connect(timer, SIGNAL(timeout()), widget, SLOT(update()));

  timer->start();

  return qapp.exec();
  
}
