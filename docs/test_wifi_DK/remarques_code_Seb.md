# Remarques sur le code caméra et lidar de Seb, vu depuis le Wi-Fi

Lecture de la branche `Note-de-Seb` du dépôt `rahim974/3Dscan2A`. Dernier commit lu : « P06: add interchangeable LiDAR source and validate continuous acquisition on STM32 », du 24/11/2026. Objectif : voir comment ce code pourra cohabiter avec le module Wi-Fi ST67W611M1 testé dans `test_wifi_DK/`.

## Ce que contient la branche

Deux firmwares pour le STM32N6570-DK :

- `firmware/camera_imx335/` : caméra IMX335 ;
- `firmware/lidar_sim/` : lidar simulé.

**Aucun système d'exploitation.** Aucun appel FreeRTOS dans les sources du projet, seulement dans les pilotes de ST.

- Le programme est une boucle `while (1)` dans `main.c`, qui appelle à tour de rôle `CameraSource_Process()`, `LidarSource_Process()`, les tests et l'affichage.
- Seules les interruptions de la caméra sont utilisées : le callback de fin d'image DCMIPP appelle `CameraSource_OnFrameEvent()`.
- Le temps vient de `HAL_GetTick()`. La cadence du lidar (100 ms) est tenue sans `HAL_Delay`.

| Élément | Ce que disent le code et les rapports de Seb |
|---|---|
| Modules | `camera_source.h/.c` (IMX335, DCMIPP, ISP ; `Process`, `GetLatestFrame`, instantané à la demande) et `lidar_source.h/.c` (interface indépendante du capteur : `Init`, `Process`, `AcquireFrame`, `ReleaseFrame`) |
| Règle d'usage | « Consommateur unique, boucle principale uniquement » ; jamais de `LidarSource` depuis une interruption ; pas d'allocation dynamique |
| Image caméra | RGB565 800 × 480 (768 000 o) à `0x34200000`, environ 30 images/s ; instantané figé dans une zone `SNAPSHOT` à `0x342E0000` (896 Ko réservés dans le `.ld`) |
| Mémoire du programme | `RAM` de 256 Ko à `0x341C0000` (`STM32N657X0HXQ_AXISRAM2_fsbl.ld`) |
| Trame lidar | 54 × 42 zones à 7 octets (distance, amplitude, ambiant sur 2 octets chacun, plus 1 octet de qualité), 15 880 o avec l'horodatage, `0xFFFF` = zone invalide, un seul tampon |
| Lidar | Simulé uniquement (10 images/s). Le VL53L9CX réel est l'étape P07, pas encore commencée (bus I3C envisagé, non confirmé) |
| Suite prévue | P08 : synchronisation caméra et lidar ; P09 à P11 : nuage de points, poses, fusion RGB-D |

## Conséquences pour le Wi-Fi

1. **FreeRTOS est obligatoire.** Le pilote Wi-Fi de ST (`ST67W6X_Network_Driver`) ne fonctionne qu'avec FreeRTOS : tâches SPI et réception du module à haute priorité, sémaphores et délais maximum. Le code de Seb tourne en boucle simple (« bare-metal »). Pour le prototype A, il faudra passer caméra, lidar, fusion et envoi en tâches FreeRTOS. Sa règle « consommateur unique » facilite ce passage : chaque capteur devient une tâche, avec un tampon protégé ou une file.

2. **Mémoire à replanifier.**
   - Le projet de Seb réserve 256 Ko pour le code et les données à partir de `0x341C0000`.
   - Le projet Wi-Fi de ST utilise 1,5 Mo à partir de `0x34000000`, dont un tas FreeRTOS de 200 000 octets.
   - Il faut un plan mémoire commun : réduire le tas, déplacer des tampons ou utiliser une autre zone.

3. **Conflit d'adresses avec la démo Wi-Fi.**
   - La couche d'état de l'écran de la démo est à `0x342C0000`, sur 768 000 o, jusqu'à `0x3437B800`.
   - L'instantané de Seb commence à `0x342E0000`. Les deux zones se chevauchent : il faut en déplacer une avant de les faire cohabiter.
   - Le fond de la démo (`0x34200000`) coïncide avec l'image caméra de Seb. C'est voulu : ce fond était prévu pour la vue caméra.

4. **Débit nécessaire.**
   - Avec le format de Seb (15 880 o à 10 trames/s), le lidar produit environ 159 Ko/s, soit 1,3 Mbit/s. C'est moins que les 2,2 Mbit/s simulés dans la démo et largement sous les 8,3 Mbit/s mesurés.
   - Une image caméra complète (768 000 o) prend environ 0,75 s : possible à la demande, mais pas en flux vidéo continu.

5. **Règles Wi-Fi à appliquer au futur code.**
   - Envoyer en flux continu, par blocs de 1 520 octets maximum : sinon, décrochages et trames à zéro (voir `RESULTATS_test_wifi_DK.md`).
   - Le plus simple : la fusion met les scans dans une file, et la tâche d'envoi la vide sans pause.
   - Garder les tâches du pilote Wi-Fi au-dessus de celles des capteurs.
   - Le callback caméra devra seulement réveiller la tâche caméra. Ses priorités d'interruption doivent être compatibles avec `configMAX_SYSCALL_INTERRUPT_PRIORITY`.
   - Remplacer les attentes par `HAL_GetTick()` par `vTaskDelayUntil`.

6. **Horloges.** Le projet caméra ne configure pas la PLL4. La démo Wi-Fi règle l'horloge de l'écran sur PLL4 / 64. À vérifier si l'on ajoute la couche d'état de la démo au projet caméra.
