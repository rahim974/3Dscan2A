"""PCB de départ de la carte fille A : contour shield Arduino Uno R3, barrettes en place,
empreintes pré-placées par bloc et reliées au schéma (chemins des symboles).

Usage (Python de KiCad) :
  /Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3 pcb_A.py
Lancer design_A.py juste avant : les chemins changent à chaque génération du schéma.
"""
import os, json
import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
PRJ = os.path.dirname(HERE)
OUT = os.path.join(PRJ, 'carte_fille_A.kicad_pcb')
FPLIB = '/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints'
mm = pcbnew.FromMM

# Origine = broche 1 de CN8. Cotes reprises de l'empreinte Module:Arduino_UNO_R3, mais avec les rangées
# inversées : vue de dessus de la carte fille = face du kit qui porte les connecteurs (photo du 03/11/2026),
# soit CN8/CN7 en bas (NC à gauche) et D15...D0 en haut (D0 à droite), comme une Uno vue de dessus.
# L'empreinte KiCad, elle, place l'alimentation en haut : utilisée telle quelle, la carte serait en miroir.
OX, OY = 100.0, 100.0
EDGE = (-27.94, -2.54, 40.64, 50.8)          # Uno R3 : 68,58 x 53,34 mm
HEADERS = {                                  # ref : (x broche 1, y, pas en x)
    'J1': (0.0, 48.26, +2.54),                 # CN8 alimentation (NC, IOREF, RESET, 3V3, 5V, GND, GND, VIN)
    'J2': (22.86, 48.26, +2.54),               # CN7 A0-A5
    'J3': (35.56, 0.0, -2.54),             # CN11 D0-D7
    'J4': (13.72, 0.0, -2.54),             # CN12 D8-D13, GND, AREF, D14, D15
}
# Placement des composants principaux (x, y relatifs à l'origine, y compté depuis la rangée CN8 vers les D, angle)
MAIN = {
    'IC1': (30.0, 26.0, 270),   # Wi-Fi au bord côté CN7, antenne vers l'extérieur (sens à vérifier sur l'empreinte)
    'J5': (24.0, 15.0, 90),
    'H1': (-20.0, 15.0, 0), 'H2': (-20.0, 33.0, 0),  # module caméra côté nappe (entraxe à mesurer)
    'U4': (-4.0, 24.0, 0),      # lidar à côté de la caméra
    'U5': (9.0, 19.0, 0), 'U6': (9.0, 25.0, 0), 'U7': (9.0, 31.0, 0), 'U8': (9.0, 37.0, 0),
    'Y1': (-4.0, 33.0, 0),
    'U9': (-4.0, 40.0, 0),      # IMU près du lidar
    'U1': (11.0, 7.5, 0), 'U2': (21.0, 6.0, 0), 'U3': (27.0, 6.0, 0),
}
# Point de départ de la recherche de place des autres composants, par feuille
ANCHOR = {'Alimentation': (4.0, 6.0), 'Lidar': (2.0, 30.0), 'IMU': (2.0, 42.0),
          'WiFi BLE': (20.0, 30.0), 'Connecteurs Arduino': (-6.0, 4.0), 'Mecanique': (-20.0, 24.0)}
ANCHOR_REF = {'TP2': (-24.0, 4.0), 'TP3': (-20.0, 4.0), 'TP4': (-16.0, 4.0), 'TP5': (-12.0, 4.0), 'TP6': (-24.0, 31.0)}
GAP = 0.25

def load(fpid):
    lib, name = fpid.split(':')
    path = os.path.join(PRJ, 'libs', 'custom_fp.pretty') if lib == 'custom_fp' else os.path.join(FPLIB, lib + '.pretty')
    fp = pcbnew.FootprintLoad(path, name)
    if fp is None: raise SystemExit(f'empreinte introuvable : {fpid}')
    return fp

ROW = 48.26                                  # distance entre les deux rangées de barrettes
def place(fp, x, y, ang=0):
    """y compté depuis la rangée CN8 (en bas) vers la rangée D0-D15 (en haut)."""
    fp.SetOrientationDegrees(ang)
    fp.SetPosition(pcbnew.VECTOR2I(mm(OX + x), mm(OY + ROW - y)))

