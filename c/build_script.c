/* Buildsupport is (c) 2008-2016 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* build_script.c

  this backend generates the build script that generates the binary files

  1st version 20 may 2009

  Copyright 2014-2015 IB Krates <info@krates.ee>
       QGenc code generator integration

 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>

#include "my_types.h"
#include "practical_functions.h"
#include "c_ast_construction.h"


void Create_script()
{
    FILE *script = NULL;

    if (file_exists(".", "build-script.sh") && rename("build-script.sh", "build-script.sh.old")) {
        ERROR ("[ERROR] Unable to rename the existing 'build-script.sh' file. Check for permission.\n");
        exit(-1);
    }
    else {
        create_file (".", "build-script.sh", &script);
        ERROR ("[INFO] A build script (build-script.sh) has been created.\n"
                    "       The next time you will run 'taste-generate-skeletons' or 'buildsupport',\n"
                    "       this script will be renamed to 'build-script.sh.old' and replaced with a new, updated one.\n"
                    "       If applicable, do not forget to merge the two files before running the new script.\n\n");
        }

    assert (NULL != script);

    fprintf (script, "#!/bin/bash\n\n"
                     "# This script will build your TASTE system.\n\n"
                     "# You should check it before running it to make it fit with your context:\n"
                     "# 1) You may need to fix some paths and filenames (path to interface/deployment views)\n"
                     "# 2) You may need to specify additional paths for the compiler to find .h file\n"
                     "#    (e.g. \"export C_INCLUDE_PATH=/usr/include/xenomai/analogy/:$C_INCLUDE_PATH\")\n"
                     "# 3) You may need to link with pre-built libraries, using the -l option\n"
                     "#    (e.g. -l /usr/lib/libanalogy.a,/usr/lib/librtdm.a)\n"
                     "# 4) You may need to change the runtime (add -p flag to select PolyORB-HI-C)\n"
                     "# etc.\n\n"
                     "# Note: TASTE will not overwrite your changes - if you need to update some parts\n"
                     "#       you will have to merge the changes with the newly-created file.\n\n");

    /* The first step is to invoke QGen to generate code from user model */
    FOREACH (fv, FV, get_system_ast()->functions, {

        if (qgenada == fv->language) {

        FOREACH(i, Interface, fv->interfaces, {
            switch (i->direction) {
                case PI: {
                    fprintf (script, "\n");
                    fprintf (script, "# Call QGen to generate Ada code\n");
                    fprintf (script, "printf \"Calling QGen to generate Ada code from %s.mdl with the following command line:\\n\"\n", i->name);
                    fprintf (script, "printf \"qgenc %s.mdl --typing %s_types.txt --incremental --no-misra --language ada --output %s\\n\"\n", i->name, i->name, i->distant_qgen->fv_name);
                    fprintf (script, "printf \"Output from QGen\\n\\n\"\n");
                    fprintf (script, "qgenc %s.mdl --typing %s_types.txt --incremental --no-misra --language ada --output %s\n", i->name, i->name, fv->name);
                    fprintf (script, "printf \"\\nEnd of output from QGen\\n\\n\"\n\n");
                    } break;
                case RI: break;
                default: break;
            }
        });
        }

        else if (qgenc == fv->language) {

        FOREACH(i, Interface, fv->interfaces, {
            switch (i->direction) {
                case PI: {
                    fprintf (script, "\n");
                    fprintf (script, "# Call QGen to generate C code\n");
                    fprintf (script, "printf \"Calling QGen to generate C code from %s.mdl with the following command line:\\n\"\n", i->name);
                    fprintf (script, "printf \"qgenc %s.mdl --typing %s_types.txt --incremental --no-misra --language c --output %s\\n\"\n", i->name, i->name, i->distant_qgen->fv_name);
                    fprintf (script, "printf \"Output from QGen\\n\\n\"\n");
                    fprintf (script, "qgenc %s.mdl --typing %s_types.txt --incremental --no-misra --language c --output %s\n\n", i->name, i->name, fv->name);
                    fprintf (script, "printf \"\\nEnd of output from QGen\\n\\n\"\n\n");
                    fprintf (script, "# Add QGen generated C code to C_INCLUDE_PATH\n");
                    fprintf (script, "export C_INCLUDE_PATH=../../%s/%s/:$C_INCLUDE_PATH\n", fv->name, fv->name);
                    } break;
                case RI: break;
                default: break;
            }
        });
        }
    });

    fprintf (script, "if [ -z \"$DEPLOYMENTVIEW\" ]\n"
                     "then\n"
                     "    DEPLOYMENTVIEW=DeploymentView.aadl\n"
                     "fi\n\n");

    fprintf (script, "SKELS=\"%s\"\n\n", OUTPUT_PATH);

    fprintf (script, "# Update the data view with local paths\n"
                     "taste-update-data-view\n\n");

    /* OpenGEODE-specific: call code generator on the fly */
    FOREACH (fv, FV, get_system_ast()->functions, {
        if (sdl == fv->language) {
            fprintf(script,
                    "# Generate code for OpenGEODE function %s\n"
                    "cd \"$SKELS\"/%s && "
                    "opengeode --toAda %s.pr system_structure.pr "
                    "&& cd $OLDPWD\n\n",
                    fv->name, fv->name, fv->name);
        }
    });

    /* VDM-Specific: call code generator and B mappers */
    FOREACH (fv, FV, get_system_ast()->functions, {
        if (vdm == fv->language) {
            fprintf(script,
                    "# Generate code for VDM function %s\n"
                    "# cd \"$SKELS\"/%s && "
                    "vdm2c %s.vdmpp %s_Interface.vdmpp out.vdm"
                    "&& cd $OLDPWD\n\n",
                    fv->name, fv->name, fv->name, fv->name);
        }
        /* TODO call B mappers or add --subVdm in orchestrator */
    });

    /* Remove old zip files and create fresh new ones from user code */
    FOREACH (fv, FV, get_system_ast()->functions, {
        //if (sdl != fv->language  PUT BACK WHEN OPENGEODE FULLY SUPPORTED
        if (vhdl != fv->language
            && gui != fv->language
            && rtds != fv->language
            && NULL == fv->zipfile) {
            fprintf (script,
                    "cd \"$SKELS\" && rm -f %s.zip && "
                    "zip %s %s/* && cd $OLDPWD\n\n",
                    fv->name, fv->name, fv->name);
        }
    })

    fprintf (script, "[ ! -z \"$CLEANUP\" ] && rm -rf binary\n\n");

    fprintf (script, "if [ -f ConcurrencyView.pro ]\n"
                     "then\n"
                     "    ORCHESTRATOR_OPTIONS+=\" -w ConcurrencyView.pro \"\n"
                     "fi\n\n"
                     "if [ ! -z \"$USE_POHIC\" ]\n"
                     "then\n"
                     "    OUTPUTDIR=binary.c\n"
                     "    ORCHESTRATOR_OPTIONS+=\" -p \"\n"
                     "elif [ ! -z \"$USE_POHIADA\" ]\n"
                     "then\n"
                     "    OUTPUTDIR=binary.ada\n"
                     "else\n"
                     "    OUTPUTDIR=binary\n"
                     "fi\n"
                     );

    fprintf (script, "assert-builder-ocarina.py \\\n");
    fprintf (script, "\t--fast \\\n"
                     "\t--debug \\\n");

    if (get_context()->polyorb_hi_c)
        fprintf (script,"\t--with-polyorb-hi-c \\\n");
    if (get_context()->aadlv2)
        fprintf (script, "\t--aadlv2 \\\n");
    if (get_context()->keep_case)
        fprintf (script, "\t--keep-case \\\n");

    fprintf (script, "\t--interfaceView %s \\\n", get_context()->ifview);

    /* When invoked with -gw, we do not know the exact name of the deployment view
       -> we assume that it's the default spelling given by TASTE-IV */
    fprintf (script, "\t--deploymentView \"$DEPLOYMENTVIEW\" \\\n");

    fprintf (script, "\t-o \"$OUTPUTDIR\"");

    FOREACH (fv, FV, get_system_ast()->functions, {

        if (gui != fv->language) {
            fprintf (script, " \\\n\t");

            switch (fv->language) {
                //case sdl: fprintf (script, "--subOG ");   UNCOMMENT WHEN OPENGEODE FULLY SUPPORTED
                //      break;
                case scade: fprintf (script, "--subSCADE ");
                      break;
                case simulink: fprintf (script, "--subSIMULINK ");
                      break;
                case rtds: fprintf (script, "--subRTDS ");
                      break;
                case c:
                case vdm:
                case blackbox_device:
                    fprintf (script, "--subC ");
                    break;
                case cpp:
                    fprintf (script, "--subCPP ");
                    break;
                case sdl:
                case ada: fprintf (script, "--subAda ");
                      break;
                case vhdl: fprintf (script, "--subVHDL ");
                      break;
                case qgenada: fprintf (script, "--subQGenAda ");
                      break;
                case qgenc: fprintf (script, "--subQGenC ");
                      break;
                default:
                     ERROR ("** Error: unsupported language (function %s)\n", fv->name);
                     ERROR ("  -> please manually check the build-script.sh file\n");
                     break;
            }

            fprintf (script, "%s", fv->name);
            /* ObjectGEODE systems are treated differently from all other languages,
               because it directly refers to .pr files, instead of zip files 
            if (sdl == fv->language) { UNCOMMENT WHEN OPENGEODE FULLY SUPPORTED
                fprintf (script, ":\"$SKELS\"/%s/system_implementation.pr, \"$SKELS\"/%s/system_structure.pr, \"$SKELS\"/%s/DataView.pr",
                fv->name, 
                fv->name, 
                fv->name);
            } */
            /*else*/
            if (rtds == fv->language && NULL == fv->zipfile) {
                fprintf (script, ":\"$SKELS\"/%s/%s.zip", fv->name, fv->name);
            }
            else if (NULL != fv->zipfile) {
                fprintf (script, ":\"$SKELS\"/%s", fv->zipfile);
            }
            else if (vhdl != fv->language) {
                fprintf(script, ":\"$SKELS\"/%s.zip", fv->name);
            }
        }

    })
    /* If node is configured with code coverage flag, set the option */
    FOREACH (process, Process, get_system_ast()->processes, {
        if (true == process->coverage) {
            fprintf(script,
                    " \\\n\t--nodeOptions %s@gcov=on",
                    process->name);
        }
    });

    /* Let user add custom orchestrator flags */
    fprintf (script, " \\\n\t$ORCHESTRATOR_OPTIONS\n");
    close_file(&script);

    if (chmod ("build-script.sh", S_IRWXG | S_IRWXO | S_IRWXU)) {
        fprintf (stderr, "Warning: Unable to change mode of build-script.sh to make it executable\n");
    }
}


/* External interface */
void Generate_Build_Script()
{
    if (get_context()->gw && !get_context()->glue && !get_context()->onlycv) {
    Create_script();
    }
}
