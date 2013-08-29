// -*- C++ -*-

/*! 
 * \file WRATHPolynomialImplement.tcc
 * \brief file WRATHPolynomialImplement.tcc
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


#if !defined(__WRATH_POLYNOMIAL_HPP__) || defined(__WRATH_POLYNOMIAL_IMPLEMENT_TCC__)
#error "Direction inclusion of private header file WRATHPolynomialImplement.tcc" 
#endif

#define __WRATH_POLYNOMIAL_IMPLEMENT_TCC__

#include "WRATHConfig.hpp"
#include <boost/multi_array.hpp>

namespace WRATHUtilPrivate
{
  void
  add_solution_if_should(float t, 
                         std::vector<WRATHUtil::polynomial_solution_solve> &return_value, 
                         bool record_all);

  
  const boost::multi_array<int, 2>&
  bernstein_conversion_matrix(int degree,
                              enum WRATHUtil::reverse_control_points_t t);

  template<typename T>
  class DoNothingFilter
  {
  public:
    const T&
    operator()(const T& v) const
    {
      return v;
    }

    template<unsigned int N>
    const T&
    operator()(const vecN<T,N>& v, int coord) const
    {
      return v[coord];
    }
  };
};

template<typename T, unsigned int N>
void
WRATHUtil::
generate_polynomial_from_bezier(const_c_array< vecN<T,N> > pts, 
                                vecN<std::vector<T>, N> &return_value,
                                enum reverse_control_points_t t)
{
  generate_polynomial_from_bezier(pts, return_value,
                                  typename WRATHUtilPrivate::DoNothingFilter<T>(),
                                  t);
}


template<typename T, unsigned int N, typename F>
void
WRATHUtil::
generate_polynomial_from_bezier(const_c_array< vecN<T,N> > pts, 
                                vecN<std::vector<T>, N> &return_value,
                                const F &filter,
                                enum reverse_control_points_t t)
{
  
  int n(pts.size());
  const boost::multi_array<int, 2> &M(WRATHUtilPrivate::bernstein_conversion_matrix(std::max(n-1,0),
                                                                                    t)); 

  /*
    Points is essentially the coefficients for a BernsteinPolynomial,
    The the polynomial in question is given by

    f(t) = pts[0]*(1-t)^n + pts[1]*t*(1-t)^(n-1) + pts[2]*t^2*t^(n-2) + ... 

         = f0 + f1*t + f2*t^2 + ... 

     getting (f0,f1,..,fn) is a linear operation in (pts[0],...,pts[n]),
     i.e. given by a matrix, that matrix is given by bernstein_conversion_matrix(n, t)
   */
  for(int k=0; k<n; ++k)
    {
      for(unsigned int coord=0; coord<N; ++coord)
        {
          return_value[coord].resize(pts.size());
          return_value[coord][k]=T(0);

          for(int i=0; i<n; ++i)
            {
              return_value[coord][k] += M[k][i]*filter(pts[i], coord);
            }
        }
    }
  
}

template<typename T>
void
WRATHUtil::
generate_polynomial_from_bezier(const_c_array<T> pts, 
                                std::vector<T> &return_value,
                                enum reverse_control_points_t t)
{
  generate_polynomial_from_bezier(pts, return_value,
                                  typename WRATHUtilPrivate::DoNothingFilter<T>(),
                                  t);
}



template<typename T,typename F>
void 
WRATHUtil::
generate_polynomial_from_bezier(const_c_array<T> pts, 
                                std::vector<T> &return_value,
                                const F &filter,
                                enum reverse_control_points_t t)
{
  return_value.resize(pts.size());
  int n(return_value.size());
  const boost::multi_array<int, 2> &M(WRATHUtilPrivate::bernstein_conversion_matrix(std::max(n-1,0), t)); 

  /*
    Points is essentially the coefficients for a BernsteinPolynomial,
    The the polynomial in question is given by

    f(t) = pts[0]*(1-t)^n + pts[1]*t*(1-t)^(n-1) + pts[2]*t^2*t^(n-2) + ... 

         = f0 + f1*t + f2*t^2 + ... 

     getting (f0,f1,..,fn) is a linear operation in (pts[0],...,pts[n]),
     i.e. given by a matrix, that matrix is given by bernstein_conversion_matrix(n)
   */
  for(int k=0; k<n; ++k)
    {
      return_value[k]=T(0);
      for(int i=0; i<n; ++i)
        {
          return_value[k] += M[k][i]*filter(pts[i]);
        }
    }
}




