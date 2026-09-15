# Projet de scanner 3D de salle vers plan 2D sémantique
## Architecture STM32N6570-DK + VL53L9CX + caméra + IA embarquée

## 1. Objectif du projet

L’objectif est de concevoir un système embarqué autonome, basé sur une **STM32N6570-DK**, capable de :

- acquérir la géométrie d’une salle avec un **VL53L9CX** ;
- reconstruire une représentation 3D ou 2,5D de l’environnement ;
- produire un **plan 2D** de la salle ;
- détecter et identifier des obstacles ou objets significatifs, par exemple :
  - tables ;
  - chaises ;
  - personnes ;
  - armoires ;
  - portes ;
- associer la géométrie mesurée par le LiDAR à une information sémantique issue d’une caméra ;
- exécuter, à terme, l’IA directement sur le microcontrôleur grâce au **Neural-ART NPU** du STM32N6 ;
- optimiser la consommation énergétique, le temps d’inférence, la mémoire et le temps total de scan.

Le problème peut être décomposé en quatre sous-problèmes indépendants :

1. **Perception géométrique** : où se trouvent les murs et obstacles ?
2. **Localisation du capteur** : depuis quelle orientation chaque mesure a-t-elle été prise ?
3. **Perception sémantique** : de quel type d’objet s’agit-il ?
4. **Fusion** : comment rattacher une détection RGB à la géométrie LiDAR ?

Cette séparation est importante afin de pouvoir valider chaque bloc indépendamment.

---

## 2. Pourquoi le STM32N6570-DK est très adapté

La carte STM32N6570-DK embarque un **STM32N657X0**, qui comprend notamment :

- un cœur **Arm Cortex-M55** ;
- un accélérateur IA **Neural-ART** ;
- environ **4,2 Mo de SRAM interne** ;
- une mémoire Flash externe Octo-SPI de grande capacité sur la carte ;
- de la PSRAM externe ;
- une interface caméra ;
- Ethernet ;
- microSD ;
- écran ;
- connecteurs et interfaces d’extension.

Le **Neural-ART** peut atteindre environ **600 GOPS** et est conçu pour accélérer l’inférence de réseaux neuronaux quantifiés.

Le STM32N6 possède également des blocs matériels utiles à la vision :

- interface **MIPI CSI-2** ;
- ISP pour le traitement caméra ;
- accélérations adaptées aux traitements d’image ;
- contrôleurs mémoire adaptés aux données volumineuses.

### Chaîne logicielle IA

ST fournit une chaîne complète permettant de partir d’un modèle entraîné sur PC et de le déployer sur le STM32N6570-DK :

```text
PyTorch / TensorFlow / ONNX
          ↓
       modèle
          ↓
 quantification INT8
          ↓
    ST Edge AI Core
          ↓
 compilation Neural-ART
          ↓
   STM32N6570-DK
```

Cette chaîne permet notamment d’analyser :

- la quantité de RAM requise ;
- la quantité de Flash nécessaire ;
- la compatibilité des opérateurs ;
- les parties exécutées par le NPU ;
- les parties qui retombent éventuellement sur le CPU ;
- la latence estimée ou mesurée.

Une métrique importante du projet peut donc être :

```text
% des opérateurs exécutés sur Neural-ART
vs
% des opérateurs exécutés sur Cortex-M55
```

---

## 3. Intérêt du VL53L9CX

Le VL53L9CX est un capteur de profondeur multi-zone beaucoup plus riche qu’un simple télémètre.

Caractéristiques principales :

- environ **54 × 42 zones**, soit plus de 2 000 mesures de profondeur ;
- champ de vision d’environ **55° × 42°** ;
- résolution angulaire proche de 1° ;
- portée de plusieurs mètres, jusqu’à environ 8,8 m selon les conditions ;
- fréquence pouvant aller jusqu’à environ 100 Hz ;
- sorties de profondeur ;
- information de confiance ;
- réflectance ;
- IR actif et ambiant selon les sorties disponibles ;
- interfaces adaptées aux systèmes embarqués.

ST cite notamment des usages comme :

- SLAM ;
- obstacle avoidance ;
- 3D room mapping.

