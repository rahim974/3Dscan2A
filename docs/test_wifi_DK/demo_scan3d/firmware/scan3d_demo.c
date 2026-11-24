/**
  * Démo Scanner 3D sur STM32N6570-DK + X-NUCLEO-67W61M1 (sans console, sur batterie).
  *
  * Trois tâches indépendantes, comme dans le futur scanner :
  *  - « capteur » (simulé) : produit 30 trames/s de 54x42 zones (4 octets/zone, ~9 Ko),
  *    quoi qu'il arrive au réseau ;
  *  - « reseau » : Wi-Fi + TCP vers le PC, envoie la dernière trame disponible.
  *    Si l'envoi est plus lent que le capteur, des trames sont sautées (comptées) ;
  *    en cas d'erreur : fermeture du socket, nouvelle connexion ;
  *  - « ecran » : rafraîchit l'écran 4 fois par seconde (carte + panneau d'état),
  *    sans jamais attendre le réseau. Il sert aussi de chien de garde : si la tâche
  *    réseau est bloquée plus de 20 s dans un appel du pilote Wi-Fi, il redémarre la carte.
  * Les messages sont aussi écrits sur la console ST-LINK (921600 bauds).
  *
  * Format d'une trame (petit-boutiste) :
  *   en-tête 20 octets : "S3DF" | uint32 numéro | uint32 temps_ms | uint16 largeur |
  *                       uint16 hauteur | uint32 taille_données
  *   données : largeur*hauteur zones de { uint16 distance_mm, uint8 confiance, uint8 statut }
  */
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "w6x_api.h"
#include "logging.h"
#include "shell.h"
#include <stdlib.h>

#include "scan3d_demo.h"
#include "lcd_status.h"

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#else
#error "Copier wifi_secrets_template.h en wifi_secrets.h et le remplir (fichier ignoré par git)."
#endif

#define S3D_STR_(x)         #x
#define S3D_STR(x)          S3D_STR_(x)

#define SCAN_W              54U
#define SCAN_H              42U
#define ZONE_BYTES          4U
#define PAYLOAD_SIZE        (SCAN_W * SCAN_H * ZONE_BYTES)
#define HEADER_SIZE         20U
#define FRAME_SIZE          (HEADER_SIZE + PAYLOAD_SIZE)
#define FRAME_PERIOD_MS     33U          /* ~30 trames/s */
#define LCD_PERIOD_MS       250U
#define SEND_TIMEOUT_MS     3000
#define WATCHDOG_MS         20000U
#define ECHECS_NCP_MAX      3U           /* échecs AT consécutifs avant redémarrage */
/* Les requêtes AT (ex. RSSI) pendant le flux TCP ont coïncidé avec des trames nulles puis
 * le blocage du module (firmware NCP 2.0.89) : désactivées pendant l'envoi par défaut. */
/* SCAN3D_ECRAN=0 : LTDC jamais initialisé (diagnostic : flux Wi-Fi sans écran) */
#ifndef SCAN3D_ECRAN
#define SCAN3D_ECRAN 1
#endif
#ifndef SCAN3D_RSSI_PENDANT_ENVOI
#define SCAN3D_RSSI_PENDANT_ENVOI 0
#endif

/* Priorités : la tâche réseau tourne à la même priorité que la tâche iperf de ST (40,
 * au-dessus des tâches du pilote W6X à 24) : iperf tient 60 s sans décrochage alors
 * qu'à 16 la démo décrochait en quelques secondes. Capteur et écran restent en dessous. */
#define PRIO_CAPTEUR        18U
#define PRIO_RESEAU         40U
#define PRIO_ECRAN          12U

static uint8_t trame_capteur[FRAME_SIZE];   /* écrite par le capteur (sous mutex) */
static uint8_t trame_envoi[FRAME_SIZE];     /* copie envoyée par la tâche réseau */
static uint8_t trame_ecran[FRAME_SIZE];     /* copie affichée par la tâche écran */
static SemaphoreHandle_t mtx;
static TaskHandle_t h_reseau, h_capteur, h_ecran;
static volatile uint32_t ecran_actif = SCAN3D_ECRAN;          /* commande console : s3d_ecran 0|1 */
/* Envois par blocs de 1520 octets (= W61_MAX_SPI_XFER, comme iperf) : avec des appels de 9 Ko,
 * des trames entières arrivaient à zéro ; avec 1520, plus aucune (essai du 24/11/2026). */
