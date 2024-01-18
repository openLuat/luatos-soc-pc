/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#define LWIP_TIMEVAL_PRIVATE 1
#define LWIP_ERRNO_STDINCLUDE	1
#define LWIP_NO_LIMITS_H 1
#define LWIP_NO_UNISTD_H 1
#define LWIP_SOCKET_EXTERNAL_HEADERS 1
#define LWIP_SOCKET_EXTERNAL_HEADER_INET_H "inaddr.h"

 /* 选择小端模式 */
 #define BYTE_ORDER LITTLE_ENDIAN

//  /* define compiler specific symbols */
//  #if defined (__ICCARM__)

 #define PACK_STRUCT_BEGIN
 #define PACK_STRUCT_STRUCT
 #define PACK_STRUCT_END
 #define PACK_STRUCT_FIELD(x) x
 #define PACK_STRUCT_USE_INCLUDES

//  #elif defined (__CC_ARM)

//  #define PACK_STRUCT_BEGIN __packed
//  #define PACK_STRUCT_STRUCT
//  #define PACK_STRUCT_END
//  #define PACK_STRUCT_FIELD(x) x

//  #elif defined (__GNUC__)

//  #define PACK_STRUCT_BEGIN
//  #define PACK_STRUCT_STRUCT __attribute__ ((__packed__))
//  #define PACK_STRUCT_END
//  #define PACK_STRUCT_FIELD(x) x

//  #elif defined (__TASKING__)

//  #define PACK_STRUCT_BEGIN
//  #define PACK_STRUCT_STRUCT
//  #define PACK_STRUCT_END
//  #define PACK_STRUCT_FIELD(x) x

//  #endif

extern unsigned int lwip_port_rand(void);
#define LWIP_RAND() (lwip_port_rand())


#endif /* LWIP_ARCH_CC_H */
