HANDOFF — Projet scanner 3D — branche Note-de-Seb

État de reprise au 10 novembre 2026



============================================================

0\. RÈGLE ABSOLUE DE REPRISE

============================================================



Ce projet doit être repris exclusivement sur la branche :



Note-de-Seb



Dépôt GitHub :



https://github.com/rahim974/3Dscan2A



Ne pas travailler sur main.

Ne pas merger vers main sans demande explicite de l'utilisateur.

Ne pas pousser sur une autre branche sans accord explicite.



Dernier baseline firmware poussé et validé :



b66709b feat: add STM32 camera and LiDAR simulation baseline



Ce commit contient les projets STM32 locaux réellement utilisés jusqu'à cette étape :



firmware/camera\_imx335/

firmware/lidar\_sim/



La branche Note-de-Seb constitue désormais la source de vérité du projet.





============================================================

1\. OBJECTIF GLOBAL DU PROJET

============================================================



Le projet vise la réalisation d'un scanner 3D portable basé sur une STM32N6570-DK avec :



\- une caméra RGB Sony IMX335 ;

\- un capteur de profondeur VL53L9CX ;

\- reconstruction géométrique ;

\- future estimation de pose ;

\- future fusion caméra / LiDAR ;

\- future IA embarquée ;

\- restitution 2D / plan sémantique.



Architecture conceptuelle actuelle :



&#x20;                  IMX335

&#x20;                    |

&#x20;               MIPI CSI-2

&#x20;                    |

&#x20;                    v

&#x20;             STM32N6570-DK

&#x20;             +-------------+

&#x20;             | Cortex-M55  |

&#x20;             | Neural-ART  |

&#x20;             | DCMIPP/ISP  |

&#x20;             +------+------+

&#x20;                    |

&#x20;       +------------+------------+

&#x20;       |                         |

&#x20;  vision RGB                géométrie

&#x20;       |                         |

&#x20;       |                     VL53L9CX

&#x20;       |                         |

&#x20;       |                        I3C

&#x20;       |                         |

&#x20;       +------------+------------+

&#x20;                    |

&#x20;             synchronisation

&#x20;                    |

&#x20;                 pose

&#x20;                    |

&#x20;             fusion RGB-D

&#x20;                    |

&#x20;            reconstruction

&#x20;                    |

&#x20;             plan / carte 2D



Principe architectural retenu :



LiDAR = géométrie

Caméra = sémantique / vision



L'IA n'est pas prioritaire actuellement.



Ordre recommandé :



caméra

→ LiDAR

→ timestamps

→ pose

→ géométrie

→ fusion

→ IA





============================================================

2\. ÉVOLUTION DE L'ARCHITECTURE

============================================================



Le document seance1.md contient encore une partie de l'ancienne architecture, qui envisageait notamment :



\- scanner rotatif ;

\- moteur ;

\- encodeur ;

\- rotation discrète ou continue.



Cette architecture est historique.



La direction actuelle est plutôt :



scanner portable / tenu à la main



La version V1 doit privilégier une approche :



Stop-and-Scan



c'est-à-dire :



déplacement

→ arrêt

→ capture caméra

→ capture LiDAR

→ timestamp

→ association à une pose

→ déplacement suivant



Cela réduit les difficultés de :



\- motion blur ;

\- synchronisation continue ;

\- interpolation temporelle ;

\- deskew LiDAR ;

\- reconstruction pendant mouvement.





============================================================

3\. ÉTAT GITHUB

============================================================



Dépôt :



rahim974/3Dscan2A



Branche de travail :



Note-de-Seb



Branche distante :



origin/Note-de-Seb



Baseline firmware :



b66709b



Message :



feat: add STM32 camera and LiDAR simulation baseline



La branche contient actuellement notamment :



.gitignore

DS\_vl53l9cx.pdf

README.md

seance1.md

seance2.zip



Protocoles testes et simulations/

├── simulation\_lidar.html

└── test\_camera.html



firmware/

├── camera\_imx335/

└── lidar\_sim/



Les dossiers de build sont ignorés :



Debug/

Release/



Les artefacts générés suivants sont également ignorés :



\*.elf

\*.bin

\*.hex

\*.map

\*.list

\*.o

\*.d

\*.su



Les fichiers CubeIDE nécessaires à la reproductibilité sont conservés :



