# P02 - Validation du refactoring camera

## Environnement

- Date : 2026-11-24.
- Depot : rahim974/3Dscan2A.
- Branche exclusive : Note-de-Seb.
- Baseline P01 : 896783dca9ad39fab0c64eddf4660d586a5fc787.
- Carte : STM32N6570-DK.
- Identifiant individuel de la carte : non precise.
- Camera : Sony IMX335.
- SDK : STM32CubeN6 v1.2.0.
- IDE : STM32CubeIDE.
- Configuration : Debug.
- Depot local : D:\Projet_2A.
- Workspace : D:\STM32_Workspace_Projet2A.

## Implementation

- Transfert des fonctions IMX335, DCMIPP et ISP vers camera_source.c.
- Transfert des callbacks DCMIPP.
- Conservation de la visibilite externe de hdcmipp pour les IRQ.
- Ajout des fonctions publiques :
  - CameraSource_PrepareHardware()
  - CameraSource_Start()
  - CameraSource_Process()
  - CameraSource_GetFrameCount()
- Conservation de l'ordre d'initialisation materiel.
- Conservation des reglages DCMIPP, CSI et IMX335.
- Conservation de la protection PRIMASK.
- Conservation des compteurs de diagnostic P01.
- LCD et horloges systeme maintenus dans main.c.
- Aucune modification de la configuration CubeIDE suivie par Git.

## Compilation

- Build Debug observe a 15:07:17.
- Duree : 3.993 secondes.
- 0 erreur.
- 0 avertissement.
- ELF, BIN et LIST generes.

| Section | Octets |
|---|---:|
| text | 148300 |
| data | 324 |
| bss | 8836 |
| Total des sections | 157460 |

## Validation materielle

- Acquisition camera et affichage LCD confirmes fonctionnels.
- Execution effective de CameraSource_Process observee dans la pile Debug.
- Pile d'appels :
  main -> CameraSource_Process -> ISP_BackgroundProcess
  -> ISP_Algo_Process -> ISP_Algo_AWB_Process.

## Mesures

| Variable | Releve 1 | Releve 2 |
|---|---:|---:|
| read_count | 563 | 652 |
| last_id | 563 | 652 |
| last_timestamp_ms | 18971 | 21894 |
| camera_fps | 30 | 30 |

- Delta frame_id : 89.
- Delta read_count : 89.
- Delta timestamp : 2923 ms.
- Cadence estimee : 89 / 2.923 = 30.45 FPS.
- Reference P01 : 30.26 FPS.
- Aucun saut observe par difference des compteurs.
- Aucune regression de cadence observable.

## Metadonnees CameraFrame

| Champ | Valeur observee |
|---|---|
| frame_id | 652 |
| timestamp_ms | 21894 |
| width | 800 |
| height | 480 |
| format | CAMERA_PIXEL_FORMAT_RGB565 |
| data | 0x34200000 |
| data_size_bytes | 768000 |

## Preuves

- Sortie PowerShell du refactoring.
- Console de compilation STM32CubeIDE.
- Capture de la pile d'appels Debug.
- Deux captures des expressions de diagnostic.
- Capture finale de CameraFrame deployee.
- Confirmation utilisateur du fonctionnement de la camera.

Ces observations sont fournies dans le chat P02.

## Conclusion

P02 est implemente, compile, teste et valide sur STM32
dans le perimetre defini par la roadmap.

main.c consomme desormais l'API CameraSource pour
le pilotage du pipeline camera.

## Limites

- Validation sur une seule carte.
- Intervalle quantitatif de 2923 ms.
- Pas de test exhaustif de concurrence.
- Pas de test longue duree.
- Pas de garantie de stabilite des pixels.
- Framebuffer toujours partage avec DCMIPP et LTDC.
- Timestamp correspondant a la fin de frame.
- Aucun snapshot ni double buffering implemente.

## Suite

P03 : snapshot, capture a la demande et integrite des buffers.
