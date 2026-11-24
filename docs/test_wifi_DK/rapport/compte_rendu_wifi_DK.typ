// Compte rendu des essais Wi-Fi : X-NUCLEO-67W61M1 sur STM32N6570-DK
// Compilation : typst compile compte_rendu_wifi_DK.typ

#set document(title: "Essais Wi-Fi X-NUCLEO-67W61M1 sur STM32N6570-DK", author: "Projet SCAN 3D")
#set page(paper: "a4", margin: (x: 2.2cm, y: 2.2cm), numbering: "1 / 1", footer-descent: 30%)
#set text(lang: "fr", size: 10.5pt)
#set par(justify: true, leading: 0.62em)
#set heading(numbering: "1.1")
#show heading.where(level: 1): set text(size: 15pt)
#show heading.where(level: 1): it => block(above: 1.6em, below: 0.8em)[#it]
#show heading.where(level: 2): set text(size: 12pt)
#show raw.where(block: true): it => block(fill: luma(245), inset: 8pt, radius: 3pt, width: 100%, it)
#show raw.where(block: false): it => box(fill: luma(240), inset: (x: 3pt), outset: (y: 3pt), radius: 2pt, it)
#show link: set text(fill: rgb("#1a56a0"))

#let accent = rgb("#2b6cb0")

// Encadré « à quoi ça sert / pour le prototype A »
#let utilite(sert, protoA) = block(
  fill: rgb("#eef4fb"), stroke: (left: 2.5pt + accent), inset: 9pt, radius: 2pt, width: 100%, breakable: false,
)[
  #text(weight: "bold", fill: accent)[À quoi sert cette étape.] #sert \
  #text(weight: "bold", fill: accent)[Dans le prototype A.] #protoA
]

#let resultat(body) = block(
  fill: rgb("#f1f8f1"), stroke: (left: 2.5pt + rgb("#2f855a")), inset: 9pt, radius: 2pt, width: 100%, breakable: false,
)[#text(weight: "bold", fill: rgb("#2f855a"))[Résultat.] #body]

// ───────────── Page de titre ─────────────
#align(center)[
  #v(1.2cm)
  #text(size: 22pt, weight: "bold")[Essais Wi-Fi de la X-NUCLEO-67W61M1 \ sur le STM32N6570-DK]
  #v(0.4cm)
  #text(size: 12pt)[Compte rendu détaillé de la séance du 24 novembre 2026]
  #v(0.2cm)
  #text(size: 10pt, fill: luma(90))[Projet SCAN 3D, test préparatoire à la carte fille du prototype A]
]
#v(0.8cm)

= Objectif et conclusion

*Objectif.* Vérifier que le module Wi-Fi 6 / BLE 5.4 ST67W611M1, celui qui équipera la carte fille du prototype A, fonctionne avec le STM32N6, et qu'il peut envoyer les scans du scanner 3D vers un PC. Le critère retenu (test 4 de l'architecture) est un débit Wi-Fi au moins deux fois supérieur au flux du lidar, estimé à environ 270 Ko/s, soit 2,2 Mbit/s.

*Montage d'essai.* La carte d'extension X-NUCLEO-67W61M1 (module ST67W611M1, liaison SPI) est enfichée sur les connecteurs Arduino du kit STM32N6570-DK. Le kit est relié à un Mac par le port USB-C du ST-LINK, ce qui donne à la fois l'alimentation, le flash et la console série.

#resultat[
  *Le critère est rempli.*
  - Le module répond, se connecte en WPA2 à 2,4 GHz, obtient une adresse par DHCP, résout les noms (DNS) et répond au ping.
  - Débit mesuré avec l'outil de test de ST : *8,3 Mbit/s pendant 60 s* sans aucun blocage, soit environ 3,8 fois le flux du lidar.
  - Une démo qui simule le scanner (trames de 54 × 42 zones, 30 trames/s) a tenu *5,5 minutes en continu à 2,2 Mbit/s*, sans décrochage, tout en affichant l'état sur l'écran du kit.
  - Le point délicat trouvé : le module décroche si on lui envoie les données *par à-coups* (une trame toutes les 33 ms avec des pauses). Il tient si on envoie *en continu*. C'est la règle à retenir pour le firmware du prototype A.
]