.project

.mxproject

.ioc

.settings/

linker scripts

Drivers/

CMSIS/

HAL/

BSP/

Middlewares/





============================================================

4\. ENVIRONNEMENT LOCAL

============================================================



Dépôt Git local :



D:\\Projet\_2A



Ancien dossier local temporaire :



D:\\Projet\_2A\_local



Projet caméra d'origine :



C:\\STM32\\DCMIPP\_ContinuousMode



Projet LiDAR simulé d'origine :



C:\\STM32\\test\_projet\_v3



Attention :



ne pas utiliser C:\\STM32\\.metadata\\... comme source de projet.



Il s'agit de métadonnées internes Eclipse / CubeIDE.





============================================================

5\. MATÉRIEL

============================================================



Carte :



STM32N6570-DK



MCU :



STM32N657X0H3Q



Éléments utiles :



\- Cortex-M55 ;

\- Neural-ART NPU ;

\- environ 4,2 Mo de SRAM interne ;

\- mémoires externes sur la Discovery ;

\- MIPI CSI-2 ;

\- DCMIPP ;

\- ISP ;

\- LTDC ;

\- interfaces I3C disponibles.





============================================================

6\. CAMÉRA

============================================================



Caméra :



B-CAMS-IMX / MB1854



Capteur :



Sony IMX335



Interface :



MIPI CSI-2



Le projet caméra actuel est dérivé de l'exemple ST :



DCMIPP\_ContinuousMode



Version STM32CubeN6 utilisée :



v1.2.0



Projet CubeIDE principal :



DCMIPP\_ContinuousMode\_FSBL



Projet Git :



firmware/camera\_imx335/





============================================================

7\. LIDAR

============================================================



Capteur cible :



VL53L9CX



Résolution utilisée :



54 × 42 zones



Soit :



2268 zones



Champ de vision approximatif utilisé :



55° × 42°



Données prévues :



\- Range ;

\- Amplitude ;

\- Ambient ;

\- DSS.



Le DSS réel est compacté sur 4 bits.



Le simulateur utilise actuellement uint8\_t par zone pour simplifier le traitement.





============================================================

8\. DÉCISION D'INTERFACE CAPTEURS

============================================================



Décision actuelle :



IMX335    → MIPI CSI-2

VL53L9CX  → I3C



Motif :



la caméra utilise le CSI-2 disponible pour l'acquisition image.



Le VL53L9CX doit donc être intégré via une autre interface.



I3C est actuellement retenu.



Cette décision doit être confirmée lors du bring-up réel du VL53L9CX.





============================================================

9\. PROJET LIDAR SIMULÉ

============================================================



Projet source :



C:\\STM32\\test\_projet\_v3



Copie Git :



firmware/lidar\_sim/



Le simulateur a réellement été compilé et exécuté sur la STM32N6570-DK.





============================================================

10\. STRUCTURE LIDARFRAME

============================================================



Structure logique utilisée :



typedef struct

{

&#x20;   uint32\_t timestamp\_ms;



&#x20;   uint16\_t range\_mm\[42]\[54];

&#x20;   uint16\_t amplitude\[42]\[54];

&#x20;   uint16\_t ambient\[42]\[54];

&#x20;   uint8\_t  dss\[42]\[54];



} LidarFrame;



Convention :



range\_mm\[row]\[column]



Donc :



42 lignes

54 colonnes



Nombre total :



2268 zones



La structure est globale/statique afin d'éviter une grosse allocation sur la stack.





============================================================

11\. TAILLE APPROXIMATIVE D'UNE LIDARFRAME

============================================================



Calcul :



range:

42 × 54 × 2 = 4536 octets



amplitude:

42 × 54 × 2 = 4536 octets



ambient:

42 × 54 × 2 = 4536 octets



dss:

42 × 54 × 1 = 2268 octets



timestamp:

4 octets



Total :



15880 octets environ



Soit environ :



15,9 kB





============================================================

12\. SIMULATEUR LIDAR

============================================================



Fichiers :



firmware/lidar\_sim/FSBL/Core/Inc/lidar\_simulator.h

firmware/lidar\_sim/FSBL/Core/Src/lidar\_simulator.c



Scènes développées :



LIDAR\_SCENE\_FLAT\_WALL

LIDAR\_SCENE\_CENTRAL\_OBSTACLE

