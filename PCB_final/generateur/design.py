"""Contenu des feuilles du PCB final (hors MCU / mémoires / caméra)."""
import os, sys, json, shutil
from kgen import Project

OUT = sys.argv[1]
P = Project()
P.force_global = {'VSYS', 'VSYS_ON', 'HSE_IN',
    'LIDAR_SCL', 'LIDAR_SDA', 'LIDAR_XSHUT', 'LIDAR_SYNC', 'LIDAR_INT', 'MCO_LIDAR',
    'IMU_SDA', 'IMU_SCL', 'IMU_INT1', 'IMU_INT2',
    'WIFI_EN', 'WIFI_RDY', 'WIFI_UART_TX', 'WIFI_UART_RX', 'WIFI_CS', 'WIFI_MOSI', 'WIFI_SCK', 'WIFI_MISO',
    'USB_DP', 'USB_DM', 'VBUS_DET', 'VBAT_SENSE',
    'SWDIO', 'SWCLK', 'SWO', 'NRST', 'BOOT0', 'USER_BTN', 'LED_STATUS'}

R = 'Device:R_Small'; C = 'Device:C_Small'; L = 'Device:L_Small'; LED = 'Device:LED_Small'
FR = 'Resistor_SMD:R_0402_1005Metric'
FC = 'Capacitor_SMD:C_0402_1005Metric'
FC6 = 'Capacitor_SMD:C_0603_1608Metric'
FC8 = 'Capacitor_SMD:C_0805_2012Metric'
FL = 'Inductor_SMD:L_Taiyo-Yuden_MD-3030'
TP = 'TestPoint:TestPoint_Pad_D1.0mm'

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

# ======================================================================
# 1. ALIMENTATION
# ======================================================================
s = P.sheet('Alimentation', 'alimentation.kicad_sch')
s.text('ALIMENTATION - composants actifs 100 % ST\n'
       'USB-C VBUS -> STC4054 (chargeur Li-ion lineaire 800 mA) -> batterie\n'
       'Power-path discret : Schottky STPS1L30A (VBUS -> VSYS) + P-MOS ST (batterie -> VSYS, coupe quand VBUS present).\n'
       'VSYS (3,0-4,7 V) -> interrupteur -> VSYS_ON -> LD39150DT33 (LDO 1,5 A) -> +3V3\n'
       '+3V3 -> LDK130-18 -> +1V8 | +3V3 -> LDK130-12 -> +1V2 (DVDD lidar). VBAT du lidar sur VSYS_ON (max 4,8 V).\n'
       'Limite : sans buck-boost, le +3V3 decroche sous ~3,5 V batterie (coupure logicielle via VBAT_SENSE).\n'
       'A VERIFIER : brochage et RPROG du STC4054, reference exacte du P-MOS, condensateurs des LDO.', 20.32, 15.24)
P.part(s, 'Connector:Conn_01x02_Pin', 'J', 'Batterie 1S Li-ion', 25.4, 101.6,
       {'1': '+BATT', '2': 'GND'}, fp='Connector_JST:JST_PH_S2B-PH-SM4-TB_1x02-1MP_P2.00mm_Horizontal')
P.part(s, 'ST_custom:STC4054GR', 'U', 'STC4054GR', 76.2, 101.6, {
    '4': 'VBUS', '3': '+BATT', '2': 'GND', '5': 'CHG_PROG', '1': 'CHG_STAT_N'})
P.part(s, 'Device:D_Schottky_Small', 'D', 'STPS1L30A', 127, 76.2, {'2': 'VBUS', '1': 'VSYS'}, fp='Diode_SMD:D_SMA')
P.part(s, 'Transistor_FET:Q_PMOS_GSD', 'Q', 'STR2P3LLH6 (P-MOS ST)', 127, 114.3,
       {'1': 'VBUS', '3': '+BATT', '2': 'VSYS'}, fp='Package_TO_SOT_SMD:SOT-23')
P.part(s, 'Switch:SW_SPDT', 'SW', 'Marche/Arret', 177.8, 96.52,
       {'2': 'VSYS', '1': 'VSYS_ON'}, nc=('3',), fp='Button_Switch_SMD:SW_SPDT_PCM12')
