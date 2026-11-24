# Handoff P03 - Snapshot et integrite des buffers

## Regles de travail

- Depot : rahim974/3Dscan2A.
- Branche exclusive : Note-de-Seb.
- Ne jamais modifier main (branche Git).
- Depot local : D:\Projet_2A.
- Workspace : D:\STM32_Workspace_Projet2A.
- Windows 11, PowerShell, STM32CubeIDE.
- Deux cartes STM32N6570-DK fonctionnelles.
- Une seule carte utilisee pour les validations courantes.
- Seconde carte reservee au secours.
- Actualiser continuellement le journal permanent.
- Regrouper les operations compatibles pour accelerer.

## Documents obligatoires

- docs/planning/ROADMAP_SCANNER3D_NOTE_DE_SEB.md
- docs/SCANNER3D_PROGRESS.md
- docs/tests/CAMERA_SOURCE_VALIDATION_2026-11-24.md
- docs/tests/CAMERA_REFACTOR_VALIDATION_2026-11-24.md
- Le present handoff.

## Sources a examiner

- firmware/camera_imx335/FSBL/Src/main.c
- firmware/camera_imx335/FSBL/Src/camera_source.c
- firmware/camera_imx335/FSBL/Inc/camera_source.h
- firmware/camera_imx335/FSBL/Inc/main.h
- firmware/camera_imx335/FSBL/Src/stm32n6xx_it.c
- firmware/camera_imx335/FSBL/Src/stm32n6xx_hal_msp.c
- firmware/camera_imx335/STM32CubeIDE/FSBL/Application/User/camera_types.h
- Configuration CubeIDE et linker script.
- Configuration memoire, caches, DMA et ressources DCMIPP.

## Baseline P02 validee

- Compilation : 0 erreur, 0 avertissement.
- Camera et LCD fonctionnels.
- Cadence estimee : 30.45 FPS.
- 89 frames recuperees sur 89 entre les releves.
- Resolution : 800 x 480.
- Format : RGB565.
- Framebuffer : 0x34200000.
- Taille : 768000 octets.

## API CameraSource

- CameraSource_PrepareHardware()
- CameraSource_Start()
- CameraSource_Process()
- CameraSource_GetFrameCount()
- CameraSource_Init()
- CameraSource_OnFrameEvent()
- CameraSource_IsFrameReady()
- CameraSource_GetLatestFrame()

## Probleme a resoudre

CameraSource_GetLatestFrame copie les metadonnees,
mais ne copie pas les pixels.

Le pointeur CameraFrame.data reference un framebuffer
partage avec DCMIPP et LTDC. Son contenu peut donc
changer pendant son exploitation.

## Mission P03

Definir puis implementer une capture a la demande
fournissant des pixels stables pendant leur exploitation.

## Sequence recommandee

1. Verifier HEAD distant et etat du depot local.
2. Examiner les sources reelles de P02.
3. Auditer la memoire disponible, le linker et les caches.
4. Verifier les possibilites reelles du DCMIPP.
5. Comparer snapshot materiel, buffers alternes et copie.
6. Choisir une strategie justifiee et compatible avec le LCD.
7. Definir le contrat de propriete et de duree de vie des buffers.
8. Implementer une solution minimale.
9. Compiler et tester sur STM32.
10. Documenter et publier les resultats.

Ne pas considerer une simple copie du framebuffer
actif comme un snapshot coherent sans preuve de synchronisation.

Ne pas choisir une adresse memoire arbitraire sans
verifier les regions disponibles et les chevauchements.

## Criteres de cloture

- Compilation sans erreur.
- Captures successives fonctionnelles.
- Metadonnees coherentes et associees aux pixels captures.
- Absence de modification du buffer capture pendant sa detention.
- Verification experimentale de l'integrite des pixels.
- Impact sur la cadence mesure et documente.
- LCD fonctionnel ou limitation explicitement justifiee.
- Rapport, journal, commit et push verifies.
- Handoff P04.

## Hors perimetre

- LiDAR.
- Fusion RGB-D.
- Reconstruction 3D.
- Estimation de pose.
- IA embarquee.

## Etat Git

Ce document est cree dans le commit de cloture P02.
Le prochain chat doit verifier son SHA distant reel
avant toute modification.

La modification locale language.settings.xml peut
subsister. Si elle concerne uniquement env-hash,
elle ne doit pas etre incluse dans les commits.
