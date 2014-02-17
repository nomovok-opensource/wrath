/*! 
 * \file WRATHColumnFormatter.hpp
 * \brief file WRATHColumnFormatter.hpp
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




#ifndef WRATH_HEADER_COLUMN_FORMATTER_HPP_
#define WRATH_HEADER_COLUMN_FORMATTER_HPP_

#include "WRATHConfig.hpp"
#include <locale>
#include "WRATHFormatter.hpp"

/*! \addtogroup Text
 * @{
 */

/*!\class WRATHColumnFormatter
  A WRATHColumnFormatter formats a stream of text
  to fit within a set of horizontal lines. 
  The formatting is specified by two sets of
  values, a left allowed and a right allowed,
  each a list of y-ranges together with a horizontal
  value.
 */
class WRATHColumnFormatter:public WRATHFormatter
{
public:
  /*!\class Constraint
    A Constraint names when a line 
    constraint begins and its value.
   */
  class Constraint
  {
  public:
    /*!\fn Constraint
      Ctor, initializes
      - \ref m_constraint as (false, 0.0)
      - \ref m_begin as 0.0
     */
    Constraint(void):
      m_constraint(false, 0.0f),
      m_begin(0.0f)
    {}

    /*!\fn Constraint& constraint
      Sets that this Contraint specifies
      a constraint, see \ref m_constraint. Default
      is that this Contraint does not specify
      a constraint.
      \param v new value of \ref m_constraint.
     */
    Constraint&
    constraint(float v)
    {
      m_constraint.second=v;
      m_constraint.first=true;
      return *this;
    }

    /*!\fn Constraint& begin
      Specifies when this Contraint takes
      effect, see \ref m_begin
      \param v new value of \ref m_begin.
     */
    Constraint&
    begin(float v)
    {
      m_begin=v;
      return *this;
    }

    /*!\fn Constraint& unconstrain
      Sets that this Contraint does not specify
      a constraint, see \ref m_constraint. Default
      is that this Contraint does not specify
      a constraint.
     */
    Constraint&
    unconstrain(void)
    {
      m_constraint.first=false;
      return *this;
    }

    /*!\var m_constraint 
      Specifies if there is a constraint 
      and if so where (default value
      is to have no constraint) as follows:
      .first: if true then there is a constraint.
      .second: if .first is true, then specifies the
               value of the constraint, value is in
               _pixels_.
      For example for text whose character text
      advance is horizontal (i.e. for example most
      European languages), the .second field
      names the horizontal value of the constraint.
     */
    std::pair<bool, float> m_constraint;

    /*!\var m_begin
      Specifies when this Constraint takes
      affect. For example for text whose character 
      text advance is horizontal (i.e. for example 
      most European languages), this field names  
      the y-coordinate when this constraint takes
      affect. Value is in _pixels_.
     */
    float m_begin;
  };

  /*!\class LayoutSpecification
    A LayoutSpecification specifies  
    how text should be layed out.
    Such specification includes:
    - Pen advance mode
    - Alignment
    - Constraints
   */
  class LayoutSpecification
  {
  public:
    /*!\fn LayoutSpecification(const vec2&, enum screen_orientation_type)
      Ctor. Initialize the LayoutSpecification 
      to not have any constraints and 
      to set the start pen position with a stated
      screen orientation.

      \param pos start position of pen, y-coordiante gives the
                 baseline of the text (for horizontally
                 oriented text).
      \param screen_orient screen orientation to use
                           for layout.
     */
    explicit
    LayoutSpecification(const vec2 &pos=vec2(0.0f,0.0f),
                        enum screen_orientation_type screen_orient=y_increases_downward):
      m_screen_orientation(screen_orient),
      m_text_orientation(horizontal_orientation),
      m_alignment(align_text_begin),
      m_pen_advance(increase_coordinate, increase_coordinate),
      m_start_position(pos),
      m_line_spacing(1.0f),
      m_eat_white_spaces(false),
      m_add_leading_eol(true),
      m_break_words(false),
      m_ignore_control_characters(false),
      m_word_space_on_line_begin(false),
      m_empty_glyph_word_break(true)
    {}

