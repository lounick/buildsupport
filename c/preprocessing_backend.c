/* Buildsupport is (c) 2008-2016 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* preprocessing_backend.c

  this backend "pre-processes" the interface view,
  to perform the following operations:
  1) if the FV is a GUI => creates a new separate,
      cyclic FV that is used to poll the GUI queue
  2) if the FV mixes cyclic/sporadic provided interfaces,
     put all the operations in a protected object and create
     one thread per cyclic/sporadic entry
  3) For each PRO/UNPRO RI of each FV,
     attach the RI to the calling thread (SPO/Cyclic FV)
  4) If some nodes use timers, generate the code for a timer
     manager (one per partition).
  5) Set the "ignore_params" flag on interfaces that run on the same node
  6) Create a TASTE API that provides useful features to User functions
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdbool.h>

#include "my_types.h"
#include "practical_functions.h"
#include "c_ast_construction.h"

static int thread_id = 0;

// Files used for the timer management
FILE *header = NULL, *code = NULL;




/* Create a new function to manage the timers of a given node */
FV *Add_timer_manager(Process *node, FV_list *fv_with_timer)
{
    FV *fv = NULL;
    Interface *interface = NULL;
    char *name = NULL;
    char *path = NULL;
    FILE *hook = NULL;
    int  timer_res = get_context()->timer_resolution;

    name = make_string ("%s_timer_manager", node->name);

    fv = (FV *) New_FV (name, strlen(name), name);

    Set_Language_To_C();

    Create_Interface (&interface);

    assert (NULL != fv && NULL != interface);

    interface->name = make_string ("tick_%dms", timer_res);

    interface->distant_fv = NULL;
    interface->direction = PI;
    interface->synchronism = asynch;
    interface->rcm = cyclic;
    /* clock rate for timers is 100 ms by default */
    interface->period = timer_res;
    interface->parent_fv = fv;
    interface->wcet_high = 1;
    interface->wcet_low = 1;
    interface->wcet_low_unit = make_string ("ms");
    interface->wcet_high_unit = make_string ("ms");
    APPEND_TO_LIST (Interface, fv->interfaces, interface);
    /* Set flag indicating that this function was created during VT */
    fv->timer = true;

    End_FV();

    /* Add the new FV to the binding list of the Process */
    Set_Current_Process (node);
    Add_Binding(name, strlen(name));

   /* Create a file called "_hook" to add this function
    * to the orchestrator work */
    path = make_string ("%s/%s", OUTPUT_PATH, fv->name);

    create_file (path, "_hook", &hook);
    create_file (path, make_string ("%s.h", fv->name), &header);
    create_file (path, make_string ("%s.c", fv->name), &code);

    fprintf (header, "/* Timer manager */\n%s", do_not_modify_warning);
    fprintf (code, "/* Timer manager */\n%s", do_not_modify_warning);

    fprintf (header, "#ifndef __AUTO_CODE_H_%s__\n"
                     "#define __AUTO_CODE_H_%s__\n\n"
                     "#include \"C_ASN1_Types.h\"\n"
                     "#ifdef __cplusplus\n"
                     "extern \"C\" {\n"
                     "#endif\n\n",
                     fv->name,
                     fv->name);

    fprintf (header, "void %s_startup();\n\n"
                     "void %s_PI_tick_%dms();\n\n",
                     fv->name,
                     fv->name,
                     timer_res);
    fprintf (code, "#include <assert.h>\n\n"
                   "#include \"%s.h\"\n\n", fv->name);

    /* Create a list of all timers present in the current node */
    String_list *all_timers = NULL;
    int count = 0;
    FOREACH (function, FV, fv_with_timer, {
        FOREACH(timer, String, function->timer_list, {
            APPEND_TO_LIST(String,
                           all_timers,
                           make_string("%s_%s", function->name, timer));
            count ++;
        });
    });

    /* In the header file, generate an enum used as array index */
    fprintf (header, "typedef enum {\n");
    bool first = true;
    int idx = 0;
    FOREACH (timer, String, all_timers, {
        fprintf(header, "%s%s = %d",
                        first?"    ": ",\n    ",
                        timer, idx++);
        first = false;
    });
    fprintf (header, "\n} %s_timers;\n\n", fv->name);

    /* In the code, declare a static variable holding the timers state */
    fprintf (code, "static struct {\n"
                     "    long long value;\n"
                     "    enum { active, inactive } state;\n"
                     "} timers[%d] = {"
                     " { .value = 0, .state = inactive } };\n\n", count);

    /* Initialisation code of the timer manager - nothing special */
    fprintf (code, "void %s_startup()\n"
                   "{\n"
                   "    /* Timer start up (nothing to do) */\n"
                   "}\n\n", fv->name);

    /* Generate the timer manager code for the cyclic interface */
    fprintf (code, "void %s_PI_tick_%dms()\n"
                   "{\n",
                   fv->name,
                   timer_res);

    FOREACH (timer, String, all_timers, {
        fprintf (code, "    if (timers[%s].state == active && "
                       "1 == __sync_fetch_and_sub(&timers[%s].value, 1)) {\n"
                       "        %s_RI_%s();\n"
                       "        timers[%s].state = inactive;\n"
                       "    }\n\n",
                       timer,
                       timer,
                       fv->name,
                       timer,
                       timer);
    });

    fprintf (code, "}\n\n");

    close_file (&hook);
    free (path);

    return fv;
}


