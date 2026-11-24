# P01 - Validation CameraSource minimal

## Environnement

- Date de validation : 2026-11-24.
- Depot : rahim974/3Dscan2A.
- Branche : Note-de-Seb.
- Carte : STM32N6570-DK.
- Identifiant DK-A ou DK-B : non precise.
- Camera : Sony IMX335.
- SDK : STM32CubeN6 v1.2.0.
- IDE : STM32CubeIDE.
- Workspace : STM32_Workspace_Projet2A.
- Configuration : Debug.
- Depot local : D:\Projet_2A.

## Implementation

- Creation de camera_source.h et camera_source.c.
- Integration de la ressource liee dans .project.
- Initialisation CameraSource avant le lancement DCMIPP.
- Publication des metadonnees depuis le callback.
- Lecture et acquittement depuis la boucle principale.
- Protection de la copie des metadonnees par PRIMASK.
- Conservation du pipeline materiel existant.

## Compilation

- Compilation effective de camera_source.c observee.
- Build Debug reussi.
- 0 erreur, 0 avertissement.

## Incidents anterieurs

- Erreurs ST-LINK et GDB Server.
- Target no device found.
- Load failed.
- Timeout localhost:61234.
- Connexion Debug finalement retablie.
- Cause racine non determinee.
- Deux cartes disponibles et affichees comme fonctionnelles
  en Debug selon le retour utilisateur.
- Validation P01 realisee sur une seule carte.

## Procedure de test

1. Compiler le firmware P01 en Debug.
2. Charger le firmware sur STM32.
3. Laisser fonctionner la camera.
4. Suspendre et relever les expressions CameraSource.
5. Reprendre l'execution sans redemarrage.
6. Suspendre et relever une seconde fois.
7. Examiner la structure CameraFrame.

## Mesures

| Variable | Mesure 1 | Mesure 2 |
|---|---:|---:|
| read_count | 3634 | 3887 |
| last_id | 3634 | 3887 |
| last_timestamp_ms | 120783 | 129144 |

- Delta identifiant : 253.
- Delta compteur de lecture : 253.
- Delta timestamp : 8361 ms.
- Cadence estimee : 253 / 8.361 = 30.26 FPS.
- Aucun saut de frame observe par difference des compteurs.
- Baseline P00 : 30 a 31 FPS.

## Structure CameraFrame observee

| Champ | Valeur |
|---|---|
| frame_id | 3887 |
| timestamp_ms | 129144 |
| width | 800 |
| height | 480 |
| format | CAMERA_PIXEL_FORMAT_RGB565 |
| data | 0x34200000 |
| data_size_bytes | 768000 |

## Resultats

- Acquisition et affichage operationnels selon utilisateur.
- Publication et lecture des metadonnees fonctionnelles.
- Metadonnees fixes conformes.
- Pas de regression de cadence observee.
- P01 validee sur STM32 dans le perimetre teste.

## Limites

- Pas de test croise P01 sur la seconde carte.
- Pas de test exhaustif de concurrence.
- Pas de test de debordement des compteurs.
- Cadence deduite des timestamps et identifiants.
- Stabilite des pixels non garantie.
- Aucun snapshot ni double buffering.
- Le framebuffer demeure partage avec DCMIPP.
- Timestamp correspondant a la fin de frame.

## Suite

P02 : deplacement progressif du pipeline materiel
hors du code applicatif.