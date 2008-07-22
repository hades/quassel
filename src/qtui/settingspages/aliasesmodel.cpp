/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include "aliasesmodel.h"

#include <QDebug>
#include <QStringList>

#include "client.h"
#include "signalproxy.h"

AliasesModel::AliasesModel(QObject *parent)
  : QAbstractItemModel(parent),
    _configChanged(false)
{
//   _aliasManager.addAlias("a", "aa");
//   _aliasManager.addAlias("b", "bb");
//   _aliasManager.addAlias("c", "cc");
//   _aliasManager.addAlias("d", "dd");

  // we need this signal for future connects to reset the data;
  connect(Client::instance(), SIGNAL(connected()), this, SLOT(clientConnected()));
  if(Client::isConnected())
    clientConnected();
}

QVariant AliasesModel::data(const QModelIndex &index, int role) const {
  if(!index.isValid() || index.row() >= rowCount() || index.column() >= columnCount())
    return QVariant();

  switch(role) {
  case Qt::ToolTipRole:
    switch(index.column()) {
    case 0:
      return "<b>The shortcut for the alias</b><br />"
	"It can be used as a regular slash command.<br /><br />"
	"<b>Example:</b> \"foo\" can be used per /foo";
    case 1:
      return "<b>The string the shortcut will be expanded to</b><br />"
	"$i represenents the i'th parameter. $0 the whole string.<br />"
	"Multiple commands can be separated with semicolons<br /><br />"
	"<b>Example:</b> \"Test $1; Test $2; Test All $0\" will be expanded to three separate messages \"Test 1\", \"Test 2\" and \"Test All 1 2 3\" when called like /test 1 2 3";
    default:
      return QVariant();
    }
  case Qt::DisplayRole:
    switch(index.column()) {
    case 0:
      return aliasManager()[index.row()].name;
    case 1:
      return aliasManager()[index.row()].expansion;
    default:
      return QVariant();
    }
  default:
    return QVariant();
  }
}

bool AliasesModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if(!index.isValid() || index.row() >= rowCount() || index.column() >= columnCount() || role != Qt::EditRole)
    return false;

  QString newValue = value.toString();
  if(newValue.isEmpty())
    return false;
  
  switch(index.column()) {
  case 0:
    if(aliasManager().contains(newValue)) {
      return false;
    } else {
      cloneAliasManager()[index.row()].name = newValue;
      return true;
    }
  case 1:
    cloneAliasManager()[index.row()].expansion = newValue;
    return true;
  default:
    return false;
  }
}

void AliasesModel::newAlias() {
  QString newName("alias");
  int i = 0;
  AliasManager &manager = cloneAliasManager();
  while(manager.contains(newName)) {
    i++;
    newName = QString("alias%1").arg(i);
  }
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  manager.addAlias(newName, "Expansion");
  endInsertRows();
}

void AliasesModel::removeAlias(int index) {
  if(index < 0 || index >= rowCount())
    return;

  AliasManager &manager = cloneAliasManager();  
  beginRemoveRows(QModelIndex(), index, index);
  manager.removeAt(index);
  endRemoveRows();
}

Qt::ItemFlags AliasesModel::flags(const QModelIndex &index) const {
  if(!index.isValid()) {
    return Qt::ItemIsDropEnabled;
  } else {
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
  }
}


QVariant AliasesModel::headerData(int section, Qt::Orientation orientation, int role) const {
  QStringList header;
  header << tr("Alias")
	 << tr("Expansion");
  
  if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return header[section];

  return QVariant();
}

QModelIndex AliasesModel::index(int row, int column, const QModelIndex &parent) const {
  Q_UNUSED(parent);
  if(row >= rowCount() || column >= columnCount())
    return QModelIndex();

  return createIndex(row, column);
}


const AliasManager &AliasesModel::aliasManager() const {
  if(_configChanged)
    return _clonedAliasManager;
  else
    return _aliasManager;
}

AliasManager &AliasesModel::aliasManager() {
  if(_configChanged)
    return _clonedAliasManager;
  else
    return _aliasManager;
}

AliasManager &AliasesModel::cloneAliasManager() {
  if(!_configChanged) {
    _clonedAliasManager = _aliasManager;
    _configChanged = true;
    emit configChanged(true);
  }
  return _clonedAliasManager;
}

void AliasesModel::revert() {
  if(!_configChanged)
    return;
  
  _configChanged = false;
  emit configChanged(false);
  reset();
}

void AliasesModel::commit() {
  if(!_configChanged)
    return;

  _aliasManager.requestUpdate(_clonedAliasManager.toVariantMap());
  revert();
}  

void AliasesModel::initDone() {
  reset();
  emit modelReady();
}

void AliasesModel::clientConnected() {
  _aliasManager = AliasManager();
  Client::signalProxy()->synchronize(&_aliasManager);
  connect(&_aliasManager, SIGNAL(initDone()), this, SLOT(initDone()));
  connect(&_aliasManager, SIGNAL(updated(const QVariantMap &)), this, SLOT(revert()));
}