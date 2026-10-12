#include "lpg_piggyback.h"

#if defined(STM32F411xE)

#include <Arduino.h>

namespace {
  // Pulsa yang lebih pendek dari ini dianggap noise/ringing setelah flyback
  constexpr uint32_t MIN_EDGE_GAP_US = 50U;
  // Pulsa bensin lebih panjang dari ini dianggap tidak valid
  constexpr uint32_t MAX_PW_US = 60000U;
  // Tidak ada pulsa selama ini => mesin dianggap berhenti
  constexpr uint32_t PULSE_TIMEOUT_US = 500000U;

  volatile uint32_t tInjOn = 0U;      // waktu injektor menyala (tepi turun)
  volatile uint32_t tLastEdge = 0U;   // waktu tepi terakhir yang diterima
  volatile uint16_t lastPw = 0U;      // lebar pulsa valid terakhir (uS)
  volatile bool haveInjOn = false;    // sudah melihat tepi turun sebelum tepi naik
}

static void petrolInjIsr(void)
{
  const uint32_t now = micros();
  if ((uint32_t)(now - tLastEdge) < MIN_EDGE_GAP_US) { return; }
  tLastEdge = now;

  if (digitalRead(LPG_PIN_PETROL_INJ1) == LOW) {
    // Injektor ON
    tInjOn = now;
    haveInjOn = true;
  } else if (haveInjOn) {
    // Injektor OFF: hitung lebar pulsa
    const uint32_t pw = now - tInjOn;
    haveInjOn = false;
    if (pw <= MAX_PW_US) { lastPw = (uint16_t)pw; }
  } else {
    // tepi naik tanpa tepi turun: abaikan
  }
}

void lpgInit(void)
{
  pinMode(LPG_PIN_PETROL_INJ1, INPUT);
  attachInterrupt(digitalPinToInterrupt(LPG_PIN_PETROL_INJ1), petrolInjIsr, CHANGE);
}

void lpgUpdate(statuses &current)
{
  const uint32_t now = micros();
  // Baca tLastEdge dan lastPw secara konsisten (32-bit dan 16-bit aligned: atomik di Cortex-M4)
  const uint32_t lastEdge = tLastEdge;
  const uint16_t pw = lastPw;

  current.petrolPW1 = ((uint32_t)(now - lastEdge) < PULSE_TIMEOUT_US) ? pw : 0U;
}

#endif
