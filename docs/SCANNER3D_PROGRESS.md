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

### 2026-11-17 — P01 : audit initial

- Branche : Note-de-Seb exclusivement.
- HEAD initial : 00b38bde2582556cf1218a3806f31df3b56e19eb.
- Verification locale : working tree clean ; divergence 0/0.
- Baseline P00 : CameraFrame dynamique, STM32 validee, ~30 FPS.
- Audit CubeIDE : camera_source.c absent des ressources liees de .project.
- Risques : concurrence IRQ/lecture des metadonnees ; ordre d'initialisation ; framebuffer partage.
- Ecriture GitHub directe : erreur 403 ; suivi effectue localement avant commit/push.
- Statut P01 : audit effectue ; implementation non commencee ; compilation et tests non realises.
- Suite : definir CameraSource, integrer les sources, compiler et tester sur STM32.
### 2026-11-17 — P01 : module cree et integre au projet

- Fichiers crees : FSBL/Inc/camera_source.h et FSBL/Src/camera_source.c.
- API initiale : Init, OnFrameEvent, IsFrameReady, GetLatestFrame.
- Concurrence : section critique courte pour la copie des metadonnees.
- Configuration : ressource liee ajoutee a STM32CubeIDE/FSBL/.project.
- Verification : XML valide et git diff --check sans erreur.
- Statut : code cree localement ; compilation non encore testee.
- Limite : aucune garantie de stabilite des pixels du framebuffer.
- Prochaine action : compilation Debug du module dans CubeIDE.
### 2026-11-17 — P01 : premiere compilation reussie

- Projet : DCMIPP_ContinuousMode_FSBL.
- Configuration : Debug, STM32CubeIDE.
- Source : FSBL/Src/camera_source.c.
- Verification : commande arm-none-eabi-gcc visible.
- Edition des liens : reussie, ELF et BIN generes.
- Resultat : 0 erreur, 0 avertissement.
- Statut : CameraSource compile ; integration dans main.c a effectuer.
- Tests STM32 P01 : non effectues.
- Prochaine action : raccorder l'API au pipeline DCMIPP.
### 2026-11-17 — P01 : raccordement et blocage ST-LINK

- CameraSource raccorde dans main.c.
- Appels Init, OnFrameEvent, IsFrameReady et GetLatestFrame presents.
- Build Debug observe a 15:46 : 0 erreur, 0 avertissement.
- Firmware ELF genere ; compilation incrementale reussie.
- Lancement Debug : Failed to start GDB server.
- Detail : Error in initializing ST-LINK device, reason (255) Unknown.
- Cause precise : non determinee.
- Statut : compile ; test STM32 P01 bloque.
- Action suivante : diagnostic USB, alimentation et connexion ST-LINK.
- Aucun flash ni validation STM32 P01 atteste.
### 2026-11-17 — P01 : echec du chargement ELF

- Compilation Debug : reussie, 0 erreur, 0 avertissement.
- Nouvelle tentative Debug : serveur GDB lance jusqu'a la commande load.
- Erreur : Failed to execute MI command: load.
- Message final : Load failed.
- Fichier cible : DCMIPP_ContinuousMode_FSBL.elf.
- Configuration Git inspectee : ST-LINK, SWD, access port 1, connect under reset.
- Linker : code en RAM interne a 0x34180400.
- Hypothese a verifier : BOOT1 en mode developpement.
- Statut : firmware compile ; chargement et tests STM32 non valides.
- Prochaine action : verifier BOOT1 et le journal ST-LINK GDB Server.

### 2026-11-24 - P01 : validation STM32

