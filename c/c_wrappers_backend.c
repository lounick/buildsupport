/* Buildsupport is (c) 2008-2016 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* c_wrappers_backend.c

  this module generates the C code to interface the VM (using PolyORB_HI/C)
  with the functional code

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>

#include "my_types.h"
#include "practical_functions.h"
#include "c_ast_construction.h"

static FILE *h = NULL, *cfile = NULL;

/* Adds header to files */
void c_wrappers_preamble(FV * fv)
{
    /*
     * hasparam is to check if one interface at least has a param (or more)
     * this is used to decide if we need to include type definitions or not
     */
    bool hasparam = false;

    /*
     * mix is to check if the function contains a mix of unprotected and
     * asynchronous provided interfaces, in order to know what includes we
     * have to put in the wrappers
     */
    int mix = 0;

    /* a. header file preamble */

    fprintf(h, "%s"
               "#ifndef %s_POLYORB_INTERFACE\n"
               "#define %s_POLYORB_INTERFACE\n",
               do_not_modify_warning,
               fv->name,
               fv->name);

    /* Generates #include "types.h" (in Ada:PolyORB_HI.Generated.Types)
     * IF at least one async IF has a param */
    FOREACH(i, Interface, fv->interfaces, {
        if (asynch == i->synchronism && (NULL != i->in || NULL != i->out)) {
            hasparam = true;
        }
    });

    fprintf (h, "#include <stddef.h>\n\n");

    if (hasparam) {
        fprintf (h, "#include \"types.h\"\n");
        fprintf(cfile, "#include \"po_hi_gqueue.h\"\n");
    }

    /* Note: if user code is in Ada, from C it is not possible to make it
     * execute its elaboration. It must be WITH'd from an Ada source */

    fprintf(h, "#include \"deployment.h\"\n"
               "#include \"po_hi_transport.h\"\n");


    /* For each sync RI, include the distant (sync) wrapper
       multiple inclusion is prevented with the #ifndef at the begining
       of the file */
    FOREACH(i, Interface, fv->interfaces, {
        if (RI == i->direction && synch == i->synchronism) {
            fprintf(h, "#include \"../../%s/%s_polyorb_interface.h\"\n",
                       i->distant_fv,
                       i->distant_fv);
        }
    });

    /*
     * Include the calling threads wrappers to the wrappers of the
     * passive functions, to have access to their "vm_" functions
     * (only if the calling thread in question has some async RI)
     * more precisely it is not only for passive functions, it is also
     * for functions that include at least one unpro PI
     * (they can be mixed with threads). The case of pro is different,
     * we cannot reach this point with a mix of pro/async because
     * there are splitted in the preprocessing backend.
     * Thus 1) we check if it is a thread and yet there is at least
     * one unpro PI and 2) if it is the case OR if it is a passive function,
     * we do the following processing
     */
    if (thread_runtime == fv->runtime_nature) {
        FOREACH(i, Interface, fv->interfaces, {
            if (PI == i->direction && unprotected == i->rcm) {
                mix = 1;
            }
        });
    }
    if (passive_runtime == fv->runtime_nature || mix) {
        FOREACH(ct, FV, fv->calling_threads, {
            bool result = false;
            FOREACH(i, Interface, ct->interfaces, {
                if (RI == i->direction && asynch == i->synchronism) {
                    result = true;
                }
            });
            if (result) {
                fprintf(h,
                        "#include \"../../%s/%s_polyorb_interface.h\"\n",
                        ct->name, ct->name);
            }
        });
    }

    /* body file preamble */
    fprintf(cfile, "%s"
                   "#include \"%s_polyorb_interface.h\"\n\n"
                   "#include \"activity.h\"\n"
                   "#include \"types.h\"\n",
                   do_not_modify_warning,
                   fv->name);

    /* If this function has protected PI, include POHIC semaphore header */
    if (passive_runtime == fv->runtime_nature) {
        int haspro = 0;
        FOREACH(i, Interface, fv->interfaces, {
            if (PI == i->direction && protected == i->rcm) haspro = 1;
        });
        if (haspro) {
            fprintf(cfile, "#include \"po_hi_protected.h\"\n\n");
        }
    }

    /* Include the header files to get the function prototypes */
    if (!fv->artificial) {
        if (blackbox_device != fv->language
                && simulink != fv->language
                && qgenc    != fv->language
                && vhdl     !=  fv->language) {
            fprintf (cfile, "#include \"%s_vm_if.h\"\n\n", fv->name);
        }
        else if(simulink != fv->language
                && qgenc != fv->language
                && vhdl  != fv->language) {
            fprintf (cfile, "#include \"%s.h\"\n\n", fv->name);
        }
        else if (simulink == fv->language) {
            Interface *simulink_entrypoint = fv->interfaces->value;
            fprintf (cfile, "#include \"%s_Simulink.Simulink.h\"\n\n",
                            simulink_entrypoint->name);
        }
    }
}

