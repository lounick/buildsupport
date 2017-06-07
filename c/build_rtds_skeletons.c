/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/*
 * Skeleton generator for Pragmadev RTDS subsystems 
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   
#include <assert.h>
#include <unistd.h>

#include "my_types.h"
#include "practical_functions.h"
#include "c_ast_construction.h"
/* 
 *  RTDS project files are huge XML files.
 *  Rather than generating them from scratch, this backend
 *  creates a standard project from an existing template 
 *  (stored in header files), and makes a call to the 'sed'
 *  command to replace strings with the actual function names,
 *  signal list and procedure lists.
 *  Note: because we use the sed command, this cannot work on
 *  Windows (unless cygwin is installed)
*/

#include "rtds_project_template.h"
#include "rtds_process_template.h"
#include "rtds_scheduled.h"

/* Create directories and static files */
void Create_New_RTDS_Project (FV *fv)
{
  char *path            = NULL,
       *filename        = NULL,
       *codegenpath     = NULL;

  FILE *project         = NULL, 
       *process         = NULL,
       *scheduled       = NULL,
       *compile_script  = NULL;
   
   /*  
    *  Build the file names with the path obtained from the context 
    *  (command line -o flag) 
   */
  path      = make_string ("%s/%s", OUTPUT_PATH, fv->name);
  filename  = make_string ("%s_project.rdp", fv->name);

  create_file(path, filename, &project);

  sprintf (filename, "%s_process.rdd", fv->name);
  create_file (path, filename, &process);

  free (filename);
  filename = NULL;

  filename = make_string ("scheduled.rdd");
  create_file(path, filename, &scheduled);

  free (filename);
  filename = NULL;  

  /* Create the directory for code generation */
  codegenpath = make_string ("%s/%s", path, fv->name);
  mkdir (codegenpath, 0700);
  free  (codegenpath);
  
  filename  = make_string ("rtds_GenerateCodeForTASTE.sh", fv->name);
  create_file (path, filename, &compile_script);
  
  free (filename);
  filename = NULL;

  /* We do not tolerate any file creation error */
  assert    (NULL != project    && 
            NULL != process     && 
            NULL != scheduled   &&
            NULL != compile_script
            );

  /* Dump the project templates from the rtds header files */
  fprintf (project,   "%s", rtds_project_template);
  fprintf (process,   "%s", rtds_process_template);
  fprintf (scheduled, "%s", rtds_scheduled);

  /*
   * Create a compile script to automate the process for the user
   * This script creates the files required in the "profile" 
   * and "profile/bricks" directories then  calls RTDS code generator,
   * then zip the result so that it is ready for the TASTE builder.
   */
  fprintf (compile_script, "#!/bin/bash\n\n");
  fprintf (compile_script, "curdir=$(pwd)\n");
  fprintf (compile_script, "cd $(dirname \"$0\")\n\n");  
  fprintf (compile_script, "echo Preparing RTDS profile\n");
  fprintf (compile_script, "mkdir -p profile && mkdir -p profile/bricks\n\n");
  fprintf (compile_script, "echo '[general]\nrtos=-\nsocketAvailable=1\nscheduling=required\nmalloc=forbidden\n\n[common]\nincludes=.;${RTDS_TEMPLATES_DIR}\n\n[debug]\ndebug=1\ndefines=RTDS_SIMULATOR\n\n[tracer]\ndefines=RTDS_MSC_TRACER\n'>profile/DefaultOptions.ini\n\n");
  fprintf (compile_script, "echo '#define RTDS_SETUP_CURRENT_CONTEXT\n#define RTDS_FORGET_INSTANCE_INFO\n' >profile/RTDS_ADDL_MACRO.h\n\n");
  fprintf (compile_script, "echo 'typedef int RTDS_RtosQueueId;\ntypedef int RTDS_RtosTaskId;\ntypedef int RTDS_TimerState;\n\n#define RTDS_GLOBAL_PROCESS_INFO_ADDITIONNAL_FIELDS\n#define RTDS_MESSAGE_HEADER_ADDITIONNAL_FIELDS\n' >profile/RTDS_BasicTypes.h\n\n");
  fprintf (compile_script, "echo '#define RTDS_MSG_INPUT_ERROR 1\n#define RTDS_TIMER_CLEAN_UP\n#define RTDS_START_SYNCHRO_WAIT\n' >profile/RTDS_MACRO.h\n\n");
  fprintf (compile_script, "echo '#include \"RTDS_MACRO.h\"\n#include \"common.h\"\n' >profile/bricks/RTDS_Include.c\n\n");

  fprintf (compile_script, "echo Invoking RTDS code generator\n");
  fprintf (compile_script, 
        "rtdsGenerateCode -f %s_project.rdp scheduled partial-linux || exit -1\n\n", 
        fv->name); 

  fprintf (compile_script, "echo Generating zipfile for TASTE\n");
  fprintf (compile_script, 
        "mv %s/RTDS-RTOS-adaptation/* %s/ && zip %s %s/*\n",
        fv->name,
        fv->name,
        fv->name,
        fv->name);
  fprintf (compile_script, 
        "echo 'All done.\nCopy the %s.zip file into your TASTE working directory'\n",
        fv->name);
  fprintf (compile_script, "cd \"$curdir\"\n");

  fclose (project);
  fclose (process);
  fclose (scheduled);
  fclose (compile_script);

  filename = make_string ("%s/rtds_GenerateCodeForTASTE.sh", path);
  if (chmod (filename, S_IRWXG | S_IRWXO | S_IRWXU)) {
            fprintf (stderr, 
                "[ERROR] Unable to make file %s executable\n",
                filename);
            exit (-1);
  }
  free (filename);
  free (path);

//  fclose(all_messages);
//  fclose(all_processes);
}

