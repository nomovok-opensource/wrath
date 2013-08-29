/*
  Copyright (c) 2009, Kevin Rogovin All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
    * notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
    * copyright notice, this list of conditions and the following
    * disclaimer in the documentation and/or other materials provided
    * with the distribution.  Neither the name of the Kevin Rogovin or
    * kRogue Technologies  nor the names of its contributors may 
    * be used to endorse or promote products derived from this 
    * software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include "generic_command_line.hpp"

/////////////////////////////////////
// command_line_register methods
command_line_register::
~command_line_register()
{
  for(std::vector<command_line_argument*>::iterator 
        iter=m_children.begin(),
        end=m_children.end();
      iter!=end; ++iter)
    {
      command_line_argument *p(*iter);

      if(p!=NULL)
        {
          p->m_parent=NULL;
          p->m_location=-1;
        }
    }
}


void
command_line_register::
parse_command_line(int argc, char **argv)
{
  std::vector<std::string> arg_strings(argc);

  for(int i=0;i<argc; ++i)
    {
      arg_strings[i]=argv[i];
    }
  parse_command_line(arg_strings);
}

void
command_line_register::
parse_command_line(const std::vector<std::string> &argv)
{
  int location(0);
  int argc(argv.size());

  while(location<argc)
    {
      bool arg_taken(false);

      /*
        we do not use the iterator interface because an argument
        value may add new ones triggering a resize of m_children.
       */
      for(unsigned int i=0; !arg_taken and i<m_children.size(); ++i)
        {
           command_line_argument *p(m_children[i]);

           if(p!=NULL)
             {
               int incr;

               incr=p->check_arg(argv, location);
               if(incr>0)
                 {
                   location+=incr;
                   arg_taken=true;
                 }
             }
        }

      if(!arg_taken)
        {
          ++location;
        }
    }
}

void
command_line_register::
print_help(std::ostream &ostr) const
{
  for(std::vector<command_line_argument*>::const_iterator 
        iter=m_children.begin(),
        end=m_children.end();
      iter!=end; ++iter)
    {
       command_line_argument *p(*iter);

       if(p!=NULL)
         {
           ostr << " ";
           p->print_command_line_description(ostr);
         }
    }
}


void
command_line_register::
print_detailed_help(std::ostream &ostr) const
{
  for(std::vector<command_line_argument*>::const_iterator 
        iter=m_children.begin(),
        end=m_children.end();
      iter!=end; ++iter)
    {
       command_line_argument *p(*iter);

       if(p!=NULL)
         {
           p->print_detailed_description(ostr);
         }
    }
}

/////////////////////////////
//command_line_argument methods
command_line_argument::
~command_line_argument()
{
  if(m_parent!=NULL and m_location>=0)
    {
      m_parent->m_children[m_location]=NULL;
    }
}
