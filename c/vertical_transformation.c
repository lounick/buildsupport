/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/*
 * Vertical Transformation (VT) backend:
 * Generates the full "Concurrency View" as needed by Ocarina
 * to generate the Ada code of the real-time architecture.
 *
 * 1st version written by Maxime Perrotin/ESA 15/07/2008
 *
 * XXX The computation of priorities must be refactored
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <assert.h>
#include "my_types.h"
#include "practical_functions.h"
#include "c_ast_construction.h"

static FILE *thread, *process, *nodes;
static long long period=0;
static char *cyclic_name=NULL;
static RCM op_kind = undefined;

static unsigned int first_interface = 0;

static char *system_root_node=NULL;

static unsigned int features_declared = 0;
static unsigned int system_connections_declared = 0;


/* 
  Make a local copy of the name of cyclic PI, if any
  This is needed because cyclic threads are treated differently
  than other threads in the Concurrency View.
*/
void set_cyclic_name(char *if_name)
{
   build_string(&cyclic_name, if_name, strlen(if_name));
}

void write_thread_preamble(FV *fv)
{
   if (NULL == thread) return;
   fprintf(thread, "THREAD %s_%s\n", fv->name, fv->name);
   /* The first fv->name should be replaced by the namespace
    * (not supported 15/07/2009) */
}

void write_thread_postamble(FV *fv)
{
   if (NULL == thread) return;
   fprintf(thread, "END %s_%s;\n\n", fv->name, fv->name);
   /* The first fv->name should be replaced by the namespace
    * (not supported 15/07/2009) */
}


/*
   Find_Highest_Period among all interfaces of a FV
*/
void Find_Highest_Period (Interface *i, long long *result)
{
   if (i->period > *result) *result = i->period;
}



