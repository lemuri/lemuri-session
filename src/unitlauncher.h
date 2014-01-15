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

#ifndef UNITLAUNCHER_H
#define UNITLAUNCHER_H

#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QProcess>

class UnitLauncher : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.foo.session.unit")
public:
    enum Kind {
        Session,
        Service,
        Autostart,
        Custom
    };

    explicit UnitLauncher(const QString &filename, const QString &session, Kind kind, QObject *parent);
    explicit UnitLauncher(const QString &program, QObject *parent);
    virtual ~UnitLauncher();

    Kind kind() const;

    Q_PROPERTY(QString Name READ name)
    QString name() const;

    Q_PROPERTY(uint State READ state NOTIFY stateChanged)
    QProcess::ProcessState state() const;

    bool isValid() const;

    static QString configPath(Kind kind, const QString &sessionName);

public Q_SLOTS:
    void Stop();
    void Start();

Q_SIGNALS:
    void started();
    void stateChanged();

private slots:
    void registerObject();
    void setupProcess(QProcess *process);
    void processStateChanged(QProcess::ProcessState state);
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

    void sessionServiceOwnerChanged(const QString &service, const QString &oldOwner, const QString &newOwner);
    void systemServiceOwnerChanged(const QString &service, const QString &oldOwner, const QString &newOwner);

private:
    QSettings m_settings;
    QString m_session;
    QStringList m_dbusSessionRequires;
    QStringList m_dbusSessionRequiresRunning;
    QStringList m_dbusSystemRequires;
    QStringList m_dbusSystemRequiresRunning;
    QStringList m_onlyShowIn;
    QString m_exec;
    QString m_dbusExec;
    QProcess *m_process;
    int m_crashCount;
    Kind m_kind;
    bool m_enabled;
    bool m_shutdownOnMissingDeps;
};

#endif // UNITLAUNCHER_H
