# Démo autonome « Scanner 3D » : Wi-Fi + écran du DK

Cette démo tourne sur le STM32N6570-DK avec la X-NUCLEO-67W61M1, alimenté par une batterie externe branchée sur l'USB-C ST-LINK, sans console. Au démarrage :

1. l'écran s'allume ;
2. le DK se connecte au Wi-Fi ;
3. il ouvre une connexion TCP vers le PC ;
4. il envoie 30 trames lidar simulées par seconde, de 54×42 zones à 4 octets, soit environ 9 Ko par trame et 2,2 Mbit/s ;
5. il affiche l'état de la liaison par-dessus une carte de profondeur colorée.

| Fichier | Rôle |
|---|---|
| `firmware/lcd_status.c/.h` | **Module d'affichage réutilisable** : fond pour la caméra + couche de statut. Indépendant du Wi-Fi. |
| `firmware/scan3d_demo.c/.h` | Logique de la démo : Wi-Fi, TCP, trames simulées. Lancée par `Scan3D_Start()` dans `main_app()`. |
| `firmware/stm32n6570_discovery_conf.h` | Configuration BSP : adresses des framebuffers. |
| `firmware/wifi_secrets_template.h` | Modèle : à copier en `wifi_secrets.h`, qui n'est **jamais** versionné. |
| `creer_projet_scan3d.py` | Crée le projet `ST67W6X_Scan3D_DK` à partir de `ST67W6X_CLI_DK`. |
| `pc/recepteur_scan3d.py` | Récepteur PC en Python 3 standard : trames/s, débit, trames perdues, CSV. |

## Construire et flasher

```bash
git clone https://github.com/STMicroelectronics/x-cube-st67w61   # v1.3.0
cd x-cube-st67w61
python3 <repo>/docs/test_wifi_DK/port_STM32N6570-DK.py Projects/NUCLEO-N657X0-Q/Applications/ST67W6X/ST67W6X_CLI
cp <repo>/docs/test_wifi_DK/demo_scan3d/firmware/wifi_secrets_template.h ~/wifi_secrets.h   # à remplir
python3 <repo>/docs/test_wifi_DK/demo_scan3d/creer_projet_scan3d.py . ~/wifi_secrets.h
```

Compiler `Projects/.../ST67W6X_Scan3D_DK/STM32CubeIDE/FSBL`, nom Eclipse `ST67W6X_Scan3D_FSBL`, en Release. Ensuite, signer et flasher comme indiqué dans `RESULTATS_test_wifi_DK.md` (fichier `ST67W6X_Scan3D_FSBL-trusted.bin` à `0x70000000`).

Côté PC, sur le même réseau :

```bash
python3 pc/recepteur_scan3d.py 5002 --save trames
```

Protocole : un en-tête de 20 octets `<4s I I H H I>` (`"S3DF"`, numéro, temps en ms, largeur, hauteur, taille), puis les zones `<H B B>` (distance en mm, confiance, statut).

## Module d'affichage `lcd_status`

| | Couche 0 « fond » | Couche 1 « statut » |
|---|---|---|
| Usage | aperçu caméra (futur) ; démo : mire + carte de profondeur | bandeau titre, panneau d'état, message du bas |
| Format | RGB565 | ARGB4444 (alpha par pixel, mélangé au fond) |
| Résolution | 800×480 | 800×480, transparente hors panneau |
| Adresse | `0x34200000` (`LCD_LAYER_0_ADDRESS`) | `0x342C0000` (`LCD_LAYER_1_ADDRESS`) |
| Taille | 768 000 octets | 768 000 octets |