    /*!\fn LayoutSpecification(enum screen_orientation_type)
      Ctor. Initialize the LayoutSpecification 
      to not not have any constraints and 
      to have that the starting position
      of pen is set at (0,0).
      \param screen_orient screen orientation to use
                           for layout.
     */
    explicit
    LayoutSpecification(enum screen_orientation_type screen_orient):
      m_screen_orientation(screen_orient),
      m_text_orientation(horizontal_orientation),
      m_alignment(align_text_begin),
      m_pen_advance(increase_coordinate, increase_coordinate),
      m_start_position(0.0f, 0.0f),
      m_line_spacing(1.0f),
      m_eat_white_spaces(false),
      m_add_leading_eol(true),
      m_break_words(false),
      m_ignore_control_characters(false),
      m_word_space_on_line_begin(false),
      m_empty_glyph_word_break(true)
    {}

    /*!\var m_screen_orientation
      see \ref screen_orientation().
     */
    enum screen_orientation_type m_screen_orientation;

    /*!\var m_text_orientation
      see \ref text_orientation().
     */
    enum text_orientation_type m_text_orientation;

    /*!\var m_alignment
      see \ref alignment().
     */
    enum alignment_type m_alignment;

    /*!\var m_pen_advance
      see \ref horizontal_pen_advance() 
      and vertical_pen_advance().
      m_pen_advance.x() gives the pen
      advance mode in the horizontal
      and m_pen_advance.y() gives
      the pen advance mode in the 
      vertical.
     */
    vecN<enum pen_advance_type, 2> m_pen_advance;
    
    /*!\var m_start_position
      Specifies the starting position of the pen.
      Default value is (0,0).
     */
    vec2 m_start_position;

    /*!\var m_line_spacing
      see \ref line_spacing(float).
     */
    float m_line_spacing;

    /*!\var m_begin_line_constraints
      The list of constraints of this LayoutSpecification.
      These give the contraints of where the text begin
      writing on a line.
     */
    std::vector<Constraint> m_begin_line_constraints;

    /*!\var m_end_line_constraints
      The list of constraints of this LayoutSpecification.
      These give the contraints of where the text ends
      writing on a line.
     */
    std::vector<Constraint> m_end_line_constraints;

    /*!\var m_eat_white_spaces
      see \ref eat_white_spaces().
     */
    bool m_eat_white_spaces;

    /*!\var m_add_leading_eol
      see \ref add_leading_eol().
     */
    bool m_add_leading_eol;

    /*!\var m_break_words
      see \ref break_words(bool).
     */
    bool m_break_words;

    /*!\var m_ignore_control_characters
      see \ref ignore_control_characters(bool)
     */
    bool m_ignore_control_characters;

    /*!\var m_word_space_on_line_begin
      see \ref word_space_on_line_begin(bool)
     */
    bool m_word_space_on_line_begin;

    /*!\var m_empty_glyph_word_break
      If m_empty_glyph_word_break is true, WRATHColumnFormatter
      will be able to break at glyph whenever the glyph
      has 0 drawn width or 0 drawn height, for example
      space is that. Default value is true.
     */
    bool m_empty_glyph_word_break;

    /*!\var m_word_breakers
      Set of characters that signify that a new line may start
      for constrained formating. WRATHColumnFormatter
      will allow for a new line if the character
      code appears in \ref m_word_breakers
      or if \ref m_empty_glyph_word_break is true
      and the glyph for the character has 0 drawn width
      or 0 drawn height. Default value is empty set.
     */
    std::set<WRATHTextData::character> m_word_breakers;

    /*!\fn LayoutSpecification& add_word_breaker
      Add a value to \ref m_word_breakers
      \param ch value to add to \ref m_word_breakers
     */
    LayoutSpecification&
    add_word_breaker(WRATHTextData::character ch)
    {
      m_word_breakers.insert(ch);
      return *this;
    }

    /*!\fn LayoutSpecification& remove_word_breaker
      Remove a value from \ref m_word_breakers
      \param ch value to remove from \ref m_word_breakers
     */
    LayoutSpecification&
    remove_word_breaker(WRATHTextData::character ch)
    {
      m_word_breakers.erase(ch);
      return *this;
    }

    /*!\fn LayoutSpecification& word_space_on_line_begin
      If true and if the first character
      in a line is a white space, adds
      the pen position the current value
      of word spacing (see WRATHText::word_spacing)
      Default value is false.
      \param v value to use
     */
    LayoutSpecification&
    word_space_on_line_begin(bool v)
    {
      m_word_space_on_line_begin=v;
      return *this;
    }

