#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
#/**
# * Software Name : gea12.py
# * Version : 0.1
# *
# * Copyright 2021. Benoit Michau. P1Sec.
# *
# * This program is free software: you can redistribute it and/or modify
# * it under the terms of the GNU Affero General Public License as published by
# * the Free Software Foundation, either version 3 of the License, or
# * (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU Affero General Public License for more details.
# *
# * You should have received a copy of the GNU Affero General Public License
# * along with this program.  If not, see <http://www.gnu.org/licenses/>.
# *
# *--------------------------------------------------------
# * File Name : gea12.py
# * Created : 2021-10-19
# * Authors : Benoit Michau 
# *--------------------------------------------------------
#*/

# GPRS encryption algorithm 1 and 2
# implemented from the excellent research paper on its cryptanalysis:
#   https://eprint.iacr.org/2021/819.pdf

from functools import reduce


#------------------------------------------------------------------------------#
# formatting routines
#------------------------------------------------------------------------------#

def uint_to_bitlist(uint, bitlen):
    """Convert a big-endian unsigned int `uint` of length `bitlen` to a list of bits
    """
    bl = bin(uint)[2:]
    if len(bl) < bitlen:
        # extend v
        bl = '0'*(bitlen-len(bl)) + bl
    return [0 if b == '0' else 1 for b in bl]


def bitlist_to_uint(bl):
    """Convert a list of bits `bl` to a big-endian unsinged integer
    """
    return reduce(lambda x, y: (x<<1)+y, bl)


def byte_rev(uint, l=None):
    """revert byte order in a big endian unsigned integer
    """
    if l is None:
        bl = uint.bit_length()
        l  = bl // 8
        if bl % 8:
            l += 1
    #
    b  = []
    for i in range(0, l):
        b.append( uint & 0xff )
        uint >>= 8
    return reduce(lambda x, y: (x<<8)+y, b)


class LFSR:
    """parent class for all LFSR
    """
    # global debugging level
    # 0: silent
    # 1: print initialized values in registers
    dbg = 0


#------------------------------------------------------------------------------#
# f boolean function
#------------------------------------------------------------------------------#

def f(x0, x1, x2, x3, x4, x5, x6):
    """Boolean function f on seven variables of degree 4
    
    section 2.1:
    x0x2x5x6 + x0x3x5x6 + x0x1x5x6 + x1x2x5x6 + x0x2x3x6 + x1x3x4x6
    + x1x3x5x6 + x0x2x4 + x0x2x3 + x0x1x3 + x0x2x6 + x0x1x4 + x0x1x6
    + x1x2x6 + x2x5x6 + x0x3x5 + x1x4x6 + x1x2x5 + x0x3 + x0x5 + x1x3
    + x1x5 + x1x6 + x0x2 + x1 + x2x3 + x2x5 + x2x6 + x4x5 + x5x6 + x2 + x3 + x5
    """
    return (
        x0*x2*x5*x6 ^ x0*x3*x5*x6 ^ x0*x1*x5*x6 ^ x1*x2*x5*x6 ^ \
        x0*x2*x3*x6 ^ x1*x3*x4*x6 ^ x1*x3*x5*x6 ^ \
        x0*x2*x4 ^ x0*x2*x3 ^ x0*x1*x3 ^ x0*x2*x6 ^ \
        x0*x1*x4 ^ x0*x1*x6 ^ x1*x2*x6 ^ x2*x5*x6 ^ \
        x0*x3*x5 ^ x1*x4*x6 ^ x1*x2*x5 ^ \
        x0*x3 ^ x0*x5 ^ x1*x3 ^ x1*x5 ^ \
        x1*x6 ^ x0*x2 ^ x2*x3 ^ x2*x5 ^ \
        x2*x6 ^ x4*x5 ^ x5*x6 ^ \
        x1 ^ x2 ^ x3 ^ x5
        )


#------------------------------------------------------------------------------#
# S LFSR for initialization
#------------------------------------------------------------------------------#

