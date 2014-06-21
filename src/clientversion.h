// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Copyright (c) 2013-2014 Memorycoin Dev Team

#ifndef CLIENTVERSION_H
#define CLIENTVERSION_H

//
// client versioning and copyright year
//

<<<<<<< HEAD
// These need to be macros, as version.cpp's and bitcoin-qt.rc's voodoo requires it
=======
// These need to be macros, as version.cpp's and memorycoin-qt.rc's voodoo requires it
>>>>>>> 96d6688834f5b5b07c8ad5ad6e1e059d6caf401f
#define CLIENT_VERSION_MAJOR       3
#define CLIENT_VERSION_MINOR       0
#define CLIENT_VERSION_REVISION    000
#define CLIENT_VERSION_BUILD       1

// Set to true for release, false for prerelease or test build
#define CLIENT_VERSION_IS_RELEASE  true

// Copyright year (2009-this)
// Todo: update this when changing our copyright comments in the source
#define COPYRIGHT_YEAR 2009-2014

// Converts the parameter X to a string after macro replacement on X has been performed.
// Don't merge these into one macro!
#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X

#endif // CLIENTVERSION_H
