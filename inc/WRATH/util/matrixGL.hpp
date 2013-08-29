/*! 
 * \file matrixGL.hpp
 * \brief file matrixGL.hpp
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


#ifndef __MATRIX_GL_HPP__
#define __MATRIX_GL_HPP__

#include "WRATHConfig.hpp"
#include <iomanip>
#include "WRATHassert.hpp" 
#include "vectorGL.hpp"

/*! \addtogroup Utility
 * @{
 */

/*!\class matrixNxN
  A generic matrix class whose 
  entries are packed in a form 
  suitable for OpenGL:
 
  \verbatim
  data[ 0 ] data[ N  ] data[2N  ]  .. data[ N(N-1)  ]
  data[ 1 ] data[ N+1] data[2N+1]  .. data[ N(N-1)+1]
  .
  .
  data[N-1] data[2N-1] data[3N-1]  .. data[  N*N - 1] \endverbatim

 i.e. data[ row+ col*N] --> matrix(row,col), 
 with 0<=row<N, 0<=col<N
 operator()(row,col) --> data[row+col*N] <--> matrix(row,col)

 If WRATH_VECTOR_BOUND_CHECK is defined, 
 will perform bounds checking.
*/
template<unsigned int N, typename T=GLfloat>
class matrixNxN
{
private:
  vecN<T,N*N> m_data;

public:

  /*!\fn matrixNxN(const matrixNxN&)
    Copy-constructor for a NxN matrix.
    \param obj matrix to copy
   */
  matrixNxN(const matrixNxN &obj):
    m_data(obj.m_data) { }

  /*!\fn matrixNxN(void)
    Ctor.
    Initializes an NxN matrix as the identity,
    i.e. diagnols are 1 and other values are 0.
   */
  matrixNxN(void)
  {
    for(unsigned int i=0;i<N;++i)
      {
        for(unsigned int j=0;j<N;++j)
          {
            m_data[N*i+j]=
              (i==j)?
              T(1):
              T(0);
          }
      }
  }

  /*!\fn void swap(matrixNxN&)
    Swaps the values between this and the parameter matrix.
    \param obj matrix to swap values with
   */
  void
  swap(matrixNxN &obj)
  {
    m_data.swap(obj.m_data);
  }

  /*!\fn T* c_ptr(void)
    Returns a c-style pointer to the data.
   */
  T*
  c_ptr(void) { return m_data.c_ptr(); }

  /*!\fn const T* c_ptr(void) const
    Returns a constant c-style pointer to the data.
   */
  const T*
  c_ptr(void) const { return m_data.c_ptr(); }

  /*!\fn vecN<T,N*N>& raw_data(void)
    Returns a reference to raw data vector in the matrix.
   */
  vecN<T,N*N>&
  raw_data(void) { return m_data; }
  
  /*!\fn const vecN<T,N*N>& raw_data(void) const
    Returns a const reference to the raw data vectors in the matrix.
    */
  const vecN<T,N*N>&
  raw_data(void) const { return m_data; }

  /*!\fn T& operator()(unsigned int, unsigned int)
    Returns the value in the vector corresponding M[i,j]
    \param row row(vertical coordinate) in the matrix
    \param col column(horizontal coordinate) in the matrix
   */
  T&
  operator()(unsigned int row, unsigned int col)
  {
    #ifdef WRATH_VECTOR_BOUND_CHECK 
      assert(row<N);
      assert(col<N);
    #endif
    return m_data[N*col+row];
  }

  /*!\fn const T& operator()(unsigned int, unsigned int) const
    Returns the value in the vector corresponding M[i,j]
    \param row row(vertical coordinate) in the matrix
    \param col column(horizontal coordinate) in the matrix
   */
  const T&
  operator()(unsigned int row, unsigned int col) const
  {
    #ifdef WRATH_VECTOR_BOUND_CHECK 
      assert(row<N);
      assert(col<N);
    #endif
    return m_data[N*col+row];
  }

  /*!\fn vecN<T,N> row_vector(unsigned int row) const
    Returns the row vector corresponding the row given in the
    parameter, i.e. return value [I] equals operator()(row, I), 
    i.e. a set of numbers going horizontally across the matrix
    \param row target row
   */
  vecN<T,N>
  row_vector(unsigned int row) const
  {
    //return row'th row (j fixed), note that the
    //matrix is stored in column-major
    //format!
    WRATHassert(row<N);
    return vecN<T,N>(m_data, row, N);
  }

  /*!\fn void row_vector(unsigned int row, const vecN<T,N>&)
    Sets the target row of the matrix, i.e. for 0<=I<N,
    operator()(row, I) = v[I].
    \param row target row
    \param v source vector
   */
  void
  row_vector(unsigned int row, const vecN<T,N> &v)
  {
    WRATHassert(row<N);
    for(unsigned int i=0; i<N; ++i)
      {
        operator()(row,i)=v[i];
      }
  }