- CameraSource minimal implemente et compile.
- Build Debug : 0 erreur, 0 avertissement.
- Blocage ST-LINK depasse ; cause racine non determinee.
- Acquisition et affichage operationnels.
- Deux cartes STM32N6570-DK disponibles.
- Choix : une seule carte pour les validations courantes.
- Seconde carte conservee en secours.
- Identifiant de la carte utilisee pour P01 non precise.
- Mesure 1 : id=3634, read_count=3634, timestamp=120783 ms.
- Mesure 2 : id=3887, read_count=3887, timestamp=129144 ms.
- 253 frames sur 8361 ms : 30.26 FPS.
- Aucun saut observe par difference des compteurs.
- CameraFrame : 800x480 RGB565, adresse 0x34200000.
- Taille du framebuffer : 768000 octets.
- Statut : P01 validee sur STM32 dans le perimetre teste.
- Stabilite des pixels reservee a P03.
- Rapport : docs/tests/CAMERA_SOURCE_VALIDATION_2026-11-24.md.
- Handoff : docs/handoffs/HANDOFF_P02_CAMERA_REFACTOR_2026-11-24.md.
- Prochaine passe : P02, refactoring du pipeline camera.



### 2026-11-24 - P02 : audit et refactoring camera

- Branche exclusive : Note-de-Seb.
- Baseline locale et distante verifiee : 896783dca9ad39fab0c64eddf4660d586a5fc787.
- Les quatre documents de reference P02 ont ete lus sur GitHub.
- Sources HAL, ISP, IMX335, interruptions et configuration CubeIDE examinees.
- DCMIPP, IMX335, ISP et callbacks transferes dans camera_source.c.
- API : PrepareHardware, Start, Process, GetFrameCount.
- Ordre initialisation DCMIPP / IMX335 / LCD / ISP conserve.
- Reglages materiels et callbacks extraits sans modification fonctionnelle intentionnelle.
- API P01 et protection PRIMASK conservees.
- Compteurs de diagnostic applicatifs conserves.
- Ressource CubeIDE existante verifiee.
- Settings : Variation locale env-hash uniquement ; conservee hors commit P02.
- Statut : implemente localement ; compilation et validation STM32 a effectuer.
- Limite : framebuffer toujours partage ; snapshot reserve a P03.
- Prochaine action : compilation Debug et validation STM32.


### 2026-11-24 - P02 : compilation Debug reussie

- Refactoring local execute avec succes.
- Controles statiques et git diff --check reussis.
- Quatre nouvelles fonctions CameraSource integrees.
- Pipeline IMX335 / DCMIPP / ISP deplace hors de main.c.
- Compilation Debug CubeIDE a 15:07:17.
- Resultat : 0 erreur, 0 avertissement.
- Duree de compilation : 3.993 secondes.
- ELF, BIN et LIST generes.
- Text : 148300 octets.
- Data : 324 octets.
- BSS : 8836 octets.
- Total des sections : 157460 octets.
- Modification language.settings.xml exclue du futur commit.
- Statut P02 : implemente et compile.
- Validation STM32 non encore effectuee.
- Prochaine action : test materiel et non-regression FPS.


### 2026-11-24 - P02 : validation STM32 et cloture

- Camera et LCD confirmes fonctionnels.
- Execution du nouveau CameraSource observee dans la pile Debug.
- Releve 1 : read_count=563 ; id=563 ; timestamp=18971 ms.
- Releve 2 : read_count=652 ; id=652 ; timestamp=21894 ms.
- 89 frames sur 2923 ms : 30.45 FPS.
- Baseline P01 : 30.26 FPS.
- Aucun saut observe entre les deux releves.
- CameraFrame : 800x480 RGB565.
- Adresse : 0x34200000.
- Taille : 768000 octets.
- Toutes les metadonnees controlees sont conformes.
- Compilation : 0 erreur, 0 avertissement.
- Statut : P02 validee sur STM32 dans le perimetre teste.
- Limites : une seule carte et intervalle de mesure court.
- Stabilite des pixels non garantie ; reservee a P03.
- Settings : Variation env-hash uniquement ; conservee hors commit.
- Rapport : docs/tests/CAMERA_REFACTOR_VALIDATION_2026-11-24.md.
- Handoff : docs/handoffs/HANDOFF_P03_CAMERA_SNAPSHOT_2026-11-24.md.
- Roadmap actualisee pour P01, P02 et P03.
- Publication : commit de cloture P02 sur Note-de-Seb ;
  verification distante executee apres le push.
