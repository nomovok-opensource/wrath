/*! 
 * \file WRATHgluniform.hpp
 * \brief file WRATHgluniform.hpp
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



#ifndef __WRATH_GL_UNIFORM_HPP__
#define __WRATH_GL_UNIFORM_HPP__

#include "WRATHConfig.hpp"
#include <vector>
#include "WRATHgl.hpp"
#include "matrixGL.hpp"
#include "vecN.hpp"
#include "WRATHNew.hpp"
#include "c_array.hpp"


/*! \addtogroup GLUtility
 * @{
 */


#include "WRATHgluniform_implement.tcc"


/*!\fn void WRATHglUniform(int, GLsizei, const vecN<T,N> &)
  Template version for setting array of uniforms,
  equivalent to
  \code
  WRATHglUniform(location, count, v.c_ptr());
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
  \param count number of elements from the array v to use.
 */
template<typename T, unsigned int N>
void
WRATHglUniform(int location, GLsizei count, const vecN<T,N> &v)
{
  WRATHglUniform(location, count, v.c_ptr());
}

/*!\fn void WRATHglUniform(int, GLsizei, const vecN<T,N>&, bool)
  Template version for setting array of uniform matrices,
  equivalent to
  \code
  WRATHglUniform(location, count, v.c_ptr(), transposed);
  \endcode
  This call is for when v is an array of matrices.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
  \param count number of elements from the array v to use
  \param transposed flag to indicate if the matrices are to be transposed
 */
template<typename T, unsigned int N>
void
WRATHglUniform(int location, GLsizei count, const vecN<T,N> &v, bool transposed)
{
  WRATHglUniform(location, count, v.c_ptr(), transposed);
}


/*!\fn void WRATHglUniform(int, GLsizei, const_c_array<T>)
  Template version for setting array of uniforms,
  equivalent to
  \code
  WRATHglUniform(location, count, &v[0]);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
  \param count number of elements from the array v to use.
 */
template<typename T>
void
WRATHglUniform(int location, GLsizei count, const_c_array<T> v)
{
  if(!v.empty())
    {
      WRATHglUniform(location, count, &v[0]);
    }
}


/*!\fn void WRATHglUniform(int, GLsizei, const std::vector<T> &)
  Template version for setting array of uniforms,
  equivalent to
  \code
  WRATHglUniform(location, count, &v[0]);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
  \param count number of elements from the array v to use.
 */
template<typename T>
void
WRATHglUniform(int location, GLsizei count, const std::vector<T> &v)
{
  if(!v.empty())
    {
      WRATHglUniform(location, count, &v[0]);
    }
}


/*!\fn void WRATHglUniform(int, GLsizei, const_c_array<T>, bool)
  Template version for setting array of uniform of matices,
  equivalent to
  \code
  WRATHglUniform(location, count, &v[0], transposed);
  \endcode
  This call is for when v is an array of matrices.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
  \param count number of elements from the array v to use
  \param transposed flag to indicate if the matrices are to be transposed
 */
template<typename T>
void
WRATHglUniform(int location, GLsizei count, const_c_array<T> v, bool transposed)
{
  if(!v.empty())
    {
      WRATHglUniform(location, count, &v[0], transposed);
    }
}


/*!\fn void WRATHglUniform(int, GLsizei, const std::vector<T>&, bool)
  Template version for setting array of uniform matrices,
  equivalent to
  \code
  WRATHglUniform(location, count, &v[0], transposed);
  \endcode
  This call is for when v is an array of matrices.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
  \param count number of elements from the array v to use
  \param transposed flag to indicate if the matrices are to be transposed
 */
template<typename T>
void
WRATHglUniform(int location, GLsizei count, const std::vector<T> &v, bool transposed)
{
  if(!v.empty())
    {
      WRATHglUniform(location, count, &v[0], transposed);
    }
}



/*!\fn void WRATHglUniform(int, const_c_array<T>)
  Template version for setting array of uniforms,
  equivalent to
  \code
  WRATHglUniform(location, v.size(), &v[0]);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
 */
template<typename T>
void
WRATHglUniform(int location, const_c_array<T> v)
{
  if(!v.empty())
    {
      WRATHglUniform(location, v.size(), &v[0]);
    }
}


/*!\fn void WRATHglUniform(int, const std::vector<T>&)
  Template version for setting array of uniforms,
  equivalent to
  \code
  WRATHglUniform(location, v.size(), &v[0]);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
 */
template<typename T>
void
WRATHglUniform(int location, const std::vector<T> &v)
{
  if(!v.empty())
    {
      WRATHglUniform(location, v.size(), &v[0]);
    }
}




/*! @} */





#endif