*Ce qui reste à confirmer :* la dernière version de la démo (v7, envoi continu dès le démarrage) n'a pas été testée sur batterie, car le Mac était sorti du réseau de test en fin de séance.

#outline(title: "Sommaire", indent: 1.2em, depth: 1)

// ───────────── 1 ─────────────
= Matériel, logiciels et conventions

== Matériel

#table(
  columns: (auto, 1fr),
  stroke: 0.5pt + luma(190), inset: 6pt,
  [*Carte*], [*Rôle*],
  [STM32N6570-DK (MB1939)], [Kit du projet : STM32N6, écran 5", connecteur caméra, ST-LINK intégré, connecteurs Arduino.],
  [X-NUCLEO-67W61M1], [Carte d'extension avec le module ST67W611M1 (Wi-Fi 6 et BLE 5.4, piloté en SPI). C'est le même module que celui de la carte fille A.],
  [Mac], [Compilation, signature, flash (STM32CubeProgrammer), console série, récepteur des trames.],
  [Téléphone en partage de connexion], [Point d'accès Wi-Fi 2,4 GHz, renommé « Scan3d » en cours de séance.],
)

== Logiciels et fichiers

- *Pack ST* : #link("https://github.com/STMicroelectronics/x-cube-st67w61")[x-cube-st67w61] version 1.3.0, projet `ST67W6X_CLI` prévu pour la NUCLEO-N657X0-Q.
- *Script de portage* `port_STM32N6570-DK.py` : adapte ce projet au kit DK (voir étape 1).
- *STM32CubeIDE 2.2.0* (compilation en ligne de commande), *STM32_SigningTool_CLI*, *STM32CubeProgrammer* avec le loader `MX66UW1G45G_STM32N6570-DK.stldr`.
- *Console série* : 921 600 bauds, 8N1, envoi CR+LF, sur `/dev/cu.usbmodem…`.
- *Dépôt du projet* : branche `module-wifi`, dossier `docs/test_wifi_DK/` (résultats, patch du portage, script de portage, dossier `demo_scan3d/` avec le firmware de la démo, le module d'écran réutilisable et le récepteur PC).

*Convention de lecture.* Chaque étape se termine par un encadré bleu : à quoi elle sert, et ce qu'on en retire pour le prototype A (carte fille enfichée sur le DK, qui garde l'écran et la caméra du kit). Les mots de passe ne figurent pas dans ce document.

// ───────────── 2 ─────────────
= Étape 1 : premier contact, portage du projet ST au kit DK

*Ce qui a été fait.* La X-NUCLEO a été enfichée sur les connecteurs Arduino du DK, puis le DK branché en USB : la LED verte de la carte Wi-Fi s'allume, ce qui prouve que le module est alimenté.

Le projet de ST est écrit pour la NUCLEO-N657X0-Q. Le brochage Arduino du DK diffère sur trois signaux. Le script de portage copie le projet `ST67W6X_CLI` en `ST67W6X_CLI_DK` et corrige les fichiers concernés.

#table(
  columns: (auto, auto, auto, auto),
  stroke: 0.5pt + luma(190), inset: 5pt, align: (left, center, center, center),
  [*Signal*], [*Arduino*], [*NUCLEO-N657X0-Q*], [*STM32N6570-DK*],
  [`SPI_SCK`], [D13], [PE15], [PE15],
  [`SPI_MOSI`], [D11], [PG2], [PG2],
  [`SPI_MISO`], [D12], [PG1], [*PH8*],
  [`SPI_CS`], [D10], [PA3], [PA3],
  [`SPI_RDY`], [D3], [PE9], [PE9],
  [`CHIP_EN`], [D5], [PE10], [PE10],
  [`BOOT` du module], [D6], [PD5], [*PE13*],
  [Bouton utilisateur], [D8], [PD12], [*PE7* (EXTI7)],
)

