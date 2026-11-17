# Scanner 3D ENSEA — Journal permanent d'avancement

**Branche exclusive :** `Note-de-Seb`  
**Planning :** [`docs/planning/ROADMAP_SCANNER3D_NOTE_DE_SEB.md`](planning/ROADMAP_SCANNER3D_NOTE_DE_SEB.md)

> Ce document est **cumulatif** : ajouter une entrée datée après chaque jalon significatif, y compris échec ou décision. Ne jamais remplacer des observations antérieures par des résultats attendus. Vérifier les nouvelles versions de la branche avant de commencer chaque passe.

## Synthèse au 17 novembre 2026

| Passe | État attesté | Source |
|---|---|---|
| P00 — Acquisition caméra / CameraFrame dynamique | **Validée STM32** ; `frame_id`, `timestamp_ms`, ~30 FPS | `bda6a43` et `docs/tests/CAMERA_DYNAMIC_FRAME_VALIDATION_2026-11-17.md` |
| P01 — CameraSource minimal | **Prévue, non implémentée** | `docs/handoffs/HANDOFF_P01_CAMERA_SOURCE_2026-11-17.md` |
| P02–P03 — Refactoring et snapshot | Prévues, non validées | Roadmap |
| P04–P06 — Traitements et source LiDAR | Prévus ; baseline simulateur déjà testée historiquement sur STM32 | Handoff du 10/11 |
| P07 — VL53L9CX réel | Non validé, disponibilité matérielle à confirmer | Roadmap |
| P08–P11 — Appariement / XYZ / pose / carte 2D | Prévues, non validées | Roadmap |
| P12 — IA embarquée | Différée | Roadmap |

## Journal chronologique

### 2026-11-10 — Firmware de base et handoff historique

- **Composants :** IMX335 + simulation LiDAR 54 × 42 sur STM32N6570-DK.
- **Commits :** `b66709b` (`feat: add STM32 camera and LiDAR simulation baseline`) puis `e4be1bc` (`docs: add detailed AI project handoff`).
- **État matériel rapporté :** IMX335 détectée, CSI-2/DCMIPP/ISP fonctionnels, framebuffer `0x34200000` (800×480 RGB565, 768000 octets), environ 30 FPS ; simulateur LiDAR `CENTRAL_OBSTACLE` exécuté, mesures 4000 mm en `[0][0]` et 1200 mm en `[20][25]`.
- **Non validés :** VL53L9CX physique, détection obstacle complète, fusion et reconstruction.
- **Preuve / contexte :** `docs/handoffs/HANDOFF_SEB_2026-11-10.md`.

### 2026-11-17 — P00 : CameraFrame dynamique validée

- **Objectif :** mise à jour de `frame_id` et `timestamp_ms` lors du callback `HAL_DCMIPP_PIPE_FrameEventCallback`.
- **Source :** `firmware/camera_imx335/FSBL/Src/main.c`.
- **Implémentation :** `NbMainFrames++`, puis `g_camera_frame.frame_id = NbMainFrames;` et `g_camera_frame.timestamp_ms = HAL_GetTick();`.
- **Compilation :** projet `DCMIPP_ContinuousMode_FSBL` dans STM32CubeIDE lié à `D:\Projet_2A`, 0 erreur et 0 avertissement observés.
- **Tests réels STM32N6570-DK :** mesure 1 : compteur=197, id=197, timestamp=6786 ms, tick=6796 ms, FPS=31 ; mesure 2 : compteur=300, id=300, timestamp=10104 ms, tick=10131 ms, FPS=30.
- **Conclusion :** **implémenté, compilé, testé et validé sur STM32** pour les métadonnées dynamiques et la non-régression de cadence observée.
- **Limites :** timestamp de fin de frame ; stabilité des pixels d'un framebuffer partagé non assurée.
- **Rapport :** `docs/tests/CAMERA_DYNAMIC_FRAME_VALIDATION_2026-11-17.md`.
- **Commit + push :** `bda6a43` — `feat(camera): update frame metadata on DCMIPP frame events`. Branche locale alignée sur `origin/Note-de-Seb` et répertoire propre dans la sortie Git du 17/11.

### 2026-11-17 — Décision : séquencement par passes et chats distincts

- **Décision :** un chat par passe, une feuille de route stable, un **journal central continuellement actualisé**, et un handoff dédié à chaque changement de chat.
- **Nature :** organisation/documentation seulement ; **aucune implémentation ni validation STM32 supplémentaire**.
- **Base de vérification :** `Note-de-Seb` à `bda6a43`.
- **Fichiers préparés :** `docs/planning/ROADMAP_SCANNER3D_NOTE_DE_SEB.md`, ce journal et `docs/handoffs/HANDOFF_P01_CAMERA_SOURCE_2026-11-17.md`.
- **Prochain jalon :** P01 — interface minimale `CameraSource`, avec vérification de l'intégration au build CubeIDE.
- **Publication Git :** à compléter lorsque ces documents seront effectivement committés et poussés sur `Note-de-Seb`.

## Modèle obligatoire pour les prochaines entrées

### AAAA-MM-JJ — PXX : titre

- **Objectif / critères de réussite :**
- **HEAD / branche / état du dépôt au départ :**
- **Fichiers examinés / modifiés :**
- **Décisions / justifications :**
- **Actions effectivement réalisées :**
- **Statuts distincts :** prévu ; implémenté ; compilé ; testé PC ; testé STM32 ; validé STM32 ; bloqué.
- **Environnement, commandes et reproduction :**
- **Résultats attendus puis mesurés (avec preuves) :**
- **Échecs, anomalies, limitations :**
- **Commit(s) / push GitHub vérifié(s) :**
- **Prochaine action / handoff :**
