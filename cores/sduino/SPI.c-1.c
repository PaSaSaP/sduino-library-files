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

static uint8_t SPI_initialized;// = 0;
// static uint8_t SPI_interruptMode;// = 0; // not used
// static uint8_t SPI_interruptMask;// = 0; // not used
// static uint8_t SPI_interruptSave;// = 0; // not used
#ifdef SPI_TRANSACTION_MISMATCH_LED
uint8_t SPI_inTransactionFlag;// = 0;
#endif

// from 1.8.0
void SPI_begin(void)
{
  uint8_t port;
  uint8_t bit;
  volatile uint8_t *reg;

  BEGIN_CRITICAL        // Protect from a scheduler and prevent transactionBegin
  if (!SPI_initialized) {
    // Set SS to high so a connected chip will be "deselected" by default
    port = digitalPinToPort(SS);
    bit = digitalPinToBitMask(SS);
    reg = portModeRegister(port);

    // if the SS pin is not already configured as an output
    // then set it high (to enable the internal pull-up resistor)
    if(!(*reg & bit)){
      digitalWrite(SS, HIGH);
    }

    // When the SS pin is set as OUTPUT, it can be used as
    // a general purpose output port (it doesn't influence
    // SPI operations).
    pinMode(SS, OUTPUT);

    // using software slave select frees the SS pins to be used as a general
    // IO pin without interfering with the SPI logic.
    SPI->CR2 = SPI_CR2_SSI | SPI_CR2_SSM;

    // Warning: if the SS pin ever becomes a LOW INPUT then SPI
    // automatically switches to Slave, so the data direction of
    // the SS pin MUST be kept as OUTPUT.
    SPI->CR1 |= SPI_CR1_MSTR | SPI_CR1_SPE;

    // Set direction register for SCK and MOSI pin.
    // MISO pin automatically overrides to INPUT.
    // By doing this AFTER enabling SPI, we avoid accidentally
    // clocking in a single bit since the lines go directly
    // from "input" to SPI control.
    // http://code.google.com/p/arduino/issues/detail?id=888
    pinMode(SCK, OUTPUT);
    pinMode(MOSI, OUTPUT);
  }
  SPI_initialized++; // reference count
  END_CRITICAL
}

void SPI_end(void) {
  BEGIN_CRITICAL
  // Decrease the reference counter
  if (SPI_initialized)
    SPI_initialized--;
  // If there are no more references disable SPI
  if (!SPI_initialized) {
    // follow the procedure recommended in RM0016, section 20.3.8:
    // Wait until RXNE = 1 to receive the last data
//    while ((SPI->SR&SPI_SR_RXNE)==0);
    // Wait until TXE = 1
    while ((SPI->SR&SPI_SR_TXE)==0);
    // Then wait until BSY = 0
    while (SPI->SR&SPI_SR_BSY);
    // Disable the SPI
    SPI->CR1 &= ~SPI_CR1_SPE;
    // SPI_interruptMode = 0;
    #ifdef SPI_TRANSACTION_MISMATCH_LED
    SPI_inTransactionFlag = 0;
    #endif
  }
  END_CRITICAL
}

