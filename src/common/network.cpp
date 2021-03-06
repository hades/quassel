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
#include <QSettings>
#include <QTextCodec>

#include "network.h"
#include "quassel.h"

QTextCodec *Network::_defaultCodecForServer = 0;
QTextCodec *Network::_defaultCodecForEncoding = 0;
QTextCodec *Network::_defaultCodecForDecoding = 0;
QString Network::_networksIniPath = QString();

// ====================
//  Public:
// ====================
INIT_SYNCABLE_OBJECT(Network)
Network::Network(const NetworkId &networkid, QObject *parent)
  : SyncableObject(parent),
    _proxy(0),
    _networkId(networkid),
    _identity(0),
    _myNick(QString()),
    _latency(0),
    _networkName(QString("<not initialized>")),
    _currentServer(QString()),
    _connected(false),
    _connectionState(Disconnected),
    _prefixes(QString()),
    _prefixModes(QString()),
    _useRandomServer(false),
    _useAutoIdentify(false),
    _useAutoReconnect(false),
    _autoReconnectInterval(60),
    _autoReconnectRetries(10),
    _unlimitedReconnectRetries(false),
    _codecForServer(0),
    _codecForEncoding(0),
    _codecForDecoding(0),
    _autoAwayActive(false)
{
  setObjectName(QString::number(networkid.toInt()));
}

Network::~Network() {
  emit aboutToBeDestroyed();
}

bool Network::isChannelName(const QString &channelname) const {
  if(channelname.isEmpty())
    return false;

  if(supports("CHANTYPES"))
    return support("CHANTYPES").contains(channelname[0]);
  else
    return QString("#&!+").contains(channelname[0]);
}

NetworkInfo Network::networkInfo() const {
  NetworkInfo info;
  info.networkName = networkName();
  info.networkId = networkId();
  info.identity = identity();
  info.codecForServer = codecForServer();
  info.codecForEncoding = codecForEncoding();
  info.codecForDecoding = codecForDecoding();
  info.serverList = serverList();
  info.useRandomServer = useRandomServer();
  info.perform = perform();
  info.useAutoIdentify = useAutoIdentify();
  info.autoIdentifyService = autoIdentifyService();
  info.autoIdentifyPassword = autoIdentifyPassword();
  info.useAutoReconnect = useAutoReconnect();
  info.autoReconnectInterval = autoReconnectInterval();
  info.autoReconnectRetries = autoReconnectRetries();
  info.unlimitedReconnectRetries = unlimitedReconnectRetries();
  info.rejoinChannels = rejoinChannels();
  return info;
}

void Network::setNetworkInfo(const NetworkInfo &info) {
  // we don't set our ID!
  if(!info.networkName.isEmpty() && info.networkName != networkName()) setNetworkName(info.networkName);
  if(info.identity > 0 && info.identity != identity()) setIdentity(info.identity);
  if(info.codecForServer != codecForServer()) setCodecForServer(QTextCodec::codecForName(info.codecForServer));
  if(info.codecForEncoding != codecForEncoding()) setCodecForEncoding(QTextCodec::codecForName(info.codecForEncoding));
  if(info.codecForDecoding != codecForDecoding()) setCodecForDecoding(QTextCodec::codecForName(info.codecForDecoding));
  if(info.serverList.count()) setServerList(toVariantList(info.serverList)); // FIXME compare components
  if(info.useRandomServer != useRandomServer()) setUseRandomServer(info.useRandomServer);
  if(info.perform != perform()) setPerform(info.perform);
  if(info.useAutoIdentify != useAutoIdentify()) setUseAutoIdentify(info.useAutoIdentify);
  if(info.autoIdentifyService != autoIdentifyService()) setAutoIdentifyService(info.autoIdentifyService);
  if(info.autoIdentifyPassword != autoIdentifyPassword()) setAutoIdentifyPassword(info.autoIdentifyPassword);
  if(info.useAutoReconnect != useAutoReconnect()) setUseAutoReconnect(info.useAutoReconnect);
  if(info.autoReconnectInterval != autoReconnectInterval()) setAutoReconnectInterval(info.autoReconnectInterval);
  if(info.autoReconnectRetries != autoReconnectRetries()) setAutoReconnectRetries(info.autoReconnectRetries);
  if(info.unlimitedReconnectRetries != unlimitedReconnectRetries()) setUnlimitedReconnectRetries(info.unlimitedReconnectRetries);
  if(info.rejoinChannels != rejoinChannels()) setRejoinChannels(info.rejoinChannels);
}

