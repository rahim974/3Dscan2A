# Tester la STEVAL-MKI227KA avec une NUCLEO-F410RB

## 1. Ce qu'il y a sur la carte

La **STEVAL-MKI227KA** est une carte adaptatrice DIL24 qui porte un **LSM6DSV16X** : une centrale inertielle 6 axes (accéléromètre et gyroscope) avec la détection électrostatique Qvar. Les électrodes Qvar fournies ne servent pas pour ce test.

| Paramètre | Valeur |
|---|---|
| WHO_AM_I (registre 0x0F) | **0x70** |
| Adresse I2C (7 bits) | **0x6A** si SDO/SA0 = GND, **0x6B** si SDO/SA0 = VDDIO |
| Alimentation | VDD 1,71 à 3,6 V, VDDIO 1,08 à 3,6 V : le 3,3 V de la Nucleo convient |
| Mode I2C | **CS doit être à VDDIO** (CS à 0 = mode SPI) |

Ressources officielles ST :
- Driver C bas niveau : https://github.com/STMicroelectronics/lsm6dsv16x-pid (inclus dans `STMems_Standard_C_drivers`, avec des exemples dans `lsm6dsv16x_STdC/examples`, comme `lsm6dsv16x_read_data_polling.c`)
- Pack CubeIDE : **X-CUBE-MEMS1** (pilote BSP du LSM6DSV16X et appli DataLogExtended pour MEMS Studio)
- Page de la carte : https://www.st.com/en/evaluation-tools/steval-mki227ka.html

Le code fourni ici n'utilise **pas** ces bibliothèques : il lit les registres directement avec la HAL, ce qui suffit pour valider la carte et évite d'importer quoi que ce soit.

## 2. Câblage (I2C)

Brochage DIL24 standard des cartes STEVAL-MKIxxx (vue de dessus, broche 1 en haut à gauche, broches 1 à 12 à gauche et 13 à 24 à droite en remontant) :