- Prochaine passe : P03, snapshot et integrite des buffers.

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


### 2026-11-24 - P03 : audit initial

- Branche exclusive : Note-de-Seb.
- Baseline distante et locale :
  dd25eb349e3d2bf0955646915ffb01155a2d2812.
- Cinq documents obligatoires lus sur GitHub.
- Sources CameraSource, HAL, ISP et linker examinees.
- D-Cache et I-Cache actives dans P02.
- Snapshot materiel et partage Pipe1/Pipe2 confirmes.
- Strategie retenue : Pipe1 continu, Pipe2 snapshot.
- AXISRAM5/6 identifiees comme zone candidate.
- Capacite candidate : 917504 octets.
- Snapshot RGB565 : 768000 octets.
- Reservation effective et permissions a confirmer.
- Callback Pipe1 a distinguer de Pipe2.
- Propriete exclusive du snapshot jusqu'a liberation.
- Statut : audit et conception ; non implemente.
- Compilation et validation STM32 non effectuees.
- Prochaine action : implementation et reservation linker.

### 2026-11-24 - P03 : implementation locale

- Prevol local : linker Debug confirme ; aucune occurrence de conflit trouvee dans les dossiers examines.
- Correction du blanc final du journal (git diff --check).
- Carte memoire : AXISRAM5/6, 0x342E0000 a 0x343BFFFF ; RM0486, 896 Kio.
- Section .camera_snapshot NOLOAD, 768000 octets, alignement 32, assertions linker.
- Activation SRAM5/6 ; configuration partagee ISP et sortie RGB565 Pipe2.
- Snapshot materiel ; Pipe1 conserve en continu ; compteurs separes.
- Transfert explicite de propriete, maintenance D-Cache, controle CPTACT/CPTREQ, erreurs et timeout.
- Test automatique : CRC32 complet avant/apres detention de 5 s, rearmement refuse pendant detention, cadence Pipe1.
- Les visuels techniques seront versions sous docs/assets/diagrams/ aux jalons majeurs ; distinguer conception et validation.
- Statut : modifications locales ; compilation, placement effectif et validation STM32 non effectues.
- Prochaine action : compilation Debug et controle du fichier .map avant lancement materiel.

### 2026-11-24 - P03 : compilation Debug

- Implementation locale appliquee avec succes.
- Compilation Debug a 15:33:42.
- Resultat : 0 erreur, 0 avertissement.
- Duree : 1.217 seconde.
- Text : 150700 octets.
- Data : 324 octets.
- BSS : 778004 octets.
- Total : 929028 octets.
- Section .camera_snapshot verifiee dans le map.
- Adresse : 0x342E0000.
- Taille : 768000 octets.
- Statut : implemente et compile.
- Validation STM32 non effectuee.
- Prochaine action : test snapshot et CRC32.

### 2026-11-24 - P03 : premier test STM32

- Pipe1 operationnel : 30 FPS.
- Frame ID et read_count : 966.
- Timestamp : 32313 ms.
- Snapshot : SNAPSHOT_ERROR.
- Phase de test : 3.
- Compteur errors : 2.
- Captures et releases : 0.
- Request_tick : 0.
- Integrite des pixels non testee.
- Hypothese : erreur CSI/DCMIPP avant la premiere demande.
- Cause racine non determinee.
- Statut : compile et teste STM32, non valide.
- Suite : diagnostic ErrorCode et registres DCMIPP.

### 2026-11-24 - P03-A : diagnostic CSI au demarrage

