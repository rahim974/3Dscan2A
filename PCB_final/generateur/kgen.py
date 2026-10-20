"""Mini générateur de schémas KiCad 10 : symboles + stubs + labels/power."""
import uuid, math, re, copy
from sexp import q, dump, uq, find, find1
import libs

PROJECT = 'PCB_final'
POWER = {'GND': 'power:GND', '+3V3': 'power:+3V3', '+1V8': 'power:+1V8', '+1V2': 'power:+1V2',
         'VBUS': 'power:VBUS', '+BATT': 'power:+BATT'}
DOWN_BODY = {'power:GND'}
STUB = 2.54

def U(): return str(uuid.uuid4())
def r2(v): return round(v / 1.27) * 1.27
def fmt(v):
    s = f'{v:.4f}'.rstrip('0').rstrip('.')
    return '0' if s in ('-0', '') else s

def font(just=None, hide=False):
    e = ['effects', ['font', ['size', '1.27', '1.27']]]
    if just: e.append(['justify'] + just.split())
    return e

def prop(name, val, x, y, a=0, hide=False, just=None):
    p = ['property', q(name), q(val), ['at', fmt(x), fmt(y), fmt(a)]]
    if hide: p.append(['hide', 'yes'])
    p.append(font(just))
    return p

class Sheet:
    def __init__(self, name, fname, page):
        self.name, self.fname, self.page = name, fname, page
        self.uuid = U(); self.items = []; self.libs = {}; self.parts = []
        self.texts = []; self.paper = 'A3'
    def text(self, txt, x, y):
        self.texts.append((txt, x, y))