void Add_timers_to_function (FV *fv, FV *timer_manager)
{
    Interface *expire = NULL, *set_timer = NULL, *reset_timer = NULL;
    Parameter *param = NULL;
    Interface *cyclic_pi = NULL;
    int timer_res = get_context()->timer_resolution;
    char *cyclic_name = make_string("tick_%dms", timer_res);

    assert (NULL != header && NULL != code);

    /* Find the cyclic interface of the timer manager */
    FOREACH(pi, Interface, timer_manager->interfaces, {
        if (!strcmp(pi->name, cyclic_name)) cyclic_pi = pi;
    });


    FOREACH (timer, String, fv->timer_list, {
        /* Declare functions in timer manager code */
        fprintf (header, "void %s_PI_%s_SET_%s(const asn1SccT_UInt32 *val);\n\n",
                         timer_manager->name,
                         fv->name,
                         timer);
        fprintf (code, "void %s_PI_%s_SET_%s(const asn1SccT_UInt32 *val)\n"
                       "{\n"
                       "    /* Timer value must be multiple of %d ms */\n"
                       "    assert (*val %% %d == 0);\n"
                       "    timers[%s_%s].state = active;\n"
                       "    __sync_lock_test_and_set(&timers[%s_%s].value, *val / %d);\n"
                       "}\n\n",
                       timer_manager->name,
                       fv->name,
                       timer,
                       timer_res,
                       timer_res,
                       fv->name,
                       timer,
                       fv->name,
                       timer,
                       timer_res);
        fprintf (header, "void %s_PI_%s_RESET_%s();\n\n",
                         timer_manager->name,
                         fv->name,
                         timer);
        fprintf (code, "void %s_PI_%s_RESET_%s()\n"
                       "{\n"
                       "    timers[%s_%s].state = inactive;\n"
                       "}\n\n",
                       timer_manager->name,
                       fv->name,
                       timer,
                       fv->name,
                       timer);

        /* Add sporadic PI - corresponding to the timer expiration */
        expire = NULL;
        Create_Interface (&expire);
        expire->name           = make_string (timer);
        expire->distant_name   = make_string ("%s_%s", fv->name, timer);
        expire->distant_fv     = make_string (timer_manager->name);
        expire->direction      = PI;
        expire->synchronism    = asynch;
        expire->rcm            = sporadic;
        expire->period         = timer_res;
        expire->parent_fv      = fv;
        expire->wcet_high      = 10;
        expire->wcet_low       = 10;
        expire->wcet_low_unit  = make_string ("ms");
        expire->wcet_high_unit = make_string ("ms");
        APPEND_TO_LIST (Interface, fv->interfaces, expire);

        /* Add corresponding RI in the timer manager */
        expire = Duplicate_Interface (RI, expire, timer_manager);
        expire->name            = expire->distant_name;
        expire->distant_name    = make_string (timer);
        free (expire->distant_fv);
        expire->distant_fv      = make_string (fv->name);
        /* Filter: set the list of calling PIs in the timer manager */
        ADD_TO_SET(Interface, expire->calling_pis, cyclic_pi);
        APPEND_TO_LIST (Interface, timer_manager->interfaces, expire);

        /* Add Protected PI "RESET_timer" to timer manager (no param) */
        reset_timer = Duplicate_Interface (PI, expire, timer_manager);
        free (reset_timer->name);
        reset_timer->name         = make_string ("%s_RESET_%s",
                                                 fv->name,
                                                 timer);
        free (reset_timer->distant_name);
        reset_timer->distant_name = make_string ("RESET_%s", timer); // irrelevant in PI
        reset_timer->rcm          = protected;
        reset_timer->synchronism  = synch;
        APPEND_TO_LIST (Interface, timer_manager->interfaces, reset_timer);

        /* Add corresponding RI in the user FV */
        reset_timer = Duplicate_Interface (RI, reset_timer, fv);
        free (reset_timer->distant_fv);
        reset_timer->distant_fv   = make_string (timer_manager->name);
        reset_timer->name         = make_string("RESET_%s", timer);
        reset_timer->distant_name = make_string("%s_%s",
                                                fv->name,
                                                reset_timer->name);
        APPEND_TO_LIST (Interface, fv->interfaces, reset_timer);

        /* Add Protected PI "SET_timer(value)" in timer manager */
        /* Note: it must be protected to avoid race condition with the tick function */
        set_timer = Duplicate_Interface (PI, reset_timer, timer_manager);
        free (set_timer->name);
        set_timer->name         = make_string ("%s_SET_%s",
                                               fv->name,
                                               timer);
        free (set_timer->distant_name);
        set_timer->distant_name = make_string ("SET_%s", timer);

        /* Add IN param holding the timer duration */
        Create_Parameter (&param);
        param->name            = make_string ("duration");
        param->type            = make_string ("T_UInt32");
        param->encoding        = native;
        param->asn1_module     = make_string ("taste_basictypes");
        param->basic_type      = integer;
        param->asn1_filename   = make_string ("taste-types.asn");
        param->interface       = set_timer;
        param->param_direction = param_in;
        APPEND_TO_LIST (Parameter, set_timer->in, param);
        APPEND_TO_LIST (Interface, timer_manager->interfaces, set_timer);

        /* Add corresponding RI in the user FV */
        set_timer = Duplicate_Interface (RI, set_timer, fv);
        free (set_timer->distant_fv);
        set_timer->name         = make_string("SET_%s", timer);
        set_timer->distant_name = make_string("%s_%s",
                                                fv->name,
                                                set_timer->name);
        set_timer->distant_fv = make_string (timer_manager->name);
        APPEND_TO_LIST (Interface, fv->interfaces, set_timer);
    });
}

void Close_Timer_Files()
{
    fprintf (header, "#ifdef __cplusplus\n"
                     "}\n"
                     "#endif\n\n"
                     "#endif");
    close_file(&header);
    close_file(&code);
}

