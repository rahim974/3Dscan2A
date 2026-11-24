# Handoff P07 — Bring-up VL53L9CX physique

## Baseline et règles

- Dépôt `rahim974/3Dscan2A`, branche exclusive `Note-de-Seb`. Ne jamais modifier main.
- Vérifier directement HEAD distant et local de clôture P06 ; le SHA ne peut figurer dans son propre commit.
- Windows 11, PowerShell, STM32CubeIDE, STM32CubeN6 v1.2.0 ; dépôt D:/Projet_2A, workspace D:/STM32_Workspace_Projet2A ; STM32N6570-DK et IMX335. Une carte pour les tests courants, seconde en secours.
- Lire roadmap, journal, rapports P04/P05/P06, ce handoff, protocole HTML historique, sources réelles lidar_types, lidar_processing, lidar_simulator, lidar_source, lidar_source_sim, CameraSource, main.c et configurations des deux firmwares.

## État P06 validé

Interface indépendante de HAL et du backend : init/start/poll, états UNINITIALIZED/IDLE/ACQUIRING/READY/HELD/ERROR, consommateur unique, vue const et libération explicite. Un seul buffer 15880 o réutilisé après P04/P05. P06 : 22/22 contrôles API, 101 frames/100 intervalles de 100 ms, 10,00 Hz, 302 frames caméra produites/lues en 10000 ms (30,20 FPS), zéro slot manqué/erreur backend, hash des deux premières frames conforme. Flux continu : 137/137/137 au relevé. P04 6/6, P05 19/19, compatibilité 6/6. LCD confirmé ; ErrorCode=0 au relevé antérieur. Debug caméra 0 erreur/0 avertissement ; historique 0 erreur/1 avertissement ancien, non flashé. MAP vérifié : buffer 0x341C06B0, snapshot 0x342E0000 (768000 o) ; aucun chevauchement. Deux fichiers language.settings.xml locaux exclus. Voir le [rapport P06](../tests/LIDAR_SOURCE_VALIDATION_2026-11-24.md).

## Mission P07 — prévue, non commencée

- Vérifier la disponibilité et la référence exacte du module physique, sa documentation constructeur et le matériel d'interface disponible. Si absent, statut bloqué matériel, sans revendication de validation physique.
- Confirmer l'interface électrique et protocolaire (I3C envisagé mais non confirmé), alimentation, contraintes de raccordement et configuration CubeIDE avant toute intégration.
- Auditer le format natif des mesures, leur résolution et leurs métadonnées ; documenter les divergences éventuelles avec le format simulé 54×42. Ne pas inventer de zones ni prétendre que la résolution simulée est une spécification du capteur.
- Intégrer un backend physique conforme à LidarBackendOps ; définir timestamp, disponibilité, gestion des erreurs, délais, propriété et éventuelles contraintes DMA/cache. FAILED doit garantir l'arrêt des écritures ; pas d'écrasement READY/HELD.
- Préserver le backend simulation, les traitements validés, le pipeline caméra/LCD et les deux builds ; conserver les diagnostics de référence. Ne pas ajouter de consommateur snapshot concurrent.
- Valider sur STM32 identification, acquisition réelle répétée, données invalides, cadence mesurée, erreurs et coexistence caméra/LCD ; vérifier le MAP ; documenter les limites et distinguer mesures physiques de simulation.
- Journaliser chaque jalon ; rapport et SVG versionnés ; commit/push exclusifs sur Note-de-Seb et vérification distante avant handoff P08.

Hors périmètre : synchronisation RGB-D, pose, reconstruction 3D, IA. Ne pas anticiper P08. Aucun driver réel ni test physique réalisé en P06.
