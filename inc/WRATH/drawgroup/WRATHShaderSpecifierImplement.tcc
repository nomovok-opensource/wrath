// -*- C++ -*-

/*! 
 * \file WRATHShaderSpecifierImplement.tcc
 * \brief file WRATHShaderSpecifierImplement.tcc
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


#if !defined(__WRATH_SHADER_SPECIFIER_HPP__) || defined(__WRATH_SHADER_SPECIFIER_TCC__)
#error "Direction inclusion of private header file WRATHShaderSpecifierImplement.tcc" 
#endif

#define __WRATH_SHADER_SPECIFIER_TCC__


template<typename T>
T*
WRATHShaderSpecifier::
fetch_two_pass_drawer(const WRATHItemDrawerFactory &factory,
                      const WRATHAttributePacker *attribute_packer,
                      int sub_drawer_id,
                      bool has_transparent_pass) const
{
  if(m_master!=this)
    {
      return m_master->fetch_two_pass_drawer<T>(factory, attribute_packer,
                                                sub_drawer_id, has_transparent_pass);
    }
  
  //ready_sub_shaders locks the mutex, so call it before locking.
  ready_sub_shaders();

  WRATHAutoLockMutex(m_mutex);

  two_pass_drawer_map::iterator iter;
  multi_pass_key_type K(has_transparent_pass, typeid(T), typeid(factory), attribute_packer, sub_drawer_id);

  iter=m_two_pass_drawers.find(K);
  if(iter!=m_two_pass_drawers.end())
    {
      WRATHassert(dynamic_cast<T*>(iter->second.first)!=NULL);
      return static_cast<T*>(iter->second.first);
    }
  else
    {
      T *new_2pass_drawer;
      vecN<WRATHItemDrawer*, 3> drawers(NULL, NULL, NULL);

      for(int i=0;i<3;++i)
        {
          if(i!=2 or has_transparent_pass)
            {
              drawers[i]=m_sub_shader_specifiers[i]->fetch_drawer(factory, attribute_packer, sub_drawer_id);
            }
        }

      std::pair<two_pass_drawer_map::iterator, bool> B;      
      B=m_two_pass_drawers.insert(two_pass_drawer_map::value_type(K, per_two_pass_drawer()));
      WRATHassert(B.second);

      per_two_pass_drawer &I(B.first->second);
      new_2pass_drawer=WRATHNew T(drawers[0], drawers[1], drawers[2]);
      I.first=new_2pass_drawer;
      I.second=I.first->connect_dtor(boost::bind(&WRATHShaderSpecifier::on_two_pass_draw_dtor, 
                                                 this, B.first) );

      return new_2pass_drawer;
    }
}
  