QString Network::prefixToMode(const QString &prefix) {
  if(prefixes().contains(prefix))
    return QString(prefixModes()[prefixes().indexOf(prefix)]);
  else
    return QString();
}

QString Network::modeToPrefix(const QString &mode) {
  if(prefixModes().contains(mode))
    return QString(prefixes()[prefixModes().indexOf(mode)]);
  else
    return QString();
}

QStringList Network::nicks() const {
  // we don't use _ircUsers.keys() since the keys may be
  // not up to date after a nick change
  QStringList nicks;
  foreach(IrcUser *ircuser, _ircUsers.values()) {
    nicks << ircuser->nick();
  }
  return nicks;
}

QString Network::prefixes() {
  if(_prefixes.isNull())
    determinePrefixes();

  return _prefixes;
}

QString Network::prefixModes() {
  if(_prefixModes.isNull())
    determinePrefixes();

  return _prefixModes;
}

// example Unreal IRCD: CHANMODES=beI,kfL,lj,psmntirRcOAQKVCuzNSMTG
Network::ChannelModeType Network::channelModeType(const QString &mode) {
  if(mode.isEmpty())
    return NOT_A_CHANMODE;

  QString chanmodes = support("CHANMODES");
  if(chanmodes.isEmpty())
    return NOT_A_CHANMODE;

  ChannelModeType modeType = A_CHANMODE;
  for(int i = 0; i < chanmodes.count(); i++) {
    if(chanmodes[i] == mode[0])
      break;
    else if(chanmodes[i] == ',')
      modeType = (ChannelModeType)(modeType << 1);
  }
  if(modeType > D_CHANMODE) {
    qWarning() << "Network" << networkId() << "supplied invalid CHANMODES:" << chanmodes;
    modeType = NOT_A_CHANMODE;
  }
  return modeType;
}

QString Network::support(const QString &param) const {
  QString support_ = param.toUpper();
  if(_supports.contains(support_))
    return _supports[support_];
  else
    return QString();
}

IrcUser *Network::newIrcUser(const QString &hostmask, const QVariantMap &initData) {
  QString nick(nickFromMask(hostmask).toLower());
  if(!_ircUsers.contains(nick)) {
    IrcUser *ircuser = new IrcUser(hostmask, this);
    if(!initData.isEmpty()) {
      ircuser->fromVariantMap(initData);
      ircuser->setInitialized();
    }

    if(proxy())
      proxy()->synchronize(ircuser);
    else
      qWarning() << "unable to synchronize new IrcUser" << hostmask << "forgot to call Network::setProxy(SignalProxy *)?";

    connect(ircuser, SIGNAL(nickSet(QString)), this, SLOT(ircUserNickChanged(QString)));

    _ircUsers[nick] = ircuser;

    SYNC_OTHER(addIrcUser, ARG(hostmask))
    // emit ircUserAdded(hostmask);
    emit ircUserAdded(ircuser);
  }

  return _ircUsers[nick];
}

IrcUser *Network::ircUser(QString nickname) const {
  nickname = nickname.toLower();
  if(_ircUsers.contains(nickname))
    return _ircUsers[nickname];
  else
    return 0;
}

void Network::removeIrcUser(IrcUser *ircuser) {
  QString nick = _ircUsers.key(ircuser);
  if(nick.isNull())
    return;

  _ircUsers.remove(nick);
  disconnect(ircuser, 0, this, 0);
  ircuser->deleteLater();
}