/* Thread implementation contains some properties set in the IF view */
void write_thread_implementation(FV *fv)
{
   long long               highest_period       = 0;
   long long               priority             = 0;
   uint64_t                wcet_low;
   uint64_t                wcet_high;
   Interface*              tmp_if;
   Interface_list*         tmp_if_list;

   /*
    * Code related to the integration of the BA annex in threads. Not used yet.
    *
    * int                     ba_preamble          = 0;
    * int                     found                = 0;
   */

   if (NULL == thread) {
      return;
   }

   fprintf(thread, "THREAD IMPLEMENTATION %s_%s.others\n", fv->name, fv->name);
   /* fv->name should be replaced by the namespace
    * (not supported 15/07/2009) (used to be aplc) */

   fprintf(thread, "PROPERTIES\n");

   /* For PolyORB-HI/Ada, Entrypoints refer to Ada subprograms
      generated by the glue generators, for PolyORB-HI/C, they refer to
      the corresponding symbol in the object table

      Note, this could be corrected when moving to AADLv2 to use only
      the C entrypoint
   */
   //char *path = NULL;
   //path =  make_string ("%s/%s", OUTPUT_PATH, fv->name);
   char *init_name = fv->name;
   //if (file_exists(path, "_hook")) {
   if (fv->artificial) {
        char *lookfor = NULL;
        FOREACH (interface, Interface, fv->interfaces, {
                if (PI == interface->direction) {
                    lookfor = interface->distant_name;
                }
        })
        FOREACH (interface, Interface, fv->interfaces, {
                if (RI == interface->direction
                        && !strcmp (lookfor, interface->distant_name) &&
                        synch == interface->synchronism) {
                    init_name = interface->distant_fv;
                 }
        })
   }

   if (0 == fv->system_ast->context->polyorb_hi_c) { /* PolyORB-HI/Ada */
        fprintf(thread,
         "\tInitialize_Entrypoint_Source_Text => \"%s_wrappers.C_Init_%s\";\n",
                init_name,
                init_name);
   }
   else {  /* PolyORB-HI/C or VT-created thread: Add init. entrypoint */
        fprintf(thread,"\tInitialize_Entrypoint_Source_Text => \"init_%s\";\n",
                init_name);
   }

   /* if the thread is cyclic, add a call to its entry point here */
   if(cyclic==op_kind) {
      if (0 == fv->system_ast->context->polyorb_hi_c) { /* PolyORB-HI/Ada */
         fprintf(thread,
                 "\tCompute_Entrypoint_Source_Text => \"%s_wrappers.%s\";\n",
               fv->name,
               cyclic_name);
      }
      else { /* PolyORB-HI/C */
         fprintf(thread,
                 "\tCompute_Entrypoint_Source_Text => \"po_hi_c_%s_%s\";\n",
                 fv->name,
                 cyclic_name);
      }
   }

   fprintf(thread,
           "\tDispatch_Protocol => %s;\n"
           "\tPeriod => %lld ms;\n"
           "\tDispatch_Offset => 0 ms;\n",
           cyclic == op_kind ? "Periodic" : "Sporadic",
           0 == period ? 1 : period);

   if (op_kind == cyclic)
   {
      tmp_if_list = fv->interfaces;
      wcet_low = 100;
      wcet_high = 0;
      while (tmp_if_list != NULL) {
         tmp_if = tmp_if_list->value;

         if ((tmp_if->wcet_low_unit == NULL) ||
            (tmp_if->wcet_high_unit == NULL)) {
            tmp_if_list = tmp_if_list->next;
            continue;
         }

         if (tmp_if->wcet_low < wcet_low) {
               wcet_low = tmp_if->wcet_low;
         }

         if (tmp_if->wcet_high > wcet_high) {
               wcet_high = tmp_if->wcet_high;
         }

         tmp_if_list = tmp_if_list->next;
      }

      fprintf(thread,"\tCompute_Execution_Time => %llu ms .. %llu ms;\n",
        (long long unsigned int)wcet_low,
        (long long unsigned int)wcet_high);
   }

   if (op_kind == sporadic) {
      tmp_if_list = fv->interfaces;
      wcet_low = 100;
      wcet_high = 0;
      while (tmp_if_list != NULL) {
         tmp_if = tmp_if_list->value;

         if ((tmp_if->wcet_low_unit == NULL) ||
            (tmp_if->wcet_high_unit == NULL)) {
            tmp_if_list = tmp_if_list->next;
            continue;
         }

         if (tmp_if->wcet_low < wcet_low) {
               wcet_low = tmp_if->wcet_low;
         }

         if (tmp_if->wcet_high > wcet_high) {
               wcet_high = tmp_if->wcet_high;
         }

         tmp_if_list = tmp_if_list->next;
      }

      fprintf(thread,"\tCompute_Execution_Time => %llu ms .. %llu ms;\n",
        (unsigned long long) wcet_low,
        (unsigned long long) wcet_high);
   }

   /* Default source stack size per thread */
   fprintf(thread,"\tSource_Stack_Size => 50 KByte;\n");

   /* Calculate the priority : temporary solution using the period */
   FOREACH(i, Interface, fv->interfaces, {
    Find_Highest_Period(i, &highest_period);
   })

   /*
    * Higher period => higher priority.
    * Priority low > priority high (POHIC/Linux)
    */
   if (highest_period < 100) priority = 1;
   else if (highest_period >= 100 && highest_period < 250) priority = 5;
   else if (highest_period >= 250 && highest_period < 500) priority = 6;
   else if (highest_period >= 500 && highest_period < 750) priority = 7;
   else if (highest_period >= 750 && highest_period < 1000) priority = 8;
   else if (highest_period == 1000) priority = 9;
   else if (highest_period > 1000) priority = 10;

   fprintf(thread,"\tPriority => %lld;\n", priority);
   /* 
    MP: To be investigated
    JH: To be computed from a schedulability analysis of the system,
        e.g. GRMA using Cheddar
   */
   fprintf(thread,"END %s_%s.others;\n\n", fv->name, fv->name);
}

/* Create the files and put headers */
int Init_VT_Backend(FV *fv)
{
   char *filename = NULL; /* to store the wrappers filename */
   size_t len = 0;           /* length of the filename */
   char *path=NULL;
   if (NULL != fv->system_ast->context->output) {
      build_string(&path, fv->system_ast->context->output,
                   strlen(fv->system_ast->context->output));
   }

   build_string(&path, "ConcurrencyView", strlen("ConcurrencyView"));

   len = strlen(fv->name) + strlen ("_CV_Thread.aadl");
   filename = (char *) malloc (len+1);
   assert(NULL != filename);

   /* create the Aadl files in a directory called "ConcurrencyView" */
   sprintf(filename, "%s_CV_Thread.aadl", fv->name);
   create_file(path, filename, &thread);

   free (filename);


   assert(NULL != thread);
   fprintf(thread,
       "--  This file was generated automatically: DO NOT MODIFY IT !\n\n"
       "--  This file contains a part of the system CONCURRENCY VIEW.\n"
       "--  It is an input file for OCARINA.\n\n");

   int hasparam=0;
   fprintf(thread,
           "package %s_CV_Thread\n"
           "public\n"
           "\twith Deployment;\n"
           "\twith process_package;\n",
           fv->name);

   FOREACH(i, Interface, fv->interfaces, {
      CheckForAsn1Params(i, &hasparam);
   })

   if (hasparam){
      fprintf(thread, "\twith Dataview;\n\n");
   }

   fv->runtime_nature = thread_runtime;
   write_thread_preamble(fv);
   free(path);
   return 0;
}

