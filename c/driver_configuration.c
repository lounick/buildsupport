/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/*
 *  Generate an ASN.1 module containing the declaration of 
 *  device driver configuration, as found in deployment view 
 *  Then call asn1.exe to make it available to Polyorb
 *   
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>

#include "my_types.h"
#include "practical_functions.h"
#include "c_ast_construction.h"


void Process_Node_Driver_Configuration(Device *device, char *node)
{
        char            *command                = NULL;
        FILE            *asn                    = NULL;
        char            *device_no_underscore   = NULL;
        char            *asn1scc_path           = NULL;
        int             will_fail               = 0 ;
        ASN1_Filename   *filename               = NULL;
        char            *driver_conf_path       = NULL;
        int             ret                     = 0;

        if (NULL == device->configuration || NULL == device->asn1_filename) return;        
     
        /* 
         * Make sure the device name has no underscore (to be ASN.1-friendly)
         */
        device_no_underscore = 
            underscore_to_dash (device->name, strlen(device->name));

        /* 
         * Create the ASN.1 file to store the configuration variable
         */
        filename = make_string ("DeviceConfig-%s.asn", device_no_underscore);     
        asn = fopen (filename, "wt");
        assert (NULL != asn);

        /*
         * Get the ASN.1 type name and the device type
         * The device type name is deduced from the ASN.1 file basename 
         * The ASN.1 type is the same with a forced upper 1st-character
         */

        fprintf(asn, "DeviceConfig-%s DEFINITIONS ::=\n", device_no_underscore);
        fprintf(asn, "BEGIN\n");

        /* 
         * Import the ASN.1 module containing the type definition
         */
        fprintf(asn, "IMPORTS %s FROM %s;\n\n", device->asn1_typename, device->asn1_modulename);
 
        /* 
         * Declare a variable of the configuration type.
         */
        fprintf(asn, "\npohidrv-%s-cv %s ::= %s\n\n", 
            device_no_underscore, 
            device->asn1_typename, 
            device->configuration);
        
	fprintf(asn, "END\n");
        fclose (asn);

        /* 
         * Call asn1.exe with the newly-created ASN.1 file 
        */
        asn1scc_path = getenv ("ASN1SCC");
        
        if (NULL == asn1scc_path) { 
            ERROR ("** Error: environment variable ASN1SCC not set\n");
            will_fail = 1;
        }
        driver_conf_path = make_string ("%s/DriversConfig/", OUTPUT_PATH);
        ret = mkdir (driver_conf_path, 0700);

        if (-1 == ret) {
            switch (errno) {
                case EEXIST : break;
                default : ERROR ("** Error: Impossible to create directory %s\n", driver_conf_path);
                          exit (-1);
                          return;
                          break;
            }
        }
        build_string (&driver_conf_path, node, strlen(node));
        ret = mkdir (driver_conf_path, 0700);
         if (-1 == ret) {
            switch (errno) {
                case EEXIST : break;
                default : ERROR ("** Error: Impossible to create directory %s\n", driver_conf_path);
                          exit (-1);
                          return;
                          break;
            }
        }


        command = make_string ("mono %s -%s -o %s %s %s", 
                asn1scc_path,
                C_DRIVER ? "c -typePrefix __po_hi_c_": "Ada",
                driver_conf_path,
                filename,
                device->asn1_filename);

        ERROR ("%s %s\n", 
                will_fail? "Because of the above error(s), the following command cannot be executed :\n":
                "Executing",
                command);

        if (will_fail) {
            ERROR ("Fix the errors and restart buildsupport!\n");
            exit (-1);
        }

        if (!will_fail && system (command)) {
            ERROR ("The command failed. Try it yourself (correct paths, access to files, etc.)\n");
            exit (-1);
        }
        else if (!get_context()->test) {
            remove (filename);
        }

        free (filename);
        free (command);
        free (device_no_underscore);
}

void Process_Driver_Configuration (Process *p)
{
    FOREACH (driver, Device, p->drivers, {
            Process_Node_Driver_Configuration (driver, p->name);
    })
}


