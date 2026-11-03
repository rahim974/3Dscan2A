# Carte fille du prototype A : composants spécifiques à commander

Projet ENSEA 2A, scanner 3D portable. Carte fille enfichée sur le kit ST STM32N6570-DK.

## Circuits intégrés et modules

| Composant | Référence fabricant | Fabricant | Boîtier | Qté carte | Qté à commander | Rôle |
|---|---|---|---|---|---|---|
| Lidar 3D à temps de vol | **VL53L9CX** | STMicroelectronics | Module LGA 12,6 × 5,9 mm | 1 | 1 | Mesure de profondeur (54 × 42 zones, jusqu'à 8,8 m) |
| Module Wi-Fi 6 / Bluetooth LE 5.4 | **ST67W611M1A6B** (version antenne PCB) | STMicroelectronics | Module LGA 12,28 × 17,28 mm | 1 | 1 | Envoi des données au PC |
| Centrale inertielle 6 axes | **LSM6DSV16X** | STMicroelectronics | LGA-14, 2,5 × 3 mm | 1 | 2 | Orientation et mouvement du scanner |
| Translateur de niveau I3C 2 bits | **P3A9606** *(suffixe de commande et boîtier à confirmer dans le datasheet)* | NXP | XSON/X2SON-8 | 1 | 2 | Bus I3C 3,3 V ↔ 1,8 V entre le kit et le lidar |
| Translateur de niveau 1 bit | **SN74LVC1T45DBVR** | Texas Instruments | SOT-23-6 | 3 | 4 | Signaux XSHUT, SYNC et INTR du lidar |
| Régulateur LDO 3,3 V 1,5 A | **LD39150DT33-R** | STMicroelectronics | DPAK (TO-252) | 1 | 1 | Rail 3,3 V à partir du 5 V du kit |
| Régulateur LDO 1,8 V 300 mA | **LDK130M18R** | STMicroelectronics | SOT-23-5 | 1 | 2 | Rail 1,8 V (entrées-sorties du lidar) |
| Régulateur LDO 1,2 V 300 mA | **LDK130M12R** | STMicroelectronics | SOT-23-5 | 1 | 2 | Rail 1,2 V (cœur du lidar) |
| Oscillateur 24 MHz | **ECS-2520MV-240-BN-TR** *(vérifier qu'il fonctionne en 1,8 V)* | ECS Inc. | 2,5 × 2,0 mm | 1 | 2 | Horloge du lidar |

