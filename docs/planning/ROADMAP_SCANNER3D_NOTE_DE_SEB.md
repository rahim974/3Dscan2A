# Scanner 3D ENSEA — Feuille de route par passes

**Établie le :** 2026-11-17  
**Dépôt :** `rahim974/3Dscan2A`  
**Branche de travail exclusive :** `Note-de-Seb`  
**Suivi chronologique :** [`docs/SCANNER3D_PROGRESS.md`](../SCANNER3D_PROGRESS.md)

## Objectif général

Construire progressivement un scanner 3D portable sur STM32N6570-DK utilisant la caméra Sony IMX335 et le capteur de profondeur VL53L9CX, avec une V1 **Stop-and-Scan** (arrêt → acquisition → timestamp → association à une pose → déplacement). La caméra assure la vision RGB via CSI-2/DCMIPP/ISP ; le LiDAR fournit la géométrie. L'I3C pour le VL53L9CX est une orientation à confirmer lors du bring-up réel. Les objectifs ultérieurs sont la reconstruction 3D, une représentation 2D et éventuellement l'IA embarquée.

## Règles communes à toutes les passes

1. **Une passe = un chat dédié.** Dans le chat suivant, relire ce plan, `docs/SCANNER3D_PROGRESS.md`, le handoff de la passe et les fichiers source réels.
2. **Source de vérité : la branche `Note-de-Seb`.** Avant toute modification, vérifier la tête distante, la branche locale, `git status`, le fichier réellement lié dans CubeIDE et les écarts éventuels. Ne jamais toucher à `main` sans instruction explicite.
3. **Séquences courtes.** Préparer, implémenter, compiler, tester, examiner les résultats puis documenter. Ne pas modifier la passe suivante par anticipation.
4. **Suivi continu.** Consigner dans le journal permanent chaque étape significative (y compris échec, blocage et décision) **pendant la séance**, et pas uniquement à la clôture.
5. **Statuts distincts :** prévu / implémenté / compilé / testé PC / testé STM32 / validé STM32 / bloqué. Une compilation seule ou un résultat attendu n'est pas une validation matérielle.
6. **Clôture :** documenter les commandes, environnements, mesures et limites dans `docs/tests/` ; actualiser le journal ; commit et push exclusivement sur `Note-de-Seb` ; vérifier la tête distante ; préparer un handoff précis pour la passe suivante.
7. Chaque handoff est **historique** : s'il contredit un code ou test ultérieur, la version réelle de la branche et les entrées récentes du journal prévalent.

## Roadmap

| Passe | Objectif | Critère principal de clôture | Statut de départ |
|---|---|---|---|
| P00 | Acquisition IMX335, framebuffer 800×480 RGB565, `CameraFrame` dynamique et cadence ~30 FPS | Test réel STM32 documenté | **Validée** ; commit `bda6a43` |
| P01 | **CameraSource minimal** : `.h` / `.c`, état nouvelle frame et API de lecture des métadonnées | Fichiers réellement construits par CubeIDE depuis Git ; test STM32 et non-régression FPS | **Validee STM32** - 30.26 FPS |
| P02 | Déplacement progressif des appels IMX335/DCMIPP/ISP hors du code applicatif | `main.c` consomme l'API ; image et FPS maintenus sur STM32 | **Validee STM32** - 30.45 FPS |
| P03 | Snapshot / capture à la demande et intégrité des buffers | Captures successives fiables ; pixels non réécrits pendant leur exploitation | **Prochaine passe** - non implementee |
| P04 | LiDAR simulé : statistiques et scènes 54×42 | Valeurs attendues vérifiées sur STM32 | Prévu |
| P05 | Détection obstacle ; `lidar_types` et `lidar_processing` | Tests STM32 ; traitements indépendants du simulateur | Prévu |
| P06 | `LidarSource` interchangeable, backend simulation | API et génération de frames testées sur STM32 | Prévu |
| P07 | Bring-up VL53L9CX réel et interface matérielle confirmée | Acquisition physique vérifiée sur STM32 ; sinon *bloqué matériel* | Prévu |
| P08 | Synchronisation caméra/LiDAR, `SensorPair`, protocole Stop-and-Scan | Paires horodatées et décalage temporel mesuré | Prévu |
| P09 | Projection profondeur → XYZ et export d'un nuage de points | Géométrie de référence reproductible | Prévu |
| P10 | Estimation/association des poses et alignement multi-captures | Scène de référence reconstruite avec erreurs caractérisées | Prévu |
| P11 | Représentation 2D (occupancy map) et fusion RGB-D après calibration | Carte reproductible avec métriques/limites explicites | Prévu |
| P12 (optionnel) | IA / Neural-ART pour sémantique | Utilité et performances évaluées ; non prioritaire | Différé |

## Dépendances et risques

- L'intégration du LiDAR physique dépend de la disponibilité du VL53L9CX. Si nécessaire, avancer les étapes géométriques **en simulation**, sans revendiquer une validation du capteur réel.
- `CameraFrame.data` est un pointeur vers un framebuffer partagé avec DCMIPP : **la copie des métadonnées ne garantit pas la stabilité des pixels**. P01 ne doit pas prétendre résoudre ce problème ; P03 définira la stratégie de buffering.
- Le callback de fin de frame peut être exécuté en contexte d'interruption. Le traitement doit rester court et les accès partagés doivent être examinés.
- Le timestamp actuel représente un événement de **fin de frame** via `HAL_GetTick()` : à documenter pour l'appariement LiDAR.
- Stop-and-Scan ne fournit pas une estimation de pose ; la méthode et les capteurs/repères nécessaires restent à étudier.

## Références existantes

- `docs/handoffs/HANDOFF_SEB_2026-11-10.md` — handoff historique, avant la validation dynamique.
- `docs/tests/CAMERA_DYNAMIC_FRAME_VALIDATION_2026-11-17.md` — rapport P00.
- `Protocoles testes et simulations/test_camera.html` et `Protocoles testes et simulations/simulation_lidar.html`.
- `firmware/camera_imx335/FSBL/Src/main.c` et `firmware/camera_imx335/STM32CubeIDE/FSBL/Application/User/camera_types.h`.
- Commits : `b66709b` (baseline), `e4be1bc` (handoff historique), `bda6a43` (CameraFrame dynamique).

Toute révision du planning doit être datée et motivée dans le journal permanent.
