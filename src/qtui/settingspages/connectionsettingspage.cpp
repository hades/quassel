/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#include "connectionsettingspage.h"

#include "client.h"
#include "networkconfig.h"

ConnectionSettingsPage::ConnectionSettingsPage(QWidget *parent)
  : SettingsPage(tr("Misc"), tr("Connection"), parent) {

  ui.setupUi(this);
  initAutoWidgets();

  connect(Client::instance(), SIGNAL(connected()), this, SLOT(clientConnected()));
  connect(Client::instance(), SIGNAL(disconnected()), this, SLOT(clientDisconnected()));

  setEnabled(false);
  if(Client::isConnected())
    clientConnected();
}

void ConnectionSettingsPage::clientConnected() {
  if(Client::networkConfig()->isInitialized())
    initDone();
  else
    connect(Client::networkConfig(), SIGNAL(initDone()), SLOT(initDone()));
}

void ConnectionSettingsPage::clientDisconnected() {
  setEnabled(false);
  setChangedState(false);
}

void ConnectionSettingsPage::initDone() {
  setEnabled(true);
}

bool ConnectionSettingsPage::hasDefaults() const {
  return true;
}

QVariant ConnectionSettingsPage::loadAutoWidgetValue(const QString &widgetName) {
  if(!isEnabled())
    return QVariant();
  NetworkConfig *config = Client::networkConfig();
  if(widgetName == "pingTimeoutEnabled")
    return config->pingTimeoutEnabled();
  if(widgetName == "pingInterval")
    return config->pingInterval();
  if(widgetName == "maxPingCount")
    return config->maxPingCount();
  if(widgetName == "autoWhoEnabled")
    return config->autoWhoEnabled();
  if(widgetName == "autoWhoInterval")
    return config->autoWhoInterval();
  if(widgetName == "autoWhoNickLimit")
    return config->autoWhoNickLimit();
  if(widgetName == "autoWhoDelay")
    return config->autoWhoDelay();

  return SettingsPage::loadAutoWidgetValue(widgetName);
}

void ConnectionSettingsPage::saveAutoWidgetValue(const QString &widgetName, const QVariant &value) {
  if(!isEnabled())
    return;
  NetworkConfig *config = Client::networkConfig();
  if(widgetName == "pingTimeoutEnabled")
    config->requestSetPingTimeoutEnabled(value.toBool());
  else if(widgetName == "pingInterval")
    config->requestSetPingInterval(value.toInt());
  else if(widgetName == "maxPingCount")
    config->requestSetMaxPingCount(value.toInt());
  else if(widgetName == "autoWhoEnabled")
    config->requestSetAutoWhoEnabled(value.toBool());
  else if(widgetName == "autoWhoInterval")
    config->requestSetAutoWhoInterval(value.toInt());
  else if(widgetName == "autoWhoNickLimit")
    config->requestSetAutoWhoNickLimit(value.toInt());
  else if(widgetName == "autoWhoDelay")
    config->requestSetAutoWhoDelay(value.toInt());

  else
    SettingsPage::saveAutoWidgetValue(widgetName, value);
}

