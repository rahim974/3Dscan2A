/*
 * lsm6dsv16x_test.c
 * Test minimal du LSM6DSV16X (carte STEVAL-MKI227KA) sur STM32F410 (NUCLEO-F410RB).
 *
 * Accès registres direct avec HAL_I2C_Mem_Read/Write : aucune dépendance externe.
 * Les adresses et champs de registres viennent du driver officiel ST
 * (github.com/STMicroelectronics/lsm6dsv16x-pid, lsm6dsv16x_reg.h).
 *
 * Sortie UART (115200 8N1) : entiers seulement, pour éviter d'avoir à activer
 * le printf flottant de newlib-nano.
 */
#include "lsm6dsv16x_test.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* ---- Adresses I2C (7 bits) ---- */
#define LSM6_ADDR_SA0_LOW   0x6A   /* SDO/SA0 à GND  -> 0xD4/0xD5 en 8 bits */
#define LSM6_ADDR_SA0_HIGH  0x6B   /* SDO/SA0 à VDDIO -> 0xD6/0xD7 en 8 bits */

/* ---- Registres ---- */
#define REG_WHO_AM_I    0x0F   /* doit valoir 0x70 */
#define REG_CTRL1       0x10   /* accéléro : ODR_XL[3:0], OP_MODE_XL[6:4] */
#define REG_CTRL2       0x11   /* gyro     : ODR_G[3:0],  OP_MODE_G[6:4]  */
#define REG_CTRL3       0x12   /* BOOT b7, BDU b6, IF_INC b2, SW_RESET b0 */
#define REG_CTRL6       0x15   /* FS_G[3:0] */
#define REG_CTRL8       0x17   /* FS_XL[1:0] */
#define REG_STATUS      0x1E   /* XLDA b0, GDA b1, TDA b2 */
#define REG_OUT_TEMP_L  0x20   /* 0x20..0x21 température */
#define REG_OUTX_L_G    0x22   /* 0x22..0x27 gyro X,Y,Z */
#define REG_OUTX_L_A    0x28   /* 0x28..0x2D accéléro X,Y,Z */

#define WHO_AM_I_VAL    0x70

/* ---- Réglages ---- */
#define ODR_30HZ        0x04   /* code ODR 30 Hz (accéléro et gyro) */
#define FS_XL_2G        0x00   /* ±2 g    -> 0.061 mg/LSB */
#define FS_G_2000DPS    0x04   /* ±2000 dps -> 70 mdps/LSB */

static I2C_HandleTypeDef  *s_i2c;
static UART_HandleTypeDef *s_uart;
static uint16_t            s_addr8;   /* adresse 8 bits pour HAL */

/* printf simple vers l'UART du ST-LINK */
static void uprintf(const char *fmt, ...)
{
    char buf[160];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (n > (int)sizeof buf - 1) n = sizeof buf - 1;
    HAL_UART_Transmit(s_uart, (uint8_t *)buf, (uint16_t)n, 100);
}

static HAL_StatusTypeDef rd(uint8_t reg, uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Read(s_i2c, s_addr8, reg, I2C_MEMADD_SIZE_8BIT, data, len, 100);
}

static HAL_StatusTypeDef wr(uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(s_i2c, s_addr8, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
}

static void i2c_scan(void)
{
    int found = 0;
    uprintf("Scan I2C...\r\n");
    for (uint8_t a = 0x08; a < 0x78; a++) {
        if (HAL_I2C_IsDeviceReady(s_i2c, (uint16_t)(a << 1), 2, 10) == HAL_OK) {
            uprintf("  peripherique trouve a 0x%02X\r\n", a);
            found++;
        }
    }
    if (!found)
        uprintf("  AUCUN peripherique. Verifier VDD/VDDIO, GND, SDA/SCL (inverses ?),"
                " pull-ups, et CS a VDDIO.\r\n");
}

static int16_t le16(const uint8_t *p) { return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8)); }