Le capteur doit donc être considéré comme une **petite caméra de profondeur** plutôt que comme un télémètre ponctuel.

ST fournit également **STSW-IMG053**, un exemple logiciel destiné à l’utilisation du VL53L9CX avec la STM32N6570-DK. Il est recommandé de partir de cet exemple avant de développer une pile bas niveau complète.

---

## 4. Architecture matérielle de référence

```text
                         CAMÉRA RGB
                             │
                       MIPI CSI-2
                             │
                             ▼
                     ┌───────────────┐
 VL53L9CX ───────────►               │
                     │               │
 Encodeur axe ──────►│ STM32N6570-DK│
                     │               │
 IMU ───────────────►│ Cortex-M55    │
                     │ Neural-ART    │
 Driver moteur ◄─────│               │
                     └───────┬───────┘
                             │
          ┌──────────────────┴──────────────────┐
          │                                     │
   géométrie LiDAR                       IA caméra
          │                                     │
    nuage 3D / 2.5D                        classe / masque
          │                                     │
          └──────────────────┬──────────────────┘
                             │
                         fusion RGB-D
                             │
                      carte sémantique
                             │
                             ▼
                          PLAN 2D
```

Principe fondamental :

- **LiDAR = géométrie**
- **caméra = sémantique**

---

## 5. Axe A — Cartographie géométrique sans IA

Avant d’intégrer l’intelligence artificielle, il faut démontrer que le système sait reconstruire correctement la géométrie.

Le VL53L9CX est monté sur un mécanisme rotatif.

Chaque point dépend de :

- `d` : distance mesurée ;
- `α` : angle horizontal associé à une zone du LiDAR ;
- `β` : angle vertical associé à cette zone ;
- `θ` : angle du bras ou de la tête rotative.

Le point peut ensuite être converti dans un repère cartésien :

```text
P = (x, y, z)
```

En accumulant les acquisitions pour plusieurs angles de rotation, on obtient un nuage de points couvrant la salle.

Cette étape doit être la baseline scientifique du système.

Une conclusion possible et parfaitement valide serait que l’IA n’est pas nécessaire pour détecter un obstacle géométrique, mais uniquement pour lui associer une classe sémantique.

---

## 6. Axe B — Détection d’objets RGB

Premier candidat : **YOLOv8n INT8**.

Une détection classique fournit :

```text
classe + bounding box
```

Exemple :

```text
TABLE
[x1, y1, x2, y2]
```

Avantages :

- simple ;
- mature ;
- facile à entraîner ;
- supporté par l’écosystème ST ;
- faible coût de calcul par rapport à des modèles plus gros.

Limite principale :

une bounding box contient souvent :

- l’objet ;
- une partie du sol ;
- un mur ;
- d’autres objets.

L’association entre points LiDAR et objet peut donc être imprécise.

---

## 7. Axe C — Instance segmentation

L’instance segmentation est très intéressante pour le projet.

Au lieu d’obtenir seulement une boîte englobante, le réseau renvoie un **masque de pixels correspondant réellement à l’objet**.

Candidats :

- YOLOv8n-seg ;
- YOLOv11n-seg.

Avantages :

- meilleure correspondance entre pixels RGB et points LiDAR ;
- meilleure localisation des contours d’objet ;
- plus adaptée à une fusion RGB-D.

Limites :

- plus de calcul ;
- plus de RAM ;
- plus de post-traitement ;
- entraînement et dataset potentiellement plus exigeants.

Pour la version finale, **YOLOv8n-seg 256×256 INT8** constitue un candidat particulièrement pertinent.

---

## 8. Axe D — Semantic segmentation

Une autre approche consiste à attribuer une classe à chaque pixel :

```text
mur
sol
table
chaise
personne
```

Un modèle de type DeepLabV3 peut être étudié.

Avantage :

- représentation dense de la scène.

Limite :

- ne distingue pas forcément plusieurs objets appartenant à la même classe.

Cela peut néanmoins être utile si l’objectif principal est de construire une carte d’occupation sémantique plutôt que d’identifier chaque instance.

---

## 9. Axe E — IA basée directement sur la profondeur

