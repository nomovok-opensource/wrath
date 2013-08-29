/*! 
 * \file WRATHPolynomial.cpp
 * \brief file WRATHPolynomial.cpp
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
#include "WRATHMutex.hpp"
#include "WRATHNew.hpp"
#include "WRATHPolynomial.hpp"
#include "WRATHStaticInit.hpp"

namespace
{
  class bernstein_matrices:boost::noncopyable
  {
  public:
    
    bernstein_matrices(void);
    ~bernstein_matrices();

    const boost::multi_array<int, 2>&
    matrix(unsigned int degree,
           enum WRATHUtil::reverse_control_points_t t);


  private:
    std::vector< boost::multi_array<int, 2>*> m_matrices;
    std::vector< boost::multi_array<int, 2>*> m_reverse_matrices;
    WRATHMutex m_mutex;
  };

  bernstein_matrices&
  the_bernstein_matrices(void)
  {
    WRATHStaticInit();
    static bernstein_matrices R;
    return R;
  }

  int
  get_entry(int k, int n, 
            const boost::multi_array<int, 2> &M,
            int degreeM)
  {
    return (k>degreeM or n>degreeM or n<0 or k<0)?
      0:
      M[k][n];
  }

}

bernstein_matrices::
bernstein_matrices(void)
{
  m_matrices.push_back(WRATHNew boost::multi_array<int, 2>(boost::extents[1][1]));
  m_reverse_matrices.push_back(WRATHNew boost::multi_array<int, 2>(boost::extents[1][1]));

  (*m_matrices.back())[0][0]=1;
  (*m_reverse_matrices.back())[0][0]=1;
}


bernstein_matrices::
~bernstein_matrices()
{
  for(std::vector< boost::multi_array<int, 2>*>::iterator 
        iter=m_matrices.begin(), end=m_matrices.end();
      iter!=end; ++iter)
    {
      WRATHDelete(*iter);
    }

  for(std::vector< boost::multi_array<int, 2>*>::iterator 
        iter=m_reverse_matrices.begin(), end=m_reverse_matrices.end();
      iter!=end; ++iter)
    {
      WRATHDelete(*iter);
    }
}

const boost::multi_array<int, 2>&
bernstein_matrices::
matrix(unsigned int degree,
       enum WRATHUtil::reverse_control_points_t t)
{
  const boost::multi_array<int, 2> *ptr;

  WRATHLockMutex(m_mutex);

  /*
    Matrix generation... the formula is essentially given by:

    M[0] [0][0] = 1

    
    M[N] [k][i] = M[N-1][k][i] + M[N-1][k-1][i-1] - M[N-1][k-1][i-1]

    with the convention that M[A][b][c] is 0 if b or c are not
    within range (i.e. b<0 or c<0 or b>A or c>A).
   */
  if(degree>=m_matrices.size())
    {
      unsigned int oldDegree(m_matrices.size());

      m_matrices.resize(degree+1);
      m_reverse_matrices.resize(degree+1);

      for(unsigned int n=oldDegree; n<=degree; ++n)
        {
          m_matrices[n]=WRATHNew boost::multi_array<int, 2>(boost::extents[n+1][n+1]);

          boost::multi_array<int, 2> &current_matrix(*m_matrices[n]);
          const boost::multi_array<int, 2> &prev_matrix(*m_matrices[n-1]);

          for(unsigned int k=0; k<=n; ++k)
            {
              for(unsigned int i=0;i<=n;++i)
                {
                  current_matrix[k][i]=
                    get_entry(k, i, prev_matrix, n-1) 
                    + get_entry(k-1, i-1, prev_matrix, n-1) 
                    - get_entry(k-1, i, prev_matrix, n-1);
                }
            }

          m_reverse_matrices[n]=WRATHNew boost::multi_array<int, 2>(boost::extents[n+1][n+1]);
          boost::multi_array<int, 2> &reverse_matrix(*m_reverse_matrices[n]);

          for(unsigned int k=0;k<=n;++k)
            {
              for(unsigned int i=0;i<=n;++i)
                {
                  reverse_matrix[k][i]=current_matrix[k][n-i];
                }
            }
        }
    }

  ptr=(t==WRATHUtil::dont_reverse_control_points)?
    m_matrices[degree]:
    m_reverse_matrices[degree];

  WRATHUnlockMutex(m_mutex);

  return *ptr;
}


void
WRATHUtilPrivate::
add_solution_if_should(float t, 
                       std::vector<WRATHUtil::polynomial_solution_solve> &return_value, 
                       bool record_all)
{
  int mult;
  
  mult= (t>0.0f and t<1.0f)?1:-1;
  if(mult==1 or record_all)
    {
      return_value.push_back( WRATHUtil::polynomial_solution_solve()
                              .t(t)
                              .multiplicity(mult));
    }
}




const boost::multi_array<int, 2>&
WRATHUtilPrivate::
bernstein_conversion_matrix(int degree,
                            enum WRATHUtil::reverse_control_points_t t)
{
  return the_bernstein_matrices().matrix(degree, t);
}
