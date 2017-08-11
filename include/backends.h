/* Buildsupport is (c) 2008-2015 European Space Agency
 * contact: maxime.perrotin@esa.int
 * License is LGPL, check LICENSE file */
/* 
        Backends.h
        External declaration of the backend functions
*/

#include "my_types.h"

/* 
  Here: to add a new backend, just declare the prototype:
*/
extern void GW_SDL_Backend(FV *);
extern void GW_Simulink_Backend(FV *);
extern void GW_C_Backend(FV *);
extern void GW_VDM_Backend(FV *);
extern void GW_VHDL_Backend(FV *);
extern void GW_Ada_Backend(FV *);
extern void GW_SCADE_Backend(FV *);
extern void GW_RTDS_Backend(FV *);
extern void GW_Driver_Backend(FV *);
extern void GW_MicroPython_Backend(FV *);

extern void Create_dataview_uniq();
extern void Delete_dataview_uniq();
extern void Call_asn2dataModel(FV *);
extern void Process_Context_Parameters(FV *);
extern void Process_Directives(FV *);
extern void AADL_CV_Unparser();

extern void  Preprocessing_Backend(System *);
extern void  ModelTransformation_Backend(System *);
extern void  Semantic_Checks();
extern void  GLUE_OG_Backend(FV *);
extern void  GLUE_RTDS_Backend(FV *);
extern void  GLUE_MiniCV_Backend(FV *);
extern void  GLUE_C_Backend(FV *);
extern void  GLUE_GUI_Backend(FV *);
extern void  GLUE_Ada_Wrappers_Backend(FV *);
extern void  GLUE_C_Wrappers_Backend(FV *);
extern void  GLUE_VT_Backend(FV *);
extern void  GLUE_MicroPython_Backend(FV *);
extern void  Backdoor_backend(System *);
extern void  Generate_Build_Script();
extern void  Generate_Full_ConcurrencyView(Process_list *, char *);
extern void  Process_Driver_Configuration(Process *);
extern void  Generate_Python_AST(System *, char *);
extern void  System_Config(System *);
