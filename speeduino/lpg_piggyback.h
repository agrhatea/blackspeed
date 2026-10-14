#pragma once

/**
 * @file
 * @brief LPG piggyback - step 1/1b/2: measure petrol injector pulse (channel 1) and
 *        derive a gas pulse that starts at the petrol injector OFF edge.
 *
 * DRAFT, belum dikompilasi atau diuji pada langkah 2.
 *
 * Sinyal injektor bensin (sisi ECM, kaki negatif) harus dikondisikan dulu ke level 3.3V
 * (pembagi + penjepit), sehingga: injektor ON = LOW, injektor OFF = HIGH.
 */

#include "globals.h"

#if defined(STM32F411xE)

/** Pin input sinyal injektor bensin kanal 1 (sebelumnya pinLaunch di blok F411 pinMapping.cpp).
 *  Jangan aktifkan launch control di TunerStudio selama pin ini dipakai. */
#define LPG_PIN_PETROL_INJ1 PB15

/** Pin driver injektor gas kanal 1. HARUS sama dengan injektor 1 di blok F411 pinMapping.cpp. */
#define LPG_PIN_GAS_INJ1 PB7

/** 0 = pulsa gas hanya DIHITUNG dan dicatat di log (gasPW1), pin tidak digerakkan (aman).
 *  1 = pin LPG_PIN_GAS_INJ1 digerakkan. Mulai uji dengan LED/beban pengganti, bukan injektor gas. */
#ifndef LPG_GAS_OUTPUT_ENABLE
#define LPG_GAS_OUTPUT_ENABLE 0
#endif

/** Panggil sekali di setup(), setelah initialiseAll(). */
void lpgInit(void);

/** Panggil tiap putaran loop(). Menyalin hasil ukur ke currentStatus. */
void lpgUpdate(statuses &current);

/** True = perhitungan bahan bakar Speeduino dimatikan (computePulseWidths mengembalikan 0).
 *  Di varian LPG ini injektor Speeduino hanya untuk gas; pulsa gas digerakkan modul ini. */
static inline bool lpgOverridesFuel(void) { return true; }

#else

// Platform lain (AVR, native, dll): tidak melakukan apa-apa
static inline void lpgInit(void) {}
static inline void lpgUpdate(statuses &) {}
static inline bool lpgOverridesFuel(void) { return false; }

#endif