/* Add a new interface */
/* Note: it concerns only Async with a single IN param */
void Add_IF_to_VT (Interface *i)
{
   if (NULL == thread) {
      if(-1 == Init_VT_Backend(i->parent_fv)) {
         return;
      }
   }

   /* Determine the nature of the thread, based on the nature of the only
    * PROVIDED (added by MP31/07/2009) interface of the FV that can either be
    * "Cyclic" or "Sporadic". Set the period accordingly.
    */

   if (PI == i->direction) {
      switch (i->rcm)
      {
         case cyclic:
            {
               op_kind=cyclic;
               period=i->period;
               set_cyclic_name(i->name);
               break;
            }
         case sporadic:
         case variator:
            {
               op_kind=sporadic;
               period=i->period; 
               break;
            }
         default:
            break;
      }
      /* If it is a sporadic PI, check that it is actually connected,
       * and otherwise do not generate any aadl FEATURE (port)
       */
      if (sporadic == op_kind && NULL == Find_All_Calling_FV(i)) {
          return;
      }
   }


   /*
    * a. Write the interface-related data in the THREAD declaration
    * Note: the case of cyclic PI is treated separately.
    */

   if (cyclic != i->rcm) {

      /*
       * Add a "FEATURE" keyword (once) if
       * the interface is sporadic or protected but NOT unprotected
       * (protected appear in the concurrency view, not unpro).
       * Addition: if protected AND using POHIC
       * (in Ada nothing is generated for protectedRI)
       */

      if (0 == first_interface && unprotected != i->rcm) {

         /* 
          * Don't do it if the interface is protected when using Ada runtime
          */
         if (!(protected == i->rcm && !get_context()->polyorb_hi_c)) {
             fprintf(thread, "FEATURES\n");
             first_interface = 1;
         }
      }

      if (asynch == i->synchronism) {

        fprintf(thread, "\t%s_%s : %s EVENT %sPORT",
            (PI==i->direction)?"INPORT":"OUTPORT",
            i->name,
            (PI==i->direction)?"IN":"OUT",
            (NULL != i->in)?"DATA ":"");

      if (NULL != i->in) {
         fprintf(thread, " DataView::%s_Buffer.impl",
               i->in->value->type
               );
      }

      if (PI == i->direction) {
         fprintf(thread,
                 "\n\t\t{ Compute_Execution_Time => %llu %s .. %llu %s;\n",
                 (long long unsigned) i->wcet_low,
                 i->wcet_low_unit,
                 (long long unsigned) i->wcet_high,
                 i->wcet_high_unit);

         if (i->rcm == sporadic) {
            fprintf(thread, "\t\t  Queue_Size => %lld;\n", i->queue_size);
         }

         if (USE_PO_HI_C (i->parent_fv)) { /* PolyORB-HI/C */
            fprintf(thread,
             "\t\t  Compute_Entrypoint_Source_Text => \"po_hi_c_%s_%s\"; };\n",
                  i->parent_fv->name,
                  i->name
                  );
         }
         else {
            fprintf(thread,
            "\t\t  Compute_Entrypoint_Source_Text => \"%s_wrappers.%s\"; };\n",
                  i->parent_fv->name,
                  i->name
                  );
         }
      }
      else fprintf(thread, ";\n");
      }
   }
}

/* Write postamble data in the files and close them. */
void End_VT_Backend(FV *fv)
{
   write_thread_postamble(fv);
   write_thread_implementation(fv);
   if (NULL != cyclic_name) {
      free(cyclic_name);
      cyclic_name = NULL;
   }

   fprintf(thread, "end %s_CV_Thread;\n", fv->name);

   close_file (&thread);
}