void Add_Artificial_Function (Interface *duplicate_pi,
                              RCM pi_rcm,
                              RCM ri_rcm,
                              FV *fv,
                              char *fv_name,
                              char *ri_name,
                              long long period,
                              char *original_name)
{
   FV *new_fv = NULL;
   Interface *duplicate_ri = NULL;
   FILE *hook = NULL;
   char *path = NULL;


   /* 1) Create a new FV called fv_name (argument of the function)
    *    and set 2 interfaces to it (PI and RI) */

   new_fv = (FV *) New_FV (fv_name, strlen(fv_name), fv_name);
   /*
      Setting the language to blackbox_device allows to avoid generating
      unnecessary code for VT-created threads (vm_if, invoke_ri, dataview glue)
   */
   Set_Language_To_BlackBox_Device();

   duplicate_pi->period = period;

   /* TODO
        Question: what about WCET? shouldn't it inherit from the user-code
        WCET, which in turn should be set to 0 ?
   */

   duplicate_pi->parent_fv = new_fv;
   duplicate_pi->rcm       = pi_rcm;

   duplicate_ri = (Interface *) Duplicate_Interface (RI, duplicate_pi, new_fv);
   duplicate_ri->rcm = ri_rcm;
   free(duplicate_ri->distant_fv);
   duplicate_ri->distant_fv = NULL;
   free(duplicate_ri->name);
   duplicate_ri->name = NULL;

   build_string(&(duplicate_ri->distant_fv), fv->name, strlen(fv->name));

   build_string(&(duplicate_ri->name), ri_name, strlen(ri_name));
   duplicate_ri->synchronism = synch;

   APPEND_TO_LIST (Interface, new_fv->interfaces, duplicate_pi);
   APPEND_TO_LIST (Interface, new_fv->interfaces, duplicate_ri);

   End_FV();

   /* 2) Add the new FV to the binding list of the Process */
   if (NULL == fv->process) {
      ERROR ("** Error: function %s is not bound to any process\n",
              fv->name);
      add_error();
      return;
   }

   Set_Current_Process (fv->process);
   Add_Binding(fv_name, strlen(fv_name));

   /* 3) Create a file called "_hook" to add this function
    *    to the orchestrator work */
    path = make_string ("%s/%s", OUTPUT_PATH, new_fv->name);
    new_fv->artificial    = true;
    new_fv->original_name = original_name;

    create_file (path, "_hook", &hook);
    close_file (&hook);
    free (path);
}

void ProcessArtificial_FV_Creation (Interface *i, RCM rcm)
{
  char *interface_name = string_to_lower (i->name);
  char *artificial_pi_name = NULL;
  char *artificial_fv_name = NULL;
  Interface *distant_RI =NULL;
  Interface *duplicate_if =  NULL; 

    artificial_pi_name = make_string ("%s", i->name);
    artificial_fv_name = make_string ("vt_%s_%s",
                                      i->parent_fv->name,
                                      interface_name);

    duplicate_if = (Interface *) Duplicate_Interface (PI, i, i->parent_fv);
    duplicate_if -> parent_fv = NULL;

    free(duplicate_if->name);
    duplicate_if->name = NULL;
    duplicate_if->name = make_string ("artificial_%s", i->name);

    // Set the "distant_name" field of the new PI to the name of the old PI
    // so that if the thread happens to be in a different process from one
    // of its caller, the interface name from TASTE-IV is respected.
    // MP 19/04/10 this is not consistent with the fact that a PI can be
    // connected to several RI we should add real pointers to connect the
    // interfaces rather than strings with their names (or only pointers to
    // distant fv - this does not cover the case of different names)
    // MP 20/04/10 This will have to be checked again with AADL v2.
    // In fact it is correct that the distant name is the original name of
    // the PI because it is the exposed name that TASTE-IV generates in
    // the deployment view. that's the port name seen from the environment.
    // In that case there is only one name and nothing should be changed here.
    // (except that "distant_name" should be renamed "port_name" maybe).
    // MP 21/11/10: aadlv2 update : I added a "port_name"

    if (NULL != duplicate_if->distant_name) {
        free (duplicate_if->distant_name);
        duplicate_if->distant_name=NULL;
    }
    build_string (&(duplicate_if->distant_name), i->name, strlen(i->name));

    Add_Artificial_Function (duplicate_if,
                             (i->rcm==cyclic)?cyclic:sporadic,
                             rcm,
                             i->parent_fv,
                             artificial_fv_name,
                             artificial_pi_name,
                             i->period,
                             i->parent_fv->name); 

    // Update the distant_Fv of the distant RIs invoking the newly created FV
    if (cyclic != i->rcm) {
        FV_list *callers = NULL;

        callers = (FV_list *)Find_All_Calling_FV(i);


        /* For each caller of the interface, find its corresponding RI
         * and update its "distant_fv" field to point to the newly created FV
         */
        FOREACH (caller, FV, callers, {
            distant_RI = NULL;

            /* Find the corresponding interface in the caller */
            distant_RI = FindCorrespondingRI(caller, i);

            if (NULL != distant_RI
                && strcmp(artificial_fv_name, distant_RI->parent_fv->name)) {
                if (NULL != distant_RI->distant_fv) {
                    free(distant_RI->distant_fv);
                    distant_RI->distant_fv = NULL;
                }
                build_string(&(distant_RI->distant_fv),
                             artificial_fv_name, strlen(artificial_fv_name));

                if (NULL != distant_RI->distant_name) {
                        free(distant_RI->distant_name);
                        distant_RI->distant_name = NULL;
                }
                build_string(&(distant_RI->distant_name),
                             "artificial_", strlen("artificial_"));
                build_string(&(distant_RI->distant_name),
                             i->name, strlen(i->name));
            }
        });

    }

    // remove the old distant_fv
    if (NULL != i->distant_fv) { 
      free (i->distant_fv);
      i->distant_fv = NULL;
    }

    build_string (&(i->distant_fv),
                  artificial_fv_name, strlen(artificial_fv_name));
    i->rcm = rcm;
    i->synchronism = synch;
    free (interface_name);
}
/*
 * 4th pre-processing:
 * check if a RI is not already present in a FV and if not, add it.
 * MP 27/07/2010 :
 *  In order to support mix between Sync and Async functions,
 *  we must check only that the EXACT SAME RI (same name but also
 *  same synchronism) is not present.
 *  We can have the same RI name but one being asynchronous,
 *  the other synchronous.
*/
void Add_RI_To_Thread (Interface *i, FV *fv)
{
    bool check = false;
    Interface *new_interface = NULL;
    char *new_name=NULL;

    assert(RI == i->direction);

    build_string(&new_name, i->name, strlen(i->name));
    build_string(&new_name, "_vt", strlen("_vt"));

    /*
     * Check if this RI was not already there 
     * Fixed (MP 27/07/2010) : now allows twice the same RI name
     * but with different synchronism
     * No issue in Ada because async RI are in an
     * "Async_RI" package.
     * New Fix (MP 29/07/2010) : now we completely rename the RI
     * in order to avoid naming conflicts in the AADL files.
    */
    FOREACH(t_i, Interface, fv->interfaces, {
           if (RI==t_i->direction &&
                     !strcmp(t_i->name, new_name)) check = true;
    });

    if(false == check) { /* if not, add it */
        new_interface = (Interface *) Duplicate_Interface (RI, i, fv);

        /*
        * change the name of the RI because there might be a PI with the
        * same name in the calling thread, and AADL does not like having
        * two ports with the same name
        */
        build_string(&(new_interface->name), "_vt", strlen("_vt"));

        if(NULL == new_interface->distant_name) {
            build_string(&(new_interface->distant_name),
                         i->name, strlen(i->name));
        }

        APPEND_TO_LIST (Interface, fv->interfaces, new_interface);
    }
    free(new_name);
}


