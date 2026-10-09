#ifndef STM32F411_CUSTOM_H
#define STM32F411_CUSTOM_H
#if defined(CORE_STM32F411_CUSTOM)

/*
 * Draft board file: Speeduino -> STM32F411CEU6 (Blackpill), core STM32duino.
 * Diisi dari board_template.h. BELUM DIKOMPILASI - lihat catatan "VERIFY".
 *
 * Alokasi timer (4 injektor + 4 ignition, mengikuti pola tabel F4 di board_stm32_official):
 *   TIM1  : CH1 cadangan (fan) | CH2 BOOST | CH3 VVT | CH4 IDLE
 *   TIM2  : CH1-4 IGN1-4   (32-bit, tetapi overflow dibatasi 16-bit)
 *   TIM3  : CH1-4 INJ1-4
 *   TIM11 : interval 1 ms (oneMSInterval)
 * Semua timer hanya dipakai untuk compare-interrupt (tanpa pin AF); pin output
 * di-toggle di ISR oleh firmware, jadi pin INJ/IGN/BOOST/IDLE bebas GPIO biasa.
 */

#include <Arduino.h>
#include <HardwareTimer.h>

/*
***********************************************************************************************************
* General
*/
  #define TS_SERIAL_BUFFER_SIZE 517 //Cukup untuk transfer SD (512 + 1 flag + 4 sector)
  #define FPU_MAX_SIZE 32           //VERIFY: F411 punya FPU; sesuaikan dengan nilai di board_stm32_official.h
  #define BOARD_MAX_IO_PINS  (NUM_DIGITAL_PINS + 1) //VERIFY: lihat definisi NUM_DIGITAL_PINS di variant Blackpill
  #define BOARD_MAX_DIGITAL_PINS NUM_DIGITAL_PINS
  #define EEPROM_LIB_H <EEPROM.h>   //VERIFY: F411 tidak punya EEPROM; library ini = emulasi di flash (atau ganti SPI flash/FRAM)
  class EEPROMClass;
  using EEPROM_t = EEPROMClass;

  // Pin terlarang: USB (PA11/PA12), SWD (PA13/PA14), BOOT1 (PB2), kristal LSE (PC14/PC15)
  static inline bool pinIsReserved(uint8_t pin) {
    return (pin == PA11) || (pin == PA12) || (pin == PA13) || (pin == PA14)
        || (pin == PB2)  || (pin == PC14) || (pin == PC15);
  }

/*
***********************************************************************************************************
* Timer tick
*/
  #define TIMER_TICK_HZ 250000UL                 //1 tick = 4 uS
  #define MAX_TIMER_TICKS 0xFFFFUL               //Overflow 16-bit agar kompatibel dengan scheduler (COMPARE_TYPE 16-bit)
  #define uS_TO_TIMER_COMPARE(uS) ((uS) >> 2)    //4 uS per tick