Le VL53L9CX produit une matrice de profondeur de faible résolution.

On peut envisager un modèle léger utilisant comme entrées :

- profondeur ;
- confiance ;
- réflectance.

Cela pourrait permettre de reconnaître grossièrement certaines formes :

- mur ;
- chaise ;
- table ;
- personne.

Avantages :

- dépend moins de la caméra RGB ;
- architecture potentiellement plus légère ;
- intéressant scientifiquement.

Limites :

- résolution très faible ;
- ambiguïtés fortes entre objets ;
- peu adapté à des classes visuellement proches.

Cet axe est intéressant comme comparaison expérimentale.

---

## 10. Axe F — Fusion RGB-D

La fusion la plus simple ne nécessite pas forcément un réseau multimodal.

On peut :

1. reconstruire les points 3D à partir du LiDAR ;
2. calibrer précisément la caméra et le LiDAR ;
3. projeter les points LiDAR dans le plan image de la caméra ;
4. attribuer à chaque point la classe ou le masque correspondant.

Schéma :

```text
RGB ───────┐
           ├── fusion géométrique/sémantique
Depth ─────┘
```

Un véritable réseau RGB-D pourrait être étudié plus tard, mais il introduit davantage de complexité :

- entraînement ;
- dataset ;
- quantification ;
- compatibilité Neural-ART ;
- mémoire ;
- latence.

Il est donc préférable de commencer par une fusion géométrique classique.

---

## 11. Axe G — Salle vide de référence ou salle inconnue

Deux stratégies peuvent être comparées.

### Méthode 1 — Salle vide de référence

```text
Map_current - Map_empty = obstacles
```

Avantages :

- détection simple des nouveaux obstacles ;
- forte robustesse si la salle ne change pas.

Limites :

- nécessite une phase de calibration ;
- suppose un environnement connu ;
- moins généraliste.

### Méthode 2 — Salle inconnue

Le système doit lui-même :

- trouver le sol ;
- trouver les murs ;
- séparer les obstacles.

Avantage :

- fonctionne dans une pièce nouvelle.

Limite :

- traitement plus complexe.

Cette comparaison est particulièrement intéressante dans un rapport d’ingénierie.

---

## 12. Rotation : scan discret ou continu

### Scan discret

```text
tourner
↓
arrêter
↓
attendre stabilisation
↓
LiDAR + RGB
↓
tourner
```

Avantages :

- haute précision angulaire ;
- synchronisation simple ;
- peu de flou ;
- très adapté au prototype.

Inconvénient :

- acquisition lente.

### Scan continu

```text
moteur tourne en continu
↓
LiDAR + caméra + encodeur timestampés
```

Avantages :

- scan rapide ;
- mécanique plus fluide.

Inconvénients :

- synchronisation plus complexe ;
- interpolation angulaire nécessaire ;
- risques de flou ou de décalage temporel.

Il est recommandé de commencer par un scan discret puis d’étudier la rotation continue comme optimisation.

---

## 13. Encodeur et IMU

L’encodeur doit être la référence principale pour l’angle de rotation.

```text
encodeur = angle de balayage
IMU = orientation / inclinaison du système
```

L’IMU peut servir à mesurer notamment :

- roll ;
- pitch ;
- perturbations mécaniques.

L’utilisation de l’IMU seule pour mesurer l’angle absolu de rotation n’est pas recommandée en raison de la dérive.

---

## 14. Calibration

La calibration constitue l’un des enjeux majeurs du projet.

### Calibration LiDAR

Déterminer la direction correspondant à chaque zone du VL53L9CX.

### Calibration mécanique

Déterminer la position réelle du LiDAR par rapport à l’axe de rotation.

### Calibration intrinsèque caméra

Déterminer notamment :

```text
fx, fy, cx, cy
```

et les coefficients de distorsion.

### Calibration extrinsèque caméra-LiDAR

Déterminer la transformation :

```text
P_camera = R × P_lidar + t
```

Cette transformation permet d’associer un point LiDAR à un pixel de l’image.

---

## 15. Limites et risques

Le système possède plusieurs limites physiques et logicielles.