  /*!\fn vecN<T,N> col_vector(unsigned int col) const
    Returns the column vector corresponding the row given in the
    parameter, i.e. return value [I] equals operator()(I, col), 
    i.e. a set of numbers going vertically down the matrix
    \param col target column index
   */
  vecN<T,N>
  col_vector(unsigned int col) const
  {    
    WRATHassert(col<N);
    return vecN<T,N>(m_data, col*N, 1);
  }

  /*!\fn void col_vector(unsigned int col, const vecN<T,N>&)
    Sets the target column of the matrix, i.e. for 0<=I<N,
    operator()(I, col) = v[I].
    \param col target column index
    \param v source column vector
   */
  void
  col_vector(unsigned int col, const vecN<T,N> &v)
  {
    WRATHassert(col<N);
    for(unsigned int i=0;i<N;++i)
      {
        operator()(i,col)=v[i];
      }
  }

  /*!\fn matrixNxN transpose(void) const
    Returns a transpose of the matrix.
   */
  matrixNxN 
  transpose(void) const
  {
    matrixNxN retval;

    for(unsigned int i=0;i<N;++i)
      {
        for(unsigned int j=0;j<N;++j)
          {
            retval.operator()(i,j)=operator()(j,i);
          }
      }
    return retval;
  }

  /*!\fn void transpose_matrix(void)
    Transposes the matrix in place.
   */
  void
  transpose_matrix(void)
  {
    for(unsigned int i=0;i<N;++i)
      {
        for(unsigned int j=i;j<N;++j)
          {
            std::swap(operator()(i,j), operator()(j,i));
          }
      }
  }

  /*!\fn matrixNxN operator+(const matrixNxN&) const
    Operator for adding matrices together.
    \param matrix target matrix
   */
  matrixNxN
  operator+(const matrixNxN &matrix) const
  {
    matrixNxN out;
    out.m_data= m_data+matrix.m_data;
    return out;
  }

  /*!\fn matrixNxN operator-(const matrixNxN&) const
    Operator for substracting matrices from each other.
    \param matrix target matrix
   */
  matrixNxN
  operator-(const matrixNxN &matrix) const
  {
    matrixNxN out;
    out.m_data= m_data-matrix.m_data;
    return out;
  }

  /*!\fn matrixNxN operator*(const T&) const
    Multiplies the matrix with a given scalar.
    \param value scalar to multiply with
   */
  matrixNxN
  operator*(const T &value) const
  {
    matrixNxN out;
    out.m_data= m_data*value;
    return out;
  }
  
  /*!\fn matrixNxN operator*(const T&, const matrixNxN&)
    Multiplies the given matrix with the given scalar
    and returns the resulting matrix.
    \param value scalar to multiply with
    \param matrix target matrix to multiply
   */
  friend
  matrixNxN
  operator*(const T &value, const matrixNxN &matrix)
  {
    matrixNxN out;
    out.m_data=matrix.m_data*value;
    return out;
  }

  /*!\fn matrixNxN operator*(const matrixNxN&) const
    Multiplies this matrix with the given matrix.
    \param matrix target matrix
   */
  matrixNxN
  operator*(const matrixNxN &matrix) const
  {
    unsigned int i,j,k;
    matrixNxN out;

    for(i=0;i<N;++i)
      {
        for(j=0;j<N;++j)
          {
            out.operator()(i,j)=T(0);
            for(k=0;k<N;++k)
              {
                out.operator()(i,j)+=operator()(i,k)*matrix.operator()(k,j);
              }
          }
      }
    return out;
  }

  /*!\fn vecN<T,N> operator*(const vecN<T,N>&) const
    Multiplies the given vector with the matrix.
    \param in target vector
   */
  vecN<T,N>
  operator*(const vecN<T,N> &in) const
  {
    vecN<T,N> retval;

    for(unsigned int i=0;i<N;++i)
    {
      retval[i]=T(0);
      for(unsigned int j=0;j<N;++j)
        {
          retval[i]+=operator()(i,j)*in[j];
        }
    }

    return retval;
  }


  /*!\fn vecN<T,N> operator*(const vecN<T,N>&, const matrixNxN&)
    Multiplies the target matrix with the given vector
    \param matrix target matrix
    \param in target vector
   */
  friend
  vecN<T,N>
  operator*(const vecN<T,N> &in, const matrixNxN &matrix)
  {
    vecN<T,N> retval;

    for(unsigned int i=0;i<N;++i)
    {
      retval[i]=T(0);
      for(unsigned int j=0;j<N;++j)
        {
          retval[i]+=in[j]*matrix.operator()(j,i);
        }
    }
    return retval;
  }

