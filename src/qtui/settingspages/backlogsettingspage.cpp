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

#include "backlogsettingspage.h"

#include "qtui.h"
#include "backlogsettings.h"

BacklogSettingsPage::BacklogSettingsPage(QWidget *parent)
  : SettingsPage(tr("Misc"), tr("Backlog"), parent)
{
  ui.setupUi(this);
  initAutoWidgets();
  // not an auto widget, because we store index + 1

  // FIXME: global backlog requester disabled until issues ruled out
  ui.requesterType->removeItem(2);

  connect(ui.requesterType, SIGNAL(currentIndexChanged(int)), this, SLOT(widgetHasChanged()));
}

bool BacklogSettingsPage::hasDefaults() const {
  return true;
}

void BacklogSettingsPage::defaults() {
  ui.requesterType->setCurrentIndex(0);

  SettingsPage::defaults();
}

void BacklogSettingsPage::load() {
  BacklogSettings backlogSettings;
  int index = backlogSettings.requesterType() - 1;
  ui.requesterType->setProperty("storedValue", index);
  ui.requesterType->setCurrentIndex(index);

  SettingsPage::load();
}

void BacklogSettingsPage::save() {
  BacklogSettings backlogSettings;
  backlogSettings.setRequesterType(ui.requesterType->currentIndex() + 1);
  ui.requesterType->setProperty("storedValue", ui.requesterType->currentIndex());

  SettingsPage::save();
}

void BacklogSettingsPage::widgetHasChanged() {
  setChangedState(ui.requesterType->currentIndex() != ui.requesterType->property("storedValue").toInt());
}