template<typename T>
void
WRATHUtil::
solve_linear(c_array<T> poly,
             std::vector<polynomial_solution_solve> &return_value,
             bool record_all)
{
  
  WRATHassert(poly.size()==2);
  int mult;

  if(poly[1]<T(0))
    {
      poly[1]=-poly[1];
      poly[0]=-poly[0];
    }
  
  mult=( poly[0]<T(0) and poly[0]+poly[1]>T(0) ) ? 1: -1;
  if(poly[1]!=T(0) and (mult==1 or record_all) )
    {
      float v;
      
      v=static_cast<float>(-poly[0])/static_cast<float>(poly[1]);
      return_value.push_back( polynomial_solution_solve()
                              .multiplicity(mult)
                              .t(v) );
    }
}

template<typename T>
void
WRATHUtil::
solve_quadratic(c_array<T> poly,
                std::vector<polynomial_solution_solve> &return_value,
                bool record_all)
{
  T desc;
  WRATHassert(poly.size()==3);

  if(poly[2]==0)
    {
      solve_linear(c_array<T>(poly.c_ptr(), 2), return_value, record_all);
      return;
    }

  //t=0 is ruled out
  if(poly[0]==T(0))
    {
      if(record_all)
        {
          return_value.push_back( polynomial_solution_solve()
                                  .multiplicity(-1)
                                  .t(0.0f));
          if(poly[1]==T(0))
            {
              --return_value.back().m_multiplicity;
              return;
            }
        }
      solve_linear( c_array<T>(poly.c_ptr()+1, 2), return_value, record_all);
      return;
    }

    
    T sum=poly[2]+poly[1]+poly[0];

    if(sum==T(0)) //t=1 is a solution, we throw it away.
      {
        //so poly(t)=at^2+ bt + -(a+b)
        // = (t-1)(at+a+b)
        vecN<T,2> v;

        v[1]=poly[2];
        v[0]=poly[1]+poly[2];

        if(record_all)
          {
            return_value.push_back( polynomial_solution_solve()
                                    .multiplicity(-1)
                                    .t(1.0f) );
            if(v[0]+v[1]==0)
              {
                --return_value.back().m_multiplicity;
                return;
              }
          }

        
        solve_linear( c_array<T>(v.c_ptr(), 2), return_value, record_all);
        return;
      }

    desc=poly[1]*poly[1] - 4*poly[0]*poly[2];
    if(desc<T(0))
      {
        //both roots not real.
        return;
      }

    //double root.
    if(desc==T(0))
      {
        vecN<int,2> v;
        int mult;

        v[0]=poly[1];
        v[1]=T(2)*poly[2];

        if(v[1]<T(0))
          {
            v[0]=-v[0];
            v[1]=-v[1];
          }

        mult=(v[0]<T(0) and v[0]+v[1]>T(0))?1:-1;

        if(mult==1 or record_all)
          {
            float t;

            t=static_cast<float>(-v[0])/static_cast<float>(v[1]);
            return_value.push_back( polynomial_solution_solve()
                                    .multiplicity(2*mult)
                                    .t(t));
          }
        return;
      }

    //make leading co-efficient positive:
    if(poly[2]<T(0))
      {
        poly[2]=-poly[2];
        poly[1]=-poly[1];
        poly[0]=-poly[0];
        sum=-sum;
      }

    T two_a_plus_b;
    bool plus_radical_root_want, negative_radical_root_want;

    two_a_plus_b=T(2)*poly[2]+poly[1];

    plus_radical_root_want=
      (two_a_plus_b>T(0) and sum>T(0)) // <1 condition
      and 
      (poly[0]<T(0) or poly[1]<T(0));  // >0 condition

    negative_radical_root_want=
      (two_a_plus_b>T(0) or sum<T(0))  //<1 condition
      and
      (poly[1]<T(0) and poly[0]>T(0));  // >0 condition


    if(plus_radical_root_want or negative_radical_root_want or record_all)
      { 

        float a, b, c, radical;
        a=static_cast<float>(poly[2]);
        b=static_cast<float>(poly[1]);
        c=static_cast<float>(poly[0]);

        WRATHunused(c);

        radical=sqrtf( static_cast<float>(desc) );
    
        float v0, v1;
        
        v0=(-b+radical)/(2.0f*a);
        v1=(-b-radical)/(2.0f*a);

        if(plus_radical_root_want or record_all)
          {
            return_value.push_back( polynomial_solution_solve()
                                    .multiplicity(plus_radical_root_want?1:-1)
                                    .t(v0));
          }
        
        if(negative_radical_root_want or record_all)
          {
            return_value.push_back( polynomial_solution_solve()
                                    .multiplicity(negative_radical_root_want?1:-1)
                                    .t(v1));
          }
      }
}