/* Add a thread to the calling list */
void Add_Thread_To_Calling_List (FV_list **calling_threads_list, FV *fv) 
{
        assert(NULL != calling_threads_list);
        assert(NULL != fv);

        FOREACH(t, FV, *calling_threads_list, {
            if (t == fv) fv = NULL;
        });

        if (NULL != fv) {
            APPEND_TO_LIST(FV, *calling_threads_list, fv);
            return;
        }
}

/* Pre-Processing: propagate Calling thread to distant FV
 *                 (used to allow sync function to make call to RI). */
void Propagate_Calling_Thread(Interface *i, FV **fv)
{
    FV           *distant_fv       = NULL;
    Interface    *corresponding_pi = NULL;

    /* Find the function to which the RI is connected */
    distant_fv = FindFV(i->distant_fv);

    if(NULL == distant_fv) {
        /* This RI is not connected */
        return;
    }

    /* Find the PI corresponding to the RI passed as first argument */
    FOREACH(dist_i, Interface, distant_fv->interfaces, {
        // Long-term temporary fix to support the new version of tasteIV
        char *res = NULL;
        if (NULL != i->distant_name) {
            res = strchr(i->distant_name, '.');
            if (NULL != res) {
                printf("[preprocessing_backend.c] FIXME\n");
                *res = '\0';
            }
        }
        if(PI == dist_i->direction &&
           !strcmp(dist_i->name,
                   NULL != i->distant_name ? i->distant_name : i->name)) {
            corresponding_pi = dist_i;
        }
    });

    assert(NULL != corresponding_pi);

    /*
    * Check that the calling thread is not already in the list of
    * the distant fv/pi. This is essential to avoid infinite recursion
    * in case of circular dependencies
    */
    FOREACH(ct, FV, corresponding_pi->calling_threads, {
        if(ct == *fv) return;
    });

    FOREACH(ct, FV, distant_fv->calling_threads, {
        if(ct == *fv) return;
    });

    /* Add the thread to the list of calling FV of the interface */
    Add_Thread_To_Calling_List (&(corresponding_pi->calling_threads), *fv);

    /* Also add the thread to the calling list of the FV itself */
    Add_Thread_To_Calling_List (&(distant_fv->calling_threads), *fv);

    /* And change the "distant fv" field of the corresponding PI */
    if(NULL != corresponding_pi->distant_fv) {
        free(corresponding_pi->distant_fv);
        corresponding_pi->distant_fv = NULL;
    }
    build_string(&(corresponding_pi->distant_fv),
                 (*fv)->name, strlen((*fv)->name));

    if(passive_runtime == distant_fv->runtime_nature) {
        /* Do the same recursively until we reach another thread */
        FOREACH(sub_i, Interface, distant_fv->interfaces, {
            if (RI == sub_i->direction) {
            /* filter: some RIs are not called by the code of a given PI
             * for example the timer expiration signal is only called by the
             * cyclic PI of the timer manager. Don't propagate in that case
             * to avoid explosion of the number of AADL ports in the CV */
                if (NULL == sub_i->calling_pis
                   || IN_SET(Interface, sub_i->calling_pis, corresponding_pi)) {
                    Propagate_Calling_Thread(sub_i, fv);
                }
            }
        });
    }
}


