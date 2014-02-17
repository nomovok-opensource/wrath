/*! 
 * \file text_item.hpp
 * \brief file text_item.hpp
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


#ifndef TEXT_ITEM_HPP
#define TEXT_ITEM_HPP



#include <QObject>
#include <QPainter>
#include "draw_item.hpp"
#include "transformation_node.hpp"


class TransformationNode;

class TextItem:public DrawItem
{
public:
  TextItem(QFont pfont, const QColor &pcolor, QString ptext,
           DrawList *drawer, 
           TransformationNode *transformation);
  ~TextItem();

  void
  setColor(const QColor &c);

  const QColor&
  color(void) const;

  void
  setText(const QString &ptext);

  const QString&
  text(void) const;

  virtual
  void
  drawItem(QPainter*);

private:
  QFont m_font;
  QString m_text;
  QColor m_color;
  TransformationNode *m_transformation;
};



#endif