Commande : `python3 port_STM32N6570-DK.py Projects/NUCLEO-N657X0-Q/Applications/ST67W6X/ST67W6X_CLI`. Le script modifie `main.h`, `stm32n6xx_hal_msp.c`, `main.c`, `stm32n6xx_it.c` et `stm32n6xx_it.h` : horloge du port GPIOH, initialisation séparée de MOSI et MISO, interruption du bouton déplacée de la ligne 12 à la ligne 7.

#utilite(
  [Sans ce portage, le SPI du STM32 ne lit pas la réponse du module (MISO sur la mauvaise broche) et rien ne fonctionne.],
  [La carte A n'utilise pas les mêmes broches que la X-NUCLEO : `SPI_RDY` sur D2 (PD0, interruption EXTI0) au lieu de D3, `CHIP_EN` sur D7 (PD6) au lieu de D5, le signal `BOOT` n'est pas relié au N6. Le portage fait ici montre exactement quelles lignes du projet ST changer (`main.h`, initialisation GPIO, ligne d'interruption). Le SPI5 est conservé sur D10 à D13.],
)

// ───────────── 3 ─────────────
= Étape 2 : compilation et signature

*Ce qui a été fait.* Compilation en ligne de commande avec STM32CubeIDE 2.2.0 (configuration Release) : 0 erreur, 0 avertissement, environ 166 Ko de code. Puis signature, obligatoire pour que le STM32N6 accepte de démarrer le programme :

```
STM32_SigningTool_CLI -nk -of 0x80000000 -t fsbl -hv 2.3 --align \
  -bin ST67W6X_CLI_DK.bin -o ST67W6X_CLI_FSBL-trusted.bin
```

Le fichier produit est `ST67W6X_CLI_FSBL-trusted.bin`.

#utilite(
  [Le STM32N6 n'exécute que du code signé, chargé depuis la Flash externe par le premier programme de démarrage (FSBL).],
  [La même chaîne (compilation, signature, flash à 0x70000000) servira pour le firmware de la carte A. À automatiser dans un script dès qu'il y aura plusieurs cartes à programmer.],
)

// ───────────── 4 ─────────────
= Étape 3 : flash du programme dans la Flash externe

*Ce qui a été fait.* Le N6 se programme en mode développement, puis démarre depuis la Flash externe. Les deux interrupteurs à glissière `BOOT0` et `BOOT1` (en haut du kit, #ref(<fig-boot>)) fixent le mode :

#table(
  columns: (auto, auto, auto),
  stroke: 0.5pt + luma(190), inset: 5pt,
  [*Mode*], [*BOOT0*], [*BOOT1*],
  [Développement (flash par ST-LINK)], [côté L], [côté H],
  [Démarrage sur la Flash externe], [côté L], [côté L],
)

Flash à l'adresse `0x70000000` avec STM32CubeProgrammer et le loader `MX66UW1G45G_STM32N6570-DK.stldr`. Sortie attendue : `File download complete`, puis `Download verified successfully`.

#figure(
  image("img/boot_switches.jpg", width: 55%),
  caption: [Interrupteurs BOOT0 (haut) et BOOT1 (bas) du STM32N6570-DK, photographiés en mode développement : BOOT1 côté H.],
) <fig-boot>

*Piège rencontré.* Après avoir passé BOOT1 côté H, un simple appui sur Reset ne suffit pas. La sonde se connecte (Device ID 0x486) et arrête le cœur, mais toute lecture mémoire échoue :

```
Error: ST-LINK error (DEV_AP_ACCESS_ERROR)
Erasing external memory sectors [0 2]
Error: failed to erase memory
```

Les interrupteurs étaient pourtant corrects (vérifié sur la photo). La solution : *débrancher l'USB, attendre 5 secondes, rebrancher*, puis relancer le flash. Même procédure pour revenir en démarrage Flash externe : BOOT1 côté L, coupure USB complète. Le bouton Reset redémarre le STM32 mais pas le module Wi-Fi, que seule une coupure d'alimentation remet à zéro.

#utilite(
  [Cette étape installe le programme sur la carte et fixe la routine de manipulation, qu'on a ensuite répétée une quinzaine de fois pendant la séance.],
  [Le prototype A reste un kit DK avec une carte fille : la procédure est identique. À retenir : toute reprogrammation exige BOOT1 côté H et une coupure USB, pas un simple Reset. Prévoir un accès facile aux interrupteurs une fois la carte fille montée.],
)

// ───────────── 5 ─────────────
= Étape 4 : première réponse du module (console)

*Ce qui a été fait.* Console série ouverte à 921 600 bauds. L'invite `w61/>` s'affiche. Commande `info` :

```
ST67W6X MW Version:       1.3.0
AT Version:               1.0.0
SDK Version:              2.0.89
Wi-Fi MAC Version:        1.6.44
BT Controller Version:    1.6.112
BT Stack Version:         1.9.56
Build Date:               Sep  4 2025 11:32:47
Module ID:                C6AFDBD111400004 (-B)
Manufacturing Year/Week:  2025 / 13
MAC Address:              40:82:7b:02:1e:c8
```

Commande `wifi_scan` : 20 réseaux affichés (maximum du pilote, d'où l'avertissement `Maximum number of APs reached: 20`, normal), sur les canaux 1, 6 et 11, avec des niveaux de -50 à -86 dBm.

#resultat[Le module répond et donne ses versions : le portage des broches est bon (y compris MISO sur PH8) et le module a déjà son firmware. Il n'a donc pas été modifié. Seul écart relevé : le module est en SDK 2.0.89, alors que ST demande 2.0.106 pour ce pack. Aucun défaut n'a été constaté par la suite qui lui soit imputable avec certitude.]

#utilite(
  [`info` est le test le plus simple : s'il répond, tout le chemin STM32, SPI, module fonctionne. `wifi_scan` prouve que la radio reçoit.],
  [C'est le test de recette à rejouer sur la carte A dès qu'elle sera montée : `info` puis `wifi_scan`. Si `info` ne répond pas, vérifier d'abord le SPI et les broches `SPI_RDY` et `CHIP_EN`, dont l'affectation change sur la carte A.],
)

// ───────────── 6 ─────────────
= Étape 5 : connexion à un réseau et ping

*Ce qui a été fait.* Le téléphone partage sa connexion (réseau 2,4 GHz, canal 6, WPA2). Commandes :

```
wifi_scan -s "<nom du réseau>"
wifi_sta_connect "<nom du réseau>" <mot de passe>
wifi_sta_state
net_sta_ip
ping google.com -c 4 -s 64 -i 1000
```

Sorties : `Connection success`, adresse obtenue par DHCP (10.62.127.x), état `STA GOT IP`, signal entre -34 et -42 dBm. Ping vers `google.com` : 4 paquets envoyés, 4 reçus, 0 % de perte, avec résolution du nom, donc le DNS fonctionne. La commande `dnslookup` n'existe pas dans ce firmware ; un ping vers `st.com` expire, simplement parce que ce site ne répond pas aux pings.

#utilite(
  [Montre que la pile réseau complète (association, DHCP, DNS, routage) fonctionne, pas seulement la radio.],
  [Le scanner devra se connecter à un réseau puis joindre le PC. En laboratoire, l'adresse du PC changeait à chaque reconnexion au partage : la carte A devra soit connaître l'adresse du PC par configuration, soit le découvrir automatiquement (diffusion sur le réseau). Cette amélioration est proposée mais pas encore faite.],
)

