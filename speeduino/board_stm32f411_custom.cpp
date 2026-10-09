#include "board_definition.h"

#if defined(CORE_STM32F411_CUSTOM)

#include "globals.h"
#include "auxiliaries.h"
#include "idle.h"
#include "scheduler.h"

HardwareTimer Timer1(TIM1);
HardwareTimer Timer2(TIM2);
HardwareTimer Timer3(TIM3);
HardwareTimer Timer11(TIM11);

// Wrapper ISR: pola sama dengan board_stm32_official.ino.
// VERIFY: jika core STM32duino Anda (2.x) menolak callback ber-argumen HardwareTimer*,
// ganti wrapper ini dengan lambda: Timer3.attachInterrupt(1, [](){ fuelSchedule1Interrupt(); });
void oneMSInterval(HardwareTimer*)             { oneMSInterval(); }
void boostInterrupt(HardwareTimer*)            { boostInterrupt(); }
void vvtInterrupt(HardwareTimer*)              { vvtInterrupt(); }
void idleInterrupt(HardwareTimer*)             { idleInterrupt(); }
void fuelSchedule1Interrupt(HardwareTimer*)    { fuelSchedule1Interrupt(); }
void fuelSchedule2Interrupt(HardwareTimer*)    { fuelSchedule2Interrupt(); }
void fuelSchedule3Interrupt(HardwareTimer*)    { fuelSchedule3Interrupt(); }
void fuelSchedule4Interrupt(HardwareTimer*)    { fuelSchedule4Interrupt(); }
void ignitionSchedule1Interrupt(HardwareTimer*){ ignitionSchedule1Interrupt(); }
void ignitionSchedule2Interrupt(HardwareTimer*){ ignitionSchedule2Interrupt(); }
void ignitionSchedule3Interrupt(HardwareTimer*){ ignitionSchedule3Interrupt(); }
void ignitionSchedule4Interrupt(HardwareTimer*){ ignitionSchedule4Interrupt(); }

// Timer free-running 250 kHz (4 uS/tick), overflow 16-bit
static void setupTickTimer(HardwareTimer &t)
{
    t.pause();
    t.setPrescaleFactor(t.getTimerClkFreq() / TIMER_TICK_HZ);
    t.setOverflow(MAX_TIMER_TICKS, TICK_FORMAT);
}

void initBoard(uint32_t baudRate)
{
    (void)baudRate; //Serial/USB CDC dimulai oleh firmware; tambahkan Serial.begin(baudRate) di sini bila diperlukan

    /*
    ***********************************************************************************************************
    * General
    */
    analogReadResolution(10); //Tabel kalibrasi Speeduino berbasis ADC 10-bit (VERIFY: cek apakah firmware sudah menanganinya)

    /*
    ***********************************************************************************************************
    * Timers
    */
    setupTickTimer(Timer1);
    setupTickTimer(Timer2);
    setupTickTimer(Timer3);

    // Interval 1 ms (VERIFY: jika variant core memakai TIM11 untuk servo/tone, pindahkan ke TIM9)
    Timer11.pause();
    Timer11.setOverflow(1000, MICROSEC_FORMAT);
    Timer11.attachInterrupt(oneMSInterval);

    /*
    ***********************************************************************************************************
    * Auxiliaries
    */
    Timer1.setMode(2, TIMER_OUTPUT_COMPARE);
    Timer1.attachInterrupt(2, boostInterrupt);
    Timer1.setMode(3, TIMER_OUTPUT_COMPARE);
    Timer1.attachInterrupt(3, vvtInterrupt);

    /*
    ***********************************************************************************************************
    * Idle
    */
    Timer1.setMode(4, TIMER_OUTPUT_COMPARE);
    Timer1.attachInterrupt(4, idleInterrupt);

    /*
    ***********************************************************************************************************
    * Schedules
    */
    Timer3.setMode(1, TIMER_OUTPUT_COMPARE);
    Timer3.attachInterrupt(1, fuelSchedule1Interrupt);
    Timer3.setMode(2, TIMER_OUTPUT_COMPARE);
    Timer3.attachInterrupt(2, fuelSchedule2Interrupt);
    Timer3.setMode(3, TIMER_OUTPUT_COMPARE);
    Timer3.attachInterrupt(3, fuelSchedule3Interrupt);
    Timer3.setMode(4, TIMER_OUTPUT_COMPARE);
    Timer3.attachInterrupt(4, fuelSchedule4Interrupt);

    Timer2.setMode(1, TIMER_OUTPUT_COMPARE);
    Timer2.attachInterrupt(1, ignitionSchedule1Interrupt);
    Timer2.setMode(2, TIMER_OUTPUT_COMPARE);
    Timer2.attachInterrupt(2, ignitionSchedule2Interrupt);
    Timer2.setMode(3, TIMER_OUTPUT_COMPARE);
    Timer2.attachInterrupt(3, ignitionSchedule3Interrupt);
    Timer2.setMode(4, TIMER_OUTPUT_COMPARE);
    Timer2.attachInterrupt(4, ignitionSchedule4Interrupt);

    // Semua compare-interrupt dimulai nonaktif; scheduler yang mengaktifkannya
    FUEL1_TIMER_DISABLE(); FUEL2_TIMER_DISABLE(); FUEL3_TIMER_DISABLE(); FUEL4_TIMER_DISABLE();
    IGN1_TIMER_DISABLE();  IGN2_TIMER_DISABLE();  IGN3_TIMER_DISABLE();  IGN4_TIMER_DISABLE();
    DISABLE_BOOST_TIMER(); DISABLE_VVT_TIMER();   IDLE_TIMER_DISABLE();

    Timer1.resume();
    Timer2.resume();
    Timer3.resume();
    Timer11.resume();
}

extern "C" char *sbrk(int incr);

uint16_t freeRam()
{
    char top;
    uint32_t freeBytes = (uint32_t)&top - (uint32_t)sbrk(0);
    return (freeBytes > 0xFFFFUL) ? 0xFFFF : (uint16_t)freeBytes;
}

void doSystemReset() { NVIC_SystemReset(); }

// Lompat ke bootloader ROM (USB DFU) F411 di system memory 0x1FFF0000.
// VERIFY: uji di bench; urutan deinit bisa perlu disesuaikan dengan core yang dipakai.
void jumpToBootloader()
{
    constexpr uint32_t SYSTEM_MEMORY = 0x1FFF0000UL;
    __disable_irq();
    SysTick->CTRL = 0; SysTick->LOAD = 0; SysTick->VAL = 0;
    for (uint8_t i = 0; i < 8; i++) { NVIC->ICER[i] = 0xFFFFFFFFUL; NVIC->ICPR[i] = 0xFFFFFFFFUL; }
    HAL_RCC_DeInit();
    HAL_DeInit();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();
    __set_MSP(*reinterpret_cast<uint32_t*>(SYSTEM_MEMORY));
    reinterpret_cast<void (*)(void)>(*reinterpret_cast<uint32_t*>(SYSTEM_MEMORY + 4))();
}

#endif
