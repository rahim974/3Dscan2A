# Handoff P05 — Détection d'obstacle et traitements LiDAR

## Baseline et règles

- Dépôt `rahim974/3Dscan2A`, branche exclusive `Note-de-Seb`. Ne jamais modifier `main`.
- Vérifier le HEAD distant et local de clôture P04 avant toute modification (ce document ne peut contenir son propre SHA).
- Dépôt local `D:\Projet_2A`, workspace `D:\STM32_Workspace_Projet2A`, Windows 11, PowerShell, STM32CubeIDE, STM32CubeN6 v1.2.0. Une carte STM32N6570-DK utilisée pour les tests courants, IMX335.
- Lire roadmap, journal permanent, `docs/tests/LIDAR_SIMULATION_VALIDATION_2026-11-24.md`, les sources réelles et `Protocoles testes et simulations/simulation_lidar.html`. Les documents historiques ne prévalent pas sur les sources.

## État validé P04

Simulateur original lié au projet caméra ; source originale non modifiée. 54 × 42, uint16_t distances mm, invalide 0xFFFF, 2268 zones, LidarFrame 15880 octets. Six cas : min/max/moyenne, valides/invalides, dimensions, timestamp et empreintes conformes (mask 63, failures 0). 151 frames caméra produites/lues en 5000 ms = 30,20 FPS ; LCD confirmé, ErrorCode 0 au relevé. Placement frame 0x341C06A0, snapshot réservé à 0x342E0000 ; aucune collision constatée. Voir rapport et quatre SVG P04.

`P03_SNAPSHOT_TEST_ENABLED=0` : test automatique P03 désactivé, API et buffer conservés. Diagnostic P04 exécute six cas puis s'arrête ; ses résultats restent visibles. Décider explicitement de conserver, isoler ou remplacer le diagnostic sans créer de consommateur snapshot concurrent.

## Mission P05 (prévue, non commencée)

Auditer puis isoler les types et traitements communs dans `lidar_types` et `lidar_processing` selon les sources et la roadmap ; concevoir la détection d'obstacle et ses références théoriques avant l'implémentation. Vérifier seuil, convention d'indexation, centroïde, valeurs invalides et cas sans obstacle sur STM32 ; conserver les six références P04 pour les tests de non-régression. Les valeurs du protocole HTML doivent être recalculées depuis les sources. Ne pas anticiper l'interface LidarSource P06, le capteur réel ou la fusion RGB-D.

Journaliser chaque jalon, distinguer implémenté/compilé/testé/validé, conserver les réglages matériels, exclure `language.settings.xml` si seulement env-hash. Ajouter un rapport et des schémas SVG adaptés, puis commit/push exclusifs et contrôle direct GitHub avant P06.