class S(LFSR):
    """64 bits S register used for initialization
    """
    
    def __init__(self, iv, dir, key):
        """Initialize the S LFSR [64 bits] 
        
        Args:
            iv  : 32 bits as uint32_t (big endian)
            dir : 1 bit (LSB), uint8_t
            key : 64 bits, uint64_t (big endian)
        """
        self.IN  = 128 * [0] + \
                   uint_to_bitlist(key, 64) + \
                   uint_to_bitlist(dir,  1) + \
                   uint_to_bitlist(iv,  32)
        print(f'K   : {uint_to_bitlist(key, 64)}')
        print(f'dir : {uint_to_bitlist(dir,  1)}')
        print(f'IV  : {uint_to_bitlist(iv,  32)}')
        self.R   = 64 * [0]
        self.clk = 0
    
    def load(self):
        #print(f'Input K + dir + IV :\n{self.IN[128:]}')
        #print(f"Etape {(self.clk):03d} : {self.R[::-1]}")
        while self.IN:
            self.clock()
            #print(f"Etape {self.clk:03d} : {self.R[::-1]}")
        if self.dbg:
            #print('S init: 0x%.16x' % bitlist_to_uint(self.R))
            print(f'S init : {hex(int("".join([str(x) for x in self.R[::-1]]), 2))}')
    
    def clock(self):
        # compute input bit
        inp = self.R[0] ^ self.f() ^ self.IN.pop()
        # shift LFSR
        self.R = self.R[1:] + [inp]
        self.clk += 1
    
    def f(self):
        return f(
            self.R[3],
            self.R[12],
            self.R[22],
            self.R[38],
            self.R[42],
            self.R[55],
            self.R[63],
            )


#------------------------------------------------------------------------------#
# LFSRs for keystream generation in GEA1
#------------------------------------------------------------------------------#

class A(LFSR):
    """31 bits A register used for keystream generation
    """
    
    def __init__(self, IN):
        self.IN  = IN
        self.R   = 31 * [0]
        self.clk = 0
    
    def load(self):
        while self.IN:
            #print(f"Etape {self.clk} : {self.R}")
            self.clock(self.IN.pop())
        #print("Etape %02d :".format(self.clk) + f"{self.R}")
        if all([b == 0 for b in self.R]):
            self.R[0] = 1
        if self.dbg:
            #print('A init: 0x%.16x' % bitlist_to_uint(self.R))
            print(f'A init : {hex(int("".join([str(x) for x in self.R[::-1]]), 2))}')
    
    def clock(self, inp=None):
        if inp is not None:
            R0 = self.R[0] ^ inp
        else:
            R0 = self.R[0]
        # feedback
        if R0:
            self.R[1]  ^= R0
            self.R[3]  ^= R0
            self.R[4]  ^= R0
            self.R[8]  ^= R0
            self.R[9]  ^= R0
            self.R[10] ^= R0
            self.R[12] ^= R0
            self.R[13] ^= R0
            self.R[16] ^= R0
            self.R[20] ^= R0
            self.R[21] ^= R0
            self.R[23] ^= R0
            self.R[24] ^= R0
            self.R[25] ^= R0
            self.R[27] ^= R0
            self.R[28] ^= R0
            self.R[29] ^= R0
        # shift
        self.R = self.R[1:] + [R0]
        self.clk += 1
    
    def f(self):
        return f(
            self.R[22],
            self.R[0],
            self.R[13],
            self.R[21],
            self.R[25],
            self.R[2],
            self.R[7],
            )


