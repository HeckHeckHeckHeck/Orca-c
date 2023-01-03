// This file is under GNU General Public License 3.0
// see LICENSE.txt
// clang-format off
#ifndef ORCA_NET_HH
#define ORCA_NET_HH
#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif


void net_init(void);

void net_test_cxx(Glyph * gbuf,Mark * mbuf,Usz height,Usz width,Usz tick_number);



#ifdef __cplusplus
};
#endif

#endif //ORCA_NET_HH