// ───────────── 7 ─────────────
= Étape 6 : débit, écho UDP, sens inverse

*Ce qui a été fait.* iperf n'étant pas installé sur le Mac, un petit script Python côté Mac reçoit les octets et les compte ; côté kit, c'est l'iperf intégré au firmware de ST.

#block(breakable: false, table(
  columns: (1.4fr, 2fr, 1.6fr),
  stroke: 0.5pt + luma(190), inset: 5pt,
  [*Test*], [*Commande sur le kit*], [*Résultat*],
  [TCP, kit vers Mac, 10 s], [`iperf -c <IP du Mac> -t 10 -i 1`], [8,6 Mbit/s émis ; le Mac mesure 8,43 Mbit/s],
  [UDP, kit vers Mac, 10 s], [`iperf -c <IP du Mac> -u -b 20 -t 10`], [8,7 Mbit/s, environ 3 % de perte],
  [TCP, 60 s (firmware ST d'origine)], [`iperf -c <IP du Mac> -t 60 -i 10`], [*59,5 Mo en 60 s, 8,3 Mbit/s, aucun blocage*],
  [Écho UDP], [`echostart 5000`, puis `nc -u <IP du kit> 5000` sur le Mac], [Le message revient à l'identique],
  [MQTT], [`mqtt_configure -s 0`, broker public], [Le kit reçoit un message publié depuis le Mac],
  [TCP, Mac vers kit], [`iperf -s` sur le kit], [Échec : la connexion expire (ping aussi)],
))

Le sens Mac vers kit échoue en TCP et en ping, alors que l'UDP passe : probablement l'isolation des clients par le partage de connexion, qui ne gêne pas le sens utile (kit vers Mac). Le plafond d'environ 8,7 Mbit/s, identique en TCP et en UDP, vient vraisemblablement du lien SPI ou du module (hypothèse non vérifiée).

#resultat[Le débit utile (kit vers PC) est d'environ *8,3 à 8,6 Mbit/s*, très stable. Le scanner a besoin d'environ 2,2 Mbit/s : marge d'un facteur 3,8, au-dessus du critère « au moins deux fois le flux du lidar ». Un scan de 10 Mo arrive en une dizaine de secondes.]

#utilite(
  [Le débit conditionne la cadence des scans qu'on peut envoyer au PC. L'écho et le MQTT valident les deux voies de dialogue envisagées.],
  [Architecture retenue : *TCP du scanner vers le PC* pour les scans (flux continu, fiable) ; *UDP ou MQTT du PC vers le scanner* pour les commandes (le TCP entrant est bloqué sur ce type de partage). À revérifier avec le réseau réel du prototype, qui n'aura peut-être pas cette isolation.],
)

// ───────────── 8 ─────────────
= Étape 7 : démo « scanner 3D » avec affichage sur l'écran

*Idée.* Simuler le scanner : le kit génère des trames comme celles du lidar, les envoie au Mac par Wi-Fi, et affiche l'état sur son écran. Le tout doit tourner seul, alimenté par une batterie externe, sans console ni câble vers le Mac.

== Description de la démo

- *Trame simulée* : 54 × 42 zones, 4 octets par zone, plus un en-tête : 9 092 octets par trame, 30 trames/s visées, en TCP vers le port 5002 du Mac.
- *Récepteur sur le Mac* : script Python qui compte les trames, le débit, les trames corrompues ou remplies de zéros, et se recale sur l'en-tête suivant en cas de problème.
- *Écran* : deux couches de l'afficheur LTDC. Le fond montre la « carte de profondeur » simulée (une boule qui bouge) ; ce fond est prévu pour accueillir plus tard la vue caméra. Un panneau semi-transparent par-dessus indique : état Wi-Fi, nom du réseau, adresse IP, RSSI, adresse du PC, état du lien, nombre de trames, débit, erreurs, temps.
- *Organisation du programme* : tâches séparées (génération des trames, réseau, écran à 4 images/s, surveillance), pour que l'écran ne dépende pas du réseau.

#figure(
  image("img/ecran_v1.jpg", width: 42%),
  caption: [Première version de la démo (v1) : le fond et la boule s'affichent, mais pas le panneau de texte (autorisation d'accès mémoire manquante pour la seconde couche).],
) <fig-v1>

