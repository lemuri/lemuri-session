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

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QGuiApplication>
#include <QSettings>

#include "unitlauncher.h"

class SessionInterface;
class SessionManager : public QGuiApplication
{
public:
    enum Phase {
        WindowManagerStarted = 0x01,
        ShellStarted         = 0x02,
        ServicesStarted      = 0x04
    };

    SessionManager(int &argc, char **argv);
    virtual ~SessionManager();

    void setSessionName(const QString &session);
    void setWindowManager(const QString &windowManager);
    void init();

private Q_SLOTS:
    void windowManagerStarted();

    /**
     * @brief loadShell
     * This method is called after the Window Manager
     * has started, because we need it to properly place
     * the Desktop Shell Windows on the Display Server,
     * on Wayland we will probably start this after the
     * QML view has properly loaded.
     */
    void loadShell();
    void shellStarted();

    /**
     * @brief loadServices
     * This method is called after the shell is
     * up and running, it loads all Session
     * specific services (networking, sound, bluetooth).
     * These aren't explicity needed to have a session
     * running.
     */
    void loadServices();
    void servicesStarted();

    /**
     * @brief loadAutostart
     * This method is called after the services
     * are loaded, this starts all XDG autostart
     * desktop files, just to complain with XDG
     */
    void loadAutostart();
    void autostartStarted();

    void createUnits(QHash<QString, UnitLauncher *> &units, UnitLauncher::Kind kind, const QString &path, const QString &sessionName);
    void unitStarted();

    int m_state;
    SessionInterface *m_sessionInterface;
    bool m_launchX11;
    QString m_sessionName;
    QString m_windowManager;
    UnitLauncher *m_windowManagerUnit;
    QSettings m_setting;
    QHash<QString, UnitLauncher *> m_shellUnits;
    QTimer *m_shellTimeout = 0;
    QHash<QString, UnitLauncher *> m_serviceUnits;
    QTimer *m_servicesTimeout = 0;
    QHash<QString, UnitLauncher *> m_autostartUnits;
};

#endif // SESSIONMANAGER_H
