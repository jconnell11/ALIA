// jhcBitMacros.h : handles packing and unpacking 16 and 32 values
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2004 IBM Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 
///////////////////////////////////////////////////////////////////////////

#ifndef _JHCBITMACROS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCBITMACROS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


// Whether a big-endian processor architecture is in use.
// uncomment the following line for big-endian architectures (e.g. PPC, 68K)
// alternatively define this macro directly in the compiler options

// #define BIG_ENDIAN 


#ifndef BIG_ENDIAN 

// little-endian byte extraction and insertion (JHC)
// the numbers refer to the read order for unsigned char buffers
// possibly change these for non-x86 machine types

//= Read 16 bit LSB (little-endian). 
#define SBYTE0(v) ((v) & 0x00FF)    
//= Read 16 bit MSB (little-endian). 
#define SBYTE1(v) ((v) >> 8)                 

//= Read 32 bit LSB (little-endian). 
#define BYTE0(v) ( (v)        & 0xFF)       
//= Read 32 bit byte 1 (little-endian). 
#define BYTE1(v) (((v) >> 8)  & 0xFF)        
//= Read 32 bit byte 2 (little-endian). 
#define BYTE2(v) (((v) >> 16) & 0xFF)        
//= Read 32 bit MSB (little-endian).    
#define BYTE3(v) ( (v) >> 24)                

//= Write 32 bit LSB (little-endian).   
#define MBYTE0(v)  (v)                       
//= Write 32 bit byte 1 (little-endian).
#define MBYTE1(v) ((v) << 8)                 
//= Write 32 bit byte 2 (little-endian).
#define MBYTE2(v) ((v) << 16)                
//= Write 32 bit MSB (little-endian).   
#define MBYTE3(v) ((v) << 24)                

//= Offset for a byte (little-endian).
#define BYTEOFF(v, w) (v)                    


#else

// big-endian versions of above utilities

//= Read 16 bit MSB (big-endian). 
#define SBYTE1(v) ((v) & 0x00FF)             
//= Read 16 bit LSB (big-endian). 
#define SBYTE0(v) ((v) >> 8)                 

//= Read 32 bit MSB (big-endian).    
#define BYTE3(v) ( (v)        & 0xFF)  
//= Read 32 bit byte 2 (big-endian).       
#define BYTE2(v) (((v) >> 8)  & 0xFF)       
//= Read 32 bit byte 1 (big-endian).  
#define BYTE1(v) (((v) >> 16) & 0xFF)       
//= Read 32 bit LSB (big-endian).    
#define BYTE0(v) ( (v) >> 24)                

//= Write 32 bit MSB (big-endian).    
#define MBYTE3(v)  (v)                       
//= Write 32 bit byte 2 (big-endian). 
#define MBYTE2(v) ((v) << 8)                 
//= Write 32 bit byte 1 (big-endian). 
#define MBYTE1(v) ((v) << 16)                
//= Write 32 bit LSB (big-endian).    
#define MBYTE0(v) ((v) << 24)                

//= Offset for a byte (big-endian). 
#define BYTEOFF(v, w) (((w) == 3) ? (v) : ((w) - 1 - (v)))  


#endif


#endif




