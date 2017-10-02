/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* semantic_checks.c

  this backend verifies some semantic properties on the interface and deployment views
 
*/
#define ID "$Id: semantic_checks.c 400 2009-11-24 13:28:24Z maxime1008 $"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "my_types.h"
#include "practical_functions.h"

/* Various semantic checks on an interface */
void Interface_Semantic_Check(Interface * i)
{
    int count_param = 0;

    /* Check that Cyclic PIs are not connected. */
    if (PI == i->direction && cyclic == i->rcm && NULL != i->distant_fv) {

        ERROR
            ("[ERROR] In Function \"%s\", interface \"%s\" is cyclic and should not be connected. \n",
             i->parent_fv->name, i->name);
        add_error();

    }

    /* Check if Cyclic PIs have parameters, which is forbidden */
    if (PI == i->direction && cyclic == i->rcm
        && (NULL != i->in || NULL != i->out)) {

        ERROR
            ("[ERROR] In Function \"%s\", interface \"%s\" is cyclic and should not have parameters. \n",
             i->parent_fv->name, i->name);
        add_error();

    }

    if (PI == i->direction && (sporadic == i->rcm || variator == i->rcm)) {


        /* Check that SPO interfaces only have ONE input parameter */
        FOREACH(p, Parameter, i->in, {
                (void) p;
                count_param++;
                });

        if (count_param > 1) {
            ERROR
                ("[ERROR] In Function \"%s\", sporadic interface \"%s\" cannot have more than 1 IN param.\n",
                 i->parent_fv->name, i->name);
            add_error();
        }

        /* Check that SPO interfaces don't have any OUT parameters */
        if (NULL != i->out) {
            ERROR
                ("[ERROR]  In Function \"%s\", sporadic interface \"%s\" cannot have OUT parameters.\n",
                 i->parent_fv->name, i->name);
            add_error();
        }

    }

}

bool Equal_Params(Parameter * p1, Parameter * p2)
{
    bool result=true;

    if (!Compare_String (p1->name, p2->name)) {
        result=false;
    }
    if (!Compare_String (p1->type, p2->type)) {
        result=false;
    }
    if (!Compare_ASN1_Filename (p1->asn1_filename, p2->asn1_filename)) {
        result=false;
    }
    if (!Compare_ASN1_Module (p1->asn1_module, p2->asn1_module)) {
        result=false;
    }
    if (p1->basic_type != p2->basic_type) {
        result=false;
    }
    if (p1->encoding != p2->encoding) {
        result=false;
    }
    if (p1->param_direction != p2->param_direction) {
        result=false;
    }

    return result;
}

bool MultiInstance_SDL_Interface_Check(Interface * i, Interface * j)
{
    bool result=true;
    int in_param_cnt_i = 0;
    int in_param_cnt_j = 0;
    int out_param_cnt_i = 0;
    int out_param_cnt_j = 0;

    if (!Compare_Interface (i, j)) {
        result=false;
    }

    if (i->direction != j->direction) {
        result=false;
    }

    FOREACH(p, Parameter, i->in, {
        Parameter *inp = FindInParameter (i, p->name);
        if (!Equal_Params (p, inp)){
            result=false;
        }
        in_param_cnt_i++;
    });
    FOREACH(p, Parameter, j->in, {
        UNUSED (p);
        in_param_cnt_j++;
    });

    FOREACH(p, Parameter, i->out, {
        Parameter *outp = FindOutParameter (i, p->name);
        if (!Equal_Params (p, outp)){
            result=false;
        }
        out_param_cnt_i++;
    });
    FOREACH(p, Parameter, j->out, {
        UNUSED (p);
        out_param_cnt_j++;
    });

    if (in_param_cnt_i != in_param_cnt_j) {
        result=false;
    }
    if (out_param_cnt_i != out_param_cnt_j) {
        result=false;
    }

    return result;
}