== Déroulé des essais, version par version

Chaque nouvelle version est compilée, signée, flashée (BOOT1 côté H, coupure USB), puis testée BOOT1 côté L. Les essais se font d'abord sur USB, pour lire la console en même temps que le récepteur.

#table(
  columns: (auto, 1.7fr, 2.3fr),
  stroke: 0.5pt + luma(190), inset: 5pt, align: (center, left, left),
  [*Version*], [*Changement*], [*Observation et résultat*],
  [v1], [Première démo : une tâche pour tout, envois de 9 Ko.], [Pas de texte à l'écran. Le kit se reconnecte 13 fois, 2 à 49 trames par connexion, puis des octets nuls et l'écran se fige avec l'envoi.],
  [v2], [Tâches séparées, délai d'envoi de 3 s, chien de garde (redémarrage après 20 s), compteur de trames sautées, récepteur qui ne coupe plus. Autorisation d'accès de la couche 2 de l'écran ajoutée.], [Environ 215 trames en 3,5 s, mais 11 trames entières à zéro (exactement 9 092 octets nuls), puis arrêt de l'envoi. Correction de l'affichage ajoutée ; le texte est confirmé à l'écran à partir de la v3.],
  [v2b, v2c], [Réseau renommé « Scan3d », puis nouvelle adresse du Mac compilée dans le firmware.], [Le Mac avait changé d'adresse deux fois (autre réseau Wi-Fi, puis nouvelle adresse sur le partage) : recompilation et reflash à chaque fois. Le kit se connecte bien, mais ne joint pas un Mac absent du réseau.],
  [v3], [Plus de lecture du RSSI pendant l'envoi ; redémarrage du module si 3 échecs de suite ; nettoyage du cache limité à la mémoire de l'écran.], [Hypothèse « la lecture du RSSI fait planter le module » *réfutée* : même scénario sans RSSI (36 trames, 3 à zéro, arrêt net).],
  [v4 (diagnostic)], [Commandes console pour couper le rafraîchissement de l'écran et changer la taille des blocs ; relevé de la mémoire et des piles toutes les 5 s ; piles agrandies.], [143 trames en 8 s, 28 à zéro, puis blocage. Mémoire saine (environ 160 Ko libres), piles à peine utilisées : *pas de débordement*. Le blocage vient du module : un envoi reste coincé une dizaine de secondes, puis une erreur `Failed to send command` et des délais dépassés (`TIMEOUT`) à chaque question posée au module.],
  [v4, écran figé], [Même firmware, rafraîchissement de l'écran coupé par la console.], [136 trames en 6 s, 14 à zéro, même blocage : couper le dessin ne suffit pas.],
  [v4, sans écran], [Variante où l'écran n'est jamais initialisé.], [88 trames, 6 à zéro, même blocage : *l'écran n'est pas en cause*.],
  [Firmware ST d'origine], [Retour au firmware de test de ST, `iperf` de 60 s.], [*59,5 Mo, 8,3 Mbit/s, aucun blocage* : ni le module en général, ni le réseau, ni l'écran.],
  [Trames à zéro], [Démo sans écran qui n'envoie que des zéros, comme iperf.], [Plante aussi après environ 6 s : le *contenu* des données n'est pas la cause.],
  [Blocs de 1 520 octets], [Envois de 1 520 octets (taille d'iperf) au lieu de 9 Ko.], [*Plus aucune trame à zéro* : elles venaient des gros envois de 9 Ko d'un coup. Le module tient environ 25 s puis décroche encore.],
  [v5], [Blocs de 1 520 octets par défaut, écran rétabli, réparation automatique (coupure du module 0,2 s via `CHIP_EN`, puis redémarrage).], [25 trames/s, environ 1,8 Mbit/s, plus aucune trame à zéro. Le module décroche toutes les 3 à 10 s ; la reprise prend 20 à 30 s. Meilleur essai : 301 trames en 17 s.],
  [v6], [Tâche d'envoi en priorité 40 (comme iperf) au lieu de 16 ; relance immédiate du module.], [Le décrochage persiste (environ 10 s) : la priorité n'était pas la cause. *Puis, en passant l'envoi en continu par la console : 333 s sans décrochage* (voir ci-dessous).],
  [v7], [Envoi continu dès le démarrage ; panneau d'écran élargi, compteur « saut. » retiré, espace rétabli après « Erreurs ».], [Compilée, signée et flashée. Test sur batterie non réalisé (Mac sorti du réseau).],
)

== Le résultat clé : envoi continu

Avec l'envoi continu (pas de pause entre les trames, comme le fait iperf), mesuré sur le kit avec écran allumé :

- durée *333 s* (5,5 minutes), jusqu'au débranchement de l'USB ;
- *10 225 trames* reçues par le Mac, 93 Mo, *2,23 Mbit/s en moyenne*, environ 30 trames/s ;
- *aucun décrochage, aucune erreur, aucune reconnexion*.

La #ref(<fig-final>) montre l'écran pendant cet essai : « connecté » sur le réseau de test, signal de -28 dBm, lien TCP en envoi, 4 539 trames, 2,64 Mbit/s (36,4 trames/s), 0 erreur, 0 reconnexion, 2 min 27 de fonctionnement. La caméra du kit, branchée en haut de la photo, est restée en place pendant tous les essais.

#figure(
  image("img/ecran_envoi_continu.jpg", width: 85%),
  caption: [Démo en envoi continu : panneau d'état au-dessus de la carte de profondeur simulée (la boule bouge). 4 539 trames envoyées en 2 min 27, 0 reconnexion.],
) <fig-final>

*Explication la plus probable (hypothèse, non testée).* Le module Wi-Fi décroche quand il reçoit des données par à-coups, avec des pauses entre les trames, peut-être parce qu'il passe en économie d'énergie pendant les pauses et se réveille mal. En flux continu, il ne s'endort jamais.

== Essai sur batterie

Avec la version 6 (envoi par à-coups), le kit branché sur une batterie externe, par le port USB-C du ST-LINK, s'est connecté au réseau puis au Mac de lui-même, sans aucun câble vers le Mac : 251 trames reçues en environ 10 s, 27 à 28 trames/s, environ 2 Mbit/s, 0 trame corrompue. Le module a ensuite décroché comme prévu pour cette version. La version 7 doit éviter ce décrochage, mais n'a pas encore été essayée sur batterie.

#utilite(
  [La démo reproduit la contrainte réelle du scanner : un flux continu de 2 Mbit/s vers un PC, un afficheur en parallèle, un fonctionnement autonome sur batterie. Elle a servi à débusquer le problème du module avant d'avoir la carte A.],
  [Quatre règles à appliquer au firmware du prototype A. (1) *Envoyer les scans en flux continu*, par blocs de 1 520 octets, jamais par rafales de plusieurs kilo-octets avec des pauses. (2) *Garder la tâche d'envoi en haute priorité*, comme iperf, par prudence. (3) *Prévoir la réparation automatique* du module (coupure puis redémarrage par `CHIP_EN`), car un Reset du STM32 ne remet pas le module à zéro. (4) *Réutiliser le module d'écran* de la démo (deux couches LTDC, panneau d'état semi-transparent) : le fond est déjà prévu pour afficher la vue de la caméra du kit, qui servira de retour d'image. Les couches de l'afficheur ont chacune leur autorisation d'accès mémoire sur le N6 : ne pas l'oublier.],
)

// ───────────── 9 ─────────────
= Ce qu'on retient pour le prototype A

#table(
  columns: (1.4fr, 2.6fr),
  stroke: 0.5pt + luma(190), inset: 6pt,
  [*Sujet*], [*Conclusion*],
  [Compatibilité], [Le ST67W611M1 fonctionne avec le STM32N6 via le pilote `ST67W6X_Network_Driver` de ST (pack x-cube-st67w61 v1.3.0).],
  [Débit], [8,3 Mbit/s mesurés, 2,2 Mbit/s nécessaires : marge d'un facteur 3,8, critère « au moins 2 fois le lidar » rempli.],
  [Flux scanner vers PC], [TCP, envoi continu, blocs de 1 520 octets.],
  [Commandes PC vers scanner], [UDP ou MQTT (le TCP entrant est bloqué par le partage de connexion utilisé).],
  [Broches de la carte A], [`SPI_RDY` sur D2 (PD0, EXTI0), `CHIP_EN` sur D7 (PD6), `BOOT` non relié au N6 : adapter les définitions de `main.h`.],
  [Firmware du module], [Version 2.0.89 en place, aucun défaut établi ; ST demande 2.0.106. La mise à jour est *irréversible* (verrouillage du module) et n'est utile que si le décrochage revenait en flux continu.],
  [Écran et caméra], [Le kit garde son écran et sa caméra ; le module d'écran de la démo est réutilisable pour le retour caméra.],
  [Procédure de programmation], [BOOT1 côté H pour flasher, côté L pour démarrer, coupure USB complète à chaque changement.],
)

= Reste à faire

+ Tester la version 7 sur batterie avec le Mac connecté au réseau avant d'allumer le kit.
+ Rendre le réseau, le mot de passe et l'adresse du PC modifiables sans recompiler, ou faire découvrir le PC automatiquement (l'adresse du Mac a changé plusieurs fois dans la séance).
+ Confirmer l'hypothèse d'économie d'énergie du module, et décider d'une éventuelle mise à jour du firmware du module vers 2.0.106 seulement si nécessaire.
+ Mesurer le vrai flux du lidar au test 3, pour remplacer l'estimation de 270 Ko/s.
+ Pousser les derniers commits sur GitHub (les commits de la version 7 étaient encore locaux en fin de séance).

#pagebreak()

= Annexe : commandes utilisées

*Console du kit (921 600 bauds)*

```
info
wifi_scan
wifi_scan -s "<nom du réseau>"
wifi_sta_connect "<nom du réseau>" <mot de passe>
wifi_sta_state
net_sta_ip
ping google.com -c 4 -s 64 -i 1000
iperf -c <IP du Mac> -t 10 -i 1          # TCP
iperf -c <IP du Mac> -u -b 20 -t 10 -i 1 # UDP
iperf -c <IP du Mac> -t 60 -i 10         # TCP, 60 s
iperf -s        # serveur ; iperf -a pour l'arrêter
echostart 5000  # écho UDP ; echostop pour l'arrêter
mqtt_configure -s 0
```

*Sur le Mac*

```
python3 port_STM32N6570-DK.py Projects/NUCLEO-N657X0-Q/Applications/ST67W6X/ST67W6X_CLI
STM32_SigningTool_CLI -nk -of 0x80000000 -t fsbl -hv 2.3 --align ...
STM32_Programmer_CLI -c port=swd mode=ur -w ST67W6X_CLI_FSBL-trusted.bin 0x70000000 \
  --extload ".../MX66UW1G45G_STM32N6570-DK.stldr"
nc -u <IP du kit> 5000   # test d'écho UDP
```

*Fichiers du projet*

#table(
  columns: (auto, 1fr),
  stroke: 0.5pt + luma(190), inset: 5pt,
  [`GUIDE_test_X-NUCLEO-67W61M1.md`], [Guide de départ : dépôt, câblage, firmware du module, séquence de test, dépannage.],
  [`port_STM32N6570-DK.py`], [Script de portage du projet ST au kit DK.],
  [`ST67W6X_CLI_DK.zip`], [Projet ST67W6X_CLI déjà porté au DK.],
  [`RESULTATS_test_wifi_DK.md`], [Résumé des résultats de la première partie de séance.],
  [`docs/test_wifi_DK/` (dépôt, branche `module-wifi`)], [Résultats complets, patch, script, dossier `demo_scan3d/` (firmware, module d'écran, récepteur PC, README).],
)
