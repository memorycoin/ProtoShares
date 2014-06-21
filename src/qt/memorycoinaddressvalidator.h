// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Memorycoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Copyright (c) 2013-2014 Memorycoin Dev Team

#ifndef MEMORYCOINADDRESSVALIDATOR_H
#define MEMORYCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base48 entry widget validator.
   Corrects near-miss characters and refuses characters that are no part of base48.
 */
class MemorycoinAddressValidator : public QValidator
{
    Q_OBJECT

public:
    explicit MemorycoinAddressValidator(QObject *parent = 0);

    State validate(QString &input, int &pos) const;

    static const int MaxAddressLength = 35;
};

#endif // MEMORYCOINADDRESSVALIDATOR_H
