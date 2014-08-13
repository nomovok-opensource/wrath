/*! 
 * \file qt_demo.cpp
 * \brief file qt_demo.cpp
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
#include "WRATHglGet.hpp"
#include "ngl_backend.hpp"
#include <QtGlobal> 
#include <QtOpenGL>
#include <QGLWidget>
#include <typeinfo>
#include <fstream>
#include "vectorGL.hpp"
#include "qt_demo.hpp"


#ifdef HARMATTAN
#include <policy/resource-set.h>
#endif

namespace
{
  #ifdef HARMATTAN
    ResourcePolicy::ResourceSet*&
    resource_set(void)
    {
      static ResourcePolicy::ResourceSet *v(NULL);
      return v;
    }
  #endif
  
  QGLFormat
  make_format(DemoKernelMaker *p)
  {
    QGLFormat fmt;

    Q_ASSERT(p!=NULL);

    if(p->m_red_bits.m_value>0)
      {
        fmt.setRedBufferSize(p->m_red_bits.m_value);
      }

    if(p->m_green_bits.m_value>0)
      {
        fmt.setGreenBufferSize(p->m_green_bits.m_value);
      }

    if(p->m_blue_bits.m_value>0)
      {
        fmt.setBlueBufferSize(p->m_blue_bits.m_value);
      }

    if(p->m_alpha_bits.m_value>0)
      {
        fmt.setAlphaBufferSize(p->m_alpha_bits.m_value);
      }

    if(p->m_depth_bits.m_value>0)
      {
        fmt.setDepthBufferSize(p->m_depth_bits.m_value);
      }

    if(p->m_stencil_bits.m_value>0)
      {
        fmt.setStencilBufferSize(p->m_stencil_bits.m_value);
      }

    if(p->m_use_msaa.m_value)
      {
        fmt.setSampleBuffers(true);
        if(p->m_msaa.m_value>0)
          {
            fmt.setSamples(p->m_msaa.m_value);
          }
      }

    return fmt;

  }

  Qt::WindowFlags
  make_flags(DemoKernelMaker*)
  {
    return 0;
  }

}


class DemoWidget:public QGLWidget
{
public:
  DemoWidget(DemoKernelMaker *maker);

  ~DemoWidget();

  virtual
  bool
  event(QEvent *ev);

protected:

  virtual
  void 
  paintGL(void);

  virtual
  void
  initializeGL(void);

private:
  friend class DemoKernel;
  friend class DemoKernelMaker;

  void
  pre_handle_event(FURYEvent::handle);

  bool m_end_demo_flag;

  std::ofstream *m_gl_log;
  std::ofstream *m_alloc_log;
  GLuint m_vao;

  DemoKernel *m_d;
  DemoKernelMaker *m_maker;
  FURYQT::EventProducer *m_ep;
  FURYQT::EventProducer::connect_t m_connect;
};



//////////////////////////////////////////
// DemoWidget methods
DemoWidget::
DemoWidget(DemoKernelMaker *pp):
  QGLWidget(make_format(pp), 0, 0, make_flags(pp)),
  m_end_demo_flag(false),
  m_gl_log(NULL),
  m_alloc_log(NULL),
  m_vao(0),
  m_d(NULL),
  m_maker(pp),
  m_ep(NULL)
{
  Q_ASSERT(m_maker!=NULL);
  m_maker->m_w=this;
  m_ep=WRATHNew FURYQT::EventProducer(this);
  m_connect=m_ep->connect( boost::bind(&DemoWidget::pre_handle_event,
                                       this, _1));

  if(m_maker->m_hide_cursor.m_value)
    {
      setCursor(Qt::BlankCursor);
    }

  setAttribute(Qt::WA_AcceptTouchEvents);
  setAttribute(Qt::WA_DeleteOnClose);

  /*
    TODO: Qt calls to observe m_make->gl_major and m_maker->gl_minor
   */

  /*
    Qt madness and idiocy... calling show or showFullscreen()
    will trigger it to go to paint() immediately rather than
    deffering it for later, we get around this by making sure 
    other bits are ready first.
   */

  if(m_maker->m_fullscreen.m_value)
    {
      showFullScreen();
    }
  else
    {
      show();
    }
}

DemoWidget::
~DemoWidget()
{
  if(m_d!=NULL)
    {
      m_connect.disconnect();
      m_maker->delete_demo(m_d);
    }
  WRATHDelete(m_ep);
  m_ep=NULL;
  m_maker->m_w=NULL;

  #ifdef glBindVertexArray
  {
    if(m_vao!=0)
      {
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &m_vao);
      }
  }
  #endif

  ngl_LogStream(NULL);
  ngl_log_gl_commands(false);
  WRATHMemory::set_new_log(NULL);

  if(m_gl_log!=NULL)
    {
      WRATHDelete(m_gl_log);
    }
  if(m_alloc_log!=NULL)
    {
      WRATHDelete(m_alloc_log);
    }
}


bool
DemoWidget::
event(QEvent *ev)
{
  if(m_end_demo_flag 
     or ev->type()==QEvent::Close 
     or ev->type()==QEvent::Quit)
    {
      m_connect.disconnect();
      m_maker->delete_demo(m_d);
      m_end_demo_flag=true;
      m_d=NULL;
    }
  else if(m_ep!=NULL)
    {
      m_ep->feed_event(ev);
    }

  return QGLWidget::event(ev);
}