LIDAR\_SCENE\_ROOM\_CORNER

LIDAR\_SCENE\_NOISY\_INVALID





============================================================

13\. SCÈNE FLAT\_WALL

============================================================



Principe :



toutes les zones ≈ 2000 mm



Utilité :



\- valider la matrice ;

\- valider les statistiques ;

\- futur test profondeur → XYZ ;

\- futur test plan.





============================================================

14\. SCÈNE CENTRAL\_OBSTACLE

============================================================



Fond :



4000 mm



Obstacle :



1200 mm



Zone obstacle :



rows 12 à 29

columns 18 à 35



Dimensions :



18 × 18



Nombre de zones obstacle :



324





============================================================

15\. SCÈNE ROOM\_CORNER

============================================================



Scène synthétique utilisant un gradient de profondeur afin de simuler grossièrement un coin de pièce.



Elle est destinée aux futurs tests géométriques.





============================================================

16\. SCÈNE NOISY\_INVALID

============================================================



Scène contenant :



\- bruit pseudo-aléatoire déterministe ;

\- trous artificiels ;

\- valeurs invalides ;

\- profondeur autour de 2500 mm.



Valeur interne utilisée pour une mesure invalide :



\#define LIDAR\_INVALID\_MM 0xFFFFu





============================================================

17\. VALIDATION RÉELLE DU SIMULATEUR LIDAR

============================================================



Le simulateur a été exécuté sur la STM32N6570-DK.



Valeurs observées dans CubeIDE :



g\_lidar\_frame.range\_mm\[0]\[0] = 4000



et :



g\_lidar\_frame.range\_mm\[20]\[25] = 1200



Ces valeurs correspondent à la scène CENTRAL\_OBSTACLE.



Conclusion validée :



simulateur

→ compilation STM32

→ exécution réelle

→ LidarFrame remplie correctement





============================================================

18\. STATISTIQUES LIDAR

============================================================



Une structure de statistiques est prévue / partiellement implémentée :



LidarFrameStats



Champs attendus :



min\_mm

max\_mm

mean\_mm

invalid\_count

valid\_count



Pour la scène CENTRAL\_OBSTACLE, valeurs de référence :



min\_mm        = 1200

max\_mm        = 4000

mean\_mm       = 3600

invalid\_count = 0

valid\_count   = 2268



Le protocole détaillé est dans :



Protocoles testes et simulations/simulation\_lidar.html





============================================================

19\. DÉTECTION D'OBSTACLE LIDAR — VALEURS ATTENDUES

============================================================



Avec un seuil de :



1500 mm



sur la scène CENTRAL\_OBSTACLE, les valeurs attendues sont :



detected       = 1

close\_count    = 324

nearest\_mm     = 1200

mean\_close\_mm  = 1200

centroid\_row   = 20.5

centroid\_col   = 26.5



ATTENTION :



ces valeurs sont des résultats attendus du protocole.



Ne pas considérer cette étape comme complètement validée tant qu'elle n'a pas été exécutée et vérifiée sur la carte.





============================================================

20\. ARCHITECTURE LIDAR CIBLE

============================================================



Le simulateur ne doit pas rester couplé directement à main.c.



Architecture visée :



&#x20;            +------------------+

&#x20;            |   LidarSource    |

&#x20;            +--------+---------+

&#x20;                     |

&#x20;         +-----------+------------+

&#x20;         |                        |

&#x20;   simulateur actuel        VL53L9CX réel

&#x20;         |                        |

&#x20;         +-----------+------------+

&#x20;                     |

&#x20;                 LidarFrame

&#x20;                     |

&#x20;                     v

&#x20;              LidarProcessing



API cible :



bool LidarSource\_Init(void);

bool LidarSource\_GetFrame(LidarFrame \*frame);



Lors du passage du simulateur au capteur réel :



les traitements ne doivent pas être réécrits.



Seul le backend de source doit changer.





============================================================

21\. PROCHAINE ROADMAP LIDAR

============================================================



Ordre recommandé :



stats

→ détection obstacle

→ lidar\_types

→ lidar\_processing

→ LidarSource

→ flux de frames

→ export PC

→ depth → XYZ

→ nuage 3D

→ occupancy map 2D

→ pose

→ capteur réel





============================================================

22\. INCIDENT ST-LINK / JP2 RÉSOLU