template<typename T>
void
WRATHUtil::
solve_cubic(c_array<T> poly,
            std::vector<polynomial_solution_solve> &return_value,
            bool record_all)
{
  WRATHassert(poly.size()==4);
  
  if(poly[3]==T(0))
    {
      solve_quadratic(c_array<T>(poly.c_ptr(), 3), return_value, record_all);
      return;
    }

  if(poly[0]==T(0))
    {
      //TODO: check if quadratic has root(s) at t=0.
      solve_quadratic( c_array<T>(poly.c_ptr()+1, 3), return_value, record_all);
      
      

      if(record_all)
        {
          return_value.push_back( polynomial_solution_solve()
                                  .multiplicity(-1)
                                  .t(0.0f));
        }
      
      return;
    }
  
  if(poly[3]+poly[2]+poly[1]+poly[0]==T(0))
    {
      //icky t=1 is valid solution, generate the qudratic..
      vecN<int,3> v;
      
      //TODO: check if quadratic has root(s) at t=1.
      if(record_all)
        {
          return_value.push_back( polynomial_solution_solve()
                                  .multiplicity(-1)
                                  .t(1.0f) );
        }
      
      v[0]=poly[3]+poly[2]+poly[1];
      v[1]=poly[3]+poly[2];
      v[2]=poly[3];
      solve_quadratic(c_array<T>(v.c_ptr(), 3), return_value, record_all);
      return;
    }
  
  
  float L, p, q, C, temp, dd;
  vecN<float, 3> a;
  
  L=static_cast<float>(poly[3]);
  a[2]=static_cast<float>(poly[2])/L;
  a[1]=static_cast<float>(poly[1])/L;
  a[0]=static_cast<float>(poly[0])/L;
  
  p=(3.0f*a[1] - a[2]*a[2])/3.0f;
  q=(9.0f*a[1]*a[2]-27*a[0]-2*a[2]*a[2]*a[2])/27.0f;
  
  dd=a[2]/3.0f;
  
  if(T(3)*poly[1]*poly[3] == poly[2]*poly[2] )
    {
      WRATHUtilPrivate::add_solution_if_should(-dd + cbrtf(q), return_value, record_all);
      return;
    }
  
  
  temp=sqrtf(3.0f/fabs(p));
  C=0.5f*q*temp*temp*temp;
  
  temp=1.0f/temp;
  temp*=2.0f;
  
  if(p>0.0f)
    {
      float v0, tau;
      
      tau=cbrtf(C+sqrtf(1.0f+C*C));
      v0=temp*(tau - 1.0f/tau)*0.5f - dd;
      //Question: which is faster on device: using cbrtf and sqrtf or using sinhf and asinhf?
      //v0=temp*sinhf( asinhf(C)/3.0f) -  dd;
      WRATHUtilPrivate::add_solution_if_should(v0, return_value, record_all);
    }
  else
    {
      if(C>=1.0f)
        {
          float v0, tau;
          tau=cbrtf(C+ sqrtf(C*C - 1.0f));
          v0=temp*( tau + 1.0/tau)*0.5f - dd;
          //Question: which is faster on device: using cbrtf and sqrtf or using coshf and acoshf?
          //v0=temp*coshf( acoshf(C)/3.0f) - dd;
          
          WRATHUtilPrivate::add_solution_if_should(v0, return_value, record_all);
        }
      else if(C<=-1.0f)
        {
          float v0, tau;
          tau=cbrtf(-C+ sqrtf(C*C - 1.0f));
          v0=-temp*( tau + 1.0/tau)*0.5f - dd;
          //Question: which is faster on device: using cbrtf and sqrtf or using coshf and acoshf?
          //v0= -temp*coshf( acoshf(-C)/3.0f) - dd;
          
          WRATHUtilPrivate::add_solution_if_should(v0, return_value, record_all);
        }
      else
        {
          float v0, v1, v2, theta;
          
          //todo: replace with using cubrt, etc.
          //not clear if that would be faster or slower though.
          theta=acosf(C);
          v0=temp*cosf( (theta          )/3.0f) - dd;
          v1=temp*cosf( (theta+2.0f*M_PI)/3.0f) - dd;
          v2=temp*cosf( (theta+4.0f*M_PI)/3.0f) - dd;
          
          WRATHUtilPrivate::add_solution_if_should(v0, return_value, record_all);
          WRATHUtilPrivate::add_solution_if_should(v1, return_value, record_all);
          WRATHUtilPrivate::add_solution_if_should(v2, return_value, record_all);
        }
    }
}