/* External interface: Generate the threads for the vertical transformation */
void GLUE_VT_Backend(FV *fv)
{
        String_list *list_of_required_po = NULL;
        first_interface = 0;


        if (thread_runtime == fv->runtime_nature) {

                FOREACH (i, Interface, fv->interfaces, {
                        if (protected == i->rcm) {
                                ADD_TO_SET (String,
                                            list_of_required_po,
                                            i->distant_fv);
                        }
                        Add_IF_to_VT(i);
                 })

        if (get_context()->polyorb_hi_c && NULL != thread) {
                FOREACH (po, String, list_of_required_po, {
                        fprintf(thread,
                                "\t%s_protected :"
                                " requires data access process_package::"
                                "TASTE_Protected.Object;\n",
                                po);
                })
        }

      End_VT_Backend(fv);
   }
}

/* Following has to be refactored: generation of the missing part of the CV */

/* Create the files and put headers */
int Init_Process_Backend()
{
   Package_list *pkgs_list = get_system_ast()->packages;
   char *filename    = NULL; /* to store the filename */
   size_t   len         = 0;           /* length of the filename */
   char  *path       = NULL;

   if (NULL != get_context()->output) {
      build_string(&path, get_context()->output,
            strlen(get_context()->output));
   }

   build_string(&path, "ConcurrencyView", strlen("ConcurrencyView"));

   len = strlen ("process.aadl");
   filename = (char *) malloc (len+1);

   assert(NULL != filename);
   /* create the Aadl files in a directory called "ConcurrencyView" */
   sprintf(filename, "process.aadl");
   create_file(path, filename, &process);

   free (filename);

   assert(NULL != process);

   fprintf(process,
       "--  This file was generated automatically: DO NOT MODIFY IT !\n\n"
       "--  This file contains a part of the system CONCURRENCY VIEW.\n"
       "--  It is an input file for OCARINA.\n\n"
       "package process_package\n"
       "public\n"
       "\twith Deployment;\n"
       "\twith DataView;\n"
       "\twith interfaceview::IV;\n"
       "\twith deploymentview::DV;\n");
  
   while (pkgs_list != NULL) {
      fprintf(process, "\twith %s;\n", pkgs_list->value);
      pkgs_list = pkgs_list->next;
   }
  
   /* add "with thread_CV_Thread;"  for all threads (bindings)
    * of all processes */
   FOREACH(p, Process, get_system_ast()->processes, {
      FOREACH(b, Aplc_binding, p->bindings, {
         if (thread_runtime == b->fv->runtime_nature) {
            fprintf(process, "\twith %s_CV_Thread;\n", b->fv->name);
         }
      });
   });

   /*
    * If there is at lease one protected interface in a process,
    * generate a "protected object" data in AADL, so that
    * PolyORB-HI-C will know that it needs to generate
    * semaphores.
    * This is not needed for PolyORB-Hi-Ada since Protected
    * objects are already part of the Ada language, so it is 
    * not required to explicitely configure the underlying 
    * operating system with the number of needed semaphores.
    */
   if (get_context()->polyorb_hi_c) {
       int haspro=0;
       FOREACH(fv, FV, get_system_ast()->functions, {
                FOREACH(i, Interface, fv->interfaces, {
                    if (protected == i->rcm) haspro = 1;
                });
       });
       if (haspro) {
           fprintf(process,
                   "\nDATA TASTE_Protected\n"
                   "PROPERTIES\n"
                   "\tConcurrency_Control_Protocol"
//                   " => Immediate_Priority_Ceiling_Protocol;\n"
                   " => Protected_Access;\n"
                   "END TASTE_Protected;\n\n"
                   "DATA IMPLEMENTATION TASTE_Protected.Object\n"
                   "PROPERTIES\n"
                   "\tConcurrency_Control_Protocol"
                   " => Protected_Access;\n"
//                   " => Immediate_Priority_Ceiling_Protocol;\n"
                   "END TASTE_Protected.Object;\n\n");
       }
   }
   free(path);
   return 0;
}


void SetThread(Aplc_binding *b)
{
   if(thread_runtime==b->fv->runtime_nature){
      fprintf(process, "\t%s : thread %s_CV_Thread::%s_%s.others;\n",
            b->fv->name,
            b->fv->name,
            b->fv->name,  /* used to be aplc / should be namespace */
            b->fv->name
            );
   }
}

