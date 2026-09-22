# Nomenclature Détaillée - Scanner 3D Sémantique Manuel

Ce document liste l'ensemble des composants nécessaires au projet en distinguant ce qui est déjà fourni par la carte de développement (le Kit) et ce qui doit être routé sur votre carte fille (le PCB intermédiaire sous KiCad).

---

## 1. Composants DÉJÀ DISPONIBLES sur le Kit (STM32N6570-DK)
*Il n'y a pas besoin de router ces composants, la carte mère s'en charge.*

*   **Microcontrôleur (MCU) :** Puce STM32N657X0 comprenant un cœur Arm Cortex-M55 et un accélérateur IA Neural-ART[span_0](start_span)[span_0](end_span).
*   **Mémoires embarquées :** Environ 4,2 Mo de SRAM interne, mémoire Flash externe Octo-SPI de grande capacité, et de la PSRAM externe[span_1](start_span)[span_1](end_span).
*   **Capteur Visuel (Caméra RGB) :** Module caméra connecté sur l'interface MIPI CSI-2[span_2](start_span)[span_2](end_span).
*   **Stockage et Interface :** Prise en charge Ethernet, microSD, écran et connecteurs d'extension[span_3](start_span)[span_3](end_span).
*   **Connectivité d'extension :** Connecteurs femelles exposant l'alimentation (5V, 3.3V) et les bus de communication (I2C, SPI, GPIOs).

---

## 2. Composants À INTÉGRER sur le PCB Intermédiaire (Shield KiCad)
*C'est le circuit imprimé que vous devez concevoir et fabriquer.*

### 2.1 Capteurs (Perception et Mouvement)
*   **Capteur LiDAR 3D :** 1x VL53L9CX (STMicroelectronics) - À placer en façade pour mesurer la géométrie[span_4](start_span)[span_4](end_span).
*   **Centrale Inertielle (IMU) :** 1x LSM6DSV16X (STMicroelectronics) - À placer au centre de la carte pour mesurer l'orientation et l'inclinaison du système lors du mouvement manuel[span_5](start_span)[span_5](end_span).

### 2.2 Gestion de l'alimentation (Régulation locale)
*   **Régulateur LDO 3.3V :** 1x AP2112K-3.3 (ou TLV70033) - Pour filtrer l'alimentation du MCU vers les capteurs.
*   **Régulateur LDO 1.8V :** 1x AP2112K-1.8 (ou TLV70018) - Pour la logique interne du capteur LiDAR (IOVDD/AVDD).

### 2.3 Composants Passifs (CMS 0402 ou 0603)
*   **Condensateurs de découplage (Filtrage) :**
    *   ~4 à 6x Condensateurs 100 nF (Céramique, X7R) - À placer au plus près de chaque broche d'alimentation (VDD, IOVDD) des puces VL53L9CX, LSM6DSV16X et des LDO.
    *   ~2 à 4x Condensateurs 1 µF (Céramique, X7R) - Pour stabiliser la sortie et l'entrée des deux régulateurs LDO.
*   **Résistances (Pull-up et Configuration) :**
    *   2x Résistances 2.2 kΩ à 4.7 kΩ - Pull-up pour les lignes du bus I2C (SDA et SCL).
    *   ~3 à 4x Résistances 10 kΩ - Pull-up ou Pull-down pour configurer les adresses I2C (ex: broche SA0 de l'IMU) et stabiliser les broches d'activation (XSHUT) et d'interruption (INT).

### 2.4 Connectique et Mécanique
*   **Connecteurs vers la carte mère :** Barrettes de connexion mâles (Pin Headers, pas de 2.54 mm). À souder sous le PCB pour s'enficher dans les embases du kit STM32N6570-DK.
*   **Fixation mécanique :** 4x Trous de montage (diamètre 2.5 mm ou 3 mm) aux coins du PCB pour visser des entretoises rigides entre le shield et la carte mère (indispensable pour que le balayage manuel ne fausse pas l'IMU).