| Limite | Conséquence |
|---|---|
| portée du VL53L9CX | grandes salles plus difficiles |
| champ vertical limité | zones mortes |
| résolution angulaire | objets très fins difficiles |
| occlusions | zones invisibles derrière les objets |
| surfaces particulières | erreurs de profondeur possibles |
| objets/personnes mobiles | incohérences entre acquisitions |
| erreurs mécaniques | propagation directe dans la carte |
| mauvaise calibration RGB/LiDAR | mauvaise fusion |
| quantification INT8 | perte possible de précision |
| petit dataset | mauvaise généralisation |
| documentation récente | risque logiciel et intégration |

Le VL53L9CX étant récent, il faut également prendre en compte la maturité des outils et de la documentation.

---

## 16. Classes IA recommandées

Il est conseillé de limiter le nombre de classes au besoin réel du projet.

Exemple :

```text
table
chaise
personne
armoire
porte
```

Éventuellement :

```text
écran
```

L’objectif n’est pas de construire un système de vision universelle, mais de reconnaître les objets utiles à un plan de salle.

---

## 17. Dataset

Deux jeux de données peuvent être comparés.

### Dataset générique

Utilisé pour valider rapidement le système.

### Dataset spécialisé

Images prises :

- dans les salles du projet ;
- avec la caméra réellement utilisée ;
- aux angles et distances représentatifs.

Expérience intéressante :

```text
modèle pré-entraîné
vs
modèle fine-tuné
vs
test dans une salle différente
```

Cette expérience permet d’étudier la généralisation.

---

## 18. Métriques

Le système doit être évalué quantitativement.

### LiDAR

- erreur moyenne ;
- RMSE ;
- répétabilité.

### Angle

- erreur angulaire ;
- erreur cumulée ;
- répétabilité mécanique.

### Carte

- erreur de position des murs ;
- erreur de dimensions ;
- erreur de position des obstacles.

### IA

- precision ;
- recall ;
- mAP ;
- IoU pour la segmentation.

### Embarqué

- RAM ;
- Flash ;
- temps d’inférence ;
- fréquence d’inférence ;
- part NPU / CPU.

### Système complet

- temps total d’un scan ;
- puissance moyenne ;
- énergie totale par scan.

Métrique intéressante :

```text
E_scan = intégrale de P(t) sur la durée du scan
```

Une architecture plus puissante peut consommer davantage instantanément mais finir plus vite et consommer moins d’énergie totale.

---

## 19. Plan expérimental en neuf jalons

### Jalon 1 — VL53L9CX fixe

Objectifs :

- utiliser STSW-IMG053 ;
- lire profondeur, confiance et autres sorties ;
- caractériser précision, bruit et répétabilité.

### Jalon 2 — Scanner rotatif

Ajouter :

- moteur ;
- encodeur ;
- éventuellement IMU.

Mesurer la précision angulaire.

### Jalon 3 — Nuage de points

Convertir les mesures en XYZ.

Visualiser d’abord sur PC.

### Jalon 4 — Plan 2D sans IA

- détecter sol ;
- détecter murs ;
- supprimer sol/plafond ;
- construire une occupancy grid.

### Jalon 5 — Caméra synchronisée

- calibrer caméra ;
- calibrer caméra-LiDAR ;
- associer RGB, LiDAR, angle et timestamp.

### Jalon 6 — IA sur PC

Comparer :

- YOLOv8n detection ;
- YOLOv8n-seg ;
- éventuellement semantic segmentation.

### Jalon 7 — Fusion RGB-D

Associer les objets détectés aux points 3D.

Mesurer l’erreur de localisation.

### Jalon 8 — Déploiement Neural-ART

- quantification INT8 ;
- compilation ST Edge AI ;
- mesure RAM ;
- Flash ;
- latence ;
- NPU/CPU.

### Jalon 9 — Optimisation

Comparer :

- 256×256 vs 320×320 ;
- plusieurs fréquences d’inférence ;
- scan discret vs continu ;
- fréquences LiDAR ;
- consommation énergétique.

---

## 20. Expériences comparatives recommandées

