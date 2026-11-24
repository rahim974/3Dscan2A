# Test Wi-Fi X-NUCLEO-67W61M1 sur STM32N6570-DK : rapport de séance du 24/11/2026

## Résumé

- **Le Wi-Fi du DK fonctionne.** Avec le CLI ST porté au DK, toutes les commandes passent : `info` (module en firmware 2.0.89), `wifi_scan`, connexion WPA2 et DHCP, ping, écho UDP, MQTT. Le débit iperf TCP atteint 8,3 Mbit/s, stable sur 60 s (59,5 Mo).
- **Écran du DK.** Il affiche une carte de profondeur simulée et un panneau d'état superposé : Wi-Fi, IP, RSSI, trames, débit, erreurs. Le module `lcd_status` est réutilisable pour le scanner.
- **Démo scanner 3D.** Le DK envoie en TCP des trames lidar simulées vers le PC (54×42 zones, 9 Ko, ~30 trames/s, ~2,2 Mbit/s). Le flux a tenu **333 s sans erreur** (10 225 trames, 93 Mo). Deux corrections ont été nécessaires :
  1. **blocs de 1 520 octets** par `W6X_Net_Send` : avec des appels plus gros, des trames entières arrivaient à zéro ;
  2. **envoi continu** : avec des pauses de 33 ms entre trames, le module décrochait en 3 à 25 s.
- **Batterie externe.** La v6 a envoyé 251 trames en 10 s sans câble vers le Mac. La v7 (envoi continu) est flashée mais pas encore testée sur batterie.
- Le firmware du module Wi-Fi n'a pas été modifié, et sa mise à jour ne semble pas nécessaire.

## Contenu du dossier

| Fichier | Contenu |
|---|---|
| `RESULTATS_test_wifi_DK.md` | Rapport détaillé : procédure, sorties des tests, tableau de tous les essais de la démo, causes, hypothèses éliminées, à faire |
| `port_STM32N6570-DK.py` | Portage du projet ST `ST67W6X_CLI` (NUCLEO-N657X0-Q) au DK, différences de broches documentées |
| `ST67W6X_CLI_DK.patch` | Différences du projet porté par rapport à x-cube-st67w61 v1.3.0 |
| `demo_scan3d/` | Démo autonome : firmware (`scan3d_demo.c`, module écran `lcd_status.c/.h`), script de création du projet, récepteur PC, README (mémoire, broches de l'écran, conflits avec la carte A, commandes console, suite caméra) |

## Rappels pratiques

- **Flasher.** BOOT1 (SW1, en bas) côté H, puis débrancher et rebrancher l'USB : un simple RESET ne suffit pas. Flasher à `0x70000000` avec `MX66UW1G45G_STM32N6570-DK.stldr`. Remettre ensuite BOOT1 côté L, puis débrancher et rebrancher.
- **Module bloqué.** Un RESET du STM32 ne le relance pas : il faut couper l'alimentation ou passer CHIP_EN à 0 (la démo le fait automatiquement).
- **Réseau.** Le SSID, le mot de passe et l'IP du PC sont compilés depuis `wifi_secrets.h`, qui n'est pas versionné. Si l'IP du PC change, il faut recompiler.

## À faire

1. Tester la v7 sur batterie, avec le Mac connecté au partage « Scan3d ».
2. Ajouter la découverte automatique du PC, pour ne plus compiler son IP.
3. Essayer `powersave 0` avec un envoi cadencé, pour confirmer la cause du décrochage.
