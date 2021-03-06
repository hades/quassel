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

#include <QMenu>
#include <QToolBar>

#include "toolbaractionprovider.h"

#include "iconloader.h"

ToolBarActionProvider::ToolBarActionProvider(QObject *parent)
: NetworkModelController(parent)
{
  registerAction(NetworkConnectAll, DesktopIcon("network-connect"), tr("Connect"))->setToolTip(tr("Connect to IRC"));
  registerAction(NetworkDisconnectAll, DesktopIcon("network-disconnect"), tr("Disconnect"))->setToolTip(tr("Disconnect from IRC"));

  registerAction(BufferPart, DesktopIcon("irc-close-channel"), tr("Part"))->setToolTip(tr("Leave currently selected channel"));
  registerAction(JoinChannel, DesktopIcon("irc-join-channel"), tr("Join"))->setToolTip(tr("Join a channel"));

  registerAction(NickQuery, DesktopIcon("mail-message-new"), tr("Query"))->setToolTip(tr("Start a private conversation")); // fix icon
  registerAction(NickWhois, DesktopIcon("im-user"), tr("Whois"))->setToolTip(tr("Request user information")); // fix icon

  registerAction(NickOp, DesktopIcon("irc-operator"), tr("Op"))->setToolTip(tr("Give operator privileges to user"));
  registerAction(NickDeop, DesktopIcon("irc-remove-operator"), tr("Deop"))->setToolTip(tr("Take operator privileges from user"));
  registerAction(NickVoice, DesktopIcon("irc-voice"), tr("Voice"))->setToolTip(tr("Give voice to user"));
  registerAction(NickDevoice, DesktopIcon("irc-unvoice"), tr("Devoice"))->setToolTip(tr("Take voice from user"));
  registerAction(NickKick, DesktopIcon("im-kick-user"), tr("Kick"))->setToolTip(tr("Remove user from channel"));
  registerAction(NickBan, DesktopIcon("im-ban-user"), tr("Ban"))->setToolTip(tr("Ban user from channel"));
  registerAction(NickKickBan, DesktopIcon("im-ban-kick-user"), tr("Kick/Ban"))->setToolTip(tr("Remove and ban user from channel"));

  _networksConnectMenu = new QMenu();
  _networksConnectMenu->setSeparatorsCollapsible(false);
  _networksConnectMenu->addSeparator();
  _networksConnectMenu->addAction(tr("Connect to all"));
  action(NetworkConnectAll)->setMenu(_networksConnectMenu);
  action(NetworkConnectAll)->setEnabled(false);

  _networksDisconnectMenu = new QMenu();
  _networksDisconnectMenu->setSeparatorsCollapsible(false);
  _networksDisconnectMenu->addSeparator();
  _networksDisconnectMenu->addAction(tr("Disconnect from all"));
  action(NetworkDisconnectAll)->setMenu(_networksDisconnectMenu);
  action(NetworkDisconnectAll)->setEnabled(false);

  connect(Client::instance(), SIGNAL(networkCreated(NetworkId)), SLOT(networkCreated(NetworkId)));
  connect(Client::instance(), SIGNAL(networkRemoved(NetworkId)), SLOT(networkRemoved(NetworkId)));

  updateStates();
}

ToolBarActionProvider::~ToolBarActionProvider() {

}

void ToolBarActionProvider::updateStates() {
  action(BufferPart)->setEnabled(_currentBuffer.isValid()
                              && _currentBuffer.data(NetworkModel::BufferTypeRole) == BufferInfo::ChannelBuffer
                              && _currentBuffer.data(NetworkModel::ItemActiveRole).toBool());

}