def place_header(fp, x1, y1, step):
    """Barrette côté B (broches vers le kit), broche 1 en (x1, y1), broches suivantes vers x + step."""
    fp.Flip(fp.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
    for ang in (0, 90, 180, 270):
        fp.SetOrientationDegrees(ang)
        p1, p2 = fp.FindPadByNumber('1').GetPosition(), fp.FindPadByNumber('2').GetPosition()
        if abs(p2.y - p1.y) < 10 and (p2.x - p1.x) * step > 0: break
    p1 = fp.FindPadByNumber('1').GetPosition()
    fp.Move(pcbnew.VECTOR2I(mm(OX + x1) - p1.x, mm(OY + y1) - p1.y))

board = pcbnew.CreateEmptyBoard()
board.SetCopperLayerCount(2)
nc = board.GetDesignSettings().m_NetSettings.GetDefaultNetclass()
nc.SetClearance(mm(0.127)); nc.SetTrackWidth(mm(0.15))      # pas fins : IMU LGA 0,5 mm, P3A9606
parts = json.load(open(os.path.join(HERE, 'netlist_A.json')))
nets = {}
def net(name):
    if name not in nets:
        n = pcbnew.NETINFO_ITEM(board, name); board.Add(n); nets[name] = n
    return nets[name]

placed = []                                   # boîtes occupées (mm, repère carte)
def bbox(fp):
    """Boîte des pads et du courtyard, en mm."""
    boxes = [pad.GetBoundingBox() for pad in fp.Pads()]
    boxes += [g.GetBoundingBox() for g in fp.GraphicalItems()
              if g.GetLayer() in (pcbnew.F_CrtYd, pcbnew.B_CrtYd)]
    if not boxes: boxes = [fp.GetBoundingBox(False)]
    T = pcbnew.ToMM
    return [min(T(x.GetX()) for x in boxes), min(T(x.GetY()) for x in boxes),
            max(T(x.GetRight()) for x in boxes), max(T(x.GetBottom()) for x in boxes)]

def free(b):
    x0, y0, x1, y1 = EDGE
    if b[0] < OX + x0 + 0.8 or b[1] < OY + y0 + 0.8 or b[2] > OX + x1 - 0.8 or b[3] > OY + y1 - 0.8: return False
    return all(b[2] + GAP <= o[0] or o[2] + GAP <= b[0] or b[3] + GAP <= o[1] or o[3] + GAP <= b[1] for o in placed)

def autoplace(fp, ax, ay):
    place(fp, ax, ay)
    for r in range(0, 120):
        rr = r * 0.5
        cand = [(dx, dy) for dx in [i * 0.5 for i in range(-r, r + 1)] for dy in (-rr, rr)] + \
               [(dx, dy) for dy in [i * 0.5 for i in range(-r + 1, r)] for dx in (-rr, rr)]
        for dx, dy in sorted(cand, key=lambda c: c[0] ** 2 + c[1] ** 2):
            place(fp, ax + dx, ay + dy)
            b = bbox(fp)
            if free(b): placed.append(b); return
    print('  ! pas de place pour', fp.GetReference())

later = []
for p in parts:
    lib, name = p['fp'].split(':')
    fp = load(p['fp'])
    fp.SetFPID(pcbnew.LIB_ID(lib, name))
    fp.SetReference(p['ref']); fp.SetValue(p['value'])
    fp.SetPath(pcbnew.KIID_PATH(p['path']))
    fp.SetSheetname(p['sheet']); fp.SetSheetfile(p['sheetfile'])
    if p['datasheet']:
        try: fp.GetField(pcbnew.FIELD_T_DATASHEET).SetText(p['datasheet'])
        except Exception: fp.SetField('Datasheet', p['datasheet'])
    if p['dnp']: fp.SetDNP(True)
    fp.SetExcludedFromBOM(False)
    board.Add(fp)
    for pad in fp.Pads():
        num = pad.GetNumber()
        if not num: continue
        n = p['pins'].get(num)
        if n is None:
            n = f"unconnected-({p['ref']}-{p['pinnames'].get(num, '')}-Pad{num})"
        pad.SetNet(net(n))
    r = p['ref']
    if r in HEADERS: place_header(fp, *HEADERS[r]); placed.append(bbox(fp))
    elif r in MAIN: place(fp, *MAIN[r]); placed.append(bbox(fp))
    else: later.append((fp, p))
for fp, p in later:
    autoplace(fp, *ANCHOR_REF.get(p['ref'], ANCHOR[p['sheet']]))

# contour
x0, y0, x1, y1 = EDGE
pts = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
for a, b in zip(pts, pts[1:] + pts[:1]):
    seg = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_SEGMENT)
    seg.SetStart(pcbnew.VECTOR2I(mm(OX + a[0]), mm(OY + a[1])))
    seg.SetEnd(pcbnew.VECTOR2I(mm(OX + b[0]), mm(OY + b[1])))
    seg.SetLayer(pcbnew.Edge_Cuts); seg.SetWidth(mm(0.1)); board.Add(seg)