| Expérience | Question |
|---|---|
| IMU vs encodeur | quelle mesure d’angle est la plus fiable ? |
| scan discret vs continu | compromis vitesse/précision |
| 256×256 vs 320×320 | précision vs temps vs énergie |
| detection vs segmentation | gain réel de localisation ? |
| RGB vs profondeur | combien apporte la caméra ? |
| salle connue vs inconnue | intérêt d’une référence vide |
| PC vs Neural-ART | coût de l’embarqué |
| modèle générique vs fine-tuné | généralisation |
| forte vs faible fréquence LiDAR | précision vs énergie |

---

## 21. Méthode de développement recommandée

Il ne faut pas intégrer tout le système en une seule fois.

Ordre recommandé :

```text
LiDAR
↓
mécanique
↓
géométrie
↓
plan 2D
↓
caméra
↓
IA
↓
fusion
↓
embarqué
↓
optimisation
```

Le PC doit rester longtemps un outil de référence.

Il peut enregistrer :

```text
RGB
depth
confidence
angle
IMU
timestamp
```

afin de rejouer les acquisitions hors ligne et déboguer sans refaire physiquement chaque scan.

---

## 22. Architecture cible proposée

```text
                   ┌───────────────┐
                   │ RGB CAMERA    │
                   └───────┬───────┘
                           │
                           │ MIPI
                           ▼
┌──────────┐       ┌─────────────────────┐
│VL53L9CX  ├──────►│                     │
└──────────┘       │                     │
                   │   STM32N6570-DK     │
┌──────────┐       │                     │
│ ENCODER  ├──────►│ Cortex-M55          │
└──────────┘       │                     │
                   │ Neural-ART          │
┌──────────┐       │                     │
│ IMU      ├──────►│                     │
└──────────┘       └──────────┬──────────┘
                              │
             ┌────────────────┴─────────────┐
             │                              │
     Occupancy grid                 YOLOv8n-seg
             │                              │
             └──────────────┬───────────────┘
                            │
                       fusion RGB-D
                            │
                            ▼
                 semantic occupancy map
                            │
                            ▼
                    ┌──────────────┐
                    │   PLAN 2D    │
                    │ TABLE        │
                    │ CHAIRS       │
                    │ WALLS        │
                    │ PEOPLE       │
                    └──────────────┘
```

---

## 23. Choix IA initial recommandé

### Baseline

**YOLOv8n detection INT8**

À tester en 256×256 puis 320×320.

### Candidat principal pour la fusion

**YOLOv8n-seg INT8**

Probablement en 256×256 au départ afin de conserver une marge mémoire et énergétique.

### Alternatives

- YOLOv11n-seg ;
- semantic segmentation ;
- petit CNN sur depth map ;
- modèle spécifique au dataset du projet.

Le choix final doit être déterminé expérimentalement, et non uniquement en fonction de la précision IA.

Critère d’ingénierie utile :

```text
gain de qualité
----------------------------
coût mémoire + temps + énergie
```

---

## 24. Conclusion

Le couple **STM32N6570-DK + VL53L9CX** est techniquement très cohérent pour ce projet.

La question principale n’est probablement pas de savoir si le STM32N657 peut exécuter une IA de vision, car le Neural-ART est précisément destiné à cela.

Les véritables défis sont :

1. précision mécanique ;
2. calibration ;
3. synchronisation ;
4. reconstruction géométrique ;
5. fusion caméra-LiDAR ;
6. généralisation du modèle ;
7. consommation énergétique.

La voie principale recommandée est :

```text
VL53L9CX
+ encodeur
+ reconstruction géométrique
+ caméra RGB
+ YOLOv8n-seg INT8
+ fusion RGB-D
+ semantic occupancy map
```

Les axes expérimentaux à conserver sont notamment :

- detection vs segmentation ;
- profondeur seule vs RGB ;
- salle connue vs salle inconnue ;
- scan discret vs continu ;
- résolution IA 256 vs 320 ;
- exécution PC vs Neural-ART ;
- modèle générique vs spécialisé ;
- optimisation énergie/scan.

L’objectif final n’est donc pas seulement de produire un scanner fonctionnel, mais de justifier quantitativement les compromis d’architecture retenus.


NOTE: Finetune possible et recommandé sur YOLO