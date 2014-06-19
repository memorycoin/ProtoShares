// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Copyright (c) 2013-2014 Memorycoin Dev Team

#ifndef URITESTS_H
#define URITESTS_H

#include <QTest>
#include <QObject>

class URITests : public QObject
{
    Q_OBJECT

private slots:
    void uriTests();
};

#endif // URITESTS_H