  /*!\fn std::ostream& operator<<(std::ostream&, const matrixNxN&)
    Pushes a string representation of the given matrix to the given
    stream.
    \param ostr output stream
    \param matrix source matrix
   */
  friend
  std::ostream&
  operator<<(std::ostream &ostr, const matrixNxN &matrix)
  {
    unsigned int i,j;

    ostr << "\n";
    for(i=0;i<N;++i)
    {
      ostr << "|";
      for(j=0;j<N;++j)
        {
          ostr << std::setw(14) << std::setprecision(6) << matrix.operator()(i,j) << " ";
        }
      ostr << "|\n";
    }
    return ostr;
  }


};





namespace std
{
  /*!\fn swap(matrixNxN<N,T>&, matrixNxN<N,T>&)
    Swaps the two given matrices in place.
    \param obj0 first matrix
    \param obj1 second matrix
   */
  template<unsigned int N, typename T>
  inline
  void
  swap(matrixNxN<N,T> &obj0, matrixNxN<N,T> &obj1)
  {
    obj0.swap(obj1);
  }
}

/*!\typedef float2x2
  Convenience typedef to \ref matrixNxN\<2, float\>
 */
typedef matrixNxN<2, float> float2x2;

/*!\class matrix3x3
  A representation of a 3x3 matrix, that in addition to the NxN
  matrix functionality provides function for calculating the
  determinant.
 */
template<typename T>
class matrix3x3:public matrixNxN<3,T>
{
public:
  /*!\fn matrix3x3(void)
    Initializes the 3x3 matrix as the identity,
    i.e. diagnols are 1 and other values are 0.
   */
  matrix3x3(void):matrixNxN<3,T>() {}

  /*!\fn matrix3x3(const matrixNxN<3,T>&)
    Copy-constructor for a 3x3 matrix.
    \param obj target matrix to copy.
   */
  matrix3x3(const matrixNxN<3,T> &obj):matrixNxN<3,T>(obj) {}
  
  /*!\fn matrix3x3(const vecN<T,3>&, const vecN<T,3>&, const vecN<T,3>&)
    Construct a matrix3x3 M so that
     - M*vecN<T,3>(1,0,0)=T
     - M*vecN<T,3>(0,1,0)=B
     - M*vecN<T,3>(0,0,1)=N
    \param t first row vector
    \param b second row vector
    \param n third row vector
   */
  matrix3x3(const vecN<T,3> &t, const vecN<T,3> &b, const vecN<T,3> &n)
  {
    for(int i=0;i<3;++i)
      {
        matrixNxN<3, T>::operator()(i,0)=t[i];
        matrixNxN<3, T>::operator()(i,1)=b[i];
        matrixNxN<3, T>::operator()(i,2)=n[i];
      }
  }

  /*!\fn T determinate(void) const
    Calculates the determinant for the matrix.
   */
  T 
  determinate(void) const
  {
    return matrixNxN<3,T>::operator()(0,0)*
      ( matrixNxN<3,T>::operator()(1,1)*matrixNxN<3,T>::operator()(2,2) 
        - matrixNxN<3,T>::operator()(1,2)*matrixNxN<3,T>::operator()(2,1) )

      - matrixNxN<3,T>::operator()(1,0)*
      ( matrixNxN<3,T>::operator()(0,1)*matrixNxN<3,T>::operator()(2,2) 
        - matrixNxN<3,T>::operator()(2,1)*matrixNxN<3,T>::operator()(0,2) )

      + matrixNxN<3,T>::operator()(2,0)*
      ( matrixNxN<3,T>::operator()(0,1)*matrixNxN<3,T>::operator()(1,2)
        - matrixNxN<3,T>::operator()(1,1)*matrixNxN<3,T>::operator()(0,2) ) ;
  }

  /*!\fn bool reverses_orientation(void) const
    Checks whether the matrix reverses orientation. This check is equivalent
    to (determinant < 0) ? true : false.
   */
  bool
  reverses_orientation(void) const
  {
    return determinate()<T(0);
  }

};

namespace std
{
  /*!\fn swap(matrix3x3<T>&, matrix3x3<T>&)
    Swaps the two 3x3 matrices in place.
    \param obj0 first matrix
    \param obj1 second matrix
   */
  template<typename T>
  inline
  void
  swap(matrix3x3<T> &obj0, matrix3x3<T> &obj1)
  {
    obj0.swap(obj1);
  }
}

/*!\typedef float3x3
  Convenience typedef to \ref matrix3x3\<float\>
 */
typedef matrix3x3<float> float3x3;


/*!\class projection_params
  A projection_params holds the 
  data to describe a projection
  matrix with and without perspective.
 */
template<typename T>
class projection_params
{
public:
  T m_top; ///< Top edge of the clipping plane
  T m_bottom; ///< Bottom edge of the clipping plane
  T m_left; ///< Left edge of the clipping plane
  T m_right; ///< Right edge of the clipping plane
  T m_near; ///< Near clipping plane distance
  T m_far; ///< Far clipping plane distance

  /*!\var m_farAtinfinity
    True when the far clipping plane is not set (and is thus in infinity).
    */
  bool m_farAtinfinity;

