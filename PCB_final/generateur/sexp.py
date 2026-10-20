import re
TOK=re.compile(r'\s*(\(|\)|"(?:[^"\\]|\\.)*"|[^\s()]+)')
def parse(s):
    pos=0; stack=[[]]
    for m in TOK.finditer(s):
        t=m.group(1)
        if t=='(': stack.append([])
        elif t==')':
            x=stack.pop(); stack[-1].append(x)
        else: stack[-1].append(t)
    return stack[0][0]
def dump(x,ind=0):
    if not isinstance(x,list): return x
    simple=all(not isinstance(e,list) for e in x)
    if simple: return '('+' '.join(x)+')'
    out='('+x[0]
    for e in x[1:]:
        out+='\n'+'\t'*(ind+1)+dump(e,ind+1) if isinstance(e,list) else ' '+e
    return out+'\n'+'\t'*ind+')'
def q(s): return '"'+s.replace('\\','\\\\').replace('"','\\"').replace('\n','\\n')+'"'
def uq(s): return s[1:-1].replace('\\"','"').replace('\\\\','\\') if s.startswith('"') else s
def find(x,key): return [e for e in x if isinstance(e,list) and e and e[0]==key]
def find1(x,key):
    r=find(x,key); return r[0] if r else None