/*
  Set of preprocessing done for each FV independently
*/
void Preprocess_FV (FV *fv)
{
   int  count_thread    = 0,
        count_pro       = 0,
        count_unpro     = 0;

   /* pre-processing: if a FV is a GUI, add a cyclic PI to the function,
    *  if the GUI has RIs (otherwise there is nothing to poll) */
   if (gui == fv->language) {
      int count_ri=0;
      FOREACH(i, Interface, fv->interfaces, {
            if(RI==i->direction) count_ri++;
      })

      if (count_ri>0)  {
         Interface *interface = NULL;
         char *cyclic_pi_name=NULL;
         build_string(&cyclic_pi_name, "gui_polling_", strlen("gui_polling_"));
         build_string(&cyclic_pi_name, fv->name, strlen(fv->name));

         Create_Interface (&interface);
         if (NULL != interface) {
            build_string (&(interface->name),
                          cyclic_pi_name, strlen(cyclic_pi_name));
            interface->distant_fv = NULL;
            interface->direction=PI;
            interface->synchronism=asynch;
            interface->rcm=cyclic;
            /* Poll GUI queue every 40 ms */
            interface->period = 40;
            interface->parent_fv = fv;
            interface->wcet_high = 1;
            interface->wcet_low = 1;
            build_string(&(interface->wcet_low_unit), "ms", 2);
            build_string(&(interface->wcet_high_unit), "ms", 2);
            APPEND_TO_LIST (Interface, fv->interfaces, interface);
         }
      }
   }

/*
     preprocessing: FV containing more than one
     'active' PI (cyclic/sporadic, protected)
     In that case create one function per PI of the block
     and put all existing PI as protected functions

     1) Count the number of PI and check if more than one is active.
        That's the condition for this VT rule.
*/
        /* Count active and passive PIs of the function */
        FOREACH(i, Interface, fv->interfaces, {
                if (PI == i->direction) 
                        switch (i->rcm) {
                                case cyclic:
                                case sporadic:
                                case variator:    count_thread ++;     break;
                                case protected:   count_pro ++;        break;
                                case unprotected: count_unpro ++;      break;
                                default :                              break;
                        }
        });

/*
        Major change (MP 07/11/2010) to allow mix of UNPRO with other RCM
        Now, user code ALWAYS resides in a "passive" function, and threads
        are created as "wrappers" around them, as blackbox devices
        (so that no additional code is generated).
        This is a generic and optimized mechanism to handle all functions
        in the same manner
*/

    /* 2) Create a new FV for each PI
     *    (except protected/unprotected which remain as is) */

        FOREACH(i, Interface, fv->interfaces, {
            RCM rcm = undefined;
            if (PI == i->direction && (cyclic == i->rcm ||
                                       sporadic == i->rcm ||
                                       variator == i->rcm)) {
                /* Set the interface of the user block as protected
                 * if there is more than 1 active PI on the FV */
                if (count_thread + count_pro > 1) {
                        rcm = protected;
                }
                /* Otherwise there is no need for mutual exclusion -
                 * define as unprotected */
                else rcm = unprotected;
                /* Create a new FV (thread) if there is more than
                 * one PI in the function */
                if (count_pro+count_unpro+count_thread>1) {
                    ProcessArtificial_FV_Creation (i, rcm);
                }
            }
        });


  /*  preprocessing: determine if a FV is a thread or a passive function
   *  (once all artificial threads have been created) */
        fv->runtime_nature = passive_runtime;

        FOREACH (i, Interface, fv->interfaces, {
            if (PI==i->direction) {
                switch (i->rcm) {
                    case cyclic:
                        fv->runtime_nature = thread_runtime;
                        break;
                    case sporadic:
                    case variator:
                        if (NULL != Find_All_Calling_FV(i)) {
                             fv->runtime_nature = thread_runtime;
                        }
                        else {
                            printf("unconnected interface %s %s\n", i->name, i->distant_name);
                            }
                        break;
                    default: break;
                 }
            }
        });

        if(thread_runtime == fv->runtime_nature) {
            /* Set the (unique) thread ID */
            fv->thread_id = ++thread_id;
        }
}

/* Create code that handles timers
 * For each node:
 *      if any timer, add a FV with one cyclic PI named "<node>_timer_manager"
 *      then for each FV of the node:
 *          for each timer with name "t":
 *              add sporadic PI "t" with no param (called when timer expires)
 *              add unpro RI "SET_t" with an int32 param and native encoding
 *              add unpro RI "RESET_t" with no param
 *              add the corresponding PI and RIs to <node>_timer_manager
 *      generate the code of the PIs of the <node>_timer_manager
 */
void Preprocess_timers (Process *node)
{
    bool any_timer = false;
    FV_list *fv_with_timer = NULL;
    FV *timer_manager = NULL;

    /* 1) check if any of the functions of this node uses timer(s) */
    FOREACH (binding, Aplc_binding, node->bindings, {
        if (has_timer(binding->fv)) {
            any_timer = true;
            APPEND_TO_LIST(FV, fv_with_timer, binding->fv);
        }
    });

    if (false == any_timer) {
        return;
    }
    /* 2) Add FV with one cyclic PI to manage the timers of this node */
    timer_manager = Add_timer_manager(node, fv_with_timer);

    /* 3) For each FV with a timer add PI and RI to set, reset, consume */
    FOREACH (fv, FV, fv_with_timer, {
        Add_timers_to_function (fv, timer_manager);
    });

    Close_Timer_Files();
}