  /*!\fn projection_params(void)
    Default constructor for projection parameters,
    values are unitialized except for
    \ref m_farAtinfinity which is initialized
    as false
   */
  projection_params(void):
    m_farAtinfinity(false)
  {}
  
  /*!\fn projection_params(T, T, T, T, T, T)
    Creates the projection parameters instance according to the given parameters.
    \param l Left
    \param r Right
    \param t Top
    \param b Bottom
    \param n Near clipping plane
    \param f Far clipping plane
   */
  projection_params(T l, T r, T b, T t, T n,T f):
    m_top(t), m_bottom(b), m_left(l), m_right(r), m_near(n), m_far(f),
    m_farAtinfinity(false) {}
  
  /*!\fn projection_params(T,T,T,T,T)
    Creates the projection parameters instance according to the given parameters,
    where far clipping plane is not defined and is, thus, set to infinity.
    \param l Left
    \param r Right
    \param t Top
    \param b Bottom
    \param n Near clipping plane
   */
  projection_params(T l, T r, T b, T t, T n):
    m_top(t), m_bottom(b), m_left(l), m_right(r), m_near(n), m_far(100000.0f*n),
    m_farAtinfinity(true) {}
 
  /*!\fn projection_params(float, float, T, T)
    Creates the projection parameters instance according to the given parameters.
    \param fov field of view
    \param aspect aspect ratio
    \param pnear near clipping plane
    \param pfar far clipping plane
   */
  projection_params(float fov, float aspect, T pnear, T pfar):
    m_near(pnear),
    m_far(pfar),
    m_farAtinfinity(false)
  {
    float f, recip_f;
    
    recip_f=std::tan(fov/2.0f * M_PI/180.0f);
    f=1.0/recip_f;
        
    // we have that 2*near/(right-left)=f/aspect and left=-right
    // thus near/right=f/aspect thus right= near * aspect/f
    m_right=m_near*aspect*recip_f;
    m_left=-m_right;
    
    // we have that 2*near/(top-bottom)=f and bottom=-bottom
    // thus near/top=f, thus top= near/f
    m_top=m_near*recip_f;
    m_bottom=-m_top;
  }

  /*!\fn projection_params(float, float, T)
    Creates the projection parameters instance according to the given parameters,
    where far clipping plane is not defined and is, thus, set to infinity.
    \param fov field of view
    \param aspect aspect ratio
    \param pnear near clipping plane
   */
  projection_params(float fov, float aspect, T pnear):
    m_near(pnear), 
    m_far(100000.0f*pnear),
    m_farAtinfinity(true) 
  {
    float f, recip_f;
    
    recip_f=std::tan(fov/2.0f * M_PI/180.0f);
    f=1.0/recip_f;
        
    // we have that 2*near/(right-left)=f/aspect and left=-right
    // thus near/right=f/aspect thus right= near * aspect/f
    m_right=m_near*aspect*recip_f;
    m_left=-m_right;
    
    // we have that 2*near/(top-bottom)=f and bottom=-bottom
    // thus near/top=f, thus top= near/f
    m_top=m_near*recip_f;
    m_bottom=-m_top;
  }
};

/*!\class orthogonal_projection_params
  An extension to projection parameters to provide projection parameters
  better suited for orthogonal projection.
  */
template<typename T>
class orthogonal_projection_params:public projection_params<T>
{
public:
  /*!\fn orthogonal_projection_params(T, T, T, T, T, T)
    Creates projection parameters defined by the given parameters.
    Equivalent to projection_params<T>(l, r, b, t, n, f).
    \param l Left
    \param r Right
    \param t Top
    \param b Bottom
    \param n Near clipping plane
    \param f Far clipping plane
   */
  orthogonal_projection_params(T l, T r, T b, T t, T n,T f):
    projection_params<T>(l,r,b,t,n,f)
  {}

  /*!\fn orthogonal_projection_params(T,T,T,T)
    Creates a projection parameters instance defined by the given plane
    size, assuming the clipping planes at Near=T(-1), Far=T(1).
    Equivalent to projection_params<T>(l, r, b, t, T(-1), T(1)).
    \param l Left
    \param r Right
    \param t Top
    \param b Bottom
   */
  orthogonal_projection_params(T l, T r, T b, T t):
    projection_params<T>(l,r,b,t, T(-1), T(1) )
  {}
};

/*!\typedef float_projection_params
  Convenience typedef for \ref projection_params\<float\>
 */
typedef projection_params<float> float_projection_params;

/*!\typedef float_orthogonal_projection_params
  Convenience typedef for \ref orthogonal_projection_params\<float\>
 */
typedef orthogonal_projection_params<float> float_orthogonal_projection_params;


/*!\class matrix4x4
  4x4 matrix class that provides functions for matrix math,
  such as scaling, translating and creating projection matrices.
  Special case of NxN matrix.
 */