static volatile uint32_t taille_envoi = 1520U;     /* commande console : s3d_bloc <octets> */
static volatile uint32_t periode_ms = FRAME_PERIOD_MS; /* commande console : s3d_cadence <ms> */
#ifndef SCAN3D_MOTIF_DEFAUT
#define SCAN3D_MOTIF_DEFAUT 0
#endif
/* Diagnostic : contenu des données de la trame (l'en-tête reste "S3DF"...).
 * 0 = carte simulée (binaire quelconque), 1 = zéros (comme iperf), 2 = texte ASCII. */
static volatile uint32_t motif = SCAN3D_MOTIF_DEFAUT;  /* commande console : s3d_motif 0|1|2 */
static volatile uint32_t trames_produites;
static volatile uint32_t trames_sautees;
static volatile uint32_t t_appel_reseau;    /* début de l'appel pilote en cours, 0 = aucun */

static LcdStatus_t st;
static uint32_t echecs_ncp;

/* Le module ne répond plus aux commandes AT : redémarrer la carte (le pilote
 * réinitialise le module via CHIP_EN au démarrage). */
/* Remise à zéro matérielle du module (CHIP_EN bas) puis redémarrage de la carte :
 * un simple NVIC_SystemReset() laisse le module dans son état bloqué. */
static void redemarrer_tout(void)
{
  HAL_GPIO_WritePin(CHIP_EN_GPIO_Port, CHIP_EN_Pin, GPIO_PIN_RESET);
  HAL_Delay(200);
  NVIC_SystemReset();
}

static void ncp_echec(const char *quoi)
{
  echecs_ncp++;
  LogWarn("Scan3D: échec module (%s), %lu/%lu\n", quoi, (unsigned long)echecs_ncp, (unsigned long)ECHECS_NCP_MAX);
  if (echecs_ncp >= ECHECS_NCP_MAX)
  {
    LogError("Scan3D: module Wi-Fi muet, redémarrage de la carte\n");
    vTaskDelay(pdMS_TO_TICKS(300));
    redemarrer_tout();
  }
}
static char ligne_pc[32];

/* ------------------------------ Capteur simulé ------------------------------- */
static void put32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

static uint32_t rng = 0x12345678U;
static uint32_t rnd(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }

/* Mur à ~1,6 m + sphère qui rebondit à ~0,7 m + bruit */
static void build_frame(uint8_t *f, uint32_t idx)
{
  memcpy(f, "S3DF", 4);
  put32(&f[4], idx);
  put32(&f[8], HAL_GetTick());
  put16(&f[12], SCAN_W);
  put16(&f[14], SCAN_H);
  put32(&f[16], PAYLOAD_SIZE);

  int32_t px = (int32_t)(idx % 88U);
  int32_t cx = (px < 44) ? (5 + px) : (5 + 88 - px);
  int32_t py = (int32_t)((idx / 2U) % 52U);
  int32_t cy = (py < 26) ? (8 + py) : (8 + 52 - py);
  const int32_t r2 = 9 * 9;

  uint8_t *z = &f[HEADER_SIZE];
  for (int32_t y = 0; y < (int32_t)SCAN_H; y++)
  {
    for (int32_t x = 0; x < (int32_t)SCAN_W; x++)
    {
      int32_t dx = x - cx, dy = y - cy, d2 = dx * dx + dy * dy;
      int32_t mm = 1600 + (y * 4);
      if (d2 < r2) { mm = 700 + d2 * 4; }
      mm += (int32_t)(rnd() % 21U) - 10;
      put16(z, (uint16_t)mm);
      z[2] = (d2 < r2) ? 230U : 180U;   /* confiance */
      z[3] = 5U;                        /* statut "valide" */
      z += ZONE_BYTES;
    }
  }
}

