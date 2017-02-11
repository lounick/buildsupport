--  *************************** buildsupport ****************************  --
--  (c) 2017 European Space Agency - maxime.perrotin@esa.int
--  LGPL license, see LICENSE file

--  Set of helper functions for buildsupport
with Ocarina,
     Types,
     Ada.Containers.Indefinite_Ordered_Maps;

use Ocarina,
    Ada.Containers;

package Buildsupport_Utils is

   use Types;

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

   function Get_ASN1_Basic_Type (E : Node_Id) return Supported_ASN1_Basic_Type;

   function Get_Ada_Package_Name (D : Node_Id) return Name_Id;

   function Get_Ellidiss_Tool_Version (D : Node_Id) return Name_Id;

   function Get_Interface_Name (D : Node_Id) return Name_Id;

   function Get_ASN1_Module_Name (D : Node_Id) return String;

   package Property_Maps is new Indefinite_Ordered_Maps (String, String);

   use Property_Maps;

   function Get_Properties_Map (D : Node_Id) return Property_Maps.Map;

end Buildsupport_Utils;
