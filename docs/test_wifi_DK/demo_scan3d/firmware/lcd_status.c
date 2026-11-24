/* lcd_status : affichage du scanner 3D, fond + couche de statut (voir lcd_status.h). */
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "main.h"
#include "lcd_status.h"
#include "stm32n6570_discovery_lcd.h"
#include "fonts.h"

#define W        LCD_STATUS_LARGEUR
#define H        LCD_STATUS_HAUTEUR

/* Couleurs de la couche de statut, ARGB4444 (A R G B, 4 bits chacun) */
#define A_TRANSP 0x0000U
#define A_PANEL  0xC000U   /* noir à 75 % */
#define A_BAR    0xE03AU   /* bleu ST, 88 % */
#define A_TXT    0xFFFFU
#define A_DIM    0xFAAAU
#define A_OK     0xF4F4U
#define A_WARN   0xFFA0U
#define A_ERR    0xFF33U

#define PANEL_X  10U
#define PANEL_Y  66U
#define PANEL_W  338U
#define LIGNE_H  32U

#define CARTE_X  356U
#define CARTE_Y  72U
#define CARTE_S  8U

static uint16_t *const fond = (uint16_t *)LCD_LAYER_0_ADDRESS;    /* RGB565   */
static uint16_t *const statut = (uint16_t *)LCD_LAYER_1_ADDRESS;  /* ARGB4444 */

/* Horloge LTDC : la PLL4 du projet est à 1600 MHz -> IC16 / 64 = 25 MHz,
 * pixel clock du RK050HR18. Remplace la version __weak du BSP (prévue pour PLL4 = 50 MHz). */
HAL_StatusTypeDef MX_LTDC_ClockConfig(LTDC_HandleTypeDef *hltdc)
{
  RCC_PeriphCLKInitTypeDef clk = {0};
  (void)hltdc;
  clk.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  clk.LtdcClockSelection = RCC_LTDCCLKSOURCE_IC16;
  clk.ICSelection[RCC_IC16].ClockSelection = RCC_ICCLKSOURCE_PLL4;
  clk.ICSelection[RCC_IC16].ClockDivider = 64;
  return HAL_RCCEx_PeriphCLKConfig(&clk);
}

static uint16_t couleur(LcdNiveau_t n)
{
  switch (n)
  {
    case LCD_OK:        return A_OK;
    case LCD_ATTENTION: return A_WARN;
    case LCD_ERREUR:    return A_ERR;
    case LCD_DISCRET:   return A_DIM;
    default:            return A_TXT;
  }
}

static void flush(void)
{
  /* Le LTDC lit la RAM, pas le cache du Cortex-M55 : nettoyage ciblé des deux
   * framebuffers (pas de SCB_CleanDCache() global, qui agit sur tout le cache
   * pendant que le pilote Wi-Fi fait ses transferts DMA). */
  SCB_CleanDCache_by_Addr((void *)fond, (int32_t)(W * H * 2U));
  SCB_CleanDCache_by_Addr((void *)statut, (int32_t)(W * H * 2U));
}

static void fill(uint16_t *fb, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
  if (x >= W || y >= H) return;
  if (x + w > W) w = W - x;
  if (y + h > H) h = H - y;
  for (uint32_t j = 0; j < h; j++)
  {
    uint16_t *p = &fb[(y + j) * W + x];
    for (uint32_t i = 0; i < w; i++) p[i] = c;
  }
}

static void draw_text(uint16_t *fb, uint32_t x, uint32_t y, const char *s, const sFONT *f,
                      uint16_t fg, uint16_t bg)
{
  const uint32_t bpr = ((uint32_t)f->Width + 7U) / 8U;
  for (; *s != '\0' && (x + f->Width) <= W; s++, x += f->Width)
  {
    char ch = (*s < ' ' || *s > '~') ? '?' : *s;
    const uint8_t *g = &f->table[(uint32_t)(ch - ' ') * f->Height * bpr];
    for (uint32_t r = 0; r < f->Height && (y + r) < H; r++)
    {
      uint32_t bits = 0;
      for (uint32_t b = 0; b < bpr; b++) bits = (bits << 8) | g[r * bpr + b];
      for (uint32_t c = 0; c < f->Width; c++)
      {
        fb[(y + r) * W + x + c] = (bits & (1UL << (bpr * 8U - 1U - c))) ? fg : bg;
      }
    }
  }
}

