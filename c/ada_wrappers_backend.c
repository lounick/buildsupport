/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* ada_wrappers_backend.c

  this program generates the Ada code to interface the VM with the functional code

  update (24/11/2008) : do not discard creation of wrappers in case of Ada
                             used as implementation language

  updated 20/04/2009 to disable in case "-onlycv" flag is set

  major updated 20/07/2009 to support protected objects
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <assert.h>

#include "my_types.h"
#include "practical_functions.h"
#include "c_ast_construction.h"

extern void Add_Protected_Interfaces(FV * fv, FILE * ads, FILE * adb);
extern void Add_Unprotected_Interfaces(FV * fv, FILE * ads, FILE * adb);

static FILE *ads = NULL, *adb = NULL, *async_ads = NULL, *async_adb = NULL;

static int contains_sync_interface = 0;

/* Check if an async interface has input or output parameters */
void CheckForParams(Interface * i, int *result)
{
    if (asynch == i->synchronism && (NULL != i->in || NULL != i->out)) {
        *result = 1;
    }
}

/* For a given passive function, add "with CallingThread_wrappers;"statement */
void Add_With_AsyncRI(FV * fv, FILE ** file)
{
    bool result = false;
    FOREACH (i, Interface, fv->interfaces, {
        if (RI == i->direction && asynch == i->synchronism) result = true;
    })

    if (result)
        fprintf(*file, "with %s_async_ri_wrappers;\n", fv->name);
}