int Init_C_Wrappers_Backend(FV * fv)
{
    char *filename = NULL;
    size_t len = 0;
    char *path = NULL;

    if (NULL != fv->system_ast->context->output)
        build_string(&path, fv->system_ast->context->output,
                     strlen(fv->system_ast->context->output));
    build_string(&path, fv->name, strlen(fv->name));

    /* Elaborate the Wrapper file name: */
    len = strlen(fv->name) + strlen("_polyorb_interface.c");
    filename = (char *) malloc(len + 1);

    /* create .c file */
    sprintf(filename, "%s_polyorb_interface.c", fv->name);
    create_file(path, filename, &cfile);

    /* create .h file */
    sprintf(filename, "%s_polyorb_interface.h", fv->name);
    create_file(path, filename, &h);

    free(filename);

    free(path);

    assert(NULL != cfile && NULL != h);

    c_wrappers_preamble(fv);

    return 0;
}


void close_c_wrappers()
{
    close_file(&cfile);
    close_file(&h);
}

/*
   Add a synchronous (protected or unprotected) interface to the wrapper
   In case of Protected PI, we use a semaphore provided by POHIC, as
   requested by the vertical transformation by specifying DATA constructs.
*/
void add_sync_PI_to_c_wrappers(Interface * i)
{
    if (NULL == h || NULL == cfile)
        return;

    /* header file : declare the function  */
    fprintf(h, "/*----------------------------------------------------\n"
               "-- %srotected Provided Interface \"%s\"\n"
               "----------------------------------------------------*/\n"

               "void sync_%s_%s(",
               protected == i->rcm ? "P" : "Unp", i->name,
               i->parent_fv->name,
               i->name);

    bool comma = false;
    FOREACH(p, Parameter, i->in, {
        (void) p;
        fprintf(h, "%svoid *, size_t", comma? ", ": "");
        comma = true;
    });

    FOREACH(p, Parameter, i->out, {
        (void) p;
        fprintf(h, "%svoid *, size_t *", comma? ", ": "");
        comma = true;
    });

    fprintf(h, ");\n\n");

    /* body file : declare the function */
    fprintf(cfile,
            "/*----------------------------------------------------\n"
            "-- %srotected Provided Interface \"%s\"\n"
            "----------------------------------------------------*/\n"

            "void sync_%s_%s(",
            protected == i->rcm ? "P" : "Unp",
            i->name,
            i->parent_fv->name,
            i->name);

    comma = false;
    FOREACH(p, Parameter, i->in, {
            fprintf(cfile, "%svoid *%s, size_t %s_len",
                           comma? ", ": "",
                           p->name,
                           p->name);
            comma = true;
    });

    FOREACH(p, Parameter, i->out, {
        fprintf(cfile, "%svoid *%s, size_t *%s_len",
                       comma? ", ": "",
                       p->name,
                       p->name);
        comma = true;
    });

    fprintf(cfile, ")\n{\n");

    /* body of the function: */
    if (protected == i->rcm) {
        fprintf(cfile, "\textern %staste_protected_object %s_protected;\n",
                       get_context()->aadlv2 ? "process_package__" : "",
                       i->parent_fv->name);
        fprintf(cfile,
                "\t__po_hi_protected_lock (%s_protected.protected_id);\n",
                i->parent_fv->name);
    }

    fprintf(cfile, "\t%s_%s(", i->parent_fv->name, i->name);

    FOREACH(p, Parameter, i->in, {
        fprintf(cfile, "%s%s, %s_len", p == i->in->value ? "" : ", ",
                       p->name, p->name);
    });

    if (NULL != i->in && NULL != i->out) {
        fprintf(cfile, ", ");
    }

    FOREACH(p, Parameter, i->out, {
        fprintf(cfile, "%s%s, %s_len",
                       p == i->out->value ? "" : ", ",
                       p->name,
                       p->name);
    });

    fprintf(cfile, ");\n");

    if (protected == i->rcm) {
        fprintf(cfile,
                "\t__po_hi_protected_unlock (%s_protected.protected_id);\n",
                i->parent_fv->name);
    }

    fprintf(cfile, "}\n\n");
}