    /*!\fn LayoutSpecification& ignore_control_characters
      Sets the formatter to ignore \\n and \\t
      control characters when formatting.
      Default value is false.
      \param v value to use
     */
    LayoutSpecification&
    ignore_control_characters(bool v)
    {
      m_ignore_control_characters=v;
      return *this;
    }

    /*!\fn LayoutSpecification& break_words
      Set to break words on line breaks,
      i.e. if true then words can be broken
      at a line break, if false a word will
      never be broken to fit within consraints.
      Default value is false.
      \param v value to which to set \ref m_break_words
     */
    LayoutSpecification&
    break_words(bool v)
    {
      m_break_words=v;
      return *this;
    }

    /*!\fn LayoutSpecification& eat_white_spaces
      A WRATHColumnFormatter will induce new lines,
      as such if a new line is added just before
      a white space, it is likely that one wishes
      for that white space to be ignored. When
      \ref m_eat_white_spaces is true, then white spaces
      that occur before any non-white spaces on
      the start of a line are ignored. Default
      value is false.
      \param v value to which to set \ref m_eat_white_spaces    
     */
    LayoutSpecification&
    eat_white_spaces(bool v)
    {
      m_eat_white_spaces=v;
      return *this;
    }

    /*!\fn LayoutSpecification& add_leading_eol
      If true, add a leading EOL before 
      any text is formatted. This has the
      effect for text with a horizontal
      alignment and a vertical advance
      down the screen that the _TOP_ of
      the first line is at the y-coordinate
      of \ref m_start_position. Default value
      is true. 
      \param v value to which to set \ref m_add_leading_eol   
     */
    LayoutSpecification&
    add_leading_eol(bool v)
    {
      m_add_leading_eol=v;
      return *this;
    }

    /*!\fn LayoutSpecification& add_begin_line_constraint
      Add a constraint for line beginnings,
      see \ref m_begin_line_constraints.
      \param C contraint to add
    */
    LayoutSpecification&
    add_begin_line_constraint(const Constraint &C)
    {
      m_begin_line_constraints.push_back(C);
      return *this;
    }

    /*!\fn LayoutSpecification& clear_begin_line_constraints
      Clear constraints for line beginnings,
      see \ref m_begin_line_constraints.
    */
    LayoutSpecification&
    clear_begin_line_constraints(void)
    {
      m_begin_line_constraints.clear();
      return *this;
    }

    /*!\fn LayoutSpecification& add_end_line_constraint
      Add a constraint for line endings, 
      see \ref m_end_line_constraints.
      \param C contraint to add
    */
    LayoutSpecification&
    add_end_line_constraint(const Constraint &C)
    {
      m_end_line_constraints.push_back(C);
      return *this;
    }

    /*!\fn clear_end_line_constraints
      Clear line ending constraints, 
      see \ref m_end_line_constraints.
    */
    LayoutSpecification&
    clear_end_line_constraints(void)
    {
      m_end_line_constraints.clear();
      return *this;
    }

    /*!\fn LayoutSpecification& line_spacing
      Sets the amount the pen advances in the  
      direction indicated by m_text_orientation
      at the start of each line. Default value
      is 1.0.
      \param v value to which to set \ref m_line_spacing  
     */
    LayoutSpecification&
    line_spacing(float v)
    {
      m_line_spacing=v;
      return *this;
    }

    /*!\fn LayoutSpecification& screen_orientation
      Set the screen orientation, 
      default value is set at
      construction.
      \param v new value for screen orientation.
     */
    LayoutSpecification&
    screen_orientation(enum screen_orientation_type v)
    {
      m_screen_orientation=v;
      return *this;
    }

    /*!\fn LayoutSpecification& text_orientation
      Set the text_orientation, 
      default value is horizontal_orientation.
      \param v new value for \ref m_text_orientation
     */
    LayoutSpecification&
    text_orientation(enum text_orientation_type v)
    {
      m_text_orientation=v;
      return *this;
    }

    /*!\fn LayoutSpecification& alignment
      Set the horizontal_alignment, 
      default value is WRATHFormatter::align_text_begin.
      \param v new value for \ref m_alignment
     */
    LayoutSpecification&
    alignment(enum alignment_type v)
    {
      m_alignment=v;
      return *this;
    }