/* 
 * In the case of PolyORB-HI-C specify the need for semaphores
 * to act as Ada protected objects
 * => in the "process implementation" part of the "process.aadl" file,
 * add "<passive_with_pro_pi_ID>_protected : data TASTE_Protected.Object;"
 * This will provoke the generation of POHIC semaphores that can
 * be locked/unlocked when the passive functions are invoked.
 */
void SetProtectedObject(Aplc_binding *b)
{
    if (passive_runtime == b->fv->runtime_nature) {
        int haspro = 0;
        FOREACH(i, Interface, b->fv->interfaces, {
                 if (PI == i->direction && protected == i->rcm) haspro = 1;
        });
        if (haspro) {
            fprintf(process, "\t%s_protected : data TASTE_Protected.Object;\n",
                    b->fv->name);
        }
    }
}

/* FindDistantFV: returns NULL if the distant FV is in the binding list. */
void FindDistantFV(Aplc_binding *b, char **result)
{
   if (NULL!=*result) {
      if(!strcmp(b->fv->name, *result)) {
         *result = NULL;
      }
   }
}


/* Create the node file used to describe distribution */
int Create_Nodes_file(Process_list *p)
{
   char *filename = "nodes"; 
   char *path=NULL;

   assert (NULL != p);

   if (NULL != p->value && NULL != p->value->bindings 
         && NULL != get_context()->output) {
      build_string(&path, get_context()->output,
            strlen(get_context()->output));
   }

   build_string(&path, "ConcurrencyView", strlen("ConcurrencyView"));

   /* create the nodes file in a directory called "ConcurrencyView" */
   create_file(path, filename, &nodes);

   assert (NULL != nodes);

   /* Write file headers. */
   fprintf(nodes,
         "--  This file was generated automatically: DO NOT MODIFY IT !\n\n"
         "--  This file contains the description of the system distribution.\n"
         "--  It is an input file for the TASTE ORCHESTRATOR\n\n");

   free(path);
   return 0;
}

//void DeclareProcessFeature(Interface *i, Process **current_process)
void DeclareProcessFeatures (Aplc_binding *b,
                             Process **current_process,
                             Port_list **port_list)
{
   char         *distant_fv     = NULL;
   Port         *port           = NULL;
   Port_list    *ports          = NULL;

   if (NULL != *port_list) ports = *port_list;

   FOREACH (i, Interface, b->fv->interfaces, {
       /*
        * Don't connect synchronous and cyclic blocks
        */

       if (NULL != i->distant_fv) distant_fv = i->distant_fv;

       /* 
        * check if distant FV is in the same process
        * (distant_fv becomes NULL in this case) 
        */
       FOREACH(binding, Aplc_binding, (*current_process)->bindings, {
           FindDistantFV(binding, &distant_fv);
       })
       if (asynch == i->synchronism && cyclic != i->rcm) {
           switch (i->direction) {
               case PI:
                   if (NULL != distant_fv) {
                       /*
                        * distant FV not in the same process
                        */

                       if (0 == features_declared) {
                               fprintf(process, "features\n");
                               features_declared = 1;
                       }

                       if (NULL != i->in) {
                               port = make_string (
                                          "\tINPORT_%s_%s : IN EVENT DATA PORT"
                                          " DataView::%s_Buffer.impl;\n",
                                          i->parent_fv->name,
                                          i->port_name,
                                          i->in->value->type);
                       }
                       else {
                               port = make_string (
                                          "\tINPORT_%s_%s : IN EVENT PORT;\n",
                                          i->parent_fv->name,
                                          i->port_name);
                       }
                       ADD_TO_SET(Port, ports, port)
                   }
                   break;
               case RI:
                   if (NULL != distant_fv) {
                       /* Distant RI not in the same process */
                       if (features_declared == 0) {
                           fprintf(process, "features\n");
                           features_declared = 1;
                       }

                       if (i->in != NULL) {
                           port = make_string (
                                "\tOUTPORT_%s_%s : OUT EVENT DATA PORT"
                                " DataView::%s_Buffer.impl;\n",
                                i->parent_fv->name,
                                i->port_name,
                                i->in->value->type);
                       }
                       else {
                           port = make_string (
                                "\tOUTPORT_%s_%s : OUT EVENT PORT;\n",
                                i->parent_fv->name,
                                i->port_name);
                       }
                       ADD_TO_SET(Port, ports, port)
                   }
                   break;
           }
       }
   });
   *port_list = ports;

}


