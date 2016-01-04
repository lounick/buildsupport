/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/*
 *  AADL Concurrency view "Unparser"  
 *  Generate an interface view AADL file containing the results of the vertical transformation
 *   
*/

/* TODO : (1) add deadlines
	  (2) add correct WCET
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdbool.h>

#include "my_types.h"
#include "practical_functions.h"
#include "c_ast_construction.h"

void AADL_CV_Unparser ()
{
	System 	*ast 			= get_system_ast ();
	FILE 	*aadl 			= NULL;
	int	x0			= -200,
		x1			= 0,
		y0			= 200,
		y1			= 0,
		max_y1			= 0;
				
	Interface_list 	*all_interfaces = NULL;

	create_file (OUTPUT_PATH, "Concurrency-View.aadl", &aadl);
	assert (NULL != aadl);

	/* AADL file preamble */
	fprintf (aadl, "--  TASTE - Warning: This is NOT the real Concurrency view  --\n");
	fprintf (aadl, "--  This file was automatically generated and should remain read-only\n");
	fprintf (aadl, "--  Open it with TASTE-IV\n");
	fprintf (aadl, "--  The only intended use of this file is visualization - not processing.\n");

	fprintf (aadl, "\npackage generated_cv::IV::ConcurrencyView\n");
	fprintf (aadl, "public\n");
	fprintf (aadl, "with dataview; \nwith exportedComponent::FV;\nwith taste;\n\n");

	/* Insert a comment in the AADL file for graphical display */
	fprintf (aadl, "  --{ interfaceview obj6480 10 10\n");
	fprintf (aadl, "  --TASTE-generated concurrency view\n");
	fprintf (aadl, "  --For visualization purposes.\n");
	fprintf (aadl, "  --}\n\n");

	fprintf (aadl, "  system exportedComponent\n  end exportedComponent;\n\n");
	fprintf (aadl, "  system implementation exportedComponent.others\n    subcomponents\n");
	
	/* Declare each subsystem */
	FOREACH (fv, FV, ast->functions, {
		fprintf (aadl, "\t%s : system interfaceview::IV::%s%s.others;\n",
			fv->name,
			thread_runtime == fv->runtime_nature ? "THREAD_" : "PASSIVE_",
			fv->name);
	});

	/* Declare all system-level connections */
	fprintf (aadl, "    connections\n");
	FOREACH (fv, FV, ast->functions, {
		char *fv_name = NULL;

		fv_name = make_string ("%s%s",
                        thread_runtime == fv->runtime_nature ? "THREAD_" : "PASSIVE_",
                        fv->name);

		FOREACH (interface, Interface, fv->interfaces, {

			if (RI == interface->direction) {
				FV *distant_fv = NULL;
				char *distant_fv_name = NULL;

				FOREACH (function, FV, ast->functions, {
					if (!strcmp (function->name, interface->distant_fv))
						distant_fv = function;
				});

				if (NULL != distant_fv) {
					distant_fv_name = make_string ("%s%s",
						thread_runtime == distant_fv->runtime_nature ? "THREAD_" : "PASSIVE_",
			                        distant_fv->name);
				
					fprintf (aadl, "\t%s_%s : subprogram access %s.%s -> %s.%s;\n",
						RCM_KIND(interface),
						interface->distant_name,
						distant_fv_name,
						interface->distant_name,
						fv_name,
						interface->name);
					free (distant_fv_name);
				}
			}
		});
		free (fv_name);
	})

	fprintf (aadl, "  end exportedComponent.others;\n\n");

	/* Define each individual subsystem and its implementation */
	FOREACH (fv, FV, ast->functions, {
		char 	*fv_name 	= NULL;
		int 	nb_pi 		= 0;
		int	nb_ri 		= 0;
		int     if_x 		= 0;
		int	if_y_pi		= 0;
		int	if_y_ri		= 0;

		/* Compute size and coordinated of subsystems */
		FOREACH (interface, Interface, fv->interfaces, {
			if (PI == interface->direction) nb_pi ++;
			else nb_ri ++;
		});

		if (nb_ri > nb_pi) nb_pi = nb_ri;
		
		x0 += 400;
		if (x0 > 2200) {
			x0 = 200;
			y0 = 200  + max_y1 + 100;
		}
		x1 = x0 + 250;
		y1 = y0 + (nb_pi*45);
		if (y1 > max_y1) max_y1 = y1;
		

		fv_name = make_string ("%s%s",
			thread_runtime == fv->runtime_nature ? "THREAD_" : "PASSIVE_",
                        fv->name);

		fprintf (aadl, "  system %s\n    features\n",
                        fv_name);
		if_y_pi = y0 - 30;
		if_y_ri = y0 - 30;
		FOREACH (interface, Interface, fv->interfaces, {
			
			int if_y = 0;
			if (PI == interface->direction) {
				if_x = x0;
				if_y_pi = if_y_pi + 45;
				if_y = if_y_pi;
			}
			else {
				if_x = x1;
				if_y_ri = if_y_ri + 45;
				if_y = if_y_ri;
			}
			
			ADD_TO_SET (Interface, all_interfaces, interface);
			fprintf (aadl, "\t%s : %s subprogram access exportedComponent::fv::%s.others\n",
				interface->name,
				PI == interface->direction ? "provides" : "requires",
				NULL != interface->distant_name ? 
					interface->distant_name:interface->name);
			fprintf (aadl, "\t  { taste::Coordinates => \"%d %d %d %d\";\n",
				if_x,
				if_y,
				if_x,
				if_y);
			fprintf (aadl, "\t    taste::RCMoperationKind => %s;\n",
				RCM_KIND(interface));
			fprintf (aadl, "\t    taste::RCMperiod => %lld;",
				interface->period);
			fprintf (aadl, "};\n");
			/* TODO add deadline */

		});
		fprintf (aadl, "    properties\n\tsource_language => C;\n");
		fprintf (aadl, "\ttaste::coordinates => \"%d %d %d %d\";\n",
			x0,
			y0,	
			x1,
			y1);
		fprintf (aadl, "\ttaste::instance_name => \"%s\";\n",
			fv->name);
		fprintf (aadl, "  end %s;\n\n",
                        fv_name);

		/* Then the system implementation */
		fprintf (aadl, "  system implementation %s.others\n",
			fv_name);
		fprintf (aadl, "    subcomponents\n");
		
		FOREACH (interface, Interface, fv->interfaces, {
			fprintf (aadl, "\t%s_impl : subprogram exportedComponent::FV::%s.others\n",
				interface->name,
				NULL != interface->distant_name ? interface->distant_name : interface->name);
			fprintf (aadl, "\t  { compute_execution_time => 0ms..0ms;};\n");
		});

		fprintf (aadl, "    connections\n");
		FOREACH (interface, Interface, fv->interfaces, {
			fprintf (aadl, "\tsubprogram access %s_impl -> %s;\n",
				interface->name,
				interface->name);
		});
		fprintf (aadl, "  end %s.others;\n\n", 
			fv_name);
		free (fv_name);
		
	});

	fprintf (aadl, "properties\n");
	fprintf (aadl, "  taste::coordinates => \"0 0 2970 2100\";\n\n");
	fprintf (aadl, "end generated_cv::IV;\n\n");

	/* Functional view */
	fprintf (aadl, "package exportedComponent::FV\npublic\n");
	fprintf (aadl, "with dataview;\nwith taste;\n\n");

	FOREACH (interface, Interface, all_interfaces, {
		fprintf (aadl, "  subprogram %s\n  end %s;\n\n",
			NULL != interface->distant_name ? interface->distant_name : interface->name,
			NULL != interface->distant_name ? interface->distant_name : interface->name);
	});
	
	fprintf (aadl, "end exportedComponent::FV;\n");

	close_file (&aadl);

}