- Premier test STM32 : Pipe1 maintenu a 30 FPS.
- HAL ErrorCode : 32768 (0x8000).
- Type : HAL_DCMIPP_CSI_ERROR_DPHY_CTRL.
- DCMIPP State : READY.
- Pipe1 : BUSY ; Pipe2 : READY.
- P2FCTCR : 0 ; snapshot_id : 0.
- Deux incidents CSI comptabilises.
- Aucune erreur AXI signalee dans le registre observe.
- Blocage logiciel avant la premiere demande de snapshot.
- Correction : separation des incidents DPHY CTRL initiaux.
- Historique CSI conserve dans des compteurs distincts.
- Verification des flags physiques CSI avant capture.
- Surveillance DPHY reactivee avant acquisition.
- Intervalle minimal sans incident : 100 ms.
- Les autres erreurs restent bloquantes.
- Aucun changement du linker ou des reglages camera.
- Cause physique CSI non determinee.
- Statut : correctif applique, compilation et test a effectuer.

### 2026-11-24 - P03 : validation STM32 et cloture

- P03-A execute sur cible ; nouveaux symboles observes. Console build P03-A non fournie.
- Pipe1 : 866 frames produites/lues, 28989 ms, FPS instantane 31 ; LCD fonctionnel confirme.
- 5 snapshots, 4 releases, 5 refus pendant HELD, 4 changements de CRC, 0 echec CRC, 0 erreur snapshot.
- Derniere detention complete : 5000 ms, 151 frames, 30.20 FPS ; derniere latence 49 ms.
- Dernier snapshot : id 5, timestamp 25875 ms, 800x480 RGB565, 768000 octets a 0x342E0000.
- CRC snapshot 5 en cours : 1116724303 ; CRC final snapshot 4 : 3491194965.
- Operateur : plafond, visage, mains, plafond pendant la session ; transitions non horodatees.
- Deux incidents CSI DPHY CTRL initiaux, dernier tick 278 ms ; cause physique non determinee. ErrorCode final 0.
- Statut : P03 validee STM32 sur une carte dans le perimetre teste ; cinquieme detention non terminee au releve.
- Limites : test court, CRC aux points de controle, pas de test exhaustif de fautes, test automatique encore actif.
- Rapport : docs/tests/CAMERA_SNAPSHOT_VALIDATION_2026-11-24.md.
- Quatre schemas SVG versionnes ; roadmap actualisee ; handoff P04 prepare.
- Publication : commit/push a effectuer par ce script et SHA distant a verifier ; aucune modification de main.
- Prochaine passe : P04, LiDAR simule.


### 2026-11-24 - P04 : audit et integration locale

- Branche exclusive : Note-de-Seb.
- Baseline distante : b1e0f50e58590a83695127733180bcb3713356bf.
- Documents de reference, sources LiDAR et configurations CubeIDE audites.
- Statistiques deja implementees et appelees dans le firmware LiDAR historique.
- Quatre scenes identifiees ; dimensions 54 x 42, distances en mm.
- Valeur invalide : 0xFFFF ; moyenne entiere sur valeurs valides.
- Six configurations de test, seed 1234 reinitialisee avant chaque generation.
- Statistiques attendues et empreintes des matrices calculees independamment.
- Scene bruitee : 79 zones invalides imposees, en plus des invalidations aleatoires.
- Simulateur original lie au projet camera sans duplication.
- Test automatique P03 isole par constante de compilation reversible.
- Reglages camera, LCD, linker et CameraSource conserves.
- Diagnostic g_p04 et mesure camera sur une fenetre de 5 secondes.
- Statut : implementation locale, compilation et tests STM32 non effectues.
- SVG et rapport de validation a produire apres les mesures.
- Prochaine action : compilation Debug et validation materielle.


### 2026-11-24 - P04-A : correction du chemin include

- Premier build Debug : 5 erreurs, 0 avertissement.
- Source lidar_simulator.c correctement liee et appelee par GCC.
- Header lidar_simulator.h non resolu dans deux unites de compilation.
- Correction du chemin include via ProjDirPath.
- Configuration Debug et Release actualisee.
- Aucun changement du simulateur ou des reglages materiels.
- Statut : correctif local ; recompilation requise.


### 2026-11-24 - P04 : compilation Debug reussie