/*
   Add a asynchronous provided interface to the wrapper.
   This generated function is called by polyorb-hi-c
*/
void add_async_PI_to_c_wrappers(Interface * i)
{
    char *pi_name = string_to_lower(i->name);

    if (NULL == h || NULL == cfile)
        return;

    /* header file : declare the function */
    fprintf(h, "/*----------------------------------------------------\n"
               "-- Asynchronous Provided Interface \"%s\"\n"
               "----------------------------------------------------*/\n"

               "void po_hi_c_%s_%s(__po_hi_task_id",
               i->name,
               i->parent_fv->name,
               pi_name);

    if (NULL != i->in) {
        fprintf(h, ", dataview__%s_buffer_impl",
                   string_to_lower(i->in->value->type));
    }

    fprintf(h, ");\n\n");

    /* body file : define the function */
    fprintf(cfile,
            "/* ------------------------------------------------------\n"
            "-- Asynchronous Provided Interface \"%s\"\n"
            "------------------------------------------------------ */\n"

            "void po_hi_c_%s_%s(__po_hi_task_id e",
            i->name,
            i->parent_fv->name,
            pi_name);

    if (NULL != i->in) {
        fprintf(cfile, ", dataview__%s_buffer_impl buf",
                       string_to_lower(i->in->value->type));
    }

    fprintf(cfile, ")\n{\n");

    /* Then 2 options:
       1) either we are in a thread created by the VT, in which case the PI
          can directly call the corresponding sync RI (no data decoding)
       2) or it is a user thread, in which case we call the function
          that is defined in vm_if.c
    */
    if (i->parent_fv->artificial) {
        char *distant_fv = NULL;
        FOREACH(interface, Interface, i->parent_fv->interfaces, {
            if (RI == interface->direction
                && !strcmp(interface->distant_name, i->distant_name)
                && synch == interface->synchronism) {
                distant_fv = interface->distant_fv;
            }
        });
        fprintf(cfile, "\tsync_%s_%s (",
                       distant_fv,
                       i->distant_name);
    } else {
        fprintf(cfile, "\t%s_%s(", i->parent_fv->name, i->name);
    }
    if (NULL != i->in) {
        fprintf(cfile, "buf.buffer, buf.length");
    }
    fprintf(cfile, ");\n}\n\n");
    free(pi_name);
}

