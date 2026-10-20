import os,copy
from sexp import *
KL='/Applications/KiCad/KiCad.app/Contents/SharedSupport/symbols'
REPO='/Users/moatsembellhelnemrawi/Documents/GitHub/3Dscan2A'
LIBFILES={}
def libpath(lib):
    loc={'IMU_symbole':REPO+'/PCB_prototype1/2026-09-29_14-03-09.kicad_sym',
         'Lidar':REPO+'/PCB_prototype1/VL53L9CX_large.kicad_sym',
         'carte_wifi':REPO+'/PCB_prototype1/module_wifi/ST67W611M1A6B.kicad_sym'}
    loc['STM32N6'] = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'STM32N6.kicad_sym')
    loc['ST_custom'] = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'ST_custom.kicad_sym')
    return loc.get(lib, f'{KL}/{lib}.kicad_sym')
_cache={}
IMU_TYPES={'1':'input','2':'input','3':'input','4':'output','5':'power_in','6':'power_in',
           '7':'power_in','8':'power_in','9':'output','10':'output','11':'bidirectional','12':'input','13':'input','14':'bidirectional'}
def fix_imu(sym):
    for sub in [sym]+find(sym,'symbol'):
        for p in find(sub,'pin'):
            p[1]=IMU_TYPES[uq(find1(p,'number')[1])]
def libsyms(lib):
    if lib not in _cache:
        if lib in ('IMU_symbole','Lidar','carte_wifi'):
            t=parse(open(REPO+'/PCB_prototype1/PCB_prototype1.kicad_sch').read())
            ls=find1(t,'lib_symbols'); d={}
            for s in find(ls,'symbol'):
                full=uq(s[1])
                if full.startswith(lib+':'):
                    s=copy.deepcopy(s); nm=full.split(':',1)[1]; s[1]=q(nm); d[nm]=s
            if lib=='IMU_symbole': fix_imu(d['LSM6DSV16X'])
            _cache[lib]=d
        else:
            t=parse(open(libpath(lib)).read())
            _cache[lib]={uq(s[1]):s for s in find(t,'symbol')}
    return _cache[lib]
def resolve(lib,name):
    """return flattened symbol sexp (top name = name) handling extends"""
    s=copy.deepcopy(libsyms(lib)[name])
    ext=find1(s,'extends')
    if ext:
        parent=resolve(lib,uq(ext[1]))
        pname=uq(parent[1])
        # child props override parent props
        cprops={uq(p[1]):p for p in find(s,'property')}
        new=[ 'symbol', q(name)]
        for e in parent[2:]:
            if isinstance(e,list) and e[0]=='property' and uq(e[1]) in cprops:
                new.append(cprops.pop(uq(e[1])))
            elif isinstance(e,list) and e[0]=='symbol':
                e=copy.deepcopy(e); e[1]=q(uq(e[1]).replace(pname,name,1)); new.append(e)
            else: new.append(e)
        # insert remaining child props before first sub symbol
        idx=next((i for i,e in enumerate(new) if isinstance(e,list) and e[0]=='symbol'),len(new))
        for p in cprops.values(): new.insert(idx,p); idx+=1
        s=new
    return s
def embedded(lib,name):
    s=resolve(lib,name); s=copy.deepcopy(s); s[1]=q(f'{lib}:{name}'); return s
def pins(lib,name,unit=1):
    s=resolve(lib,name); out=[]
    for sub in find(s,'symbol'):
        sn=uq(sub[1]); parts=sn.rsplit('_',2)
        u,b=int(parts[-2]),int(parts[-1])
        if u not in (0,unit) or b not in (0,1): continue
        for p in find(sub,'pin'):
            at=find1(p,'at'); nm=uq(find1(p,'name')[1]); num=uq(find1(p,'number')[1])
            hidden= 'hide' in p or bool(find1(p,'hide'))
            out.append(dict(name=nm,num=num,x=float(at[1]),y=float(at[2]),a=float(at[3]) if len(at)>3 else 0.0,type=p[1],hidden=hidden))
    return out
def units(lib,name):
    s=resolve(lib,name)
    return sorted({int(uq(sub[1]).rsplit('_',2)[-2]) for sub in find(s,'symbol')}-{0}) or [1]
if __name__=='__main__':
    import sys
    lib,name=sys.argv[1].split(':')
    print(units(lib,name))
    for p in pins(lib,name): print(p['num'],p['name'],p['type'],p['x'],p['y'],p['a'],'H' if p['hidden'] else '')