/* Create a function named <node>_api and add a number of unprotected PIs */
void Add_api(Process *node, FV_list *all_fv)
{
    FV   *fv   = NULL;
    char *name = NULL;
    char *path = NULL;
    FILE *hook = NULL;

    name = make_string("%s_taste_api", node->name);

    fv = New_FV(name, strlen(name), name);
    assert (NULL != fv);

    Set_Language_To_C();

    /* Add a PI for each of the node's functions */
    FOREACH (function, FV, all_fv, {
        Interface *pi = NULL;
        Interface *ri = NULL;
        Create_Interface(&pi);
        assert(NULL != pi);
        pi->name           = make_string("%s_has_pending_msg", function->name);
        pi->distant_fv     = make_string("%s", function->name);
        pi->distant_name   = make_string("check_queue", pi->name);
        pi->direction      = PI;
        pi->synchronism    = synch;
        pi->rcm            = unprotected;
        pi->period         = 0;
        pi->parent_fv      = fv;
        pi->wcet_high      = 10;
        pi->wcet_low       = 10;
        pi->wcet_low_unit  = make_string("ms");
        pi->wcet_high_unit = make_string("ms");

        /* Add OUT param to handle the result */
        Parameter *param      = NULL;
        Create_Parameter(&param);
        param->name            = make_string("res");
        param->type            = make_string("T_Boolean");
        param->encoding        = native;
        param->asn1_module     = make_string("taste_basictypes");
        param->basic_type      = boolean;
        param->asn1_filename   = make_string("taste-types.asn");
        param->interface       = pi;
        param->param_direction = param_out;
        APPEND_TO_LIST(Parameter, pi->out, param);
        APPEND_TO_LIST(Interface, fv->interfaces, pi);

        /* Add the corresponding RI in the user FV */
        ri = Duplicate_Interface(RI, pi, function);
        ri->name              = make_string("check_queue");
        ri->distant_name      = make_string("%s", pi->name);
        free(ri->distant_fv);
        ri->distant_fv = make_string("%s", fv->name);
        APPEND_TO_LIST(Interface, function->interfaces, ri);
    });

    /* Set flag indicating that this function was created during VT */
    fv->timer = true;
    End_FV();

    /* Add the new FV to the binding list of the Process */
    Set_Current_Process (node);
    Add_Binding(name, strlen(name));

   /* Create a file called "_hook" to add this function
    * to the orchestrator work */
    path = make_string ("%s/%s", OUTPUT_PATH, fv->name);
    create_file(path, "_hook", &hook);
    close_file(&hook);

   /* Create files containing the implementation of the function */
    create_file (path, make_string ("%s.h", fv->name), &header);
    create_file (path, make_string ("%s.c", fv->name), &code);

    fprintf (header, "/* TASTE API */\n%s", do_not_modify_warning);
    fprintf (code,   "/* TASTE API */\n%s", do_not_modify_warning);

    fprintf (code, "#include <deployment.h>\n\n"
                   "#include \"%s.h\"\n\n"
                   "extern int __po_hi_gqueue_get_count(int, int);\n\n",
                   fv->name);


    fprintf (header, "#ifndef __AUTO_CODE_H_%s__\n"
                     "#define __AUTO_CODE_H_%s__\n\n"
                     "#include \"C_ASN1_Types.h\"\n"
                     "#ifdef __cplusplus\n"
                     "    extern \"C\" {\n"
                     "#endif\n\n",
                     fv->name,
                     fv->name);

    /* Debug mode - Unix platform, when env variable CHECKQ_DEBUG is set */
    fprintf (header, "#ifdef __unix__\n"
                     "    #include <stdbool.h>\n"
                     "    #include <stdlib.h>\n"
                     "    static bool debugCheckQ = false;\n"
                     "#endif\n\n");

    fprintf (code,   "#ifdef __unix__\n"
                     "    #include <stdio.h>\n"
                     "#endif\n\n");

    fprintf (header, "void %s_startup();\n\n", fv->name);
    fprintf (code,   "void %s_startup()\n"
                     "{\n"
                     "    /* TASTE API start up */\n"
                     "    #ifdef __unix__\n"
                     "        debugCheckQ = getenv(\"CHECKQ_DEBUG\");\n"
                     "    #endif\n"
                     "}\n\n", fv->name);

    FOREACH(function, FV, all_fv, {
        char *decl = NULL;
        char *task_id = NULL;
        char *port = NULL;
        decl = make_string("void %s_PI_%s_has_pending_msg(asn1SccT_Boolean *res)", fv->name, function->name);
        fprintf(header, "%s;\n\n", decl);
        fprintf(code, "%s {\n"
                      "    /* Check all incoming queues (if any) for a pending message */\n", decl);
        /* Naming of ports/task id is different if there is more than 1 active PI */
        int active = CountActivePI(function->interfaces);
        FOREACH(pi, Interface, function->interfaces, {
            if(PI == pi->direction && asynch == pi->synchronism && cyclic != pi->rcm && NULL != Find_All_Calling_FV(pi)) {
                if (1 == active) {
                    task_id = make_string("%s_%s_k", node->name, function->name);
                    port = make_string("%s_local_inport_%s", function->name, pi->name);
                }
                else { /* More than one active PI */
                    task_id = make_string("%s_vt_%s_%s_k", node->name, function->name, pi->name);
                    port = make_string("vt_%s_%s_local_inport_artificial_%s", function->name, pi->name, pi->name);
                }
                fprintf(code, "    *res = 0;\n"
                              "    if (__po_hi_gqueue_get_count(%s, %s)) {\n"
                              "        *res = 1;\n"
                              "        #ifdef __unix__\n"
                              "            if (debugCheckQ) {\n"
                              "                printf (\"[DEBUG] Pending message %s in function %s\\n\");\n"
                              "            }\n"
                              "        #endif\n"
                              "    }\n",
                              string_to_lower(task_id),
                              string_to_lower(port),
                              pi->name,
                              function->name);
                free(task_id);
                free(port);
            }
        });

        fprintf(code, "}\n\n");
        free(decl);
    });

    fprintf (header, "#ifdef __cplusplus\n"
                     "}\n"
                     "#endif\n\n"
                     "#endif");
    close_file(&header);
    close_file(&code);
    free(path);
}