- Correction P04-A appliquee.
- Build Debug a 16:07:10 : 0 erreur, 0 avertissement.
- Dernier build incremental : 194 ms.
- Text : 152388 octets.
- Data : 324 octets.
- BSS : 793076 octets.
- Total : 945788 octets.
- Reservation snapshot conservee : 768000 octets.
- LidarFrame : 15880 octets.
- Adresse LidarFrame : 0x341C06A0.
- Placement memoire verifie dans le fichier map.
- Statut : implemente et compile.
- Validation STM32 non encore effectuee.
- Prochaine action : six tests LiDAR et non-regression camera.

### 2026-11-24 - P04 : validation STM32 et cloture documentaire

- Build Debug valide a 16:07:10, 0 erreur, 0 avertissement ; invocation incrementale 194 ms.
- Map : LidarFrame 0x341C06A0 a 0x341C44A8 (fin exclusive), 15880 octets ; snapshot 0x342E0000 a 0x3439B800, 768000 octets ; diagnostic 320 octets a 0x341C44B0.
- Symboles LidarSim_Generate, LidarSim_ComputeStats, P04Hash et P04TestProcess presents.
- STM32 : completed=6, pass_mask=63, failures=0, done=1 ; dimensions 54 x 42, frame 15880 octets.
- Six configurations conformes aux references, statistiques et empreintes ; valeurs individuelles attestees par le masque, durees individuelles non relevees.
- start_tick=1000, last_tick=3507 ; 151 frames produites et lues en 5000 ms, 30.20 FPS ; g_camera_fps=30, hdcmipp.ErrorCode=0 au releve.
- Operateur confirme le LCD affichant la scene camera.
- Statut : P04 validee STM32 sur une carte dans le perimetre teste ; charge LiDAR ponctuelle, pas de test de debit continu ni de capteur physique.
- Test automatique snapshot P03 desactive, API et reservation conservees ; integrite snapshot non retestee pendant P04.
- Rapport : docs/tests/LIDAR_SIMULATION_VALIDATION_2026-11-24.md.
- Quatre SVG P04 : matrice, scenes theoriques, pipeline et validation.
- Roadmap actualisee ; handoff P05 prepare. P05 non commencee.
- Publication : commit/push et verification de Note-de-Seb par le script de cloture ; aucun changement main.

### 2026-11-24 - P05 : audit, conception et implementation locale

- Branche exclusive : Note-de-Seb ; baseline locale et distante controlee : 8cc3395930605d7e1495617f92d618ee4984723f.
- Roadmap, journal, rapport P04, handoff P05, protocole HTML, sources LiDAR, points d'integration et configurations CubeIDE examines sur GitHub.
- Huit SVG P03/P04 identifies comme references visuelles.
- Types communs deplaces sans changement de layout ; statistiques originales deplacees ; fonction historique conservee comme wrapper.
- Traitements independants du simulateur et de HAL ; seuil strict, invalide 0xFFFF exclu, moyenne tronquee, centroide non pondere et indexe a partir de zero.
- References recalculees en JS et Python : obstacle central 324 zones a 1200 mm ; gradient 251 zones, min 1137, moyenne 1333, sommes row/col 4795/636 au seuil 1500 mm.
- Diagnostic P04 conserve et augmente de 19 essais P05, dont 5 essais synthetiques ; un seul buffer LiDAR, aucun consommateur snapshot ajoute.
- Les six references P04 et les six comparaisons de compatibilite sont verifiees par le nouveau diagnostic lors de son execution future.
- Firmware LiDAR historique : main.c conserve ; compilation du nouveau fichier via Core ; firmware camera : source liee supplementaire dans .project.
- Statut : implementation locale ; compilation et validation STM32 non effectuees ; SVG P05 et rapport final a produire.
- Prochaine action : compiler les deux projets Debug, verifier le map camera et tester la camera sur STM32.

### 2026-11-24 - P05 : tests STM32 conformes

