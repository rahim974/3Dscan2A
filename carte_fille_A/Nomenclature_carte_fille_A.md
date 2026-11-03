# Nomenclature de la carte fille A, feuille par feuille

Composants du schéma `carte_fille_A.kicad_sch`, état du 03/11/2026 (commit 668abcc). **DNP** = empreinte prévue sur le PCB, composant non monté. La page 1 (racine) ne contient que les blocs des feuilles, sans composant.

## Page 2 : Connecteurs Arduino

| Repère | Composant | Boîtier | Utilité |
|---|---|---|---|
| J1 | Barrette mâle 1×8 (CN8) | 2,54 mm, traversant | Prend le **5 V** et la masse du kit. Donne aussi IOREF, la tension des E/S du kit. |
| J2 | Barrette mâle 1×6 (CN7) | 2,54 mm | A4/A5 : **bus I3C de repli** si D14/D15 ne conviennent pas. A0–A3 non utilisées. |
| J3 | Barrette mâle 1×8 (CN11) | 2,54 mm | D0–D7 : UART du Wi-Fi, SPI_RDY, SYNC_IN et INTR du lidar, INT1 de l'IMU, CHIP_EN du Wi-Fi |
| J4 | Barrette mâle 1×10 (CN12) | 2,54 mm | D8 = XSHUT du lidar, D9 = INT2 de l'IMU, D10–D13 = **SPI5 vers le Wi-Fi**, D14/D15 = **bus I3C/I²C** du lidar et de l'IMU, GND |
| TP1 | Point de test | Pastille Ø 1,5 mm | Mesurer IOREF pour vérifier que le kit est bien en 3,3 V |

## Page 3 : Alimentation

| Repère | Composant | Boîtier | Utilité |
|---|---|---|---|
| U1 | ST **LD39150DT33**, LDO 3,3 V 1,5 A | DPAK (TO-252) | Fabrique le **+3V3** à partir du 5 V du kit : Wi-Fi, lidar (AVDD, VBAT), IMU, translateurs |
| U2 | ST **LDK130-18**, LDO 1,8 V 300 mA | SOT-23-5 | Fabrique le **+1V8** : E/S du lidar (IOVDD), oscillateur, côté lidar des translateurs |
| U3 | ST **LDK130-12**, LDO 1,2 V 300 mA | SOT-23-5 | Fabrique le **+1V2** : cœur numérique du lidar (DVDD) |
| C1 | 10 µF | 0603 | Découplage de l'entrée 5 V du LD39150 |
| C2, C3 | 22 µF | 0805 | Sortie du LD39150 : stabilité du LDO et réserve pour les pics de courant du lidar et du Wi-Fi |
| C4 | 100 nF | 0402 | Filtrage haute fréquence du +3V3 |
| C5, C7 | 1 µF | 0402 | Entrées des LDK130 (côté +3V3) |
| C6 | 1 µF | 0402 | Sortie du LDK130-18 (+1V8) |
| C8 | 1 µF | 0402 | Sortie du LDK130-12 (+1V2) |
| R1 | 1 kΩ | 0402 | Limite le courant de la LED de présence (~1,5 mA) |
| D1 | LED verte | 0603 | Témoin : le +3V3 est présent |
| TP2 à TP6 | Points de test | Pastilles Ø 1,5 mm | Mesurer +5V, +3V3, +1V8, +1V2 et GND à la mise sous tension |

## Page 4 : Lidar et translateurs