template<typename T>
void
WRATHUtil::
solve_polynomial(c_array<T> poly,
                 std::vector<polynomial_solution_solve> &return_value,
                 bool record_all)
{
  if(poly.size()<=1)
      {
        return;
      }

  WRATHassert(poly.size()==2 or poly.size()==3 or poly.size()==4);

  switch(poly.size())
    {
    case 2:
      solve_linear(poly, return_value, record_all);
      break;
      
    case 3:
      solve_quadratic(poly, return_value, record_all);     
      break;

    case 4:
      solve_cubic(poly, return_value, record_all);
      break;
      
    default:
      WRATHwarning("Invalid degree, polynomial has "
                   << poly.size() << " coefficients, i.e has degree "
                   << poly.size() - 1);
    }
}

///////////////////////////////////////////////
//WRATHUtil::BernsteinPolynomial methods
template<typename T>
WRATHUtil::BernsteinPolynomial<T>
WRATHUtil::BernsteinPolynomial<T>::
compute_derivative(void) const
{
  if(m_coefficients.empty())
    {
      const_c_array<T> nothing(NULL, 0);

      return BernsteinPolynomial<T>(nothing);
    }

  std::vector<T> new_coeffs(m_coefficients.size()-1, T(0));

  for(int k=1, endk=m_coefficients.size()-1; k<endk; ++k)
    {
      int i0, i1;

      i0=k-1;
      i1=k;

      new_coeffs[i0]+=m_coefficients[k];
      new_coeffs[i1]-=m_coefficients[k];
    }

  new_coeffs[0]-=m_coefficients[0];
  new_coeffs.back()+=m_coefficients.back();

  
  T dd(degree());
  for(typename std::vector<T>::iterator iter=new_coeffs.begin(),
        end=new_coeffs.end(); iter!=end; ++iter)
    {
      (*iter) *= dd;
    }

  return BernsteinPolynomial<T>(const_c_array<T>(new_coeffs));
}


template<typename T>
void
WRATHUtil::BernsteinPolynomial<T>::
generate_polynomial(std::vector<T> &return_value) const
{
  generate_polynomial_from_bezier<T>(m_coefficients, return_value);
}


template<typename T>
template<typename F>
void
WRATHUtil::BernsteinPolynomial<T>::
prepare_workroom(std::vector<F> &work_room, int sz, F t)
{
  F one_minus_t( F(1) - t);
  F pow_wow(1);

  work_room.resize( std::max( static_cast<int>(work_room.size()), sz) );
  
  for(int i=0; i<sz; ++i, pow_wow*=one_minus_t)
    {
      work_room[i]=pow_wow;
    }
}


template<typename T>
template<typename F>
T
WRATHUtil::BernsteinPolynomial<T>::
evaluate(F t, std::vector<F> &one_minus_t_power) const
{
  T return_value(0);
  F t_power(1);

  prepare_workroom(one_minus_t_power, m_coefficients.size(), t);


  for(int i=0, endi=m_coefficients.size(), j=endi-1; i<endi; ++i, t_power*=t, --j)
    {
      F B;

      B=F(m_bi_coeffs[i])*t_power*one_minus_t_power[j];
      return_value+= B*m_coefficients[i];

    }

  return return_value;
  
}


