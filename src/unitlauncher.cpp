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

#include "unitlauncher.h"

#include "unitadaptor.h"

#include <QProcess>
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QRegularExpression>

UnitLauncher::UnitLauncher(const QString &filename, const QString &session, UnitLauncher::Kind kind, QObject *parent) :
    QObject(parent),
    m_settings(filename, QSettings::IniFormat),
    m_session(session),
    m_process(0),
    m_crashCount(0),
    m_kind(kind)
{
    m_settings.beginGroup(QLatin1String("Desktop Entry"));

    QString dbusSessionRequires = m_settings.value(QLatin1String("DBusSessionRequires")).toString().trimmed();
    if (!dbusSessionRequires.isEmpty()) {
        m_dbusSessionRequires = dbusSessionRequires.split(QLatin1String(" "));
    }

    QString dbusSystemRequires = m_settings.value(QLatin1String("DBusSystemRequires")).toString().trimmed();
    if (!dbusSystemRequires.isEmpty()) {
        m_dbusSystemRequires = dbusSystemRequires.split(QLatin1String(" "));
    }

    QString onlyShowIn = m_settings.value(QLatin1String("OnlyShowIn")).toString().trimmed();
    if (!onlyShowIn.isEmpty()) {
        m_onlyShowIn = onlyShowIn.split(QLatin1String(";"));
    }

    m_exec = m_settings.value(QLatin1String("Exec")).toString().trimmed();
    m_dbusExec = m_settings.value(QLatin1String("DBusExec")).toString().trimmed();
    m_enabled = m_settings.value(QLatin1String("Enabled")).toBool();

    if (m_dbusExec.isEmpty()) {
        m_process = new QProcess(this);
        setupProcess(m_process);
    }

    QDBusServiceWatcher *sessionWatcher = new QDBusServiceWatcher(this);
    sessionWatcher->setConnection(QDBusConnection::sessionBus());
    sessionWatcher->setWatchMode(QDBusServiceWatcher::WatchForOwnerChange);
    connect(sessionWatcher, &QDBusServiceWatcher::serviceOwnerChanged,
            this, &UnitLauncher::sessionServiceOwnerChanged);
    foreach (const QString &service, m_dbusSessionRequires) {
        sessionWatcher->addWatchedService(service);
    }

    QDBusServiceWatcher *systemWatcher = new QDBusServiceWatcher(this);
    systemWatcher->setConnection(QDBusConnection::systemBus());
    systemWatcher->setWatchMode(QDBusServiceWatcher::WatchForOwnerChange);
    connect(systemWatcher, &QDBusServiceWatcher::serviceOwnerChanged,
            this, &UnitLauncher::systemServiceOwnerChanged);
    foreach (const QString &service, m_dbusSystemRequires) {
        systemWatcher->addWatchedService(service);
    }

    QFileInfo fileInfo(filename);
    QString unit = fileInfo.baseName().replace(QRegularExpression("\\W"), QLatin1String("_"));
    if (kind == Session) {
        setObjectName("/org/lemuri/session_units/" % unit);
    } else if (kind == Autostart) {
        setObjectName("/org/lemuri/autostart_units/" % unit);
    } else {
        setObjectName("/org/lemuri/service_units/" % unit);
    }

    registerObject();
}

UnitLauncher::UnitLauncher(const QString &program, QObject *parent) :
    QObject(parent),
    m_process(new QProcess),
    m_crashCount(0),
    m_kind(Custom),
    m_exec(program)
{
    QString unit = program;
    unit = unit.replace(QRegularExpression("\\W"), QLatin1String("_"));
    setObjectName("/org/lemuri/custom_units/" % unit);

    setupProcess(m_process);

    registerObject();
}

UnitLauncher::~UnitLauncher()
{
    if (m_process) {
        m_process->terminate();
    }
}

UnitLauncher::Kind UnitLauncher::kind() const
{
    return m_kind;
}

QString UnitLauncher::name() const
{
    return objectName();
}

QProcess::ProcessState UnitLauncher::state() const
{
    if (m_process) {
        return m_process->state();
    } else {
        return QProcess::NotRunning;
    }
}