- Nouveau firmware P05 execute sur STM32.
- Detection : 19/19 essais conformes.
- P05 pass_mask=524287, failures=0, done=1.
- Compatibilite statistiques historiques : mask=63.
- Non-regression P04 : 6/6, mask=63, failures=0.
- Dimensions : 54x42 ; frame : 15880 octets.
- Camera : 151 frames produites et 151 lues en 5000 ms.
- Cadence mesuree : 30.20 FPS, identique a P04.
- hdcmipp.ErrorCode=0 au releve precedent.
- Diagnostic automatique termine.
- Confirmation LCD et verification MAP a finaliser.
- Compilation du firmware LiDAR historique a effectuer.
- Statut : tests STM32 P05 conformes ; cloture en attente.

### 2026-11-24 - P05 : confirmation LCD

- Flux camera affiche correctement sur le LCD.
- Confirmation visuelle par l'operateur.
- Non-regression camera et LCD confirmee.
- Controle memoire MAP en cours.

### 2026-11-24 - P05 : clôture technique

- Détection STM32 19/19 ; régression P04 6/6 ; compatibilité 6/6 ; aucun échec.
- Camera 151/151 en 5000 ms ; 30,20 FPS ; LCD confirmé ; ErrorCode=0 au relevé.
- Debug caméra : 0 erreur, 0 avertissement ; Text=153980, Data=324, BSS=793780.
- Debug historique : 0 erreur, 1 avertissement ancien (variable test inutilisée) ; ELF/BIN générés, non flashé.
- Premier script de clôture interrompu : deux occurrences .camera_snapshot dans le MAP ; aucun commit ni push. Parseur corrigé pour accepter seulement les occurrences de même adresse et taille.
- MAP de clôture : tailles et absence de chevauchement contrôlées ; snapshot à 0x342E0000 conservé.
- Statut : P05 validée STM32 dans le périmètre testé ; pas de validation Release ni de débit continu ni de snapshot retesté.
- Rapport : docs/tests/LIDAR_PROCESSING_VALIDATION_2026-11-24.md ; quatre SVG P05 ; roadmap mise à jour.
- Handoff P06 créé ; P06 non commencée. Publication à vérifier après push.


### 2026-11-24 - P06-A : audit initial et conception

- Branche exclusive : Note-de-Seb.
- Baseline locale et distante verifiee :
  b45ea57a3a5623b618dd0c337f42954bbcfb6ee4.
- Roadmap, journal, rapports P04/P05, handoff P06, protocole HTML et sources LiDAR examines.
- CameraSource et configurations CubeIDE des deux firmwares examines.
- Douze SVG P03/P04/P05 inventories et examines comme references graphiques.
- Index Git vide ; configurations locales residuelles controlees et exclues des futurs commits.
- Architecture retenue : interface LidarSource independante de HAL et du backend, injection par fonctions.
- Backend simulation : reutilisation de LidarSim sans duplication du generateur.
- Propriete exclusive en lecture seule, acquisition et liberation explicites.
- Format LidarFrame conserve ; identifiant porte par les metadonnees de source.
- Reutilisation du buffer P04 apres le diagnostic P04/P05 ; aucune seconde frame allouee.
- Cadence cible : 100 ms, soit 10 Hz ; pas de rattrapage en rafale.
- Tests de propriete et erreurs separes de la mesure de cadence.
- Validation prevue : P04/P05, API P06, 100 intervalles LiDAR, coexistence camera et LCD.
- P03_SNAPSHOT_TEST_ENABLED reste a zero.
- Statut : audit et conception ; implementation, compilation et validation STM32 non effectuees.
- Prochaine action : implementation locale des modules et du diagnostic P06.

### 2026-11-24 - P06-B : implementation locale