/* Adds header to files */
void ada_wrappers_preamble(FV * fv)
{
    int hasparam = 0;
    int mix = 0;

    if (NULL == ads || NULL == adb)
        return;

    /*  wrappers.ads top header */
    fprintf(ads,
            "--  This file was generated automatically: DO NOT MODIFY IT !\n\n");
    fprintf(ads, "pragma Style_Checks (Off);\npragma Warnings (Off);\n\n");

    /*  wrappers.adb top header */
    fprintf(adb,
            "--  This file was generated automatically: DO NOT MODIFY IT !\n\n");
    fprintf(adb,
            "pragma Style_Checks (Off);\npragma Warnings (Off);\n\nwith PolyORB_HI_Generated.Activity;\nuse PolyORB_HI_Generated.Activity;\n\n");


    fprintf(ads, "with Interfaces.C;\n");
    /* Generates "with PolyORB_HI.Generated.Types IF at least one async IF has a param */
    FOREACH(i, Interface, fv->interfaces, {
        CheckForParams(i, &hasparam);
    })
    if (hasparam) {
        fprintf(ads, "with PolyORB_HI_Generated.Types;\n");
    }
    // In case of Ada, make an include to the user code package to execute the elaboration part (optional user-initialization code)
    if (ada == fv->language || qgenada == fv->language) {
        fprintf(ads, "with %s;\n", fv->name);
    }

    fprintf(ads, "with PolyORB_HI_Generated.Deployment;\n");
    fprintf(ads, "use PolyORB_HI_Generated.Deployment;\n\n");

    /* 
     *  For each sync RI, add a "with distant_fv_wrappers;" clause in the adb file 
     *  Note: this cannot be put in the ads file because it may cause circular dependency errors 
     */
    String_list *sync_list = NULL;

    FOREACH(i, Interface, fv->interfaces, {
        if (RI == i->direction && synch == i->synchronism
            && i->distant_qgen->language == other) {
                ADD_TO_SET(String, sync_list, i->distant_fv);
        }
    })
    FOREACH(sync_callee, String, sync_list, {
        fprintf(adb, "with %s_wrappers;\n", sync_callee);
    });

    if (NULL != async_ads)
        fprintf(ads, "with %s_async_ri_wrappers;\n", fv->name);

    /* Include with and use clauses for QGenAda code if it is used */
    FOREACH(i, Interface, fv->interfaces, {
    if (qgenada == i->distant_qgen->language) {
        fprintf(ads, "with taste_dataview;\n");
        fprintf(ads, "use taste_dataview;\n");
        switch (i->direction) {
            case RI: fprintf(adb, "with %s_QGenAda_wrapper;\nuse %s_QGenAda_wrapper;\n\n",
                i->distant_name, i->distant_name); break;
            case PI: break;
            default: break;
        }
    })}

    /* 
     * Include the calling threads wrappers to the wrappers of the passive functions, to have access to their "vm_" functions
     * (only if the calling thread in question has some async RI). This is also extended to threads that contain at least 
     * one unpro function (see the C wrapper generation for more details).
     */
     if (thread_runtime == fv->runtime_nature) {
         FOREACH (i, Interface, fv->interfaces, {
             if (PI == i->direction && unprotected == i->rcm) mix = 1;
         })
     }
    if (passive_runtime == fv->runtime_nature || mix) {
        FOREACH(ct, FV, fv->calling_threads, {
                Add_With_AsyncRI(ct, &ads);
        });
    }

    fprintf(ads, "\npackage %s_wrappers is\n\n", fv->name);
    // To debug backdoor-related misuse of the passive functions stack:
     fprintf(adb, "with PolyORB_HI.Output;\n\n");

    fprintf(adb, "package body %s_wrappers is\n\n", fv->name);

    //char *path = NULL;
    //path = make_string ("%s/%s", OUTPUT_PATH, fv->name);
    
    //if (!file_exists(path, "_hook")) {
    if (!fv->artificial) {
        fprintf(ads, "\tprocedure C_Init_%s;\n\n", fv->name);
        fprintf(adb, "\tprocedure C_Init_%s is\n", fv->name);
        fprintf(adb, "\t\tprocedure Init_%s;\n", fv->name);
        fprintf(adb, "\t\tpragma Import (C, Init_%s, \"init_%s\");\n\n",
            fv->name, fv->name);
        fprintf(adb, "\tbegin\n\t\tInit_%s;\n\tend C_Init_%s;\n\n",
            fv->name, fv->name);
    }

    /* Include call to QGenAda init code if it is present */
    FOREACH(i, Interface, fv->interfaces, {
    if (qgenada == i->distant_qgen->language
        && RI == i->direction && NULL != i->distant_qgen->qgen_init) {
        fprintf(ads, "\tprocedure QGen_Init_%s;\n\n", i->distant_name);
        fprintf(ads, "\tpragma Export (C, QGen_Init_%s, \"vm_QGen_Init_%s\");\n\n", i->distant_name, i->distant_name);
        fprintf(adb, "\tprocedure QGen_Init_%s is\n", i->distant_name);
        fprintf(adb, "\tbegin\n\t\t%s.init;\n\tend QGen_Init_%s;\n\n",
            i->distant_name, i->distant_name);
    })}

    if (NULL != async_ads && NULL != async_adb) {

        fprintf(async_ads,
                "--  This file was generated automatically: DO NOT MODIFY IT !\n\n");
        fprintf(async_ads,
                "pragma Style_Checks (Off);\npragma Warnings (Off);\n\n");
        fprintf(async_ads, "with Interfaces.C;\n");

        if (hasparam)
            fprintf(async_ads, "with PolyORB_HI_Generated.Types;\n");

        fprintf(async_ads, "with PolyORB_HI_Generated.Deployment;\n");
        fprintf(async_ads, "use PolyORB_HI_Generated.Deployment;\n\n");

        fprintf(async_ads, "\npackage %s_async_ri_wrappers is\n\n",
                fv->name);

        fprintf(async_adb,
                "--  This file was generated automatically: DO NOT MODIFY IT !\n\n");
        fprintf(async_adb,
                "pragma Style_Checks (Off);\npragma Warnings (Off);\n\nwith PolyORB_HI_Generated.Activity;\nuse PolyORB_HI_Generated.Activity;\n");
        fprintf(async_adb, "with PolyORB_HI.Errors;\n\n");
        fprintf(async_adb, "package body %s_async_ri_wrappers is\n\n",
                fv->name);

    }
}