template<typename T>
class matrix4x4:public matrixNxN<4, T> 
{
public:
  /*!\fn matrix4x4(void)
    Initializes the 4x4 matrix as the identity,
    i.e. diagnols are 1 and other values are 0.
   */
  matrix4x4(void):matrixNxN<4,T>() {}

  /*!\fn matrix4x4(const matrixNxN<4,T>&)
    Copy-constructor for const 4x4 matrix.
    \param obj matrix to be copied from
   */
  matrix4x4(const matrixNxN<4,T> &obj):matrixNxN<4,T>(obj) {}

  /*!\fn matrix4x4(const vecN<T,3>&,
                   const vecN<T,3>&,
                   const vecN<T,3>&,
                   const vecN<T,3>&)
   Constructs a matrix4x4 from the given vectors so that
   - M*vecN<T,4>(0,0,0,1)=origin
   - M*vecN<T,4>(1,0,0,0)=right
   - M*vecN<T,4>(0,1,0,0)=up
   - M*vecN<T,4>(0,0,1,0)=backwards
   \param origin M(0,0,0,1)
   \param right M(1,0,0,0)
   \param up M(0,1,0,0)
   \param backwards M(0,0,1,0)
   */
  matrix4x4(const vecN<T,3> &origin, 
            const vecN<T,3> &right, 
            const vecN<T,3> &up, 
            const vecN<T,3> &backwards)
  {
    for(int i=0;i<3;++i)
      {
        matrixNxN<4, T>::operator()(i,0)=right[i];
        matrixNxN<4, T>::operator()(i,1)=up[i];
        matrixNxN<4, T>::operator()(i,2)=backwards[i];
        matrixNxN<4, T>::operator()(i,3)=origin[i];
        
        matrixNxN<4, T>::operator()(3,i)=T(0);
      }
    matrixNxN<4, T>::operator()(3,3)=T(1);    
  }

  /*!\fn matrix4x4(const vecN<T,3>&)
    Create a 4x4 matrix representing translation by translate.
    \param translate translation matrix
    */
  explicit 
  matrix4x4(const vecN<T,3> &translate):
    matrixNxN<4,T>()
  {
    for(int i=0;i<3;++i)
      {
        matrixNxN<4, T>::operator()(i,3)=translate[i];
      }
  }

  /*!\fn matrix4x4(const matrixNxN<3,T>&, const vecN<T,3>&)
    Creates a translated 4x4 matrix from a given matrix and
    translation.
    \param m source matrix
    \param translate translation matrix
    */
  explicit
  matrix4x4(const matrixNxN<3,T> &m,
            const vecN<T,3> &translate):
    matrixNxN<4,T>()
  {
    for(int i=0;i<3;++i)
      {
        matrixNxN<4, T>::operator()(i,3)=translate[i];
        matrixNxN<4, T>::operator()(3,i)=T(0);

        for(int j=0;j<3;++j)
          {
            matrixNxN<4, T>::operator()(i,j)=m.operator()(i,j);
          }
      }
    matrixNxN<4, T>::operator()(3,3)=T(1);   
  }

  /*!\fn matrix4x4(const matrixNxN<3,T>&)
    Copy-constructor for a 4x4 matrix from
    a 3x3 matrix
    \param m source matrix to copy
    */
  explicit
  matrix4x4(const matrixNxN<3,T> &m):
    matrixNxN<4,T>()
  {
    for(int i=0;i<3;++i)
      {
        for(int j=0;j<3;++j)
          {
            matrixNxN<4, T>::operator()(i,j)=m.operator()(i,j);
          }
      }
  }

  /*!\fn matrix4x4(const T&, const T&, const T&)
    Creates a scaling 4x4 matrix for scaling point's x, y and z axis.
    \param scaleX X-axis scaling factor
    \param scaleY Y-axis scaling factor
    \param scaleZ Z-axis scaling factor
    */
  matrix4x4(const T &scaleX, const T &scaleY, const T &scaleZ)
  {
    matrixNxN<4, T>::operator()(0,0)=scaleX;
    matrixNxN<4, T>::operator()(1,1)=scaleY;
    matrixNxN<4, T>::operator()(2,2)=scaleZ;
  } 