void ToolBarActionProvider::addActions(QToolBar *bar, ToolBarType type) {
  switch(type) {
    case MainToolBar:
      bar->addAction(action(NetworkConnectAll));
      bar->addAction(action(NetworkDisconnectAll));
      bar->addAction(action(JoinChannel));
      bar->addAction(action(BufferPart));
      break;
    case NickToolBar:
      bar->addAction(action(NickQuery));
      bar->addAction(action(NickWhois));
      bar->addSeparator();
      bar->addAction(action(NickOp));
      bar->addAction(action(NickDeop));
      bar->addAction(action(NickVoice));
      bar->addAction(action(NickDevoice));
      bar->addAction(action(NickKick));
      bar->addAction(action(NickBan));
      bar->addAction(action(NickKickBan));
      break;
    default:
      return;
  }
}

void ToolBarActionProvider::currentBufferChanged(const QModelIndex &index) {
  _currentBuffer = index;
  updateStates();
}

void ToolBarActionProvider::nickSelectionChanged(const QModelIndexList &indexList) {
  _selectedNicks = indexList;
  updateStates();
}

// override those to set indexes right
void ToolBarActionProvider::handleNetworkAction(ActionType type, QAction *action) {
  setIndexList(_currentBuffer);
  NetworkModelController::handleNetworkAction(type, action);
}

void ToolBarActionProvider::handleBufferAction(ActionType type, QAction *action) {
  setIndexList(_currentBuffer);
  NetworkModelController::handleBufferAction(type, action);
}

void ToolBarActionProvider::handleNickAction(ActionType type, QAction *action) {
  setIndexList(_selectedNicks);
  NetworkModelController::handleNickAction(type, action);
}

void ToolBarActionProvider::handleGeneralAction(ActionType type, QAction *action) {
  setIndexList(_currentBuffer);
  NetworkModelController::handleGeneralAction(type, action);
}

void ToolBarActionProvider::networkCreated(NetworkId id) {
  const Network *net = Client::network(id);
  Action *act = new Action(net->networkName(), this);
  _networkActions[id] = act;
  act->setObjectName(QString("NetworkAction-%1").arg(id.toInt()));
  act->setData(QVariant::fromValue<NetworkId>(id));
  connect(net, SIGNAL(updatedRemotely()), SLOT(networkUpdated()));
  connect(act, SIGNAL(triggered()), SLOT(connectOrDisconnectNet()));
  networkUpdated(net);
}

void ToolBarActionProvider::networkRemoved(NetworkId id) {
  Action *act = _networkActions.take(id);
  if(act)
    act->deleteLater();
}

void ToolBarActionProvider::networkUpdated(const Network *net) {
  if(!net)
    net = qobject_cast<const Network *>(sender());
  if(!net)
    return;
  Action *act = _networkActions.value(net->networkId());
  if(!act)
    return;

  _networksConnectMenu->removeAction(act);
  _networksDisconnectMenu->removeAction(act);

  QMenu *newMenu = net->connectionState() != Network::Disconnected ? _networksDisconnectMenu : _networksConnectMenu;
  act->setText(net->networkName());

  const int lastidx = newMenu->actions().count() - 2;
  QAction *beforeAction = newMenu->actions().at(lastidx);
  for(int i = 0; i < newMenu->actions().count() - 2; i++) {
    QAction *action = newMenu->actions().at(i);
    if(net->networkName().localeAwareCompare(action->text()) < 0) {
      beforeAction = action;
      break;
    }
  }
  newMenu->insertAction(beforeAction, act);

  action(NetworkConnectAll)->setEnabled(_networksConnectMenu->actions().count() > 2);
  action(NetworkDisconnectAll)->setEnabled(_networksDisconnectMenu->actions().count() > 2);
  action(JoinChannel)->setEnabled(_networksDisconnectMenu->actions().count() > 2);
}

void ToolBarActionProvider::connectOrDisconnectNet() {
  QAction *act = qobject_cast<QAction *>(sender());
  if(!act)
    return;
  const Network *net = Client::network(act->data().value<NetworkId>());
  if(!net)
    return;

  if(net->connectionState() == Network::Disconnected) net->requestConnect();
  else net->requestDisconnect();
}

//void ToolBarActionProvider::
