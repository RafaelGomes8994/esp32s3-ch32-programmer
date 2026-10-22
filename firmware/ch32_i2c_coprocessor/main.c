/*
 * ============================================================================
 * CH32V003 I2C-to-PN532 NFC Coprocessor Firmware
 * ============================================================================
 * Topologia:
 *   [ESP32-S3] <-- I2C Hardware (PC1=SDA, PC2=SCL @ 0x30) --> [CH32V003]
 *   [CH32V003] <-- I2C Software (PD2=SDA, PD3=SCL @ 0x24) --> [PN532 NFC]
 *
 * Protocolo de Resposta I2C Slave (8 bytes em 0x30):
 *   Byte 0: Status
 *           0x01 = Nova tag disponivel para leitura
 *           0xAA = Coprocessador ocioso (PN532 online em PD2/PD3)
 *           0xEE = Coprocessador ocioso (aguardando PN532 em PD2/PD3)
 *   Byte 1: Tamanho do UID (4 ou 7 bytes)
 *   Bytes 2..7: Bytes do UID da tag ISO14443A
 * ============================================================================
 */

#include <stdint.h>
#include <stddef.h>

void* memset(void* dest, int c, size_t n) {
    uint8_t* p = (uint8_t*)dest;
    while (n--) *p++ = (uint8_t)c;
    return dest;
}

/* Registradores de Clock e Reset (RCC) */
#define RCC_BASE        0x40021000UL
#define RCC_CTLR        (*(volatile uint32_t*)(RCC_BASE + 0x00UL))
#define RCC_CFGR0       (*(volatile uint32_t*)(RCC_BASE + 0x04UL))
#define RCC_INTR        (*(volatile uint32_t*)(RCC_BASE + 0x08UL))
#define RCC_APB2PRSTR   (*(volatile uint32_t*)(RCC_BASE + 0x0CUL))
#define RCC_APB1PRSTR   (*(volatile uint32_t*)(RCC_BASE + 0x10UL))
#define RCC_AHBPCENR    (*(volatile uint32_t*)(RCC_BASE + 0x14UL))
#define RCC_APB2PCENR   (*(volatile uint32_t*)(RCC_BASE + 0x18UL))
#define RCC_APB1PCENR   (*(volatile uint32_t*)(RCC_BASE + 0x1CUL))

/* Registrador de Flash (Latencia) */
#define FLASH_ACTLR     (*(volatile uint32_t*)(0x40022000UL))

/* AFIO */
#define AFIO_BASE       0x40010000UL
#define AFIO_PCFR1      (*(volatile uint32_t*)(AFIO_BASE + 0x04UL))

/* GPIOC (PC1 = I2C1_SDA, PC2 = I2C1_SCL) */
#define GPIOC_BASE      0x40011000UL
#define GPIOC_CFGLR     (*(volatile uint32_t*)(GPIOC_BASE + 0x00UL))
#define GPIOC_INDR      (*(volatile uint32_t*)(GPIOC_BASE + 0x08UL))
#define GPIOC_OUTDR     (*(volatile uint32_t*)(GPIOC_BASE + 0x0CUL))
#define GPIOC_BSHR      (*(volatile uint32_t*)(GPIOC_BASE + 0x10UL))

/* GPIOD (PD2 = SW_SDA, PD3 = SW_SCL; PD1 = SWIO intocado!) */
#define GPIOD_BASE      0x40011400UL
#define GPIOD_CFGLR     (*(volatile uint32_t*)(GPIOD_BASE + 0x00UL))
#define GPIOD_INDR      (*(volatile uint32_t*)(GPIOD_BASE + 0x08UL))
#define GPIOD_OUTDR     (*(volatile uint32_t*)(GPIOD_BASE + 0x0CUL))
#define GPIOD_BSHR      (*(volatile uint32_t*)(GPIOD_BASE + 0x10UL))
#define GPIOD_BCR       (*(volatile uint32_t*)(GPIOD_BASE + 0x14UL))

/* I2C1 Hardware Slave */
#define I2C1_BASE       0x40005400UL
#define I2C1_CTLR1      (*(volatile uint16_t*)(I2C1_BASE + 0x00UL))
#define I2C1_CTLR2      (*(volatile uint16_t*)(I2C1_BASE + 0x04UL))
#define I2C1_OADDR1     (*(volatile uint16_t*)(I2C1_BASE + 0x08UL))
#define I2C1_OADDR2     (*(volatile uint16_t*)(I2C1_BASE + 0x0CUL))
#define I2C1_DATAR      (*(volatile uint16_t*)(I2C1_BASE + 0x10UL))
#define I2C1_STAR1      (*(volatile uint16_t*)(I2C1_BASE + 0x14UL))
#define I2C1_STAR2      (*(volatile uint16_t*)(I2C1_BASE + 0x18UL))
#define I2C1_CKCFGR     (*(volatile uint16_t*)(I2C1_BASE + 0x1CUL))

