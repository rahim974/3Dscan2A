/* Configuration BSP STM32N6570-DK réduite à l'écran LCD (démo Scanner 3D). */
#ifndef STM32N6570_DISCOVERY_CONF_H
#define STM32N6570_DISCOVERY_CONF_H

#include "stm32n6xx_hal.h"

#define USE_COM_LOG                         0U
#define USE_BSP_COM_FEATURE                 0U

/* Framebuffers en AXISRAM3-6 (allumées par LcdStatus_Init), voir lcd_status.h :
 * couche 0 fond RGB565 800x480 (768 000 o), couche 1 statut ARGB4444 800x480 (768 000 o). */
#define LCD_LAYER_0_ADDRESS                 0x34200000U
#define LCD_LAYER_1_ADDRESS                 0x342C0000U

#define BSP_BUTTON_USER1_IT_PRIORITY        15U
#define BSP_BUTTON_USER2_IT_PRIORITY        15U
#define BSP_BUTTON_TAMP_IT_PRIORITY         15U

#endif /* STM32N6570_DISCOVERY_CONF_H */