| Repère | Composant | Boîtier | Utilité |
|---|---|---|---|
| U4 | ST **VL53L9CX**, lidar 3D à temps de vol | Module LGA | Mesure la **profondeur** : 54×42 zones, jusqu'à 8,8 m |
| U5 | NXP **P3A9606**, translateur I3C 2 bits | X2SON-8 *(boîtier et brochage à vérifier)* | Fait passer **SCL/SDA de l'I3C** de 3,3 V (kit) à 1,8 V (lidar), à 12,5 MHz |
| U6 | TI **SN74LVC1T45** | SOT-23-6 | Translateur 3,3 V → 1,8 V pour **XSHUT** (allumage et extinction du lidar) |
| U7 | TI **SN74LVC1T45** | SOT-23-6 | Translateur 3,3 V → 1,8 V pour **SYNC_IN** (déclenchement des mesures, synchronisé avec l'IMU) |
| U8 | TI **SN74LVC1T45** | SOT-23-6 | Translateur 1,8 V → 3,3 V pour **INTR** (le lidar signale qu'une mesure est prête) |
| Y1 | Oscillateur 24 MHz 1,8 V (ECS-2520MV) | 2,5 × 2,0 mm | Horloge **AP_CLK** du lidar |
| R2 | 33 Ω | 0402 | Résistance série sur l'horloge : limite les rebonds et le rayonnement |
| R3, R4 | 0 Ω | 0402 | Relient l'I3C du lidar à **D15/D14** (choix par défaut) |
| R5, R6 | 0 Ω **DNP** | 0402 | Variante : I3C sur **A5/A4**. On monte celles-ci et on retire R3/R4 si D14/D15 ne gèrent pas l'I3C. |
| R7 | 10 kΩ | 0402 | Tire XSHUT à 0 côté kit : lidar éteint tant que le N6 ne l'allume pas |
| R8 | 10 kΩ | 0402 | Même rôle côté lidar, quand le translateur n'est pas alimenté |
| R9 | 10 kΩ | 0402 | Tire SYNC_IN à 0 au repos |
| R10 | 10 kΩ | 0402 | Pull-up de INTR (sortie open-drain du lidar) au +1V8 |
| R11, R12 | 2,2 kΩ **DNP** | 0402 | Pull-ups SDA/SCL côté 1,8 V, pour un repli du lidar en I²C |
| C9 à C13 | 100 nF | 0402 | Découplage côté 3,3 V du P3A9606 et des SN74LVC1T45 |
| C14 à C17 | 100 nF | 0402 | Découplage côté 1,8 V des translateurs |
| C18, C19 | 22 µF | 0805 | Réservoir de **VBAT_LDD** (laser du lidar, pics ~0,75 A) |
| C20 | 100 nF | 0402 | Filtrage haute fréquence de VBAT_LDD |
| C21, C22 | 10 µF + 100 nF | 0603 / 0402 | Découplage de **VBAT_RX** (récepteur du lidar) |
| C23, C24 | 4,7 µF + 100 nF | 0603 / 0402 | Découplage de **AVDD** (analogique du lidar, 3,3 V) |
| C25, C26 | 4,7 µF + 100 nF | 0603 / 0402 | Découplage de **DVDD** (numérique du lidar, 1,2 V) |
| C27, C28 | 1 µF + 100 nF | 0402 | Découplage de **IOVDD** (E/S du lidar, 1,8 V) et de l'oscillateur |

## Page 5 : IMU

| Repère | Composant | Boîtier | Utilité |
|---|---|---|---|
| U9 | ST **LSM6DSV16X**, centrale inertielle 6 axes | LGA-14 | Accéléromètre + gyroscope : **orientation et mouvement** du scanner, pour recaler les scans entre eux |
| R13, R14 | 2,2 kΩ **DNP** | 0402 | Pull-ups SDA/SCL. Non montées car le kit en a déjà (1,5 kΩ sur I2C1). |
| C29 | 100 nF | 0402 | Découplage de VDD (alimentation des capteurs) |
| C30 | 100 nF | 0402 | Découplage de VDD_IO (E/S) |
| C31 | 10 µF | 0603 | Réservoir local du +3V3 de l'IMU |

## Page 6 : Wi-Fi / BLE

| Repère | Composant | Boîtier | Utilité |
|---|---|---|---|
| IC1 | ST **ST67W611M1A6B**, module Wi-Fi 6 / BLE 5.4 | Module LGA, antenne PCB | Envoie les données traitées au PC (Wi-Fi) et reçoit des commandes (BLE). Relié au N6 en **SPI5**. |
| J5 | Header 1×4 | 2,54 mm | Connecteur de service : GND, TX, RX, BOOT. Sert à mettre à jour le firmware du module avec un adaptateur USB-série. |
| R15 | 100 kΩ | 0402 | Tire CHIP_EN à 0 : module éteint tant que le N6 ne l'active pas (D7) |
| R16 | 10 kΩ | 0402 | Tire BOOT à 0 : démarrage normal. On le met à 3,3 V par J5 pour la mise à jour. |
| R17, R18 | 0 Ω | 0402 | Relient l'UART du module à D0/D1 du kit. À retirer si un adaptateur est branché sur J5. |
| C32 | 10 µF | 0603 | Réservoir du +3V3 du module (pics à l'émission) |
| C33 à C35 | 100 nF | 0402 | Découplage de VDD33 et des deux VDDIO du module |

## Page 7 : Mécanique

| Repère | Composant | Boîtier | Utilité |
|---|---|---|---|
| H1, H2 | Trous de fixation M2 | Ø 2,2 mm | Fixent le **module caméra** à côté du lidar, pour que les deux regardent dans la même direction. Entraxe à mesurer. |

## Totaux

| Catégorie | Quantité |
|---|---|
| Circuits intégrés et modules | 9 (3 LDO, lidar, P3A9606, 3 SN74LVC1T45, IMU) + module Wi-Fi |
| Oscillateur | 1 |
| Condensateurs | 35 |
| Résistances | 18 (dont 6 DNP) |
| LED | 1 |
| Connecteurs | 5 (4 barrettes Arduino + J5) |
| Points de test | 6 |
| Trous de fixation | 2 |
| **Total** | **78** empreintes, dont 6 DNP (soit 64 composants à souder, hors points de test et trous) |
