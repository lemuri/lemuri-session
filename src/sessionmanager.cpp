/***************************************************************************
 *   Copyright (C) 2014 by Daniel Nicoletti <dantti12@gmail.com>           *
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
 *   along with this program; see the file COPYING. If not, write to       *
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,  *
 *   Boston, MA 02110-1301, USA.                                           *
 ***************************************************************************/

#include "sessionmanager.h"

#include "sessioninterface.h"

#include <QDirIterator>
#include <QProcess>
#include <QTimer>
#include <QDebug>

#define UNIT_TIMEOUT 200

SessionManager::SessionManager(int &argc, char **argv) :
    QGuiApplication(argc, argv),
    m_state(0),
    m_sessionInterface(0),
    m_windowManagerUnit(0)
{
    setQuitOnLastWindowClosed(false);
}

SessionManager::~SessionManager()
{
}

void SessionManager::setSessionName(const QString &session)
{
    qDebug() << session;
    m_sessionName = session;
    QSettings settings(QString("/etc/foo/%1.conf").arg(session), QSettings::IniFormat);
    settings.beginGroup(QLatin1String("Desktop Entry"));

    qDebug() << m_windowManager << settings.fileName();
    settings.setPath(QSettings::IniFormat, QSettings::UserScope, QString("/etc"));
    m_windowManager = settings.value(QLatin1String("X-WindowManager")).toString();
    qDebug() << m_windowManager << settings.fileName();
}

void SessionManager::setWindowManager(const QString &windowManager)
{
    m_windowManager = windowManager;
}

void SessionManager::init()
{
    m_sessionInterface = new SessionInterface(this);
    if (!m_sessionInterface->isRegistered()) {
        exit(1);
        return;
    }

    if (m_windowManager.isEmpty()) {
        loadShell();
    } else {
        m_windowManagerUnit = new UnitLauncher(m_windowManager, this);
        connect(m_windowManagerUnit, &UnitLauncher::started,
                this, &SessionManager::windowManagerStarted);
        m_windowManagerUnit->Start();
    }
}

void SessionManager::windowManagerStarted()
{
    if (m_state & WindowManagerStarted) {
        return;
    }

    qDebug() << "Window Manager started";
    m_state |= WindowManagerStarted;
    loadShell();
}

void SessionManager::loadShell()
{
    QString sessionPath = UnitLauncher::configPath(UnitLauncher::Session, m_sessionName);
    qDebug() << "Load shell units" << sessionPath;

    createUnits(m_shellUnits, UnitLauncher::Session, sessionPath, m_sessionName);

    if (m_shellUnits.isEmpty()) {
        m_state |= ShellStarted;
        loadServices();
    } else {
        m_shellTimeout = new QTimer(this);
        connect(m_shellTimeout, &QTimer::timeout,
                this, &SessionManager::servicesStarted);
        m_shellTimeout->setSingleShot(true);
        m_shellTimeout->start(m_shellUnits.size() * UNIT_TIMEOUT);
    }
}

void SessionManager::shellStarted()
{
    if (m_state & ShellStarted) {
        return;
    }

    int missingUnits = 0;
    QTimer *timer = qobject_cast<QTimer*>(sender());
    if (!timer) {
        QHash<QString, UnitLauncher *>::ConstIterator it = m_shellUnits.constBegin();
        while (it != m_shellUnits.constEnd()) {
            UnitLauncher *launcher = it.value();
            if (launcher->state() == QProcess::Starting) {
                missingUnits += UNIT_TIMEOUT;
            }
            ++it;
        }
    }

    if (missingUnits) {
        m_shellTimeout->start(missingUnits);
    } else {
        m_state |= ShellStarted;
        loadServices();
    }
}

void SessionManager::loadServices()
{
    QString servicesPath = UnitLauncher::configPath(UnitLauncher::Service, m_sessionName);
    qDebug() << "Load services units" << servicesPath;

    createUnits(m_serviceUnits, UnitLauncher::Service, servicesPath, m_sessionName);

    if (m_serviceUnits.isEmpty()) {
        m_state |= ShellStarted;
        loadAutostart();
    } else {
        m_servicesTimeout = new QTimer(this);
        connect(m_servicesTimeout, &QTimer::timeout,
                this, &SessionManager::servicesStarted);
        m_servicesTimeout->setSingleShot(true);
        m_servicesTimeout->start(m_serviceUnits.size() * UNIT_TIMEOUT);
    }
}

void SessionManager::servicesStarted()
{
    if (m_state & ServicesStarted) {
        return;
    }

    int missingUnits = 0;
    QTimer *timer = qobject_cast<QTimer *>(sender());
    if (!timer) {
        QHash<QString, UnitLauncher *>::ConstIterator it = m_shellUnits.constBegin();
        while (it != m_shellUnits.constEnd()) {
            UnitLauncher *launcher = it.value();
            if (launcher->state() == QProcess::Starting) {
                missingUnits += UNIT_TIMEOUT;
            }
            ++it;
        }
    }

    if (missingUnits) {
        m_servicesTimeout->start(missingUnits);
    } else {
        m_state |= ServicesStarted;
        loadAutostart();
    }
}

void SessionManager::loadAutostart()
{
    createUnits(m_autostartUnits, UnitLauncher::Autostart,
                QLatin1String("/etc/xdg/autostart"), m_sessionName);
    createUnits(m_autostartUnits, UnitLauncher::Autostart,
                QLatin1String("/usr/share/autostart"), m_sessionName);
}

void SessionManager::autostartStarted()
{

}

void SessionManager::createUnits(QHash<QString, UnitLauncher *> &units, UnitLauncher::Kind kind, const QString &path, const QString &sessionName)
{
    QDirIterator it(path, QDir::Files);
    while (it.hasNext()) {
        qDebug() << it.next();
        if (!units.contains(it.filePath())) {
            UnitLauncher *launcher = new UnitLauncher(it.filePath(),
                                                      sessionName,
                                                      kind,
                                                      this);
            if (!launcher->isValid()) {
                delete launcher;
                continue;
            }

            connect(launcher, &UnitLauncher::started,
                    this, &SessionManager::unitStarted);
            launcher->Start();
            units.insert(it.filePath(), launcher);
        }
    }
}

void SessionManager::unitStarted()
{
    UnitLauncher *launcher = qobject_cast<UnitLauncher*>(sender());
    switch (launcher->kind()) {
    case UnitLauncher::Session:
        shellStarted();
        break;
    case UnitLauncher::Service:
        servicesStarted();
        break;
    case UnitLauncher::Autostart:
        autostartStarted();
        break;
    default:
        qCritical() << "Custon unit started" << launcher->objectName();
        break;
    }
}
