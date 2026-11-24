# Handoff - P02 : refactoring du pipeline camera

## Regles

- Depot : rahim974/3Dscan2A.
- Branche exclusive : Note-de-Seb.
- Depot local : D:\Projet_2A.
- Ne jamais modifier main (branche Git).
- Une passe par chat.
- Actualiser le journal permanent apres chaque jalon.
- Verifier le HEAD distant avant modification.
- Privilegier des sequences courtes et rapides.
- Tester sur une seule carte STM32N6570-DK.
- Seconde carte reservee au secours.

## Documents a lire

- docs/planning/ROADMAP_SCANNER3D_NOTE_DE_SEB.md
- docs/SCANNER3D_PROGRESS.md
- docs/tests/CAMERA_SOURCE_VALIDATION_2026-11-24.md
- Le present handoff.

## Sources a examiner

- firmware/camera_imx335/FSBL/Src/main.c
- firmware/camera_imx335/FSBL/Src/camera_source.c
- firmware/camera_imx335/FSBL/Inc/camera_source.h
- firmware/camera_imx335/STM32CubeIDE/FSBL/Application/User/camera_types.h
- Configuration CubeIDE .project et .cproject.

## Baseline P01 validee

API disponible :

- CameraSource_Init()
- CameraSource_OnFrameEvent()
- CameraSource_IsFrameReady()
- CameraSource_GetLatestFrame()

Mesures :

- Cadence : 30.26 FPS.
- 253 frames recuperees entre les deux releves.
- Resolution : 800 x 480.
- Format : RGB565.
- Framebuffer : 0x34200000.
- Taille : 768000 octets.

## Mission P02

Deplacer progressivement les appels et etats
specifiques a IMX335, DCMIPP et ISP hors de main.c.

Le code applicatif doit progressivement consommer
une interface CameraSource plus autonome.

## Sequence recommandee

1. Audit de la version reelle sur Note-de-Seb.
2. Cartographie des dependances HAL, ISP et IMX335.
3. Definition des responsabilites du module.
4. Refactoring progressif sans changer les reglages materiels.
5. Compilation Debug.
6. Test acquisition, affichage et metadonnees sur STM32.
7. Verification de cadence par rapport a P01.
8. Rapport, journal, commit, push et handoff P03.

## Criteres de cloture

- Compilation sans erreur.
- Camera et LCD operationnels.
- CameraFrame dynamique et coherent.
- Cadence proche de 30 FPS.
- main.c simplifie.
- Aucun changement fonctionnel non justifie.

## Hors perimetre

- Snapshot.
- Stabilisation des pixels.
- Double buffering.
- LiDAR.
- Fusion RGB-D.
- Reconstruction 3D.
- IA.

Ces fonctions appartiennent aux passes suivantes.
Ne pas commencer P03 pendant P02.

## Priorite

La deadline approche : limiter les tests redondants,
sans supprimer les controles indispensables.