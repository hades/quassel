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

#ifndef SESSIONTHREAD_H
#define SESSIONTHREAD_H

#include <QMutex>
#include <QThread>

#include "types.h"

class CoreSession;
class QIODevice;
class SignalProxy;

class SessionThread : public QThread {
  Q_OBJECT

public:
  SessionThread(UserId user, bool restoreState, QObject *parent = 0);
  ~SessionThread();

  void run();

  CoreSession *session();
  UserId user();

public slots:
  void addClient(QObject *peer);

private slots:
  void setSessionInitialized();

signals:
  void initialized();
  void shutdown();

  void addRemoteClient(QIODevice *);
  void addInternalClient(SignalProxy *);

private:
  CoreSession *_session;
  UserId _user;
  QList<QObject *> clientQueue;
  bool _sessionInitialized;
  bool _restoreState;

  bool isSessionInitialized();
  void addClientToSession(QObject *peer);
  void addRemoteClientToSession(QIODevice *socket);
  void addInternalClientToSession(SignalProxy *proxy);
};

#endif