void
DemoWidget::
pre_handle_event(FURYEvent::handle ev)
{
  if(!m_end_demo_flag and m_d!=NULL 
     and ev.valid() 
     and ev->type()!=FURYEvent::Quit
     and ev->type()!=FURYEvent::Close)
    {
      m_d->handle_event(ev);
    }
}


void
DemoWidget::
initializeGL(void)
{
  Q_ASSERT(m_d==NULL);

  if(!m_maker->m_log_gl_commands.m_value.empty())
    {
      std::ostream *ostr;
      if(m_maker->m_log_gl_commands.m_value=="stderr")
	{
	  ostr=&std::cerr;
	}
      else if(m_maker->m_log_gl_commands.m_value=="stdout")
	{
	  ostr=&std::cout;
	} 
      else
	{
	  m_gl_log=WRATHNew std::ofstream(m_maker->m_log_gl_commands.m_value.c_str());
	  ostr=m_gl_log;
	}
      
      ngl_log_gl_commands(true);
      ngl_LogStream(ostr);
    }

  if(!m_maker->m_log_alloc_commands.m_value.empty())
    {
      std::ostream *ostr;
      if(m_maker->m_log_gl_commands.m_value=="stderr")
	{
	  ostr=&std::cerr;
	}
      else if(m_maker->m_log_gl_commands.m_value=="stdout")
	{
	  ostr=&std::cout;
	} 
      else
	{
          if(m_maker->m_log_alloc_commands.m_value!=m_maker->m_log_gl_commands.m_value)
            {
              m_alloc_log=WRATHNew std::ofstream(m_maker->m_log_gl_commands.m_value.c_str());
              ostr=m_alloc_log;
            }
          else
            {
              ostr=m_gl_log;
            }
	}
      WRATHMemory::set_new_log(ostr);
    }

  #ifdef glBindVertexArray
  {
    if(ngl_functionExists(glBindVertexArray))
      {
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
      }
  }
  #endif

  if(m_maker->m_print_gl_info.m_value)
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

  m_d=m_maker->make_demo();
}

void
DemoWidget::
paintGL(void)
{
  if(m_d!=NULL and !m_end_demo_flag)
    {
      m_d->paint();
    }
}

///////////////////////////
// DemoKernel methods 
void
DemoKernel::
end_demo(void)
{
  if(m_q!=NULL and !m_q->m_w->m_end_demo_flag)
    {
      m_q->m_w->releaseMouse();
      m_q->m_w->releaseKeyboard();
      m_q->m_w->m_end_demo_flag=true;
      m_q->m_w->m_connect.disconnect();
      m_q->m_w->deleteLater();
    }
}

bool
DemoKernel::
demo_ended(void)
{
  return m_q->m_w->m_end_demo_flag;
}


void
DemoKernel::
update_widget(void)
{
  if(!demo_ended())
    {
      m_q->m_w->update();
    }
}


ivec2
DemoKernel::
size(void)
{
  return ivec2(m_q->m_w->width(), m_q->m_w->height());
}

int
DemoKernel::
width(void)
{
  return m_q->m_w->width();
}

int
DemoKernel::
height(void)
{
  return m_q->m_w->height();
}

void
DemoKernel::
titlebar(const std::string &title)
{
  m_q->m_w->setWindowTitle(QString(title.c_str()));
}

void //true=grab
DemoKernel::
grab_mouse(bool v)
{
  if(v)
    {
      m_q->m_w->grabMouse();
    }
  else
    {
      m_q->m_w->releaseMouse();
    }
}

void //true=grab
DemoKernel::
grab_keyboard(bool v)
{
  if(v)
    {
      m_q->m_w->grabKeyboard();
    }
  else
    {
      m_q->m_w->releaseKeyboard();
    }
}

void
DemoKernel::
enable_key_repeat(bool v)
{
  m_q->m_w->m_ep->enable_key_repeat(v);
}

void
DemoKernel::
enable_text_event(bool v)
{
  m_q->m_w->m_ep->enable_text_mode(v);
}

///////////////////////////
// DemoKernelMaker methods
int
DemoKernelMaker::
main(int argc, char **argv)
{

  if(argc==2 and std::string(argv[1])==std::string("-help"))
    {
      std::cout << "\n\nUsage: " << argv[0];
      print_help(std::cout);
      print_detailed_help(std::cout);

      std::cout << "\nDon't foget Qt's -geometry XxY+A+B to set "
                << "the window size to XxY and position to (A,B).\n";
      return 0;
    }


 
  //create widget and application:
  QApplication qapp(argc, argv);

  std::cout << "\n\nRunning: \"";
  for(int i=0;i<argc;++i)
    {
      std::cout << argv[i] << " ";
    }

  parse_command_line(argc, argv);
  std::cout << "\n\n" << std::flush;

  #ifdef HARMATTAN
  {
    resource_set()=new ResourcePolicy::ResourceSet("player");
    ResourcePolicy::ScaleButtonResource *r;
    
    r=new ResourcePolicy::ScaleButtonResource;
    r->setOptional(false);
    resource_set()->addResourceObject(r);
    resource_set()->update();
    resource_set()->acquire();
  }
  #endif
  
  DemoWidget *w;

  w=new DemoWidget(this);
  WRATHassert(w==m_w);
  WRATHunused(w);

  int return_value;
  return_value=qapp.exec();

  
  
  #ifdef HARMATTAN
  {
    resource_set()->release();
    delete resource_set();
    resource_set()=NULL;
  }
  #endif

  return return_value;
}
