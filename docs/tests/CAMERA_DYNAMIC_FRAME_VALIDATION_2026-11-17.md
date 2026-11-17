# Validation CameraFrame dynamique — 17 novembre 2026

## Environnement

- Carte : STM32N6570-DK
- Capteur : Sony IMX335
- SDK : STM32CubeN6 v1.2.0
- IDE : STM32CubeIDE
- Depot : rahim974/3Dscan2A
- Branche : Note-de-Seb
- Workspace : D:\STM32_Workspace_Projet2A
- Source : firmware/camera_imx335/FSBL/Src/main.c

## Objectif

Actualiser frame_id et timestamp_ms lors de chaque
evenement de fin de frame DCMIPP.

## Modification

Dans HAL_DCMIPP_PIPE_FrameEventCallback :

    NbMainFrames++;
    g_camera_frame.frame_id = NbMainFrames;
    g_camera_frame.timestamp_ms = HAL_GetTick();

## Procedure de validation

1. Importer le projet CubeIDE depuis le depot Git.
2. Verifier le chemin physique de main.c.
3. Compiler DCMIPP_ContinuousMode_FSBL en Debug.
4. Lancer le firmware sur STM32N6570-DK.
5. Laisser la camera fonctionner sans breakpoint.
6. Suspendre et relever les expressions.
7. Reprendre, puis suspendre une seconde fois.

## Compilation

Projet : DCMIPP_ContinuousMode_FSBL
Resultat observe : 0 errors, 0 warnings.

## Mesures sur STM32

| Expression | Mesure 1 | Mesure 2 |
|---|---:|---:|
| NbMainFrames | 197 | 300 |
| g_camera_frame.frame_id | 197 | 300 |
| g_camera_frame.timestamp_ms | 6786 | 10104 |
| HAL_GetTick() | 6796 | 10131 |
| g_camera_fps | 31 | 30 |

## Resultats

- frame_id suit exactement NbMainFrames.
- timestamp_ms augmente entre deux observations.
- HAL_GetTick() progresse normalement.
- La cadence reste proche de 30 FPS.
- Le framebuffer reste reference par CameraFrame.
- Resolution : 800 x 480.
- Format : RGB565.
- Adresse framebuffer : 0x34200000.
- Taille framebuffer : 768000 octets.

## Statuts

- Modification : implementee
- Compilation : reussie
- Execution STM32 : testee
- Metadonnees dynamiques : validees
- CameraSource : non implementee
- Synchronisation RGB-D : non validee

## Limites

Le timestamp correspond a l'evenement de fin de frame
et non necessairement au debut de l'exposition.

La coherence d'une image lors de sa lecture concurrente
avec le DCMIPP reste a traiter dans CameraSource.

## Prochain jalon

Creer camera_source.h et camera_source.c, puis valider
leur fonctionnement sur la STM32.