  /*!\fn matrix4x4(T, vecN<T,3>)
    Creates a rotation 4x4 matrix based on the angle variable around
    axis determined by the 3 dimensional vector.
    \param angle_radians rotation angle in radians
    \param rotation_axis axis to rotate around: [x,y,z]
    */
  matrix4x4(T angle_radians, vecN<T,3> rotation_axis)
  {
    T s, c, one_minus_c;
    
    rotation_axis.normalize();
    const T &x(rotation_axis.x());
    const T &y(rotation_axis.y());
    const T &z(rotation_axis.z());
    

    s=std::sin(angle_radians);
    c=std::cos(angle_radians);
    
    one_minus_c=T(1)-c;

    matrixNxN<4, T>::operator()(0,0)=x*x*one_minus_c + c;
    matrixNxN<4, T>::operator()(1,0)=y*x*one_minus_c + z*s;
    matrixNxN<4, T>::operator()(2,0)=x*z*one_minus_c - y*s;
    matrixNxN<4, T>::operator()(3,0)=T(0);

    matrixNxN<4, T>::operator()(0,1)=x*y*one_minus_c - z*s;
    matrixNxN<4, T>::operator()(1,1)=y*y*one_minus_c + c;
    matrixNxN<4, T>::operator()(2,1)=y*z*one_minus_c + x*s;
    matrixNxN<4, T>::operator()(3,1)=T(0);

    matrixNxN<4, T>::operator()(0,2)=x*z*one_minus_c + y*s;
    matrixNxN<4, T>::operator()(1,2)=y*z*one_minus_c - x*s;
    matrixNxN<4, T>::operator()(2,2)=z*z*one_minus_c + c;
    matrixNxN<4, T>::operator()(3,2)=T(0);
  }

  /*!\fn matrix4x4(const projection_params<T>&)
    Creates a 4x4 projection matrix from the given projection parameters.
    \param P projection parameters for the matrix
   */
  explicit
  matrix4x4(const projection_params<T> &P):
    matrixNxN<4,T>()
  {
    projection_matrix(P);
  }

  /*!\fn matrix4x4(const orthogonal_projection_params<T>&)
    Creates an 4x4 orthogonal projection matrix from the given projection
    parameters.
    \param P orthogonal projection parameters for the matrix
    */
  explicit
  matrix4x4(const orthogonal_projection_params<T> &P):
    matrixNxN<4,T>()
  {
    orthogonal_projection_matrix(P);
  }

  /*!\fn void projection_matrix(const projection_params<T>&)
    Sets the matrix values to correspond the projection matrix
    determined by the given projection parameters.
    \param P projection parameters for this matrix
   */
  void
  projection_matrix(const projection_params<T> &P)
  {
    matrixNxN<4, T>::operator()(0,0)=T(2)*P.m_near/(P.m_right-P.m_left);
    matrixNxN<4, T>::operator()(1,0)=T(0);
    matrixNxN<4, T>::operator()(2,0)=T(0);
    matrixNxN<4, T>::operator()(3,0)=T(0);
    
    matrixNxN<4, T>::operator()(0,1)=T(0);
    matrixNxN<4, T>::operator()(1,1)=T(2)*P.m_near/(P.m_top-P.m_bottom);
    matrixNxN<4, T>::operator()(2,1)=T(0);
    matrixNxN<4, T>::operator()(3,1)=T(0);


    matrixNxN<4, T>::operator()(0,2)=(P.m_right+P.m_left)/(P.m_right-P.m_left);
    matrixNxN<4, T>::operator()(1,2)=(P.m_top+P.m_bottom)/(P.m_top-P.m_bottom);
    matrixNxN<4, T>::operator()(3,2)=T(-1);

    matrixNxN<4, T>::operator()(0,3)=T(0);
    matrixNxN<4, T>::operator()(1,3)=T(0);
    matrixNxN<4, T>::operator()(3,3)=T(0);
    
    if(!P.m_farAtinfinity)  
      {
        matrixNxN<4, T>::operator()(2,2)=(P.m_near+P.m_far)/(P.m_near-P.m_far);
        matrixNxN<4, T>::operator()(2,3)=T(2)*P.m_near*P.m_far/(P.m_near-P.m_far);
      }
    else
      {
        matrixNxN<4, T>::operator()(2,2)=T(-1);
        matrixNxN<4, T>::operator()(2,3)=T(-2)*P.m_near;
      }
  }

  /*!\fn void inverse_projection_matrix(const projection_params<T>&)
    Sets the matrix values to correspond the inverse of the projection
    matrix determined by the given projection parameters.

    \param P projection parameters for this matrix
   */
  void
  inverse_projection_matrix(const projection_params<T> &P)
  {
    matrixNxN<4, T>::operator()(0,0)=(P.m_right-P.m_left)/( T(2)*P.m_near);
    matrixNxN<4, T>::operator()(1,0)=T(0);
    matrixNxN<4, T>::operator()(2,0)=T(0);
    matrixNxN<4, T>::operator()(3,0)=T(0);
    
    matrixNxN<4, T>::operator()(0,1)=T(0);
    matrixNxN<4, T>::operator()(1,1)=(P.m_top-P.m_bottom)/( T(2)*P.m_near);
    matrixNxN<4, T>::operator()(2,1)=T(0);
    matrixNxN<4, T>::operator()(3,1)=T(0);
    
    matrixNxN<4, T>::operator()(0,2)=T(0);
    matrixNxN<4, T>::operator()(1,2)=T(0);
    matrixNxN<4, T>::operator()(2,2)=T(0);
    
    matrixNxN<4, T>::operator()(0,3)=(P.m_right+P.m_left)/( T(2)*P.m_near);
    matrixNxN<4, T>::operator()(1,3)=(P.m_top+P.m_bottom)/( T(2)*P.m_near);
    matrixNxN<4, T>::operator()(2,3)=T(-1);
    
    if(!P.m_farAtinfinity)  
      {      
        matrixNxN<4, T>::operator()(3,2)=(P.m_near-P.m_far)/(P.m_far*P.m_near*T(2));
        matrixNxN<4, T>::operator()(3,3)=(P.m_near+P.m_far)/(P.m_far*P.m_near*T(-2));
      }
    else
      {
        matrixNxN<4, T>::operator()(3,2)=T(-1)/ (2.0f*P.m_near);
        matrixNxN<4, T>::operator()(3,3)=T(-1)/ (2.0f*P.m_near);
      }
  }
  
