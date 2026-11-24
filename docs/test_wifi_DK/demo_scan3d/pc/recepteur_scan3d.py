#!/usr/bin/env python3
"""Récepteur PC des trames lidar simulées envoyées par le DK (démo Scanner 3D).

Usage :  python3 recepteur_scan3d.py [port] [--save dossier]
  - écoute en TCP (port 5002 par défaut) ;
  - vérifie chaque trame (en-tête "S3DF", taille, numéros consécutifs) ;
  - affiche chaque seconde : trames/s, débit, trames perdues, distance au centre ;
  - --save : enregistre la dernière trame reçue en CSV (distance en mm) toutes les 5 s.
Aucune dépendance (Python 3 standard).

Format : en-tête 20 octets <4s I I H H I> = magic, numéro, temps_ms, largeur, hauteur,
taille ; puis largeur*hauteur zones de <H B B> = distance_mm, confiance, statut.
"""
import socket, struct, sys, time, pathlib

sys.stdout.reconfigure(line_buffering=True)
HDR = struct.Struct("<4sIIHHI")
port = 5002
save_dir = None
args = sys.argv[1:]
if "--save" in args:
    i = args.index("--save"); save_dir = pathlib.Path(args[i + 1]); del args[i:i + 2]
    save_dir.mkdir(parents=True, exist_ok=True)
if args:
    port = int(args[0])


def _inutilise(c, n):
    b = bytearray()
    while len(b) < n:
        d = c.recv(n - len(b))
        if not d:
            raise ConnectionError("connexion fermée par le DK")
        b += d
    return bytes(b)


def save_csv(w, h, payload, path):
    with open(path, "w") as f:
        for y in range(h):
            row = [struct.unpack_from("<H", payload, (y * w + x) * 4)[0] for x in range(w)]
            f.write(",".join(map(str, row)) + "\n")


srv = socket.socket()
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("0.0.0.0", port))
srv.listen(1)
print(f"En attente du DK sur le port TCP {port}... (Ctrl+C pour arrêter)")

class Flux:
    """Lecture tamponnée du socket, avec resynchronisation sur l'en-tête "S3DF"."""
    def __init__(self, c):
        self.c, self.buf = c, bytearray()

    def lire(self, n):
        while len(self.buf) < n:
            d = self.c.recv(65536)
            if not d:
                raise ConnectionError("connexion fermée par le DK")
            self.buf += d
        out = bytes(self.buf[:n]); del self.buf[:n]
        return out

    def resync(self):
        """Jette les octets jusqu'au prochain "S3DF". Retourne (jetés, dont nuls)."""
        jetes = nuls = 0
        while True:
            i = self.buf.find(b"S3DF")
            if i >= 0:
                nuls += self.buf[:i].count(0); jetes += i; del self.buf[:i]
                return jetes, nuls
            k = max(0, len(self.buf) - 3)
            nuls += self.buf[:k].count(0); jetes += k; del self.buf[:k]
            d = self.c.recv(65536)
            if not d:
                raise ConnectionError("connexion fermée par le DK")
            self.buf += d


while True:
    c, addr = srv.accept()
    print(f"DK connecté depuis {addr[0]}")
    c.settimeout(5.0)   # plus rien depuis 5 s : connexion morte, on attend la suivante
    f = Flux(c)
    t0 = t_ref = time.time(); n = n_ref = 0; octets = oct_ref = 0
    perdues = 0; dernier = None; t_save = 0; resyncs = 0; parasites = 0
    try:
        while True:
            magic, idx, t_ms, w, h, size = HDR.unpack(f.lire(HDR.size))
            if magic != b"S3DF" or size != w * h * 4:
                f.buf[0:0] = struct.pack("<4sIIHHI", magic, idx, t_ms, w, h, size)
                jetes, nuls = f.resync()
                resyncs += 1; parasites += jetes
                print(f"  ! flux désaligné après la trame {dernier} : {jetes} octets jetés "
                      f"(dont {nuls} nuls) avant le prochain en-tête")
                continue
            payload = f.lire(size)
            if dernier is not None and idx != dernier + 1:
                perdues += idx - dernier - 1
            dernier = idx; n += 1; octets += HDR.size + size
            now = time.time()
            if now - t_ref >= 1.0:
                dt = now - t_ref
                centre = struct.unpack_from("<H", payload, ((h // 2) * w + w // 2) * 4)[0]
                print(f"[{now - t0:6.1f} s] trame {idx:6d} | {(n - n_ref) / dt:5.1f} trames/s | "
                      f"{(octets - oct_ref) * 8 / dt / 1e6:5.2f} Mbit/s | perdues {perdues} | "
                      f"resync {resyncs} | {w}x{h} | centre {centre} mm")
                t_ref, n_ref, oct_ref = now, n, octets
            if save_dir and now - t_save >= 5.0:
                save_csv(w, h, payload, save_dir / "derniere_trame.csv"); t_save = now
    except (ConnectionError, OSError) as e:
        dt = time.time() - t0
        print(f"Fin : {e}. {n} trames, {octets / 1e6:.2f} Mo en {dt:.1f} s "
              f"({octets * 8 / max(dt, 1e-3) / 1e6:.2f} Mbit/s moyen), perdues {perdues}, "
              f"resynchronisations {resyncs} ({parasites} octets jetés).")
        c.close()