/* Add a RI */
void add_RI_to_c_wrappers(Interface * i)
{
    FILE *s = h, *b = cfile;

    if (NULL == s || NULL == b)
        return;

    /* b. header : declare the function */
    fprintf(s,
            "/* ------------------------------------------------------\n");
    fprintf(s, "--  %s Required Interface \"%s\"\n",
               asynch == i->synchronism ? "Asynchronous" : "Synchronous",
               i->name);
    fprintf(s, "------------------------------------------------------ */\n");

    fprintf(s, "void vm_%s%s_%s(",
               asynch == i->synchronism ? "async_" : "",
               i->parent_fv->name,
               i->name);

    FOREACH(p, Parameter, i->in, {
        fprintf(s, "%svoid *%s, size_t %s_len",
                   p == i->in->value ? "" : ", ",
                   p->name,
                   p->name);
    });

    if (NULL != i->in && NULL != i->out) {
        fprintf(s, ", ");
    }

    FOREACH(p, Parameter, i->out, {
        fprintf(s, "%svoid *, size_t *",
                   p == i->out->value ? "" : ", ");
    });

    fprintf(s, ");\n");

    /* c. code : definition of the function */
    fprintf(b,
            "/* ------------------------------------------------------\n"
            "--  %s Required Interface \"%s\"\n"
            "------------------------------------------------------ */\n"
            "void vm_%s%s_%s(",
            asynch == i->synchronism ? "Asynchronous" : "Synchronous",
            i->name,
            asynch == i->synchronism ? "async_" : "", i->parent_fv->name,
            i->name);

    FOREACH(p, Parameter, i->in, {
        fprintf(b, "%svoid *%s, size_t %s_len",
                   p == i->in->value ? "" : ", ",
                   p->name,
                   p->name);
    });

    if (NULL != i->in && NULL != i->out) {
        fprintf(b, ", ");
    }

    FOREACH(p, Parameter, i->out, {
        fprintf(b, "%svoid *%s, size_t *%s_len",
                   p == i->out->value ? "" : ", ",
                   p->name,
                   p->name);
    });

    fprintf(b, ")\n{\n");

    if (asynch == i->synchronism) {
        char *ri_name = string_to_lower(i->name);
        /* 
         * RI is asynchronous.
         * This means that depending on the nature of the current FV
         * (thread, passive) we have either to call the VM
         * (if we are in a thread) or to switch-case the caller thread to call
         * his own access to the VM.
         * Only one IN parameter in the case of Async RI.
         */
        if (thread_runtime == i->parent_fv->runtime_nature) {
            /* Current FV is a thread -> send the message to PolyORB */
            fprintf(b, "\t__po_hi_request_t request;\n\n");

            /* If the message has parameters, then copy it to POHIC buffers */
            if (NULL != i->in) {
                fprintf(b, "\t__po_hi_copy_array"
                           "(&(request.vars.%s_global_outport_%s."
                           "%s_global_outport_%s.buffer),"
                           " %s, %s_len);\n",
                           i->parent_fv->name, /* sending port identifier */
                           ri_name,            /* data identifier */
                           i->parent_fv->name, /* sending port identifier */
                           ri_name,            /* data identifier */
                           i->in->value->name, /* unique IN parameter buffer */
                           i->in->value->name);/* unique IN parameter size   */

                fprintf(b, "\trequest.vars.%s_global_outport_%s"
                           ".%s_global_outport_%s.length = %s_len;\n",
                           i->parent_fv->name,  /* sending port identifier */
                           ri_name,             /* data identifier */
                           i->parent_fv->name,  /* sending port identifier */
                           ri_name,             /* data identifier = RI name */
                           i->in->value->name); /* unique IN parameter size  */
            }

            /* Also set the port number to identify the message */
            fprintf(b, "\trequest.port = %s_global_outport_%s;\n",
                    i->parent_fv->name, ri_name);

            fprintf(b, "\t__po_hi_gqueue_store_out("
                       "%s_%s_k, %s_local_outport_%s, &request);\n",
                       i->parent_fv->process->identifier, /* sending node */
                       i->parent_fv->name,
                       i->parent_fv->name,
                       ri_name);

            /* Direct invocation of RI */
            fprintf(b, "\t__po_hi_send_output("
                       "%s_%s_k, %s_global_outport_%s);\n",
                       i->parent_fv->process->identifier,
                       i->parent_fv->name,
                       i->parent_fv->name,
                       ri_name);

            free(ri_name);

        } else {                /* Current FV = Passive function */
            /* When a passive function calls a required interface, it uses
             * the RI interface defined for the thread of its caller */

            int count = 0;

            /* Build the list of calling threads for this RI */
            FV_list *calltmp = NULL;
            if (NULL == i->calling_pis) calltmp = i->parent_fv->calling_threads;
            else {
                FOREACH(calling_pi, Interface, i->calling_pis, {
                    FOREACH(thread_caller, FV, calling_pi->calling_threads, {
                        ADD_TO_SET(FV, calltmp, thread_caller);
                    });
                });
            }

            /* Count the number of calling threads */
            FOREACH(ct, FV, calltmp, {
                (void) ct;
                count ++;
            });
            if (1 == count) {
                fprintf(b, "\tvm_async_%s_%s_vt(",
                           calltmp->value->name,
                           i->name);

                /*  If any, add the IN parameter (async RI) */
                if (NULL != i->in) {
                    fprintf(b, "%s, %s_len",
                               i->in->value->name,
                               i->in->value->name);
                }
                fprintf(b, ");\n");
            }
            else if (count > 1) {
                /* Several possible callers: get current thread id */
                fprintf(b, "\tswitch(__po_hi_get_task_id()) {\n");
                FOREACH(caller, FV, calltmp, {
                    fprintf(b, "\t\tcase %s_%s_k: vm_async_%s_%s_vt(",
                            caller->process->identifier,
                            caller->name,
                            caller->name,
                            i->name);
                    if (NULL != i->in) {
                        /* Add the IN parameter (async RI) */
                        fprintf(b, "%s, %s_len",
                                   i->in->value->name,
                                   i->in->value->name);
                        }
                        fprintf(b, "); break;\n");
                });
                fprintf(b, "\t\tdefault: break;\n");
                fprintf(b, "\t}\n");
            }
        }
    }

    else {   /* Synchronous RI: direct call to remote polyorb_interface.c */

        fprintf(b, "\tsync_%s_%s(",
                i->distant_fv,
                NULL != i->distant_name ? i->distant_name : i->name);

        bool comma = false;
        FOREACH(p, Parameter, i->in, {
            fprintf(b, "%s%s, %s_len",
                       comma? ", ": "",
                       p->name,
                       p->name);
            comma = true;
        });

        FOREACH(p, Parameter, i->out, {
            fprintf(b, "%s%s, %s_len",
                       comma? ", ": "",
                       p->name,
                       p->name);
            comma = true;
        });
        fprintf(b, ");\n");
    }

    fprintf(b, "}\n\n");
}

