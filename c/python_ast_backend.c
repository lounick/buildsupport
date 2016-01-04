/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
// python_AST_backend
//
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "my_types.h"
#include "practical_functions.h"

void Generate_Python_AST(System *s, char *dest_directory)
{
    FILE *py = NULL;
    if (NULL == dest_directory) {
        dest_directory = s->context->output;
    }

    create_file (dest_directory, "iv.py", &py);
    assert (NULL != py);

    fprintf (py,
        "#! /usr/bin/python\n\n"
        "Ada, C, GUI, SIMULINK, VHDL, OG, RTDS, SYSTEM_C, SCADE6 = range(9)\n"
        "thread, passive, unknown = range(3)\n"
        "PI, RI = range(2)\n"
        "synch, asynch = range(2)\n"
        "param_in, param_out = range(2)\n"
        "UPER, NATIVE, ACN = range(3)\n"
        "cyclic, sporadic, variator, protected, unprotected = range(5)\n"
        "enumerated, sequenceof, sequence, set, setof, integer, boolean, real, choice, octetstring, string = range(11)\n" 
        "functions = {}");

    FOREACH(fv, FV, s->functions, {
        fprintf (py,
            "\n\nfunctions['%s'] = {\n"
            "    'name_with_case' : '%s',\n"
            "    'runtime_nature': %s,\n"
            "    'language': %s,\n"
            "    'zipfile': '%s',\n"
            "    'interfaces': {},\n"
            "    'functional_states' : {}\n"
            "}",
                fv->name,
                fv->nameWithCase,
                NATURE(fv),
                LANGUAGE(fv),
                NULL != fv->zipfile? fv->zipfile:""
        );

        /* Generate C/Ada functional state (not directives or simulink params) */
        FOREACH(cp, Context_Parameter, fv->context_parameters, {
            if (strcmp (cp->type.name, "Taste-directive") &&
                strcmp (cp->type.name, "Simulink-Tunable-Parameter")) {
               fprintf (py,
                   "\n\nfunctions['%s']['functional_states']['%s'] = {\n"
                   "    'fullFsName' : '%s',\n"
                   "    'typeName' : '%s',\n"
                   "    'moduleName' : '%s',\n"
                   "    'asn1FileName' : '%s'\n"
                   "}",
                   fv->name,
                   cp->name,
                   cp->fullNameWithCase,
                   cp->type.name,
                   cp->type.module,
                   cp->type.asn1_filename
               );
           }
        }); 

        FOREACH(i, Interface, fv->interfaces, {
            fprintf (py,
                "\n\nfunctions['%s']['interfaces']['%s'] = {\n"
                "    'port_name': '%s',\n"
                "    'parent_fv': '%s',\n"
                "    'direction': %s,\n"
                "    'in': {},\n"
                "    'out': {},\n"
                "    'synchronism': %s,\n"
                "    'rcm': %s,\n"
                "    'period': %lld,\n"
                "    'wcet_low': %llu,\n"
                "    'wcet_low_unit': '%s',\n"
                "    'wcet_high': %llu,\n"
                "    'wcet_high_unit': '%s',\n"
                "    'distant_fv': '%s',\n"
                "    'calling_threads': {},\n"
                "    'distant_name': '%s',\n"
                "    'queue_size': %lld\n"
                "}",
                    fv->name,
                    i->name,
                    NULL != i->port_name? i->port_name:"",
                    i->parent_fv->name,
                    DIRECTION(i),
                    SYNCHRONISM(i),
                    RCM_KIND(i),
                    i->period,
                    (unsigned long long int)i->wcet_low,
                    NULL != i->wcet_low_unit?i->wcet_low_unit:"",
                    (unsigned long long int)i->wcet_high,
                    NULL != i->wcet_high_unit?i->wcet_high_unit:"",
                    NULL != i->distant_fv? i->distant_fv:"",
                    NULL != i->distant_name? i->distant_name:"",
                    i->queue_size
            );

            /* Emit a list of IN and OUT params in order (Python dicts are unordered) */
            fprintf (py,
                    "\n\nfunctions['%s']['interfaces']['%s']['paramsInOrdered'] = [",
                    fv->name,                                                                                                                                                                     
                    i->name);
            bool needsComma = false;
            FOREACH(p, Parameter, i->in, {
                fprintf (py,
                    "%s'%s'",
                    needsComma?", ":"",
                    p->name);
                needsComma = true;
            });

            fprintf (py, "]");

            fprintf (py,
                    "\n\nfunctions['%s']['interfaces']['%s']['paramsOutOrdered'] = [",
                    fv->name,                                                                                                                                                                     
                    i->name);

            needsComma = false;

            FOREACH(p, Parameter, i->out, {
                fprintf (py,
                    "%s'%s'",
                    needsComma?", ":"",
                    p->name);
                needsComma = true;
            });

            fprintf (py, "]");

            FOREACH(p, Parameter, i->in, {
                fprintf (py,
                    "\n\nfunctions['%s']['interfaces']['%s']['in']['%s'] = {\n"
                    "    'type': '%s',\n"
                    "    'asn1_module': '%s',\n"
                    "    'basic_type': %s,\n"
                    "    'asn1_filename': '%s',\n"
                    "    'encoding': %s,\n"
                    "    'interface': '%s',\n"
                    "    'param_direction': %s\n"
                    "}",
                    fv->name,
                    i->name,
                    p->name,
                    p->type,
                    p->asn1_module,
                    BASIC_TYPE(p),
                    p->asn1_filename,
                    BINARY_ENCODING(p),
                    i->name,
                    PARAM_DIRECTION(p)
                );
            });
            
            FOREACH(p, Parameter, i->out, {
                fprintf (py,
                    "\n\nfunctions['%s']['interfaces']['%s']['out']['%s'] = {\n"
                    "    'type': '%s',\n"
                    "    'asn1_module': '%s',\n"
                    "    'basic_type': %s,\n"
                    "    'asn1_filename': '%s',\n"
                    "    'encoding': %s,\n"
                    "    'interface': '%s',\n"
                    "    'param_direction': %s\n"
                    "}",
                        fv->name,
                        i->name,
                        p->name,
                        p->type,
                        p->asn1_module,
                        BASIC_TYPE(p),
                        p->asn1_filename,
                        BINARY_ENCODING(p),
                        i->name,
                        PARAM_DIRECTION(p)
                );
            });
        });
    });
    fprintf (py, "\n");
    close_file (&py);

    return;
}