/* Creates fv_wrappers.ads, fv_wrappers.adb, and potentially RI async wrappers  */
int Init_Ada_Wrappers_Backend(FV * fv)
{
    char *filename = NULL;      // to store the wrappers filename (fv_name_wrappers.ad*)
    size_t len = 0;             // length of the filename
    char *path = NULL;

    contains_sync_interface = 0;

    if (NULL != fv->system_ast->context->output)
        build_string(&path, fv->system_ast->context->output,
                     strlen(fv->system_ast->context->output));
    build_string(&path, fv->name, strlen(fv->name));

    // Elaborate the Wrapper file name:
    len = strlen(fv->name) + strlen("_wrappers.ads");
    filename = (char *) malloc(len + 1);

    // create .ads file
    sprintf(filename, "%s_wrappers.ads", fv->name);
    create_file(path, filename, &ads);

    // create .adb file
    sprintf(filename, "%s_wrappers.adb", fv->name);
    create_file(path, filename, &adb);

    free(filename);

    if (thread_runtime == fv->runtime_nature) {

        bool async_ri = 0;
        FOREACH(i, Interface, fv->interfaces, {
            if (RI == i->direction && asynch == i->synchronism) async_ri = true;
        })

        if (async_ri) {

            len = strlen(fv->name) + strlen("_async_ri_wrappers.ads");
            filename = (char *) malloc(len + 1);

            // create .ads file
            sprintf(filename, "%s_async_ri_wrappers.ads", fv->name);
            create_file(path, filename, &async_ads);

            // create .adb file
            sprintf(filename, "%s_async_ri_wrappers.adb", fv->name);
            create_file(path, filename, &async_adb);

            free(filename);

            assert(async_ads != NULL && async_adb != NULL);
        }
    }


    free(path);
    if (NULL == ads || NULL == adb)
        return -1;

    ada_wrappers_preamble(fv);

    return 0;
}


void close_ada_wrappers()
{
    close_file(&ads);
    close_file(&adb);
    close_file(&async_ads);
    close_file(&async_adb);
}

