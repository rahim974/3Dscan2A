/* Modèle : copier en wifi_secrets.h (ignoré par git) et remplir.
 * Ne jamais committer wifi_secrets.h. */
#ifndef WIFI_SECRETS_H
#define WIFI_SECRETS_H

#define SCAN3D_WIFI_SSID      "NomDuReseau"     /* réseau 2,4 GHz uniquement */
#define SCAN3D_WIFI_PASSWORD  "MotDePasse"

/* PC qui reçoit les trames (lancer pc/recepteur_scan3d.py dessus) */
#define SCAN3D_PC_IP          "192.168.1.10"
#define SCAN3D_PC_PORT        5002

#endif /* WIFI_SECRETS_H */