/* External interface */
void GW_RTDS_Backend(FV *fv)
{
  char *inputline   = NULL,
       *outputline  = NULL,
       *sig_decl    = NULL,
       *proc_decl   = NULL,
       *path        = NULL,
       *script_name = NULL;
  FILE *script      = NULL;

  if (fv->system_ast->context->onlycv) return;

  if (rtds == fv->language && fv->system_ast->context->gw) {

#ifndef __unix__
        printf("Warning: skeleton generation for RTDS works only on Linux/FreeBSD\n");
#endif

        /*
         * (1) Dump the template project files
         */
        Create_New_RTDS_Project (fv);

        /*
         * (2) Create strings to declare each interface
         * The strings will be used to fill the project templates
         */
        FOREACH(i, Interface, fv->interfaces, {

                /* 
                 * Signal declarations (asynchronous interfaces) 
                 * They can have only one input parameter 
                 */
                if (asynch == i->synchronism) {
                        build_string (&sig_decl, "signal ", strlen("signal "));
                        build_string (&sig_decl, i->name, strlen(i->name));
                        if (NULL != i->in) {
                                build_string (&sig_decl, "(", strlen("("));
                                build_string (&sig_decl, 
                                             i->in->value->type,
                                             strlen(i->in->value->type));
                                build_string (&sig_decl, ")", strlen(")"));
                        }
                        build_string (&sig_decl, ";\\\n\\\n", strlen(";\\\n\\\n"));
                }

                /*
                 * Provided interfaces can only by asynchronous and have 
                 * a single input parameter 
                 */
                if (PI == i->direction) {
                        build_comma_string(&inputline, i->name, strlen(i->name));
                }
        
                /* 
                 * Required interfaces can be synchronous or asynchronous. 
                 * Synchronous interfaces can have several parameters 
                 */
                else if (RI == i->direction) {

                        /*
                         * Add SDL SIGNAL for Asynchronous RI
                         */
                        if (asynch == i->synchronism) {
                                build_comma_string(&outputline, 
                                                   i->name, 
                                                   strlen(i->name));
                        }
                        /*
                         *  Else : SDL external procedure declaration 
                         */
                        else if (synch == i->synchronism) {
                                build_string (&proc_decl, 
                                             "PROCEDURE syncRI_",
                                             strlen("PROCEDURE syncRI_"));
                                build_string (&proc_decl, 
                                             i->name, 
                                             strlen (i->name));
                                build_string (&proc_decl, 
                                             "(\\\n",
                                             strlen("(\\\n"));
                                FOREACH (p, Parameter, i->in, {
                                        if (p!=i->in->value) {
                                                build_string (&proc_decl,
                                                             ",\\\n",
                                                             strlen (",\\\n"));
                                        }
                                        build_string (&proc_decl, 
                                                     "IN ",
                                                     strlen ("IN "));
                                        build_string (&proc_decl,
                                                     p->name,
                                                     strlen (p->name));
                                        build_string (&proc_decl, " ", 1);
                                        build_string (&proc_decl,
                                                     p->type,
                                                     strlen (p->type));
                                })
                                FOREACH (p, Parameter, i->out, {
                                        if (NULL != i->in || p!=i->out->value) {
                                                build_string (&proc_decl,
                                                             ",\\\n",
                                                             strlen (",\\\n"));
                                        }
                                        build_string (&proc_decl, 
                                                     "IN\\/OUT ",
                                                     strlen ("IN\\/OUT "));
                                        build_string (&proc_decl,
                                                     p->name,
                                                     strlen (p->name));
                                        build_string (&proc_decl, " ", 1);
                                        build_string (&proc_decl,
                                                     p->type,
                                                     strlen (p->type));
                                })
                                build_string (&proc_decl,
                                             "\\\n) EXTERNAL;\\\n\\\n",
                                             strlen ("\\\n) EXTERNAL;\\\n\\\n"));

                        }
                }
        })
#ifndef __unix__
        printf ("!! Non-Linux users, do the following:\n");
        printf ("Open the generated RTDS project and set the process name to");
        printf (" \"%s\"\n", fv->name);
        printf ("Then in SDL text boxes add the following declarations:\n");
        printf ("%s\n\n%s\n\n", 
                NULL!=sig_decl? sig_decl: "", 
                NULL!=proc_decl?proc_decl:"");
        printf ("And fill the SDL connections:\nFrom ENV To %s with", fv->name);
        printf ("[%s]\nand from %s to ENV with [%s]\n\n",
                NULL != inputline? inputline: "",
                fv->name,
                NULL != outputline? outputline: "");
#else
        /*
         * (3) Invoke sed to replace strings in the template files 
         */
        script_name = make_string ("./tmp_rtds_script_%s.sh", fv->name);
        script  = fopen (script_name, "w");

        assert (NULL != script);

        path    = fv->system_ast->context->output;

        /* Project template file: */
        fprintf (script, "sed \'s,MODELNAME,%s,g\' %s%s/%s_project.rdp >_tmp && mv _tmp %s%s/%s_project.rdp\n\n", 
                        fv->name, 
                        NULL!=path?path:"",
                        fv->name,
                        fv->name,
                        NULL!=path?path:"",
                        fv->name,
                        fv->name
        );

        fprintf (script, "sed \'s,generated-process,%s_process,g\' %s%s/%s_project.rdp >_tmp && mv _tmp %s%s/%s_project.rdp\n\n",
                        fv->name,
                        NULL!=path?path:"",
                        fv->name,
                        fv->name,
                        NULL!=path?path:"",
                        fv->name,
                        fv->name
        );

        fprintf (script, "sed \'s,CODEGENDIR,%s,g\' %s%s/%s_project.rdp >_tmp && mv _tmp %s%s/%s_project.rdp\n\n",
                        fv->name,
                        NULL!=path?path:"",
                        fv->name,
                        fv->name,
                        NULL!=path?path:"",
                        fv->name,
                        fv->name
        );

        fprintf (script, "sed \'s,TEMPLATEDIR,../profile,g\' %s%s/%s_project.rdp >_tmp && mv _tmp %s%s/%s_project.rdp\n\n",
                        NULL!=path?path:"",
                        fv->name,
                        fv->name,
                        NULL!=path?path:"",
                        fv->name,
                        fv->name
        );

        /* Process template file: */
        fprintf (script, "sed \'s,MODELNAME,%s,g\' %s%s/%s_process.rdd >_tmp && mv _tmp %s%s/%s_process.rdd\n\n",  
                        fv->name, 
                        NULL!=path?path:"",
                        fv->name,
                        fv->name,
                        NULL!=path?path:"",
                        fv->name,
                        fv->name
        );

        fprintf (script, "sed \'s/INPUTLINE/%s/g\' %s%s/%s_process.rdd >_tmp && mv _tmp %s%s/%s_process.rdd\n\n",  
                        NULL != inputline? inputline:"", 
                        NULL!=path?path:"",
                        fv->name,
                        fv->name,
                        NULL!=path?path:"",
                        fv->name,
                        fv->name
        );

        fprintf (script, "sed \'s/OUTPUTLINE/%s/g\' %s%s/%s_process.rdd >_tmp && mv _tmp %s%s/%s_process.rdd\n\n",  
                        NULL != outputline? outputline: "", 
                        NULL!=path?path:"",
                        fv->name,
                        fv->name,
                        NULL!=path?path:"",
                        fv->name,
                        fv->name
        );

        fprintf (script, "sed \'s,SIGNALS_DECLARATION,%s,g\' %s%s/%s_process.rdd >_tmp && mv _tmp %s%s/%s_process.rdd\n\n",  
                        NULL != sig_decl? sig_decl: "", 
                        NULL!=path?path:"",
                        fv->name,
                        fv->name,
                        NULL!=path?path:"",
                        fv->name,
                        fv->name
        );

        fprintf (script, "sed \'s/PROCEDURE_DECLARATION/%s/g\' %s%s/%s_process.rdd >_tmp && mv _tmp %s%s/%s_process.rdd\n\n",  
                        NULL != proc_decl? proc_decl: "", 
                        NULL!=path?path:"",
                        fv->name,
                        fv->name,
                        NULL!=path?path:"",
                        fv->name,
                        fv->name
        );

        /* Schedule template file */
        fprintf (script, "sed \'s,MODELNAME,%s,g\' %s%s/scheduled.rdd >_tmp && mv _tmp %s%s/scheduled.rdd\n\n",
                        fv->name,
                        NULL!=path?path:"",
                        fv->name,
                        NULL!=path?path:"",
                        fv->name
        );


        fclose (script);

        if (chmod (script_name, S_IRWXG | S_IRWXO | S_IRWXU)) {
            fprintf (stderr, "Unable to change mode of RTDS script and make it executable\n");
            exit (-1);
        }

        if (system(script_name)) {
            fprintf (stderr, "Unable to run %s\n", script_name);
            exit (-1);
        }

        /*
         * Delete the sed script
         */
        if (!get_context()->test) {
            unlink (script_name);
        }
        free (script_name);
        script_name = NULL;
#endif
  }
}