class Project:
    def __init__(self):
        self.root_uuid = U(); self.sheets = []; self.refcount = {}
        self.pwr = 0; self.netuse = {}
    def sheet(self, name, fname):
        s = Sheet(name, fname, str(len(self.sheets) + 2)); self.sheets.append(s); return s
    def ref(self, prefix):
        self.refcount[prefix] = self.refcount.get(prefix, 0) + 1
        return f'{prefix}{self.refcount[prefix]}'

    def part(self, sh, lib_id, prefix, value, x, y, conns, fp=None, dnp=False, nc=(), props=None, ref=None, unit=1):
        """conns: {pin number | 're:<regex sur nom>': net}"""
        lib, name = lib_id.split(':')
        if lib_id not in sh.libs: sh.libs[lib_id] = libs.embedded(lib, name)
        pins = libs.pins(lib, name, unit)
        ref = ref or self.ref(prefix)
        x, y = r2(x), r2(y)
        pinnet = {}
        for k, net in conns.items():
            if k.startswith('re:'):
                hit = [p for p in pins if re.fullmatch(k[3:], p['name'])]
            else:
                hit = [p for p in pins if p['num'] == k]
            if not hit: raise ValueError(f'{ref}: pin {k} introuvable')
            for p in hit: pinnet[p['num']] = net
        sh.parts.append(dict(lib_id=lib_id, ref=ref, value=value, x=x, y=y, fp=fp, dnp=dnp,
                             pins=pins, props=props or {}, unit=unit))
        for p in pins:
            px, py = x + p['x'], y - p['y']
            net = pinnet.get(p['num'])
            if net is None:
                if p['type'] == 'no_connect' or p['num'] in nc or p['name'] in nc:
                    sh.items.append(['no_connect', ['at', fmt(px), fmt(py)], ['uuid', q(U())]])
                elif p['hidden'] or p['type'] == 'power_in' and lib == 'power':
                    pass
                else:
                    print(f'  ! {sh.name} {ref} pin {p["num"]} ({p["name"]}) non connectée -> NC')
                    sh.items.append(['no_connect', ['at', fmt(px), fmt(py)], ['uuid', q(U())]])
                continue
            a = math.radians(p['a'])
            dx, dy = -round(math.cos(a)), round(math.sin(a))
            ex, ey = px + dx * STUB, py + dy * STUB
            sh.items.append(['wire', ['pts', ['xy', fmt(px), fmt(py)], ['xy', fmt(ex), fmt(ey)]],
                             ['stroke', ['width', '0'], ['type', 'default']], ['uuid', q(U())]])
            ang = {(1, 0): 0, (0, -1): 90, (-1, 0): 180, (0, 1): 270}[(dx, dy)]
            self.netuse.setdefault(net, set()).add(sh.name)
            sh.items.append(('NET', net, ex, ey, ang))
        return ref

    def flag(self, sh, net, x, y):
        """PWR_FLAG relié au net via un stub vertical."""
        self.part(sh, 'power:PWR_FLAG', '#FLG', 'PWR_FLAG', x, y, {'1': net})

    # ---------- sérialisation ----------
    def _power(self, sh, net, x, y, ang):
        lib_id = POWER[net]
        if lib_id not in sh.libs: sh.libs[lib_id] = libs.embedded(*lib_id.split(':'))
        if lib_id in DOWN_BODY: rot = {270: 0, 0: 90, 90: 180, 180: 270}[ang]
        else: rot = {90: 0, 180: 90, 270: 180, 0: 270}[ang]
        self.pwr += 1
        return self._symbol(sh, dict(lib_id=lib_id, ref=f'#PWR{self.pwr:03d}', value=net, x=x, y=y,
                                     fp=None, dnp=False, pins=[{'num': '1'}], props={}), rot, power=True)

    def _symbol(self, sh, p, rot=0, power=False):
        x, y = p['x'], p['y']
        sym = ['symbol', ['lib_id', q(p['lib_id'])], ['at', fmt(x), fmt(y), str(rot)], ['unit', str(p.get('unit', 1))],
               ['exclude_from_sim', 'no'], ['in_bom', 'no' if power else 'yes'],
               ['on_board', 'no' if power else 'yes'], ['dnp', 'yes' if p['dnp'] else 'no'],
               ['uuid', q(U())]]
        if power:
            sym.append(prop('Reference', p['ref'], x, y, 0, hide=True))
            sym.append(prop('Value', p['value'], x, y + (3.8 if p['lib_id'] in DOWN_BODY else -3.8), 0,
                            hide=p['lib_id'] == 'power:PWR_FLAG' and False))
        else:
            sym.append(prop('Reference', p['ref'], x + 1.5, y - 1.0, 0, just='left'))
            sym.append(prop('Value', p['value'], x + 1.5, y + 1.0, 0, just='left'))
        lib = sh.libs[p['lib_id']]
        fpd = [uq(e[2]) for e in lib if isinstance(e, list) and e[0] == 'property' and uq(e[1]) == 'Footprint']
        sym.append(prop('Footprint', p['fp'] if p['fp'] is not None else (fpd[0] if fpd else ''), x, y, 0, hide=True))
        ds = [uq(e[2]) for e in lib if isinstance(e, list) and e[0] == 'property' and uq(e[1]) == 'Datasheet']
        sym.append(prop('Datasheet', ds[0] if ds else '', x, y, 0, hide=True))
        for k, v in p['props'].items(): sym.append(prop(k, v, x, y, 0, hide=True))
        for pin in p['pins']: sym.append(['pin', q(pin['num']), ['uuid', q(U())]])
        sym.append(['instances', ['project', q(PROJECT), ['path', q(f'/{self.root_uuid}/{sh.uuid}'),
                    ['reference', q(p['ref'])], ['unit', str(p.get('unit', 1))]]]])
        return sym

    def _label(self, net, x, y, ang, glob):
        just = {0: 'left', 90: 'left', 180: 'right', 270: 'right'}[ang]
        if glob:
            return ['global_label', q(net), ['shape', 'passive'], ['at', fmt(x), fmt(y), str(ang)],
                    ['fields_autoplaced', 'yes'], font(just), ['uuid', q(U())],
                    prop('Intersheetrefs', '${INTERSHEET_REFS}', x, y, ang, hide=True)]
        return ['label', q(net), ['at', fmt(x), fmt(y), str(ang)], ['fields_autoplaced', 'yes'],
                font(just + ' bottom'), ['uuid', q(U())]]

    def render_sheet(self, sh):
        body = []
        for it in sh.items:
            if isinstance(it, tuple):
                _, net, x, y, ang = it
                if net in POWER: body.append(self._power(sh, net, x, y, ang))
                else:
                    glob = len(self.netuse[net]) > 1 or net in self.force_global
                    body.append(self._label(net, x, y, ang, glob))
            else: body.append(it)
        for p in sh.parts: body.append(self._symbol(sh, p))
        for t, x, y in sh.texts:
            body.append(['text', q(t), ['exclude_from_sim', 'no'], ['at', fmt(x), fmt(y), '0'],
                         font('left top'), ['uuid', q(U())]])
        doc = ['kicad_sch', ['version', '20260306'], ['generator', q('eeschema')],
               ['generator_version', q('10.0')], ['uuid', q(sh.uuid)], ['paper', q(sh.paper)],
               ['title_block', ['title', q(sh.name)], ['company', q('Scanner 3D portable - projet 2A')]],
               ['lib_symbols'] + list(sh.libs.values())] + body + [['embedded_fonts', 'no']]
        return dump(doc) + '\n'

    def render_root(self, notes):
        body = []; x0, y0 = 30.48, 40.64
        for i, sh in enumerate(self.sheets):
            x = x0 + (i % 4) * 88.9; y = y0 + (i // 4) * 50.8
            body.append(['sheet', ['at', fmt(x), fmt(y)], ['size', '63.5', '25.4'],
                         ['exclude_from_sim', 'no'], ['in_bom', 'yes'], ['on_board', 'yes'], ['dnp', 'no'],
                         ['fields_autoplaced', 'yes'],
                         ['stroke', ['width', '0.1524'], ['type', 'solid']],
                         ['fill', ['color', '0', '0', '0', '0.0000']], ['uuid', q(sh.uuid)],
                         prop('Sheetname', sh.name, x, y - 0.7, 0, just='left bottom'),
                         prop('Sheetfile', sh.fname, x, y + 26.0, 0, just='left top'),
                         ['instances', ['project', q(PROJECT), ['path', q(f'/{self.root_uuid}'), ['page', q(sh.page)]]]]])
        for t, x, y in notes:
            body.append(['text', q(t), ['exclude_from_sim', 'no'], ['at', fmt(x), fmt(y), '0'],
                         font('left top'), ['uuid', q(U())]])
        doc = ['kicad_sch', ['version', '20260306'], ['generator', q('eeschema')],
               ['generator_version', q('10.0')], ['uuid', q(self.root_uuid)], ['paper', q('A3')],
               ['title_block', ['title', q('Scanner 3D portable - PCB final')],
                ['company', q('Projet 2A')]],
               ['lib_symbols']] + body + [['sheet_instances', ['path', q('/'), ['page', q('1')]]],
                                          ['embedded_fonts', 'no']]
        return dump(doc) + '\n'
