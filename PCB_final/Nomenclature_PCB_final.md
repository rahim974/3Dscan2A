# Nomenclature du PCB final : scanner 3D portable (STM32H563, sans fil, composants ST)

Cette liste est extraite de la netlist du schéma `PCB_final` le 20/10/2026. **Tous les composants actifs sont des composants STMicroelectronics.** Seuls l'oscillateur, les LEDs, les connecteurs et les passifs restent génériques, car ST n'en fabrique pas.

## 1. Circuits intégrés, modules et semi-conducteurs (tous ST)

| Repère | Composant | Référence ST | Boîtier | Utilité |
|---|---|---|---|---|
| U8 | Microcontrôleur | STM32H563VIT6 | LQFP-100 | Pilote les capteurs (I3C, I²C), horodate les mesures et les envoie par Wi-Fi ou USB |
| U6 | Lidar 3D ToF | VL53L9CX | Module LGA | Mesure de profondeur (jusqu'à 54×42 zones, 8,8 m) |
| U7 | Centrale inertielle 6 axes | LSM6DSV16X | LGA-14 | Orientation et mouvement du scanner tenu à la main |
| IC1 | Module Wi-Fi 6 / BLE 5.4 | ST67W611M1A6B | Module LGA, antenne PCB | Envoi sans fil des scans vers l'application PC |
| U1 | Chargeur Li-ion linéaire 800 mA | STC4054GR | TSOT23-5 | Charge la batterie depuis l'USB-C |
| U2 | Régulateur LDO 3,3 V 1,5 A | LD39150DT33 | DPAK | Rail +3V3 (Wi-Fi, AVDD du lidar, VDDUSB) |
| U3 | Régulateur LDO 1,8 V 300 mA | LDK130-18 | SOT-23-5 | Rail +1V8 (MCU, I/O du lidar, IMU) |
| U4 | Régulateur LDO 1,2 V 300 mA | LDK130-12 | SOT-23-5 | Rail +1V2 = DVDD du lidar |
| U5 | Protection ESD USB | USBLC6-2SC6 | SOT-23-6 | Protège D+/D− et VBUS du connecteur USB-C |
| D1 | Diode Schottky 1 A 30 V | STPS1L30A | SMA | Power-path : l'USB alimente le système quand il est branché |
| Q1 | MOSFET canal P | STR2P3LLH6 *(référence à confirmer)* | SOT-23 | Power-path : la batterie alimente le système, coupée quand l'USB est branché |

## 2. Composants génériques (ST n'en fabrique pas)

| Repère | Composant | Référence | Utilité |
|---|---|---|---|
| Y1 | Oscillateur 24 MHz 1,8 V | ECS-2520MV-240 | Horloge partagée : HSE du MCU et AP_CLK du lidar |
| J2 | Connecteur USB-C 16 broches | GCT USB4105 | Charge + liaison USB avec le PC |
| J1 | Connecteur batterie | JST PH 2 broches | Batterie Li-ion 1S |
| J4 | Connecteur SWD | 2×5 au pas de 1,27 mm | Programmation, debug et reset (ST-LINK) |
| J3 | Header 1×4 | 2,54 mm | UART + BOOT du module Wi-Fi (mise à jour de son firmware) |
| SW1 | Interrupteur à glissière | PCM12 | Marche / arrêt |
| SW2 | Bouton poussoir | PTS810 | Bouton utilisateur « SCAN » |
| D2 | LED rouge 0603 | — | Charge en cours |
| D3 | LED rouge 0603 | — | LED d'état pilotée par le MCU |

## 3. Résistances (toutes en 0402)

| Valeur | Qté | Repères | Utilité |
|---|---|---|---|
| 10 kΩ | 5 | R13, R14, R20, R21, R22 | XSHUT et INTR du lidar, BOOT du Wi-Fi, BOOT0 à la masse, bouton SCAN |
| 100 kΩ | 4 | R2, R5, R8, R19 | Grille du P-MOS, ponts diviseurs (batterie, VBUS), pull-up CHIP_EN du Wi-Fi |
| 2,2 kΩ | 2 | R17, R18 | Pull-ups I²C de l'IMU |
| 5,1 kΩ | 2 | R6, R7 | CC1/CC2 de l'USB-C |
| 33 Ω | 2 | R10, R11 | Résistances série de l'oscillateur (vers le lidar et vers le MCU) |
| 1,24 kΩ | 1 | R1 | PROG du chargeur (≈ 800 mA, *à vérifier*) |
| 1,5 kΩ | 1 | R3 | LED de charge |
| 200 kΩ | 1 | R4 | Pont diviseur de mesure batterie |
| 47 kΩ | 1 | R9 | Pont diviseur VBUS_DET |
| 1 kΩ | 1 | R23 | LED d'état |
| 2,2 kΩ, **non montée** | 2 | R15, R16 | Repli I²C du lidar si l'I3C pose problème |
| 0 Ω, **non montée** | 1 | R12 | Option : AP_CLK fournie par le MCO1 du MCU |

## 4. Condensateurs

| Valeur | Boîtier | Qté | Utilité |
|---|---|---|---|
| 100 nF | 0402 | 22 | Découplage au plus près de chaque broche d'alimentation + filtres RESET et mesure batterie |
| 1 µF | 0402 | 8 | Entrées/sorties des LDO 1V8 et 1V2, IOVDD du lidar, VDDA / VREF+ / VDDUSB du MCU |
| 10 µF | 0603 | 5 | Entrée du LDO 3V3, VBUS, découplage réservoir (lidar, IMU, Wi-Fi) |
| 4,7 µF | 0603 | 5 | Entrée/sortie du chargeur, AVDD et DVDD du lidar, VDD du MCU |
| 22 µF | 0805 | 4 | VSYS, sortie 3V3, VBAT_LDD du lidar (pics ~0,75 A) |
| 2,2 µF *(à vérifier)* | 0402 | 2 | VCAP du régulateur interne du MCU |

## 5. Totaux

| Catégorie | Quantité |
|---|---|
| **Condensateurs** | **46** |
| **Résistances** | **23** (dont 3 non montées) |
| Circuits intégrés et modules | 9 |
| Diode + MOSFET | 2 |
| Oscillateur | 1 |
| LEDs | 2 |
| Bouton / interrupteur | 2 |
| Connecteurs | 4 |
| **Total** | **89** (86 à monter) |

Hors carte : une **batterie Li-ion / LiPo 1S protégée** (≥ 2000 mAh, connecteur JST PH).

## 6. Alimentation : ce qui a changé avec le passage au 100 % ST

- **Chargeur** : le TI BQ24074 est remplacé par un ST **STC4054** (plus simple, 800 mA). Le STC4054 n'a pas de power-path intégré, je l'ai donc refait avec une diode Schottky et un MOSFET canal P.
- **3,3 V** : la bibliothèque KiCad n'a pas de buck-boost ST. Le buck-boost TI est donc remplacé par un **LDO LD39150** (1,5 A). Inconvénient : quand la batterie descend sous ~3,5 V, le 3,3 V décroche. On perd environ 10 % de l'autonomie, et le firmware doit couper proprement grâce à la mesure VBAT_SENSE. ST fabrique des buck-boost (famille STBB) : on peut en remettre un si on récupère son symbole.
- **1,8 V et 1,2 V** : LDO ST **LDK130**, tous deux alimentés par le +3V3.
- **Translateur TI SN74LVC1T45 supprimé** : CHIP_EN du Wi-Fi est tiré au +3V3 et commandé par PB1 en open-drain. Il faut vérifier que PB1 est bien une broche tolérante 5 V (FT) du H563.
- **Pas de protection batterie sur la carte** : il faut une batterie avec protection intégrée (PCM).
