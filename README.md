# 3Dscan2A : prototype A (carte fille sur STM32N6570-DK)

Scanner 3D portable, projet ENSEA 2A. Cette branche (`prototype_A`) porte le **prototype A** : une carte fille enfichée sur le kit ST **STM32N6570-DK**, comme un shield Arduino. Le kit apporte le calcul (STM32N6 avec NPU), la caméra IMX335 et l'écran. La carte fille apporte les capteurs et le sans-fil.

Le prototype C (carte autonome STM32H7S3, LQFP-144) viendra ensuite et réutilisera les blocs lidar, IMU et Wi-Fi validés ici.

```
            ┌──────────────────────── Carte fille (ce PCB) ────────────────────────┐
            │  VL53L9CX ◄─ I3C 1,8 V ─ P3A9606 ─ I3C 3,3 V ─┐                      │
            │  LSM6DSV16X ◄──────────── I²C 3,3 V ──────────┤                      │
            │  ST67W611M1 ◄──────────── SPI 3,3 V ──────────┤   connecteurs        │
            │  LDO 3V3 / 1V8 / 1V2, oscillateur 24 MHz      ├── Arduino (+STMod+)  │
            └───────────────────────────────────────────────┼──────────────────────┘
                                                            ▼
                      STM32N6570-DK : STM32N6 + NPU, caméra IMX335, écran, USB
                                                            │ Wi-Fi 6 / USB
                                                            ▼
                                                 PC : reconstruction 3D
```

---

## 1. Le PCB KiCad à concevoir