/* Create a function to handle the calls to the TASTE API
 * For each node:
 *      create a function named <node>_taste_api
 *      for each FV of the node:
 *          add a PI named "<fv>_has_pending_msg (result: boolean)
 *             (reports true if any of the PI queue holds a message)
 *      (later) add a PI named "event"
 */
void Preprocess_taste_api (Process *node)
{
    FV_list *all_fv = NULL;

    /* 1) List all functions of this node */
    FOREACH (binding, Aplc_binding, node->bindings, {
        bool is_async = false;
        FOREACH(iface, Interface, binding->fv->interfaces, {
            /* Discard functions with no active interface */
            if (PI == iface->direction && asynch == iface->synchronism) {
                is_async = true;
            }
        });
        if (gui != binding->fv->language && true == is_async) {
            APPEND_TO_LIST(FV, all_fv, binding->fv);
        }
    });

    /* 2) Create the FV */
    if (NULL != all_fv) {
        Add_api(node, all_fv);
    }
}

/* Create code that handles the collection of code coverage data
 * For each node:
 *      if flag "coverage" is set, add a periodic task that dumps the
 *      coverage data to disk (using gcov flush function)
 */
void Preprocess_coverage (Process *node)
{
    FV *fv = NULL;
    Interface *interface = NULL;
    char *name = NULL;
    char *path = NULL;
    FILE *hook = NULL;

    name = make_string ("%s_coverage_collector", node->name);

    fv = (FV *) New_FV (name, strlen(name), name);

    Set_Language_To_C();

    Create_Interface (&interface);

    assert (NULL != fv && NULL != interface);

    interface->name = make_string ("dump_coverage");

    interface->distant_fv = NULL;
    interface->direction = PI;
    interface->synchronism = asynch;
    interface->rcm = cyclic;
    /* Interface needs to be cyclic but the call does not do anything */
    /* because to collect the coverage data user must send SIGUSR2 */
    interface->period = 100000;
    interface->parent_fv = fv;
    interface->wcet_high = 10;
    interface->wcet_low = 10;
    interface->wcet_low_unit = make_string ("ms");
    interface->wcet_high_unit = make_string ("ms");
    APPEND_TO_LIST (Interface, fv->interfaces, interface);
    /* Set flag indicating that this function was created during VT */
    fv->timer = true;

    End_FV();

    /* Add the new FV to the binding list of the Process */
    Set_Current_Process (node);
    Add_Binding(name, strlen(name));

   /* 3) Create a file called "_hook" to add this function
    *    to the orchestrator work */
    path = make_string ("%s/%s", OUTPUT_PATH, fv->name);

    create_file (path, "_hook", &hook);
    create_file (path, make_string ("%s.h", fv->name), &header);
    create_file (path, make_string ("%s.c", fv->name), &code);

    fprintf (header, "/* Code Coverage manager */\n%s", do_not_modify_warning);
    fprintf (code, "/* Code Coverage manager */\n%s", do_not_modify_warning);

    fprintf (header, "#ifndef __COVERAGE_CODE_H_%s__\n"
                     "#define __COVERAGE_CODE_H_%s__\n\n"
                     "#ifdef __cplusplus\n"
                     "extern \"C\" {\n"
                     "#endif\n\n",
                     fv->name,
                     fv->name);

    fprintf (header, "void %s_startup();\n\n"
                     "void %s_PI_dump_coverage();\n\n",
                     fv->name,
                     fv->name);
    fprintf (code, "#include <signal.h>\n\n"
                   "#include \"%s.h\"\n\n", fv->name);

    fprintf (code, "/* Dummy cyclic function */\n"
                   "void %s_PI_dump_coverage()\n"
                   "{\n"
                   "}\n\n", fv->name);

    fprintf (code, "/* Signal handler to dump coverage information */\n"
                   "void gc_handler()\n"
                   "{\n"
                   "    puts(\"Collecting code coverage\");\n"
                   "    extern void __gcov_flush();\n"
                   "    __gcov_flush();\n"
                   "    exit(0);\n"
                   "}\n\n");

    /* Initialisation code of the coverage manager */
    fprintf (code, "void %s_startup()\n"
                   "{\n"
                   "    puts(\"use kill -SIGUSR2 to collect code coverage\");\n"
                   "    signal(SIGUSR2, gc_handler);\n"
                   "}\n\n",
                   fv->name);

    close_file (&hook);
    free (path);

    fprintf (header, "#ifdef __cplusplus\n"
                     "}\n"
                     "#endif\n\n"
                     "#endif");
    close_file(&header);
    close_file(&code);
}



/* Look at all provided interfaces of a function and if some do
 * not reside on the same node as any of their callers set the ignore_params
 * flag to false. This enables some runtime optimisations wrt buffer copies.
*/
void Set_Ignore_Params(FV *fv)
{
    Interface *distant_RI = NULL;

    FOREACH(i, Interface, fv->interfaces, {
        if (PI == i->direction) {
            FOREACH (remote, FV, Find_All_Calling_FV(i), {
                if (strcmp(remote->process->name, fv->process->name)) {
                    i->ignore_params = false;
                    //printf("set PI %s->ignore_params to FALSE", i->name);
                    distant_RI = FindCorrespondingRI(remote, i);
                    if (NULL != distant_RI) {
                        distant_RI->ignore_params = false;
                    //printf("set RI %s->ignore_params to FALSE", i->name);
                    }
                }
            });
        }
    });
}