P.part(s, 'Regulator_Linear:LD39150DT33', 'U', 'LD39150DT33', 228.6, 96.52, {
    '1': 'VSYS_ON', '3': '+3V3', '2': 'GND'})
P.part(s, 'Regulator_Linear:LDK130-18_SOT23_SOT353', 'U', 'LDK130-18', 284.48, 96.52, {
    '1': '+3V3', '3': '+3V3', '5': '+1V8', '2': 'GND'}, nc=('4',), fp='Package_TO_SOT_SMD:SOT-23-5')
P.part(s, 'Regulator_Linear:LDK130-12_SOT23_SOT353', 'U', 'LDK130-12', 340.36, 96.52, {
    '1': '+3V3', '3': '+3V3', '5': '+1V2', '2': 'GND'}, nc=('4',), fp='Package_TO_SOT_SMD:SOT-23-5')
g = Grid(20.32, 154.94)
# chargeur
res(s, g, '1.24k (800mA)', 'CHG_PROG', 'GND')
res(s, g, '100k', 'VBUS', 'GND')          # tire la grille du P-MOS a la masse sans USB
res(s, g, '1.5k', 'VSYS', 'LED_CHG_A')
x, y = g(); P.part(s, LED, 'D', 'LED_CHG', x, y, {'2': 'LED_CHG_A', '1': 'CHG_STAT_N'}, fp='LED_SMD:LED_0603_1608Metric'); g()
cap(s, g, '4.7uF', 'VBUS')
cap(s, g, '4.7uF', '+BATT')
cap(s, g, '22uF', 'VSYS')
# mesure batterie (VBAT/3, max 1,4 V)
res(s, g, '200k', '+BATT', 'VBAT_SENSE')
res(s, g, '100k', 'VBAT_SENSE', 'GND')
cap(s, g, '100nF', 'VBAT_SENSE')
# LDO 3V3
cap(s, g, '10uF', 'VSYS_ON')
cap(s, g, '22uF', '+3V3')
cap(s, g, '100nF', '+3V3')
# LDO 1V8 / 1V2
cap(s, g, '1uF', '+3V3')
cap(s, g, '1uF', '+1V8')
cap(s, g, '1uF', '+3V3')
cap(s, g, '1uF', '+1V2')
for net, xx in (('VBUS', 290), ('VSYS', 305), ('VSYS_ON', 320), ('GND', 345)):
    P.flag(s, net, xx, 170)

# ======================================================================
# 2. USB
# ======================================================================
s = P.sheet('USB', 'usb.kicad_sch')
s.text('USB-C (puits 5 V, USB 2.0) -> USB FS du STM32H563 (PA11/PA12). Paire D+/D- 90 ohms.\n'
       'VBUS_DET = VBUS x 0,32 (max 1,6 V) pour la detection de branchement.', 20.32, 15.24)
P.part(s, 'Connector:USB_C_Receptacle_USB2.0_16P', 'J', 'USB-C', 60.96, 101.6, {
    're:VBUS': 'VBUS', 're:GND': 'GND', 'SH': 'GND', 'A5': 'USB_CC1', 'B5': 'USB_CC2',
    'A6': 'USB_DP', 'B6': 'USB_DP', 'A7': 'USB_DM', 'B7': 'USB_DM'}, nc=('A8', 'B8'),
    fp='Connector_USB:USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal')
P.part(s, 'Power_Protection:USBLC6-2SC6', 'U', 'USBLC6-2SC6', 139.7, 101.6, {
    '1': 'USB_DP', '6': 'USB_DP', '3': 'USB_DM', '4': 'USB_DM', '5': 'VBUS', '2': 'GND'})
g = Grid(190.5, 76.2)
res(s, g, '5.1k', 'USB_CC1', 'GND')
res(s, g, '5.1k', 'USB_CC2', 'GND')
res(s, g, '100k', 'VBUS', 'VBUS_DET')
res(s, g, '47k', 'VBUS_DET', 'GND')
cap(s, g, '10uF', 'VBUS')

