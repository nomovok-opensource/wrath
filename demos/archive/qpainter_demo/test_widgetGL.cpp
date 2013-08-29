/*! 
 * \file test_widgetGL.cpp
 * \brief file test_widgetGL.cpp
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


#include "test_widgetGL.hpp"

TestWidgetGL::
TestWidgetGL(const main_widget_command_line &cmd,
             const QGLFormat &fmt):
  QGLWidget(fmt)
{
  m_list=new TestList(cmd, this);
  if(!cmd.m_fullscreen.m_value)
    {
      resize(cmd.m_window_width.m_value,
             cmd.m_window_height.m_value);
    }
  else
    {
      setWindowState(windowState() | Qt::WindowFullScreen);
    }

}

TestWidgetGL::
~TestWidgetGL()
{}

void
TestWidgetGL::
resizeEvent(QResizeEvent*)
{
  //std::cout << "\nRes=" << size().width()
  //        << "x" << size().height();
  m_list->resize(size());
}

void
TestWidgetGL::
keyPressEvent(QKeyEvent*)
{
  deleteLater();
}

void
TestWidgetGL::
mousePressEvent(QMouseEvent*)
{
  m_list->togglePaused();
}

void
TestWidgetGL::
paintEvent(QPaintEvent*)
{
  

  QPainter p(this);
  p.setRenderHint(QPainter::SmoothPixmapTransform);

  p.beginNativePainting();
  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
  p.endNativePainting();

  m_list->draw(&p);
  m_list->update_data(size());
  //update();

  if(m_list->timeToWRATHDelete())
    {
      deleteLater();
    }
}
