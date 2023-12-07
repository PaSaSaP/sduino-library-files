/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@arduino.cc>
 * Copyright (c) 2014 by Paul Stoffregen <paul@pjrc.com> (Transaction API)
 * Copyright (c) 2014 by Matthijs Kooijman <matthijs@stdin.nl> (SPISettings AVR)
 * Copyright (c) 2014 by Andrew J. Kroll <xxxajk@gmail.com> (atomicity fixes)
 * SPI Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

//#include "pins_arduino.h"
#include "SPI.h"

#ifdef DONT_USE_ASSEMBLER
void SPI_transfer_s(void *buf, size_t count) {
    uint8_t *p;
    uint8_t in,out;

    if (count == 0) return;
    p = (uint8_t *)buf;
    SPI->DR = *p;                       // start sending the first byte
    while (--count > 0) {
      out = *(p + 1);
      while ((SPI->SR & SPI_SR_TXE)==0) ; // wait for transmit buffer empty
          SPI->DR = out;                // second byte into transmit buffer
      while ((SPI->SR & SPI_SR_RXNE)==0) ; // wait for first received byte
      in = SPI->DR;
      *p++ = in;
    }
    while ((SPI->SR & SPI_SR_RXNE)==0) ; // wait for last received byte
    *p = SPI->DR;
  }
#else
/*
 * to allow for an (almost) continues transfer the the following data byte
 * is written to TX before the received byte is read. So we have to make
 * sure to read the RX buffer within less than 16 clock cycles from the
 * moment of writing the TX data. This requires an interrupt block.
 *
 * pseudocode:

        init x,y
        dr = *x
        while (--y)
        {
                wait for txe, dr=x[1]
                wait for rxne, *x = dr
                x++
        }
        wait for rxne, *x = dr

 */
#if 0
void SPI_transfer_asm(void *buf, size_t count) {
        (void) buf;             // dummy code to avoid unreferenced
        (void) count;           // function argument warning
__asm
        ldw     y,(5,SP)
        jreq    $00001
        ldw     x,(3,SP)
        ld      a,(x)           ; SPI->DR = *p
        ld      0x5204,a
$00002:
        decw    y               ; while (--count>0) {
        jreq    $00003
        ld      a,(1,x)         ;   out = *(p+1)
        btjf    0x5203,#1,.     ;   while (!(SPI->SR & SPI_SR_TXE));
        sim                     ;   make sure to avoid a possible
                                ;   race condition to not miss the
                                ;   received data
        ld      0x5204,a        ;   SPI->DR = out
        btjf    0x5203,#0,.     ;   while (!(SPI->SR & SPI_SR_RXNE));
        ld      a,0x5204        ;   in = SPI->DR
        rim
        ld      (x),a           ;   *p++ = in
        incw    x
        jra     $00002          ; } // while
$00003:
        btjf    0x5203,#0,.     ; while (!(SPI->SR & SPI_SR_RXNE));
        ld      a,0x5204        ; *p = SPI->DR
        ld      (x),a
$00001:
__endasm;
}
#endif

#if 1
void SPI_transfer_asm(void *buf, size_t count) {
        (void) buf;             // dummy code to avoid unreferenced
        (void) count;           // function argument warning
__asm
        ldw     y,(5,SP)
        jreq    $00001
        ldw     x,(3,SP)
        ld      a,(x)           ; SPI->DR = *p
        ld      0x5204,a
        jra     $00003          ; start the while loop
$00002:
        ld      a,(1,x)         ;   out = *(p+1)
        btjf    0x5203,#1,.     ;   while (!(SPI->SR & SPI_SR_TXE));
        sim                     ;   make sure to avoid a possible
                                ;   race condition to not miss the
                                ;   received data
        ld      0x5204,a        ;   SPI->DR = out
        btjf    0x5203,#0,.     ;   while (!(SPI->SR & SPI_SR_RXNE));
        ld      a,0x5204        ;   in = SPI->DR
        rim
        ld      (x),a           ;   *p++ = in
        incw    x
$00003:
        decw    y
        jrne    $00002          ; } // while
        btjf    0x5203,#0,.     ; while (!(SPI->SR & SPI_SR_RXNE));
        ld      a,0x5204        ; *p = SPI->DR
        ld      (x),a
$00001:
__endasm;
}
#endif

#endif

#if 0
// slightly simplyfied functon:
__asm
        ldw     y,#8
        ldw     x,#_buf
        ld      a,(x)           ; dr = *x
        ld      0x5204,a
$00030:                         ; while (--y) {
        decw    y
        jreq    $00031
        btjf    0x5203,#1,.     ;   while (!(SPI->SR & SPI_SR_TXE));
        ld      a,(1,x)
        ld      0x5204,a
        btjf    0x5203,#0,.     ;   while (!(SPI->SR & SPI_SR_RXNE));
        ld      a,0x5204        ;   *x = dr
        ld      (x),a
        incw    x               ;   x++
        jra     $00030          ; }
$00031:
        btjf    0x5203,#0,.     ; while (!(SPI->SR & SPI_SR_RXNE));
        ld      a,0x5204        ; *x = dr
        ld      (x),a
__endasm;
#endif

