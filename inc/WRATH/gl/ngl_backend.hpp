/*! 
 * \file ngl_backend.hpp
 * \brief file ngl_backend.hpp
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



#ifndef WRATH_NGL_BACKEND_HPP_
#define WRATH_NGL_BACKEND_HPP_

#include "WRATHConfig.hpp"
#include <iostream>


/*! \addtogroup GLUtility
 * @{
 */


/*!\def GL_DEBUG
  if GL_DEBUG is defined, each GL call will be preceded by 
  a callback and postceeded by another call back.
  The precede call back will print and flush the GL command
  (file, line and arguments) to the stream specified
  by ngl_LogStream(std::ostream *str) <i>if</i> GL logging is
  on (see \ref ngl_log_gl_commands(bool)). The postceed
  call back will call glGetError until no errors are returned
  and write to the log stream(see \ref ngl_log_gl_commands(bool)) 
  any error enocountered. If logging is on (see 
  \ref ngl_log_gl_commands(bool)) then regardless of any errors
  a log message will be written indicating the GL call
  succeeded.

  
  This is implemented by creating a macro for each
  GL call.  If GL_DEBUG is not defined, noe of these
  logging and error calls backs are executed.
  The mechanism is implemented by defining a macro
  for each GL function, hence using a GL function
  name as a function pointer will fail to compile
  and likely give an almost impossible to read
  error message.

  To fetch the function pointer of a GL function,
  use the macro <B>ngl_functionPointer</B> together
  with <B>ngl_functionExists</B>. The value
  returned by <B>ngl_functionPointer</B> may or 
  may not point to GL implementation function
  pointer. Indeed, the initial value points to
  function that sets the value. The setting of
  the function pointer is done on the fist call to 
  the GL function or on the first call of 
  <B>ngl_functionExists</B>, which ever is first. 
  Thus to save the function pointer preceed 
  saving the function pointer with a call to 
  <B>ngl_functionExists</B>. The macro 
  <B>ngl_functionExists</B>  maps to a function 
  returning non-zero if and only if the GL
  implementation supports the function.
  An example code of using/saving a GL function
  pointer is as follows:

  \code
  //get a function pointer for a GL function
  //which takes no arguments and returns nothing.
  void (*functionPointer)(void)=NULL;
  if(ngl_functionExists(glSomething)
    {
      functionPointer=ngl_functionPointer(glSomeFunction);
    }
  else
    {
      //ngl_functionPointer(glSomeFunction) is NOT NULL,
      //rather it maps to a no-op function.
      //in this example we leave the value of 
      //functionPointer as NULL to indicate the function
      //is not supported by the GL implementation.
    } 
  \endcode

  Calling a GL function through a function pointer
  will bypass the GL error checking though.

  In addition, the macro <B>ngl_functionPointer</B>
  gives an <i>L</i>-value and can be used to remap
  the GL function to one's own choosing.  Remapping 
  of GL function pointers is supported even when GL_DEBUG
  is not defined. Recall that the initial value of 
  the function pointer does NOT map to the GL function, 
  rather it maps to a function that sets the function pointer
  value to the GL function. For example if one
  wanted to add a custom call back to function glFoo,
  one would do as follows
  \code
  
  // glFoo has fucntion signature void glFoo(GLenum, void*)
  void (*gl_implementation_glFooValue)(GLenum, void*)
  void myglFoo(GLenum v, void *p) 
  {
    //do stuff before the GL function
    do_stuff_before_gl_function(...);

    //call gl function
    gl_implementation_glFooValue(v, p);

    //do stuff after the GL function
    do_stuff_after_gl_function(...);
  }

  .
  .
  .

  if(ngl_functionExists(glFoo))
    {
      //ngl_functionPointer(glFoo) is initialized when
      //the underlying function called by the macro
      //instance ngl_functionExists(glFoo) is called.
      gl_implementation_glFooValue=ngl_functionPointer(glFoo);
      ngl_functionPointer(glFoo)=myglFoo;
    }
  else
    {
      //gl function not supported, you can still do the remap
      //be aware that ngl_functionPointer(glFoo) maps to a no-op 
      //function
      gl_implementation_glFooValue=ngl_functionPointer(glFoo);
      ngl_functionPointer(glFoo)=myglFoo;
    }
  \endcode

  Remapping a function pointer as above will, 
  regarless if GL_DEBUG is defined, will have the affect
  all <B>glFoo</B> calls will be <B>myglFoo</B> calls.
  When GL_DEBUG defined the preceed log call back is 
  called before the function pointer of <B>ngl_functionPointer</B>
  and the postcede error check callback is called afterwards.
 */


/*!\fn std::ostream* ngl_LogStream(void)
  Returns a _pointer_ to the stream
  that to which ngl sends logs. If
  the stream is NULL, then ngl logs
  are "not printed". The default value
  is a pointer to std::cerr, see also 
  \ref GL_DEBUG.
 */
std::ostream*
ngl_LogStream(void);

/*!\fn void ngl_LogStream(std::ostream*)
  Sets the stream that to which ngl 
  sends logs. If the stream is NULL, 
  then ngl logs are "not printed". 
  The default value is a pointer 
  to std::cerr, see also 
  \ref GL_DEBUG.
  \param str pointer to std::ostream to which to print ngl log.
 */
void
ngl_LogStream(std::ostream *str);

/*!\fn bool ngl_log_gl_commands(void)
  Returns true if ngl logs all
  GL API calls. If returns false,
  ngl only logs those GL API calls
  that generated GL errors, see also 
  \ref GL_DEBUG. 
  Default value is false.
 */
bool
ngl_log_gl_commands(void);

/*!\fn void ngl_log_gl_commands(bool)
  Set if ngl logs all GL API calls. 
  If false, ngl only logs those 
  GL API calls that generated GL 
  errors. If true ngl logs ALL GL
  API calls. Default value is false, 
  see also \ref GL_DEBUG.
  \param v value to set for logging
 */
void
ngl_log_gl_commands(bool v);

/*! @} */




#endif
