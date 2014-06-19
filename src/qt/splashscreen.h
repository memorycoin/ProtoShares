// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Copyright (c) 2013-2014 Memorycoin Dev Team

#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QSplashScreen>

/** class for the splashscreen with information of the running client
 */
class SplashScreen : public QSplashScreen
{
    Q_OBJECT

public:
    explicit SplashScreen(const QPixmap &pixmap = QPixmap(), Qt::WindowFlags f = 0);
};

#endif // SPLASHSCREEN_H