# ======================================================================
# 3. LIDAR
# ======================================================================
s = P.sheet('Lidar', 'lidar.kicad_sch')
s.text('LIDAR VL53L9CX - IOVDD = 1,8 V : toutes les E/S (I3C, AP_CLK, XSHUT, INTR, SYNC_IN) en 1,8 V.\n'
       'DVDD = 1,2 V (alimentation externe), AVDD = 3,3 V, VBAT_LDD/VBAT_RX = VSYS_ON (2,8-4,8 V).\n'
       'Pas de pull-up sur SCL/SDA en I3C (R DNP pour un repli en I2C, adresse 0x52).\n'
       'AP_CLK : oscillateur 24 MHz 1,8 V partage avec le HSE du H563, ou MCO1 (PA8) via R 0 ohm (DNP).\n'
       'MIPI CSI du lidar non utilise (le H563 n a pas de CSI) : broches non connectees.\n'
       'Decouplage au plus pres des billes, vias thermiques sous les VSS (TN1579).', 20.32, 15.24)
P.part(s, 'Lidar:VL53L9CX_large', 'U', 'VL53L9CX', 101.6, 114.3, {
    'A11': 'LIDAR_SCL', 'A12': 'LIDAR_SDA', 'E11': 'LIDAR_APCLK', 'B12': 'LIDAR_XSHUT',
    'D12': 'LIDAR_SYNC', 'A10': 'LIDAR_INT', 're:VSS.*': 'GND',
    'B1': 'VSYS_ON', 'D1': 'VSYS_ON', 'E6': '+3V3', 'E7': '+3V3', 'C12': '+1V2', 'E8': '+1V8'},
    nc=('A5', 'A6', 'A8', 'A9'), fp='custom_fp:VL53L9CX')
P.part(s, 'Oscillator:ECS-2520MV-xxx-xx', 'Y', '24MHz 1V8', 25.4, 190.5, {
    '4': '+1V8', '1': '+1V8', '2': 'GND', '3': 'OSC_24M'})
g = Grid(20.32, 228.6, cols=14)
res(s, g, '33R', 'OSC_24M', 'LIDAR_APCLK')
res(s, g, '33R', 'OSC_24M', 'HSE_IN')
res(s, g, '0R DNP', 'MCO_LIDAR', 'LIDAR_APCLK', dnp=True)
res(s, g, '10k', 'LIDAR_XSHUT', 'GND')
res(s, g, '10k', 'LIDAR_INT', '+1V8')
res(s, g, '2.2k DNP', 'LIDAR_SDA', '+1V8', dnp=True)
res(s, g, '2.2k DNP', 'LIDAR_SCL', '+1V8', dnp=True)
cap(s, g, '100nF', '+1V8')
g = Grid(190.5, 50.8, cols=14)
for v in ('22uF', '22uF', '100nF'): cap(s, g, v, 'VSYS_ON')        # VBAT_LDD
for v in ('10uF', '100nF'): cap(s, g, v, 'VSYS_ON')                # VBAT_RX
for v in ('4.7uF', '100nF'): cap(s, g, v, '+3V3')               # AVDD
for v in ('4.7uF', '100nF'): cap(s, g, v, '+1V2')               # DVDD
for v in ('1uF', '100nF'): cap(s, g, v, '+1V8')                 # IOVDD

# ======================================================================
# 4. IMU
# ======================================================================
s = P.sheet('IMU', 'imu.kicad_sch')
s.text('IMU LSM6DSV16X en I2C (adresse 0x6A : CS=VDDIO, SA0=GND), bus I2C2 du H563 en 1,8 V, pull-ups 2,2k.\n'
       'Pas de magnetometre : le cap est recale par le traitement des scans lidar sur le PC.\n'
       'Sensor hub inutilise : SDx/SCx (broches 2 et 3) a la masse. OCS_Aux (10) et SDO_Aux (11, nommee\n'
       'SDA_AUX dans le symbole) non connectees. A verifier dans le datasheet du LSM6DSV16X.\n'
       'Placer l IMU au centre de la carte, loin des regulateurs et des pistes VBAT du lidar.', 20.32, 15.24)
