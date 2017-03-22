--  *************************** buildsupport ****************************  --
--  (c) 2017 European Space Agency - maxime.perrotin@esa.int
--  LGPL license, see LICENSE file

--  Set of helper functions for buildsupport
with Ocarina,
     Ocarina.Types,
     Ada.Containers.Indefinite_Ordered_Maps,
     Ada.Containers.Vectors;

use Ocarina,
    Ada.Containers;

package Buildsupport_Utils is

   use Ocarina.Types;

   procedure Banner;

   procedure Usage;

   procedure Exit_On_Error (Error : Boolean; Reason : String);

   type Synchronism is (Sync, Async);

   procedure Init;

   type Supported_RCM_Operation_Kind is (Unprotected_Operation,
                                         Protected_Operation,
                                         Variator_Operation,
                                         Modifier_Operation,
                                         Cyclic_Operation,
                                         Sporadic_Operation,
                                         Unknown_Operation);

   function Get_RCM_Operation_Kind
     (E : Node_Id)
     return Supported_RCM_Operation_Kind;

   function Get_RCM_Operation (E : Node_Id) return Node_Id;

   function Get_RCM_Period (D : Node_Id) return Unsigned_Long_Long;

   function Get_APLC_Binding (E : Node_Id) return List_Id;

   type Supported_ASN1_Encoding is (Default, Native, UPER, ACN);

   function Get_ASN1_Encoding (E : Node_Id) return Supported_ASN1_Encoding;

   type Supported_ASN1_Basic_Type is (ASN1_Sequence,
                                      ASN1_SequenceOf,
                                      ASN1_Enumerated,
                                      ASN1_Set,
                                      ASN1_SetOf,
                                      ASN1_Integer,
                                      ASN1_Boolean,
                                      ASN1_Real,
                                      ASN1_OctetString,
                                      ASN1_Choice,
                                      ASN1_String,
                                      ASN1_Unknown);

   package Property_Maps is new Indefinite_Ordered_Maps (String, String);
   package String_Vectors is new Vectors (Natural, String);

   function Get_ASN1_Basic_Type (E : Node_Id) return Supported_ASN1_Basic_Type;

   function Get_Ada_Package_Name (D : Node_Id) return Name_Id;

   function Get_Ellidiss_Tool_Version (D : Node_Id) return Name_Id;

   function Get_Interface_Name (D : Node_Id) return Name_Id;

   function Get_ASN1_Module_Name (D : Node_Id) return String;

   function Get_Properties_Map (D : Node_Id) return Property_Maps.Map;

   type Taste_Interface is
       record
           null;
       end record;

   subtype Taste_Provided_Interface is Taste_Interface;
   subtype Taste_Required_Interface is Taste_Interface;

   package Interfaces is new Vectors (Natural, Taste_Interface);

   type Taste_Function is
       record
           Name_Full_Case : String;
           Language       : Supported_Source_Language;
           Zip_File       : String;
           Context_Params : Property_Maps.Map;
           Timers         : String_Vectors.Vector;
           Provided       : Interfaces.Vector;
           Required       : Interfaces.Vector;
       end record;

end Buildsupport_Utils;