    /*!\fn LayoutSpecification& horizontal_pen_advance
      Set the pen advance in the horizontal, 
      default value is increase_coordinate.
      \param v new value for x-coordinate of
      \ref m_pen_advance
     */
    LayoutSpecification&
    horizontal_pen_advance(enum pen_advance_type v)
    {
      m_pen_advance.x()=v;
      return *this;
    }
    
    /*!\fn LayoutSpecification& vertical_pen_advance
      Set the pen advance in the vertical, 
      default value is increase_coordinate.
      \param v new value for y-coordinate of
      \ref m_pen_advance 
     */
    LayoutSpecification&
    vertical_pen_advance(enum pen_advance_type v)
    {
      m_pen_advance.y()=v;
      return *this;
    }

    /*!\fn LayoutSpecification& start_position(const vec2&)
      Set the value of the pen starting position,
      \ref m_start_position
      \param c _starting_ position of pen
     */
    LayoutSpecification&
    start_position(const vec2 &c) 
    {
      m_start_position=c;
      return *this;
    }

    /*!\fn LayoutSpecification& start_position(float, float)
      Equivalent to \code start_position(vec2(x,y)) \endcode
      see \ref start_position(const vec2&).
      \param x x-coordinate to use
      \param y y-coordinate to use
     */
    LayoutSpecification&
    start_position(float x, float y)
    {
      return start_position(vec2(x,y));
    }

  };

  /*!\fn WRATHColumnFormatter
    Ctor. Create a WRATHColumnFormatter using layout
    specifications of a LayoutSpecification.
    \param L parameters that govern how the
             created WRATHColumnFormatter
             will perform text layout.
   */
  explicit
  WRATHColumnFormatter(const LayoutSpecification &L);

  ~WRATHColumnFormatter();

  virtual
  pen_position_return_type 
  format_text(const WRATHTextData &raw_data,
              const WRATHStateStream &state_stream,
              std::vector<WRATHFormatter::glyph_instance> &out_data,
              std::vector<std::pair<int, LineData> > &out_eols);

 

  /*!\fn enum screen_orientation_type screen_orientation(void)
    Virtual method that returns the screen orientation.
   */
  virtual
  enum screen_orientation_type
  screen_orientation(void);

private:

  enum
    {
      record_eol=1,
      advance_pen_to_next_line=2
    };

  void
  reset(void);

  
  void
  add_new_line(std::vector<WRATHFormatter::glyph_instance> &out_data,
               std::vector<std::pair<int, LineData> > &out_eols,
               int flags);

  bool
  require_new_line(void);

  bool
  constraint_in_affect(float begin);

  void
  increment_contraints(void);

  void
  increment_contraint(std::vector<Constraint>::iterator &iter,
                      std::pair<bool, float> &update_value,
                      std::vector<Constraint>::iterator end);

 

  //state:
  vec2 m_pen_position;
  float m_current_max_descend, m_current_max_ascend;
  float m_newline_space, m_tab_width, m_space_width;
  WRATHTextureFont *m_font;
  float m_font_scale, m_last_character_advance;
  vec2 m_base_line_offset;
  vec2 m_scaled_factor;
  bool m_line_empty, m_added_line;
  int m_last_eol_idx;
  std::pair<WRATHTextureFont*, WRATHTextureFont::glyph_index_type> m_previous_glyph;
  std::vector<Constraint>::iterator m_begin_line_constraint_iter;
  std::vector<Constraint>::iterator m_end_line_constraint_iter;

  std::pair<bool, float> m_begin_line_current_value;
  std::pair<bool, float> m_end_line_current_value;

  //formatting specification:
  LayoutSpecification m_layout;
  int m_advance_character_index, m_advance_line_index;
  vec2 m_factor;  
  std::locale m_locale;
};

/*!\fn std::ostream& operator<<(std::ostream&, const WRATHColumnFormatter::Constraint&)
  Overloaded operator<< to print the values of a WRATHColumnFormatter::Constraint.
  \param ostr std::ostream stream to which to print
  \param obj WRATHColumnFormatter::Constraint to print
*/
std::ostream&
operator<<(std::ostream &ostr, 
           const WRATHColumnFormatter::Constraint &obj);
/*! @} */

#endif