Une carte 2 couches (4 si le module Wi-Fi l'exige), au format shield Arduino, sans microcontrôleur, sans BGA et sans mémoire externe.

**Feuilles de schéma prévues**

| Feuille | Contenu |
|---|---|
| `connecteurs` | Connecteurs Arduino Uno R3 (mâles, vers le kit), STMod+ si on l'utilise, points de test |
| `alimentation` | LDO 3V3 depuis le 5 V du kit (si le 3V3 du kit ne suffit pas), LDO 1V8 et 1V2 pour le lidar, LED de présence |
| `lidar` | VL53L9CX, découplage (figure 7 du datasheet), oscillateur 24 MHz, translateurs de niveau I3C et GPIO |
| `imu` | LSM6DSV16X, pull-ups I²C si besoin, INT1/INT2 |
| `wifi` | ST67W611M1, SPI + SPI_RDY + CHIP_EN, header UART/BOOT pour sa mise à jour, zone d'antenne dégagée |

**Contraintes de placement**

- Le lidar doit regarder **dans la même direction que la caméra** du kit, sans masquer l'écran ni les boutons. Sa position sur la carte dépend des mesures du §3.
- L'**antenne PCB** du module Wi-Fi doit être en bord de carte, sans cuivre ni composant dessous ni autour (voir le datasheet du module), et loin des parties métalliques du kit.
- L'IMU près du lidar, sur la même carte rigide, pour que la transformation lidar ↔ IMU reste fixe.
- Les pistes I3C courtes et sans stub, le translateur au plus près du lidar.

---

## 2. Composants nécessaires

### Actifs

| Composant | Référence | Boîtier | Utilité |
|---|---|---|---|
| Lidar 3D ToF | ST **VL53L9CX** | Module LGA | Carte de profondeur 54×42 zones, jusqu'à 8,8 m |
| Centrale inertielle 6 axes | ST **LSM6DSV16X** | LGA-14 | Orientation et mouvement du scanner (fusion SFLP) |
| Module Wi-Fi 6 / BLE 5.4 | ST **ST67W611M1** | Module LGA, antenne PCB | Envoi des scans au PC, commandes en BLE |
| Translateur I3C 3,3 V ↔ 1,8 V | NXP **P3A9606** | — | Passe l'I3C du N6 (3,3 V) au lidar (1,8 V) à 12,5 MHz |
| Translateur GPIO 3,3 V ↔ 1,8 V | à choisir (ex. TI SN74LVC2T45 ×2, ou NXP NTB0104) | SOT/XSON | XSHUT, INTR et SYNC_IN du lidar |
| Oscillateur 24 MHz 1,8 V | ECS-2520MV-240 | 2520 | Horloge AP_CLK du lidar |
| LDO 1,8 V 300 mA | ST **LDK130-18** | SOT-23-5 | IOVDD du lidar, côté 1,8 V des translateurs, oscillateur |
| LDO 1,2 V 300 mA | ST **LDK130-12** | SOT-23-5 | DVDD du lidar |
| LDO 3,3 V 1,5 A *(si besoin, voir §3)* | ST **LD39150DT33** | DPAK | 3V3 propre depuis le 5 V du kit pour le Wi-Fi et le lidar (pics ≈ 0,75 A) |

### Passifs et connectique

| Composant | Utilité |
|---|---|
| Barrettes mâles Arduino Uno R3 : 1×10, 1×8, 1×8, 1×6, pas de 2,54 mm (hauteur à choisir selon §3) | Enfichage sur le kit |
| Connecteur STMod+ 2×10 *(optionnel, type et pas à vérifier)* | Accès au SPI5 / UART / GPIO supplémentaires |
| Header 1×4, 2,54 mm | UART + BOOT du module Wi-Fi |
| Condensateurs 100 nF / 1 µF (0402), 4,7 µF / 10 µF (0603), 22 µF (0805) | Découplage du lidar, du Wi-Fi, de l'IMU et des LDO |
| Résistances 10 kΩ, 33 Ω, 2,2 kΩ non montées, 0 Ω | Pull-ups XSHUT/INTR, série de l'horloge, repli I²C du lidar, options |
| LED + résistance | Présence d'alimentation |

Les valeurs exactes sont reprises de la nomenclature du schéma H563 (`Nomenclature_PCB_final`), qui partage les blocs lidar, IMU et Wi-Fi.

---

## 3. Mesures et vérifications à faire sur le kit

À faire **avant** de dessiner le contour du PCB. Sources : schéma MB1939 (STM32N6570-DK, rév. C-02), user manual UM3300 et le kit lui-même avec un pied à coulisse.

### Mécanique

- [ ] **Face des connecteurs Arduino et STMod+** : dessus ou dessous du kit, par rapport à l'écran et à la caméra.
- [ ] **Position exacte** des quatre barrettes Arduino (cote depuis un coin de la carte, entraxe) et du STMod+.
- [ ] **Position et orientation de la caméra** IMX335 (module B-CAMS-IMX sur nappe) : direction de visée, possibilité de la fixer à côté du lidar.
- [ ] **Encombrement** au-dessus des barrettes : écran, boutons, connecteurs USB, nappe caméra, jack audio. En déduire la hauteur des barrettes et la zone libre pour la carte fille.
- [ ] Trous de fixation disponibles et hauteur des composants les plus hauts du kit sous la carte fille.

### Électrique

- [ ] **Tension des I/O** sur les connecteurs Arduino et STMod+ (VDDIO à 3,3 V ou 1,8 V ? réglable ?). Si les I/O sont déjà en 1,8 V, on retire les translateurs.
- [ ] **Courant disponible** sur les broches 3V3 et 5V de l'Arduino. Le lidar et le Wi-Fi demandent des pics d'environ 0,75 A : si le 3V3 du kit ne tient pas, on ajoute le LD39150 sur le 5 V.
- [ ] Le kit peut-il être **alimenté par la carte fille** (VIN / 5V de l'Arduino) ? Sinon il reste sur l'USB.
- [ ] **Pull-ups 1,5 kΩ** de l'I2C1 (PH9/PC1, R153/R149 vers VDDIO) : vérifier si elles gênent l'I3C et s'il faut retirer R153/R149.

### Broches (avec STM32CubeMX sur le STM32N657X0H3)

Relevé du schéma MB1939 page 3 :

| Connecteur | Broches | Port du N6 |
|---|---|---|
| Arduino D0–D7 | D0 PF6, D1 PD5, D2 PD0, D3 PE9, D4 PH5 (ou PC13 par SB44), D5 PE10, D6 PE13, D7 PD6 | |
| Arduino D8–D13 | D8 PE7, D9 PE14, **D10 PA3, D11 PG2, D12 PH8, D13 PE15** | D10–D13 = **SPI5** (NSS, MOSI, MISO, SCK) |
| Arduino A0–A5 | A0 PA5, A1 PA9, A2 PA10, A3 PA12, A4 PF3, A5 PB10 | |
| I2C1 | SCL PH9, SDA PC1 | pull-ups 1,5 kΩ, a priori sur D15/D14 de l'Arduino *(à confirmer)* |
| I2C2 | SCL PD14, SDA PD4 | pull-ups 1,5 kΩ |
| STMod+ SPI5 | NSS PA3, MOSI PG2, MISO PH8, SCK PE15 | **partagé** avec D10–D13 |
| STMod+ divers | UART TX PD5 / RX PF6 / CTS PG5 / RTS PG14, INT PC11, RST PB3, PWM PC7, ADC PC9, IO1–IO4 PD13 / PF1 / PB8 / PG9 | TX/RX partagés avec D0/D1 |

À vérifier :

- [ ] **I3C** : quelles broches I3C1/I3C2 du N6 tombent sur l'Arduino ou le STMod+ (PH9/PC1 ? PD14/PD4 ?). C'est le point bloquant du prototype A.
- [ ] **SPI5** sur D10–D13 pour le Wi-Fi, et une broche libre pour SPI_RDY (interruption EXTI) et une pour CHIP_EN.
- [ ] Une sortie **timer** libre pour SYNC_IN du lidar, et des EXTI pour INTR du lidar et INT1/INT2 de l'IMU.
- [ ] Conflits avec les fonctions du kit déjà câblées sur ces broches (Ethernet, LCD, audio, SD, USB…).
- [ ] Le connecteur caméra porte déjà **TOF_INT / TOF_LPn (PQ0/PQ5) et IMU_INT1/2 (PQ1/PQ2)** : vérifier ce que le module caméra embarque (ToF et IMU ?) et si on peut s'en servir pour les premiers tests.

---

## 4. Étapes suivantes

1. **Relevés sur le kit** (§3) : cotes mécaniques, tension des I/O, courant disponible, broches I3C/SPI. Les noter dans `docs/kit_mesures.md`.
2. **Tests sans PCB** sur le kit : faire tourner un exemple ST caméra + IA, puis brancher l'IMU (X-NUCLEO-IKS4A1) et le module Wi-Fi (X-NUCLEO-67W61M1) sur l'Arduino pour valider les broches et les drivers.
3. **Lidar** : kit d'évaluation VL53L9CX relié au kit par un translateur I3C, ou en I²C si l'I3C n'est pas sorti.
4. **Schéma KiCad** de la carte fille (feuilles du §1), ERC sans erreur, revue des valeurs avec les datasheets.
5. **Placement et routage** : contour issu des cotes du §3, lidar aligné avec la caméra, antenne en bord de carte, DRC sans erreur.
6. **Fabrication** (Gerbers, BOM, fichiers de placement), commande des composants, montage.
7. **Mise en route** : rails d'alimentation, puis IMU, Wi-Fi, lidar un par un, puis fusion lidar + IMU + caméra et IA sur le NPU, envoi au PC.

## Documents du projet

- `Presentation_prototypes` : comparaison des prototypes A, B et C.
- `Architecture_PCB_final` et `Nomenclature_PCB_final` : schéma de la carte H563 (prototype B), dont on reprend les blocs lidar, IMU et Wi-Fi.
- Schéma du kit : MB1939 (STM32N6570-DK), page 3 « STM32 microcontroller IOs ».
