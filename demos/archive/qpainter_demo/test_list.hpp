/*! 
 * \file test_list.hpp
 * \brief file test_list.hpp
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


#ifndef MAIN_WIDGET_HPP
#define MAIN_WIDGET_HPP

//Qt timers ony "run" Qt slots, which must be member
//function of QObject derived types.
//we cannot place the class definition in a source
//because moc runs through headers.
#include <vector>
#include <QTime>
#include "transformation_node.hpp"
#include "image_item.hpp"
#include "draw_item.hpp"
#include "text_item.hpp"
#include "generic_command_line.hpp"

class item_type
{
public:
  TextItem *m_text;
  ImageItem *m_image;
  TransformationNode *m_rotation_node;
  TransformationNode *m_translation_node;
  float m_v_x, m_v_y, m_omega;

  item_type(void):
    m_text(NULL),
    m_image(NULL),
    m_rotation_node(NULL),
    m_translation_node(NULL),
    m_v_x(0.0f),
    m_v_y(0.0f),
    m_omega(0.0f)
  {}
};

class main_widget_command_line:public command_line_register
{
public:
  command_line_argument_value<bool> m_fullscreen;
  command_line_argument_value<int> m_window_width, m_window_height;
  
  command_line_argument_value<int> m_animation_timer;
  command_line_argument_value<std::string> m_image_filename;
  command_line_argument_value<std::string> m_image_filename2;
  command_line_argument_value<std::string> m_ttf_filename;
  command_line_argument_value<int> m_ttf_size;
  command_line_argument_value<int> m_time_ms;
  command_line_argument_value<bool> m_fast_quit;
  command_line_argument_value<int> m_count;
  command_line_argument_value<bool> m_draw_text, m_draw_images;
  command_line_argument_value<float> m_item_font_size;
  command_line_argument_value<std::string> m_item_text_prefix1, m_item_text_prefix2;
  command_line_argument_value<int> m_time_to_change_prefix_text;
  command_line_argument_value<bool> m_show_fps_on_items;
  command_line_argument_value<float> m_item_size_x, m_item_size_y;
  command_line_argument_value<int> m_number_per_row;
  command_line_argument_value<float> m_velocity_x, m_velocity_y, m_velocity_rotation;
  command_line_argument_value<std::string> m_text;
  command_line_argument_value<bool> m_rotate;
  command_line_argument_value<int> m_text_red, m_text_blue, m_text_green;
  command_line_argument_value<bool> m_time_limit_off;
  command_line_argument_value<float> m_max_zoom_factor;
  command_line_argument_value<float> m_min_zoom_factor;
  command_line_argument_value<std::string> m_image_dir;

  main_widget_command_line(void):
    command_line_register(),
    m_fullscreen(false, "fullscreen", "Fullscreen", *this),
    m_window_width(400, "width", "Window width", *this),
    m_window_height(400, "height", "Window Height", *this),
    m_animation_timer(16, "animation_tick", "Number of ms between timer going off", *this),
    m_image_filename("./images/image.png", "image", "Image filename to use for texture", *this),
    m_image_filename2("./images/image.png", "image2", "Image filename to use for texture2", *this),
    m_ttf_filename("ttf/FreeSerif.ttf", "font", "True Type Font to use", *this),
    m_ttf_size(64, "font_size", "True Type Font Size", *this),
    m_time_ms(500, "time", "time in ms to run test", *this),
    m_fast_quit(false, "quit", "quit after one draw loop", *this),
    m_count(10, "count", "Number of elements to draw", *this),
    m_draw_text(true, "draw_text", "Draw text on each item", *this),
    m_draw_images(true, "draw_image", "Draw images on each item", *this),
    m_item_font_size(24, "item_font_size", "Font size to use for each item", *this),
    m_item_text_prefix1("PICTURE #", "prefix_text1", "prefix1 draw ID# of image elements", *this),
    m_item_text_prefix2("picture #", "prefix_text2", "prefix2 draw ID# of image elements", *this),
    m_time_to_change_prefix_text(200, "text_change_time", "Time to change text drawn on extra items", *this),
    m_show_fps_on_items(true, "extra_text_fps", "Show FPS on extra text items", *this),
    m_item_size_x(100, "item_size_x", "horizontal size in pixels of each item", *this),
    m_item_size_y(100, "item_size_y", "vertical size in pixels of each item", *this),
    m_number_per_row(20, "per_row_count", "Number of items per row", *this),
    m_velocity_x(1.0f, "v_x", "velocity in x direction measured in pixels per second", *this),
    m_velocity_y(1.0f, "v_y", "velocity in y direction measured in pixels per second", *this),
    m_velocity_rotation(10*M_PI, "omega", "angular velocity of items measured in radians/second", *this),
    m_text("QPainter Demo", 
           "text", "specify text test string", *this),
    m_rotate(false, "rotatexy", "exchange x with y coordinate", *this),
    m_text_red(0x00, "text_red", "red value for dynamic text values, [0-255]", *this), 
    m_text_blue(0xFF, "text_blue", "blue value for dynamic text values, [0-255]", *this), 
    m_text_green(0xFF, "text_green", "green value for dynamic text values, [0-255]", *this), 
    m_time_limit_off(false, "ignore_time", 
                     "If true, then applicaion does not automatically quit after time ms", *this),
    m_max_zoom_factor(4.0f, "max_dyn_zoom", "Maximum zoom factor for dynamic scaled text", *this),
    m_min_zoom_factor(1.0f, "min_dyn_zoom", "Minimum zoom factor for dynamic scaled text", *this),
    m_image_dir("", "image_dir", 
                "If non-empty string, use all images from the specified image directory", *this)
  {}
};

class TestList:public DrawList
{
public:
  
  explicit
  TestList(const main_widget_command_line &cmd_line, QObject *parent);
  ~TestList();
 
  void
  update_data(const QSize &window_size);

  void
  resize(QSize window_size);

  void
  setPaused(bool p);

  bool
  paused(void);

  void
  togglePaused(void);

  bool
  timeToWRATHDelete(void);
    
private:

  std::vector<item_type> m_items;
  TextItem *m_scaling_text, *m_fps_text;
  TransformationNode *m_scaling_node, *m_root;
  TransformationNode *m_draw_at_bottom, *m_draw_at_top;
  TransformationNode *m_actual_root;

  float m_avg_size, m_apt_wave_size;
  int m_max_time;
  bool m_ignore_time;

  QTime m_time_object;
  int m_last_running_time, m_running_time;
  int m_start_record_time, m_simulation_time;
  bool m_paused;
  int m_frame_draw_count; 

  const main_widget_command_line &m_cmd_line;

  bool m_stuff_ready;

  void
  create_stuff(const QSize &pwindow_size);
  

  
};

#endif