void add_PI_to_ada_wrappers(Interface * i)
{
    if (NULL == ads || NULL == adb)
        return;

    // a. Case of Sync PI: don't do anything but recording that a sync interface exists for this function
    //    this information will be used to generate the package elaboration part once all other PI have 
    //    been processed.
    if (synch == i->synchronism) {
        contains_sync_interface = 1;
        return;
    }


    // b. wrappers.ads : declare the function
    fprintf(ads,
            "\t------------------------------------------------------\n");
    fprintf(ads, "\t--  Provided Interface \"%s\"\n", i->name);
    fprintf(ads,
            "\t------------------------------------------------------\n");
    fprintf(ads, "\tprocedure %s (Entity : PolyORB_HI_Generated.Deployment.Entity_Type", i->name);
    if (NULL != i->in) {
        fprintf(ads,";\n\t\t ");
         fprintf(ads,
            "%sBuffer:PolyORB_HI_Generated.Types.%s_Buffer_Impl",
            i->in->value->name,
            i->in->value->type);
    } 
    fprintf(ads,");\n\n");
    

    // c. wrappers.adb : define the function
    fprintf(adb,
            "\t------------------------------------------------------\n");
    fprintf(adb, "\t--  Asynchronous Provided Interface \"%s\"\n",
            i->name);
    fprintf(adb,
            "\t------------------------------------------------------\n");
    fprintf(adb, "\tprocedure %s (Entity : PolyORB_HI_Generated.Deployment.Entity_Type", i->name);
    if (NULL != i->in) {
        fprintf(adb,";\n\t\t ");
        fprintf(adb,
            "%sBuffer:PolyORB_HI_Generated.Types.%s_Buffer_Impl",
            i->in->value->name,
            i->in->value->type);
    }
    fprintf(adb, ")\n\t");


    fprintf(adb, "is\n");
  // Make an Ada buffer for each input:
    if (NULL != i->in) {
        fprintf(adb,
            "\t\t%s_AdaBuffer: Interfaces.C.Char_Array(1..Interfaces.C.size_t (%sBuffer.Length));\n",
                i->in->value->name, i->in->value->name);
        fprintf(adb, "\t\tpragma Import (Ada, %s_AdaBuffer);\n",
                i->in->value->name);
        fprintf(adb,
            "\t\tfor %s_AdaBuffer'Address use %sBuffer.Buffer'Address;\n\n",
                i->in->value->name, i->in->value->name);
   } 
 
    /* Update 26-10-2010 :  if we are in a thread created by the VT 
     * we must call directly the sync RI and NOT the function from vm_if.c
     */
    //char *path = NULL;
    //path = make_string ("%s/%s", OUTPUT_PATH, i->parent_fv->name);
    if (i->parent_fv->artificial) {
    //if (file_exists(path, "_hook")) {
        char *distant_fv = NULL;
            RCM rcm = protected;
        FOREACH (interface, Interface, i->parent_fv->interfaces, {
            if (RI == interface->direction
                    && !strcmp (interface->distant_name, i->distant_name) &&
                    synch == interface->synchronism) { 
                distant_fv = interface->distant_fv;
                rcm = interface->rcm;
            }
        });

        /* Call sync (protected) function: */   
        fprintf(adb, "\tbegin\n");
        fprintf(adb, "\t\t%s_wrappers%s%s.%s(%d%s",
                    distant_fv, 
            protected == rcm ? ".protected_" : "",
            protected == rcm ? distant_fv : "",
            i->distant_name,
            i->parent_fv->thread_id,
            NULL != i->in ? ", ": "");
        if (NULL != i->in) {
            fprintf(adb, "%s_AdaBuffer, %s_AdaBuffer'Length",
                i->in->value->name, 
                i->in->value->name);
            // ABOVE LINE TO BE CHECKED...
        }
        fprintf(adb, ");\n");
    }

    else {  /* User-defined function: call function in vm_if.c */
         fprintf(adb, "\t\tprocedure C_%s_%s", i->parent_fv->name, i->name);
         if (NULL != i->in) {
                fprintf(adb, "\n\t\t\t(");
                fprintf(adb,
                    "C_%sBuffer:Interfaces.C.char_array;\n\t\t\tC_%sMaxSize:Integer",
                    i->in->value->name,
                    i->in->value->name);
                fprintf(adb, ")");
         }
         fprintf(adb, ";\n");
        fprintf(adb, "\t\tpragma Import (C, C_%s_%s, \"%s_%s\");\n\n",
            i->parent_fv->name, i->name, i->parent_fv->name, i->name);

        // Body of the procedure: make the call to the C function
        fprintf(adb, "\tbegin\n\t\tC_%s_%s", i->parent_fv->name, i->name);
        if (NULL != i->in) {
                fprintf(adb, " (");
                fprintf(adb, "%s_AdaBuffer, %s_AdaBuffer'Length",
                        i->in->value->name, 
                        i->in->value->name);
                fprintf(adb, ")");
        }
        fprintf(adb, ";\n");
    }
    fprintf(adb, "\tend %s;\n\n", i->name);

}

