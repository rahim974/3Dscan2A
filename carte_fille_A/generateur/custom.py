"""Symboles absents de la bibliothèque KiCad standard."""
from sexp import q, dump

def pin(t, x, y, a, name, num):
    return ['pin', t, 'line', ['at', f'{x:g}', f'{y:g}', str(a)], ['length', '2.54'],
            ['name', q(name), ['effects', ['font', ['size', '1.27', '1.27']]]],
            ['number', q(num), ['effects', ['font', ['size', '1.27', '1.27']]]]]

def prop(k, v, y=0, hide=False):
    p = ['property', q(k), q(v), ['at', '0', f'{y:g}', '0']]
    if hide: p.append(['hide', 'yes'])
    return p + [['effects', ['font', ['size', '1.27', '1.27']]]]

def stc4054():
    n = 'STC4054GR'
    return ['symbol', q(n), ['pin_names', ['offset', '1.016']], ['exclude_from_sim', 'no'],
            ['in_bom', 'yes'], ['on_board', 'yes'],
            prop('Reference', 'U', 8.89), prop('Value', n, 6.35),
            prop('Footprint', 'Package_TO_SOT_SMD:TSOT-23-5', hide=True),
            prop('Datasheet', 'https://www.st.com/resource/en/datasheet/stc4054.pdf', hide=True),
            prop('Description', 'Chargeur Li-ion lineaire 800 mA, TSOT23-5 (brochage type 4054, a verifier)', hide=True),
            ['symbol', q(f'{n}_0_1'),
             ['rectangle', ['start', '-7.62', '5.08'], ['end', '7.62', '-5.08'],
              ['stroke', ['width', '0.254'], ['type', 'default']], ['fill', ['type', 'background']]]],
            ['symbol', q(f'{n}_1_1'),
             pin('power_in', -10.16, 2.54, 0, 'VCC', '4'),
             pin('passive', -10.16, -2.54, 0, 'PROG', '5'),
             pin('power_out', 10.16, 2.54, 180, 'BAT', '3'),
             pin('open_collector', 10.16, -2.54, 180, '~{CHRG}', '1'),
             pin('power_in', 0, -7.62, 90, 'GND', '2')]]

def p3a9606():
    n = 'P3A9606'
    return ['symbol', q(n), ['pin_names', ['offset', '1.016']], ['exclude_from_sim', 'no'],
            ['in_bom', 'yes'], ['on_board', 'yes'],
            prop('Reference', 'U', 10.16), prop('Value', n, 7.62),
            prop('Footprint', 'Package_SON:X2SON-8_1.4x1mm_P0.35mm', hide=True),
            prop('Datasheet', 'https://www.nxp.com/docs/en/data-sheet/P3A9606.pdf', hide=True),
            prop('Description', 'Translateur de niveau I3C/I2C 2 bits NXP. BROCHAGE ET BOITIER A VERIFIER dans le datasheet', hide=True),
            ['symbol', q(f'{n}_0_1'),
             ['rectangle', ['start', '-7.62', '6.35'], ['end', '7.62', '-6.35'],
              ['stroke', ['width', '0.254'], ['type', 'default']], ['fill', ['type', 'background']]]],
            ['symbol', q(f'{n}_1_1'),
             pin('power_in', -2.54, 8.89, 270, 'VCCA', '3'),
             pin('power_in', 2.54, 8.89, 270, 'VCCB', '6'),
             pin('input', -10.16, -3.81, 0, 'EN', '5'),
             pin('bidirectional', -10.16, 2.54, 0, 'A1', '1'),
             pin('bidirectional', -10.16, 0, 0, 'A2', '2'),
             pin('bidirectional', 10.16, 2.54, 180, 'B1', '8'),
             pin('bidirectional', 10.16, 0, 180, 'B2', '7'),
             pin('power_in', 0, -8.89, 90, 'GND', '4')]]

def write(path):
    doc = ['kicad_symbol_lib', ['version', '20251024'], ['generator', q('kicad_symbol_editor')],
           ['generator_version', q('10.0')], p3a9606()]
    open(path, 'w').write(dump(doc) + '\n')

if __name__ == '__main__':
    write('../libs/Custom.kicad_sym')
