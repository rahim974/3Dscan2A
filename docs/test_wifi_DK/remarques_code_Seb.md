# Remarques sur le code de Seb pour l'intégration du Wi-Fi

Ces remarques portent sur la branche `Note-de-Seb` (caméra IMX335 et lidar simulé, état du 24/11/2026). Elles visent le prototype A : une carte fille posée sur le kit STM32N6570-DK, qui porte le module Wi-Fi ST67W611M1 et le lidar, et qui garde l'écran et la caméra du kit. Le module y est relié au même SPI que lors de nos essais : les remarques s'appliquent donc au prototype A.

**Il faudra passer à FreeRTOS.** Le pilote Wi-Fi de ST ne fonctionne qu'avec ce système d'exploitation, alors que le code de Seb tourne dans une simple boucle `while (1)`. Il faudra en faire des tâches séparées pour la caméra, le lidar, la fusion et l'envoi Wi-Fi. Sa règle actuelle, qui veut que chaque module ne soit appelé que d'un seul endroit, rend ce passage simple.

**Il faudra refaire le plan mémoire.** Le code de Seb et le projet Wi-Fi rangent leurs données à des adresses différentes, qui ne peuvent pas être réunies telles quelles. En particulier, la photo figée de la caméra (à partir de `0x342E0000`) chevauche le panneau d'état de la démo Wi-Fi (de `0x342C0000` à `0x3437B800`) : il faudra déplacer l'une des deux zones.

**Le débit Wi-Fi est suffisant.** Le lidar au format de Seb produit environ 1,3 Mbit/s, alors que le Wi-Fi a tenu 8,3 Mbit/s. Une photo complète de la caméra met en revanche près d'une seconde à partir : on pourra en envoyer à la demande, mais pas en vidéo continue.

**Il faudra envoyer en continu.** Nos essais ont montré que le module décroche si on lui envoie les données avec des pauses, et que des données arrivent à zéro si on lui confie plus de 1 520 octets à la fois. Les scans devront donc passer par une file d'attente vidée sans pause par la tâche d'envoi, par blocs de 1 520 octets au plus.

**Trois points à vérifier sur la carte A.** Les broches SPI_RDY (D2, PD0) et CHIP_EN (D7, PD6) changent : il faudra adapter `main.h`. Le module soudé peut avoir un autre firmware que celui de nos essais (2.0.89) : il faudra lire sa version avec `info` et refaire un court essai d'envoi continu. Enfin, le projet caméra ne règle pas l'horloge PLL4 que la démo utilise pour l'écran : il faudra l'ajouter si l'on reprend le panneau d'état.