- Modules lidar_source.h/.c et lidar_source_sim.h/.c crees dans Core ; liens CubeIDE camera ajoutes.
- Interface generique sans HAL : backend init/start/poll, etats UNINITIALIZED/IDLE/ACQUIRING/READY/HELD/ERROR, metadonnees et compteurs.
- Un seul buffer g_p04_frame : P04/P05 executes sans modification puis transfert au diagnostic P06 et au flux normal.
- Backend simulation synchrone, periodique a 100 ms ; slots manques comptes, sans rattrapage en rafale ; PRNG conserve entre frames.
- 22 controles API avec ticks artificiels, backend de test injecte et verification de l'empreinte du buffer ; aucun second buffer.
- Mesure cible : 101 frames/100 intervalles, coexistence camera, statistiques et detection sur chaque frame. Resultats figes puis flux maintenu.
- References theoriques sequence bruitee seed 1234 : hash 3424744673 puis 4087108911 ; 112 puis 113 zones invalides.
- Test de coherence hors cible par GCC natif avec doublures du simulateur, traitements, horloge et camera : 22/22 controles, 101 frames, 10000 ms artificielles ; ne constitue pas une compilation des firmwares ni une mesure materielle.
- Firmware historique main.c et chemins d'inclusion Debug/Release inchanges ; nouvelles sources dans Core.
- P03_SNAPSHOT_TEST_ENABLED reste a zero ; aucun nouveau consommateur snapshot.
- Statut : implementation locale, compilation CubeIDE et validation STM32 non effectuees.
- Prochaine action : rafraichir les deux projets, compiler Debug, verifier le MAP puis lancer le test materiel camera.


### 2026-11-24 - P06-C : compilation et controle MAP

- Implementation locale P06-B reussie.
- Debug camera : 16:52:21, 0 erreur, 0 avertissement.
- Text=158900, Data=344, BSS=793992, total=953236 octets.
- Surcout P05 : Text +4920, Data +20, BSS +212 octets.
- Debug historique : 16:52:46, 0 erreur, 1 avertissement ancien.
- Text=15028, Data=12, BSS=18500, total=33540 octets.
- Nouveaux objets LidarSource presents dans les deux builds.
- Symboles LidarSource controles dans le MAP camera.
- Buffer LiDAR : 0x341C06B0, 15880 octets.
- Contexte source : 0x341C48C0, 44 octets.
- Diagnostic P06 : 0x341C4910, 136 octets.
- Snapshot : 0x342E0000, 768000 octets.
- Aucun chevauchement entre les regions controlees.
- Statut : implemente et compile ; validation STM32 non effectuee.
- Prochaine action : session materielle P06.

### 2026-11-24 - P06-E : validation et cloture

- Validation STM32 : 22/22 API, mask 4194303, aucun echec ; ticks artificiels pour les essais de contrat et doublure pour les chemins asynchrones.
- P04 6/6, mask 63, 150/150 frames camera en 5000 ms ; P05 19/19, mask 524287 ; compatibilite 6/6, mask 63.
- Mesure P06 : 101 frames, 100 intervalles, min=max=100 ms ; timestamps 6101/16101 ms, ID 1/101 ; 10,00 Hz ; erreurs ID/timestamp/donnees=0.
- Hash 3424744673 puis 4087108911 ; generation max 2 ms, traitement 0 ms a la resolution de 1 ms.
- Camera : 302/302 en 10000 ms, 30,20 FPS ; ErrorCode=0 au releve anterieur ; LCD explicitement confirme.
- Flux apres diagnostic : 137/137/137 ; aucun slot manque ni erreur backend ; derniere frame 2160 valides, 108 invalides, min/moy/max 2450/2499/2550 mm, obstacle non detecte au seuil 1500 mm.
- Premiere session Expressions en erreur ; relance sur le bon firmware reussie ; pas de cause racine demontrée.
- MAP P06-C conforme ; buffer unique 0x341C06B0, snapshot reserve 0x342E0000 ; Debug des deux firmwares compile, historique non flashe.
- Statut : P06 validee STM32 dans le perimetre teste ; capteur physique absent des essais, Release et endurance non validees, snapshot non reteste.
- Rapport : docs/tests/LIDAR_SOURCE_VALIDATION_2026-11-24.md ; quatre SVG P06 ; roadmap actualisee ; handoff P07 cree.
- Publication : commit/push exclusifs Note-de-Seb a verifier apres push ; P07 non commencee.