void Network::removeIrcChannel(IrcChannel *channel) {
  QString chanName = _ircChannels.key(channel);
  if(chanName.isNull())
    return;

  _ircChannels.remove(chanName);
  disconnect(channel, 0, this, 0);
  channel->deleteLater();
}

void Network::removeChansAndUsers() {
  QList<IrcUser *> users = ircUsers();
  _ircUsers.clear();
  QList<IrcChannel *> channels = ircChannels();
  _ircChannels.clear();

  foreach(IrcChannel *channel, channels) {
    proxy()->detachObject(channel);
    disconnect(channel, 0, this, 0);
  }
  foreach(IrcUser *user, users) {
    proxy()->detachObject(user);
    disconnect(user, 0, this, 0);
  }

  // the second loop is needed because quit can have sideffects
  foreach(IrcUser *user, users) {
    user->quit();
  }

  qDeleteAll(users);
  qDeleteAll(channels);
}

IrcChannel *Network::newIrcChannel(const QString &channelname, const QVariantMap &initData) {
  if(!_ircChannels.contains(channelname.toLower())) {
    IrcChannel *channel = ircChannelFactory(channelname);
    if(!initData.isEmpty()) {
      channel->fromVariantMap(initData);
      channel->setInitialized();
    }

    if(proxy())
      proxy()->synchronize(channel);
    else
      qWarning() << "unable to synchronize new IrcChannel" << channelname << "forgot to call Network::setProxy(SignalProxy *)?";

    _ircChannels[channelname.toLower()] = channel;

    SYNC_OTHER(addIrcChannel, ARG(channelname))
    // emit ircChannelAdded(channelname);
    emit ircChannelAdded(channel);
  }
  return _ircChannels[channelname.toLower()];
}

IrcChannel *Network::ircChannel(QString channelname) const {
  channelname = channelname.toLower();
  if(_ircChannels.contains(channelname))
    return _ircChannels[channelname];
  else
    return 0;
}

QByteArray Network::defaultCodecForServer() {
  if(_defaultCodecForServer)
    return _defaultCodecForServer->name();
  return QByteArray();
}

void Network::setDefaultCodecForServer(const QByteArray &name) {
  _defaultCodecForServer = QTextCodec::codecForName(name);
}

QByteArray Network::defaultCodecForEncoding() {
  if(_defaultCodecForEncoding)
    return _defaultCodecForEncoding->name();
  return QByteArray();
}

void Network::setDefaultCodecForEncoding(const QByteArray &name) {
  _defaultCodecForEncoding = QTextCodec::codecForName(name);
}

QByteArray Network::defaultCodecForDecoding() {
  if(_defaultCodecForDecoding)
    return _defaultCodecForDecoding->name();
  return QByteArray();
}

void Network::setDefaultCodecForDecoding(const QByteArray &name) {
  _defaultCodecForDecoding = QTextCodec::codecForName(name);
}

QByteArray Network::codecForServer() const {
  if(_codecForServer)
    return _codecForServer->name();
  return QByteArray();
}

void Network::setCodecForServer(const QByteArray &name) {
  setCodecForServer(QTextCodec::codecForName(name));
}

void Network::setCodecForServer(QTextCodec *codec) {
  _codecForServer = codec;
  QByteArray codecName = codecForServer();
  SYNC_OTHER(setCodecForServer, ARG(codecName))
  emit configChanged();
}

QByteArray Network::codecForEncoding() const {
  if(_codecForEncoding)
    return _codecForEncoding->name();
  return QByteArray();
}

void Network::setCodecForEncoding(const QByteArray &name) {
  setCodecForEncoding(QTextCodec::codecForName(name));
}

void Network::setCodecForEncoding(QTextCodec *codec) {
  _codecForEncoding = codec;
  QByteArray codecName = codecForEncoding();
  SYNC_OTHER(setCodecForEncoding, ARG(codecName))
  emit configChanged();
}