  /*!\fn void orthogonal_projection_matrix(const projection_params<T>&)
    Sets the matrix variables to correspond an orthogonal projection matrix
    determined by the given projection parameters.
    \param P orthogonal projection parameters for this matrix
   */
  void
  orthogonal_projection_matrix(const projection_params<T> &P)
  {
    matrixNxN<4, T>::operator()(0,0)=T(2)/(P.m_right-P.m_left);
    matrixNxN<4, T>::operator()(1,0)=T(0);
    matrixNxN<4, T>::operator()(2,0)=T(0);
    matrixNxN<4, T>::operator()(3,0)=T(0);
    
    matrixNxN<4, T>::operator()(0,1)=T(0);
    matrixNxN<4, T>::operator()(1,1)=T(2)/(P.m_top-P.m_bottom);
    matrixNxN<4, T>::operator()(2,1)=T(0);
    matrixNxN<4, T>::operator()(3,1)=T(0);
    
    matrixNxN<4, T>::operator()(0,2)=T(0);
    matrixNxN<4, T>::operator()(1,2)=T(0);
    matrixNxN<4, T>::operator()(2,2)=T(2)/(P.m_near-P.m_far);
    matrixNxN<4, T>::operator()(3,2)=T(0);
    
    matrixNxN<4, T>::operator()(0,3)=(P.m_right+P.m_left)/(P.m_left-P.m_right);
    matrixNxN<4, T>::operator()(1,3)=(P.m_top+P.m_bottom)/(P.m_bottom-P.m_top);
    matrixNxN<4, T>::operator()(2,3)=(P.m_near+P.m_far)/(P.m_near-P.m_far);
    matrixNxN<4, T>::operator()(3,3)=T(1);
  }

  /*!\fn void orthogonal_projection_matrix(const T&, const T&, const T&, const T&, const T&, const T&)
    Convenience function for matrix4x4::orthogonal_projection_matrix(const project_params<T>&).
    \param l Left
    \param r Right
    \param b Bottom
    \param t Top
    \param n Near clip plane
    \param f Far clip plane
   */
  void
  orthogonal_projection_matrix(const T &l,  const T &r,  const T &b,  const T &t,  const T &n,  const T &f)
  {
    orthogonal_projection_matrix( projection_params<T>(l,r,b,t,n,f));
  }

  /*!\fn void orthogonal_projection_matrix(const T&, const T&, const T&, const T&)
    Convenience function for matrix4x4::orthogonal_projection_matrix(const project_params<T>&).
    Equivalent to
    \code
    orthogonal_projection_matrix(l, r, b, t, T(-1), T(1) );
    \endcode
    \param l Left
    \param r Right
    \param b Bottom
    \param t Top
   */
  void
  orthogonal_projection_matrix(const T &l,  const T &r,  const T &b,  const T &t)
  {
    orthogonal_projection_matrix(l,r,b,t, T(-1), T(1) );
  }

  /*!\fn void translate_matrix(const vecN<T,3>&)
    Compose this matrix with a tanslation matrix
    \param v translation by axis (x,y,z,1)
   */
  void 
  translate_matrix(const vecN<T,3> &v)
  {
    matrix4x4 temp(v);

    (*this)=(*this) * temp;
  }

  /*!\fn void scale_matrix(float, float, float)
    Scales the matrix based on the per-axis basis
    \param sx x-axis scaling
    \param sy y-axis scaling
    \param sz z-axis scaling
   */
  void
  scale_matrix(float sx, float sy, float sz)
  {
    matrix4x4 temp(sx, sy, sz);
    (*this)=(*this) * temp;
  }

  /*!\fn void rotate_matrix(T, const vecN<T,3>&)
    Rotates the matrix the given amount in radians around axis
    determined by a vec3.
    \param angle_radians Angle to rotate in radians
    \param rotation_axis Axis to rotate around [x, y, z]
   */
  void
  rotate_matrix(T angle_radians, const vecN<T,3> &rotation_axis)
  {
    matrix4x4 temp(angle_radians, rotation_axis);

    (*this)=(*this) * temp;
  }