# plan de masse sur B.Cu
z = pcbnew.ZONE(board)
z.SetLayer(pcbnew.B_Cu); z.SetNet(net('GND')); z.SetZoneName('GND')
ol = z.Outline(); ol.NewOutline()
for a in pts: ol.Append(mm(OX + a[0] + 0.3) if a[0] < 0 else mm(OX + a[0] - 0.3), mm(OY + a[1] + 0.3) if a[1] < 0 else mm(OY + a[1] - 0.3))
board.Add(z)

# zone sans cuivre sous l'antenne du module Wi-Fi (DS14784 §6.2), toutes couches
ic1 = board.FindFootprintByReference('IC1')
P = lambda n: ic1.FindPadByNumber(n).GetPosition()
p32, p25 = P('32'), P('25')                    # pastilles 32 et 25 : (-4,445 ; -3,216) et (4,445 ; -3,216) en local
ux, uy = (p25.x - p32.x) / 8.89, (p25.y - p32.y) / 8.89
vx, vy = -uy, ux                               # axe y local, sens vérifié avec la pastille 1
p1 = P('1')
if (p1.x - p32.x) * vx + (p1.y - p32.y) * vy < 0: vx, vy = -vx, -vy
def loc(x, y):
    return pcbnew.VECTOR2I(int(p32.x + (x + 4.445) * ux + (y + 3.216) * vx),
                           int(p32.y + (x + 4.445) * uy + (y + 3.216) * vy))
ka = pcbnew.ZONE(board)
ka.SetIsRuleArea(True); ka.SetZoneName('antenne Wi-Fi')
ka.SetLayerSet(pcbnew.LSET.AllCuMask())
ka.SetDoNotAllowZoneFills(True); ka.SetDoNotAllowTracks(True); ka.SetDoNotAllowVias(True)
ka.SetDoNotAllowPads(True); ka.SetDoNotAllowFootprints(False)
ol = ka.Outline(); ol.NewOutline()
for x, y in ((-6.14, -8.64), (6.14, -8.64), (6.14, -3.70), (-6.14, -3.70)):
    v = loc(x, y); ol.Append(v.x, v.y)
board.Add(ka)

# textes d'aide
def note(txt, x, y, layer=pcbnew.F_SilkS, size=1.0):
    t = pcbnew.PCB_TEXT(board); t.SetText(txt); t.SetLayer(layer)
    t.SetPosition(pcbnew.VECTOR2I(mm(OX + x), mm(OY + ROW - y)))
    t.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size))); t.SetTextThickness(mm(size * 0.15)); board.Add(t)
note('CARTE FILLE A - scanner 3D', 14.0, 41.5, size=0.8)
for txt, x, y in (('CN8', 8.9, 2.2), ('CN7 A0-A5', 29.2, 2.2), ('CN12 D8-D15', 2.3, 46.0), ('CN11 D0-D7', 26.7, 46.0)):
    note(txt, x, y, size=0.8)
note('Face F : vers la scene (lidar, camera). Barrettes sur B, vers le kit.', 6.0, 56.0, pcbnew.Cmts_User)
note('Antenne Wi-Fi : bord droit (cote CN7), zone sans cuivre a definir (datasheet ST67W611M1).', 6.0, 58.0, pcbnew.Cmts_User)
note('Trous camera H1/H2 (bord gauche, cote nappe) : entraxe a mesurer sur le module B-CAMS-IMX.', 6.0, 60.0, pcbnew.Cmts_User)

board.Save(OUT)
print('ok', OUT, len(parts), 'empreintes,', len(nets), 'nets')
