import os,copy
from sexp import *
KL='/Applications/KiCad/KiCad.app/Contents/SharedSupport/symbols'
HERE=os.path.dirname(os.path.abspath(__file__))
LOCAL=os.path.join(HERE,'..','libs')
def libpath(lib):
    p=os.path.join(LOCAL,lib+'.kicad_sym')
    return p if os.path.exists(p) else f'{KL}/{lib}.kicad_sym'
_cache={}
def libsyms(lib):
    if lib not in _cache:
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
