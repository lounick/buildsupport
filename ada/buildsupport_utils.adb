--  *************************** buildsupport ****************************  --
--  (c) 2008-2017 European Space Agency - maxime.perrotin@esa.int
--  LGPL license, see LICENSE file

with Ada.Text_IO;
with GNAT.OS_Lib;

--  with Imported_Routines;
with Namet;
with Ocarina.Configuration;
with Ocarina.Instances.Queries;
with Ocarina.ME_AADL.AADL_Instances.Nodes;
with Ocarina.ME_AADL.AADL_Instances.Nutils;
with Ada.Characters.Latin_1;

package body Buildsupport_Utils is

   use Ada.Text_IO;
   use GNAT.OS_Lib;

   use Namet;
   use Ocarina.Instances.Queries;
   use Ocarina.ME_AADL.AADL_Instances.Nodes;
   use Ocarina.ME_AADL.AADL_Instances.Nutils;
   use Ada.Characters.Latin_1;

   ------------
   -- Banner --
   ------------

   procedure Banner is
      The_Banner : constant String :=
        "buildsupport - contact: Maxime.Perrotin@esa.int or "
        & "Thanassis.Tsiodras@esa.int "
        & ASCII.LF & ASCII.CR
        & "Based on Ocarina: " & Ocarina.Configuration.Ocarina_Version
        & " (" & Ocarina.Configuration.Ocarina_SVN_Revision & ")";
   begin
      Put_Line (The_Banner);
   end Banner;

   -----------
   -- Usage --
   -----------

   procedure Usage is
   begin
      Put_Line
        ("Usage: buildsupport <options> otherfiles");
      Put_Line
        ("Where <options> are:");
      New_Line;
      Put ("-l, --glue" & HT & HT & HT & HT);
      Put_Line ("Generate glue code");
      Put ("-w, --gw" & HT & HT & HT & HT);
      Put_Line ("Generate code skeletons");
      Put ("-j, --keep-case" & HT & HT & HT & HT);
      Put_Line ("Respect the case for interface names");
      Put ("-o, --output <outputDir>" & HT & HT);
      Put_Line ("Root directory for the output files");
      Put ("-i, --interfaceview <i_view.aadl>" & HT);
      Put_Line ("The interface view in AADL");
      Put ("-c, --deploymentview <d_view.aadl>" & HT);
      Put_Line ("The deployment view in AADL");
      Put ("-d, --dataview <dataview.aadl>" & HT & HT);
      Put_Line ("The data view in AADL");
      Put ("-t, --test" & HT & HT & HT & HT);
      Put_Line ("Dump model information");
      Put ("-g, --debug" & HT & HT & HT & HT);
      Put_Line ("Generate runtime debug output");
      Put ("-s, --stack <stack-value>" & HT & HT);
      Put_Line ("Set the size of the stack per thread in kbytes (default 50)");
      Put ("-x, --timer <timer-resolution in ms>" & HT);
      Put_Line ("Set the timer resolution (default 100 ms)");
      Put ("-v, --version" & HT & HT & HT & HT);
      Put_Line ("Display buildsupport version number");
      Put ("-p, --polyorb-hi-c" & HT & HT & HT);
      Put_Line ("Interface glue code with PolyORB-HI-C");
      Put ("otherfiles" & HT & HT & HT & HT);
      Put_Line ("Any other aadl file you want to parse");
      New_Line;
      New_Line;
      Put_Line ("For example, this command will generate your application"
       & " skeletons:");
      New_Line;
      Put_Line ("buildsupport -i InterfaceView.aadl -d DataView.aadl"
       & " -o code --gw --keep-case");
      New_Line;

   end Usage;

   -------------------
   -- Exit_On_Error --
   -------------------

   procedure Exit_On_Error (Error : Boolean; Reason : String) is
   begin
      if Error then
         Put_Line (Reason);
         --  Imported_Routines.C_End;
         OS_Exit (1);
      end if;
   end Exit_On_Error;

   ----------------------------
   -- Get_RCM_Operation_Kind --
   ----------------------------

   RCM_Operation_Kind : Name_Id;
   Unprotected_Name   : Name_Id;
   Protected_Name     : Name_Id;
   Variator_Name      : Name_Id;
   Modifier_Name      : Name_Id;
   Cyclic_Name        : Name_Id;
   Sporadic_Name      : Name_Id;

   function Get_RCM_Operation_Kind
     (E : Node_Id) return Supported_RCM_Operation_Kind
   is
      RCM_Operation_Kind_N : Name_Id;

   begin
      if Is_Defined_Enumeration_Property (E, RCM_Operation_Kind) then
         RCM_Operation_Kind_N
           := Get_Enumeration_Property (E, RCM_Operation_Kind);

         if RCM_Operation_Kind_N = Unprotected_Name then
            return Unprotected_Operation;

         elsif RCM_Operation_Kind_N =  Protected_Name then
            return Protected_Operation;

         elsif RCM_Operation_Kind_N =  Variator_Name then
            return Variator_Operation;

         elsif RCM_Operation_Kind_N =  Modifier_Name then
            return Modifier_Operation;

         elsif RCM_Operation_Kind_N =  Cyclic_Name then
            return Cyclic_Operation;

         elsif RCM_Operation_Kind_N =  Sporadic_Name then
            return Sporadic_Operation;
         end if;
      end if;
      return Unknown_Operation;
   end Get_RCM_Operation_Kind;

   -----------------------
   -- Get_RCM_Operation --
   -----------------------

   RCM_Operation : Name_Id;

   function Get_RCM_Operation (E : Node_Id) return Node_Id is
   begin
      if Is_Subprogram_Access (E) then
         return Corresponding_Instance (E);
      else
         if Is_Defined_Property (E, RCM_Operation) then
            return Get_Classifier_Property (E, RCM_Operation);
         else
            return No_Node;
         end if;
      end if;
   end Get_RCM_Operation;

   -----------------------
   -- Get_APLC_Binding --
   -----------------------

   APLC_Binding : Name_Id;

   function Get_APLC_Binding (E : Node_Id) return List_Id is
   begin
      if Is_Defined_Property (E, APLC_Binding) then
         return Get_List_Property (E, APLC_Binding);
      else
         return No_List;
      end if;
   end Get_APLC_Binding;

   --------------------
   -- Get_RCM_Period --
   --------------------

   RCM_Period : Name_Id;

   function Get_RCM_Period (D : Node_Id) return Unsigned_Long_Long is
   begin
      if Is_Defined_Integer_Property (D, RCM_Period) then
         return Get_Integer_Property (D, RCM_Period);
      else
         return 0;
      end if;
   end Get_RCM_Period;

   --------------------------
   -- Get_Ada_Package_Name --
   --------------------------

   Ada_Package_Name : Name_id;

   function Get_Ada_Package_Name (D : Node_Id) return Name_Id is
   begin
      return Get_String_Property (D, Ada_Package_Name);
   end Get_Ada_Package_Name;

   -------------------------------
   -- Get_Ellidiss_Tool_Version --
   -------------------------------

   Ellidiss_Tool_Version : Name_id;

   function Get_Ellidiss_Tool_Version (D : Node_Id) return Name_Id is
   begin
      return Get_String_Property (D, Ellidiss_Tool_Version);
   end Get_Ellidiss_Tool_Version;

   ------------------------
   -- Get_Interface_Name --
   ------------------------

   Interface_Name : Name_id;

   function Get_Interface_Name (D : Node_Id) return Name_Id is
   begin
      return Get_String_Property (D, Interface_Name);
   end Get_Interface_Name;

   ---------------------------
   -- Get ASN.1 Module name --
   ---------------------------

   ASN1_Module : Name_id;

   function Get_ASN1_Module_Name (D : Node_Id) return String is
      id : Name_Id := No_Name;
   begin
      if Is_Defined_String_Property (D, ASN1_Module) then
         id := Get_String_Property (D, ASN1_Module);
         return Get_Name_String (id);
      else
         return Get_Name_String (Get_String_Name ("nomodule"));
      end if;
   end Get_ASN1_Module_Name;

   -----------------------
   -- Get_ASN1_Encoding --
   -----------------------

   ASN1_Encoding : Name_Id;
   Native_Name   : Name_Id;
   UPER_Name     : Name_Id;
   ACN_Name      : Name_Id;

   function Get_ASN1_Encoding (E : Node_Id) return Supported_ASN1_Encoding is
      ASN1_Encoding_N : Name_Id;
   begin
      if Is_Defined_Enumeration_Property (E, ASN1_Encoding) then
         ASN1_Encoding_N := Get_Enumeration_Property (E, ASN1_Encoding);

         if ASN1_Encoding_N = Native_Name then
            return Native;

         elsif ASN1_Encoding_N = UPER_Name then
            return UPER;

         elsif ASN1_Encoding_N = ACN_Name then
            return ACN;
         end if;
      end if;
      Exit_On_Error (True, "ASN1 Encoding not set");
      return Default;
   end Get_ASN1_Encoding;

   -------------------------
   -- Get_ASN1_Basic_Type --
   -------------------------

   ASN1_Basic_Type  : Name_Id;
   Sequence_Name    : Name_Id;
   SequenceOf_Name  : Name_Id;
   Enumerated_Name  : Name_Id;
   Set_Name         : Name_Id;
   SetOf_Name       : Name_Id;
   Integer_Name     : Name_Id;
   Boolean_Name     : Name_Id;
   Real_Name        : Name_Id;
   OctetString_Name : Name_Id;
   Choice_Name      : Name_Id;
   String_Name      : Name_Id;

   function Get_ASN1_Basic_Type
     (E : Node_Id)
     return Supported_ASN1_Basic_Type
   is
      ASN1_Basic_Type_N : Name_Id;

   begin
      if Is_Defined_Enumeration_Property (E, ASN1_Basic_Type) then
         ASN1_Basic_Type_N := Get_Enumeration_Property (E, ASN1_Basic_Type);

            if ASN1_Basic_Type_N = Sequence_Name then
               return ASN1_Sequence;

            elsif ASN1_Basic_Type_N = SequenceOf_Name then
               return ASN1_SequenceOf;

            elsif ASN1_Basic_Type_N = Enumerated_Name then
               return ASN1_Enumerated;

            elsif ASN1_Basic_Type_N = Set_Name then
               return ASN1_Set;

            elsif ASN1_Basic_Type_N = SetOf_Name then
               return ASN1_SetOf;

            elsif ASN1_Basic_Type_N = Integer_Name then
               return ASN1_Integer;

            elsif ASN1_Basic_Type_N = Boolean_Name then
               return ASN1_Boolean;

            elsif ASN1_Basic_Type_N = Real_Name then
               return ASN1_Real;

            elsif ASN1_Basic_Type_N = OctetString_Name then
               return ASN1_OctetString;

            elsif ASN1_Basic_Type_N = Choice_Name then
               return ASN1_Choice;

            elsif ASN1_Basic_Type_N = String_Name then
               return ASN1_String;

            else
               raise Program_Error with "Undefined choice "
                 & Get_Name_String (ASN1_Basic_Type_N);
            end if;
      end if;
      Exit_On_Error (True, "Error: ASN.1 Basic type undefined!");
      return ASN1_Unknown;
   end Get_ASN1_Basic_Type;

   ----------
   -- Init --
   ----------

   procedure Init is
   begin
      RCM_Operation_Kind :=
         Get_String_Name ("taste::rcmoperationkind");

      ASN1_Encoding := Get_String_Name ("taste::encoding");

      ASN1_Basic_Type
         := Get_String_Name ("taste::asn1_basic_type");

      RCM_Period := Get_String_Name ("taste::rcmperiod");

      RCM_Operation := Get_String_Name ("taste::rcmoperation");

      APLC_Binding := Get_String_Name ("taste::aplc_binding");

      Ellidiss_Tool_Version := Get_String_Name ("taste::version");
      Interface_Name := Get_String_Name ("taste::interfacename");

      Ada_Package_Name := Get_String_Name ("taste::ada_package_name");

      ASN1_Module := Get_String_Name ("deployment::asn1_module_name");

      Unprotected_Name := Get_String_Name ("unprotected");
      Protected_Name   := Get_String_Name ("protected");
      Variator_Name    := Get_String_Name ("variator");
      Modifier_Name    := Get_String_Name ("modifier");
      Cyclic_Name      := Get_String_Name ("cyclic");
      Sporadic_Name    := Get_String_Name ("sporadic");

      Native_Name   := Get_String_Name ("native");
      UPER_Name     := Get_String_Name ("uper");
      ACN_Name      := Get_String_Name ("acn");

      Sequence_Name    := Get_String_Name ("asequence");
      SequenceOf_Name  := Get_String_Name ("asequenceof");
      Enumerated_Name  := Get_String_Name ("aenumerated");
      Set_Name         := Get_String_Name ("aset");
      SetOf_Name       := Get_String_Name ("asetof");
      Integer_Name     := Get_String_Name ("ainteger");
      Boolean_Name     := Get_String_Name ("aboolean");
      Real_Name        := Get_String_Name ("areal");
      OctetString_Name := Get_String_Name ("aoctetstring");
      Choice_Name      := Get_String_Name ("achoice");
      String_Name      := Get_String_Name ("astring");
   end Init;

end Buildsupport_Utils;