P.part(s, 'IMU_symbole:LSM6DSV16X', 'U', 'LSM6DSV16X', 76.2, 88.9, {
    '14': 'IMU_SDA', '13': 'IMU_SCL', '12': '+1V8', '1': 'GND', '4': 'IMU_INT1', '9': 'IMU_INT2',
    '2': 'GND', '3': 'GND', '5': '+1V8', '8': '+1V8', '6': 'GND', '7': 'GND'}, nc=('10', '11'),
    fp='custom_fp:LGA14_LSM6_STM')
g = Grid(20.32, 165.1, cols=14)
res(s, g, '2.2k', 'IMU_SDA', '+1V8')
res(s, g, '2.2k', 'IMU_SCL', '+1V8')
cap(s, g, '100nF', '+1V8')   # VDD IMU
cap(s, g, '100nF', '+1V8')   # VDD_IO IMU
cap(s, g, '10uF', '+1V8')

# ======================================================================
# 5. WIFI / BLE
# ======================================================================
s = P.sheet('WiFi BLE', 'wifi.kicad_sch')
s.text('ST67W611M1A6B (Wi-Fi 6 / BLE 5.4) - VDD33 = 3,3 V, VDDIO = 1,8 V (niveau du H563).\n'
       'Hote : SPI esclave jusqu a 40 MHz (bus dedie), SPI_RDY = interruption vers le H563.\n'
       'CHIP_EN (domaine VDD33) : pull-up 100k au +3V3, commande par PB1 du H563 en open-drain (broche FT 5 V, a verifier).\n'
       'BOOT : pull-down 10k, header pour la MAJ firmware.\n'
       'UART du module ramene sur J (programmation / debug) et vers le H563 (USART2).\n'
       'Antenne PCB en bord de carte : zone sans cuivre sur toutes les couches.', 20.32, 15.24)
P.part(s, 'carte_wifi:ST67W611M1A6B', 'IC', 'ST67W611M1A6B', 76.2, 76.2, {
    '3': 'WIFI_BOOT', '9': '+1V8', '25': '+1V8', '16': '+3V3', 're:GND_.*': 'GND',
    '17': 'WIFI_EN', '21': 'WIFI_RDY', '22': 'WIFI_UART_TX', '23': 'WIFI_UART_RX', '24': 'WIFI_CS',
    '27': 'WIFI_MOSI', '28': 'WIFI_SCK', '29': 'WIFI_MISO'},
    nc=('1', '2', '4', '5', '6', '7', '8', '11', '12', '13', '14', '19', '20'), fp='custom_fp:ST67W611M1A6B')
P.part(s, 'Connector:Conn_01x04_Pin', 'J', 'UART/BOOT Wi-Fi', 203.2, 76.2, {
    '1': 'GND', '2': 'WIFI_UART_TX', '3': 'WIFI_UART_RX', '4': 'WIFI_BOOT'},
    fp='Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical')
g = Grid(20.32, 160.02, cols=14)
res(s, g, '100k', 'WIFI_EN', '+3V3')
res(s, g, '10k', 'WIFI_BOOT', 'GND')
cap(s, g, '10uF', '+3V3')
cap(s, g, '100nF', '+3V3')
cap(s, g, '100nF', '+1V8')
cap(s, g, '100nF', '+1V8')


# ======================================================================
# 6. DEBUG / IHM
# ======================================================================
s = P.sheet('Debug IHM', 'debug_ihm.kicad_sch')
s.text('SWD (connecteur Cortex 2x5 1,27 mm) : programmation et reset par la sonde. BOOT0 a la masse (10k).\n'
       'Bouton utilisateur SCAN, LED d etat.\n'
       'VTref = +1V8 (le H563 est alimente en 1,8 V : la sonde ST-LINK V3 s adapte au niveau).\n'
       'LED d etat : anode au +3V3, cathode sur une sortie open-drain du H563 (active a l etat bas).', 20.32, 15.24)
P.part(s, 'Connector:Conn_ARM_JTAG_SWD_10', 'J', 'SWD', 63.5, 88.9, {
    '1': '+3V3', '2': 'SWDIO', '3': 'GND', '5': 'GND', '4': 'SWCLK', '6': 'SWO', '10': 'NRST', '9': 'GND'},
    nc=('8',), fp='Connector_PinHeader_1.27mm:PinHeader_2x05_P1.27mm_Vertical_SMD')
