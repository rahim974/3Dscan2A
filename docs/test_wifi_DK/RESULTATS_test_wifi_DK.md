# Test Wi-Fi X-NUCLEO-67W61M1 sur STM32N6570-DK (24/11/2026)

## Logiciel

- Pack ST **x-cube-st67w61 v1.3.0** (commit `572b72d`), projet `ST67W6X_CLI` de la NUCLEO-N657X0-Q, porté au DK avec `port_STM32N6570-DK.py` :
  - MISO : PG1 → PH8 ;
  - BOOT : PD5 → PE13 ;
  - bouton : PD12 → PE7 (EXTI7).
- Différences complètes : `ST67W6X_CLI_DK.patch` (diff par rapport à `ST67W6X_CLI` v1.3.0).

## Compilation, signature, flash

- Compilation : STM32CubeIDE 2.2.0 en ligne de commande, 0 erreur.
- Signature : `STM32_SigningTool_CLI -nk -of 0x80000000 -t fsbl -hv 2.3 --align`.
- Flash : à `0x70000000`, avec le loader `MX66UW1G45G_STM32N6570-DK.stldr`.
- Interrupteurs : BOOT1 côté H pour flasher, puis BOOT0 et BOOT1 côté L pour démarrer.
- **Piège** : après le passage de BOOT1 sur H, un Reset ne suffit pas. Il faut débrancher puis rebrancher l'USB. Sinon : `DEV_AP_ACCESS_ERROR`, puis `failed to erase memory`.

## Résultats (console 921600 bauds)

- `info` OK :
  - module en firmware 2.0.89 (ST demande 2.0.106 pour ce projet) ;
  - aucun défaut constaté ;
  - firmware du module non modifié.
- `wifi_scan` : 20 réseaux.
- Connexion WPA2 2,4 GHz + DHCP OK.
- `ping google.com` : 4/4.
- Écho UDP OK.
- MQTT OK (`mqtt_configure -s 0`).
- Débit DK → Mac :
  - TCP 8,6 Mbit/s ;
  - UDP 8,7 Mbit/s avec environ 3 % de perte ;
  - plafond probablement dû au SPI ou au module (hypothèse non vérifiée).
- Mac → DK : TCP et ping échouent sur le partage (isolation probable), l'UDP passe.

## Conclusion

- Flux lidar estimé à environ 270 Ko/s (2,2 Mbit/s).
- Critère « Wi-Fi ≥ 2× le flux lidar » rempli (environ 3,9×).
- Flux réel à mesurer au test 3.

## Carte A

- SPI_RDY sur D2 (PD0, EXTI0) au lieu de D3.
- CHIP_EN sur D7 (PD6) au lieu de D5.
- BOOT non relié au N6.

## Démo autonome « Scanner 3D » (écran + envoi TCP continu), 24/11/2026

Code dans `demo_scan3d/` (README). Réseau « Scan3d » (partage 2,4 GHz), PC récepteur sur le même partage.

**Ce qui marche**
- L'écran affiche le titre, le panneau d'état (Wi-Fi, IP, RSSI, lien, trames, débit, erreurs) et la carte de profondeur simulée.
  - Correctif nécessaire : le RIF autorise LTDC1 **et** LTDC2 (un maître AXI par couche). Sans LTDC2, la couche de texte est transparente.
- Le DK se connecte seul au Wi-Fi puis au PC et envoie 20 à 30 trames/s (54×42 zones, 9 092 octets), soit environ 1,5 à 2 Mbit/s.
- Réparation automatique : quand le module ne répond plus 3 fois de suite, la carte met CHIP_EN bas 200 ms, redémarre, puis le flux reprend sans intervention.

**Ce qui ne marche pas encore** : le module ST67W611M1 (firmware 2.0.89) décroche après quelques secondes de flux. On voit « Failed to send command », puis des TIMEOUT sur toutes les commandes AT, puis plus de socket libre. Un RESET du STM32 ne remet pas le module à zéro : il faut couper l'alimentation, ou CHIP_EN.

| Essai | Taille par `W6X_Net_Send` | Écran | Trames entièrement nulles | Décrochage après |
|---|---|---|---|---|
| v1-v4 | 9 092 o | oui | 6 à 28 par essai | 3 à 8 s |
| sans écran (LTDC non initialisé) | 9 092 o | non | 6 | ~6 s |
| données nulles (comme iperf) | 9 092 o | non | 9 | ~6 s |
| blocs 1 520 o | 1 520 o | non | **0** | ~25 s |
| v5 (blocs 1 520 o) | 1 520 o | oui | 0 | 3 à 10 s (puis auto-réparation) |
| iperf ST (`iperf -c … -t 60`) | 1 520 o, en continu | non | données iperf nulles | **aucun** : 59,5 Mo, 8,3 Mbit/s en 60 s |
| v6 (tâche réseau en priorité 40, comme iperf) | 1 520 o, 30 trames/s avec pauses | oui | 0 | ~5 s : la priorité ne change rien |
| **v6, envoi continu** (`s3d_cadence 1`) | 1 520 o, sans pause | non, puis oui | 2 recalages | **aucun en 333 s** : 10 225 trames, 93 Mo, 2,23 Mbit/s |

**Conclusions**
- Les trames nulles venaient des appels `W6X_Net_Send` de plus de 1 520 octets (`W61_MAX_SPI_XFER`). Elles disparaissent avec des blocs de 1 520 octets.
- Le décrochage du module vient des **pauses entre les rafales d'envoi** (une trame toutes les 33 ms). En envoi continu, comme iperf, le flux a tenu 333 s (5,5 min) sans aucune erreur, écran allumé la dernière minute. L'arrêt est venu du débranchement de l'USB, pas d'un décrochage.
- Hypothèse non vérifiée : l'économie d'énergie Wi-Fi du module se déclenche pendant les pauses et se réveille mal (firmware 2.0.89). Essai à faire : `powersave 0` avec des envois cadencés.
- Hypothèses éliminées : débit, réseau, Mac, contenu des données, lecture du RSSI, écran, priorité des tâches, mémoire et piles du STM32 (tas libre 160 Ko, piles < 15 % utilisées).
- Version retenue (v7) : envoi continu par défaut, blocs de 1 520 octets, relance immédiate du module (CHIP_EN) en cas d'échec.
- Pour le scanner : garder le lien occupé en continu, ou couper l'économie d'énergie si l'essai le confirme. La mise à jour du firmware du module en 2.0.106 n'est plus indispensable ; c'est une opération irréversible, à faire seulement avec l'accord de l'équipe.
