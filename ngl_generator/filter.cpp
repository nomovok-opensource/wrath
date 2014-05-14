/*! 
 * \file filter.cpp
 * \brief file filter.cpp
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



#include <iostream>
#include <fstream>
#include <set>
#include <string>

bool
use_function_pointer(const std::string &filename)
{
  return true;

  /*
    Bah humbug: just require that ngl_backend implementation
    can handle functions that are core in GLES2.so The issue
    is that GLES3 is the same .so as GLES2, so to get its
    function pointers means one needs to use eglGetProcAddress,
    so telling by the filename will not work. The correct thing
    is that we have a list of all the GLES2 core functions
    and if it is not one of those, then do the eglGetProcAdress.
    However, dlopen and dlsym can be used instead to get the
    function pointers even if the function is core GLES2,
    so we just always fetch the fnction pointer.

  return filename.find("gl2.h")==std::string::npos;
  */
}



int 
main(int argc, char **argv)
{
  /*read each file passed and output them to stdout after filtering */

  std::set<std::string> fileNames;

  for(int i=1;i<argc;++i)
    {
      fileNames.insert(argv[i]);
    }

  for(std::set<std::string>::const_iterator i=fileNames.begin(); i!=fileNames.end(); ++i)
    {
      std::ifstream inFile;

      inFile.open(i->c_str());
      if(inFile)
        {
          int parenCount;
          bool last_char_is_white;

          if(use_function_pointer(*i))
            {
              std::cout << "\nFUNCTIONPOINTERMODE\n";
            }
          else
            {
              std::cout << "\nNONFUNCTIONPOINTERMODE\n";
            }

          parenCount=0; last_char_is_white=false;
          while(inFile)
            {
              char ch;
              

              inFile.get(ch);
              if(ch=='(')
                {
                  ++parenCount;
                  std::cout << ch;
                }
              else if(ch==')')
                {
                  --parenCount;
                  std::cout << ch;
                }
              else if(ch=='\n' and parenCount>0)
                {
                  
                }
              else if(last_char_is_white and isspace(ch) and ch!='\n') 
                {
                  
                }
              else
                {
                  std::cout << ch;
                  last_char_is_white=isspace(ch) and ch!='\n';
                }
            }
          inFile.close();
        }
    }

  return 0;
}
