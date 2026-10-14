#include "lpg_piggyback.h"

#if defined(STM32F411xE)

#include <Arduino.h>
#include <HardwareTimer.h>

namespace {
  // ---- Pengukuran injektor bensin ----
  // Pulsa yang lebih pendek dari ini dianggap noise/ringing setelah flyback
  constexpr uint32_t MIN_EDGE_GAP_US = 50U;
  // Pulsa bensin lebih panjang dari ini dianggap tidak valid
  constexpr uint32_t MAX_PW_US = 60000U;
  // Selang antar injeksi lebih panjang dari ini dianggap tidak valid
  constexpr uint32_t MAX_PERIOD_US = 2000000U;
  // Tidak ada pulsa selama ini => mesin dianggap berhenti
  constexpr uint32_t PULSE_TIMEOUT_US = 500000U;

  // ---- Pulsa gas (K tetap 1.0 dulu; tabel K menyusul) ----
  constexpr uint32_t LPG_K_NUM = 100U;
  constexpr uint32_t LPG_K_DEN = 100U;
  // Pulsa bensin lebih pendek dari ini: tidak ada pulsa gas (mis. injeksi ekstra/glitch)
  constexpr uint32_t LPG_MIN_PETROL_PW_US = 200U;
  // Batas atas pulsa gas (timer 16-bit pada 1 MHz maksimal 65 ms)
  constexpr uint32_t LPG_MAX_GAS_PW_US = 20000U;

  volatile uint32_t tInjOn = 0U;      // waktu injektor menyala (tepi turun)
  volatile uint32_t tLastEdge = 0U;   // waktu tepi terakhir yang diterima
  volatile uint16_t lastPw = 0U;      // lebar pulsa valid terakhir (uS)
  volatile uint32_t lastPeriod = 0U;  // selang antar awal injeksi bensin terakhir (uS)
  volatile uint16_t lastGasPw = 0U;   // pulsa gas terhitung terakhir (uS)
  volatile uint8_t  gasOverruns = 0U; // pulsa gas yang dilewati karena pulsa sebelumnya masih jalan
  volatile bool haveInjOn = false;    // sudah melihat tepi turun sebelum tepi naik
  volatile bool havePrevOn = false;   // sudah ada tepi turun sebelumnya (untuk hitung periode)
  volatile bool gasActive = false;    // pulsa gas sedang berjalan
}

// TIM9: belum dipakai di board_stm32_official.cpp. Free-running 1 MHz, dipakai compare kanal 1.
static HardwareTimer lpgTimer(TIM9);

// Dipanggil saat compare tercapai: akhiri pulsa gas
static void gasPulseEnd(void)
{
  TIM9->DIER &= ~TIM_DIER_CC1IE;
  digitalWrite(LPG_PIN_GAS_INJ1, LOW);
  gasActive = false;
}

// Mulai pulsa gas dari lebar pulsa bensin (uS). Dipanggil dari ISR tepi naik injektor bensin.
static inline void startGasPulse(uint32_t petrolPw)
{
  uint32_t gasPw = (petrolPw * LPG_K_NUM) / LPG_K_DEN;
  if (gasPw > LPG_MAX_GAS_PW_US) { gasPw = LPG_MAX_GAS_PW_US; }
  if (petrolPw < LPG_MIN_PETROL_PW_US) { gasPw = 0U; }
  lastGasPw = (uint16_t)gasPw;

#if LPG_GAS_OUTPUT_ENABLE
  if (gasPw == 0U) { return; }
  if (gasActive) {
    if (gasOverruns < 255U) { gasOverruns++; }
    return;
  }
  gasActive = true;
  digitalWrite(LPG_PIN_GAS_INJ1, HIGH);
  // Counter free-running 1 MHz: compare = sekarang + lebar pulsa (+2 uS untuk waktu eksekusi)
  TIM9->CCR1 = (uint16_t)(TIM9->CNT + gasPw + 2U);
  TIM9->DIER |= TIM_DIER_CC1IE;
#endif
}

static void petrolInjIsr(void)
{
  const uint32_t now = micros();
  if ((uint32_t)(now - tLastEdge) < MIN_EDGE_GAP_US) { return; }
  tLastEdge = now;

  if (digitalRead(LPG_PIN_PETROL_INJ1) == LOW) {
    // Injektor ON: hitung periode dari tepi turun sebelumnya
    if (havePrevOn) {
      const uint32_t period = now - tInjOn;
      if (period <= MAX_PERIOD_US) { lastPeriod = period; }
    }
    havePrevOn = true;
    tInjOn = now;
    haveInjOn = true;
  } else if (haveInjOn) {
    // Injektor OFF: hitung lebar pulsa, lalu mulai pulsa gas
    const uint32_t pw = now - tInjOn;
    haveInjOn = false;
    if (pw <= MAX_PW_US) {
      lastPw = (uint16_t)pw;
      startGasPulse(pw);
    }
  } else {
    // tepi naik tanpa tepi turun: abaikan
  }
}

void lpgInit(void)
{
  // Pin driver gas: mulai dalam keadaan mati
  pinMode(LPG_PIN_GAS_INJ1, OUTPUT);
  digitalWrite(LPG_PIN_GAS_INJ1, LOW);

  // Timer pulsa gas: 1 MHz, free-running, compare kanal 1 tanpa pin
  lpgTimer.setPrescaleFactor(lpgTimer.getTimerClkFreq() / 1000000U);
  lpgTimer.setOverflow(0xFFFFU, TICK_FORMAT);
  lpgTimer.setMode(1, TIMER_OUTPUT_COMPARE);
  lpgTimer.attachInterrupt(1, gasPulseEnd);
  TIM9->DIER &= ~TIM_DIER_CC1IE;  // mulai nonaktif
  lpgTimer.resume();

  // Input injektor bensin
  pinMode(LPG_PIN_PETROL_INJ1, INPUT);
  attachInterrupt(digitalPinToInterrupt(LPG_PIN_PETROL_INJ1), petrolInjIsr, CHANGE);
}

void lpgUpdate(statuses &current)
{
  const uint32_t now = micros();
  // Baca nilai bersama secara konsisten (32-bit dan 16-bit aligned: atomik di Cortex-M4)
  const uint32_t lastEdge = tLastEdge;
  const uint16_t pw = lastPw;
  const uint32_t period = lastPeriod;
  const uint16_t gasPw = lastGasPw;

  const bool alive = ((uint32_t)(now - lastEdge) < PULSE_TIMEOUT_US);
  current.petrolPW1 = alive ? pw : 0U;
  current.petrolPeriod1 = alive ? period : 0U;
  current.gasPW1 = alive ? gasPw : 0U;
  current.gasOverruns = gasOverruns;
}

#endif
