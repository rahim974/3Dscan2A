/**
  * lcd_status : affichage du scanner 3D sur le LCD 800x480 du STM32N6570-DK.
  *
  * Deux couches LTDC superposées :
  *   - couche 0 « fond »   : RGB565, 800x480, 768 000 octets à LCD_LAYER_0_ADDRESS (0x34200000).
  *                           Destinée à l'aperçu caméra (IMX335 via DCMIPP) ; pour la démo :
  *                           mire de test + carte de profondeur simulée.
  *   - couche 1 « statut » : ARGB4444, 800x480, 768 000 octets à LCD_LAYER_1_ADDRESS (0x342C0000).
  *                           Transparente sauf le bandeau de titre, le panneau d'état
  *                           (Wi-Fi, IP, RSSI, trames, débit, erreurs) et le message du bas,
  *                           mélangés au fond par l'alpha de chaque pixel.
  * Les deux framebuffers sont en RAM interne AXISRAM3-6 (0x34200000-0x343BFFFF, 1,75 Mo),
  * allumées par LcdStatus_Init() ; la RAM du programme (AXISRAM1/2) n'est pas touchée.
  *
  * Ressources occupées (toutes internes au DK, aucune broche Arduino) :
  *   - LTDC (couches 1 et 2) + dalle RK050HR18 (BSP stm32n6570_discovery_lcd.c) ;
  *   - GPIO : PA0 PA1 PA2 PA7 PA8 PA15, PB2 PB4 PB11-PB15, PD8 PD9 PD15, PE1 PE11,
  *            PG0 PG1 PG6 PG8 PG11 PG12 PG13, PH3 PH4 PH6, PQ3 PQ4 PQ6 ;
  *   - horloge : IC16 <- PLL4 / 64 = 25 MHz (MX_LTDC_ClockConfig redéfini ici) ;
  *   - RIF : LTDC1 et LTDC2 (un port maître AXI par couche) sécurisés/privilégiés ;
  *           LTDC, LTDCL1, LTDCL2 esclaves sécurisés. Sans LTDC2, la couche 2 lit des zéros
  *           (transparente) : c'était le cas du premier essai, texte invisible ;
  *   - DMA2D : horloge activée par le BSP, non utilisé (dessin au CPU).
  *
  * Module indépendant du Wi-Fi et du format des trames : l'application remplit un
  * LcdStatus_t et appelle LcdStatus_Draw() (2 à 5 fois par seconde suffit).
  */
#ifndef LCD_STATUS_H
#define LCD_STATUS_H

#include <stdint.h>

#define LCD_STATUS_LARGEUR   800U
#define LCD_STATUS_HAUTEUR   480U

typedef enum
{
  LCD_NEUTRE = 0,   /* blanc */
  LCD_OK,           /* vert */
  LCD_ATTENTION,    /* orange */
  LCD_ERREUR,       /* rouge */
  LCD_DISCRET       /* gris */
} LcdNiveau_t;

typedef struct
{
  const char *wifi;        LcdNiveau_t wifi_niv;    /* "connecte", "connexion..." */
  const char *ssid;
  char ip[20];                                       /* "-" si pas d'adresse */
  int32_t rssi_dbm;                                  /* 0 = inconnu */
  const char *pc;                                    /* "10.62.127.8:5002" */
  const char *lien;        LcdNiveau_t lien_niv;    /* état de la liaison PC */
  uint32_t trames;                                   /* trames envoyées */
  uint32_t sautees;                                  /* trames du capteur non envoyées */
  uint32_t debit_kbps;                               /* débit utile en kbit/s */
  uint32_t trames_par_s_x10;                         /* 300 = 30,0 trames/s */
  uint32_t erreurs;
  uint32_t reconnexions;
  const char *message;     LcdNiveau_t message_niv; /* bandeau du bas, NULL = rien */
} LcdStatus_t;

/* Allume la RAM des framebuffers, l'écran et les deux couches. 0 = OK, <0 = échec. */
int32_t LcdStatus_Init(const char *titre);

/* Couche de statut (superposée) : redessine le panneau d'état. */
void LcdStatus_Draw(const LcdStatus_t *s);

/* Couche de fond : adresse du framebuffer RGB565 800x480 (cible du futur flux caméra). */
uint16_t *LcdStatus_Fond(void);

/* Couche de fond : mire de test (dégradé). */
void LcdStatus_FondMire(void);

/* Couche de fond : carte de profondeur (zones de `pas` octets, distance uint16 mm en tête),
 * agrandie 8x à droite du panneau d'état (54x42 -> 432x336 px). */
void LcdStatus_FondProfondeur(const uint8_t *zones, uint32_t largeur, uint32_t hauteur, uint32_t pas);

#endif /* LCD_STATUS_H */
