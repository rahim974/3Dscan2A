# Carte fille du prototype A : composants spécifiques à commander

Projet ENSEA 2A, scanner 3D portable. Carte fille enfichée sur le kit ST STM32N6570-DK.

| Composant | Référence fabricant | Fabricant | Boîtier | Qté carte | Qté à commander | Prix unitaire | Total | Rôle | Lien |
|---|---|---|---|---|---|---|---|---|---|
| Lidar 3D à temps de vol | **VL53L9CXV0VE/1** (référence commandable du VL53L9CX) | STMicroelectronics | Module LGA 12,83 × 6,10 mm | 1 | 1 | 54,49 € | 54,49 € | Mesure de profondeur (54 × 42 zones, jusqu'à 8,8 m) | [Mouser](https://www.mouser.fr/ProductDetail/STMicroelectronics/VL53L9CXV0VE-1)|
| Module Wi-Fi 6 / Bluetooth LE 5.4 | **ST67W611M1A6B** (version antenne PCB) | STMicroelectronics | Module LGA 12,28 × 17,28 mm | 1 | 1 | 6,43 € | 6,43 € | Envoi des données au PC | [Mouser](https://www.mouser.fr/ProductDetail/STMicroelectronics/ST67W611M1A6BTR) |
| Centrale inertielle 6 axes | **LSM6DSV16X** | STMicroelectronics | LGA-14, 2,5 × 3 mm | 1 | 2 | 5,76 € | 11,52 € | Orientation et mouvement du scanner | [Mouser](https://www.mouser.fr/ProductDetail/STMicroelectronics/LSM6DSV16XTR) |
| Translateur de niveau I2C/I3C 2 bits | **TCA39306DCUR** (remplace le P3A9606, limité à 1,98 V) | Texas Instruments | VSSOP-8 (DCU), pas de 0,5 mm | 1 | 2 | 1,23 € (DigiKey) | 2,46 € | Bus I3C 3,3 V ↔ 1,8 V entre le kit et le lidar | [DigiKey](https://www.digikey.fr/fr/products/detail/texas-instruments/TCA39306DCUR/15222289) |
| Translateur de niveau 1 bit | **SN74LVC1T45DBVR** | Texas Instruments | SOT-23-6 | 3 | 4 | 0,76 € (rupture chez Mouser ; en stock chez DigiKey, ~0,62 $) | 3,03 € | Signaux XSHUT, SYNC et INTR du lidar | [DigiKey](https://www.digikey.fr/fr/products/detail/texas-instruments/SN74LVC1T45DBVR/639455) · [Mouser](https://www.mouser.fr/ProductDetail/Texas-Instruments/SN74LVC1T45DBVR) |
| Régulateur LDO 3,3 V 1,5 A | **LD39150DT33-R** | STMicroelectronics | DPAK (TO-252) | 1 | 1 | 1,21 € | 1,21 € | Rail 3,3 V à partir du 5 V du kit | [Mouser](https://www.mouser.fr/ProductDetail/STMicroelectronics/LD39150DT33-R) |
| Régulateur LDO 1,8 V 300 mA | **LDK130M18R** | STMicroelectronics | SOT-23-5 | 1 | 2 | 0,34 € | 0,67 € | Rail 1,8 V (entrées-sorties du lidar) | [Mouser](https://www.mouser.fr/ProductDetail/STMicroelectronics/LDK130M18R) |
| Régulateur LDO 1,2 V 300 mA | **LDK130M12R** | STMicroelectronics | SOT-23-5 | 1 | 2 | 0,34 € | 0,69 € | Rail 1,2 V (cœur du lidar) | [Mouser](https://www.mouser.fr/ProductDetail/STMicroelectronics/LDK130M12R) |
| Oscillateur 24 MHz | **ECS-2520MV-240-BN-TR** (alimentation 1,6 à 3,6 V : fonctionne en 1,8 V) | ECS Inc. | 2,5 × 2,0 mm | 1 | 2 | 1,00 € | 2,00 € | Horloge du lidar | [Mouser](https://www.mouser.fr/ProductDetail/ECS/ECS-2520MV-240-BN-TR) |

**Total circuits intégrés et modules : environ 82,50 € HT** (prix à l'unité relevés chez Mouser France le 09/11/2026, sauf mention contraire, hors frais de port). Le lidar représente à lui seul les deux tiers du budget.