/* Bits do I2C1 */
#define I2C_CTLR1_PE        (1U << 0)
#define I2C_CTLR1_STOP      (1U << 9)
#define I2C_CTLR1_ACK       (1U << 10)
#define I2C_CTLR1_SWRST     (1U << 15)

#define I2C_STAR1_SB        (1U << 0)
#define I2C_STAR1_ADDR      (1U << 1)
#define I2C_STAR1_BTF       (1U << 2)
#define I2C_STAR1_STOPF     (1U << 4)
#define I2C_STAR1_RXNE      (1U << 6)
#define I2C_STAR1_TXE       (1U << 7)
#define I2C_STAR1_BERR      (1U << 8)
#define I2C_STAR1_ARLO      (1U << 9)
#define I2C_STAR1_AF        (1U << 10)
#define I2C_STAR1_OVR       (1U << 11)

#define CH32_I2C_SLAVE_ADDR 0x30
#define PN532_I2C_ADDR      0x24

/* Bloco de diagnostico no inicio da RAM (0x20000000) para inspecao via SWIO */
typedef struct {
    uint32_t magic;
    uint32_t i2c_slave_hits;
    uint32_t pn532_status;
    uint32_t poll_count;
} diag_state_t;

__attribute__((section(".data")))
volatile diag_state_t g_diag = {0xCAFE0030UL, 0, 0, 0};