/* External interface (the one and unique) */
void Function_Semantic_Check(FV * fv)
{
    if (fv->system_ast->context->glue && NULL == fv->process) {
        ERROR
            ("[ERROR] Function \"%s\" is not bound to any partition.\n",
             fv->name);
        add_error();
    }

    if (fv->system_ast->context->onlycv) {
        return;
    }

    /*
     * A function must have at least ONE PI - otherwise it is dead code
     * (with the exception of blackbox devices which may be connected to
     * an interrupt in the future, and GUIs that may have only TCs)
     */
    if (blackbox_device != fv->language && gui != fv->language) {
        int count_pi = 0;
        FOREACH(i, Interface, fv->interfaces, {
                if (PI == i->direction) count_pi++;}
        );
        if (0 == count_pi) {
            ERROR
                ("[ERROR] Function \"%s\" shall contain at least one provided interface.\n",
                 fv->name);
            add_error();
        }
    }

    /* 
     * SCADE/Simulink specific checks: exactly ONE PI and *NO* RI 
     */
    if (scade == fv->language || simulink == fv->language) {
        int count_pi = 0, count_ri = 0;

        FOREACH(i, Interface, fv->interfaces, {
                if (RI == i->direction) {
                count_ri++;}
                else {
                count_pi++;}
                }
        );

        if (1 != count_pi) {
            ERROR
                ("[ERROR] %s function \"%s\" must contain ONE provided interface\n",
                 scade == fv->language ? "SCADE" : "Simulink", fv->name);
            ERROR
                ("** (but possibly several input and output parameters).\n");
            add_error();
        }

        if (0 != count_ri) {
            ERROR
                ("[ERROR] %s function \"%s\" must NOT contain any required interface\n",
                 scade == fv->language ? "SCADE" : "Simulink", fv->name);
            ERROR
                ("** (use OUT parameters in the interface specification).\n");
            add_error();
        }
    }

    /* 
     * QGenC/QGenAda specific checks: *NO* RI
     */
    if (qgenc == fv->language || qgenada == fv->language) {
        int count_ri = 0;

        FOREACH(i, Interface, fv->interfaces, {
            if (RI == i->direction) {
                count_ri++;}
            else {
                if (asynch == i->synchronism) {
                    ERROR
                    ("[ERROR] Required interface %s of function %s should be PROTECTED or UNPROTECTED\n",
                     i->name, fv->name);
                    ERROR
                    ("   because it is connected to a function that is using QGen as a source language\n");
                    ERROR
                    ("   (remote calls cannot be made to QGen functions).\n");
                    add_error();
                }
            }
        });

        if (0 != count_ri) {
            ERROR
                ("[ERROR] %s function \"%s\" must NOT contain any required interface\n",
                 qgenc == fv->language ? "QGenC" : "QGenAda", fv->name);
            ERROR
                ("** (use OUT parameters in the interface specification).\n");
            add_error();
        }
    }

    /* 
     * SCADE specific check: function name and PI name must match
     */
    if (scade == fv->language) {
        Interface *i = NULL;

        if (NULL != fv->interfaces) {
            i = fv->interfaces->value;
        }

        if (strcmp(i->name, fv->name)) {
            ERROR
            ("[ERROR] PI \"%s\" should have the same name as SCADE function \"%s\"\n",
             i->name, fv->name);
            add_error();
        }
    }

    /* 
     * GUI-specific checks : 
     * (1) RIs must have a parameter
     * (2) All PI/RIs of GUIs must be sporadic
     */
    if (gui == fv->language) {
        FOREACH(i, Interface, fv->interfaces, {
            if (RI == i->direction) {
                if (NULL == i->in) {
                    ERROR("[ERROR] in GUI function \"%s\",\n"
                           "**    interface \"%s\" must contain a parameter.\n",
                           fv->name,
                           i->name); 
                    add_error();}
                }
                /* Undefined RCM corresponds to "any type" (=inherited from PI) */ 
                if (undefined != i->rcm && sporadic != i->rcm && variator != i->rcm) {
                    ERROR("[ERROR] All interfaces of GUI \"%s\"\n"
                           "**       must be SPORADIC (e.g. %s is not).\n",
                           fv->name,
                           i->name);
                    add_error();
                }
            }
        );
    }

    /*
     * Pro/Unpro checks: Passive functions must be bound to the same process
     * as all their calling threads (to handle possible static data).
     */
    FOREACH(i, Interface, fv->interfaces, {
            if (RI == i->direction && synch == i->synchronism
                && NULL != i->distant_fv) {
            FOREACH(function, FV, get_system_ast()->functions, {
                    if (!strcmp(function->name, i->distant_fv)
                        && function->process != fv->process) {
                    ERROR
                    ("[ERROR] Required interface %s of function %s should be SPORADIC\n",
                     i->name, fv->name);
                    ERROR
                    ("   because it is connected to a function that is bound to a different\n");
                    ERROR
                    ("   partition in your deployment view (remote calls cannot be synchronous)\n");
                    add_error();}
                    }
            );}
            }
    );

    FOREACH(i, Interface, fv->interfaces, {
            Interface_Semantic_Check(i);
            }
    );

    /*
     * SDL specific checks: is_component_type can only be true for SDL functions.
     * instance_of can only point to functions with is_component_type set to true.
     */
    if (fv->is_component_type) {
        if (sdl != fv->language) {
            ERROR
            ("[ERROR] Is_Component_Type is True for \"%s\". This is allowed only for SDL functions.\n",
             fv->name);
            add_error();
        }
    }

    if (NULL != fv->instance_of) {
        FV *definition = FindFV (string_to_lower(fv->instance_of));
        if (definition == NULL) {
            ERROR
            ("[ERROR] Component type \"%s\" (for instance \"%s\") not found.\n",
             fv->instance_of, fv->name);
            add_error();
            return;
        }
        else {
            if (false == definition->is_component_type) {
                ERROR
                ("[ERROR] Is_Component_Type is False for \"%s\". It should be True,\n",
                 definition->name);
                 ERROR
                ("   because function \"%s\" claims that it is an instance of \"%s\".\n",
                 fv->name, definition->name);
                add_error();
            }
            int fv_intf_cnt = 0;
            int definition_intf_cnt = 0;
            FOREACH(i, Interface, fv->interfaces, {
                Interface *def_i = FindInterface (definition, i->name);
                    if (def_i == NULL) {
                        ERROR
                        ("** Error: Interface \"%s\" of instance \"%s\" not found from definition \"%s\".\n",
                        i->name, fv->name, definition->name);
                        add_error();
                    } else {
                        if (!MultiInstance_SDL_Interface_Check (i, def_i)) {
                            ERROR
                            ("** Error: Interface \"%s\" of instance \"%s\" not matching with definition \"%s\".\n",
                            i->name, fv->name, definition->name);
                            add_error();
                        }
                    }

                fv_intf_cnt++;
            });
            FOREACH(i, Interface, definition->interfaces, {
                UNUSED (i);
                definition_intf_cnt++;
            });
            if (fv_intf_cnt != definition_intf_cnt) {
                ERROR
                ("** Error: Interface count mismatch between definition \"%s\" and instance \"%s\".\n",
                 definition->name, fv->name);
                add_error();
            }
        }
    }
}

/* External interface */
void Semantic_Checks()
{
    FOREACH(fv, FV, get_system_ast()->functions, {
            Function_Semantic_Check(fv);
    });

    /*
     * VHDL-specific check: there can be only ONE VHDL subsystem per node
     */
    FOREACH(process, Process, get_system_ast()->processes, {
            int count = 0;
            FOREACH(binding, Aplc_binding, process->bindings, {
                    if (vhdl == binding->fv->language) count++;}
            ); if (count > 1) {
            ERROR
            ("[ERROR] in a distributed system, a partition can contain only ONE\n");
            ERROR("   VHDL component. Partition \"%s\" has %d.\n",
                   process->name, count); add_error();}

    });
}