| Signal | Broche DIL24 | NUCLEO-F410RB |
|---|---|---|
| VDD | 1 | 3V3 (CN6) |
| VDDIO | 2 | 3V3 |
| GND | 13 | GND |
| SCL | 20 | **D15 = PB8** |
| SDA | 21 | **D14 = PB9** |
| SDO/SA0 | 22 | GND (adresse 0x6A) |
| CS | 23 | 3V3 (sélectionne l'I2C) |
| INT1 / INT2 | — | non utilisées |

> ⚠️ VDD (1), VDDIO (2) et GND (13) sont confirmées par le manuel de la carte mère ST (UM2116). Les broches 20 à 23 viennent du manuel de la X-NUCLEO-IKS5A1, qui utilise le même support DIL24. **Vérifie au multimètre avant de mettre sous tension**, en mode continuité entre la broche DIL24 et la puce LSM6DSV16X (boîtier LGA-14) : puce broche 13 = SCL, 14 = SDA, 1 = SDO/SA0, 12 = CS, 5 = VDDIO, 8 = VDD, 6 et 7 = GND.
>
> Si le scan I2C ne trouve rien, la première chose à essayer est d'**inverser SDA et SCL**.

**Pull-ups I2C** : la Nucleo n'en a pas sur D14/D15. Mets **2 résistances de 4,7 kΩ** de SDA et SCL vers 3V3. À défaut, active les pull-ups internes (voir plus bas) : ça fonctionne à 100 kHz avec des fils courts.

**Cavalier de la carte (SEL)** : laisse-le dans sa position d'origine. Il sert à router les électrodes Qvar ou le bus auxiliaire, pas le bus I2C principal.

## 3. Projet STM32CubeIDE (même procédure sur Mac et sur Windows)

> **Pas de projet « Empty »** : il ne contient ni la HAL ni la config I2C/UART. Dans **CubeIDE 2.x**, CubeMX n'est plus intégré : crée le projet dans l'application **STM32CubeMX** (Board Selector NUCLEO-F410RB, Yes aux périphériques par défaut, I2C1 sur PB8/PB9, Project Manager > Toolchain = STM32CubeIDE, Generate Code), puis dans CubeIDE **File > Import > Existing Projects into Workspace**. Ensuite, passe directement à l'étape 3.

1. (CubeIDE 1.x) **File > New > STM32 Project**, onglet **Board Selector**, choisis **NUCLEO-F410RB**, nomme le projet `test_lsm6dsv16x`, Targeted Project Type = **STM32Cube**, puis réponds **Yes** à « Initialize all peripherals with their default mode ? ». USART2 (PA2/PA3, relié au VCP du ST-LINK) est alors déjà configuré à 115200 8N1.
2. Dans le `.ioc` :
   - **Connectivity > I2C1** : mode `I2C`, vitesse Standard Mode 100 kHz.
   - Dans la vue des broches, clique sur **PB8** et choisis `I2C1_SCL`, puis sur **PB9** et choisis `I2C1_SDA`. CubeMX propose parfois PB6/PB7 par défaut : il faut bien PB8/PB9 pour D15/D14.
   - Sans pull-ups externes : **System Core > GPIO > I2C**, mets PB8 et PB9 en `Pull-up`.
   - Enregistre (Ctrl/Cmd+S) et accepte la génération du code.
3. Copie `lsm6dsv16x_test.c` dans `Core/Src/` et `lsm6dsv16x_test.h` dans `Core/Inc/`.
4. Dans `Core/Src/main.c` :

```c
/* USER CODE BEGIN Includes */
#include "lsm6dsv16x_test.h"
/* USER CODE END Includes */
```

```c
  /* USER CODE BEGIN 2 */
  LSM6_Test_Run(&hi2c1, &huart2);
  /* USER CODE END 2 */
```

5. **Project > Build Project** (marteau). Ça doit compiler sans erreur.

## 4. Flasher et lire la sortie

1. Branche la Nucleo en USB (connecteur du ST-LINK).
2. **Run > Run As > STM32 C/C++ Application**, puis OK dans la fenêtre de configuration (le débogueur ST-LINK GDB server par défaut convient).
3. Ouvre un terminal série à **115200 bauds, 8N1** :
   - Dans CubeIDE : vue **Console > Open Console > Command Shell Console > Serial Port**.
   - Mac : `screen /dev/tty.usbmodem* 115200` (pour quitter : Ctrl-A puis K).
   - Windows : PuTTY ou Tera Term sur le port « STMicroelectronics STLink Virtual COM Port (COMx) » qui apparaît dans le Gestionnaire de périphériques.
4. Appuie sur le bouton noir **RESET** pour revoir l'en-tête.

### Sortie attendue

```
=== Test STEVAL-MKI227KA (LSM6DSV16X) sur STM32F410 ===
Scan I2C...
  peripherique trouve a 0x6A
WHO_AM_I @0x6A = 0x70 (OK, LSM6DSV16X)
Config : XL ±2g 30Hz, G ±2000dps 30Hz. Unites : mg, mdps, centiemes de degC
   ax     ay     az   |      gx       gy       gz  |  temp
    12    -25   1003 |      280     -420      140 | 26.41
```

Carte posée à plat et immobile : **az ≈ +1000 mg** (1 g), ax et ay proches de 0, gyroscope à quelques centaines de mdps au plus (biais normal), température proche de l'ambiante. En retournant la carte, az passe à ≈ −1000 mg.

## 5. Dépannage

| Symptôme | Cause probable |
|---|---|
| Rien dans le terminal | Mauvais port COM ou mauvais débit, ou USART2 pas initialisé (le projet a-t-il été créé avec le Board Selector ?) |
| « AUCUN peripherique » | SDA/SCL inversés, pas de pull-ups, CS pas à 3V3, VDD/VDDIO non alimentés |
| Trouvé à 0x6B au lieu de 0x6A | SA0 en l'air ou à 3V3 : c'est normal, le code essaie les deux adresses |
| WHO_AM_I ≠ 0x70 | Ce n'est pas le bon capteur, ou une autre puce répond à cette adresse |
| Valeurs figées | Le registre STATUS ne signale aucune donnée prête : vérifie que CTRL1 et CTRL2 sont bien écrits (relance après un reset de l'alimentation) |

## 6. Pour aller plus loin

- **Driver officiel** : copie `lsm6dsv16x_reg.c/.h` du dépôt `lsm6dsv16x-pid` dans le projet et adapte l'exemple `lsm6dsv16x_read_data_polling.c` (il suffit d'écrire les fonctions `platform_write`, `platform_read` et `tx_com` avec `HAL_I2C_Mem_Write`, `HAL_I2C_Mem_Read` et `HAL_UART_Transmit`). Ça donne accès au FIFO, à la fusion de capteurs (SFLP) et au Qvar.
- **SPI** au lieu de l'I2C : CS (23) sur un GPIO de la Nucleo, SCL (20) = SPC, SDA (21) = SDI, SDO/SA0 (22) = SDO, avec SPI mode 3 et bit 7 de l'adresse de registre à 1 pour une lecture.