============================================================



Au premier lancement caméra, erreur :



Target no device found



Error in initializing ST-LINK device.

Reason: No device found on target.



Cause identifiée :



configuration JP2



Correction utilisée :



JP2 → position 1-2



Après cette modification :



\- STM32 détectée ;

\- debug CubeIDE opérationnel ;

\- breakpoints opérationnels.



Ce point doit être conservé dans toute procédure de reproduction.





============================================================

23\. BRING-UP CAMÉRA VALIDÉ

============================================================



Breakpoints utilisés sur :



MX\_DCMIPP\_Init();



IMX335\_Probe(...);



ISP\_Init(...);



HAL\_DCMIPP\_CSI\_PIPE\_Start(...);



ISP\_Start(...);



Un breakpoint de sécurité a été placé dans :



Error\_Handler()



Résultat :



MX\_DCMIPP\_Init                OK

IMX335\_Probe                  OK

ISP\_Init                      OK

HAL\_DCMIPP\_CSI\_PIPE\_Start     OK

ISP\_Start                     OK

Error\_Handler                 jamais atteint



Conclusion validée :



IMX335 détectée

CSI OK

DCMIPP OK

ISP OK

pipeline démarré





============================================================

24\. FRAMEBUFFER CAMÉRA

============================================================



Adresse déterminée :



BUFFER\_ADDRESS = 0x34200000



Le DCMIPP utilise ce buffer.



Le LTDC utilise également :



pLayerCfg.FBStartAdress = BUFFER\_ADDRESS;



Chaîne validée :



IMX335

→ MIPI CSI-2

→ DCMIPP

→ ISP

→ framebuffer

→ LTDC

→ écran





============================================================

25\. RÉSOLUTION CAMÉRA

============================================================



Résolution utilisée :



800 × 480



Macros :



FRAME\_WIDTH  = 800

FRAME\_HEIGHT = 480





============================================================

26\. FORMAT PIXEL CAMÉRA

============================================================



Format LTDC :



RGB565



Donc :



16 bits / pixel

2 octets / pixel





============================================================

27\. TAILLE DU FRAMEBUFFER

============================================================



Calcul :



800 × 480 × 2



Résultat :



768000 octets



Soit :



750 KiB





============================================================

28\. PLAGE MÉMOIRE FRAMEBUFFER

============================================================



Début :



0x34200000



Fin incluse :



0x342BB7FF



Première adresse après buffer :



0x342BB800





============================================================

29\. VÉRIFICATION EXPÉRIMENTALE DU FRAMEBUFFER

============================================================



Dans STM32CubeIDE :



Window

→ Show View

→ Memory



Adresse inspectée :



0x34200000



Observation :



\- mémoire non nulle ;

\- données structurées ;

\- contenu change quand la scène devant la caméra change.



Conclusion :



de vraies données image arrivent dans le framebuffer.



Ce n'est pas seulement une initialisation réussie.





============================================================

30\. CALLBACK DE FIN DE FRAME

============================================================



Callback trouvé :