static volatile uint8_t g_tx_buf[8] = {0xEE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static volatile uint8_t g_tx_idx = 0;
static volatile uint8_t g_pn532_ok = 0;
static volatile uint8_t g_tag_consumed = 0;

/* Pinos dinamicos do Software I2C Master em GPIOD (padrao: PD2=SDA, PD3=SCL) */
static uint8_t g_sda_pin = 2;
static uint8_t g_scl_pin = 3;

/*
 * Servico de Polling ultrarrapido do I2C1 Slave (PC1=SDA, PC2=SCL).
 * NUNCA le I2C1_STAR2 fora de I2C_STAR1_ADDR para nao limpar ADDR acidentalmente!
 */
static inline void i2c_slave_service(void) {
    uint16_t star1 = I2C1_STAR1;
    if (star1 == 0) return;

    if (star1 & I2C_STAR1_ADDR) {
        (void)I2C1_STAR2; /* Sequencia obrigatoria: ler STAR1 e depois STAR2 limpa ADDR */
        if (g_tag_consumed && g_tx_buf[0] == 0x01) {
            g_tx_buf[0] = g_pn532_ok ? 0xAA : 0xEE;
            g_tag_consumed = 0;
        }
        g_tx_idx = 0;
        g_diag.i2c_slave_hits++;
        star1 = I2C1_STAR1;
    }

    if (star1 & I2C_STAR1_RXNE) {
        (void)I2C1_DATAR;
    }

    if (star1 & I2C_STAR1_TXE) {
        if (g_tx_idx < 8) {
            uint8_t idx = g_tx_idx++;
            I2C1_DATAR = g_tx_buf[idx];
            if (idx >= 5 && g_tx_buf[0] == 0x01) {
                g_tag_consumed = 1;
            }
        } else if (star1 & I2C_STAR1_BTF) {
            I2C1_DATAR = 0x00;
        }
    }

    if (star1 & I2C_STAR1_STOPF) {
        I2C1_CTLR1 |= (I2C_CTLR1_PE | I2C_CTLR1_ACK);
        if (g_tag_consumed && g_tx_buf[0] == 0x01) {
            g_tx_buf[0] = g_pn532_ok ? 0xAA : 0xEE;
            g_tag_consumed = 0;
        }
        g_tx_idx = 0;
    }

    if (star1 & (I2C_STAR1_BERR | I2C_STAR1_ARLO | I2C_STAR1_AF | I2C_STAR1_OVR)) {
        if (g_tag_consumed && g_tx_buf[0] == 0x01) {
            g_tx_buf[0] = g_pn532_ok ? 0xAA : 0xEE;
            g_tag_consumed = 0;
        }
        I2C1_STAR1 = (uint16_t)~(I2C_STAR1_BERR | I2C_STAR1_ARLO | I2C_STAR1_AF | I2C_STAR1_OVR);
        g_tx_idx = 0;
    }
}

static void delay_us(uint32_t us) {
    while (us--) {
        i2c_slave_service();
        __asm__ volatile("nop; nop;");
    }
}

static void delay_ms(uint32_t ms) {
    while (ms--) {
        delay_us(500);
    }
}

/* ============================================================================
 * Inicializacao do Hardware I2C1 Slave (PC1=SDA, PC2=SCL @ 0x30)
 * ============================================================================ */
static void hw_i2c_slave_init(void) {
    FLASH_ACTLR = 0x00000000UL;
    RCC_CFGR0   = 0x00000000UL;
    RCC_CTLR   |= 0x00000001UL;

    RCC_APB2PCENR |= (1UL << 0) | (1UL << 4) | (1UL << 5);
    RCC_APB1PCENR |= (1UL << 21);

    AFIO_PCFR1 &= ~((1UL << 1) | (1UL << 22));

    GPIOC_OUTDR |= (1UL << 1) | (1UL << 2);
    uint32_t cfgc = GPIOC_CFGLR;
    cfgc &= ~((0xFUL << 4) | (0xFUL << 8));
    cfgc |=  ((0xDUL << 4) | (0xDUL << 8));
    GPIOC_CFGLR = cfgc;

    RCC_APB1PRSTR |=  (1UL << 21);
    RCC_APB1PRSTR &= ~(1UL << 21);

    I2C1_CTLR1 |=  I2C_CTLR1_SWRST;
    I2C1_CTLR1 &= ~I2C_CTLR1_SWRST;

    I2C1_CTLR2  = 12;
    I2C1_CKCFGR = 8 | 0x8000U;

    I2C1_OADDR1 = (CH32_I2C_SLAVE_ADDR << 1);
    I2C1_OADDR2 = 0;

    I2C1_CTLR1 |= I2C_CTLR1_PE;
    I2C1_CTLR1 |= I2C_CTLR1_ACK;
}

/* ============================================================================
 * Software I2C Master em GPIOD (PD2 / PD3) para leitura do PN532 (0x24)
 * ============================================================================ */
static inline void sda_high(void) { GPIOD_BSHR = (1UL << g_sda_pin); }
static inline void sda_low(void)  { GPIOD_BCR  = (1UL << g_sda_pin); }
static inline void scl_high(void) {
    GPIOD_BSHR = (1UL << g_scl_pin);
    uint16_t t = 400;
    while (((GPIOD_INDR & (1UL << g_scl_pin)) == 0) && --t) {
        i2c_slave_service();
    }
}
static inline void scl_low(void)  { GPIOD_BCR  = (1UL << g_scl_pin); }
static inline uint8_t sda_read(void) { return (GPIOD_INDR & (1UL << g_sda_pin)) ? 1 : 0; }

static void sw_i2c_init_pins(void) {
    GPIOD_BSHR = (1UL << 2) | (1UL << 3);
    uint32_t cfgd = GPIOD_CFGLR;
    cfgd &= ~((0xFUL << 8) | (0xFUL << 12));
    cfgd |=  ((0x5UL << 8) | (0x5UL << 12));
    GPIOD_CFGLR = cfgd;
    delay_us(20);
}

static void sw_i2c_start(void) {
    sda_high();
    scl_high();
    delay_us(4);
    sda_low();
    delay_us(4);
    scl_low();
    delay_us(4);
}

static void sw_i2c_stop(void) {
    sda_low();
    delay_us(4);
    scl_high();
    delay_us(4);
    sda_high();
    delay_us(4);
}

static uint8_t sw_i2c_write_byte(uint8_t data) {
    for (uint8_t i = 0; i < 8; ++i) {
        if (data & 0x80) sda_high();
        else             sda_low();
        delay_us(3);
        scl_high();
        delay_us(4);
        scl_low();
        delay_us(3);
        data <<= 1;
    }
    sda_high();
    delay_us(3);
    scl_high();
    delay_us(3);
    uint8_t ack = (sda_read() == 0) ? 1 : 0;
    delay_us(2);
    scl_low();
    delay_us(3);
    return ack;
}

static uint8_t sw_i2c_read_byte(uint8_t send_ack) {
    uint8_t data = 0;
    sda_high();
    for (uint8_t i = 0; i < 8; ++i) {
        data <<= 1;
        delay_us(3);
        scl_high();
        delay_us(3);
        if (sda_read()) data |= 1;
        delay_us(2);
        scl_low();
    }
    if (send_ack) sda_low();
    else          sda_high();
    delay_us(3);
    scl_high();
    delay_us(4);
    scl_low();
    delay_us(3);
    sda_high();
    return data;
}

static void pn532_abort_and_flush(void) {
    /* 1. Recupera barramento I2C de software caso o PN532 tenha ficado preso no meio de um byte */
    sda_high();
    for (uint8_t i = 0; i < 9; ++i) {
        scl_low();
        delay_us(4);
        scl_high();
        delay_us(4);
    }
    sw_i2c_stop();

    /* 2. Envia ACK frame (00 00 FF 00 FF 00) para abortar qualquer comando InListPassiveTarget pendente no PN532 */
    static const uint8_t ack_abort[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
    sw_i2c_start();
    if (sw_i2c_write_byte((PN532_I2C_ADDR << 1) | 0)) {
        for (uint8_t i = 0; i < sizeof(ack_abort); ++i) {
            sw_i2c_write_byte(ack_abort[i]);
        }
    }
    sw_i2c_stop();
    delay_ms(2);

    /* 3. Se houver resposta antiga pendente no buffer do PN532 (RDY=1), drena os bytes */
    sw_i2c_start();
    if (sw_i2c_write_byte((PN532_I2C_ADDR << 1) | 1)) {
        uint8_t rdy = sw_i2c_read_byte(1);
        if (rdy == 0x01) {
            for (uint8_t i = 0; i < 20; ++i) {
                (void)sw_i2c_read_byte(i < 19 ? 1 : 0);
            }
        } else {
            (void)sw_i2c_read_byte(0);
        }
    }
    sw_i2c_stop();
    delay_ms(1);
}

static uint8_t pn532_wait_ready(uint16_t timeout_ms) {
    while (timeout_ms--) {
        sw_i2c_start();
        if (sw_i2c_write_byte((PN532_I2C_ADDR << 1) | 1)) {
            uint8_t rdy = sw_i2c_read_byte(0);
            sw_i2c_stop();
            if (rdy == 0x01) return 1;
        } else {
            sw_i2c_stop();
        }
        delay_ms(1);
    }
    return 0;
}

static uint8_t pn532_read_ack(void) {
    if (!pn532_wait_ready(25)) return 0;
    uint8_t ack[7] = {0};
    sw_i2c_start();
    if (!sw_i2c_write_byte((PN532_I2C_ADDR << 1) | 1)) {
        sw_i2c_stop();
        return 0;
    }
    for (uint8_t i = 0; i < 7; ++i) {
        ack[i] = sw_i2c_read_byte(i < 6 ? 1 : 0);
    }
    sw_i2c_stop();
    return (ack[0] == 0x01 && ack[3] == 0xFF && ack[4] == 0x00 && ack[5] == 0xFF) ? 1 : 0;
}

static uint8_t pn532_send_cmd_wait_resp(const uint8_t* cmd, uint8_t cmd_len, uint8_t* resp, uint8_t resp_len, uint16_t wait_ms) {
    sw_i2c_start();
    if (!sw_i2c_write_byte((PN532_I2C_ADDR << 1) | 0)) {
        sw_i2c_stop();
        return 0;
    }
    for (uint8_t i = 0; i < cmd_len; ++i) {
        sw_i2c_write_byte(cmd[i]);
    }
    sw_i2c_stop();

    if (!pn532_read_ack()) {
        pn532_abort_and_flush();
        return 0;
    }
    if (!pn532_wait_ready(wait_ms)) {
        pn532_abort_and_flush();
        return 0;
    }

    sw_i2c_start();
    if (!sw_i2c_write_byte((PN532_I2C_ADDR << 1) | 1)) {
        sw_i2c_stop();
        return 0;
    }
    for (uint8_t i = 0; i < resp_len; ++i) {
        resp[i] = sw_i2c_read_byte(i < (resp_len - 1) ? 1 : 0);
    }
    sw_i2c_stop();
    return 1;
}

static uint8_t pn532_sam_config(void) {
    /* 0. Aborta qualquer comando anterior preso no PN532 e limpa o buffer I2C */
    pn532_abort_and_flush();

    /* 1. SAMConfiguration (Normal Mode = 0x01, Timeout = 0x14, IRQ = 0x01) */
    static const uint8_t sam_cmd[] = {
        0x00, 0x00, 0xFF, 0x05, 0xFB, 0xD4, 0x14, 0x01, 0x14, 0x01, 0x02, 0x00
    };
    uint8_t resp[10] = {0};
    if (!pn532_send_cmd_wait_resp(sam_cmd, sizeof(sam_cmd), resp, sizeof(resp), 35)) {
        return 0;
    }
    uint8_t sam_ok = 0;
    for (uint8_t i = 0; i < sizeof(resp) - 1; ++i) {
        if (resp[i] == 0xD5 && resp[i + 1] == 0x15) {
            sam_ok = 1;
            break;
        }
    }
    if (!sam_ok) return 0;

    /* 2. CRITICO: RFConfiguration (Item 5 = MaxRetries = 2)
     * Faz o PN532 responder imediatamente (D5 4B 00 em ~12ms) quando nao ha cartao,
     * evitando que o PN532 trave aguardando cartao infinitamente e dessincronize o buffer I2C! */
    static const uint8_t rf_retries_cmd[] = {
        0x00, 0x00, 0xFF, 0x06, 0xFA, 0xD4, 0x32, 0x05, 0xFF, 0x01, 0x02, 0xF3, 0x00
    };
    pn532_send_cmd_wait_resp(rf_retries_cmd, sizeof(rf_retries_cmd), resp, sizeof(resp), 35);
    return 1;
}

static void pn532_poll_tag(void) {
    static uint8_t hold_cycles = 0;
    static uint8_t fail_streak = 0;

    if (g_tag_consumed) {
        g_tag_consumed = 0;
        g_tx_buf[0] = g_pn532_ok ? 0xAA : 0xEE;
        hold_cycles = 0;
    }

    /* Se houver uma tag pendente ainda nao lida pela ESP32, mantem por ~600ms (200 ciclos rapidos) */
    if (g_tx_buf[0] == 0x01) {
        if (++hold_cycles < 200) return;
        hold_cycles = 0;
        g_tag_consumed = 0;
        g_tx_buf[0] = g_pn532_ok ? 0xAA : 0xEE;
    }

    /* Comando InListPassiveTarget (1 tag ISO14443A 106kbps) */
    static const uint8_t list_cmd[] = {
        0x00, 0x00, 0xFF, 0x04, 0xFC, 0xD4, 0x4A, 0x01, 0x00, 0xE1, 0x00
    };
    uint8_t rx[22] = {0};
    if (!pn532_send_cmd_wait_resp(list_cmd, sizeof(list_cmd), rx, sizeof(rx), 35)) {
        if (++fail_streak >= 6) {
            fail_streak = 0;
            g_pn532_ok = 0;
        }
        return;
    }
    fail_streak = 0;

    /* Procura cabecalho de resposta D5 4B (InListPassiveTarget response) */
    for (uint8_t k = 1; k <= 10; ++k) {
        if (rx[k] == 0xD5 && rx[k + 1] == 0x4B) {
            uint8_t nb_tg = rx[k + 2];
            if (nb_tg >= 1) {
                uint8_t uid_len = rx[k + 7];
                if (uid_len >= 4 && uid_len <= 7 && (k + 8 + uid_len) <= sizeof(rx)) {
                    uint8_t copy_len = (uid_len > 6) ? 6 : uid_len;
                    for (uint8_t b = 0; b < 6; ++b) {
                        g_tx_buf[2 + b] = (b < copy_len) ? rx[k + 8 + b] : 0x00;
                    }
                    g_tx_buf[1] = copy_len;
                    g_tag_consumed = 0;
                    g_tx_buf[0] = 0x01; /* Sinaliza nova tag disponivel para a ESP32! */
                    hold_cycles = 0;
                }
            }
            break;
        }
    }
}

int main(void) {
    hw_i2c_slave_init();
    sw_i2c_init_pins();

    g_diag.magic = 0xCAFE0030UL;

    uint16_t retry_div = 0;
    while (1) {
        i2c_slave_service();

        /* Polling rapido do PN532 a cada ~20ms */
        if (++retry_div >= 400) {
            retry_div = 0;
            g_diag.poll_count++;

            if (!g_pn532_ok) {
                g_sda_pin = 2;
                g_scl_pin = 3;
                if (pn532_sam_config()) {
                    g_pn532_ok = 1;
                    g_diag.pn532_status = 1;
                    if (g_tx_buf[0] != 0x01) g_tx_buf[0] = 0xAA;
                } else {
                    g_sda_pin = 3;
                    g_scl_pin = 2;
                    if (pn532_sam_config()) {
                        g_pn532_ok = 2;
                        g_diag.pn532_status = 2;
                        if (g_tx_buf[0] != 0x01) g_tx_buf[0] = 0xAA;
                    } else {
                        g_sda_pin = 2;
                        g_scl_pin = 3;
                    }
                }
            } else {
                pn532_poll_tag();
            }
        }
        delay_us(5);
    }
}
