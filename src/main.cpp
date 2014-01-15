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

#include <QLocale>
#include <QCommandLineParser>
#include <QTranslator>
#include <QGuiApplication>
#include <QLibraryInfo>
#include <QDebug>

#include "sessionmanager.h"

using namespace std;

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("lemuri");
    QCoreApplication::setOrganizationDomain("lemuri.org");
    QCoreApplication::setApplicationName("LemuriSession");
    QCoreApplication::setApplicationVersion("0.0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Lemuri session manager");
    parser.addHelpOption();
    parser.addVersionOption();

    // An option with a value
    QCommandLineOption targetSessionOption(QStringList() << "s" << "session-name",
            QCoreApplication::translate("main", "Name of the session we are about to start <session>."),
            QCoreApplication::translate("main", "session"));
    parser.addOption(targetSessionOption);

    SessionManager app(argc, argv);

    // Process the actual command line arguments given by the user
    parser.process(app);

    if (parser.isSet(targetSessionOption)) {
        app.setSessionName(parser.value(targetSessionOption));
    } else {
        parser.showHelp(1);
    }

    app.init();

    return app.exec();
}
