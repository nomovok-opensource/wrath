/*! 
 * \file WRATHFillRule.hpp
 * \brief file WRATHFillRule.hpp
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



/*! \addtogroup Shape
 * @{
 */


#ifndef __WRATH_FILL_RULE_HPP__
#define __WRATH_FILL_RULE_HPP__

#include "WRATHConfig.hpp"
#include "WRATHReferenceCountedObject.hpp"
#include "WRATHShapeAttributePacker.hpp"

/*!\namespace WRATHFillRule
  Namespace to specify fill rules for filling
  WRATHShape objects.
 */
namespace WRATHFillRule
{
  /*!\typedef fill_rule_function
    Function type to specify a fill rule.
    Input argument is the winding number
    of a component together with a pointer
    to custom data. Output is a boolean
    to indicate weather or not to fill,
    i.e. return true if to fill, false if 
    not.
    \param winding_number winding number of the region
    \param data user defined data
   */
  typedef bool (*fill_rule_function)(int winding_number, void *data);


  /*!\class fill_rule
    Class to specify a fill rule, just a nice
    wrapper over std::pair<fill_rule_function, void*>.
    Constructable from a \ref fill_rule_function
    where the .second (the data) is made NULL then.
   */
  class fill_rule:public std::pair<fill_rule_function, void*>
  {
  public:

    /*!\fn fill_rule
      Ctor.
      \param fn function to compute the fill
      \param data user defined data to pass to the function
     */
    fill_rule(fill_rule_function fn=NULL, void *data=NULL):
      std::pair<fill_rule_function, void*>(fn, data)
    {}

    /*!\fn fill_rule_function& function(void)
      Returns a reference to the \ref fill_rule_function 
      of the \ref fill_rule, i.e. 
      \code
      first
      \endcode
     */
    fill_rule_function&
    function(void)
    {
      return first;
    }

    /*!\fn fill_rule_function function(void) const
      Returns the \ref fill_rule_function 
      of the \ref fill_rule, i.e. 
      \code
      first
      \endcode
     */
    fill_rule_function
    function(void) const
    {
      return first;
    }

    /*!\fn void*& data(void) 
      Returns a reference to the data 
      of the \ref fill_rule, i.e. 
      \code
      second
      \endcode
     */
    void*&
    data(void)
    {
      return second;
    }

    /*!\fn void* data(void) const
      Returns the data 
      of the \ref fill_rule, i.e. 
      \code
      second
      \endcode
     */
    void*
    data(void) const
    {
      return second;
    }

    /*!\fn bool valid(void) const
      Provided as a readability conveniace,
      equivalent to
      \code
      function()!=NULL
      \endcode
     */
    bool
    valid(void) const
    {
      return function()!=NULL;
    }

    /*!\fn bool operator()(int) const
      Provided as a readability conveniace,
      equivalent to
      \code
      function()!=NULL and function()(winding_number, data())
      \endcode
      \param winding_number winding number of region
     */ 
    bool
    operator()(int winding_number) const
    {
      return first!=NULL and first(winding_number, second);
    }
  };


  /*!\fn bool non_zero_rule(int, void*)
    Function with same signature as the function
    type \ref fill_rule_function. Implements the non-zero
    winding number, i.e. returns true if and 
    only if the winding number is non-zero.
    \param winding_number winding number of component
   */
  inline
  bool 
  non_zero_rule(int winding_number, void*) { return winding_number!=0; }

  /*!\fn bool odd_even_rule(int, void*)
    Implements the odd-even fill rule, i.e. to fill
    if and only if the winding number is odd. The odd-even
    fill rule is equivalent to saying a point is "inside"
    if and only if given a random ray, originating at the 
    point, it intersects the boundary an odd number of 
    times. In this situation, by random, we disclude 
    those rays that do not intersect transversally with
    the boundary. (The more formal precise definition of 
    random is that outside a set of measure 0 of rays).
    \param winding_number winding number of component
  */
  inline
  bool
  odd_even_rule(int winding_number, void*) 
  {
    return (std::abs(winding_number)&1)==1;
  }
  
  /*!\fn bool winding_abs_greater_equal_2(int, void*)
    Implement the fill rule where to fill
    if the winding number is greater than
    or equal to 2.
    \param winding_number winding number of component
   */
  inline
  bool
  winding_abs_greater_equal_2(int winding_number, void*)
  {
    return std::abs(winding_number)>=2;
  }

  /*!\fn bool winding_positive(int, void*)
    Implement the winding positive fill rule,
    i.e. fill if the winding number is positive.
    \param winding_number winding number of component
   */
  inline
  bool
  winding_positive(int winding_number, void*)
  {
    return winding_number>0;
  }

  /*!\fn bool winding_negative(int, void*) 
    Implement the winding negative fill rule,
    i.e. fill if the winding number is positive.
    \param winding_number winding number of component
   */
  inline
  bool
  winding_negative(int winding_number, void*)
  {
    return winding_number<0;
  }

};



#endif

/*! @} */

