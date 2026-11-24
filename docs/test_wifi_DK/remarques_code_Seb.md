# Remarques sur le code caméra et lidar de Seb, du point de vue du Wi-Fi

Ce document explique comment le code écrit par Seb pour la caméra et le lidar pourra fonctionner avec le module Wi-Fi, que nous avons testé sur le kit STM32N6570-DK (voir les autres fichiers de ce dossier). Il s'appuie sur la lecture de la branche `Note-de-Seb` du dépôt, telle qu'elle était le 24 novembre 2026. Le dernier commit lu porte le message « P06: add interchangeable LiDAR source and validate continuous acquisition on STM32 ».

Le but n'est pas de critiquer ce code, qui fonctionne et qui est bien organisé, mais de repérer à l'avance ce qu'il faudra adapter quand le Wi-Fi, la caméra et le lidar devront tourner ensemble sur le prototype A.

## Ce que contient la branche de Seb

La branche contient deux programmes séparés pour le kit : `firmware/camera_imx335/` fait fonctionner la caméra IMX335 du kit, et `firmware/lidar_sim/` simule un lidar. Le vrai capteur lidar (VL53L9CX) n'est pas encore branché : c'est l'étape P07 prévue par Seb, qui n'a pas commencé. Les étapes suivantes (P08 à P11) prévoient de synchroniser la caméra et le lidar, puis de construire le nuage de points et la carte 3D.

Les deux programmes fonctionnent sans système d'exploitation. Le cœur du programme est une boucle infinie (`while (1)` dans `main.c`) qui appelle à tour de rôle la caméra, le lidar, les tests et l'affichage. Seule la caméra utilise une interruption : quand une image est prête, le matériel prévient le programme, qui note simplement que l'image est disponible. Le lidar simulé produit une image toutes les 100 ms (10 images par seconde) ; le programme vérifie l'heure à chaque tour de boucle pour savoir quand produire la suivante.

Le code est découpé proprement en modules. Le module caméra (`camera_source`) s'occupe de démarrer la caméra, de récupérer la dernière image et de prendre une photo figée à la demande. Le module lidar (`lidar_source`) offre une interface qui ne dépend pas du capteur utilisé, ce qui permettra de remplacer le lidar simulé par le vrai sans changer le reste du programme. Seb a fixé une règle claire : ces modules ne sont appelés que depuis la boucle principale, jamais depuis une interruption, et le programme n'alloue jamais de mémoire en cours de route.

Côté données, l'image de la caméra fait 800 × 480 pixels en couleurs (768 000 octets) et arrive environ 30 fois par seconde. Une image lidar contient 54 × 42 zones ; chaque zone occupe 7 octets (distance, amplitude, lumière ambiante et un indicateur de qualité), ce qui donne 15 880 octets par image avec l'horodatage.

## Ce que cela implique pour le Wi-Fi

### Il faudra passer à FreeRTOS

Le pilote Wi-Fi fourni par ST ne fonctionne qu'avec FreeRTOS, un système d'exploitation temps réel qui découpe le programme en plusieurs tâches tournant en parallèle, chacune avec sa priorité. Le pilote a besoin de ses propres tâches, à haute priorité, pour dialoguer avec le module Wi-Fi sans jamais le faire attendre. Comme le code de Seb tourne dans une simple boucle, sans FreeRTOS, il faudra le réorganiser en tâches pour le prototype A : une tâche pour la caméra, une pour le lidar, une pour la fusion des données et une pour l'envoi Wi-Fi.

Ce passage devrait être assez simple, car la règle de Seb (chaque module n'est utilisé que par un seul endroit du programme) correspond déjà à cette organisation : chaque module deviendra une tâche, et les tâches se passeront les images par des files d'attente ou des tampons protégés.

### Il faudra revoir l'organisation de la mémoire

Les deux projets ne rangent pas leurs données au même endroit. Le programme de Seb utilise 256 Ko de mémoire interne à partir de l'adresse `0x341C0000`, plus des zones à part pour les images. Le projet Wi-Fi de ST, lui, utilise 1,5 Mo à partir de l'adresse `0x34000000`, dont 200 000 octets réservés à FreeRTOS. Ces deux organisations ne peuvent pas être additionnées telles quelles. Il faudra établir un plan mémoire commun, quitte à réduire la mémoire réservée à FreeRTOS ou à déplacer certains tampons.

Il existe aussi un chevauchement précis entre la démo Wi-Fi et le code de Seb. La démo affiche son panneau d'état dans une zone de mémoire qui commence à `0x342C0000` et s'étend jusqu'à `0x3437B800`. Or Seb range sa photo figée de la caméra à partir de `0x342E0000`. Les deux zones se recouvrent : si on les assemble sans rien changer, la photo et le panneau d'état s'écraseraient mutuellement. Il faudra déplacer l'une des deux. En revanche, le fait que la démo Wi-Fi et la caméra de Seb utilisent toutes les deux l'adresse `0x34200000` pour l'image de fond est voulu : cette zone était prévue dès le départ pour recevoir l'image de la caméra.

### Le débit Wi-Fi est largement suffisant

Avec le format de Seb, le lidar produit 15 880 octets dix fois par seconde, soit environ 159 Ko par seconde (1,3 Mbit/s). Le Wi-Fi a tenu 8,3 Mbit/s pendant nos essais : il reste donc une marge confortable. En revanche, une image complète de la caméra (768 000 octets) mettrait environ trois quarts de seconde à partir : on peut envoyer une photo de temps en temps, mais pas une vidéo en continu.

### Il faudra respecter les règles d'envoi trouvées pendant les essais Wi-Fi

Nos essais ont montré que le module Wi-Fi décroche si on lui envoie les données par à-coups, avec des pauses entre les envois, et que des données arrivent remplies de zéros si on lui confie plus de 1 520 octets à la fois. Le futur programme devra donc envoyer les scans en continu, par blocs de 1 520 octets au maximum. La solution la plus simple sera de faire déposer les scans par la tâche de fusion dans une file d'attente, que la tâche d'envoi videra sans s'arrêter.

Il faudra aussi veiller à trois détails techniques lors du passage à FreeRTOS. D'abord, les tâches du pilote Wi-Fi doivent rester plus prioritaires que celles des capteurs. Ensuite, l'interruption de la caméra devra se contenter de réveiller la tâche caméra, et son niveau de priorité devra être compatible avec FreeRTOS. Enfin, les attentes qui reposent aujourd'hui sur la lecture de l'heure dans la boucle devront être remplacées par les fonctions d'attente de FreeRTOS.

### Un réglage d'horloge à vérifier

Le projet caméra de Seb ne règle pas l'une des horloges du microcontrôleur (la PLL4). La démo Wi-Fi, elle, s'en sert pour cadencer l'écran. Si l'on ajoute le panneau d'état de la démo au projet de Seb, il faudra vérifier ce réglage, sinon l'écran risque de ne pas s'afficher correctement.