QByteArray Network::codecForDecoding() const {
  if(_codecForDecoding)
    return _codecForDecoding->name();
  else return QByteArray();
}

void Network::setCodecForDecoding(const QByteArray &name) {
  setCodecForDecoding(QTextCodec::codecForName(name));
}

void Network::setCodecForDecoding(QTextCodec *codec) {
  _codecForDecoding = codec;
  QByteArray codecName = codecForDecoding();
  SYNC_OTHER(setCodecForDecoding, ARG(codecName))
  emit configChanged();
}

// FIXME use server encoding if appropriate
QString Network::decodeString(const QByteArray &text) const {
  if(_codecForDecoding)
    return ::decodeString(text, _codecForDecoding);
  else return ::decodeString(text, _defaultCodecForDecoding);
}

QByteArray Network::encodeString(const QString &string) const {
  if(_codecForEncoding) {
    return _codecForEncoding->fromUnicode(string);
  }
  if(_defaultCodecForEncoding) {
    return _defaultCodecForEncoding->fromUnicode(string);
  }
  return string.toAscii();
}

QString Network::decodeServerString(const QByteArray &text) const {
  if(_codecForServer)
    return ::decodeString(text, _codecForServer);
  else
    return ::decodeString(text, _defaultCodecForServer);
}

QByteArray Network::encodeServerString(const QString &string) const {
  if(_codecForServer) {
    return _codecForServer->fromUnicode(string);
  }
  if(_defaultCodecForServer) {
    return _defaultCodecForServer->fromUnicode(string);
  }
  return string.toAscii();
}

/*** Handle networks.ini ***/

QStringList Network::presetNetworks(bool onlyDefault) {
  // lazily find the file, make sure to not call one of the other preset functions first (they'll fail else)
  if(_networksIniPath.isNull()) {
    _networksIniPath = Quassel::findDataFilePath("networks.ini");
    if(_networksIniPath.isNull()) {
      _networksIniPath = ""; // now we won't check again, as it's not null anymore
      return QStringList();
    }
  }
  if(!_networksIniPath.isEmpty()) {
    QSettings s(_networksIniPath, QSettings::IniFormat);
    QStringList networks = s.childGroups();
    if(!networks.isEmpty()) {
      // we sort the list case-insensitive
      QMap<QString, QString> sorted;
      foreach(QString net, networks) {
        if(onlyDefault && !s.value(QString("%1/Default").arg(net)).toBool())
          continue;
        sorted[net.toLower()] = net;
      }
      return sorted.values();
    }
  }
  return QStringList();
}

QStringList Network::presetDefaultChannels(const QString &networkName) {
  if(_networksIniPath.isEmpty())  // be sure to have called presetNetworks() first, else this always fails
    return QStringList();
  QSettings s(_networksIniPath, QSettings::IniFormat);
  return s.value(QString("%1/DefaultChannels").arg(networkName)).toStringList();
}

NetworkInfo Network::networkInfoFromPreset(const QString &networkName) {
  NetworkInfo info;
  if(!_networksIniPath.isEmpty()) {
    info.networkName = networkName;
    QSettings s(_networksIniPath, QSettings::IniFormat);
    s.beginGroup(info.networkName);
    foreach(QString server, s.value("Servers").toStringList()) {
      bool ssl = false;
      QStringList splitserver = server.split(':', QString::SkipEmptyParts);
      if(splitserver.count() != 2) {
        qWarning() << "Invalid server entry in networks.conf:" << server;
        continue;
      }
      if(splitserver[1].at(0) == '+')
        ssl = true;
      uint port = splitserver[1].toUInt();
      if(!port) {
        qWarning() << "Invalid port entry in networks.conf:" << server;
        continue;
      }
      info.serverList << Network::Server(splitserver[0].trimmed(), port, QString(), ssl);
    }
  }
  return info;
}


// ====================
//  Public Slots:
// ====================
void Network::setNetworkName(const QString &networkName) {
  _networkName = networkName;
  SYNC(ARG(networkName))
  emit networkNameSet(networkName);
  emit configChanged();
}

