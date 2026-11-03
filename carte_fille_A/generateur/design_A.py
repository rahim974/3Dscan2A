"""Schéma de la carte fille du prototype A (shield Arduino sous le STM32N6570-DK).

Usage : python3 design_A.py ..   (écrit les feuilles dans carte_fille_A/)
"""
import os, sys, json
from kgen import Project, POWER
import libs
from sexp import find, uq

OUT = sys.argv[1]
P = Project()
P.title = 'Carte fille prototype A - scanner 3D'
P.force_global = {
    'ARD_SCL', 'ARD_SDA', 'ARD_A4', 'ARD_A5', 'IOREF',
    'LIDAR_SYNC_3V3', 'LIDAR_INT_3V3', 'LIDAR_XSHUT_3V3',
    'IMU_INT1', 'IMU_INT2',
    'WIFI_EN', 'WIFI_RDY', 'WIFI_CS', 'WIFI_MOSI', 'WIFI_SCK', 'WIFI_MISO', 'ARD_D0_RX', 'ARD_D1_TX'}

R = 'Device:R_Small'; C = 'Device:C_Small'; LED = 'Device:LED_Small'
FR = 'Resistor_SMD:R_0402_1005Metric'
FC = 'Capacitor_SMD:C_0402_1005Metric'
FC6 = 'Capacitor_SMD:C_0603_1608Metric'
FC8 = 'Capacitor_SMD:C_0805_2012Metric'
TP = 'TestPoint:TestPoint_Pad_D1.5mm'
HDR = 'Connector_PinHeader_2.54mm:PinHeader_1x{:02d}_P2.54mm_Vertical'

