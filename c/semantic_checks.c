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
            ("** Error: In Function \"%s\", interface \"%s\" is cyclic and should not be connected. \n",
             i->parent_fv->name, i->name);
        add_error();

    }

    /* Check if Cyclic PIs have parameters, which is forbidden */
    if (PI == i->direction && cyclic == i->rcm
        && (NULL != i->in || NULL != i->out)) {

        ERROR
            ("** Error: In Function \"%s\", interface \"%s\" is cyclic and should not have parameters. \n",
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
                ("** Error: In Function \"%s\", sporadic interface \"%s\" cannot have more than 1 IN param.\n",
                 i->parent_fv->name, i->name);
            add_error();
        }

        /* Check that SPO interfaces don't have any OUT parameters */
        if (NULL != i->out) {
            ERROR
                ("** Error:  In Function \"%s\", sporadic interface \"%s\" cannot have OUT parameters.\n",
                 i->parent_fv->name, i->name);
            add_error();
        }

    }

}


/* External interface (the one and unique) */
void Function_Semantic_Check(FV * fv)
{
    if (fv->system_ast->context->glue && NULL == fv->process) {
        ERROR
            ("** Error: Function \"%s\" is not bound to any partition.\n",
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
                ("** Error: Function \"%s\" shall contain at least one provided interface.\n",
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
                ("** Error: %s function \"%s\" must contain ONE provided interface\n",
                 scade == fv->language ? "SCADE" : "Simulink", fv->name);
            ERROR
                ("** (but possibly several input and output parameters).\n");
            add_error();
        }

        if (0 != count_ri) {
            ERROR
                ("** Error: %s function \"%s\" must NOT contain any required interface\n",
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
                    ("** Error: Required interface %s of function %s should be PROTECTED or UNPROTECTED\n",
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
                ("** Error: %s function \"%s\" must NOT contain any required interface\n",
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
            ("** Error: PI \"%s\" should have the same name as SCADE function \"%s\"\n",
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
                    ERROR("** Error: in GUI function \"%s\",\n"
                           "**    interface \"%s\" must contain a parameter.\n",
                           fv->name,
                           i->name); 
                    add_error();}
                }
                /* Undefined RCM corresponds to "any type" (=inherited from PI) */ 
                if (undefined != i->rcm && sporadic != i->rcm && variator != i->rcm) {
                    ERROR("** Error: All interfaces of GUI \"%s\"\n"
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
                    ("** Error: Required interface %s of function %s should be SPORADIC\n",
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
            ("** Error: in a distributed system, a partition can contain only ONE\n");
            ERROR("   VHDL component. Partition \"%s\" has %d.\n",
                   process->name, count); add_error();}

    });
}