  /*!\fn vecN<T,3> apply_to_point(const vecN<T,3>&) const
    Multiplies the matrix with the given vecN<T,4>(x,y,z,1) and returns
    the resulting vecN<T,3>. This effectively transforms the point according
    to the matrix transforms.
    \param in target vector to apply matrix transform to
   */
  vecN<T,3> 
  apply_to_point(const vecN<T,3> &in) const
  {
    vecN<T,4> temp;
    
    temp[0]=in[0];
    temp[1]=in[1];
    temp[2]=in[2];
    temp[3]=T(1);

    temp=(*this)*temp;
    
    return vecN<T,3>(temp);
  }

  /*!\fn vecN<T,3> apply_to_direction(const vecN<T,3>&) const
    Multiplies the matrix with the given vecN<T,4>(x,y,z,0) and returns
    the resulting vecN<T,3>. This effectively transforms the direction
    defined by the vector according to the matrix transforms.
    \param in target direction vector to apply the transform to
   */
  vecN<T,3> 
  apply_to_direction(const vecN<T,3> &in) const
  {
    vecN<T,4> temp;

    temp[0]=in[0];
    temp[1]=in[1];
    temp[2]=in[2];
    temp[3]=T(0);

    temp=(*this)*temp;

    return vecN<T,3>(temp);
  }

  /*!\fn matrixNxN<3,T> upper3x3_submatrix(void) const
    Returns a 3x3 matrix representing the upper-left part
    of the matrix:\n\n
      M11 M12 M13|M14\n
      M21 M22 M23|M24\n
      M31 M32 M33|M34\n
      ---------------\n
      M41 M42 M43|M44\n
   */
  matrixNxN<3,T> 
  upper3x3_submatrix(void) const
  {
    matrixNxN<3,T> retval;
    for(int i=0;i<3;++i)
      {
        for(int j=0;j<3;++j)
          {
            retval.operator()(i,j)=matrixNxN<4,T>::operator()(i,j);
          }
      }
    return retval;
  }

  /*!\fn vecN<T,3> translation_vector(void) const
    Returns the translation vector of the matrix,
    i.e. vecN<T,3>(operator()(0,0), operator()(1,0), operator()(2,0)),
    i.e. the last column of the matrix truncating
    the last element of the last column.
    */
  vecN<T,3> 
  translation_vector(void) const
  {
    vecN<T,3> retval;

    for(int i=0;i<3;++i)
      {
        retval[i]=matrixNxN<4,T>::operator()(i,3);
      }

    return retval;
  }

  /*!\fn void translation_vector(const vecN<T,3> &)
    Sets the translation vector of the matrix,
    i.e. for each 0<=I<3, operator(i,3)=v[i].
    \param v new translation vector
   */
  void
  translation_vector(const vecN<T,3> &v) 
  {
    for(int i=0;i<3;++i)
      {
        matrixNxN<4,T>::operator()(i,3)=v[i];
      }
  }

  /*!\fn T upper3x3_determinate(void) const  
    Calculate and return the determinant of the upper-left 3x3 block.
  */
  T 
  upper3x3_determinate(void) const
  {
    return matrixNxN<4,T>::operator()(0,0)*
      ( matrixNxN<4,T>::operator()(1,1)*matrixNxN<4,T>::operator()(2,2) 
        - matrixNxN<4,T>::operator()(1,2)*matrixNxN<4,T>::operator()(2,1) )

      - matrixNxN<4,T>::operator()(1,0)*
      ( matrixNxN<4,T>::operator()(0,1)*matrixNxN<4,T>::operator()(2,2) 
        - matrixNxN<4,T>::operator()(2,1)*matrixNxN<4,T>::operator()(0,2) )

      + matrixNxN<4,T>::operator()(2,0)*
      ( matrixNxN<4,T>::operator()(0,1)*matrixNxN<4,T>::operator()(1,2)
        - matrixNxN<4,T>::operator()(1,1)*matrixNxN<4,T>::operator()(0,2) ) ;
  }

  /*!\fn bool reverses_orientation(void) const
    Returns true if the matrix reverses orientation (upper 3x3 determinant is
    lesser than 0).
   */
  bool 
  reverses_orientation(void) const
  {
    return upper3x3_determinate()<T(0);
  }
  

};


namespace std
{
  /*!\fn swap(matrix4x4<T>&, matrix4x4<T>&)

    Swaps two matrices in place.
   */
  template<typename T>
  inline
  void
  swap(matrix4x4<T> &obj0, matrix4x4<T> &obj1)
  {
    obj0.swap(obj1);
  }
}

/*!\typedef float4x4
  A convenience typedef to \ref matrix4x4\<float\>
 */
typedef matrix4x4<float> float4x4;


/*! @} */


#endif
