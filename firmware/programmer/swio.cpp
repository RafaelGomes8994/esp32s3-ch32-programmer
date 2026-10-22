#include "swio.h"

#include "driver/gpio.h"
#include "soc/gpio_periph.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"

namespace {

constexpr uint32_t kSwioMask = 1UL << SWIO_PIN;
// Kept verbatim from CNLohr's ESP32-S2 PrecDelay().  ESP32-S2 and ESP32-S3
// both use Xtensa LX7, so this preserves the upstream t1coeff=10 instruction
// loop rather than guessing a cycle-count conversion.
constexpr int kDefaultT1Coeff = 10;
int g_t1Coeff = kDefaultT1Coeff;
constexpr uint32_t kReadTimeoutSamples = 1000;

static inline uint32_t IRAM_ATTR readCpuCycleCount() {
  uint32_t ccount = 0;
  __asm__ __volatile__("rsr %0, ccount" : "=a"(ccount));
  return ccount;
}

static inline void IRAM_ATTR upstreamPrecDelay(int delay) {
  __asm__ __volatile__(
      ".balign 4\n"
      "1:\taddi.n %[delay], %[delay], -1\n"
      "\tbbci %[delay], 31, 1b\n"
      : [delay] "+r"(delay));
}

inline bool IRAM_ATTR swioLineHigh() {
  return (REG_READ(GPIO_IN_REG) & kSwioMask) != 0;
}

}  // namespace

void swioSetT1Coeff(int coeff) {
  if (coeff >= 2 && coeff <= 64) {
    g_t1Coeff = coeff;
  }
}

int swioGetT1Coeff() {
  return g_t1Coeff;
}

bool swioInit() {
  // Match the upstream startup state: input pull-up enabled, then output
  // enabled with its latch high.  The external pull-up remains installed.
  pinMode(SWIO_PIN, INPUT_PULLUP);
  // Ensure simple GPIO output matrix (0x100) and IO_MUX input buffer are both active (IDF 4.x & 5.x)
  GPIO.func_out_sel_cfg[SWIO_PIN].val = 0x100;
  PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[SWIO_PIN]);
  swioDriveHigh();
  delay(5);
  return true;
}

void swioSynchronize() {
  // InitializeSWDSWIO begins its configuration writes from this driven-high
  // state; it has no 20 ms SWIO-low reset pulse.
  PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[SWIO_PIN]);
  swioDriveHigh();
  delay(5);
}

void swioResetPulse() {
  PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[SWIO_PIN]);
  swioDriveLow();
  delay(20);
  swioDriveHigh();
  delayMicroseconds(50);
}

uint32_t swioMeasureUpstreamT1Cycles() {
    constexpr uint32_t kSamples = 128;

    const uint32_t start = readCpuCycleCount();

    for (uint32_t i = 0; i < kSamples; ++i) {
        upstreamPrecDelay(g_t1Coeff);
    }

    return (readCpuCycleCount() - start) / kSamples;
}
void IRAM_ATTR swioDriveLow() {
  REG_WRITE(GPIO_OUT_W1TC_REG, kSwioMask);
  REG_WRITE(GPIO_ENABLE_W1TS_REG, kSwioMask);
}

void IRAM_ATTR swioDriveHigh() {
  REG_WRITE(GPIO_OUT_W1TS_REG, kSwioMask);
  REG_WRITE(GPIO_ENABLE_W1TS_REG, kSwioMask);
}

void IRAM_ATTR swioRelease() {
  REG_WRITE(GPIO_ENABLE_W1TC_REG, kSwioMask);
}

void IRAM_ATTR swioWriteBit(bool value) {
  swioDriveLow();
  upstreamPrecDelay(value ? g_t1Coeff : g_t1Coeff * 4);
  swioDriveHigh();
  upstreamPrecDelay(g_t1Coeff);
}

SwioBitResult IRAM_ATTR swioReadBit() {
  // Port of upstream ReadBitSWIO(): low trigger; release while preloading the
  // output latch high; sample after 2*t1; then restore driven-high state.
  swioDriveLow();
  upstreamPrecDelay(g_t1Coeff);
  swioRelease();
  REG_WRITE(GPIO_OUT_W1TS_REG, kSwioMask);
  upstreamPrecDelay(g_t1Coeff * 2);

  const bool sampledHigh = swioLineHigh();
  for (uint32_t i = 0; i < kReadTimeoutSamples; ++i) {
    if (swioLineHigh()) {
      swioDriveHigh();
      upstreamPrecDelay(g_t1Coeff / 2);
      return sampledHigh ? SwioBitResult::One : SwioBitResult::Zero;
    }
  }
  // This is the upstream timeout recovery state, not an indication of target
  // communication success.
  swioDriveHigh();
  return SwioBitResult::Timeout;
}
