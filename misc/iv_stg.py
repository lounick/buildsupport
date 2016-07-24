#!/usr/bin/env python2

import iv
import stringtemplate3

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

print str(tpl).encode('latin1')

    