/* Connect async interfaces (internal and external to the process) */
void ProcessConnectInterfaces(Interface *i, Process **current_process)
{
   bool remote_in_same_process = true;
   char *connection = NULL;

   /*
    * Don't connect synchronous and cyclic blocks
    */
   if(asynch != i->synchronism || cyclic == i->rcm) return;

   /* Determine if the remote FV connected to this interface is
      is the same process or it if it distributed */
   if (PI == i->direction) {
       FOREACH (ct, FV, i->calling_threads, {
           bool here = false;
           FOREACH (b, Aplc_binding, (*current_process)->bindings, {
               if (!strcmp (b->fv->name, ct->name)) here = true;
           });
           if (false == here) remote_in_same_process = false;
       });
   }
   else if (RI == i->direction && NULL != i->distant_fv) {
       remote_in_same_process = false;
       FOREACH(b, Aplc_binding, (*current_process)->bindings, {
           if (!strcmp (b->fv->name, i->distant_fv)) {
               remote_in_same_process = true;
           }
       });
   }


   switch (i->direction) {
      case PI:
         if (true == remote_in_same_process) {
            /*
             * if distant FV is in the same process, we do not do anything here
             * because it is handled by the RI (see below)
             */
         }
         else {
            /*
             * distant FV not in the same process
             */
            connection = make_string("\tPORT INPORT_%s_%s -> %s.INPORT_%s;\n",
                                     i->parent_fv->name,
                                     i->port_name,
                                     i->parent_fv->name,
                                     i->name);
         }
         break;
      case RI:
         if (true == remote_in_same_process) {
             /* if distant FV is in the same process */
             /* We do it only for RI and not for PI (see above) because RIs
             * have a unique, thus more reliable Distant FV value. */
             connection = make_string("\tPORT %s.OUTPORT_%s -> %s.INPORT_%s;\n",
                                      i->parent_fv->name,
                                      i->name,
                                      i->distant_fv,
                                      (NULL != i->distant_name)? i->distant_name:i->name
                                      );
         }
         else { /* Distant RI not in the same process */
            connection = make_string("\tPORT %s.OUTPORT_%s -> OUTPORT_%s_%s;\n",
                                     i->parent_fv->name,
                                     i->name,
                                     i->parent_fv->name,
                                     i->port_name);
         }
         break;
   }
   if (NULL != connection) {
       if (0==(*current_process)->connections) {
          fprintf(process,"connections\n");
          (*current_process)->connections = 1;
       }

       fprintf(process, "%s", connection);

       free(connection);
   }
}

void ProcessMakeConnections (Aplc_binding *b, Process **p)
{
   FOREACH(i, Interface, b->fv->interfaces, {
    ProcessConnectInterfaces(i, p);
   });
}