class Grid:
    def __init__(self, x0, y0, dx=15.24, cols=12, dy=48.26):
        self.x0, self.y0, self.dx, self.cols, self.dy, self.i = x0, y0, dx, cols, dy, 0
    def __call__(self):
        x = self.x0 + (self.i % self.cols) * self.dx; y = self.y0 + (self.i // self.cols) * self.dy
        self.i += 1; return x, y

def res(sh, g, val, a, b, dnp=False, fp=FR):
    x, y = g(); return P.part(sh, R, 'R', val, x, y, {'1': a, '2': b}, fp=fp, dnp=dnp)
def cap(sh, g, val, a, b='GND', fp=None):
    x, y = g()
    if fp is None: fp = FC8 if val in ('22uF', '47uF') else FC6 if val in ('10uF', '4.7uF') else FC
    return P.part(sh, C, 'C', val, x, y, {'1': a, '2': b}, fp=fp)
def testpoint(sh, g, net):
    x, y = g(); return P.part(sh, 'Connector:TestPoint', 'TP', net, x, y, {'1': net}, fp=TP)

# ======================================================================
# 1. CONNECTEURS ARDUINO DU KIT
# ======================================================================
s = P.sheet('Connecteurs Arduino', 'connecteurs.kicad_sch')
s.text('BARRETTES ARDUINO UNO R3 (males) - la carte s enfiche SOUS le STM32N6570-DK (face opposee a l ecran).\n'
       'Reperes du kit (photos du 03/11/2026, a confirmer) : CN8 alimentation, CN7 A0-A5, CN11 D0-D7, CN12 D8-D15.\n'
       'Broches du N6 d apres le schema MB1939 page 3 :\n'
       '  D0 PF6, D1 PD5, D2 PD0, D3 PE9, D4 PH5, D5 PE10, D6 PE13, D7 PD6\n'
       '  D8 PE7, D9 PE14, D10 PA3 (SPI5_NSS), D11 PG2 (SPI5_MOSI), D12 PH8 (SPI5_MISO), D13 PE15 (SPI5_SCK)\n'
       '  D14/D15 : I2C1 PC1/PH9 a priori (pull-ups 1,5k au VDDIO sur le kit), A4 PF3, A5 PB10\n'
       'A VERIFIER (CubeMX) : I3C disponible sur D14/D15 ? sinon option A4/A5 (resistances 0R DNP, feuille Lidar).\n'
       'A VERIFIER : timer sur PE9 (SYNC_IN), EXTI distinctes (PD0, PH5, PE10, PE13), conflits avec les fonctions du kit.\n'
       'Alimentation : seul le 5 V du kit est utilise (LDO 3,3 V local). Le 3V3 du kit et VIN ne sont pas utilises.',
       20.32, 15.24)
P.part(s, 'Connector:Conn_01x08_Pin', 'J', 'CN8 alim', 50.8, 101.6, {
    '2': 'IOREF', '5': '+5V', '6': 'GND', '7': 'GND'}, nc=('1', '3', '4', '8'), fp=HDR.format(8))
P.part(s, 'Connector:Conn_01x06_Pin', 'J', 'CN7 A0-A5', 50.8, 160.02, {
    '5': 'ARD_A4', '6': 'ARD_A5'}, nc=('1', '2', '3', '4'), fp=HDR.format(6))
P.part(s, 'Connector:Conn_01x08_Pin', 'J', 'CN11 D0-D7', 152.4, 101.6, {
    '1': 'ARD_D0_RX', '2': 'ARD_D1_TX', '3': 'WIFI_RDY', '4': 'LIDAR_SYNC_3V3', '5': 'LIDAR_INT_3V3',
    '6': 'IMU_INT1', '7': 'IMU_INT2', '8': 'WIFI_EN'}, fp=HDR.format(8))
P.part(s, 'Connector:Conn_01x10_Pin', 'J', 'CN12 D8-D15', 152.4, 165.1, {
    '1': 'LIDAR_XSHUT_3V3', '3': 'WIFI_CS', '4': 'WIFI_MOSI', '5': 'WIFI_MISO', '6': 'WIFI_SCK',
    '7': 'GND', '9': 'ARD_SDA', '10': 'ARD_SCL'}, nc=('2', '8'), fp=HDR.format(10))
g = Grid(228.6, 101.6, cols=6)
testpoint(s, g, 'IOREF')
for net, xx in (('+5V', 254), ('GND', 274.32)):
    P.flag(s, net, xx, 165.1)

# ======================================================================
# 2. ALIMENTATION
# ======================================================================
s = P.sheet('Alimentation', 'alimentation.kicad_sch')
s.text('ALIMENTATION - tout vient du 5 V du kit (CN8 broche 5).\n'
       '+5V -> LD39150DT33 (LDO 1,5 A) -> +3V3 : Wi-Fi (VDD33 + VDDIO), lidar (AVDD, VBAT_LDD, VBAT_RX), IMU, translateurs cote kit.\n'
       '+3V3 -> LDK130-18 -> +1V8 (IOVDD lidar, oscillateur, translateurs cote lidar) | +3V3 -> LDK130-12 -> +1V2 (DVDD lidar).\n'
       'Pics : lidar ~0,75 A + Wi-Fi ~0,4 A. LDO 3V3 : (5-3,3) x 0,5 A moyen = 0,85 W dans le DPAK -> plan de cuivre.\n'
       'A VERIFIER : courant disponible sur le 5 V de l Arduino du kit (sinon kit alimente par USB et 5 V suffisant ?).',
       20.32, 15.24)
P.part(s, 'Regulator_Linear:LD39150DT33', 'U', 'LD39150DT33', 76.2, 96.52, {
    '1': '+5V', '3': '+3V3', '2': 'GND'})
P.part(s, 'Regulator_Linear:LDK130-18_SOT23_SOT353', 'U', 'LDK130-18', 152.4, 96.52, {
    '1': '+3V3', '3': '+3V3', '5': '+1V8', '2': 'GND'}, nc=('4',), fp='Package_TO_SOT_SMD:SOT-23-5')
P.part(s, 'Regulator_Linear:LDK130-12_SOT23_SOT353', 'U', 'LDK130-12', 228.6, 96.52, {
    '1': '+3V3', '3': '+3V3', '5': '+1V2', '2': 'GND'}, nc=('4',), fp='Package_TO_SOT_SMD:SOT-23-5')
g = Grid(20.32, 154.94)
cap(s, g, '10uF', '+5V')
cap(s, g, '22uF', '+3V3')
cap(s, g, '22uF', '+3V3')
cap(s, g, '100nF', '+3V3')
cap(s, g, '1uF', '+3V3')
cap(s, g, '1uF', '+1V8')
cap(s, g, '1uF', '+3V3')
cap(s, g, '1uF', '+1V2')
res(s, g, '1k', '+3V3', 'LED_PWR_A')
x, y = g(); P.part(s, LED, 'D', 'LED 3V3 verte', x, y, {'2': 'LED_PWR_A', '1': 'GND'}, fp='LED_SMD:LED_0603_1608Metric')
g = Grid(20.32, 220.98)
for net in ('+5V', '+3V3', '+1V8', '+1V2', 'GND'): testpoint(s, g, net)

# ======================================================================
# 3. LIDAR + TRANSLATEURS
# ======================================================================
s = P.sheet('Lidar', 'lidar.kicad_sch')
s.text('LIDAR VL53L9CX - IOVDD = 1,8 V, DVDD = 1,2 V, AVDD = 3,3 V, VBAT_LDD / VBAT_RX = +3V3 (plage 2,8-4,8 V).\n'
       'Le kit est en 3,3 V : translateurs entre le kit et le lidar.\n'
       '  I3C SCL/SDA : NXP P3A9606 (translateur I3C 12,5 MHz). BROCHAGE ET BOITIER DU SYMBOLE A VERIFIER (datasheet NXP).\n'
       '  XSHUT, SYNC_IN (kit -> lidar) et INTR (lidar -> kit) : SN74LVC1T45, sens fixe par DIR.\n'
       'Choix des broches I3C du kit par resistances 0R : D14/D15 montees, A4/A5 non montees (DNP).\n'
       'L IMU partage le bus D14/D15 (le LSM6DSV16X accepte l I3C). Repli I2C du lidar : pull-ups 2,2k DNP cote 1,8 V.\n'
       'AP_CLK : oscillateur 24 MHz 1,8 V dedie. MIPI CSI du lidar non utilise.\n'
       'Decouplage au plus pres des billes (figure 7 du datasheet), vias thermiques sous les VSS.', 20.32, 15.24)
P.part(s, 'Lidar:VL53L9CX_large', 'U', 'VL53L9CX', 101.6, 114.3, {
    'A11': 'LIDAR_SCL', 'A12': 'LIDAR_SDA', 'E11': 'LIDAR_APCLK', 'B12': 'LIDAR_XSHUT',
    'D12': 'LIDAR_SYNC', 'A10': 'LIDAR_INT', 're:VSS.*': 'GND',
    'B1': '+3V3', 'D1': '+3V3', 'E6': '+3V3', 'E7': '+3V3', 'C12': '+1V2', 'E8': '+1V8'},
    nc=('A5', 'A6', 'A8', 'A9', 'E3', 'E12'), fp='custom_fp:VL53L9CX')
P.part(s, 'Custom:P3A9606', 'U', 'P3A9606', 228.6, 63.5, {
    '3': '+3V3', '6': '+1V8', '5': '+3V3', '1': 'I3C_SCL', '2': 'I3C_SDA',
    '8': 'LIDAR_SCL', '7': 'LIDAR_SDA', '4': 'GND'})
TR = 'Logic_LevelTranslator:SN74LVC1T45DBV'
P.part(s, TR, 'U', 'SN74LVC1T45 XSHUT', 228.6, 114.3, {
    '1': '+3V3', '6': '+1V8', '5': '+3V3', '3': 'LIDAR_XSHUT_3V3', '4': 'LIDAR_XSHUT', '2': 'GND'})
P.part(s, TR, 'U', 'SN74LVC1T45 SYNC', 228.6, 152.4, {
    '1': '+3V3', '6': '+1V8', '5': '+3V3', '3': 'LIDAR_SYNC_3V3', '4': 'LIDAR_SYNC', '2': 'GND'})
P.part(s, TR, 'U', 'SN74LVC1T45 INTR', 228.6, 190.5, {
    '1': '+1V8', '6': '+3V3', '5': '+1V8', '3': 'LIDAR_INT', '4': 'LIDAR_INT_3V3', '2': 'GND'})
P.part(s, 'Oscillator:ECS-2520MV-xxx-xx', 'Y', '24MHz 1V8', 25.4, 190.5, {
    '4': '+1V8', '1': '+1V8', '2': 'GND', '3': 'OSC_24M'})
g = Grid(20.32, 236.22, cols=16)
res(s, g, '33R', 'OSC_24M', 'LIDAR_APCLK')
res(s, g, '0R', 'I3C_SCL', 'ARD_SCL')
res(s, g, '0R', 'I3C_SDA', 'ARD_SDA')
res(s, g, '0R DNP', 'I3C_SCL', 'ARD_A5', dnp=True)
res(s, g, '0R DNP', 'I3C_SDA', 'ARD_A4', dnp=True)
res(s, g, '10k', 'LIDAR_XSHUT_3V3', 'GND')
res(s, g, '10k', 'LIDAR_XSHUT', 'GND')
res(s, g, '10k', 'LIDAR_SYNC_3V3', 'GND')
res(s, g, '10k', 'LIDAR_INT', '+1V8')
res(s, g, '2.2k DNP', 'LIDAR_SDA', '+1V8', dnp=True)
res(s, g, '2.2k DNP', 'LIDAR_SCL', '+1V8', dnp=True)
for _ in range(5): cap(s, g, '100nF', '+3V3')                  # P3A9606 A, 3 x LVC1T45, osc
g = Grid(20.32, 284.48, cols=16)
for v in ('100nF',) * 4: cap(s, g, v, '+1V8')                  # cotes 1,8 V des translateurs
g = Grid(190.5, 236.22, cols=10)
for v in ('22uF', '22uF', '100nF'): cap(s, g, v, '+3V3')       # VBAT_LDD
for v in ('10uF', '100nF'): cap(s, g, v, '+3V3')               # VBAT_RX
for v in ('4.7uF', '100nF'): cap(s, g, v, '+3V3')              # AVDD
for v in ('4.7uF', '100nF'): cap(s, g, v, '+1V2')              # DVDD
for v in ('1uF', '100nF'): cap(s, g, v, '+1V8')                # IOVDD + oscillateur
s.paper = 'A2'

# ======================================================================
# 4. IMU
# ======================================================================
s = P.sheet('IMU', 'imu.kicad_sch')
s.text('IMU LSM6DSV16X sur le bus D14/D15 du kit (3,3 V), adresse 0x6A (CS = VDDIO, SA0 = GND).\n'
       'Pull-ups deja presentes sur le kit (1,5k sur I2C1) : les 2,2k locales sont DNP.\n'
       'INT1 -> D5 (PE10), INT2 -> D6 (PE13). Sensor hub inutilise (SDx/SCx a la masse).\n'
       'Placer l IMU pres du lidar, sur la meme carte rigide, loin du LDO 3V3.', 20.32, 15.24)
P.part(s, 'IMU_symbole:LSM6DSV16X', 'U', 'LSM6DSV16X', 76.2, 88.9, {
    '14': 'ARD_SDA', '13': 'ARD_SCL', '12': '+3V3', '1': 'GND', '4': 'IMU_INT1', '9': 'IMU_INT2',
    '2': 'GND', '3': 'GND', '5': '+3V3', '8': '+3V3', '6': 'GND', '7': 'GND'}, nc=('10', '11'),
    fp='custom_fp:LGA14_LSM6_STM')
g = Grid(20.32, 165.1, cols=14)
res(s, g, '2.2k DNP', 'ARD_SDA', '+3V3', dnp=True)
res(s, g, '2.2k DNP', 'ARD_SCL', '+3V3', dnp=True)
cap(s, g, '100nF', '+3V3')
cap(s, g, '100nF', '+3V3')
cap(s, g, '10uF', '+3V3')

# ======================================================================
# 5. WIFI / BLE
# ======================================================================
s = P.sheet('WiFi BLE', 'wifi.kicad_sch')
s.text('ST67W611M1A6B (Wi-Fi 6 / BLE 5.4) - VDD33 = VDDIO = 3,3 V (meme niveau que le kit).\n'
       'Hote : SPI5 du N6 sur D10-D13 (NSS PA3, MOSI PG2, MISO PH8, SCK PE15), SPI_RDY -> D2 (PD0, EXTI).\n'
       'CHIP_EN -> D7 (PD6), pull-down 100k : module arrete tant que le N6 ne l active pas.\n'
       'UART du module : vers D0/D1 du kit par 0R, et sur le header J (GND, TX, RX, BOOT) pour la MAJ firmware.\n'
       'Antenne PCB en bord de carte, cote exterieur : zone sans cuivre sur toutes les couches, loin du kit.', 20.32, 15.24)
P.part(s, 'carte_wifi:ST67W611M1A6B', 'IC', 'ST67W611M1A6B', 76.2, 76.2, {
    '3': 'WIFI_BOOT', '9': '+3V3', '25': '+3V3', '16': '+3V3', 're:GND_.*': 'GND',
    '17': 'WIFI_EN', '21': 'WIFI_RDY', '22': 'WIFI_UART_TX', '23': 'WIFI_UART_RX', '24': 'WIFI_CS',
    '27': 'WIFI_MOSI', '28': 'WIFI_SCK', '29': 'WIFI_MISO'},
    nc=('1', '2', '4', '5', '6', '7', '8', '11', '12', '13', '14', '19', '20'), fp='custom_fp:ST67W611M1A6B')
P.part(s, 'Connector:Conn_01x04_Pin', 'J', 'UART/BOOT Wi-Fi', 203.2, 76.2, {
    '1': 'GND', '2': 'WIFI_UART_TX', '3': 'WIFI_UART_RX', '4': 'WIFI_BOOT'},
    fp='Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical')
g = Grid(20.32, 160.02, cols=14)
res(s, g, '100k', 'WIFI_EN', 'GND')
res(s, g, '10k', 'WIFI_BOOT', 'GND')
res(s, g, '0R', 'WIFI_UART_TX', 'ARD_D0_RX')
res(s, g, '0R', 'WIFI_UART_RX', 'ARD_D1_TX')
cap(s, g, '10uF', '+3V3')
cap(s, g, '100nF', '+3V3')
cap(s, g, '100nF', '+3V3')
cap(s, g, '100nF', '+3V3')

# ======================================================================
# 6. MECANIQUE
# ======================================================================
s = P.sheet('Mecanique', 'mecanique.kicad_sch')
s.text('Trous de fixation du module camera (B-CAMS-IMX, MB1854) a cote du lidar, vis M2.\n'
       'ENTRAXE A MESURER sur le module. La nappe fait le tour du bord haut du kit (longueur verifiee).', 20.32, 15.24)
for i, x in enumerate((50.8, 76.2)):
    P.part(s, 'Mechanical:MountingHole', 'H', 'Camera M2', x, 63.5, {}, fp='MountingHole:MountingHole_2.2mm_M2')

# ======================================================================
os.makedirs(OUT, exist_ok=True)
notes = [('CARTE FILLE PROTOTYPE A - SCANNER 3D PORTABLE\n\n'
          'Shield Arduino Uno R3 enfiche SOUS le STM32N6570-DK : lidar VL53L9CX (I3C via translateur NXP P3A9606),\n'
          'IMU LSM6DSV16X, module Wi-Fi 6 / BLE ST67W611M1. Le calcul (IA sur NPU) et la camera sont sur le kit.\n'
          'Le lidar et le module camera sont sur la face exterieure et regardent la scene, a l oppose de l ecran.\n'
          'Rails : +5V du kit -> LDO +3V3 -> LDO +1V8 / +1V2.\n'
          'Schema genere par generateur/design_A.py (base : schema du prototype 1), a reprendre a la main pour la mise en page.',
          30.48, 160.02)]
open(f'{OUT}/carte_fille_A.kicad_sch', 'w').write(P.render_root(notes))
for sh in P.sheets:
    open(f'{OUT}/{sh.fname}', 'w').write(P.render_sheet(sh))

# netlist simplifiée pour placer les empreintes sur le PCB (generateur/pcb_A.py)
parts = []
for sh in P.sheets:
    for p in sh.parts:
        if p['lib_id'].startswith('power:'): continue
        sym = libs.resolve(*p['lib_id'].split(':'))
        symprop = {uq(e[1]): uq(e[2]) for e in find(sym, 'property')}
        fp = p['fp'] if p['fp'] is not None else symprop.get('Footprint', '')
        full = lambda n: n if n in POWER or len(P.netuse[n]) > 1 or n in P.force_global else f'/{sh.name}/{n}'
        parts.append(dict(ref=p['ref'], value=p['value'], fp=fp, dnp=p['dnp'], sheet=sh.name,
                          path=f'/{sh.uuid}/{p["uuid"]}', sheetfile=sh.fname, pins={k: full(v) for k, v in p['pinnet'].items()},
                          pinnames={q['num']: q['name'] for q in p['pins']},
                          datasheet=symprop.get('Datasheet', '')))
json.dump(parts, open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'netlist_A.json'), 'w'), indent=1)

open(f'{OUT}/sym-lib-table', 'w').write('(sym_lib_table\n\t(version 7)\n' + ''.join(
    f'\t(lib (name "{n}") (type "KiCad") (uri "${{KIPRJMOD}}/libs/{n}.kicad_sym") (options "") (descr ""))\n'
    for n in ('IMU_symbole', 'Lidar', 'carte_wifi', 'Custom')) + ')\n')
open(f'{OUT}/fp-lib-table', 'w').write('(fp_lib_table\n\t(version 7)\n'
    '\t(lib (name "custom_fp") (type "KiCad") (uri "${KIPRJMOD}/libs/custom_fp.pretty") (options "") (descr "IMU, lidar, module Wi-Fi"))\n)\n')
pro_path = f'{OUT}/carte_fille_A.kicad_pro'
pro = json.load(open(pro_path))
pro['meta']['filename'] = 'carte_fille_A.kicad_pro'
pro['sheets'] = [[P.root_uuid, 'Root']] + [[s.uuid, s.name] for s in P.sheets]
json.dump(pro, open(pro_path, 'w'), indent=2)
print('sheets:', [s.fname for s in P.sheets])