static void ligne(uint32_t row, const char *label, const char *val, uint16_t col)
{
  const uint32_t y = PANEL_Y + 10U + row * LIGNE_H;
  fill(statut, PANEL_X + 8U, y, PANEL_W - 16U, Font20.Height, A_PANEL);
  draw_text(statut, PANEL_X + 10U, y, label, &Font20, A_DIM, A_PANEL);
  draw_text(statut, PANEL_X + 10U + 7U * Font20.Width, y, (val != NULL) ? val : "-", &Font20, col, A_PANEL);
}

static void ram_framebuffers_on(void)
{
  /* AXISRAM3..6 sont éteintes au reset : horloge + sortie du mode shutdown */
  __HAL_RCC_RAMCFG_CLK_ENABLE();
  __HAL_RCC_AXISRAM3_MEM_CLK_ENABLE();
  __HAL_RCC_AXISRAM4_MEM_CLK_ENABLE();
  __HAL_RCC_AXISRAM5_MEM_CLK_ENABLE();
  __HAL_RCC_AXISRAM6_MEM_CLK_ENABLE();
  RAMCFG_SRAM3_AXI->CR &= ~RAMCFG_CR_SRAMSD;
  RAMCFG_SRAM4_AXI->CR &= ~RAMCFG_CR_SRAMSD;
  RAMCFG_SRAM5_AXI->CR &= ~RAMCFG_CR_SRAMSD;
  RAMCFG_SRAM6_AXI->CR &= ~RAMCFG_CR_SRAMSD;
}

static void rif_ltdc(void)
{
  /* Le LTDC lit les framebuffers via l'alias sécurisé 0x34xxxxxx : maître sécurisé privilégié */
  RIMC_MasterConfig_t m = {0};
  __HAL_RCC_RIFSC_CLK_ENABLE();
  m.MasterCID = RIF_CID_1;
  m.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV;
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_LTDC1, &m);   /* port AXI de la couche 1 */
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_LTDC2, &m);   /* port AXI de la couche 2 (statut) */
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDC, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDCL1, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDCL2, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
}

int32_t LcdStatus_Init(const char *titre)
{
  BSP_LCD_LayerConfig_t cfg = {0};

  ram_framebuffers_on();
  rif_ltdc();

  LcdStatus_FondMire();
  fill(statut, 0, 0, W, H, A_TRANSP);
  fill(statut, 0, 0, W, 56, A_BAR);
  draw_text(statut, 20, 16, titre, &Font24, A_TXT, A_BAR);
  fill(statut, PANEL_X, PANEL_Y, PANEL_W, 10U * LIGNE_H + 14U, A_PANEL);
  flush();

  if (BSP_LCD_Init(0, LCD_ORIENTATION_LANDSCAPE) != BSP_ERROR_NONE)   /* couche 0, RGB565 */
  {
    return -1;
  }
  cfg.X0 = 0; cfg.X1 = W; cfg.Y0 = 0; cfg.Y1 = H;
  cfg.PixelFormat = LCD_PIXEL_FORMAT_ARGB4444;
  cfg.Address = LCD_LAYER_1_ADDRESS;
  if (BSP_LCD_ConfigLayer(0, 1, &cfg) != BSP_ERROR_NONE)              /* couche 1, ARGB4444 */
  {
    return -2;
  }
  (void)BSP_LCD_DisplayOn(0);
  return 0;
}

