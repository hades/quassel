/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef CHATLINE_H_
#define CHATLINE_H_

#include <QGraphicsItem>

#include "chatlinemodel.h"
#include "chatitem.h"
#include "chatscene.h"

class ChatLine : public QGraphicsItem {
public:
  ChatLine(int row, QAbstractItemModel *model,
	   const qreal &width,
	   const qreal &timestampWidth, const qreal &senderWidth, const qreal &contentsWidth,
	   const QPointF &senderPos, const QPointF &contentsPos,
	   QGraphicsItem *parent = 0);

  virtual inline QRectF boundingRect () const { return QRectF(0, 0, _width, _height); }

  inline int row() { return _row; }
  inline void setRow(int row) { _row = row; }

  inline const QAbstractItemModel *model() const { return _model; }
  inline ChatScene *chatScene() const { return qobject_cast<ChatScene *>(scene()); }

  inline qreal width() const { return _width; }
  inline qreal height() const { return _height; }

  ChatItem &item(ChatLineModel::ColumnType);
  inline ChatItem &timestampItem() { return _timestampItem; }
  inline ChatItem &senderItem() { return _senderItem; }
  inline ContentsChatItem &contentsItem() { return _contentsItem; }

  virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
  enum { Type = ChatScene::ChatLineType };
  virtual inline int type() const { return Type; }

  void setFirstColumn(const qreal &timestampWidth, const qreal &senderWidth, const QPointF &senderPos);
  // setSecondColumn and setGeometryByWidth both also relocate the chatline.
  // the _bottom_ position is passed via linePos. linePos is updated to the top of the chatLine.
  void setSecondColumn(const qreal &senderWidth, const qreal &contentsWidth, const QPointF &contentsPos, qreal &linePos);
  void setGeometryByWidth(const qreal &width, const qreal &contentsWidth, qreal &linePos);

  void setSelected(bool selected, ChatLineModel::ColumnType minColumn = ChatLineModel::ContentsColumn);
  void setHighlighted(bool highlighted);

private:
  int _row;
  QAbstractItemModel *_model;
  ContentsChatItem _contentsItem;
  SenderChatItem _senderItem;
  TimestampChatItem _timestampItem;
  qreal _width, _height;

  enum { ItemMask = 0x3f,
	 Selected = 0x40,
	 Highlighted = 0x80
  };
  // _selection[1..0] ... Min Selected Column (See MessageModel::ColumnType)
  // _selection[5..2] ... reserved for new column types
  // _selection[6] ...... Selected
  // _selection[7] ...... Highlighted
  quint8 _selection;  // save space, so we put both the col and the flags into one byte
};

#endif