P.part(s, 'Switch:SW_Push', 'SW', 'SCAN (utilisateur)', 152.4, 127, {'1': 'USER_BTN', '2': 'GND'},
       fp='Button_Switch_SMD:SW_SPST_PTS810')
g = Grid(203.2, 76.2, cols=8)
cap(s, g, '100nF', 'NRST')
res(s, g, '10k', 'BOOT0', 'GND')
res(s, g, '10k', 'USER_BTN', '+1V8')
res(s, g, '1k', '+3V3', 'LED_ST_A')
x, y = g(); P.part(s, LED, 'D', 'LED_ST rouge', x, y, {'2': 'LED_ST_A', '1': 'LED_STATUS'}, fp='LED_SMD:LED_0603_1608Metric'); g()

# ======================================================================
# 7. MCU STM32H563VIT6
# ======================================================================
s = P.sheet('MCU', 'mcu.kicad_sch')
s.text('STM32H563VIT6 (LQFP100) alimente en 1,8 V (VDD, VDDA, VREF+, VBAT) : meme niveau que l IOVDD du lidar,\n'
       'donc pas de translateur sur l I3C. Seul VDDUSB est en 3,3 V (USB FS PA11/PA12).\n'
       'Horloge : oscillateur 24 MHz 1,8 V partage (HSE bypass sur PH0 + AP_CLK du lidar). Pas de LSE (RTC sur LSI si besoin).\n'
       'Lidar : I3C1 PB8/PB9 (= D15/D14 Arduino de la NUCLEO-H563ZI), XSHUT PE0, INTR PE2, SYNC PE9 (TIM1_CH1), MCO1 PA8.\n'
       'IMU : I2C2 PB10/PB12, INT1 PE3, INT2 PE4. Wi-Fi : SPI1 PA4-PA7, RDY PB0, EN PB1, USART2 PD5/PD6.\n'
       'ADC : VBAT_SENSE PC0, VBUS_DET PC1. A VERIFIER (datasheet H563) : valeur des condensateurs VCAP.', 20.32, 15.24)
P.part(s, 'MCU_ST_STM32H5:STM32H563VITx', 'U', 'STM32H563VIT6', 152.4, 152.4, {
    '6': '+1V8', '11': '+1V8', '28': '+1V8', '50': '+1V8', '75': '+1V8', '100': '+1V8', '22': '+1V8', '21': '+1V8',
    '10': 'GND', '27': 'GND', '49': 'GND', '74': 'GND', '99': 'GND', '19': 'GND', '20': 'GND',
    '48': 'VCAP1', '98': 'VCAP2', '73': '+3V3',
    '14': 'NRST', '94': 'BOOT0', '12': 'HSE_IN',
    '70': 'USB_DM', '71': 'USB_DP', '72': 'SWDIO', '76': 'SWCLK', '89': 'SWO',
    '29': 'WIFI_CS', '30': 'WIFI_SCK', '31': 'WIFI_MISO', '32': 'WIFI_MOSI', '35': 'WIFI_RDY', '36': 'WIFI_EN',
    '86': 'WIFI_UART_RX', '87': 'WIFI_UART_TX',
    '95': 'LIDAR_SCL', '96': 'LIDAR_SDA', '97': 'LIDAR_XSHUT', '1': 'LIDAR_INT', '40': 'LIDAR_SYNC', '67': 'MCO_LIDAR',
    '47': 'IMU_SCL', '51': 'IMU_SDA', '2': 'IMU_INT1', '3': 'IMU_INT2',
    '15': 'VBAT_SENSE', '16': 'VBUS_DET', '7': 'USER_BTN', '63': 'LED_STATUS'})
g = Grid(254, 101.6, cols=10)
cap(s, g, '2.2uF a verifier', 'VCAP1'); cap(s, g, '2.2uF a verifier', 'VCAP2')
for v in ('4.7uF',) + ('100nF',) * 5: cap(s, g, v, '+1V8')     # VDD
for v in ('1uF', '100nF'): cap(s, g, v, '+1V8')                 # VDDA
for v in ('1uF', '100nF'): cap(s, g, v, '+1V8')                 # VREF+
for v in ('1uF', '100nF'): cap(s, g, v, '+3V3')                 # VDDUSB