class B(LFSR):
    """32 bits B register used for keystream generation
    """
    
    def __init__(self, IN):
        self.IN  = IN
        self.R   = 32 * [0]
        self.clk = 0
    
    def load(self):
        while self.IN:
            self.clock(self.IN.pop())
        if all([b == 0 for b in self.R]):
            self.R[0] = 1
        if self.dbg:
            #print('B init: 0x%.16x' % bitlist_to_uint(self.R))
            print(f'B init : {hex(int("".join([str(x) for x in self.R[::-1]]), 2))}')
    
    def clock(self, inp=None):
        if inp is not None:
            R0 = self.R[0] ^ inp
        else:
            R0 = self.R[0]
        # feedback
        if R0:
            self.R[1]  ^= R0
            self.R[3]  ^= R0
            self.R[7]  ^= R0
            self.R[13] ^= R0
            self.R[14] ^= R0
            self.R[15] ^= R0
            self.R[16] ^= R0
            self.R[23] ^= R0
            self.R[24] ^= R0
            self.R[25] ^= R0
            self.R[29] ^= R0
            self.R[30] ^= R0
            self.R[31] ^= R0
        # shift
        self.R = self.R[1:] + [R0]
        self.clk += 1
    
    def f(self):
        return f(
            self.R[12],
            self.R[27],
            self.R[0],
            self.R[1],
            self.R[29],
            self.R[21],
            self.R[5],
            )


class C(LFSR):
    """33 bits C register used for keystream generation
    """
    
    def __init__(self, IN):
        self.IN  = IN
        self.R   = 33 * [0]
        self.clk = 0
    
    def load(self):
        while self.IN:
            self.clock(self.IN.pop())
        if all([b == 0 for b in self.R]):
            self.R[0] = 1
        if self.dbg:
            #print('C init: 0x%.16x' % bitlist_to_uint(self.R))
            print(f'C init : {hex(int("".join([str(x) for x in self.R[::-1]]), 2))}')
    
    def clock(self, inp=None):
        if inp is not None:
            R0 = self.R[0] ^ inp
        else:
            R0 = self.R[0]
        # feedback
        if R0:
            self.R[3]  ^= R0
            self.R[6]  ^= R0
            self.R[10] ^= R0
            self.R[12] ^= R0
            self.R[13] ^= R0
            self.R[14] ^= R0
            self.R[15] ^= R0
            self.R[16] ^= R0
            self.R[18] ^= R0
            self.R[19] ^= R0
            self.R[22] ^= R0
            self.R[23] ^= R0
            self.R[24] ^= R0
            self.R[29] ^= R0
            self.R[31] ^= R0
        # shift
        self.R = self.R[1:] + [R0]
        self.clk += 1
    
    def f(self):
        return f(
            self.R[10],
            self.R[30],
            self.R[32],
            self.R[3],
            self.R[19],
            self.R[0],
            self.R[4],
            )


#------------------------------------------------------------------------------#
# Additional LFSRs for initialization and keystream generation in GEA2
#------------------------------------------------------------------------------#

class W(LFSR):
    """97 bits W register used for initialization in GEA2
    """
    
    def __init__(self, iv, dir, key):
        """Initialize the W LFSR [97 bits] 
        
        Args:
            iv  : 32 bits as uint32_t (big endian)
            dir : 1 bit (LSB), uint8_t
            key : 64 bits, uint64_t (big endian)
        """
        self.IN  = 194 * [0] + \
                   uint_to_bitlist(key, 64) + \
                   uint_to_bitlist(dir,  1) + \
                   uint_to_bitlist(iv,  32)
        self.R   = 97 * [0]
        self.clk = 0
    
    def load(self):   
        while self.IN:
            self.clock()
        if self.dbg:
            #print('W init: 0x%.32x' % bitlist_to_uint(self.R))
            print(f'W init : {hex(int("".join([str(x) for x in self.R[::-1]]), 2))}')
    
    def clock(self):
        # compute input bit
        inp = self.R[0] ^ self.f() ^ self.IN.pop()
        # shift LFSR
        self.R = self.R[1:] + [inp]
        self.clk += 1
    
    def f(self):
        return f(
            self.R[4],
            self.R[18],
            self.R[33],
            self.R[57],
            self.R[63],
            self.R[83],
            self.R[96],
            )


