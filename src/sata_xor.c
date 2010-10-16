/*
 * Simple table-based SATA (de)scrambler
 * Copyright (C) 2010 Werner Johansson, <wj@xnk.nu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "sata_xor.h"

const uint32_t SATA_XOR_scramblerdata[512/4] = { \
    0x4467c108, 0x3d0d9104, 0x61db449c, 0x5c0063ba, 0x19c47848, 0x1f8ac89f, 0x837fa38f, 0x717acf08, \
    0xcd1da489, 0xe132d2e7, 0xfad4ad27, 0xeb99030e, 0x505083f7, 0xbe792d11, 0xe3f1b43c, 0x9f3bd98f, \
    0x67f3b0d9, 0x5de09087, 0x60f0f9ff, 0x2e1a5561, 0x73af5281, 0xc7ad25ee, 0x6bce6e01, 0x74498d01, \
    0x7eed9e9c, 0xa2f33be9, 0x39a0458e, 0x6b96cd0f, 0xb4a73c90, 0xba726f5a, 0x1f586b08, 0xc62a4235, \
    0xa251f44f, 0x896e48a1, 0xc86d36e9, 0x3aec123b, 0x372f89ad, 0x63de1aab, 0xee74ef2f, 0xd151a9c7, \
    0x92ad63ae, 0xf198781b, 0xfabb40b6, 0x22f30722, 0xa346b795, 0x85162bca, 0xc50a4140, 0xadc761f3, \
    0x651bfb53, 0xee55c9ac, 0xa002c173, 0x1553fe29, 0x8dad1f8f, 0xef15de77, 0xd81bf36b, 0xaae39644, \
    0x10dc2a5a, 0x3cda967b, 0x383df28b, 0x2cf381a4, 0x54f54158, 0x739d4573, 0x2ae8fdc5, 0x5030c6be, \
    0xa08f4f9e, 0xe94aed29, 0xca03322f, 0x7a5bd813, 0xe6589dae, 0x90227388, 0x416c430a, 0x1ac4175b, \
    0xeff95e27, 0x23579f63, 0x7097276c, 0x7b5ba8f5, 0x370fa95d, 0xb28bffae, 0x28d9cac8, 0x46b25b8e, \
    0x384080be, 0x2cbbeee4, 0x72c182d2, 0x4b4f115a, 0xf1b9e254, 0x3d539624, 0x50f18133, 0x71041a2e, \
    0x66bff980, 0x226f9c69, 0xbb69d044, 0x988493ad, 0x3267af74, 0xf3658fb9, 0x85f40f4b, 0xffbab5ef, \
    0x9e9edae1, 0x19a99632, 0xf7434fb8, 0x0f1c4cf6, 0xb667d2ce, 0x278de3e3, 0x4c98271e, 0xff5c3773, \
    0x64ca16ab, 0x6dc0917d, 0x1af060ae, 0xf4e61243, 0xc2bae8d6, 0xcee62f9b, 0x8d6a0807, 0x31a76228, \
    0x9b4b3de9, 0x1318195b, 0x08c1a9d2, 0x8c1262ce, 0x43e36412, 0x1c59e3bb, 0xb9cd7f57, 0xab476572, \
    0xc161feb8, 0x25ecc208, 0x891cb98e, 0xa7d26ddf, 0x5210a736, 0xaa2d212a, 0x77d13198, 0x403ba835 };

void SATA_XOR( uint32_t* theData ) {
    uint32_t i;

    for( i=0; i<(512/4); i++ ) {
        theData[i] ^= SATA_XOR_scramblerdata[i];
    }
}