bool UnitLauncher::isValid() const
{
    if (!m_onlyShowIn.isEmpty() && !m_onlyShowIn.contains(m_session, Qt::CaseInsensitive)) {
        return false;
    }

    return true;
}

QString UnitLauncher::configPath(UnitLauncher::Kind kind, const QString &sessionName)
{
    if (kind == Session) {
        return QLatin1String("/etc/lemuri/session/") % sessionName;
    } else {
        return QLatin1String("/etc/lemuri/service/") % sessionName;
    }
}

void UnitLauncher::Stop()
{
    if (m_process) {
        if (m_process->state() == QProcess::Running ||
                m_process->state() == QProcess::Starting) {
            m_process->terminate();
        }
    } else {
        // TODO DBus launch
    }
}

void UnitLauncher::Start()
{
    if (m_dbusSessionRequires.size() != m_dbusSessionRequiresRunning.size() ||
            m_dbusSystemRequires.size() != m_dbusSystemRequiresRunning.size()) {
        // Not ready yet
        qDebug() << "not ready" << objectName();
        qDebug() << m_dbusSessionRequires << m_dbusSessionRequiresRunning;
        qDebug() << m_dbusSystemRequires << m_dbusSystemRequiresRunning;
        return;
    }

    if (m_process) {
//        m_process->setProcessEnvironment(*Environment::global());
        qDebug() << "starting" << objectName();
        m_process->start();
    } else {
        // TODO DBus launch
    }
}

void UnitLauncher::registerObject()
{
    if (!isValid()) {
        return;
    }

    (void) new UnitAdaptor(this);
    QDBusConnection::sessionBus().registerService(QLatin1String("org.foo.session.unit"));

    if (!QDBusConnection::sessionBus().registerObject(objectName(), this)) {
        qWarning() << "unable to register object to dbus" << objectName();
        return;
    }

    // show info about this unit
    qDebug() << objectName();
    qDebug() << "DBusExec" << m_dbusExec;
    qDebug() << "OnlyShowIn" << m_onlyShowIn;
    qDebug() << "Exec" << m_exec;
    qDebug() << "DBusSessionRequires" << m_dbusSessionRequires;
    qDebug() << "DBusSystemRequires" << m_dbusSystemRequires;
    qDebug() << "Enabled" << m_enabled;
    qDebug();
}

void UnitLauncher::setupProcess(QProcess *process)
{
    m_process->setProgram(m_exec);
    m_process->setProcessChannelMode(QProcess::ForwardedChannels);
    connect(m_process, &QProcess::started,
            this, &UnitLauncher::started);
    connect(m_process, &QProcess::stateChanged,
            this, &UnitLauncher::processStateChanged);
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(finished(int,QProcess::ExitStatus)));
}

void UnitLauncher::processStateChanged(QProcess::ProcessState state)
{
    qDebug() << objectName() << state;
    emit stateChanged();
}

void UnitLauncher::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << objectName() << exitCode << exitStatus;
    if (exitStatus == QProcess::CrashExit && ++m_crashCount < 5) {
        qDebug() << objectName() << "Has crashed respawing..." << m_crashCount;
        Start();
    }
}

void UnitLauncher::sessionServiceOwnerChanged(const QString &service, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(oldOwner)

    if (newOwner.isEmpty()) {
        m_dbusSessionRequiresRunning.removeOne(service);
        if (m_shutdownOnMissingDeps) {
            Stop();
        }
    } else if (!m_dbusSessionRequiresRunning.contains(service)) {
        m_dbusSessionRequiresRunning.append(service);
    }
}

void UnitLauncher::systemServiceOwnerChanged(const QString &service, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(oldOwner)

    if (newOwner.isEmpty()) {
        m_dbusSystemRequiresRunning.removeOne(service);
        if (m_shutdownOnMissingDeps) {
            Stop();
        }
    } else if (!m_dbusSystemRequiresRunning.contains(service)) {
        m_dbusSystemRequiresRunning.append(service);
        Start();
    }
}
