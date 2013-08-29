/*! 
 * \file WRATHLayerIntermediateTransformation.hpp
 * \brief file WRATHLayerIntermediateTransformation.hpp
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


#ifndef __WRATH_LAYER_INTERMEDIATE_TRANSFORMATION_HPP__
#define __WRATH_LAYER_INTERMEDIATE_TRANSFORMATION_HPP__


#include "WRATHConfig.hpp"
#include "matrixGL.hpp"
#include "WRATHReferenceCountedObject.hpp"

/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerIntermediateTransformation
  A WRATHLayerIntermediateTransformation provides
  an interface to change a transformation used
  for _rendering_ of WRATHLayer. 
 */
class WRATHLayerIntermediateTransformation:
  public WRATHReferenceCountedObjectT<WRATHLayerIntermediateTransformation>
{
public:

  virtual
  ~WRATHLayerIntermediateTransformation()
  {}

  /*!\fn void modify_matrix
    To be implemented by a derived class
    to change the matrix that a WRATHLayer
    used BEFORE it is composed with the
    parent matrix. The details are as follows:
    - For each matrix type (see \ref WRATHLayer::matrix_type),
      the matrix of that matrix type of the WRATHLayer
      is copied to a matrix M. That matrix M is passed to modify_matrix() 
      which can modify the matrix anyway it sees fit.
    - That matrix is then optionally composed with the
      transformations that have accumulated on the parent
      WRATHLayer (see \ref WRATHLayer::compose_matrix).
   */
  virtual
  void
  modify_matrix(float4x4& in_out_matrix)=0;
};

/*! @} */

#endif
