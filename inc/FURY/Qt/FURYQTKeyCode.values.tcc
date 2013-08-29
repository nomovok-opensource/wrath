/*  -*- C++ -*- */

/*! 
 * \file FURYQTKeyCode.values.tcc
 * \brief file FURYQTKeyCode.values.tcc
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


/*
   I am going to be flamed by someone eventually.
   Rather than manning up and making something
   that coverts from Qt keycodes to FURY keycodes
   I will simply copy-paste from the Qt header values.
*/

#if !defined(__FURY_QT_EVENT_HPP__) || defined(__FURY_QT_EVENT_CODE_VALUES_TCC__)
#error "Direction inclusion of private header file FURYQTKeyCodes.values.tcc"
#endif

#define __FURY_QT_EVENT_CODE_VALUES_TCC__
#define __FURY_QT_KEYCODE(X) FURYKey_##X

/*
  FURYQTKeyCodeMadness files are sed/gcc genarated files.
*/
#if defined(QT3_SUPPORT) && !defined(Q_MOC_RUN)
#include "FURYQTKeyCodeMadnessQt3defined.tcc"
#else
#include "FURYQTKeyCodeMadnessQt3Notdefined.tcc"
#endif

/*
  Special tactics: Also define FURYKey_Pageup and FURYKey_Pagedown
  as aliases for FURYKey_PageUp and FURYKey_PageDown, respectively
*/

        __FURY_QT_KEYCODE(Pageup)=Qt::Key_PageUp,
        __FURY_QT_KEYCODE(Pagedown)=Qt::Key_PageDown,


#undef __FURY_QT_KEYCODE