void GenerateProcessImplementation(Process *p)
{
   if (NULL == process || NULL == nodes) return;

   /* Update the Nodes file (used for distribution): */
   fprintf (nodes, "* %s", p->identifier);
   if ( (p->cpu != NULL) && (p->cpu->platform_name != NULL)
           && (strlen (p->cpu->platform_name) > 0)) {
      fprintf (nodes, " %s", p->cpu->platform_name);
   }
   else {
      fprintf (nodes, " default");
   }
   if (p->coverage) {
       fprintf(nodes, " coverage");
   }
   fprintf (nodes, "\n");
   FOREACH(b, Aplc_binding, p->bindings, { 
      fprintf(nodes, "%s\n", b->fv->name);
   })
   /* End distribution */

   Port_list *ports=NULL;
   /* The following line will only work with Ellidiss 1.3.5
    * Old deployment views have to be converted first
    * Using the "future" flag to keep supporting old models
    * (could be fixed by reading the Version property, but
    * a bug in ocarina prevents from doing so - TBC by Jerome)
    */
   if (!get_context()->future) {
       /* 1.3.5 form */
       fprintf(process,"\nprocess %s extends deploymentview::DV::%s::%s\n",
           p->name, p->processor_board_name, p->name);
   }
   else {
       /* 1.2 form */
       fprintf(process,"\nprocess %s extends deploymentview::DV::%s\n",
           p->name, p->name);
   }
   FOREACH(b, Aplc_binding, p->bindings, {
      if (thread_runtime == b->fv->runtime_nature) {
          DeclareProcessFeatures(b, &p, &ports);
      }
   })
   FOREACH (port, Port, ports, {
     fprintf(process, "%s", port);
   })
   FREE_LIST(Port, ports)

   fprintf(process,"end %s;\n", p->name);

   fprintf(process,"\nprocess implementation %s.final\n", p->name);
   fprintf(process,"subcomponents\n");

   FOREACH(b, Aplc_binding, p->bindings, {
     SetThread(b);
     if (get_context()->polyorb_hi_c) SetProtectedObject(b);
   })

   FOREACH(b, Aplc_binding, p->bindings, {
    if (thread_runtime == b->fv->runtime_nature) ProcessMakeConnections(b, &p);
   })

   if (get_context()->polyorb_hi_c) {
       /* The algo is :
        * for each thread T binded to the current process P
        *     for each protected RI of T is protected,
        *         get the corresponding (distant) FV name and store it in a set
        * then
        * for each FV stored in the set, add a link between the thread
        * and the corresponding protected object (FV of the set)
        */
        FOREACH(b, Aplc_binding, p->bindings, {
            Protected_Object_Name_list  *set_of_protected = NULL;

            if (thread_runtime == b->fv->runtime_nature) {
                if (get_context()->test) {
                    printf("\n[thread %s]\n", b->fv->name);
                }
                FOREACH (i, Interface, b->fv->interfaces, {
                    if (protected == i->rcm && RI == i->direction) {
                        ADD_TO_SET (Protected_Object_Name,
                                    set_of_protected,
                                    i->distant_fv);
                        if (get_context()->test) {
                            printf(" PO %s (Pro RI %s)\n",
                                i->distant_fv, i->name);
                        }
                    }
                });
                FOREACH(po, Protected_Object_Name, set_of_protected, {
                    if (0 == p->connections) {
                        fprintf(process,"connections\n");
                        p->connections = 1;
                    }
                    fprintf(process,
                     "\t%s_%s: data access %s_protected -> %s.%s_protected;\n",
                         b->fv->name,
                         po,
                         po,
                         b->fv->name,
                         po);
                });
            }
        });
   }

   fprintf(process,"end %s.final;\n\n", p->name);
   features_declared = 0;
}

void GenerateProcessRefinement(Process *p)
{
   fprintf(process, "  %s: process %s.final;\n", p->identifier, p->name);
}

void GenerateProcessor(Process *p, Process_list *processes)
{
   FOREACH(p2, Process, processes, {
      if (p2 == p) {
         break;
      }

      if ( (p->cpu != NULL) &&
           (p2->cpu != NULL) &&
           (p2->cpu->name != NULL) &&
           (p->cpu->name != NULL) &&
           strcmp (p2->cpu->name, p->cpu->name) == 0) {
         return;
      }
   })

   fprintf(process, "%s : processor %s;\n", p->cpu->name, p->cpu->classifier);

}

