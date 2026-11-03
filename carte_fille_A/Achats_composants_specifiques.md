# Carte fille du prototype A : composants spécifiques à commander

Projet ENSEA 2A, scanner 3D portable. Carte fille enfichée sur le kit ST STM32N6570-DK (déjà disponible).

Cette liste ne contient que les composants qu'on ne trouvera probablement pas au magasin de l'école. Les références sont celles des fabricants : elles sont à rechercher chez un distributeur (Mouser, DigiKey, Farnell) ou sur le site du fabricant. Je propose une pièce de rechange pour les petits boîtiers difficiles à souder.

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

## Passifs et connectique (à commander si l'école n'en a pas)

| Composant | Valeur / référence | Boîtier | Qté carte |
|---|---|---|---|
| Condensateurs céramiques X5R/X7R | 100 nF (×20), 1 µF (×5), 4,7 µF (×2), 10 µF (×4), 22 µF (×4) | 0402, 0603 et 0805 | 35 |
| Résistances 1 % | 10 kΩ (×5), 100 kΩ, 1 kΩ, 33 Ω, 0 Ω (×2) | 0402 | 10 |
| LED verte | — | 0603 | 1 |
| Barrettes mâles 2,54 mm (pour Arduino) | 1×10, 1×8 (×2), 1×6 | Traversant | 4 |
| Barrette mâle 2,54 mm | 1×4 | Traversant | 1 |
| Vis et entretoises M2 | Fixation du module caméra | — | 2 |

Les passifs sont en 0402 pour gagner de la place. Si l'école n'a que du 0603, on peut adapter les empreintes.

## Points à signaler

- **Soudure** : le lidar, le module Wi-Fi et l'IMU sont en boîtier LGA (pastilles sous le composant). Ils se soudent au **four de refusion ou à la plaque chauffante, avec un pochoir de pâte à braser**, et pas au fer. Le P3A9606 (pas de 0,35 mm) demande la même méthode.
- **PCB** : 2 couches, 68,6 × 53,3 mm (format shield Arduino Uno), isolation minimale 0,127 mm à cause des pas fins.
- **Alimentation pour l'utilisation portable** : une batterie externe USB-C 5 V / 3 A pour alimenter le kit sans PC.