void Network::setCurrentServer(const QString &currentServer) {
  _currentServer = currentServer;
  SYNC(ARG(currentServer))
  emit currentServerSet(currentServer);
}

void Network::setConnected(bool connected) {
  if(_connected == connected)
    return;

  _connected = connected;
  if(!connected) {
    setMyNick(QString());
    setCurrentServer(QString());
    removeChansAndUsers();
  }
  SYNC(ARG(connected))
  emit connectedSet(connected);
}

//void Network::setConnectionState(ConnectionState state) {
void Network::setConnectionState(int state) {
  _connectionState = (ConnectionState)state;
  //qDebug() << "netstate" << networkId() << networkName() << state;
  SYNC(ARG(state))
  emit connectionStateSet(_connectionState);
}

void Network::setMyNick(const QString &nickname) {
  _myNick = nickname;
  if(!_myNick.isEmpty() && !ircUser(myNick())) {
    newIrcUser(myNick());
  }
  SYNC(ARG(nickname))
  emit myNickSet(nickname);
}

void Network::setLatency(int latency) {
  if(_latency == latency)
    return;
  _latency = latency;
  SYNC(ARG(latency))
}

void Network::setIdentity(IdentityId id) {
  _identity = id;
  SYNC(ARG(id))
  emit identitySet(id);
  emit configChanged();
}

void Network::setServerList(const QVariantList &serverList) {
  _serverList = fromVariantList<Server>(serverList);
  SYNC(ARG(serverList))
  emit configChanged();
}

void Network::setUseRandomServer(bool use) {
  _useRandomServer = use;
  SYNC(ARG(use))
  emit configChanged();
}

void Network::setPerform(const QStringList &perform) {
  _perform = perform;
  SYNC(ARG(perform))
  emit configChanged();
}

void Network::setUseAutoIdentify(bool use) {
  _useAutoIdentify = use;
  SYNC(ARG(use))
  emit configChanged();
}

void Network::setAutoIdentifyService(const QString &service) {
  _autoIdentifyService = service;
  SYNC(ARG(service))
  emit configChanged();
}

void Network::setAutoIdentifyPassword(const QString &password) {
  _autoIdentifyPassword = password;
  SYNC(ARG(password))
  emit configChanged();
}

void Network::setUseAutoReconnect(bool use) {
  _useAutoReconnect = use;
  SYNC(ARG(use))
  emit configChanged();
}

void Network::setAutoReconnectInterval(quint32 interval) {
  _autoReconnectInterval = interval;
  SYNC(ARG(interval))
  emit configChanged();
}

void Network::setAutoReconnectRetries(quint16 retries) {
  _autoReconnectRetries = retries;
  SYNC(ARG(retries))
  emit configChanged();
}

void Network::setUnlimitedReconnectRetries(bool unlimited) {
  _unlimitedReconnectRetries = unlimited;
  SYNC(ARG(unlimited))
  emit configChanged();
}

void Network::setRejoinChannels(bool rejoin) {
  _rejoinChannels = rejoin;
  SYNC(ARG(rejoin))
  emit configChanged();
}

void Network::addSupport(const QString &param, const QString &value) {
  if(!_supports.contains(param)) {
    _supports[param] = value;
    SYNC(ARG(param), ARG(value))
  }
}

void Network::removeSupport(const QString &param) {
  if(_supports.contains(param)) {
    _supports.remove(param);
    SYNC(ARG(param))
  }
}

QVariantMap Network::initSupports() const {
  QVariantMap supports;
  QHashIterator<QString, QString> iter(_supports);
  while(iter.hasNext()) {
    iter.next();
    supports[iter.key()] = iter.value();
  }
  return supports;
}