// Add a RI
void add_RI_to_ada_wrappers(Interface * i)
{
    Parameter_list *tmp;
    int count = 0;
    FV_list *calltmp = NULL;

    FILE *s = ads, *b = adb;

    //Print_Interface (i);

    if (thread_runtime == i->parent_fv->runtime_nature
        && asynch == i->synchronism) {
        s = async_ads;
        b = async_adb;
    }

    if (NULL == s || NULL == b)
        return;

    // b. wrappers.ads : declare the function
    fprintf(s,
            "\t------------------------------------------------------\n");
    fprintf(s, "\t--  %s Required Interface \"%s\"\n",
            asynch == i->synchronism ? "Asynchronous" : "Synchronous",
            i->name);
    fprintf(s,
            "\t------------------------------------------------------\n");
    fprintf(s, "\tprocedure vm_%s", i->name);

    if (NULL != i->in || NULL != i->out)
        fprintf(s, "(");

    if (qgenada == i->distant_qgen->language) {
        FOREACH(p, Parameter, i->in, {
            List_QGen_Param_Types_And_Names(p, &s);
        })
        FOREACH(p, Parameter, i->out, {
            List_QGen_Param_Types_And_Names(p, &s);
        })
    } else {
        FOREACH(p, Parameter, i->in, {
            List_Ada_Param_Types_And_Names(p, &s);
        })
        FOREACH(p, Parameter, i->out, {
            List_Ada_Param_Types_And_Names(p, &s);
        })
    }

    if (NULL != i->in || NULL != i->out)
        fprintf(s, ")");
    fprintf(s, ";\n");

    fprintf(s, "\tpragma Export (C, vm_%s, \"vm_%s%s_%s\");\n\n",
           i->name, asynch == i->synchronism?"async_":"",i->parent_fv->name, i->name);

    // c. wrappers.b : definition of the function
    fprintf(b,
            "\t------------------------------------------------------\n");
    fprintf(b, "\t--  %s Required Interface \"%s\"\n",
            asynch == i->synchronism ? "Asynchronous" : "Synchronous",
            i->name);
    fprintf(b,
            "\t------------------------------------------------------\n");
    fprintf(b, "\tprocedure vm_%s", i->name);

    if (NULL != i->in || NULL != i->out)
        fprintf(b, "(");

    if (qgenada == i->distant_qgen->language) {
        FOREACH(p, Parameter, i->in, {
            List_QGen_Param_Types_And_Names(p, &b);
        })
        FOREACH(p, Parameter, i->out, {
            List_QGen_Param_Types_And_Names(p, &b);
        })
    } else {
        FOREACH(p, Parameter, i->in, {
            List_Ada_Param_Types_And_Names(p, &b);
        })
        FOREACH(p, Parameter, i->out, {
            List_Ada_Param_Types_And_Names(p, &b);
        })
    }

    if (NULL != i->in || NULL != i->out)
        fprintf(b, ")");

    fprintf(b, " is\n");

    if (qgenada == i->distant_qgen->language) {
        char *qgen_args = NULL;

        fprintf(b, "\tbegin\n");

        fprintf(b, "\t\tExecute_%s_QGenAda(", i->distant_name);

        /* Add IN and OUT parameters */
            FOREACH(p, Parameter, i->in, {
                Create_QGenAda_Argument(p->name, &qgen_args, false);
            })
            FOREACH(p, Parameter, i->out, {
                Create_QGenAda_Argument(p->name, &qgen_args, false);
            })
        if (qgen_args != NULL)
            fprintf(b, "%s", qgen_args);
        fprintf(b, ");\n\n");

    } else {
        if (asynch == i->synchronism) {
        /*
         * Distant FV is a thread (asynchronous RI). This means that depending on the nature of the current FV (thread or passive)
         * we have either to call the VM (if we are in a thread) or to switch-case the caller thread to call his own access to the VM
         * We retrieve the caller thread ID using the "stack" 
         */
        if (thread_runtime == i->parent_fv->runtime_nature) {

            fprintf(b, "\t\tValue: %s%s%s_%s_others_Interface \n\t\t(%s%s%s_%s_others_Port_Type'(OUTPORT_%s));\n", (get_context()->aadlv2) ? i->parent_fv->name : "",   // AADL v2 only
                (get_context()->aadlv2) ? "_CV_Thread_" : "",   // AADL v2 only
                i->parent_fv->name,     // fv->name should be replaced by the namespace (not supported 15/07/2009) (used to be aplc)
                i->parent_fv->name, (get_context()->aadlv2) ? i->parent_fv->name : "",  // AADL v2 only
                (get_context()->aadlv2) ? "_CV_Thread_" : "",   // AADL v2 only
                i->parent_fv->name,     // fv->name should be replaced by the namespace (not supported 15/07/2009) (used to be aplc)
                i->parent_fv->name, i->name);
            fprintf(b,
                "\t\tErr: PolyORB_HI.Errors.Error_Kind;\n\t\tuse type PolyORB_HI.Errors.Error_Kind;\n");
            fprintf(b, "\tbegin\n");
            /* Copy the buffer(s) from C to Ada */
            if (NULL != i->in) {
            tmp = i->in;
            while (NULL != tmp) {
                fprintf(b, "\t\tfor J in 1 .. IN_%s_size loop\n",
                    tmp->value->name);
                fprintf(b,
                    "\t\t\tValue.OUTPORT_%s_DATA.Buffer (J) := PolyORB_HI_Generated.Types.Stream_Element%s\n",
                    i->name,
                    get_context()->aadlv2?"_Buffer":"");
                fprintf(b,
                    "\t\t\t\t(IN_%s (Interfaces.C.size_t (J - 1)));\n",
                    tmp->value->name);
                fprintf(b, "\t\tend loop;\n");
                if (!(get_context()->aadlv2))
                fprintf(b, "\t\tValue.OUTPORT_%s_DATA.Length := PolyORB_HI_Generated.Types.uint16 (IN_%s_size);\n",     // old V1
                    i->name, tmp->value->name);
                else
                fprintf(b, "\t\tValue.OUTPORT_%s_DATA.Length := PolyORB_HI_Generated.Types.Unsigned_16 (IN_%s_size);\n",        // AADL V2, should work with V1 but not supported by Ocarina yet also, otherwise put back previous line for V1
                    i->name, tmp->value->name);
                tmp = tmp->next;
            }
            }


            if (NULL != i->parent_fv->process) {
            fprintf(b, "\t\tPut_Value (%s_%s_K, Value);\n",
                i->parent_fv->process->identifier,
                i->parent_fv->name);

            /* Following is added only for the Backdoor backend : call the Send_Output to
               flush immediately the output buffer */
            fprintf(b, "\t\tErr:=Send_Output(%s_%s_K, %s%s%s_%s_others_Port_Type'(OUTPORT_%s));\n", i->parent_fv->process->identifier, i->parent_fv->name, (get_context()->aadlv2) ? i->parent_fv->name : "",   // AADL v2 only
                (get_context()->aadlv2) ? "_CV_Thread_" : "",   // AADL v2 only
                i->parent_fv->name,     // fv->name should be replaced by the namespace (not supported 15/07/2009) (used to be aplc)
                i->parent_fv->name, i->name);
            }
        }
        else {                // Current FV = Passive function
            fprintf(b, "\tbegin\n");
            fprintf(b,
                "\t\t -- This function is passive, thus not have direct access to the VM\n");
            fprintf(b,
                "\t\t -- It must use its calling thread to invoke this asynchronous RI\n\n");

           /* ForEachWithParam element of i->parent_fv->calling_threads,
            execute CountFV and update & count thanks;*/

            /* Count the number of calling threads */
            if (NULL == i->calling_pis) calltmp = i->parent_fv->calling_threads;
            else {
                FOREACH(calling_pi, Interface, i->calling_pis, {
                    FOREACH(thread_caller, FV, calling_pi->calling_threads, {
                        ADD_TO_SET(FV, calltmp, thread_caller);
                    });
                });
            }
            FOREACH(ct, FV, calltmp, {
                (void) ct;
                count ++;
            });

            if (1 == count) {
                fprintf(b, "\t\t%s_async_ri_wrappers.vm_%s_vt",
                    calltmp->value->name,
                    i->name);

                if (NULL != i->in) {        // Add parameters
                    fprintf(b, "(");
                    FOREACH(p, Parameter, i->in, {
                        List_Ada_Param_Names(p, &b);
                    });
                    fprintf(b, ")");
                }
                fprintf(b, ";\n");
            }
            else if (count > 1) {
                fprintf(b, "\t\tcase callinglist.get_top_value is\n");
                FOREACH(caller, FV, calltmp, {
                    fprintf(b,
                        "\t\t\twhen %d => %s_async_ri_wrappers.vm_%s_vt",
                        caller->thread_id,
                        caller->name, i->name);

                    if (NULL != i->in) {    // Add parameters
                        fprintf(b, "(");
                        FOREACH(p, Parameter, i->in, {
                            List_Ada_Param_Names(p, &b);
                        });
                        fprintf(b, ")");
                    }
                    fprintf(b, ";\n");
                });
                fprintf(b, "\t\t\twhen others => null;\n");
                fprintf(b, "\t\tend case;\n");
            }
        }
    }
    else {
            int cnt = 0;
            FOREACH(t, FV, i->parent_fv->calling_threads, {
            (void) t;
                cnt++;
            })

            fprintf(b, "\tbegin\n");

            if (protected == i->rcm) {
                fprintf(b, "\t\t%s_wrappers.protected_%s.%s(",
                    i->distant_fv, i->distant_fv, NULL != i->distant_name? i->distant_name: i->name);
            } else if (unprotected == i->rcm) {
                fprintf(b, "\t\t%s_wrappers.%s(", i->distant_fv, NULL != i->distant_name? i->distant_name:i->name);
            }

            if (passive_runtime == i->parent_fv->runtime_nature) {
                if (cnt > 1) {
                fprintf(b, "callinglist.get_top_value");
                }
                else if (1 == cnt) {
                fprintf(b, "%d", i->parent_fv->calling_threads->value->thread_id);
                }
                else if (0 == cnt) {
                ERROR ("** Error: function \"%s\" is not called by anyone (dead code)!\n", i->parent_fv->name);
                        ERROR ("** This is not supported by the Ada runtime. You may have to change two things:\n");
                        ERROR ("**    1) (In any case) use the -p flag when calling the TASTE orchestrator, and\n");
                        ERROR ("**    2) In your deployment view, if applicable, choose a non-Ada runtime\n");
                        ERROR ("**       (do not use \"LEON_ORK\" ; \"Native\" or \"LEON_RTEMS\" are OK)\n\n");
                        exit(-1);
                }
            }
            else
                fprintf(b, "%d", i->parent_fv->thread_id);

            if (NULL != i->in || NULL != i->out)
                fprintf(b, ",");

            /* Add IN and OUT parameters */
            FOREACH(p, Parameter, i->in, {
                List_Ada_Param_Names(p, &b);
            })
            FOREACH(p, Parameter, i->out, {
                List_Ada_Param_Names(p, &b);
            })

            fprintf(b, ");\n");
        }
    }

    fprintf(b, "\tend vm_%s;\n\n", i->name);
}