static void tache_capteur(void *arg)
{
  (void)arg;
  TickType_t wake = xTaskGetTickCount();
  for (;;)
  {
    xSemaphoreTake(mtx, portMAX_DELAY);
    build_frame(trame_capteur, trames_produites);
    if (motif == 1U)
    {
      memset(&trame_capteur[HEADER_SIZE], 0, PAYLOAD_SIZE);
    }
    else if (motif == 2U)
    {
      for (uint32_t i = 0; i < PAYLOAD_SIZE; i++) trame_capteur[HEADER_SIZE + i] = (uint8_t)('A' + (i % 26U));
    }
    trames_produites++;
    xSemaphoreGive(mtx);
    xTaskNotifyGive(h_reseau);            /* nouvelle trame disponible */
    vTaskDelayUntil(&wake, pdMS_TO_TICKS((periode_ms != 0U) ? periode_ms : 1U));
  }
}

/* ------------------------------- Réseau -------------------------------------- */
static void message(const char *msg, LcdNiveau_t niv)
{
  st.message = msg;
  st.message_niv = niv;
}

/* Encadre chaque appel bloquant du pilote pour le chien de garde */
#define APPEL(x)  (t_appel_reseau = HAL_GetTick() | 1U, (x)); t_appel_reseau = 0U