void End_C_Wrappers_Backend(FV * fv)
{
    (void) fv;
// If necessary, add a protected object to the wrapper files
    /* TBD for PO HI C
       extern void Add_Protected_Interfaces(FV *, FILE *, FILE *);
       extern void Add_Unprotected_Interfaces(FV *, FILE *, FILE *);
       Add_Protected_Interfaces (fv, h, cfile);
       Add_Unprotected_Interfaces(fv, h, cfile); */

    fprintf(h, "#endif\n");
    close_c_wrappers();
}

/* Function to process one interface of the FV */
void GLUE_C_Wrappers_Interface(Interface * i)
{

    if (PI == i->direction) {
        if (asynch == i->synchronism)
            add_async_PI_to_c_wrappers(i);
        else
            add_sync_PI_to_c_wrappers(i);
    } else if (RI == i->direction) {
        add_RI_to_c_wrappers(i);
    }
}

/* External interface for the Ada wrappers backend */
void GLUE_C_Wrappers_Backend(FV * fv)
{
    if (fv->system_ast->context->onlycv)
        return;

    if (!fv->system_ast->context->polyorb_hi_c)
        return;

    else {
        Init_C_Wrappers_Backend(fv);
        FOREACH(i, Interface, fv->interfaces, {
            GLUE_C_Wrappers_Interface(i);
        });

        End_C_Wrappers_Backend(fv);
    }

}