void LSM6_Test_Run(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart)
{
    s_i2c = hi2c;
    s_uart = huart;
    uint8_t v = 0;

    uprintf("\r\n=== Test STEVAL-MKI227KA (LSM6DSV16X) sur STM32F410 ===\r\n");
    i2c_scan();

    /* 1) Trouver le capteur sur 0x6A ou 0x6B et lire WHO_AM_I */
    const uint8_t addrs[2] = { LSM6_ADDR_SA0_LOW, LSM6_ADDR_SA0_HIGH };
    int ok = 0;
    for (int i = 0; i < 2 && !ok; i++) {
        s_addr8 = (uint16_t)(addrs[i] << 1);
        if (rd(REG_WHO_AM_I, &v, 1) == HAL_OK) {
            uprintf("WHO_AM_I @0x%02X = 0x%02X %s\r\n", addrs[i], v,
                    v == WHO_AM_I_VAL ? "(OK, LSM6DSV16X)" : "(INATTENDU, attendu 0x70)");
            ok = (v == WHO_AM_I_VAL);
        }
    }
    if (!ok) {
        uprintf("ECHEC : LSM6DSV16X introuvable. Arret du test.\r\n");
        return;
    }

    /* 2) Reset logiciel, attente fin de reset */
    wr(REG_CTRL3, 0x01);                       /* SW_RESET */
    for (int t = 0; t < 100; t++) {
        HAL_Delay(1);
        if (rd(REG_CTRL3, &v, 1) == HAL_OK && !(v & 0x01)) break;
    }

    /* 3) Configuration */
    wr(REG_CTRL3, 0x44);                       /* BDU=1, IF_INC=1 */
    wr(REG_CTRL8, FS_XL_2G);                   /* ±2 g */
    wr(REG_CTRL6, FS_G_2000DPS);               /* ±2000 dps */
    wr(REG_CTRL1, ODR_30HZ);                   /* accéléro 30 Hz, mode haute perf. */
    wr(REG_CTRL2, ODR_30HZ);                   /* gyro 30 Hz, mode haute perf. */

    uprintf("Config : XL ±2g 30Hz, G ±2000dps 30Hz. Unites : mg, mdps, centiemes de degC\r\n");
    uprintf("   ax     ay     az   |      gx       gy       gz  |  temp\r\n");

    /* 4) Boucle de lecture */
    uint8_t buf[6];
    int32_t ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0, tc = 0;
    uint32_t last_print = 0;
    int paused = 0;
    GPIO_PinState prev_btn = GPIO_PIN_SET;

    uprintf("(Bouton bleu USER = pause / reprise de l'affichage)\r\n");

    for (;;) {
        /* Bouton bleu B1 (PC13, actif à l'état bas) : bascule pause/reprise */
        GPIO_PinState btn = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
        if (btn == GPIO_PIN_RESET && prev_btn == GPIO_PIN_SET) {
            paused = !paused;
            uprintf(paused ? "--- PAUSE (bouton bleu pour reprendre) ---\r\n"
                           : "--- REPRISE ---\r\n");
            HAL_Delay(30);                       /* anti-rebond */
        }
        prev_btn = btn;

        if (rd(REG_STATUS, &v, 1) != HAL_OK) {
            uprintf("Erreur I2C (STATUS)\r\n");
            HAL_Delay(500);
            continue;
        }
        if (v & 0x01) {                         /* XLDA */
            rd(REG_OUTX_L_A, buf, 6);
            /* 0.061 mg/LSB = 61/1000 */
            ax = (int32_t)le16(&buf[0]) * 61 / 1000;
            ay = (int32_t)le16(&buf[2]) * 61 / 1000;
            az = (int32_t)le16(&buf[4]) * 61 / 1000;
        }
        if (v & 0x02) {                         /* GDA */
            rd(REG_OUTX_L_G, buf, 6);
            /* 70 mdps/LSB */
            gx = (int32_t)le16(&buf[0]) * 70;
            gy = (int32_t)le16(&buf[2]) * 70;
            gz = (int32_t)le16(&buf[4]) * 70;
        }
        if (v & 0x04) {                         /* TDA */
            rd(REG_OUT_TEMP_L, buf, 2);
            /* T = 25 + raw/256 °C -> en centièmes : 2500 + raw*100/256 */
            tc = 2500 + (int32_t)le16(&buf[0]) * 100 / 256;
        }

        if (!paused && HAL_GetTick() - last_print >= 200) { /* affichage à 5 Hz */
            last_print = HAL_GetTick();
            uprintf("%6ld %6ld %6ld | %8ld %8ld %8ld | %ld.%02ld\r\n",
                    (long)ax, (long)ay, (long)az,
                    (long)gx, (long)gy, (long)gz,
                    (long)(tc / 100), (long)(tc < 0 ? -tc % 100 : tc % 100));
        }
        HAL_Delay(5);
    }
}