void LcdStatus_Draw(const LcdStatus_t *s)
{
  char v[40];
  uint32_t up = HAL_GetTick() / 1000U;

  ligne(0, "Wi-Fi", s->wifi, couleur(s->wifi_niv));
  ligne(1, "SSID", s->ssid, A_TXT);
  ligne(2, "IP", s->ip, A_TXT);
  if (s->rssi_dbm != 0) { snprintf(v, sizeof(v), "%" PRIi32 " dBm", s->rssi_dbm); } else { strcpy(v, "-"); }
  ligne(3, "RSSI", v, (s->rssi_dbm > -70) ? A_OK : A_WARN);
  ligne(4, "PC", s->pc, A_TXT);
  ligne(5, "Lien", s->lien, couleur(s->lien_niv));
  snprintf(v, sizeof(v), "%" PRIu32 " (%" PRIu32 " saut.)", s->trames, s->sautees);
  ligne(6, "Trames", v, A_TXT);
  snprintf(v, sizeof(v), "%lu.%02lu Mb/s %lu.%lu/s",
           (unsigned long)(s->debit_kbps / 1000U), (unsigned long)((s->debit_kbps % 1000U) / 10U),
           (unsigned long)(s->trames_par_s_x10 / 10U), (unsigned long)(s->trames_par_s_x10 % 10U));
  ligne(7, "Debit", v, A_OK);
  snprintf(v, sizeof(v), "%" PRIu32 " err %" PRIu32 " reco", s->erreurs, s->reconnexions);
  ligne(8, "Erreurs", v, (s->erreurs != 0U) ? A_WARN : A_OK);
  snprintf(v, sizeof(v), "%02lu:%02lu:%02lu", (unsigned long)(up / 3600U),
           (unsigned long)((up / 60U) % 60U), (unsigned long)(up % 60U));
  ligne(9, "Temps", v, A_DIM);

  fill(statut, 0, H - 34U, W, 34, (s->message != NULL) ? A_BAR : A_TRANSP);
  if (s->message != NULL)
  {
    draw_text(statut, 20, H - 26U, s->message, &Font16, couleur(s->message_niv), A_BAR);
  }
  flush();
}

uint16_t *LcdStatus_Fond(void)
{
  return fond;
}

void LcdStatus_FondMire(void)
{
  for (uint32_t y = 0; y < H; y++)
  {
    for (uint32_t x = 0; x < W; x++)
    {
      uint32_t r = (x * 31U) / W, g = (y * 63U) / H, b = 31U - r;
      fond[y * W + x] = (uint16_t)(((r / 2U) << 11) | ((g / 2U) << 5) | (b / 2U));   /* mire sombre */
    }
  }
  flush();
}

/* Proche = rouge, loin = bleu (0,5 m .. 2 m) */
static uint16_t depth_color(uint16_t mm)
{
  int32_t t = ((int32_t)mm - 500) * 255 / 1500;
  if (t < 0) t = 0;
  if (t > 255) t = 255;
  uint32_t r = (t < 128) ? 255U : (uint32_t)(255 - (t - 128) * 2);
  uint32_t g = (t < 128) ? (uint32_t)(t * 2) : (uint32_t)(255 - (t - 128) * 2);
  uint32_t b = (t < 128) ? 0U : (uint32_t)((t - 128) * 2);
  return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

void LcdStatus_FondProfondeur(const uint8_t *zones, uint32_t largeur, uint32_t hauteur, uint32_t pas)
{
  for (uint32_t zy = 0; zy < hauteur; zy++)
  {
    for (uint32_t zx = 0; zx < largeur; zx++)
    {
      const uint8_t *z = &zones[(zy * largeur + zx) * pas];
      uint16_t mm = (uint16_t)(z[0] | (z[1] << 8));
      fill(fond, CARTE_X + zx * CARTE_S, CARTE_Y + zy * CARTE_S, CARTE_S, CARTE_S, depth_color(mm));
    }
  }
  flush();
}