# ======================================================================
os.makedirs(OUT, exist_ok=True)
notes = [('PCB FINAL - SCANNER 3D PORTABLE (STM32H563VIT6)\n\n'
          'Composants actifs 100 % ST. Rails : VBUS (USB-C) -> STC4054 + power-path -> VSYS -> LDO +3V3 / +1V8 / +1V2.\n'
          'Traitement (IA) deporte sur l application PC : pas de camera ni de memoire externe.\n'
          'Les signaux vers le MCU sont des labels globaux (LIDAR_*, IMU_*, WIFI_*, USB_*, SWD...).\n'
          'Genere automatiquement puis a reprendre a la main pour la mise en page.', 30.48, 160.02)]
open(f'{OUT}/{"PCB_final"}.kicad_sch', 'w').write(P.render_root(notes))
for sh in P.sheets:
    open(f'{OUT}/{sh.fname}', 'w').write(P.render_sheet(sh))
glob = sorted(n for n, v in P.netuse.items() if (len(v) > 1 or n in P.force_global) and n not in
              ('GND', '+3V3', '+1V8', '+1V2', 'VBUS', '+BATT'))
single = sorted(n for n, v in P.netuse.items() if len(v) == 1 and n.isupper() and n not in P.force_global)
json.dump({'global': glob}, open(f'{OUT}/../gen_nets.json', 'w'), indent=1)
# bibliothèques locales du projet
import copy
from sexp import dump as sdump, q as sq
import libs as L
os.makedirs(f'{OUT}/libs/custom_fp.pretty', exist_ok=True)
shutil.copy(os.path.join(os.path.dirname(os.path.abspath(L.__file__)), 'ST_custom.kicad_sym'), f'{OUT}/libs/ST_custom.kicad_sym')
for lib, name in (('IMU_symbole', 'LSM6DSV16X'), ('Lidar', 'VL53L9CX_large'), ('carte_wifi', 'ST67W611M1A6B')):
    sym = copy.deepcopy(L.resolve(lib, name))
    doc = ['kicad_symbol_lib', ['version', '20251024'], ['generator', sq('kicad_symbol_editor')],
           ['generator_version', sq('10.0')], sym]
    open(f'{OUT}/libs/{lib}.kicad_sym', 'w').write(sdump(doc) + '\n')
SRC = L.REPO + '/PCB_prototype1'
for src in ('footprints.pretty/LGA14_LSM6_STM.kicad_mod', 'module_wifi/ST67W611M1A6B.kicad_mod', 'VL53L9CX.kicad_mod'):
    shutil.copy(f'{SRC}/{src}', f'{OUT}/libs/custom_fp.pretty/')
open(f'{OUT}/sym-lib-table', 'w').write('(sym_lib_table\n\t(version 7)\n' + ''.join(
    f'\t(lib (name "{n}") (type "KiCad") (uri "${{KIPRJMOD}}/libs/{n}.kicad_sym") (options "") (descr ""))\n'
    for n in ('IMU_symbole', 'Lidar', 'carte_wifi', 'ST_custom')) + ')\n')
open(f'{OUT}/fp-lib-table', 'w').write('(fp_lib_table\n\t(version 7)\n'
    '\t(lib (name "custom_fp") (type "KiCad") (uri "${KIPRJMOD}/libs/custom_fp.pretty") (options "") (descr "IMU, lidar, module Wi-Fi"))\n)\n')
pro = json.load(open(SRC + '/PCB_prototype1.kicad_pro'))
pro['meta']['filename'] = 'PCB_final.kicad_pro'
pro['sheets'] = [[P.root_uuid, 'Root']] + [[s.uuid, s.name] for s in P.sheets]
pro['boards'] = []
json.dump(pro, open(f'{OUT}/PCB_final.kicad_pro', 'w'), indent=2)
print('sheets:', [s.fname for s in P.sheets])