template<typename T>
template<typename F>
void
WRATHUtil::BernsteinPolynomial<T>::
multiple_evaluate(F t,
                  const_c_array<BernsteinPolynomial> polys,
                  c_array<T> results,
                  std::vector<F> &one_minus_t_power)
{
  unsigned int M;
  int biggest_degree(0);

  M=std::min(results.size(), polys.size());

  for(unsigned int i=0; i<M; ++i)
    {
      biggest_degree=std::max(biggest_degree, polys[i].degree());
    }

  prepare_workroom(one_minus_t_power, biggest_degree+1, t);
  for(unsigned int m=0; m<M; ++m)
    {
      F t_power(1);

      results[m]=T(0);
      for(int i=0, endi=polys[m].m_coefficients.size(); i<endi; ++i, t_power*=t)
        {
          F B;
          
          B=F(polys[m].m_bi_coeffs[i])*t_power*one_minus_t_power[endi-1-i];
          results[m]+= B*polys[m].m_coefficients[i];
        }
    }
                             
}

#if 1

template<typename T>
template<typename F>
void
WRATHUtil::BernsteinPolynomial<T>::
split_curve(BernsteinPolynomial &out0,
            BernsteinPolynomial &out1,
            F t)
{
  /*
    use De Casteljau's algorithm to get the job done:
   */
  WRATHassert(out0.degree()==degree());
  WRATHassert(out1.degree()==degree());

  /*
    Messy in place bits:
        without doing it in place we would essentially do this:
        
        1) copy this->control_points() to work_room0
        2) run the algorithm:
          for(unsigned int j=1, d=degree(); j<=d; ++j)
          {
            for(int i=0; i<=d-j; ++i)
            {
              (*current)[i] = (*last)[i]*one_minus_t + (*last)[i+1]*t;
            }

            out0.control_point(j)=(*current)[0];
            out1.control_point(d-j)=(*current)[d-j];

            std::swap(current, last);
          }

    doing it in place is ickier.
    work_room0 is going to be stored in out0 and
    work_room1 is going to be stored in out1 

    the trick is that we want to NOT use the front in out0 and 
    not use the back in out1 as we fill them up. This is the
    purpose of s and r c_array's. The s's are for before
    the pointer shift, the r's for after
   */
  F one_minus_t(F(1)-t);
  vecN<c_array<T>, 2> r, s;

  std::copy(control_points().begin(), control_points().end(), out0.control_points().begin()); 

  s[0]=out0.control_points();
  s[1]=out1.control_points();  
  r[0]=s[0].sub_array(1, s[0].size() - 1);
  r[1]=s[1].sub_array(0, s[1].size() - 1);

  out0.control_point(0)=control_point(0);
  out1.control_point(degree())=control_point(degree());

  int last(0);
  for(unsigned int j=1, d=degree(); j<=d; ++j, last=1-last)
    {
      int current;

      current=1-last;
      for(unsigned int i=0; i<=d-j; ++i)
        {
          r[current][i] = s[last][i]*one_minus_t + s[last][i+1]*t;
        }

      
      out0.control_point(j)=r[current][0];
      out1.control_point(d-j)=r[current][d-j];

      /*
        save the pointer position before changing..
       */
      s[0]=r[0];
      s[1]=r[1];

      /*
        we want to pop the front element off of the 
        work room to avoid overwriting it
       */
      r[0]=r[0].sub_array(1, r[0].size() - 1);

      /*
        we want to pop the back element of work
        room since we want to save that value
       */
      r[1]=r[1].sub_array(0, r[1].size() - 1);

    }

}

#else

template<typename T>
void
WRATHUtil::BernsteinPolynomial<T>::
split_curve(BernsteinPolynomial &out0,
            BernsteinPolynomial &out1,
            float t)
{
  
  float one_minus_t(1.0f-t);
  std::vector<T> work_room0(m_coefficients), work_room1;
  work_room1.resize(degree()+1);

  std::vector<T> *current(&work_room1), *last(&work_room0);

  out0.control_point(0)=control_point(0);
  out1.control_point(degree())=control_point(degree());

  for(unsigned int j=1, d=degree(); j<=d; ++j)
    {
      for(unsigned int i=0; i<=d-j; ++i)
        {
          (*current)[i] = (*last)[i]*one_minus_t + (*last)[i+1]*t;
        }

      out0.control_point(j)=(*current)[0];
      out1.control_point(d-j)=(*current)[d-j];

      std::swap(current, last);
    }
}

#endif

