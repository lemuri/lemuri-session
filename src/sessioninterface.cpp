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

#include "sessioninterface.h"

#include "sessionadaptor.h"

#include <QtDBus/QDBusConnection>

#include <QProcess>
#include <QDebug>

SessionInterface::SessionInterface(QObject *parent) :
    QObject(parent),
    m_registered(true)
{
    if (qgetenv("DBUS_SESSION_BUS_ADDRESS").isNull()) {
        qWarning() << "Launching DBus";
        QProcess process;
        process.closeReadChannel(QProcess::StandardError);
        process.start(QLatin1String("dbus-launch"), {"--close-stderr", "--exit-with-session"},
                      QProcess::ReadWrite | QProcess::Unbuffered);
        process.waitForFinished(5000);

        QByteArray line = process.readAllStandardOutput();
        QList<QByteArray> vars = line.trimmed().split('\n');
        foreach (const QByteArray &env, vars) {
            int equalIndex = env.indexOf('=');
             qputenv(env.mid(0, equalIndex), env.mid(equalIndex + 1));
        }
    }

    (void) new SessionAdaptor(this);
    if (!QDBusConnection::sessionBus().registerService(QLatin1String("org.lemuri.session"))) {
        qWarning() << "unable to register service to dbus";
        m_registered = false;
        return;
    }

//    if (!QDBusConnection::sessionBus().registerService("org.le.ksmserver")) {
//        qWarning() << "unable to register ksmserver interface to dbus";
//        m_registered = false;
//        return;
//    }

    if (!QDBusConnection::sessionBus().registerObject("/org/lemuri/session", this)) {
        qWarning() << "unable to register object to dbus";
        m_registered = false;
        return;
    }

//    if (!QDBusConnection::sessionBus().registerService("org.kde.kded")) {
//        kDebug() << "unable to register ksmserver interface to dbus";
//        m_registered = false;
//        return;
//    }
}

SessionInterface::~SessionInterface()
{
}

bool SessionInterface::isRegistered() const
{
    return m_registered;
}