- **Mémoire.** Les framebuffers sont en RAM interne AXISRAM3-6 (`0x34200000`-`0x343BFFFF`, 1,75 Mo), allumée par `LcdStatus_Init()` : horloge et sortie de shutdown via `RAMCFG`. On utilise environ 1,5 Mo, il reste environ 250 Ko. La RAM du programme (AXISRAM1, 1,5 Mo) et le code (AXISRAM2) ne sont pas modifiés. La PSRAM externe de 32 Mo du DK (XSPI1) n'est pas utilisée.
- **Horloge.** IC16 ← PLL4 / 64 = 25 MHz. La fonction `MX_LTDC_ClockConfig()` est redéfinie, car la PLL4 du projet tourne à 1600 MHz.
- **RIF.** Le LTDC est déclaré maître sécurisé et privilégié, puisque le FSBL tourne en sécurisé et les buffers sont lus via l'alias `0x34…`. LTDC, LTDCL1 et LTDCL2 sont déclarés esclaves sécurisés.
- **Cache.** `SCB_CleanDCache()` est appelé après chaque dessin, car le D-cache du M55 est actif.
- **API.** `LcdStatus_Init()`, `LcdStatus_Draw(&etat)`, `LcdStatus_Fond()` (pointeur vers le framebuffer RGB565), `LcdStatus_FondMire()`, `LcdStatus_FondProfondeur()`.

### Broches occupées par l'écran (internes au DK)

LTDC + dalle RK050HR18 :
- PA0 PA1 PA2 PA7 PA8 PA15 ;
- PB2 PB4 PB11 PB12 PB13 PB14 PB15 ;
- PD8 PD9 PD15 ;
- PE1 PE11 ;
- PG0 PG1 PG6 PG8 PG11 PG12 PG13 ;
- PH3 PH4 PH6 ;
- PQ3 (LCD_ON), PQ4 (INT tactile), PQ6 (rétroéclairage).

Vérification avec les signaux de la carte fille A :

| Signal carte A | Broche DK | Conflit avec l'écran ? |
|---|---|---|
| SPI5 D10 CS / D11 MOSI / D12 MISO / D13 SCK | PA3 / PG2 / PH8 / PE15 | non |
| D2 SPI_RDY | PD0 | non |
| D7 CHIP_EN | PD6 | non |
| Console ST-LINK | PE5 / PE6 | non |
| D14 / D15 (I3C) | à relever sur le schéma MB1939 | à vérifier, mais aucune broche PC/PF n'est utilisée par l'écran |

PB4 et PA15 (JTAG) sont pris par l'écran : il faut déboguer en SWD seulement, ce qui est le cas avec le ST-LINK.

### Reste à faire pour l'aperçu caméra (IMX335)

1. Ajouter le BSP caméra du DK (`stm32n6570_discovery_camera.c`), les pilotes `imx335` + ISP (`isp_conf.h`), et les HAL DCMIPP + CSI.
2. Configurer le pipe DCMIPP (pipe 1 avec réduction de taille) : 2592×1944 RAW → ISP → 800×480 RGB565, écrit en continu dans `LcdStatus_Fond()` (`0x34200000`). Pour éviter le déchirement, prévoir un double buffer : second buffer en AXISRAM restante si la taille le permet, sinon en PSRAM avec `BSP_XSPI_RAM`, puis basculer l'adresse de couche à chaque VSYNC (`BSP_LCD_SetLayerAddress`).
3. Ajouter au RIF le DCMIPP comme maître sécurisé (RIMC), comme pour le LTDC.
4. Arrêter d'appeler `LcdStatus_FondMire()` et `LcdStatus_FondProfondeur()`. La couche de statut reste inchangée.
5. Si le NPU ou l'IA utilisent aussi AXISRAM3-6, déplacer les framebuffers en PSRAM : il suffit de changer `LCD_LAYER_*_ADDRESS`.

## État de validation (24/11/2026)

- Compilation : OK (0 erreur, 0 warning).
- Écran : OK (fond, carte simulée, panneau d'état superposé).
- Wi-Fi : connexion automatique et envoi de 20 à 30 trames/s vers le PC. Le module décroche cependant après quelques secondes. La carte le relance alors automatiquement (CHIP_EN + redémarrage). Détails et pistes dans `../RESULTATS_test_wifi_DK.md`.

### Commandes console de diagnostic (921600 bauds)

| Commande | Effet |
|---|---|
| `s3d_ecran 0/1` | coupe ou relance le rafraîchissement de l'écran |
| `s3d_bloc <octets>` | taille maximale par `W6X_Net_Send` (1520 par défaut ; au-delà, des trames entières arrivent nulles) |
| `s3d_cadence <ms>` | période du capteur simulé (33 ms par défaut) |
| `s3d_motif 0/1/2` | contenu des trames : carte simulée, zéros ou texte |

`#define SCAN3D_ECRAN 0` compile une variante sans écran (LTDC non initialisé).