void End_Ada_Wrappers_Backend(FV * fv)
{
    /* If necessary, add a protected object to the wrapper files */
                                           
    Add_Protected_Interfaces(fv, ads, adb);
    Add_Unprotected_Interfaces(fv, ads, adb);

    /* Postamble to wrappers.ads/adb */
    if (contains_sync_interface && NULL != adb) {
        fprintf(adb, "begin\n\tC_Init_%s;\n\n", fv->name);
    }

    if (NULL != ads)
        fprintf(ads, "end %s_wrappers;\n", fv->name);
    if (NULL != adb)
        fprintf(adb, "end %s_wrappers;\n", fv->name);

    if (NULL != async_ads)
        fprintf(async_ads, "end %s_async_ri_wrappers;\n", fv->name);
    if (NULL != async_adb)
        fprintf(async_adb, "end %s_async_ri_wrappers;\n", fv->name);

    close_ada_wrappers();
}

/* 
 * Generate the code of a stack that handle the calling thread list needed to transparently make sure that
 * when a sync PI has to call a RI, it does it through its calling thread.
 */
void Generate_Ada_CallingStack(FV * fv)
{
    int count = 0;
    if (NULL == ads || NULL == adb)
        return;

    FOREACH(ct, FV, fv->calling_threads, {
        (void) ct;
        count ++;
    })

    if (2 > count)
        return;

    fprintf(ads, "\ttype t_stack is array (1..%d) of integer;\n\n", count);
    fprintf(ads, "\tprotected callinglist is\n");
    fprintf(ads, "\t\tprocedure push (value: integer);\n");
    fprintf(ads, "\t\tprocedure pop;\n");
    fprintf(ads, "\t\tfunction get_top_value return integer;\n");
    fprintf(ads, "\t\tprivate\n\t\t\tstack: t_stack := (others=>0);\n");
    fprintf(ads, "\tend callinglist;\n\n");

    fprintf(adb, "\tprotected body callinglist is\n");
    fprintf(adb, "\t\tprocedure push(value: integer) is\n\t\tbegin\n");
    fprintf(adb, "\t\t\tif stack(1) /= 0 then\n\tPolyORB_HI.Output.Put_Line(\"### STACK ERROR (OVERFLOW), in %s\"); end if;\n", fv->name);      // Added for debug
    fprintf(adb, "\t\t\tfor i in 1..%d loop\n", count);
    fprintf(adb,
            "\t\t\t\tif stack(i) /= 0 then if stack (i-1) = 0 then stack (i-1) := value; return; end if; end if;\n");
    fprintf(adb, "\t\t\tend loop;\n");
    fprintf(adb, "\t\t\tstack(%d) := value;\n", count);
    fprintf(adb, "\t\tend push;\n\n");

    fprintf(adb, "\t\tprocedure pop is\n\t\tbegin\n");
    fprintf(adb, "\t\t\tif stack(1) /= 0 then stack(1) := 0; return;\n");
    fprintf(adb,
            "\t\t\telsif stack(%d) = 0 then\n\tPolyORB_HI.Output.Put_Line(\"### STACK ERROR (POP EMPTY STACK) in %s\"); return;\n",
            count, fv->name);
    fprintf(adb, "\t\t\telse\n");
    fprintf(adb, "\t\t\t\tfor i in 1..%d loop\n", count);
    fprintf(adb,
            "\t\t\t\t\tif stack(i) /= 0 then if stack (i-1) = 0 then stack (i) := 0; return; end if; end if;\n");
    fprintf(adb, "\t\t\t\tend loop;\n");
    fprintf(adb, "\t\t\tend if;\n");
    fprintf(adb, "\t\tend pop;\n\n");

    fprintf(adb,
            "\t\tfunction get_top_value return integer is\n\t\tbegin\n");
    fprintf(adb, "\t\t\tif stack(1) /= 0 then return stack(1);\n");
    fprintf(adb,
            "\t\t\telsif stack(%d) = 0 then\n\tPolyORB_HI.Output.Put_Line(\"### STACK ERROR (GET_TOP EMPTY STACK) in %s. Check that your startup/init function does not call any RIs.\"); return 0;\n",
            count, fv->name);
    fprintf(adb, "\t\t\telse\n");
    fprintf(adb, "\t\t\t\tfor i in 1..%d loop\n", count);
    fprintf(adb,
            "\t\t\t\t\tif stack(i) /= 0 then if stack (i-1) = 0 then return stack (i); end if; end if;\n");
    fprintf(adb, "\t\t\t\tend loop;\n");
    fprintf(adb, "\t\t\tend if;\n");
    fprintf(adb, "\t\t\treturn(-1);\n");
    fprintf(adb, "\t\tend get_top_value;\n\n");

    fprintf(adb, "\tend callinglist;\n\n");
}

/* Function to process one interface of the FV */
void GLUE_Ada_Wrappers_Interface(Interface * i)
{

    if (PI == i->direction) {
        add_PI_to_ada_wrappers(i);
    } else if (RI == i->direction) {
        add_RI_to_ada_wrappers(i);
    }
}

// External interface for the Ada wrappers backend
void GLUE_Ada_Wrappers_Backend(FV * fv)
{
    if (fv->system_ast->context->onlycv)
        return;

    if (fv->system_ast->context->polyorb_hi_c)
        return;

    FOREACH(i, Interface, fv->interfaces, {
        if (PI == i->direction && (qgenc == fv->language)) {
            return;
        }
    })

        Init_Ada_Wrappers_Backend(fv);

        if (passive_runtime == fv->runtime_nature) {
            Generate_Ada_CallingStack(fv);
        }

        /*ForEach(fv->interfaces, GLUE_Ada_Wrappers_Interface);*/
        FOREACH(i, Interface, fv->interfaces, {
        if (!(qgenc == fv->language ||
            qgenada == fv->language ||
            qgenc == i->distant_qgen->language)) {
            GLUE_Ada_Wrappers_Interface(i);
        }
        })

        End_Ada_Wrappers_Backend(fv);

}
