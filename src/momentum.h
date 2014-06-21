// Copyright (c) 2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Copyright (c) 2013-2014 Memorycoin Dev Team

#include "bignum.h"

namespace mc {
	std::vector< std::pair<uint32_t,uint32_t> > momentum_search( uint256 midHash, char* scratchpad, int totalThreads );
	bool momentum_verify( uint256 midHash, uint32_t a, uint32_t b );
    bool hasAESNIInstructions();
}