QVariantMap Network::initIrcUsersAndChannels() const {
  QVariantMap usersAndChannels;
  QVariantMap users;
  QVariantMap channels;

  QHash<QString, IrcUser *>::const_iterator userIter = _ircUsers.constBegin();
  QHash<QString, IrcUser *>::const_iterator userIterEnd = _ircUsers.constEnd();
  while(userIter != userIterEnd) {
    users[userIter.value()->hostmask()] = userIter.value()->toVariantMap();
    userIter++;
  }
  usersAndChannels["users"] = users;

  QHash<QString, IrcChannel *>::const_iterator channelIter = _ircChannels.constBegin();
  QHash<QString, IrcChannel *>::const_iterator channelIterEnd = _ircChannels.constEnd();
  while(channelIter != channelIterEnd) {
    channels[channelIter.value()->name()] = channelIter.value()->toVariantMap();
    channelIter++;
  }
  usersAndChannels["channels"] = channels;

  return usersAndChannels;
}

void Network::initSetIrcUsersAndChannels(const QVariantMap &usersAndChannels) {
  Q_ASSERT(proxy());
  if(isInitialized()) {
    qWarning() << "Network" << networkId() << "received init data for users and channels allthough there allready are known users or channels!";
    return;
  }

  QVariantMap users = usersAndChannels.value("users").toMap();
  QVariantMap::const_iterator userIter = users.constBegin();
  QVariantMap::const_iterator userIterEnd = users.constEnd();
  while(userIter != userIterEnd) {
    newIrcUser(userIter.key(), userIter.value().toMap());
    userIter++;
  }

  QVariantMap channels = usersAndChannels.value("channels").toMap();
  QVariantMap::const_iterator channelIter = channels.constBegin();
  QVariantMap::const_iterator channelIterEnd = channels.constEnd();
  while(channelIter != channelIterEnd) {
    newIrcChannel(channelIter.key(), channelIter.value().toMap());
    channelIter++;
  }
}

void Network::initSetSupports(const QVariantMap &supports) {
  QMapIterator<QString, QVariant> iter(supports);
  while(iter.hasNext()) {
    iter.next();
    addSupport(iter.key(), iter.value().toString());
  }
}

IrcUser *Network::updateNickFromMask(const QString &mask) {
  QString nick(nickFromMask(mask).toLower());
  IrcUser *ircuser;

  if(_ircUsers.contains(nick)) {
    ircuser = _ircUsers[nick];
    ircuser->updateHostmask(mask);
  } else {
    ircuser = newIrcUser(mask);
  }
  return ircuser;
}

void Network::ircUserNickChanged(QString newnick) {
  QString oldnick = _ircUsers.key(qobject_cast<IrcUser*>(sender()));

  if(oldnick.isNull())
    return;

  if(newnick.toLower() != oldnick) _ircUsers[newnick.toLower()] = _ircUsers.take(oldnick);

  if(myNick().toLower() == oldnick)
    setMyNick(newnick);
}

void Network::emitConnectionError(const QString &errorMsg) {
  emit connectionError(errorMsg);
}

// ====================
//  Private:
// ====================
void Network::determinePrefixes() {
  // seems like we have to construct them first
  QString prefix = support("PREFIX");

  if(prefix.startsWith("(") && prefix.contains(")")) {
    _prefixes = prefix.section(")", 1);
    _prefixModes = prefix.mid(1).section(")", 0, 0);
  } else {
    QString defaultPrefixes("~&@%+");
    QString defaultPrefixModes("qaohv");

    if(prefix.isEmpty()) {
      _prefixes = defaultPrefixes;
      _prefixModes = defaultPrefixModes;
      return;
    }

    // we just assume that in PREFIX are only prefix chars stored
    for(int i = 0; i < defaultPrefixes.size(); i++) {
      if(prefix.contains(defaultPrefixes[i])) {
	_prefixes += defaultPrefixes[i];
	_prefixModes += defaultPrefixModes[i];
      }
    }
    // check for success
    if(!_prefixes.isNull())
      return;

    // well... our assumption was obviously wrong...
    // check if it's only prefix modes
    for(int i = 0; i < defaultPrefixes.size(); i++) {
      if(prefix.contains(defaultPrefixModes[i])) {
	_prefixes += defaultPrefixes[i];
	_prefixModes += defaultPrefixModes[i];
      }
    }
    // now we've done all we've could...
  }
}

