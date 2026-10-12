#pragma once

/**
 * @file
 * @brief LPG piggyback - step 1: measure the petrol injector pulse width (channel 1).
 *
 * DRAFT, belum dikompilasi atau diuji.
 *
 * Sinyal injektor bensin (sisi ECM, kaki negatif) harus dikondisikan dulu ke level 3.3V
 * (pembagi + penjepit), sehingga: injektor ON = LOW, injektor OFF = HIGH.
 */

#include "globals.h"

#if defined(STM32F411xE)

/** Pin input sinyal injektor bensin kanal 1 (sebelumnya pinLaunch di blok F411 pinMapping.cpp).
 *  Jangan aktifkan launch control di TunerStudio selama pin ini dipakai. */
#define LPG_PIN_PETROL_INJ1 PB15

/** Panggil sekali di setup(), setelah initialiseAll(). */
void lpgInit(void);

/** Panggil tiap putaran loop(). Menyalin hasil ukur ke currentStatus.petrolPW1 (uS, 0 = tidak ada pulsa). */
void lpgUpdate(statuses &current);

#else

// Platform lain (AVR, native, dll): tidak melakukan apa-apa
static inline void lpgInit(void) {}
static inline void lpgUpdate(statuses &) {}

#endif