class D(LFSR):
    """29 bits D register used for keystream generation in GEA2
    """
    
    def __init__(self, IN):
        self.IN  = IN
        self.R   = 29 * [0]
        self.clk = 0
    
    def load(self):
        while self.IN:
            self.clock(self.IN.pop())
        if all([b == 0 for b in self.R]):
            self.R[0] = 1
        if self.dbg:
            #print('D init: 0x%.16x' % bitlist_to_uint(self.R))
            print(f'D init : {hex(int("".join([str(x) for x in self.R[::-1]]), 2))}')
    
    def clock(self, inp=None):
        if inp is not None:
            R0 = self.R[0] ^ inp
        else:
            R0 = self.R[0]
        # feedback
        if R0:
            self.R[1]  ^= R0
            self.R[4]  ^= R0
            self.R[5]  ^= R0
            self.R[6]  ^= R0
            self.R[7]  ^= R0
            self.R[8]  ^= R0
            self.R[9]  ^= R0
            self.R[10] ^= R0
            self.R[12] ^= R0
            self.R[14] ^= R0
            self.R[16] ^= R0
            self.R[17] ^= R0
            self.R[20] ^= R0
            self.R[21] ^= R0
            self.R[23] ^= R0
            self.R[26] ^= R0
            self.R[28] ^= R0
        # shift
        self.R = self.R[1:] + [R0]
        self.clk += 1
    
    def f(self):
        return f(
            self.R[12],
            self.R[23],
            self.R[3],
            self.R[0],
            self.R[10],
            self.R[27],
            self.R[17],
            )


#------------------------------------------------------------------------------#
# GEA
#------------------------------------------------------------------------------#

class GEA1:
    """GPRS Encryption Algorithm 1
    
    Warning: this is a highly insecure encryption algorithm, providing only 40
    bits of security from a key of 64 bits.
    This algorithm should not be used in production systems.
    """
    
    def __init__(self, iv, dir, key):
        """
        Args:
            iv : uint32 integral value
            dir: 0 or 1
            key: uint64 integral value
        """
        self._iv, self._dir, self._key = iv, dir, key
        # initialization phase
        self.S = S(iv, dir, key)
        self.S.load()
        self.A = A(list(reversed(self.S.R)))
        self.A.load()
        self.B = B(list(reversed(self.S.R[16:] + self.S.R[:16])))
        self.B.load()
        self.C = C(list(reversed(self.S.R[32:] + self.S.R[:32])))
        self.C.load()
    
    def gen(self, bl):
        """
        Args:
            bl : keystream length in bits required
        
        Returns:
            keystream : list of bits (0 or 1)
        """
        # keystream generation phase
        self.K = []
        for i in range(bl):
            self.K.append(self.A.f() ^ self.B.f() ^ self.C.f())
            self.A.clock()
            self.B.clock()
            self.C.clock()
        return list(reversed(self.K))


class GEA2:
    """GPRS Encryption Algorithm 2
    
    Warning: this is an old and quite insecure encryption algorithm, providing a
    of less than the 64 bits of key.
    This algorithm should not be used in production systems.
    """
    
    def __init__(self, iv, dir, key):
        """
        Args:
            iv : uint32 integral value
            dir: 0 or 1
            key: uint64 integral value
        """
        self._iv, self._dir, self._key = iv, dir, key
        # initialization phase
        self.W = W(iv, dir, key)
        self.W.load()
        self.D = D(list(reversed(self.W.R)))
        self.D.load()
        self.A = A(list(reversed(self.W.R[16:] + self.W.R[:16])))
        self.A.load()
        self.B = B(list(reversed(self.W.R[33:] + self.W.R[:33])))
        self.B.load()
        self.C = C(list(reversed(self.W.R[51:] + self.W.R[:51])))
        self.C.load()
    
    def gen(self, bl):
        """
        Args:
            bl : keystream length in bits required
        
        Returns:
            keystream : list of bits (0 or 1)
        """
        # keystream generation phase
        self.K = []
        for i in range(bl):
            self.K.append(self.A.f() ^ self.B.f() ^ self.C.f() ^ self.D.f())
            self.A.clock()
            self.B.clock()
            self.C.clock()
            self.D.clock()
        return list(reversed(self.K))