/************************************************************************
 * NetworkInfo
 ************************************************************************/

NetworkInfo::NetworkInfo()
: networkId(0),
  identity(1),
  useRandomServer(false),
  useAutoIdentify(false),
  autoIdentifyService("NickServ"),
  useAutoReconnect(true),
  autoReconnectInterval(60),
  autoReconnectRetries(20),
  unlimitedReconnectRetries(false),
  rejoinChannels(true)
{

}

bool NetworkInfo::operator==(const NetworkInfo &other) const {
  if(networkId != other.networkId) return false;
  if(networkName != other.networkName) return false;
  if(identity != other.identity) return false;
  if(codecForServer != other.codecForServer) return false;
  if(codecForEncoding != other.codecForEncoding) return false;
  if(codecForDecoding != other.codecForDecoding) return false;
  if(serverList != other.serverList) return false;
  if(useRandomServer != other.useRandomServer) return false;
  if(perform != other.perform) return false;
  if(useAutoIdentify != other.useAutoIdentify) return false;
  if(autoIdentifyService != other.autoIdentifyService) return false;
  if(autoIdentifyPassword != other.autoIdentifyPassword) return false;
  if(useAutoReconnect != other.useAutoReconnect) return false;
  if(autoReconnectInterval != other.autoReconnectInterval) return false;
  if(autoReconnectRetries != other.autoReconnectRetries) return false;
  if(unlimitedReconnectRetries != other.unlimitedReconnectRetries) return false;
  if(rejoinChannels != other.rejoinChannels) return false;
  return true;
}

bool NetworkInfo::operator!=(const NetworkInfo &other) const {
  return !(*this == other);
}

QDataStream &operator<<(QDataStream &out, const NetworkInfo &info) {
  QVariantMap i;
  i["NetworkId"] = QVariant::fromValue<NetworkId>(info.networkId);
  i["NetworkName"] = info.networkName;
  i["Identity"] = QVariant::fromValue<IdentityId>(info.identity);
  i["CodecForServer"] = info.codecForServer;
  i["CodecForEncoding"] = info.codecForEncoding;
  i["CodecForDecoding"] = info.codecForDecoding;
  i["ServerList"] = toVariantList(info.serverList);
  i["UseRandomServer"] = info.useRandomServer;
  i["Perform"] = info.perform;
  i["UseAutoIdentify"] = info.useAutoIdentify;
  i["AutoIdentifyService"] = info.autoIdentifyService;
  i["AutoIdentifyPassword"] = info.autoIdentifyPassword;
  i["UseAutoReconnect"] = info.useAutoReconnect;
  i["AutoReconnectInterval"] = info.autoReconnectInterval;
  i["AutoReconnectRetries"] = info.autoReconnectRetries;
  i["UnlimitedReconnectRetries"] = info.unlimitedReconnectRetries;
  i["RejoinChannels"] = info.rejoinChannels;
  out << i;
  return out;
}

QDataStream &operator>>(QDataStream &in, NetworkInfo &info) {
  QVariantMap i;
  in >> i;
  info.networkId = i["NetworkId"].value<NetworkId>();
  info.networkName = i["NetworkName"].toString();
  info.identity = i["Identity"].value<IdentityId>();
  info.codecForServer = i["CodecForServer"].toByteArray();
  info.codecForEncoding = i["CodecForEncoding"].toByteArray();
  info.codecForDecoding = i["CodecForDecoding"].toByteArray();
  info.serverList = fromVariantList<Network::Server>(i["ServerList"].toList());
  info.useRandomServer = i["UseRandomServer"].toBool();
  info.perform = i["Perform"].toStringList();
  info.useAutoIdentify = i["UseAutoIdentify"].toBool();
  info.autoIdentifyService = i["AutoIdentifyService"].toString();
  info.autoIdentifyPassword = i["AutoIdentifyPassword"].toString();
  info.useAutoReconnect = i["UseAutoReconnect"].toBool();
  info.autoReconnectInterval = i["AutoReconnectInterval"].toUInt();
  info.autoReconnectRetries = i["AutoReconnectRetries"].toInt();
  info.unlimitedReconnectRetries = i["UnlimitedReconnectRetries"].toBool();
  info.rejoinChannels = i["RejoinChannels"].toBool();
  return in;
}