static int wifi_connect(void)
{
  W6X_WiFi_Connect_Opts_t opts = {0};
  W6X_WiFi_StaStateType_e state = W6X_WIFI_STATE_STA_OFF;
  W6X_WiFi_Connect_t cd = {0};
  uint8_t ip[4] = {0}, gw[4] = {0}, mask[4] = {0};
  W6X_Status_t r;

  st.wifi = "connexion..."; st.wifi_niv = LCD_ATTENTION; strcpy(st.ip, "-"); st.rssi_dbm = 0;
  message("Connexion au Wi-Fi...", LCD_ATTENTION);

  r = APPEL(W6X_WiFi_Station_GetState(&state, &cd));
  if (r != W6X_STATUS_OK)
  {
    ncp_echec("GetState");
    st.wifi = "module muet"; st.wifi_niv = LCD_ERREUR;
    return -1;
  }
  if (state != W6X_WIFI_STATE_STA_GOT_IP)
  {
    strncpy((char *)opts.SSID, SCAN3D_WIFI_SSID, W6X_WIFI_MAX_SSID_SIZE);
    strncpy((char *)opts.Password, SCAN3D_WIFI_PASSWORD, W6X_WIFI_MAX_PASSWORD_SIZE);
    r = APPEL(W6X_WiFi_Connect(&opts));
    if (r != W6X_STATUS_OK)
    {
      LogWarn("Scan3D: W6X_WiFi_Connect = %d\n", (int)r);
      st.wifi = "echec"; st.wifi_niv = LCD_ERREUR;
      return -1;
    }
    for (uint32_t i = 0; i < 40U; i++)   /* DHCP : jusqu'à 20 s */
    {
      APPEL(W6X_WiFi_Station_GetState(&state, &cd));
      if (state == W6X_WIFI_STATE_STA_GOT_IP) break;
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
  if (state != W6X_WIFI_STATE_STA_GOT_IP)
  {
    st.wifi = "pas d'IP"; st.wifi_niv = LCD_ERREUR;
    return -1;
  }
  r = APPEL(W6X_Net_Station_GetIPAddress(ip, gw, mask));
  if (r != W6X_STATUS_OK)
  {
    ncp_echec("GetIPAddress");
    st.wifi = "module muet"; st.wifi_niv = LCD_ERREUR;
    return -1;
  }
  snprintf(st.ip, sizeof(st.ip), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  st.rssi_dbm = cd.Rssi;
  st.wifi = "connecte"; st.wifi_niv = LCD_OK;
  LogInfo("Scan3D: Wi-Fi OK, IP %s, RSSI %" PRIi32 "\n", st.ip, st.rssi_dbm);
  return 0;
}

static int32_t pc_connect(void)
{
  struct sockaddr_in a = {0};
  uint8_t pc[4] = {0};
  unsigned b0, b1, b2, b3;
  int32_t timeout = SEND_TIMEOUT_MS, r;

  if (sscanf(SCAN3D_PC_IP, "%u.%u.%u.%u", &b0, &b1, &b2, &b3) != 4) return -1;
  pc[0] = (uint8_t)b0; pc[1] = (uint8_t)b1; pc[2] = (uint8_t)b2; pc[3] = (uint8_t)b3;

  int32_t s = APPEL(W6X_Net_Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
  if (s < 0)
  {
    ncp_echec("plus de socket libre");
    return -1;
  }
  (void)APPEL(W6X_Net_Setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)));
  a.sin_family = AF_INET;
  a.sin_port = PP_HTONS(SCAN3D_PC_PORT);
  a.sin_addr.s_addr = ATON(pc);
  r = APPEL(W6X_Net_Connect(s, (struct sockaddr *)&a, sizeof(a)));
  if (r != 0)
  {
    LogWarn("Scan3D: W6X_Net_Connect = %" PRIi32 "\n", r);
    (void)APPEL(W6X_Net_Close(s));
    return -1;
  }
  return s;
}

static int send_all(int32_t s, const uint8_t *p, uint32_t len)
{
  uint32_t done = 0;
  while (done < len)
  {
    uint32_t bloc = len - done;
    if (bloc > taille_envoi) bloc = taille_envoi;
    int32_t n = (int32_t)APPEL(W6X_Net_Send(s, &p[done], bloc, 0));
    if ((n <= 0) || ((uint32_t)n > bloc))
    {
      LogWarn("Scan3D: W6X_Net_Send = %" PRIi32 " (octets %" PRIu32 "/%" PRIu32 ")\n", n, done, len);
      return -1;
    }
    done += (uint32_t)n;
  }
  return 0;
}

static void tache_reseau(void *arg)
{
  (void)arg;
  uint32_t dernier_envoye = 0xFFFFFFFFU;

  for (;;)
  {
    while (wifi_connect() != 0)
    {
      message("Wi-Fi indisponible, nouvel essai dans 3 s", LCD_ERREUR);
      vTaskDelay(pdMS_TO_TICKS(3000));
    }

    st.lien = "connexion..."; st.lien_niv = LCD_ATTENTION;
    message("Connexion au PC (lancer recepteur_scan3d.py)", LCD_ATTENTION);
    int32_t s = pc_connect();
    if (s < 0)
    {
      st.lien = "PC injoignable"; st.lien_niv = LCD_ERREUR;
      message("PC injoignable : recepteur lance ? meme reseau ?", LCD_ERREUR);
      vTaskDelay(pdMS_TO_TICKS(2000));
      continue;
    }
    st.lien = "TCP, envoi"; st.lien_niv = LCD_OK;
    message("Envoi des trames 54x42 au PC", LCD_OK);
    LogInfo("Scan3D: connecté au PC %s\n", ligne_pc);

    uint32_t t_ref = HAL_GetTick(), f_ref = st.trames, t_rssi = HAL_GetTick();
    uint64_t octets = 0, o_ref = 0;

    for (;;)
    {
      (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));   /* attend une nouvelle trame */
      if (trames_produites == 0U) continue;                  /* rien de produit : ne pas envoyer un tampon vide */
      xSemaphoreTake(mtx, portMAX_DELAY);
      memcpy(trame_envoi, trame_capteur, FRAME_SIZE);
      xSemaphoreGive(mtx);

      uint32_t idx = (uint32_t)trame_envoi[4] | ((uint32_t)trame_envoi[5] << 8) |
                     ((uint32_t)trame_envoi[6] << 16) | ((uint32_t)trame_envoi[7] << 24);
      if (idx == dernier_envoye) continue;
      if ((dernier_envoye != 0xFFFFFFFFU) && (idx > dernier_envoye + 1U))
      {
        trames_sautees += idx - dernier_envoye - 1U;
      }

      if (send_all(s, trame_envoi, FRAME_SIZE) != 0)
      {
        st.erreurs++;
        st.reconnexions++;
        /* Après « Failed to send command » le module ne répond plus jamais :
         * inutile d'attendre 3 échecs, on le relance tout de suite. */
        LogError("Scan3D: envoi TCP échoué, relance immédiate du module\n");
        message("Module Wi-Fi bloque : relance...", LCD_ERREUR);
        vTaskDelay(pdMS_TO_TICKS(300));
        redemarrer_tout();
        st.lien = "perdu"; st.lien_niv = LCD_ERREUR;
        message("Liaison perdue, reconnexion...", LCD_ERREUR);
        break;
      }
      dernier_envoye = idx;
      echecs_ncp = 0U;                 /* le module répond : on remet le compteur à zéro */
      st.trames++;
      octets += FRAME_SIZE;

      uint32_t now = HAL_GetTick();
      if (now - t_ref >= 1000U)
      {
        uint32_t dt = now - t_ref;
        st.debit_kbps = (uint32_t)(((octets - o_ref) * 8U) / dt);   /* bits/ms = kbit/s */
        st.trames_par_s_x10 = ((st.trames - f_ref) * 10000U) / dt;
        t_ref = now; f_ref = st.trames; o_ref = octets;
      }
      if (SCAN3D_RSSI_PENDANT_ENVOI && (now - t_rssi >= 3000U))
      {
        W6X_WiFi_StaStateType_e state;
        W6X_WiFi_Connect_t cd = {0};
        W6X_Status_t r = APPEL(W6X_WiFi_Station_GetState(&state, &cd));
        if (r == W6X_STATUS_OK) st.rssi_dbm = cd.Rssi;
        t_rssi = now;
      }
    }
    (void)APPEL(W6X_Net_Close(s));
    st.debit_kbps = 0; st.trames_par_s_x10 = 0;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

/* ------------------------------- Écran --------------------------------------- */
static void tache_ecran(void *arg)
{
  (void)arg;
  char msg[64];

  uint32_t t_stats = 0;
  for (;;)
  {
    if (ecran_actif != 0U)
    {
      xSemaphoreTake(mtx, portMAX_DELAY);
      memcpy(trame_ecran, trame_capteur, FRAME_SIZE);
      xSemaphoreGive(mtx);
      LcdStatus_FondProfondeur(&trame_ecran[HEADER_SIZE], SCAN_W, SCAN_H, ZONE_BYTES);
    }
    if (HAL_GetTick() - t_stats >= 5000U)
    {
      t_stats = HAL_GetTick();
      LogInfo("Scan3D: envoyees %lu sautees %lu err %lu | tas libre %u (min %u) | pile libre net %lu capt %lu lcd %lu\n",
              (unsigned long)st.trames, (unsigned long)trames_sautees, (unsigned long)st.erreurs,
              (unsigned)xPortGetFreeHeapSize(), (unsigned)xPortGetMinimumEverFreeHeapSize(),
              (unsigned long)uxTaskGetStackHighWaterMark(h_reseau) * 4UL,
              (unsigned long)uxTaskGetStackHighWaterMark(h_capteur) * 4UL,
              (unsigned long)uxTaskGetStackHighWaterMark(h_ecran) * 4UL);
    }

    /* Chien de garde de la tâche réseau */
    uint32_t t0 = t_appel_reseau, maintenant = HAL_GetTick();
    /* t0 peut valoir maintenant+1 (bit 0 forcé à 1) : éviter le dépassement négatif */
    uint32_t bloque = ((t0 != 0U) && (maintenant > t0)) ? (maintenant - t0) : 0U;
    if (bloque > 5000U)
    {
      snprintf(msg, sizeof(msg), "Pilote Wi-Fi bloque depuis %lu s", (unsigned long)(bloque / 1000U));
      st.message = msg; st.message_niv = LCD_ERREUR;
      st.lien = "bloque"; st.lien_niv = LCD_ERREUR;
    }
    st.sautees = trames_sautees;
    if (ecran_actif != 0U) LcdStatus_Draw(&st);
    if (bloque > WATCHDOG_MS)
    {
      LogError("Scan3D: tâche réseau bloquée %lu ms, redémarrage\n", (unsigned long)bloque);
      vTaskDelay(pdMS_TO_TICKS(200));
      redemarrer_tout();
    }
    vTaskDelay(pdMS_TO_TICKS(LCD_PERIOD_MS));
  }
}

void Scan3D_Start(void)
{
  snprintf(ligne_pc, sizeof(ligne_pc), "%s:%u", SCAN3D_PC_IP, (unsigned)SCAN3D_PC_PORT);
  st.ssid = SCAN3D_WIFI_SSID;
  st.pc = ligne_pc;
  st.lien = "-"; st.lien_niv = LCD_DISCRET;
  st.wifi = "-"; st.wifi_niv = LCD_DISCRET;
  strcpy(st.ip, "-");

#if SCAN3D_ECRAN
  if (LcdStatus_Init("SCANNER 3D - demo Wi-Fi") != 0)
  {
    LogError("Scan3D: initialisation de l'écran échouée\n");
  }
#else
  LogInfo("Scan3D: version SANS ECRAN (LTDC non initialisé)\n");
#endif
  mtx = xSemaphoreCreateMutex();
  if ((mtx == NULL) ||
      (xTaskCreate(tache_reseau, "s3d_net", 8192U / 4U, NULL, PRIO_RESEAU, &h_reseau) != pdPASS) ||
      (xTaskCreate(tache_capteur, "s3d_capt", 2048U / 4U, NULL, PRIO_CAPTEUR, &h_capteur) != pdPASS) ||
      (xTaskCreate(tache_ecran, "s3d_lcd", 6144U / 4U, NULL, PRIO_ECRAN, &h_ecran) != pdPASS))
  {
    LogError("Scan3D: création des tâches impossible (tas FreeRTOS ?)\n");
  }
}

/* ----------------------- Commandes console (diagnostic) ---------------------- */
static int32_t s3d_ecran(int32_t argc, char **argv)
{
  if ((argc > 1) && SCAN3D_ECRAN) ecran_actif = (uint32_t)atoi(argv[1]);
  LogInfo("Scan3D: rafraichissement ecran %s\n", (ecran_actif != 0U) ? "actif" : "arrete");
  return 0;
}
SHELL_CMD_EXPORT_ALIAS(s3d_ecran, s3d_ecran, s3d_ecran [ 0:arret ; 1:actif ]. Rafraichissement de l ecran);

static int32_t s3d_bloc(int32_t argc, char **argv)
{
  if (argc > 1)
  {
    int32_t v = atoi(argv[1]);
    if ((v >= 64) && (v <= (int32_t)FRAME_SIZE)) taille_envoi = (uint32_t)v;
  }
  LogInfo("Scan3D: %lu octets max par W6X_Net_Send\n", (unsigned long)taille_envoi);
  return 0;
}
SHELL_CMD_EXPORT_ALIAS(s3d_bloc, s3d_bloc, s3d_bloc [ octets 64..9092 ]. Taille max par envoi TCP);

static int32_t s3d_motif(int32_t argc, char **argv)
{
  if (argc > 1) motif = (uint32_t)atoi(argv[1]);
  LogInfo("Scan3D: motif %lu (0 carte simulee, 1 zeros, 2 texte)\n", (unsigned long)motif);
  return 0;
}
SHELL_CMD_EXPORT_ALIAS(s3d_motif, s3d_motif, s3d_motif [ 0:carte ; 1:zeros ; 2:texte ]. Contenu des trames);

static int32_t s3d_cadence(int32_t argc, char **argv)
{
  if (argc > 1) periode_ms = (uint32_t)atoi(argv[1]);
  LogInfo("Scan3D: une trame toutes les %lu ms\n", (unsigned long)periode_ms);
  return 0;
}
SHELL_CMD_EXPORT_ALIAS(s3d_cadence, s3d_cadence, s3d_cadence [ ms ]. Periode du capteur simule);
