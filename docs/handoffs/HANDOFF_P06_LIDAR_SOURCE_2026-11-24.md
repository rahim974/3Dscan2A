# Handoff P06 — LidarSource interchangeable

## Baseline et règles

- Dépôt `rahim974/3Dscan2A`, branche exclusive `Note-de-Seb`. Ne jamais modifier `main`.
- Vérifier HEAD distant et local de clôture P05 avant toute modification ; ce document ne peut contenir son propre SHA.
- Windows 11, PowerShell, STM32CubeIDE, STM32CubeN6 v1.2.0 ; dépôt `D:\Projet_2A`, workspace `D:\STM32_Workspace_Projet2A` ; STM32N6570-DK + IMX335, une carte pour les validations courantes.
- Lire roadmap, journal, `docs/tests/LIDAR_PROCESSING_VALIDATION_2026-11-24.md`, rapport P04, sources réelles des modules et des deux firmwares, configurations CubeIDE, protocole HTML historique.

## État P05

Types et traitements séparés ; wrapper historique préservé. 19/19 essais de détection (mask 524287), 6/6 P04 (mask 63), 6/6 compatibilité (mask 63), aucune défaillance. Caméra 151 frames produites/lues en 5000 ms, 30,20 FPS ; LCD confirmé ; ErrorCode=0 au relevé. Debug caméra 0 erreur/0 avertissement ; Debug LiDAR historique 0 erreur/1 avertissement ancien (variable `test` inutilisée), non flashé. Frame unique 15880 o, snapshot réservé, test P03 désactivé. Voir rapport pour mémoire et conventions. Diagnostic P04 étendu P05, terminé automatiquement. Pas de débit continu ni de capteur réel testé.

## Mission P06 (prévue, non commencée)

- Auditer et définir une interface `LidarSource` indépendante de la provenance, avec backend simulation et conventions de propriété, timestamp, disponibilité et erreurs explicites.
- Conserver les traitements indépendants de la source ; préserver les API historiques et les deux builds.
- Définir cadence et contrôle de non-régression ; décider explicitement du devenir du diagnostic P04/P05 ; aucun consommateur snapshot concurrent.
- Tester les API et le backend sur STM32, préserver caméra/LCD, documenter cadence et limites ; ne pas anticiper P07 ou la fusion RGB-D.
- Journaliser chaque jalon, distinguer implémenté/compilé/testé/validé ; rapport, SVG, commit/push exclusifs et vérification directe GitHub avant P07.