/* External interface */
void Preprocessing_Backend (System *s)
{

    FOREACH (node, Process, s->processes, {
        /* Manage timers that may be declared as context parameters */
        Preprocess_timers(node);
        /* Create a TASTE API function with a set of unprotected PIs */
        if(get_context()->polyorb_hi_c) {
            /* Functionality requires POHIC API */
            Preprocess_taste_api(node);
        }
    });

    /* Manage coverage flag for each node */
    FOREACH (node, Process, s->processes, {
        if(node->coverage) Preprocess_coverage(node);
    });

    FOREACH (fv, FV, s->functions, {
        Preprocess_FV(fv);
    });

    /* Propagate calling thread to distant PI (recursively)
     * i.e look for all threads (functions have already been preprocessed)
     * then go through all their RIs recursively until reaching another thread,
     * and set that the function is a calling thread of the reached one */
    FOREACH(fv, FV, s->functions, {
        if (thread_runtime == fv->runtime_nature) {
            FOREACH(i, Interface, fv->interfaces, {
                if (RI == i->direction) Propagate_Calling_Thread (i, &fv);
            })
        }
    });

    /* Add each RI of passive functions as RI of their calling threads */
    FOREACH(fv, FV, s->functions, {
        if(passive_runtime==fv->runtime_nature) {
            FOREACH(i, Interface, fv->interfaces, {
                if (RI==i->direction && i->distant_qgen->language == other) {
                    /* Add to calling threads only of PIs that actually call
                     * the current RI - filtering to be done, but the result
                     * is the ri->calling_pis list */
                    FV_list *callers = NULL;
                    if (NULL == i->calling_pis) {
                        callers = fv->calling_threads;
                    }
                    else {
                        /* Build a set of actual calling threads of this RI */
                        FOREACH(calling_pi, Interface, i->calling_pis, {
                            FOREACH(thread_caller, FV, calling_pi->calling_threads, {
                                ADD_TO_SET(FV, callers, thread_caller);
                            });
                        });
                    }
                    //FOREACH(thread, FV, fv->calling_threads, {
                    FOREACH(thread, FV, callers, {
                        Add_RI_To_Thread(i, thread);
                        Propagate_Calling_Thread(i, &thread);
                    });
                }
            });
        }
    });

    /*
     * After the preprocessing, we must refine the system-level connections:
     * During AST construction, connections are only between threads
     * but since we transform some threads into (un)protected functions,
     * and we create new threads we must refactor the connections 
     * For each connection:
     *   if destination is passive: define actual_dest=corresponding thread
     *   else keep current destination
     *   Then if source is passive:
     *      for each calling thread CT of source
     *          create a new connection between CT and actual_dest
     *   else create a new connection between source and actual dest
     *   then remove the original connnection 
     *  Note: for some reasons src and dest are inverted in the AST:
     *   src_system means FV of the callee (destination of the message)
     *   dst_system means FV of the caller (source of the message)
     */
    Connection_list *new_connections = NULL;
    Connection_list *connections_to_remove = NULL;
    FOREACH (cnt, Connection, get_system_ast()->connections, {
        FV          *caller = NULL;
        FV          *callee = NULL;
        Interface   *ri     = NULL;

        /* Find the FV of the caller */
        FOREACH (fv, FV, get_system_ast()->functions, {
            if (!strcmp (fv->name, cnt->dst_system)) caller = fv;
         });
        assert (NULL != caller);

        /* Find the corresponding RI in the caller */
        FOREACH (i, Interface, caller->interfaces, {
            if (!strcmp (i->name, cnt->dst_port)) ri = i;
        });
        assert (NULL != ri);

        /* Find the FV of the callee */
        FOREACH (fv, FV, get_system_ast()->functions, {
            if (!strcmp (fv->name, ri->distant_fv)) callee = fv;
        });

        /* If the caller is passive we must create one connection
         * per calling thread */
        if (passive_runtime == caller->runtime_nature && NULL != callee) {
            FOREACH (ct, FV, caller->calling_threads, {
                New_Connection (
                    callee->name,   strlen (callee->name),
                    cnt->src_port,    strlen (cnt->src_port),
                    cnt->bus,       strlen (cnt->bus),
                    ct->name,       strlen (ct->name),
                    cnt->dst_port,    strlen (cnt->dst_port)
                );
                APPEND_TO_LIST (Connection, new_connections, Get_Connection());
            });
            APPEND_TO_LIST (Connection, connections_to_remove, cnt);
        }
        else if (NULL != callee && strcmp (callee->name, cnt->src_system)) {
                New_Connection (
                    callee->name,     strlen (callee->name),
                    cnt->src_port,    strlen (cnt->src_port),
                    cnt->bus,         strlen (cnt->bus),
                    caller->name,     strlen (caller->name),
                    cnt->dst_port,    strlen (cnt->dst_port)
                );
                APPEND_TO_LIST (Connection, new_connections, Get_Connection());
                APPEND_TO_LIST (Connection, connections_to_remove, cnt);
        }
    });
    /* Add newly-created connections to the global list, remove the old ones */
    if (NULL != new_connections) {
        FOREACH (cnt, Connection, new_connections, {
            APPEND_TO_LIST (Connection, get_system_ast()->connections, cnt);
        });
        FOREACH (cnt, Connection, connections_to_remove, {
            REMOVE_FROM_LIST (Connection, get_system_ast()->connections, cnt);
        });
    }

    new_connections = NULL;
    connections_to_remove = NULL;

    /* Once all AST transformations are done, set the ignore_params flags
     * in all interfaces, if needed 
     */
    FOREACH (fv, FV, s->functions, {
        Set_Ignore_Params(fv);
    });
}
