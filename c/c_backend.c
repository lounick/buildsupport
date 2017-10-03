/* Buildsupport is (c) 2008-2016 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/*
 * C backend for the AADL parser: C Entry point for buildsupport
 *
 *
 * The function C_End() is called by the AADL Parser (buildsupport.adb).
 * It triggers the execution of all the backends:
 *   - Model transformations
 *   - Semantic checks
 *   - Generation of function skeletons
 *   - Generation of glue code
 *   - Generation of the Concurrency view
 *
 * The data structures for the C AST are defined in the file my_types.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sys/stat.h>

#include "my_types.h"
#include "practical_functions.h"
#include "backends.h"
#include "c_ast_construction.h"

/* 
 * Fatal error counter
*/
static int error_count = 0;
void add_error()
{
    error_count++;
}

/*:
 * Main function called after the Ada AADL parser has completed.
*/
void C_End()
{
    /* Perform semantic checks of the user input */
    Semantic_Checks();

    /*
     * OpenGEODE support:
     * Replace SDL language with Ada for building glue code
     */
    if (get_context()->glue) {
        FOREACH(fv, FV, get_system_ast()->functions, {
                if (sdl == fv->language) fv->language = ada;
        });
    }

    /* Mark the callers of QGen functions */
    FOREACH(fv, FV, get_system_ast()->functions, {
        FOREACH(i, Interface, fv->interfaces, {
            if (RI == i->direction) {
                FV         *connected_fv = NULL;
                Interface  *connected_pi = NULL;
                connected_fv = FindFV (i->distant_fv);
                if ((connected_fv != NULL) &&
                         ((qgenada == connected_fv->language) || (qgenc == connected_fv->language))) {
                    i->distant_qgen->language = connected_fv->language;
                    i->distant_qgen->fv_name = connected_fv->name;
                    connected_pi = FindInterface (connected_fv, i->distant_name);
                    if (connected_pi != NULL) {
                        connected_pi->distant_qgen->language = connected_fv->language;
                        connected_pi->distant_qgen->fv_name = fv->name;
                    }
                }
            }
        });
    });

    /* Find out if init functions are present in QGen code and save the info */
    FOREACH(fv, FV, get_system_ast()->functions, {
        FOREACH(i, Interface, fv->interfaces, {
            if (i->distant_qgen != NULL) {
                char *qgeninit = NULL;
                if (qgenc == fv->language)
                    Build_QGen_Init(&qgeninit, fv->name, i->name, qgenc);
                else if (qgenada == fv->language)
                    Build_QGen_Init(&qgeninit, fv->name, i->name, qgenada);
                else if (qgenada == i->distant_qgen->language)
                    Build_QGen_Init(&qgeninit, fv->name, i->distant_name, qgenada);
                else if (qgenc == i->distant_qgen->language)
                    Build_QGen_Init(&qgeninit, fv->name, i->distant_name, qgenc);

                if (qgeninit != NULL)
                    i->distant_qgen->qgen_init = qgeninit;
            }
        });
    });

    /*
     * If Semantic errors have been found (errors in the user AADL models),
     * then the application is exited with an error message.
     */
    if (error_count > 0) {
        fprintf(stderr, "\nFound %d errors.. Aborting...\n", error_count);
        exit(1);
    }

    else {
        /* Generation of the build script */
        Generate_Build_Script();

        /* Skeleton-generation, if -gw flag is set. Done before the preprocesing backend
           which possibly creates additional functions */
        FOREACH(fv, FV, get_system_ast()->functions, {
            if (get_context()->gw && NULL != fv->zipfile && !get_context()->glue) {
                printf ("[INFO] No skeleton is generated for function \"%s\"\n"
                        "       because source code is provided in file \"%s\"\n"
                        "       (as specified in the interface view)\n\n",
                        fv->name,
                        fv->zipfile
                        );
            }

            if (get_context()->gw &&
                (NULL == fv->zipfile || get_context()->glue)) {
                    GW_SDL_Backend(fv);
                    GW_Simulink_Backend(fv);
                    GW_C_Backend(fv);
                    GW_VDM_Backend(fv);
                    GW_VHDL_Backend(fv);
                    GW_Ada_Backend(fv);
                    GW_SCADE_Backend(fv);
                    GW_RTDS_Backend(fv);
                    GW_Driver_Backend(fv);
                    GW_MicroPython_Backend(fv);
            }

            /* Export to SMP2: generate glue code and Python AST */
            if (true == get_context()->smp2 && (false == fv->is_component_type)) {
                GLUE_OG_Backend(fv);
                GLUE_RTDS_Backend(fv);
                GLUE_MiniCV_Backend(fv);
                GLUE_C_Backend(fv);
                Generate_Python_AST(get_system_ast(), NULL);

            }
        })


        /*
         * Perform the first part of the Vertical transformation (-glue flag):
         * Various model tranformations of the interface view
         */
        if (get_context()->glue) {
            Preprocessing_Backend(get_system_ast());
        }
        /*
         * Preprocessing may have raised some further semantic errors.
         */
        if (error_count > 0) {
            fprintf(stderr, "\nFound %d errors.. Aborting...\n", error_count);
            exit(1);
        }

        /*
         * Debug mode (userflag -test) : dump the result of the transformation
         * TODO: generate an AADL model (equivalent to the Concurrency View)
         */
        if (get_system_ast()->context->test) {
            Dump_model(get_system_ast());
        }

        /*
         * Execute various backends applicable to each FV
         */
        FOREACH(fv, FV, get_system_ast()->functions, {
            if (false == fv->is_component_type) {
                /*
                 * Call 'asn2dataModel' if glue code is not required
                 * (to avoid slowing down the orchestrator, which is
                 * the only place where the glue code is requested
                 */
                if (get_context()->gw && !get_context()->glue && NULL == fv->zipfile) {
                    Call_asn2dataModel(fv);
                }

                /* Process all functional states declared in the interface view */
                if (get_context()->gw && (NULL == fv->zipfile || get_context()->glue)) {
                    Process_Context_Parameters(fv);
                }

                /* Process function directives */
                if (get_context()->glue || get_context()->test) {
                    Process_Directives (fv);
                }

                if (get_context()->glue) {
                    GLUE_OG_Backend(fv);
                    GLUE_RTDS_Backend(fv);
                    GLUE_MiniCV_Backend(fv);
                    GLUE_C_Backend(fv);
                    GLUE_GUI_Backend(fv);
                    GLUE_Ada_Wrappers_Backend(fv);
                    GLUE_C_Wrappers_Backend(fv); 
                    GLUE_VT_Backend(fv);
                    GLUE_MicroPython_Backend(fv);
                }
            }
        })

        /*
         * Perform the second part of the Vertical transformation:
         * Generate driver configuration
         * Generate the full concurrency view (process.aadl et al.)
         * Additionnally create an AADL file of the concurrency view
         * for display only in TASTE-IV
         */
            if (get_context()->glue) {
                FOREACH (process, Process, get_system_ast()->processes, {
                    Process_Driver_Configuration (process);
                })

                Generate_Full_ConcurrencyView((get_system_ast()->processes),
                                             (get_system_ast()->name));
                AADL_CV_Unparser ();
            }

        /* Generation of system configuration used by C_ASN1_Types.h */
        System_Config(get_system_ast());

    }
}