QDebug operator<<(QDebug dbg, const NetworkInfo &i) {
  dbg.nospace() << "(id = " << i.networkId << " name = " << i.networkName << " identity = " << i.identity
		<< " codecForServer = " << i.codecForServer << " codecForEncoding = " << i.codecForEncoding << " codecForDecoding = " << i.codecForDecoding
		<< " serverList = " << i.serverList << " useRandomServer = " << i.useRandomServer << " perform = " << i.perform
		<< " useAutoIdentify = " << i.useAutoIdentify << " autoIdentifyService = " << i.autoIdentifyService << " autoIdentifyPassword = " << i.autoIdentifyPassword
		<< " useAutoReconnect = " << i.useAutoReconnect << " autoReconnectInterval = " << i.autoReconnectInterval
		<< " autoReconnectRetries = " << i.autoReconnectRetries << " unlimitedReconnectRetries = " << i.unlimitedReconnectRetries
		<< " rejoinChannels = " << i.rejoinChannels << ")";
  return dbg.space();
}

QDataStream &operator<<(QDataStream &out, const Network::Server &server) {
  QVariantMap serverMap;
  serverMap["Host"] = server.host;
  serverMap["Port"] = server.port;
  serverMap["Password"] = server.password;
  serverMap["UseSSL"] = server.useSsl;
  serverMap["sslVersion"] = server.sslVersion;
  serverMap["UseProxy"] = server.useProxy;
  serverMap["ProxyType"] = server.proxyType;
  serverMap["ProxyHost"] = server.proxyHost;
  serverMap["ProxyPort"] = server.proxyPort;
  serverMap["ProxyUser"] = server.proxyUser;
  serverMap["ProxyPass"] = server.proxyPass;
  out << serverMap;
  return out;
}

QDataStream &operator>>(QDataStream &in, Network::Server &server) {
  QVariantMap serverMap;
  in >> serverMap;
  server.host = serverMap["Host"].toString();
  server.port = serverMap["Port"].toUInt();
  server.password = serverMap["Password"].toString();
  server.useSsl = serverMap["UseSSL"].toBool();
  server.sslVersion = serverMap["sslVersion"].toInt();
  server.useProxy = serverMap["UseProxy"].toBool();
  server.proxyType = serverMap["ProxyType"].toInt();
  server.proxyHost = serverMap["ProxyHost"].toString();
  server.proxyPort = serverMap["ProxyPort"].toUInt();
  server.proxyUser = serverMap["ProxyUser"].toString();
  server.proxyPass = serverMap["ProxyPass"].toString();
  return in;
}


bool Network::Server::operator==(const Server &other) const {
  if(host != other.host) return false;
  if(port != other.port) return false;
  if(password != other.password) return false;
  if(useSsl != other.useSsl) return false;
  if(sslVersion != other.sslVersion) return false;
  if(useProxy != other.useProxy) return false;
  if(proxyType != other.proxyType) return false;
  if(proxyHost != other.proxyHost) return false;
  if(proxyPort != other.proxyPort) return false;
  if(proxyUser != other.proxyUser) return false;
  if(proxyPass != other.proxyPass) return false;
  return true;
}

bool Network::Server::operator!=(const Server &other) const {
  return !(*this == other);
}

QDebug operator<<(QDebug dbg, const Network::Server &server) {
  dbg.nospace() << "Server(host = " << server.host << ":" << server.port << ", useSsl = " << server.useSsl << ")";
  return dbg.space();
}
