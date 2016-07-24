#!/usr/bin/env python2

from collections import defaultdict
import stringtemplate3
import iv

STG = None

new = lambda inst: STG.getInstanceOf(inst)

def initialize_stg(stgfile='iv.st'):
    ''' Load the STG backend and set gobal STG pointer '''
    global STG

    # Load the file containing a group of templates
    STG = stringtemplate3.StringTemplateGroup(file=open(stgfile))

initialize_stg()

tpl = new("interface_view")

tpl['arrsFunctNames'] = iv.functions.keys()


connections = []  #  type: List[str]
for fromName, content in iv.functions.viewitems():
    group = defaultdict(list)
    for iName, iContent in content['interfaces'].viewitems():
        if iContent['direction'] == iv.RI:
            group[iContent['distant_fv']].append(iName)
    for destName, destContent in group.viewitems():
        tplConn = new("connection")
        tplConn['sFrom'] = fromName
        tplConn['sTo'] = destName
        tplConn['arrsMessages'] = destContent
        connections.append(str(tplConn))

tpl['arrsConnections'] = connections

print str(tpl).encode('latin1')



