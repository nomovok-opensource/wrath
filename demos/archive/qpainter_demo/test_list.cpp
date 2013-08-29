/*! 
 * \file test_list.cpp
 * \brief file test_list.cpp
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


#include <QFontDatabase>
#include <iostream>
#include <QList>
#include "test_list.hpp"
#include <dirent.h>
#include <cstring>

bool
recursrive_load_images(const std::string &full_path, 
                       QList<QImage> &images)
{
  DIR *ptr;
      
  ptr=::opendir(full_path.c_str());
  if(ptr==NULL)
    {
      return false;
    }
  else
    {
      for(struct dirent *currentEntry=::readdir(ptr);
          currentEntry!=NULL;currentEntry=::readdir(ptr))
        {
          if(!std::strcmp(currentEntry->d_name,".") or !std::strcmp(currentEntry->d_name,".."))
             continue;

          if(!recursrive_load_images(full_path+currentEntry->d_name+"/", images))
            {
              std::string filename(full_path+std::string(currentEntry->d_name));
              images.push_back(QImage(filename.c_str()));
            }
        }
      ::closedir(ptr);
      return true;
    }
}


TestList::
TestList(const main_widget_command_line &pcmd_line, QObject *parent):
  DrawList(parent),
  m_scaling_text(NULL),
  m_fps_text(NULL),
  m_scaling_node(NULL),
  m_running_time(0),
  m_simulation_time(0),
  m_paused(false),
  m_frame_draw_count(0),
  m_cmd_line(pcmd_line),
  m_stuff_ready(false)
{
  m_actual_root=m_root=new TransformationNode();
  m_time_object.start();
  
  if(m_cmd_line.m_rotate.m_value)
    {
      m_root=new TransformationNode(m_actual_root);
    }

  m_avg_size=(m_cmd_line.m_max_zoom_factor.m_value+m_cmd_line.m_min_zoom_factor.m_value)*0.5f;
  m_apt_wave_size=(m_cmd_line.m_max_zoom_factor.m_value-m_cmd_line.m_min_zoom_factor.m_value)*0.5f;

  m_ignore_time=m_cmd_line.m_time_limit_off.m_value;
  m_max_time=m_cmd_line.m_time_ms.m_value;


}

TestList::
~TestList()
{
  int end_record_time;

  end_record_time=m_time_object.elapsed();
  std::cout << "\n" 
            << m_frame_draw_count << " frames in "
            << (end_record_time-m_start_record_time)
            << " ms\nN=" 
            << m_items.size() << " ["
            << static_cast<float>(end_record_time-m_start_record_time)/static_cast<float>(m_frame_draw_count)
            << " ms per frame, "
            << 1000.0f*static_cast<float>(m_frame_draw_count)/static_cast<float>(end_record_time-m_start_record_time)
            << " FPS]\n";

  for(int i=0, end_i=m_items.size(); i<end_i; ++i)
    {
      delete m_items[i].m_text;
      delete m_items[i].m_image;
    }
  delete m_actual_root;
}

void
TestList::
create_stuff(const QSize &window_size)
{
  m_stuff_ready=true;


  //load font
  //TODO:
  int fontID;
  fontID=QFontDatabase::addApplicationFont(m_cmd_line.m_ttf_filename.m_value.c_str());


  QFontDatabase database;
  QString family("Helvetica");
  QString style("Normal");

  if(fontID!=-1)
    {
      QStringList font_families;
      font_families=QFontDatabase::applicationFontFamilies(fontID);
#if 0
      

      for(QStringList::const_iterator iter=font_families.begin(),
            end=font_families.end(); iter!=end; ++iter)
        {
          QStringList font_styles;

          font_styles=database.styles(*iter);
          std::cout << "\nFamily: " << iter->toStdString();

          for(QStringList::const_iterator fiter=font_styles.begin(),
                fend=font_styles.end(); fiter!=fend; ++fiter)
            {
              std::cout << "\n\t" << fiter->toStdString();
            }
        }
#endif

      if(font_families.begin()!=font_families.end())
        {
          family=*font_families.begin();
        }
    }


  QFont pfont(database.font(family, style, m_cmd_line.m_ttf_size.m_value));
  
  //load images
  QList<QImage> images;
  images.push_back(QImage(m_cmd_line.m_image_filename.m_value.c_str()));
  if(m_cmd_line.m_image_filename.m_value!=m_cmd_line.m_image_filename2.m_value)
    {
      images.push_back(QImage(m_cmd_line.m_image_filename2.m_value.c_str()));
    }
  else
    {
      images.push_back(images[0]);
    }

  if(m_cmd_line.m_draw_images.m_value and !m_cmd_line.m_image_dir.m_value.empty())
    {
      std::string image_dir(m_cmd_line.m_image_dir.m_value);

      if(*image_dir.rbegin()!='/')
        {
          image_dir.push_back('/');
        }
      recursrive_load_images(image_dir, images);
    }

  //create items:
  m_items.resize( std::max(0, m_cmd_line.m_count.m_value));
  
  float delta_x, delta_y;
  int row_count;

  row_count=std::max( size_t(1), m_items.size()/m_cmd_line.m_number_per_row.m_value);
  delta_x=static_cast<float>(window_size.width())/static_cast<float>(m_cmd_line.m_number_per_row.m_value);
  delta_y=static_cast<float>(window_size.height())/static_cast<float>(row_count);
  
  

  for(int i=0, row=1, col=0; i<m_cmd_line.m_count.m_value; ++i, ++col)
    {
      m_items[i].m_v_x=m_cmd_line.m_velocity_x.m_value*(1.0f+0.5f*cosf(static_cast<float>(i)))*0.0001;
      m_items[i].m_v_y=m_cmd_line.m_velocity_y.m_value*(1.0f+0.5f*sinf(static_cast<float>(i+1)))*0.0001;
      m_items[i].m_omega=m_cmd_line.m_velocity_rotation.m_value*(1.0f+0.5f*sinf(static_cast<float>(i+1)))*0.0001;

     
      m_items[i].m_translation_node=new TransformationNode(m_root);
      m_items[i].m_translation_node->getRefValue().translation(static_cast<qreal>(col)*delta_x + delta_x/2.0,
                                                               static_cast<qreal>(row)*delta_y - delta_y/2.0);

      

      
      if(col>=m_cmd_line.m_number_per_row.m_value)
        {
          col=-1;
          ++row;
        }

      m_items[i].m_rotation_node=new TransformationNode(m_items[i].m_translation_node);
      
      if(m_cmd_line.m_draw_images.m_value)
        {
          
          m_items[i].m_image=new ImageItem( images[i%images.size()],
                                            QRectF(QPointF( -m_cmd_line.m_item_size_x.m_value/2.0f,
                                                            -m_cmd_line.m_item_size_y.m_value/2.0f),
                                                   QPointF( m_cmd_line.m_item_size_x.m_value/2.0f,
                                                            m_cmd_line.m_item_size_y.m_value/2.0f)),
                                            this,
                                            m_items[i].m_rotation_node);

        }

      if(m_cmd_line.m_draw_text.m_value)
        {
          TransformationNode *text_scale;
          float factor;
          std::ostringstream text_body;

          text_body << m_cmd_line.m_item_text_prefix1.m_value << i;

          factor=m_cmd_line.m_item_font_size.m_value/static_cast<float>(m_cmd_line.m_ttf_size.m_value);
          text_scale=new TransformationNode(m_items[i].m_rotation_node);
          text_scale->getRefValue().scale(factor);

          m_items[i].m_text=new TextItem(pfont,
                                         QColor(m_cmd_line.m_text_red.m_value,
                                                m_cmd_line.m_text_green.m_value,
                                                m_cmd_line.m_text_blue.m_value,
                                                0xFF),
                                         text_body.str().c_str(),
                                         this,
                                         text_scale);
        }
    }

  //now also have scaling text at the bottom:
  TransformationNode *nudge;

  m_draw_at_bottom=new TransformationNode(m_root);
  m_draw_at_bottom->getRefValue().translation(0.0f, window_size.height() - m_cmd_line.m_ttf_size.m_value);  

  m_draw_at_top=new TransformationNode(m_root);
  m_draw_at_top->getRefValue().translation(0.0f, m_cmd_line.m_ttf_size.m_value);  



  m_scaling_node=new TransformationNode(m_draw_at_bottom);
  nudge=new TransformationNode(m_scaling_node);
  nudge->getRefValue().translation(-20.0f, -10.0f);
  
      
  m_scaling_text=new TextItem(pfont,
                              QColor(m_cmd_line.m_text_red.m_value,
                                     m_cmd_line.m_text_green.m_value,
                                     m_cmd_line.m_text_blue.m_value,
                                     0xFF),
                              m_cmd_line.m_text.m_value.c_str(),
                              this, nudge);
  
  m_fps_text=new TextItem(pfont,
                          QColor(m_cmd_line.m_text_red.m_value,
                                 m_cmd_line.m_text_green.m_value,
                                 m_cmd_line.m_text_blue.m_value,
                                 0xFF),
                          "",
                          this, m_draw_at_top);
  
    


}

void
TestList::
resize(QSize window_size)
{
  if(!m_stuff_ready)
    {
      return;
    }

  float delta_x, delta_y;
  int row_count;

  if(m_cmd_line.m_rotate.m_value)
    {
      std::swap(window_size.rwidth(), window_size.rheight());
    }

  row_count=std::max( size_t(1), m_items.size()/m_cmd_line.m_number_per_row.m_value);
  delta_x=static_cast<float>(window_size.width())/static_cast<float>(m_cmd_line.m_number_per_row.m_value);
  delta_y=static_cast<float>(window_size.height())/static_cast<float>(row_count);
  
  

  for(int i=0, row=1, col=0; i<m_cmd_line.m_count.m_value; ++i, ++col)
    {
     
     
      m_items[i].m_translation_node->getRefValue().translation(static_cast<qreal>(col)*delta_x + delta_x/2.0,
                                                               static_cast<qreal>(row)*delta_y - delta_y/2.0);
      if(col>=m_cmd_line.m_number_per_row.m_value)
        {
          col=-1;
          ++row;
        }
    }


  m_draw_at_bottom->getRefValue().translation(0.0f, window_size.height() - m_cmd_line.m_ttf_size.m_value);  
  
}

void
TestList::
setPaused(bool p)
{
  m_paused=p;
}

bool
TestList::
paused(void)
{
  return m_paused;
}

void
TestList::
togglePaused(void)
{
  m_paused=!m_paused;
}

bool
TestList::
timeToWRATHDelete(void)
{
  return (m_max_time<m_running_time and !m_ignore_time);
}

void
TestList::
update_data(const QSize &pwindow_size)
{
  int delta_time;
  float fps;

  QSize window_size(pwindow_size);
  if(m_cmd_line.m_rotate.m_value)
    {
      std::swap(window_size.rwidth(), window_size.rheight());
    }

  if(!m_stuff_ready)
    {
      create_stuff(window_size);
    }


  if(m_cmd_line.m_rotate.m_value)
    {
      //adjust m_actual_root for the window_size:      
      m_actual_root->getRefValue().rotation(M_PI/2.0f);
      m_actual_root->getRefValue().translation(pwindow_size.width(),0);
    }

  

  m_last_running_time=m_running_time;
  m_running_time=m_time_object.elapsed();
  delta_time=m_running_time - m_last_running_time;
  ++m_frame_draw_count;

  if(!m_paused)
    {
      m_simulation_time+=delta_time;
    }

  if(m_frame_draw_count<=5)
    {
      m_start_record_time=m_running_time;
    }

  if(delta_time!=0)
    {
      fps=static_cast<int>(1000.0f/static_cast<float>(delta_time));
    }
  else
    {
      fps=1000;
    }

  //update transformations for animation:
  for(int i=0, end_i=m_items.size(); !m_paused and i<end_i; ++i)
    {
      std::complex<float> tr;
      std::complex<float> delta_tr(m_items[i].m_v_x, m_items[i].m_v_y);
      qreal rot;
      
      tr=m_items[i].m_translation_node->getValue().translation();
      delta_tr*=static_cast<float>( delta_time );
      rot=m_items[i].m_omega*static_cast<float>(delta_time);
      
      m_items[i].m_translation_node->getRefValue().translation(delta_tr+tr);
      m_items[i].m_rotation_node->getRefValue().rotateby(rot);
      
      if(tr.real()+delta_tr.real()>window_size.width() and m_items[i].m_v_x>0.0f)
        {
          m_items[i].m_v_x*=-1.0f;
        }
      else if(tr.real()+delta_tr.real()<0.0f and m_items[i].m_v_x<0.0f)
        {
          m_items[i].m_v_x*=-1.0f;
        }
      
      if(tr.imag()+delta_tr.imag()>window_size.height() and m_items[i].m_v_y>0.0f)
        {
          m_items[i].m_v_y*=-1.0f;
        }
      else if(tr.imag()+delta_tr.imag()<0.0f and m_items[i].m_v_y<0.0f)
        {
          m_items[i].m_v_y*=-1.0f;
        }
    }

  std::ostringstream fps_message;

  fps_message << "FPS: " << fps;
  m_fps_text->setText( fps_message.str().c_str());

  m_scaling_node->getRefValue().scale(m_avg_size + m_apt_wave_size*cosf(static_cast<float>(m_simulation_time)/1000.0f));
  
}