void HAL\_DCMIPP\_PIPE\_FrameEventCallback(

&#x20;   DCMIPP\_HandleTypeDef \*hdcmipp,

&#x20;   uint32\_t Pipe)

{

&#x20;   NbMainFrames++;

}



Compteur :



static \_\_IO uint32\_t NbMainFrames = 0;



Le compteur augmente continuellement.



Conclusion :



frame terminée

→ callback DCMIPP

→ NbMainFrames++



Le mécanisme de frame-ready est donc fonctionnel.





============================================================

31\. MESURE DU FRAMERATE

============================================================



Variables ajoutées :



static uint32\_t g\_last\_fps\_tick = 0;

static uint32\_t g\_last\_frame\_count = 0;

static uint32\_t g\_camera\_fps = 0;



Code dans le vrai while(1) de main() :



if ((HAL\_GetTick() - g\_last\_fps\_tick) >= 1000u)

{

&#x20;   g\_camera\_fps = NbMainFrames - g\_last\_frame\_count;



&#x20;   g\_last\_frame\_count = NbMainFrames;

&#x20;   g\_last\_fps\_tick = HAL\_GetTick();

}



Résultat observé :



g\_camera\_fps ≈ 30



Donc :



≈ 30 FPS



Attention historique :



ce code avait d'abord été placé par erreur dans le while(1) de Error\_Handler().



Il a ensuite été déplacé correctement dans main().





============================================================

32\. CAMERAFRAME

============================================================



Fichier :



firmware/camera\_imx335/STM32CubeIDE/FSBL/Application/User/camera\_types.h



Contenu actuel :



\#ifndef CAMERA\_TYPES\_H

\#define CAMERA\_TYPES\_H



\#include <stdint.h>



typedef enum

{

&#x20;   CAMERA\_PIXEL\_FORMAT\_UNKNOWN = 0,

&#x20;   CAMERA\_PIXEL\_FORMAT\_RGB565,

&#x20;   CAMERA\_PIXEL\_FORMAT\_RGB888,

&#x20;   CAMERA\_PIXEL\_FORMAT\_RAW10

} CameraPixelFormat;



typedef struct

{

&#x20;   uint32\_t frame\_id;

&#x20;   uint32\_t timestamp\_ms;



&#x20;   uint16\_t width;

&#x20;   uint16\_t height;



&#x20;   CameraPixelFormat format;



&#x20;   uint8\_t \*data;

&#x20;   uint32\_t data\_size\_bytes;



} CameraFrame;



\#endif /\* CAMERA\_TYPES\_H \*/





============================================================

33\. POURQUOI CAMERAFRAME CONTIENT UN POINTEUR

============================================================



La structure contient :



uint8\_t \*data;



et non un tableau de 768000 octets.



Motif :



éviter de copier le framebuffer.



CameraFrame décrit la frame et référence directement les pixels.





============================================================

34\. PROBLÈME D'INCLUDE CAMERAFRAME RÉSOLU

============================================================



Erreur initiale :



fatal error: camera\_types.h: No such file or directory



Cause :



le dossier contenant camera\_types.h n'était pas dans les include paths.



Correction STM32CubeIDE :



Project

→ Properties

→ C/C++ Build

→ Settings

→ MCU GCC Compiler

→ Include paths



Ajout du dossier correspondant à :



Application/User



Résultat :



build OK





============================================================

35\. FICHIER MAIN.C RÉELLEMENT COMPILÉ

============================================================



Le fichier source principal réellement compilé se trouve dans :



firmware/camera\_imx335/FSBL/Src/main.c



Même si CubeIDE affiche parfois une arborescence virtuelle / liée sous :



Application/User



Ne pas confondre :



vue logique CubeIDE



avec :



chemin physique source





============================================================

36\. CAMERAFRAME BRANCHÉE SUR LA CAMÉRA RÉELLE

============================================================



Variable globale :



static CameraFrame g\_camera\_frame;



Initialisation ajoutée :



g\_camera\_frame.frame\_id = NbMainFrames;

g\_camera\_frame.timestamp\_ms = HAL\_GetTick();



g\_camera\_frame.width = FRAME\_WIDTH;

g\_camera\_frame.height = FRAME\_HEIGHT;



g\_camera\_frame.format = CAMERA\_PIXEL\_FORMAT\_RGB565;



g\_camera\_frame.data = (uint8\_t \*)BUFFER\_ADDRESS;



g\_camera\_frame.data\_size\_bytes =

&#x20;   FRAME\_WIDTH \* FRAME\_HEIGHT \* 2u;





============================================================

37\. VALEURS CAMERAFRAME VALIDÉES DANS LE DEBUGGER

============================================================



Valeurs observées :



g\_camera\_frame.frame\_id        = 0

g\_camera\_frame.timestamp\_ms    ≈ 298

g\_camera\_frame.width           = 800

g\_camera\_frame.height          = 480

g\_camera\_frame.format          = CAMERA\_PIXEL\_FORMAT\_RGB565

g\_camera\_frame.data            = 0x34200000

g\_camera\_frame.data\_size\_bytes = 768000



Toutes les métadonnées statiques sont correctes.



Le frame\_id = 0 est normal à ce stade.



La structure est initialisée juste après le démarrage du pipeline et n'est pas encore actualisée à chaque frame.





============================================================

38\. POINT EXACT DE REPRISE CAMÉRA

============================================================



Le baseline Git actuel contient encore le callback suivant :



void HAL\_DCMIPP\_PIPE\_FrameEventCallback(

&#x20;   DCMIPP\_HandleTypeDef \*hdcmipp,

&#x20;   uint32\_t Pipe)

{

&#x20;   NbMainFrames++;

}



La prochaine modification prévue est :



void HAL\_DCMIPP\_PIPE\_FrameEventCallback(

&#x20;   DCMIPP\_HandleTypeDef \*hdcmipp,

&#x20;   uint32\_t Pipe)

{

&#x20;   NbMainFrames++;



&#x20;   g\_camera\_frame.frame\_id = NbMainFrames;

&#x20;   g\_camera\_frame.timestamp\_ms = HAL\_GetTick();

}



IMPORTANT :



cette modification a été proposée mais n'a PAS encore été confirmée comme :



compilée

\+

flashée/debuggée

\+

validée



Ne pas la considérer comme terminée.





============================================================

39\. TEST À FAIRE APRÈS MODIFICATION DU CALLBACK

============================================================



Dans CubeIDE Expressions :



NbMainFrames

g\_camera\_frame.frame\_id

g\_camera\_frame.timestamp\_ms



Résultat attendu :



g\_camera\_frame.frame\_id ≈ NbMainFrames



et :



timestamp\_ms augmente à chaque frame



À environ 30 FPS :



intervalle théorique ≈ 33 ms





============================================================

40\. ATTENTION ISR / CALLBACK

============================================================



HAL\_DCMIPP\_PIPE\_FrameEventCallback() est exécuté dans un contexte événementiel / interruption.



Pour une première validation, la mise à jour de :



frame\_id

timestamp



est acceptable.



Mais pour une architecture plus propre, éviter à terme de faire trop de traitement lourd dans le callback.



Approche future possible :



callback :

\- incrémente compteur

\- pose flag frame\_ready

\- éventuellement sauvegarde timestamp minimal



main :

\- consomme le flag

\- met à jour la structure complète





============================================================

41\. ÉTAPE SUIVANTE : CAMERASOURCE

============================================================



Après validation de la frame dynamique, créer :



camera\_source.h

camera\_source.c



API cible possible :



bool CameraSource\_Init(void);

bool CameraSource\_StartContinuous(void);

bool CameraSource\_Stop(void);

bool CameraSource\_IsFrameReady(void);

bool CameraSource\_GetLatestFrame(CameraFrame \*frame);





============================================================

42\. OBJECTIF DE CAMERASOURCE

============================================================



Le futur main.c ne doit plus connaître directement :



IMX335\_Probe

DCMIPP

ISP

BUFFER\_ADDRESS

NbMainFrames



Le code applicatif doit manipuler seulement :



CameraFrame



et :



CameraSource\_\*





============================================================

43\. FUTUR MODE SNAPSHOT

============================================================



Le projet actuel utilise :



DCMIPP\_ContinuousMode



Ce mode est utilisé pour :



\- comprendre la caméra ;

\- valider le pipeline ;

\- mesurer les FPS ;

\- valider le framebuffer.



Pour le scanner portable V1, un mode snapshot sera probablement plus adapté.



Logique :



demande capture

→ attente frame complète

→ CameraFrame prête

→ LiDAR

→ association temporelle





============================================================

44\. STRATÉGIE STOP-AND-SCAN

============================================================



La V1 recommandée :



move

↓

stop

↓

camera capture

↓

LiDAR capture

↓

pose

↓

fusion

↓

move



Avantages :



\- synchronisation simple ;

\- réduction des erreurs temporelles ;

\- moins de flou ;

\- reconstruction plus simple ;

\- meilleur debug.





============================================================

45\. FUTURE STRUCTURE POSE

============================================================



Structure envisagée :



typedef struct

{

&#x20;   float x;

&#x20;   float y;

&#x20;   float z;



&#x20;   float roll;

&#x20;   float pitch;

&#x20;   float yaw;



&#x20;   uint32\_t timestamp\_ms;



} Pose;





============================================================

46\. FUTURE SYNCHRONISATION

============================================================



Les éléments suivants devront être synchronisés :



CameraFrame.timestamp\_ms

LidarFrame.timestamp\_ms

Pose.timestamp\_ms



Le timestamp doit être documenté précisément :



capture demandée

ou

frame terminée



Il faudra conserver une convention cohérente.





============================================================

47\. FUTURE STRUCTURE SENSORPAIR

============================================================



Structure possible :



typedef struct

{

&#x20;   CameraFrame camera;

&#x20;   LidarFrame lidar;



&#x20;   uint32\_t pair\_id;

&#x20;   uint32\_t delta\_t\_ms;



} SensorPair;



Objectif :



mesurer explicitement le décalage temporel entre les deux acquisitions.





============================================================

48\. IA / YOLO

============================================================



L'intégration IA est volontairement repoussée.



Ne pas commencer maintenant par :



YOLO

fine-tuning

segmentation

Neural-ART

optimisation modèle



Avant cela, il faut stabiliser :



caméra

LiDAR

timestamps

pose

reconstruction

fusion





============================================================

49\. FUTURE IA ENVISAGÉE

============================================================



Plus tard, modèles légers INT8 compatibles STM32N6.



Axes possibles :



détection

vs

segmentation



L'objectif final est de rattacher une classe sémantique à la géométrie reconstruite.



Exemples de classes potentielles :



table

chaise

personne

armoire

porte





============================================================

50\. DOCUMENTS DE RÉFÉRENCE PRÉSENTS

============================================================



seance1.md



Contient :



\- architecture initiale ;

\- objectifs ;

\- réflexion IA ;

\- LiDAR ;

\- calibration ;

\- métriques.



Attention :



certaines parties sont anciennes, notamment l'architecture rotative.





============================================================

51\. PROTOCOLE LIDAR

============================================================



Fichier :



Protocoles testes et simulations/simulation\_lidar.html



Contient notamment :



\- stats ;

\- détection ;

\- découplage ;

\- LidarSource ;

\- flux temporel ;

\- nouvelles scènes ;

\- export PC ;

\- profondeur → XYZ ;

\- occupancy map ;

\- pose ;

\- préparation du driver VL53L9CX ;

\- validation future du capteur réel.





============================================================

52\. PROTOCOLE CAMÉRA

============================================================



Fichier :



Protocoles testes et simulations/test\_camera.html



Contient notamment :



\- exemple ST ;

\- bring-up ;

\- DCMIPP ;

\- ISP ;

\- framebuffer ;

\- mode continu ;

\- CameraFrame ;

\- CameraSource ;

\- timestamps ;

\- benchmark ;

\- snapshot ;

\- future fusion caméra / LiDAR.





============================================================

53\. CE QUI EST VALIDÉ EXPÉRIMENTALEMENT

============================================================



Les éléments suivants sont considérés validés :



STM32N6570-DK debug opérationnel

JP2 = 1-2



Caméra :



IMX335 détectée

MX\_DCMIPP\_Init OK

IMX335\_Probe OK

ISP\_Init OK

HAL\_DCMIPP\_CSI\_PIPE\_Start OK

ISP\_Start OK

Error\_Handler non atteint



Framebuffer :



adresse = 0x34200000

résolution = 800 × 480

format = RGB565

taille = 768000 octets

≈ 750 KiB



Acquisition :



framebuffer change avec scène réelle

callback frame actif

≈ 30 FPS



CameraFrame :



width = 800

height = 480

format = RGB565

data = 0x34200000

data\_size\_bytes = 768000



LiDAR simulé :



simulateur exécuté sur STM32

range\_mm\[0]\[0] = 4000

range\_mm\[20]\[25] = 1200



Git :



baseline b66709b poussé

branche Note-de-Seb propre





============================================================

54\. CE QUI N'EST PAS ENCORE VALIDÉ

============================================================



Ne pas présenter comme terminé :



CameraFrame dynamique dans callback

CameraSource

snapshot final

LidarSource

détection obstacle complète sur carte

VL53L9CX réel

driver I3C réel

depth → XYZ complet

nuage de points réel

occupancy map réelle

estimation de pose réelle

fusion RGB-D

IA embarquée

Neural-ART





============================================================

55\. PROCHAINE MISSION EXACTE

============================================================



Reprendre la caméra.



Fichier :



firmware/camera\_imx335/FSBL/Src/main.c



Localiser :



HAL\_DCMIPP\_PIPE\_FrameEventCallback(...)



Ajouter :



g\_camera\_frame.frame\_id = NbMainFrames;

g\_camera\_frame.timestamp\_ms = HAL\_GetTick();



Puis :



build

→ debug

→ Expressions



Expressions :



NbMainFrames

g\_camera\_frame.frame\_id

g\_camera\_frame.timestamp\_ms



Valider :



frame\_id suit le compteur

timestamp progresse

pas d'Error\_Handler

FPS toujours cohérent





============================================================

56\. APRÈS VALIDATION DE LA FRAME DYNAMIQUE

============================================================



Créer :



camera\_source.h

camera\_source.c



Puis abstraire progressivement :



IMX335

DCMIPP

ISP

framebuffer

frame-ready



derrière CameraSource.





============================================================

57\. APRÈS CAMERASOURCE

============================================================



Reprendre LiDAR :



stats

→ obstacle detection

→ lidar\_types

→ lidar\_processing

→ LidarSource



Objectif :



avoir deux interfaces symétriques :



CameraSource

LidarSource





============================================================

58\. APRÈS LES DEUX SOURCES

============================================================



Créer une base de synchronisation :



CameraFrame

\+

LidarFrame

\+

Pose



Puis :



timestamp matching

→ SensorPair

→ reconstruction





============================================================

59\. RÈGLES DE DÉVELOPPEMENT

============================================================



Pour chaque modification importante :



1\. modifier ;

2\. compiler ;

3\. tester sur carte ;

4\. noter le résultat ;

5\. distinguer :

&#x20;  - prévu ;

&#x20;  - implémenté ;

&#x20;  - compilé ;

&#x20;  - testé ;

&#x20;  - validé ;

6\. commit ;

7\. push sur Note-de-Seb.



Ne jamais déclarer une étape "terminée" simplement parce que le code a été écrit.





============================================================

60\. RÈGLES GIT

============================================================



Toujours vérifier :



git status



Avant commit :



git diff

git diff --cached



Commit sur :



Note-de-Seb



Push :



git push origin Note-de-Seb



Ne pas push sur main sans accord explicite.





============================================================

61\. BASELINE À NE PAS PERDRE

============================================================



Commit important :



b66709b



Message :



feat: add STM32 camera and LiDAR simulation baseline



Ce commit doit être considéré comme le baseline reproductible avant la prochaine modification caméra.





============================================================

62\. RÉSUMÉ ULTRA-COURT POUR UNE IA DE REPRISE

============================================================



REPO:

rahim974/3Dscan2A



BRANCHE:

Note-de-Seb uniquement



BASELINE:

b66709b



CAMÉRA:

Sony IMX335

MIPI CSI-2

DCMIPP OK

ISP OK

800×480

RGB565

framebuffer = 0x34200000

buffer = 768000 octets

≈ 30 FPS

CameraFrame statique validée



NEXT CAM:

mettre frame\_id + timestamp à jour dans FrameEventCallback

tester

puis créer CameraSource



LIDAR:

VL53L9CX futur

simulateur actuel 54×42

2268 zones

fond 4000 mm

obstacle 1200 mm

simulateur validé sur STM32



NEXT LIDAR:

stats

détection

LidarSource

puis backend réel I3C



ARCHITECTURE:

IMX335 → CSI-2

VL53L9CX → I3C

scanner portable

Stop-and-Scan V1

fusion après validation individuelle

IA plus tard





============================================================

63\. PREMIÈRE ACTION CONSEILLÉE À UNE IA QUI REPREND

============================================================



Avant toute modification :



1\. lire ce fichier ;

2\. lire :

&#x20;  Protocoles testes et simulations/test\_camera.html

3\. lire :

&#x20;  Protocoles testes et simulations/simulation\_lidar.html

4\. inspecter :

&#x20;  firmware/camera\_imx335/FSBL/Src/main.c

5\. inspecter :

&#x20;  firmware/camera\_imx335/STM32CubeIDE/FSBL/Application/User/camera\_types.h

6\. vérifier :

&#x20;  git status

&#x20;  git branch

&#x20;  git log -1 --oneline

7\. ne modifier ensuite que le callback caméra prévu.





============================================================

64\. ÉTAT FINAL AU MOMENT DU HANDOFF

============================================================



Le dépôt Git est propre après le baseline firmware.



Le code caméra est figé juste AVANT la mise à jour dynamique de CameraFrame.



Le simulateur LiDAR est présent et fonctionnel.



La prochaine étape doit rester petite et vérifiable :



FrameEventCallback

→ frame\_id

→ timestamp



Puis seulement :



CameraSource



FIN DU HANDOFF.

