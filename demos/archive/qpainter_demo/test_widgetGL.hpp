/*! 
 * \file test_widgetGL.hpp
 * \brief file test_widgetGL.hpp
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


#ifndef TEST_WIDGET_GL_HPP
#define TEST_WIDGET_GL_HPP

#include <QGLWidget>
#include "test_list.hpp"

class TestWidgetGL:public QGLWidget
{
public:

  TestWidgetGL(const main_widget_command_line &cmd,
               const QGLFormat &fmt);
  ~TestWidgetGL(void);

protected:
    
  virtual 
  void
  keyPressEvent(QKeyEvent *event);
  
  virtual
  void
  mousePressEvent(QMouseEvent *event);
  
  void
  paintEvent(QPaintEvent*);

  void
  resizeEvent(QResizeEvent*);
  
private:

  TestList *m_list;
};


#endif
