/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#ifndef _NICKVIEW_H_
#define _NICKVIEW_H_

#include <QTreeView>

class NickModel;
class FilteredNickModel;
class QSortFilterProxyModel;

class NickView : public QTreeView {
  Q_OBJECT

  public:
    NickView(QWidget *parent = 0);
    virtual ~NickView();

  protected:
    void rowsInserted(const QModelIndex &, int, int);

  public slots:
    void setModel(NickModel *model);

  private:
    QSortFilterProxyModel *filteredModel;

};

#endif