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

/** This is called at compile time and generates a suitable version.gen.
 *  usage: genversion git_root target_file
 */

#include <QDebug>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QFile>

int main(int argc, char **argv) {
  if(argc < 3) {
    qFatal("Usage: ./genversion <git_root> <target_file>");
    return 255;
  }
  QString gitroot = argv[1];
  QString target = argv[2];
  QString basever, protover, clientneeds, coreneeds, descrver, dirty;
  QString committish, commitdate;

  // check Git for information if present
  if(QFile::exists(gitroot + "/.git")) {
    // try to execute git-describe to get a version string
    QProcess git;
    git.setWorkingDirectory(gitroot);
    git.start("git", QStringList() << "describe" << "--long");
    if(git.waitForFinished(10000)) {
      QString descr = git.readAllStandardOutput().trimmed();
      if(!descr.isEmpty() && !descr.contains("fatal")) {
        // seems we have a valid git describe string
        descrver = descr;
        // check if the workdir is dirty
        git.start("git", QStringList() << "diff-index" << "--name-only" << "HEAD");
        if(git.waitForFinished(10000)) {
          if(!git.readAllStandardOutput().isEmpty()) dirty = "*";
        }
        // get a full committish
        git.start("git", QStringList() << "rev-parse" << "HEAD");
        if(git.waitForFinished(10000)) {
          committish = git.readAllStandardOutput().trimmed();
        }
        // Now we do some replacement magic...
        //QRegExp rxCheckTag("(.*)-0-g[0-9a-f]+\n$");
        //QRegExp rxGittify("(.*)-(\\d+)-g([0-9a-f]+)\n$");
        //gitversion.replace(rxCheckTag, QString("\\1%1").arg(dirty));
        //gitversion.replace(rxGittify, QString("\\1:git-\\3+\\2%1").arg(dirty));
      }
    }
  }

  // parse version.inc
  QFile verfile(gitroot + "/version.inc");
  if(verfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QString ver = verfile.readAll();

    QRegExp rxBasever("baseVersion\\s*=\\s*\"(.*)\";");
    if(rxBasever.indexIn(ver) >= 0)
      basever = rxBasever.cap(1);

    QRegExp rxProtover("protocolVersion\\s*=\\s*(\\d+)");
    if(rxProtover.indexIn(ver) >= 0)
      protover = rxProtover.cap(1);

    QRegExp rxClientneeds("clientNeedsProtocol\\s*=\\s*(\\d+)");
    if(rxClientneeds.indexIn(ver) >= 0)
      clientneeds = rxClientneeds.cap(1);

    QRegExp rxCoreneeds("coreNeedsProtocol\\s*=\\s*(\\d+)");
    if(rxCoreneeds.indexIn(ver) >= 0)
      coreneeds = rxCoreneeds.cap(1);

    if(committish.isEmpty()) {
      QRegExp rxCommit("distCommittish\\s*=\\s*([0-9a-f]+)");
      if(rxCommit.indexIn(ver) >= 0) committish = rxCommit.cap(1);
    }

    QRegExp rxTimestamp("distCommitDate\\s*=\\s*([0-9]+)");
    if(rxTimestamp.indexIn(ver) >= 0) commitdate = rxTimestamp.cap(1);
    verfile.close();
  }

  // ok, create our version.gen now
  QFile gen(target);
  if(!gen.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qFatal("%s", qPrintable(QString("Could not write %1!").arg(target)));
    return EXIT_FAILURE;
  }
  gen.write(QString("QString buildinfo = \"%1,%2,%3,%4,%5,%6,%7,%8\";\n")
           .arg(basever, descrver, dirty, committish, commitdate, protover, clientneeds, coreneeds).toAscii());
  gen.close();
  return EXIT_SUCCESS;
}
