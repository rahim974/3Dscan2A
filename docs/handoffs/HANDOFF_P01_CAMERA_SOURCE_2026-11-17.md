# Handoff — P01 : CameraSource minimal

**Date :** 2026-11-17  
**Dépôt :** `rahim974/3Dscan2A`  
**Branche autorisée :** `Note-de-Seb` exclusivement  
**Dernier jalon firmware matériel :** `bda6a43` (P00 : CameraFrame dynamique)  
**Statut P01 :** prévu, **aucune implémentation P01 validée**.

## À lire dans le nouveau chat, dans cet ordre

1. `docs/planning/ROADMAP_SCANNER3D_NOTE_DE_SEB.md`.
2. `docs/SCANNER3D_PROGRESS.md`.
3. Le présent handoff.
4. `docs/tests/CAMERA_DYNAMIC_FRAME_VALIDATION_2026-11-17.md`.
5. `firmware/camera_imx335/FSBL/Src/main.c`.
6. `firmware/camera_imx335/STM32CubeIDE/FSBL/Application/User/camera_types.h`.
7. `firmware/camera_imx335/STM32CubeIDE/FSBL/.project` et `.cproject`.
8. Au besoin : `Protocoles testes et simulations/test_camera.html` et `docs/handoffs/HANDOFF_SEB_2026-11-10.md` (historique).

Toujours vérifier le dernier commit sur GitHub : ce handoff peut devenir obsolète après de nouveaux commits. **Ne jamais travailler sur `main`.**

## Baseline à préserver

- Caméra Sony IMX335 : acquisition via CSI-2/DCMIPP/ISP et LCD fonctionnels sur STM32N6570-DK.
- Image 800×480 RGB565 ; framebuffer `0x34200000`, taille 768000 octets ; cadence ~30 FPS.
- `CameraFrame` est une structure de métadonnées avec **pointeur**, et non une copie de l'image.
- Callback DCMIPP : `NbMainFrames++`, `g_camera_frame.frame_id = NbMainFrames`, `g_camera_frame.timestamp_ms = HAL_GetTick()`.
- Mesures depuis la **version importée directement du dépôt** : compteur/id 197 puis 300 ; timestamp 6786 puis 10104 ms ; FPS 31 puis 30.
- CubeIDE doit utiliser le vrai fichier `D:\Projet_2A\firmware\camera_imx335\FSBL\Src\main.c`, et non `C:\STM32\DCMIPP_ContinuousMode\FSBL\Src\main.c`.
- Le simulateur LiDAR n'est pas modifié pendant P01.

## Mission et limites de P01

**Objectif :** fournir une API `CameraSource` minimale pour savoir qu'une nouvelle frame est disponible et récupérer ses **métadonnées** sans casser le pipeline actuellement dans `main.c`.

- Créer `camera_source.h` et `camera_source.c` aux emplacements compatibles avec la disposition du projet (candidats `FSBL/Inc` et `FSBL/Src`).
- Définir les contrats de `CameraSource_IsFrameReady()` et `CameraSource_GetLatestFrame()` ; clarifier si l'API consomme le signal de disponibilité, ce que signifie « dernière frame », la copie des métadonnées et les appels en présence d'une ISR.
- Examiner la cohérence des métadonnées lues depuis des contextes concurrents. Préférer une stratégie courte et explicable au callback ; ne pas copier 768000 octets dans une structure.
- Vérifier que le nouveau `.c` est **vraiment compilé** : les sources référencées via le fichier CubeIDE `.project` sont liées. Adapter la configuration si nécessaire.
- Ajouter un contrôle minimal via `main.c` et tester réellement sur STM32, avec les FPS proches de 30 et sans régression d'affichage.
- **Hors périmètre :** transfert massif de l'initialisation matérielle (P02), snapshot et stabilisation des pixels (P03), LiDAR, fusion RGB-D, IA.

## Séquence de travail

1. Lecture de la branche distante + vérification `git branch --show-current`, `git status --short`, `git log -1 --oneline`.
2. Audit de l'API visée, de la mémoire et du build CubeIDE ; consigner les décisions au journal.
3. Implémentation minimale et intégration **réelle** du `.c` dans le projet.
4. Build Debug `DCMIPP_ContinuousMode_FSBL` depuis `D:\Projet_2A` et inspection du journal de compilation.
5. Debug STM32 : frames consécutives, métadonnées cohérentes, cadence, non-régression.
6. Mettre à jour le journal **après chaque étape significative**, y compris erreurs.
7. Rapport `docs/tests/`, commit et push sur `Note-de-Seb` ; vérification du HEAD distant.
8. Rédiger un handoff P02 pour un nouveau chat et **s'arrêter avant d'entamer P02**.

## Prompt à copier pour ouvrir le chat P01

> Reprends `rahim974/3Dscan2A` exclusivement sur `Note-de-Seb`, pour la passe **P01 — CameraSource minimal**. Commence par lire `docs/planning/ROADMAP_SCANNER3D_NOTE_DE_SEB.md`, `docs/SCANNER3D_PROGRESS.md` et `docs/handoffs/HANDOFF_P01_CAMERA_SOURCE_2026-11-17.md`, puis inspecte le code et les réglages CubeIDE réels sur GitHub. Vérifie la tête du dépôt avant toute modification. P00 est validée sur STM32 au commit `bda6a43` : caméra IMX335, CameraFrame dynamique et ~30 FPS. L'objectif de P01 est une API minimale pour les métadonnées et la disponibilité des frames, **sans refondre le pipeline matériel** et en prenant en compte les accès concurrentiels. Guide-moi étape par étape, actualise le **journal permanent** sur `Note-de-Seb` après chaque jalon significatif et distingue prévu / implémenté / compilé / testé STM32 / validé STM32. Pas de travail sur `main`. En fin de passe, fournir le rapport de test, commit, push et handoff P02, puis arrêter cette conversation.