/* External interface: Create the PROCESS IMPLEMENTATION part */
void Generate_Full_ConcurrencyView(Process_list *processes, char *root_node)
{
   system_root_node = root_node;

   Create_Nodes_file(processes);

   /* Create the process.aadl file */
   Init_Process_Backend();

   FOREACH(p, Process, processes, {
     GenerateProcessImplementation(p);
   })

   /* postamble: new system implementation */
   if (NULL!= process) {

       fprintf(process, "system %s\nend %s;\n\n", root_node, root_node);
       fprintf(process, "system implementation %s.final\n", root_node);

       fprintf(process, "subcomponents\n");
       FOREACH(b, Bus, get_system_ast()->buses, {
           fprintf(process, "  %s_cv : bus %s;\n", b->name, b->classifier);
       });

      /* Add all drivers of all processes to the complete system */
      FOREACH (p, Process, processes, {
        FOREACH(b, Device, p->drivers, {
                char *driver_no_underscore =
                        underscore_to_dash (b->name, strlen (b->name));
                fprintf(process,
                    "  %s_cv : device %s\n\t{\n\t\tSource_Text =>"
                    " (\"../DriversConfig/%s/DeviceConfig-%s",
                    b->name,
                    b->classifier,
                    p->name,
                    driver_no_underscore);
                if (get_context()->polyorb_hi_c) {
                        fprintf (process,".c");
                }
                else {
                        fprintf (process, ".ads");
                }
                fprintf (process, "\");\n");
                /* Specify the name of the configuration variable
                 * for the driver */
                fprintf (process, "\t\tType_Source_Name => \"");
                /* in C: */
                if (get_context()->polyorb_hi_c) {
                        fprintf (process, "pohidrv_%s_cv",
                            b->name);
                }
                /* or in Ada */
                else {
                        fprintf (process, "DeviceConfig_%s.pohidrv_%s_cv",
                                b->name,
                                b->name);
                }
                fprintf (process, "\";\n");
                fprintf (process, "\t}");
                fprintf(process, ";\n");
                free (driver_no_underscore);
        });
     });

      FOREACH(p, Process, processes, {
        GenerateProcessRefinement(p);
      });

      FOREACH(p, Process, processes, {
          FOREACH(p2, Process, processes, {
              if (p2 == p) {
                  break;
              }

               if ((p->cpu != NULL) &&
                   (p2->cpu != NULL) &&
                   (p2->cpu->name != NULL) &&
                   (p->cpu->name != NULL) &&
                  strcmp (p2->cpu->name, p->cpu->name) == 0) {
                  return;
               }
            });
            fprintf(process, "  %s_cv : processor %s;\n",
                             p->cpu->name,
                             p->cpu->classifier);
         });

          /* Create the connections between the ports at
           * "system implementation deploymentview.final" level
           * this means connect the port name of async RIs to
           * the distant port name */

          if (NULL != get_system_ast()->connections) {
              fprintf(process, "connections\n");
              system_connections_declared = 1;
          }
          FOREACH(cnt, Connection, get_system_ast()->connections, {
                  char  *src_proc=NULL;
                  char  *dest_proc=NULL;

                  FOREACH (local_fv, FV, get_system_ast()->functions, {
                      if (!strcmp(local_fv->name, cnt->src_system)) {
                          src_proc = local_fv->process->name;
                      }
                      if (!strcmp(local_fv->name, cnt->dst_system)) {
                          dest_proc = local_fv->process->name;
                      }
                  });

                  fprintf(process,
                       "  %s_%s_%s_conn_cv :"
                       " port %s.OUTPORT_%s_%s -> %s.INPORT_%s_%s;\n",
                       src_proc,
                       cnt->dst_port,
                       cnt->dst_system,
                       dest_proc,
                       cnt->dst_system,
                       cnt->dst_port,
                       src_proc,
                       cnt->src_system,
                       cnt->src_port);
          })

         FOREACH (p, Process, processes, {
             FOREACH(b, Device, p->drivers, {
                 if (b->accessed_bus != NULL) {
                     if (0 == system_connections_declared) {
                         fprintf(process, "connections\n");
                         system_connections_declared = 1;
                      }
                     fprintf(process, "  bus access %s_cv -> %s_cv.%s;\n",
                                b->accessed_bus,
                                b->name,
                                b->access_port);
                 }
             });
        });

         fprintf (process, "properties\n");
         FOREACH(p, Process, processes, {
             fprintf (process, "  Actual_Processor_Binding =>"
                              " (reference (%s_cv)) applies to %s;\n",
                              p->cpu->name,
                              p->name);
         });

         FOREACH (p, Process, processes, {
             FOREACH(b, Device, p->drivers, {
                 fprintf(process,
                         "  Actual_Processor_Binding =>"
                         " (reference (%s_cv)) applies to %s_cv;\n",
                         b->associated_processor,
                         b->name);
             });
         });

         FOREACH(cnt, Connection, get_system_ast()->connections, {
             char *src_proc = NULL;
             FOREACH (local_fv, FV, get_system_ast()->functions, {
                 if (!strcmp(local_fv->name, cnt->src_system)) {
                     src_proc = local_fv->process->name;
                 }
             });
             fprintf(process,
                 "  Actual_Connection_Binding =>"
                 " (reference (%s_cv)) applies to %s_%s_%s_conn_cv;\n", 
                 cnt->bus,
                 src_proc,
                 cnt->dst_port,
                 cnt->dst_system);
         });


        fprintf(process, "end %s.final;\n", root_node);


        fprintf(process, "end process_package;\n");

        /*
         * Following line is used by the main orchestrator 
         * to know the name of the system (for -I purposes)
        */
        fprintf(process, "\n-- %s.final\n", root_node);

        fclose (process);
    }
    close_file (&nodes);
}
