/***************************************************************************
 *   Copyright (C) 2005/06 by the Quassel IRC Team                         *
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

#ifndef _TABCOMPLETER_H_
#define _TABCOMPLETER_H_

#include <QtCore>
#include <QObject>
#include <QLineEdit>

class TabCompleter : public QObject {
  Q_OBJECT
  
  public:
    TabCompleter(QLineEdit *l, QObject *parent = 0);
    void disable();
    void complete();
  
  public slots:
    void updateNickList(QStringList);
    void updateChannelList(QStringList);
    
  private:
    bool enabled;
    QString startOfLineSuffix;
    QLineEdit *lineEdit;
    QStringList completionTemplates;
    QStringList channelList;

    QStringList nickList;
    QStringList completionList;
    QStringList::Iterator nextCompletion;
    int lastCompletionLength;
        
    void buildCompletionList();
    
};

#endif