/*
***********************************************************************************************************
* Schedules
*/
  //Fuel: TIM3
  #define FUEL1_COUNTER (TIM3)->CNT
  #define FUEL2_COUNTER (TIM3)->CNT
  #define FUEL3_COUNTER (TIM3)->CNT
  #define FUEL4_COUNTER (TIM3)->CNT

  #define IGN1_COUNTER  (TIM2)->CNT
  #define IGN2_COUNTER  (TIM2)->CNT
  #define IGN3_COUNTER  (TIM2)->CNT
  #define IGN4_COUNTER  (TIM2)->CNT

  #define FUEL1_COMPARE (TIM3)->CCR1
  #define FUEL2_COMPARE (TIM3)->CCR2
  #define FUEL3_COMPARE (TIM3)->CCR3
  #define FUEL4_COMPARE (TIM3)->CCR4

  #define IGN1_COMPARE  (TIM2)->CCR1
  #define IGN2_COMPARE  (TIM2)->CCR2
  #define IGN3_COMPARE  (TIM2)->CCR3
  #define IGN4_COMPARE  (TIM2)->CCR4

  static inline void FUEL1_TIMER_ENABLE(void)  { (TIM3)->DIER |= TIM_DIER_CC1IE; }
  static inline void FUEL2_TIMER_ENABLE(void)  { (TIM3)->DIER |= TIM_DIER_CC2IE; }
  static inline void FUEL3_TIMER_ENABLE(void)  { (TIM3)->DIER |= TIM_DIER_CC3IE; }
  static inline void FUEL4_TIMER_ENABLE(void)  { (TIM3)->DIER |= TIM_DIER_CC4IE; }

  static inline void FUEL1_TIMER_DISABLE(void) { (TIM3)->DIER &= ~TIM_DIER_CC1IE; }
  static inline void FUEL2_TIMER_DISABLE(void) { (TIM3)->DIER &= ~TIM_DIER_CC2IE; }
  static inline void FUEL3_TIMER_DISABLE(void) { (TIM3)->DIER &= ~TIM_DIER_CC3IE; }
  static inline void FUEL4_TIMER_DISABLE(void) { (TIM3)->DIER &= ~TIM_DIER_CC4IE; }

  static inline void IGN1_TIMER_ENABLE(void)   { (TIM2)->DIER |= TIM_DIER_CC1IE; }
  static inline void IGN2_TIMER_ENABLE(void)   { (TIM2)->DIER |= TIM_DIER_CC2IE; }
  static inline void IGN3_TIMER_ENABLE(void)   { (TIM2)->DIER |= TIM_DIER_CC3IE; }
  static inline void IGN4_TIMER_ENABLE(void)   { (TIM2)->DIER |= TIM_DIER_CC4IE; }

  static inline void IGN1_TIMER_DISABLE(void)  { (TIM2)->DIER &= ~TIM_DIER_CC1IE; }
  static inline void IGN2_TIMER_DISABLE(void)  { (TIM2)->DIER &= ~TIM_DIER_CC2IE; }
  static inline void IGN3_TIMER_DISABLE(void)  { (TIM2)->DIER &= ~TIM_DIER_CC3IE; }
  static inline void IGN4_TIMER_DISABLE(void)  { (TIM2)->DIER &= ~TIM_DIER_CC4IE; }

  //Channel 5-8 (opsional): F411 masih punya TIM4 (IGN5-8) dan TIM5 (INJ5-8) jika kelak dibutuhkan.

/*
***********************************************************************************************************
* Auxiliaries (TIM1)
*/
  #define ENABLE_BOOST_TIMER()   ((TIM1)->DIER |= TIM_DIER_CC2IE)
  #define DISABLE_BOOST_TIMER()  ((TIM1)->DIER &= ~TIM_DIER_CC2IE)

  #define ENABLE_VVT_TIMER()     ((TIM1)->DIER |= TIM_DIER_CC3IE)
  #define DISABLE_VVT_TIMER()    ((TIM1)->DIER &= ~TIM_DIER_CC3IE)

  #define BOOST_TIMER_COMPARE    (TIM1)->CCR2
  #define BOOST_TIMER_COUNTER    (TIM1)->CNT
  #define VVT_TIMER_COMPARE      (TIM1)->CCR3
  #define VVT_TIMER_COUNTER      (TIM1)->CNT

/*
***********************************************************************************************************
* Idle (TIM1 CH4)
*/
  #define IDLE_COUNTER           (TIM1)->CNT
  #define IDLE_COMPARE           (TIM1)->CCR4

  #define IDLE_TIMER_ENABLE()    ((TIM1)->DIER |= TIM_DIER_CC4IE)
  #define IDLE_TIMER_DISABLE()   ((TIM1)->DIER &= ~TIM_DIER_CC4IE)

/*
***********************************************************************************************************
* Timer objects & ISR wrappers (didefinisikan di board_stm32f411_custom.cpp)
*/
  extern HardwareTimer Timer1;
  extern HardwareTimer Timer2;
  extern HardwareTimer Timer3;
  extern HardwareTimer Timer11;

  void oneMSInterval(HardwareTimer*);
  void boostInterrupt(HardwareTimer*);
  void vvtInterrupt(HardwareTimer*);
  void idleInterrupt(HardwareTimer*);
  void fuelSchedule1Interrupt(HardwareTimer*);
  void fuelSchedule2Interrupt(HardwareTimer*);
  void fuelSchedule3Interrupt(HardwareTimer*);
  void fuelSchedule4Interrupt(HardwareTimer*);
  void ignitionSchedule1Interrupt(HardwareTimer*);
  void ignitionSchedule2Interrupt(HardwareTimer*);
  void ignitionSchedule3Interrupt(HardwareTimer*);
  void ignitionSchedule4Interrupt(HardwareTimer*);

/*
***********************************************************************************************************
* CAN / Second serial
*/
  //F411 tidak punya CAN native. Serial2 (HC-05) = USART1 di PA9 (TX) / PA10 (RX).

#endif //CORE_STM32F411_CUSTOM
#endif //STM32F411_CUSTOM_H
