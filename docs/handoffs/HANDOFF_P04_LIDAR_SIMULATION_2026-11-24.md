# Handoff P04 — LiDAR simulé : statistiques et scènes 54 × 42

- Dépôt `rahim974/3Dscan2A`, branche exclusive `Note-de-Seb`. Ne jamais modifier `main`.
- Dépôt local `D:\Projet_2A`, workspace `D:\STM32_Workspace_Projet2A`, Windows 11 / PowerShell / STM32CubeIDE / STM32CubeN6 v1.2.0.
- Le SHA de clôture P03 doit être lu sur la branche distante et comparé au HEAD local avant toute modification. Ce document est inclus dans ce commit ; il ne peut contenir son propre SHA.
- Lire `docs/planning/ROADMAP_SCANNER3D_NOTE_DE_SEB.md`, `docs/SCANNER3D_PROGRESS.md`, `docs/tests/CAMERA_SNAPSHOT_VALIDATION_2026-11-24.md` et les sources réelles de simulation LiDAR dans la branche. Les anciens handoffs sont historiques.

## Baseline P03

Pipe1 continu, LCD opérationnel ; Pipe2 snapshot RGB565 800 × 480, 768000 octets à 0x342E0000, propriété unique via Request/Acquire/Release. 5 captures, 4 cycles CRC terminés sans échec, 5 refus pendant HELD, 4 variations inter-captures ; 151 frames en 5 s = 30.20 FPS ; dernière latence 49 ms. Deux incidents DPHY CTRL initiaux isolés et consignés (cause physique non déterminée). Voir rapport et schémas. Test automatique caméra laissé actif dans la baseline : décider explicitement de son isolation/désactivation si nécessaire, sans modifier le contrat CameraSource ni introduire un second consommateur concurrent.

## Mission P04

Auditer le simulateur existant et ses scènes 54 × 42 ; définir valeurs attendues et statistiques à partir des sources réelles, pas de valeurs inventées. Vérifier sur STM32 les données générées et les statistiques (périmètre précis à définir après lecture). Préserver la caméra et les paramètres matériels validés. Le VL53L9CX physique, la fusion RGB-D, LidarSource et les traitements de P05/P06 restent hors périmètre. Ajouter un schéma de fonctionnement versionné lorsque l'architecture évolue.

## Clôture

Consigner chaque étape dans le journal, distinguer prévu/implémenté/compilé/testé/validé, rapport de validation réel, roadmap, commit/push uniquement Note-de-Seb puis contrôle distant. Exclure language.settings.xml si seul env-hash a changé. Préparer handoff P05. Ne pas démarrer P05 avant clôture de P04.
