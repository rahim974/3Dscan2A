# Carte fille du prototype A : composants spécifiques à commander

Projet ENSEA 2A, scanner 3D portable. Carte fille enfichée sur le kit ST STM32N6570-DK.

| Composant | Référence fabricant | Fabricant | Boîtier | Qté carte | Qté à commander | Rôle | Lien |
|---|---|---|---|---|---|---|---|
| Lidar 3D à temps de vol | **VL53L9CXV0VE/1** (référence commandable du VL53L9CX) | STMicroelectronics | Module LGA 12,83 × 6,10 mm | 1 | 1 | Mesure de profondeur (54 × 42 zones, jusqu'à 8,8 m) | [Lien](https://mou.sr/4AeKF3e) · |
| Module Wi-Fi 6 / Bluetooth LE 5.4 | **ST67W611M1A6B** (version antenne PCB) | STMicroelectronics | Module LGA 12,28 × 17,28 mm | 1 | 1 | Envoi des données au PC | [Lien](https://mou.sr/47TdNAW) |
| Centrale inertielle 6 axes | **LSM6DSV16X** | STMicroelectronics | LGA-14, 2,5 × 3 mm | 1 | 2 | Orientation et mouvement du scanner | [Lien](https://mou.sr/4sGqo40) |
| Translateur de niveau I2C/I3C 2 bits | **TCA39306DCUR** (remplace le P3A9606, limité à 1,98 V) | Texas Instruments | VSSOP-8 (DCU), pas de 0,5 mm | 1 | 2 | Bus I3C 3,3 V ↔ 1,8 V entre le kit et le lidar | [DigiKey](https://www.digikey.fr/short/9r21cj7m) |
| Translateur de niveau 1 bit | **SN74LVC1T45DBVR** | Texas Instruments | SOT-23-6 | 3 | 4 | Signaux XSHUT, SYNC et INTR du lidar | [Lien](https://mou.sr/4AeMYTW) |
| Régulateur LDO 3,3 V 1,5 A | **LD39150DT33-R** | STMicroelectronics | DPAK (TO-252) | 1 | 1 | Rail 3,3 V à partir du 5 V du kit | [Lien](https://mou.sr/4jSOFS0) |
| Régulateur LDO 1,8 V 300 mA | **LDK130M18R** | STMicroelectronics | SOT-23-5 | 1 | 2 | Rail 1,8 V (entrées-sorties du lidar) | [Lien](https://mou.sr/4kuVdq7) |
| Régulateur LDO 1,2 V 300 mA | **LDK130M12R** | STMicroelectronics | SOT-23-5 | 1 | 2 | Rail 1,2 V (cœur du lidar) | [Lien](https://mou.sr/4ixnLwD) |
| Oscillateur 24 MHz | **ECS-2520MV-240-BN-TR** (alimentation 1,6 à 3,6 V : fonctionne en 1,8 V) | ECS Inc. | 2,5 × 2,0 mm | 1 | 2 | Horloge du lidar | [Lien](https://mou.sr/4zsxtry) |

