/*! 
 * \file WRATHGLExtensionList.hpp
 * \brief file WRATHGLExtensionList.hpp
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




#ifndef WRATH_HEADER_GL_EXTENSION_LIST_HPP_
#define WRATH_HEADER_GL_EXTENSION_LIST_HPP_

#include "WRATHConfig.hpp"
#include <set>
#include <string>

/*! \addtogroup GLUtility
 * @{
 */


/*!\class WRATHGLExtensionList
  A WRATHGLExtensionList is a container for holding
  a list of the GL extenions that a GL implementation
  supports. 
 */
class WRATHGLExtensionList
{
public:

  /*!\fn WRATHGLExtensionList(void)
    Default ctor, requires a valid GL context
    to be current in the calling thread.
   */
  WRATHGLExtensionList(void);

  /*!\fn WRATHGLExtensionList(const WRATHGLExtensionList&)
    Copy ctor, does NOT require a valid
    GL context to be current in the calling 
    thread.
   */
  WRATHGLExtensionList(const WRATHGLExtensionList &obj):
    m_extensions(obj.m_extensions)
  {}

  /*!\fn bool extension_supported
    Returns true if and only if the named
    string is a GL extension that the GL
    context from which this WRATHGLExtensionList
    was created supports.
    \param pname name of extension to query.
   */
  bool
  extension_supported(const std::string &pname) const
  {
    return m_extensions.find(pname)!=m_extensions.end();
  }

  /*!\fn const std::set<std::string>& extension_list
    Returns the GL extensions as a set of strings.
   */
  const std::set<std::string>&
  extension_list(void) const
  {
    return m_extensions;
  }

private:
  std::set<std::string> m_extensions;
};

/*! @} */

#endif
