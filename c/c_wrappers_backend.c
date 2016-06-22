/* Buildsupport is (c) 2008-2015 European Space Agency
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
    int hasparam = 0;

    /*
     * mix is to check if the function contains a mix of unprotected and
     * asynchronous provided interfaces, in order to know what includes we
     * have to put in the wrappers
     */
    int mix = 0;

    /* a. header file preamble */

    fprintf(h,
            "%s"
            "#ifndef %s_POLYORB_INTERFACE\n"
            "#define %s_POLYORB_INTERFACE\n",
            do_not_modify_warning,
            fv->name,
            fv->name);

    /* Generates #include "types.h" (in Ada:PolyORB_HI.Generated.Types)
     * IF at least one async IF has a param */
    FOREACH(i, Interface, fv->interfaces, {
            if (asynch == i->synchronism
                && (NULL != i->in || NULL != i->out)) hasparam = 1;}
    );

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
            if (RI == i->direction && synch == i->synchronism)
            fprintf(h, "#include \"../../%s/%s_polyorb_interface.h\"\n",
                    i->distant_fv, i->distant_fv);}
    );

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
                if (PI == i->direction && unprotected == i->rcm) mix = 1;}
        );
    }
    if (passive_runtime == fv->runtime_nature || mix) {
        FOREACH(ct, FV, fv->calling_threads, {
                int result = 0; FOREACH(i, Interface, ct->interfaces, {
                                        if (RI == i->direction
                                            && asynch ==
                                            i->synchronism) result = 1;}
                ); if (result) {
                fprintf(h,
                        "#include \"../../%s/%s_polyorb_interface.h\"\n",
                        ct->name, ct->name);}
                }
        );
    }


    /* b. body file preamble */
    fprintf(cfile,
            "%s"
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

    /* Include stdio for the printf in the stack handlers */
    fprintf(cfile, "#ifdef __unix__\n"
                   "#include <stdio.h>\n"
                   "#endif\n\n");

    /* Include the header files to get the function prototypes */
    //char *path = make_string("%s/%s", OUTPUT_PATH, fv->name);
    //if (!file_exists(path, "_hook")) {
    if (!fv->artificial) {

        if (blackbox_device != fv->language
                && simulink != fv->language
                && qgenc != fv->language
                && vhdl !=  fv->language) {
            fprintf (cfile, "#include \"%s_vm_if.h\"\n\n", fv->name);
        }
        else if(simulink != fv->language
                && qgenc != fv->language
                && vhdl != fv->language) {
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

    /* Count the number of calling threads for this passive function */
    int count = 0;
    FOREACH(t, FV, i->parent_fv->calling_threads, {
            (void) t; count++;});

    /* header file : declare the function  */
    fprintf(h, "/*----------------------------------------------------\n");
    fprintf(h, "-- %srotected Provided Interface \"%s\"\n",
            protected == i->rcm ? "P" : "Unp", i->name);
    fprintf(h, "----------------------------------------------------*/\n");
    fprintf(h, "void sync_%s_%s(int", i->parent_fv->name, i->name);
    FOREACH(p, Parameter, i->in, {
            (void) p; fprintf(h, ", void *, size_t");});

    FOREACH(p, Parameter, i->out, {
            (void) p; fprintf(h, ", void *, size_t *");});

    fprintf(h, ");\n\n");

    /* body file : declare the function */
    fprintf(cfile,
            "/*----------------------------------------------------\n");
    fprintf(cfile, "-- %srotected Provided Interface \"%s\"\n",
            protected == i->rcm ? "P" : "Unp", i->name);
    fprintf(cfile,
            "----------------------------------------------------*/\n");
    fprintf(cfile, "void sync_%s_%s(int calling_thread",
            i->parent_fv->name, i->name);
    FOREACH(p, Parameter, i->in, {
            fprintf(cfile, ", void *%s, size_t %s_len", p->name, p->name);
            });

    FOREACH(p, Parameter, i->out, {
            fprintf(cfile, ", void *%s, size_t *%s_len", p->name, p->name);
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
    if (count > 1) {
        fprintf(cfile, "\t%s_callinglist_push(calling_thread);\n",
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
        fprintf(cfile, "%s%s, %s_len", p == i->out->value ? "" : ", ",
                    p->name, p->name);
    });

    fprintf(cfile, ");\n");

    if (count > 1) {
        fprintf(cfile, "\t%s_callinglist_pop();\n", i->parent_fv->name);
    }

    if (protected == i->rcm) {
        fprintf(cfile,
                "\t__po_hi_protected_unlock (%s_protected.protected_id);\n",
                i->parent_fv->name);
    }

    fprintf(cfile, "}\n\n");
}

/*
   Add a asynchronous provided interface to the wrapper.
   This generated function is called by polyorb-hi/C only
*/
void add_async_PI_to_c_wrappers(Interface * i)
{
    //char *path = NULL;
    char *pi_name = string_to_lower(i->name);

    if (NULL == h || NULL == cfile)
        return;

    /* header file : declare the function */
    fprintf(h, "/*----------------------------------------------------\n");
    fprintf(h, "-- Asynchronous Provided Interface \"%s\"\n", i->name);
    fprintf(h, "----------------------------------------------------*/\n");
    fprintf(h, "void po_hi_c_%s_%s(__po_hi_task_id", i->parent_fv->name,
            pi_name);
    if (NULL != i->in)
        fprintf(h, ", dataview__%s_buffer_impl",
                string_to_lower(i->in->value->type));
    fprintf(h, ");\n\n");

    /* body file : define the function */
    fprintf(cfile,
            "/* ------------------------------------------------------\n");
    fprintf(cfile, "-- Asynchronous Provided Interface \"%s\"\n", i->name);
    fprintf(cfile,
            "------------------------------------------------------ */\n");
    fprintf(cfile, "void po_hi_c_%s_%s(__po_hi_task_id e",
            i->parent_fv->name, pi_name);
    if (NULL != i->in)
        fprintf(cfile, ", dataview__%s_buffer_impl buf",
                string_to_lower(i->in->value->type));

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
                    && !strcmp(interface->distant_name, i->distant_name) &&
                    synch == interface->synchronism) distant_fv =
                interface->distant_fv;}
        );
        fprintf(cfile, "\tsync_%s_%s (%d%s", distant_fv, i->distant_name,       /* ok */
                i->parent_fv->thread_id, NULL != i->in ? ", " : "");
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
    fprintf(s,
            "------------------------------------------------------ */\n");

    fprintf(s, "void vm_%s%s_%s(",
            asynch == i->synchronism ? "async_" : "", i->parent_fv->name,
            i->name);

    FOREACH(p, Parameter, i->in, {
            fprintf(s, "%svoid *%s, size_t %s_len",
                    p == i->in->value ? "" : ", ", p->name, p->name);
            }
    );
    if (NULL != i->in && NULL != i->out) {
        fprintf(s, ", ");
    }
    FOREACH(p, Parameter, i->out, {
            fprintf(s, "%svoid *, size_t *",
                    p == i->out->value ? "" : ", ");
            }
    );

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
                   p == i->in->value ? "" : ", ", p->name, p->name);
    });

    if (NULL != i->in && NULL != i->out) {
        fprintf(b, ", ");
    }

    FOREACH(p, Parameter, i->out, {
            fprintf(b, "%svoid *%s, size_t *%s_len",
                    p == i->out->value ? "" : ", ", p->name, p->name);
            }
    );

    fprintf(b, ")\n{\n");

    if (asynch == i->synchronism) {
        char *ri_name = string_to_lower(i->name);
        /* 
         * RI is asynchronous.
         * This means that depending on the nature of the current FV
         * (thread, passive) we have either to call the VM
         * (if we are in a thread) or to switch-case the caller thread to call
         * his own access to the VM.
         * We retrieve the caller thread ID using the "stack"
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

            /* Direct invocation of RI (MP 15/08/13, on Astrium request) */
            fprintf(b, "\t__po_hi_send_output("
                       "%s_%s_k, %s_global_outport_%s);\n",
                       i->parent_fv->process->identifier,
                       i->parent_fv->name,
                       i->parent_fv->name,
                       ri_name);

            free(ri_name);

        } else {                /* Current FV = Passive function */

            int count = 0;
            fprintf(b,
                    "\t /* This function is passive, thus not have direct access to the VM\n");
            fprintf(b,
                    "\t  It must use its calling thread to invoke this asynchronous RI */\n\n");

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
            /* If there is only one possible caller, no need for stack */
            if (1 == count) {
                fprintf(b, "\tvm_async_%s_%s_vt(",
                        calltmp->value->name,
                        i->name);

                /*  If any, add the IN parameter (async RI) */
                if (NULL != i->in) {
                    fprintf(b, "%s, %s_len", i->in->value->name,
                            i->in->value->name);
                }

                fprintf(b, ");\n");
            }
            else if (count > 1) {     /* Several possible callers: switch-case based on the stack top value */
                fprintf(b, "\tswitch(%s_callinglist_get_top_value()) {\n",
                        i->parent_fv->name);
                FOREACH(caller, FV, calltmp, {
                        fprintf(b, "\t\tcase %d: vm_async_%s_%s_vt(",
                                   caller->thread_id,
                                   caller->name,
                                   i->name);
                        if (NULL != i->in) {     /* Add the unique IN parameter (async RI) */
                            fprintf(b, "%s, %s_len", i->in->value->name,
                                    i->in->value->name);
                        }
                        fprintf(b, "); break;\n");
                });
                fprintf(b, "\t\tdefault: break;\n");
                fprintf(b, "\t}\n");

            }
        }
    }

    else {                      /* synch==i->synchronism (RI is synchronous) :
                                   make a direct call to the remote function
                                   defined in remote polyorb_interface.c,
                                   and pass the thread id as parameter */
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

        fprintf(b, "\tsync_%s_%s(", i->distant_fv,
                NULL != i->distant_name ? i->distant_name : i->name);

        if (passive_runtime == i->parent_fv->runtime_nature) {
            if (count > 1) {
                fprintf(b, "%s_callinglist_get_top_value()",
                        i->parent_fv->name);
            } else if (1 == count) {
                fprintf(b, "%d",  calltmp->value->thread_id);
            } else if (0 == count) {
                printf
                    ("Warning: function \"%s\" is not called by anyone (dead code)!\n",
                     i->parent_fv->name);
                fprintf(b, "0");        /* whatever value we put, it is dead code anyway */
            }
        } else {
            fprintf(b, "%d", i->parent_fv->thread_id);
        }

        FOREACH(p, Parameter, i->in, {
                fprintf(b, ", %s, %s_len", p->name, p->name);
                }
        );

        FOREACH(p, Parameter, i->out, {
                fprintf(b, ", %s, %s_len", p->name, p->name);
                }
        );
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

/*
 Generate the code of a stack that handle the calling thread list needed to transparently make sure that
 when a sync PI has to call a RI, it does it through its calling thread.
*/
void Generate_C_CallingStack(FV * fv)
{
    int count = 0;

    /* Count the number of calling threads */
    FOREACH(ct, FV, fv->calling_threads, {
            (void) ct; count++;});

    if (2 > count)
        return;

    fprintf(cfile, "static int %s_stack[%d] = {0};\n\n", fv->name, count);
    /*
       In C we would need a semaphore to protect the stack.
       TO BE INVESTIGATED (we could create a new function with protected interfaces at system level)
       But when we are already in a protected function there is no risk to be preempted by a higher
       priority function so nothing special has to be done. What about unprotected functions?
     */

    /* get_top_value function */
    fprintf(cfile, "int %s_callinglist_get_top_value()\n{\n", fv->name);
    fprintf(cfile, "\tint i=0;\n");
    /* fprintf(cfile, "\tprintf(\"* %s_callinglist_get_top_value() invoked\\n\");\n", fv->name);  debug */
    fprintf(cfile, "\tfor(i=%d; i>=0; i--) {\n", count - 1);
    fprintf(cfile, "\t\tif (0 != %s_stack[i]) return %s_stack[i];\n\t}\n",
            fv->name, fv->name);
    fprintf(cfile,
            "#ifdef __unix__\n\tprintf(\"### STACK ERROR (GET TOP EMPTY STACK) in %s\\n\");\n#endif\n",
            fv->name);
    fprintf(cfile, "return -1;\n");
    fprintf(cfile, "}\n\n");

    /* push function */
    fprintf(cfile, "void %s_callinglist_push(int tid)\n{\n", fv->name);
    fprintf(cfile, "\tint i=0;\n");
    /* fprintf(cfile, "\tprintf(\"* %s_callinglist_push(%%d) invoked\\n\", tid);\n", fv->name); */
    fprintf(cfile, "\tif (0 != %s_stack[%d]) {\n", fv->name, count - 1);
    fprintf(cfile, "#ifdef __unix__\n\t\tprintf(\"### STACK ERROR (OVERFLOW), in %s\\n\");\n#endif\n", fv->name);       /* debug */
    fprintf(cfile, "\t}\n\telse {\n");
    fprintf(cfile,
            "\t\tfor(i=%d; i>=0; i--)\n\t\t\t if (0 != %s_stack[i]) {\n",
            count - 2, fv->name);
    fprintf(cfile,
            "\t\t\t\t%s_stack[i+1] = tid;\n\t\t\t\treturn;\n\t\t}\n",
            fv->name);
    fprintf(cfile, "\t%s_stack[0] = tid;\n", fv->name);
    fprintf(cfile, "\t}\n");
    fprintf(cfile, "}\n\n");

    /* pop function */
    fprintf(cfile, "void %s_callinglist_pop()\n{\n", fv->name);
    fprintf(cfile, "\tint i=0;\n");
    /* fprintf(cfile, "\tprintf(\"* %s_callinglist_pop() invoked\\n\");\n", fv->name); */
    fprintf(cfile, "\tfor(i=%d; i>=0; i--) {\n", count - 1);
    fprintf(cfile,
            "\t\tif (0 != %s_stack[i]) {\n\t\t\t%s_stack[i] = 0;\n\t\t\treturn;\n\t\t}\n\t}\n",
            fv->name, fv->name);
    fprintf(cfile, "#ifdef __unix__\n\tprintf(\"### STACK ERROR (POP EMPTY STACK) in %s\\n\");\n#endif\n", fv->name);   /* debug */
    fprintf(cfile, "}\n\n");

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

        if (passive_runtime == fv->runtime_nature) {
            Generate_C_CallingStack(fv);
        }

        /*ForEach(fv->interfaces, GLUE_C_Wrappers_Interface); */
        FOREACH(i, Interface, fv->interfaces, {
                GLUE_C_Wrappers_Interface(i);}
        );

        End_C_Wrappers_Backend(fv);
    }

}
