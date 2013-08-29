/*! 
 * \file test_widget.cpp
 * \brief file test_widget.cpp
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


#include <QKeyEvent>
#include <QMouseEvent>
#include "test_widget.hpp"

TestWidget::
TestWidget(const main_widget_command_line &cmd):
  QWidget()
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

TestWidget::
~TestWidget()
{}

void
TestWidget::
keyPressEvent(QKeyEvent *qe)
{
  switch(qe->key())
    {
    case Qt::Key_Space:
      m_list->togglePaused();
      break;
    default:
      deleteLater();
    }
  qe->accept();
}

void
TestWidget::
mousePressEvent(QMouseEvent *qe)
{
  m_list->togglePaused();
  qe->accept();
}

void
TestWidget::
resizeEvent(QResizeEvent*)
{
  // std::cout << "\nRes=" << size().width()
  //        << "x" << size().height();
  m_list->resize(size());
}

void
TestWidget::
paintEvent(QPaintEvent*)
{
  QPainter p(this);
  p.setRenderHint(QPainter::SmoothPixmapTransform);

  m_list->draw(&p);
  m_list->update_data(size());
  //update();
  if(m_list->timeToWRATHDelete())
    {
      deleteLater();
    }
}
