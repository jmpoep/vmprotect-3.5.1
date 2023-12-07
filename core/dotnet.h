#ifndef DOTNET_H
#define DOTNET_H

typedef LPVOID  mdScope;                // Why is this still needed?
typedef ULONG32 mdToken;                // Generic token


// Token  definitions


typedef mdToken mdModule;               // Module token (roughly, a scope)
typedef mdToken mdTypeRef;              // TypeRef reference (this or other scope)
typedef mdToken mdTypeDef;              // TypeDef in this scope
typedef mdToken mdFieldDef;             // Field in this scope  
typedef mdToken mdMethodDef;            // Method in this scope 
typedef mdToken mdParamDef;             // param token  
typedef mdToken mdInterfaceImpl;        // interface implementation token

typedef mdToken mdMemberRef;            // MemberRef (this or other scope)  
typedef mdToken mdCustomAttribute;      // attribute token
typedef mdToken mdPermission;           // DeclSecurity 

typedef mdToken mdSignature;            // Signature object 
typedef mdToken mdEvent;                // event token  
typedef mdToken mdProperty;             // property token   

typedef mdToken mdModuleRef;            // Module reference (for the imported modules)  

// Assembly tokens.
typedef mdToken mdAssembly;             // Assembly token.
typedef mdToken mdAssemblyRef;          // AssemblyRef token.
typedef mdToken mdFile;                 // File token.
typedef mdToken mdExportedType;         // ExportedType token.
typedef mdToken mdManifestResource;     // ManifestResource token.

#ifndef __IMAGE_COR20_HEADER_DEFINED__

typedef enum ReplacesCorHdrNumericDefines
{
	// COM+ Header entry point flags.
	COMIMAGE_FLAGS_ILONLY               =0x00000001,
	COMIMAGE_FLAGS_32BITREQUIRED        =0x00000002,
	COMIMAGE_FLAGS_IL_LIBRARY           =0x00000004,
	COMIMAGE_FLAGS_STRONGNAMESIGNED     =0x00000008,
	COMIMAGE_FLAGS_NATIVE_ENTRYPOINT    =0x00000010,
	COMIMAGE_FLAGS_TRACKDEBUGDATA       =0x00010000,

	// Version flags for image.
	COR_VERSION_MAJOR_V2                =2,
	COR_VERSION_MAJOR                   =COR_VERSION_MAJOR_V2,
	COR_VERSION_MINOR                   =0,
	COR_DELETED_NAME_LENGTH             =8,
	COR_VTABLEGAP_NAME_LENGTH           =8,

	// Maximum size of a NativeType descriptor.
	NATIVE_TYPE_MAX_CB                  =1,
	COR_ILMETHOD_SECT_SMALL_MAX_DATASIZE=0xFF,

	// #defines for the MIH FLAGS
	IMAGE_COR_MIH_METHODRVA             =0x01,
	IMAGE_COR_MIH_EHRVA                 =0x02,
	IMAGE_COR_MIH_BASICBLOCK            =0x08,

	// V-table constants
	COR_VTABLE_32BIT                    =0x01,          // V-table slots are 32-bits in size.
	COR_VTABLE_64BIT                    =0x02,          // V-table slots are 64-bits in size.
	COR_VTABLE_FROM_UNMANAGED           =0x04,          // If set, transition from unmanaged.
	COR_VTABLE_FROM_UNMANAGED_RETAIN_APPDOMAIN  =0x08,  // If set, transition from unmanaged with keeping the current appdomain.
	COR_VTABLE_CALL_MOST_DERIVED        =0x10,          // Call most derived method described by

	// EATJ constants
	IMAGE_COR_EATJ_THUNK_SIZE           =32,            // Size of a jump thunk reserved range.

	// Max name lengths
	//@todo: Change to unlimited name lengths.
	MAX_CLASS_NAME                      =1024,
	MAX_PACKAGE_NAME                    =1024,
} ReplacesCorHdrNumericDefines;

typedef struct IMAGE_COR20_HEADER
{
    // Header versioning
    DWORD                   cb;
    WORD                    MajorRuntimeVersion;
    WORD                    MinorRuntimeVersion;

    // Symbol table and startup information
    IMAGE_DATA_DIRECTORY    MetaData;
    DWORD                   Flags;

    // If COMIMAGE_FLAGS_NATIVE_ENTRYPOINT is not set, EntryPointToken represents a managed entrypoint.
    // If COMIMAGE_FLAGS_NATIVE_ENTRYPOINT is set, EntryPointRVA represents an RVA to a native entrypoint.
    union {
        DWORD               EntryPointToken;
        DWORD               EntryPointRVA;
    } DUMMYUNIONNAME;

    // Binding information
    IMAGE_DATA_DIRECTORY    Resources;
    IMAGE_DATA_DIRECTORY    StrongNameSignature;

    // Regular fixup and binding information
    IMAGE_DATA_DIRECTORY    CodeManagerTable;
    IMAGE_DATA_DIRECTORY    VTableFixups;
    IMAGE_DATA_DIRECTORY    ExportAddressTableJumps;

    // Precompiled image info (internal use only - set to zero)
    IMAGE_DATA_DIRECTORY    ManagedNativeHeader;

} IMAGE_COR20_HEADER, *PIMAGE_COR20_HEADER;
#endif

// TypeDef/ExportedType attr bits, used by DefineTypeDef.
typedef enum CorTypeAttr
{
	// Use this mask to retrieve the type visibility information.
	tdVisibilityMask        =   0x00000007,
	tdNotPublic             =   0x00000000,     // Class is not public scope.
	tdPublic                =   0x00000001,     // Class is public scope.
	tdNestedPublic          =   0x00000002,     // Class is nested with public visibility.
	tdNestedPrivate         =   0x00000003,     // Class is nested with private visibility.
	tdNestedFamily          =   0x00000004,     // Class is nested with family visibility.
	tdNestedAssembly        =   0x00000005,     // Class is nested with assembly visibility.
	tdNestedFamANDAssem     =   0x00000006,     // Class is nested with family and assembly visibility.
	tdNestedFamORAssem      =   0x00000007,     // Class is nested with family or assembly visibility.

	// Use this mask to retrieve class layout information
	tdLayoutMask            =   0x00000018,
	tdAutoLayout            =   0x00000000,     // Class fields are auto-laid out
	tdSequentialLayout      =   0x00000008,     // Class fields are laid out sequentially
	tdExplicitLayout        =   0x00000010,     // Layout is supplied explicitly
	// end layout mask

	// Use this mask to retrieve class semantics information.
	tdClassSemanticsMask    =   0x00000020,
	tdClass                 =   0x00000000,     // Type is a class.
	tdInterface             =   0x00000020,     // Type is an interface.
	// end semantics mask

	// Special semantics in addition to class semantics.
	tdAbstract              =   0x00000080,     // Class is abstract
	tdSealed                =   0x00000100,     // Class is concrete and may not be extended
	tdSpecialName           =   0x00000400,     // Class name is special.  Name describes how.

	// Implementation attributes.
	tdImport                =   0x00001000,     // Class / interface is imported
	tdSerializable          =   0x00002000,     // The class is Serializable.

	// Use tdStringFormatMask to retrieve string information for native interop
	tdStringFormatMask      =   0x00030000,     
	tdAnsiClass             =   0x00000000,     // LPTSTR is interpreted as ANSI in this class
	tdUnicodeClass          =   0x00010000,     // LPTSTR is interpreted as UNICODE
	tdAutoClass             =   0x00020000,     // LPTSTR is interpreted automatically
	// end string format mask

	tdBeforeFieldInit       =   0x00100000,     // Initialize the class any time before first static field access.

	// Flags reserved for runtime use.
	tdReservedMask          =   0x00040800,
	tdRTSpecialName         =   0x00000800,     // Runtime should check name encoding.
	tdHasSecurity           =   0x00040000,     // Class has security associate with it.
} CorTypeAttr;

typedef enum CorILMethodFlags
{ 
	CorILMethod_InitLocals      = 0x0010,           // call default constructor on all local vars   
	CorILMethod_MoreSects       = 0x0008,           // there is another attribute after this one    

	CorILMethod_CompressedIL    = 0x0040,           // FIX Remove this and do it on a per Module basis  

	// Indicates the format for the COR_ILMETHOD header 
	CorILMethod_FormatShift     = 2,
	CorILMethod_FormatMask      = ((1 << CorILMethod_FormatShift) - 1), 
	CorILMethod_TinyFormat      = 0x0002,
	CorILMethod_SmallFormat     = 0x0000,
	CorILMethod_FatFormat       = 0x0003,
} CorILMethodFlags;

typedef struct IMAGE_COR_ILMETHOD_FAT
{
	unsigned Flags    : 12;     // Flags    
	unsigned Size     :  4;     // size in DWords of this structure (currently 3)   
	unsigned MaxStack : 16;     // maximum number of items (I4, I, I8, obj ...), on the operand stack   
	DWORD   CodeSize;           // size of the code 
	mdSignature   LocalVarSigTok;     // token that indicates the signature of the local vars (0 means none)  
} IMAGE_COR_ILMETHOD_FAT;

typedef enum CorILMethodSect                             // codes that identify attributes
{
	CorILMethod_Sect_Reserved    = 0,
	CorILMethod_Sect_EHTable     = 1,
	CorILMethod_Sect_OptILTable  = 2,

	CorILMethod_Sect_KindMask    = 0x3F,        // The mask for decoding the type code
	CorILMethod_Sect_FatFormat   = 0x40,        // fat format
	CorILMethod_Sect_MoreSects   = 0x80,        // there is another attribute after this one
} CorILMethodSect;

typedef enum CorExceptionFlag
{
	COR_ILEXCEPTION_CLAUSE_NONE,                    
	COR_ILEXCEPTION_CLAUSE_FILTER  = 0x0001,
	COR_ILEXCEPTION_CLAUSE_FINALLY = 0x0002,
	COR_ILEXCEPTION_CLAUSE_FAULT = 0x0004,
} CorExceptionFlag;

// MethodDef attr bits, Used by DefineMethod.
typedef enum CorMethodAttr
{
    // member access mask - Use this mask to retrieve accessibility information.
    mdMemberAccessMask          =   0x0007,
    mdPrivateScope              =   0x0000,     // Member not referenceable.
    mdPrivate                   =   0x0001,     // Accessible only by the parent type.  
    mdFamANDAssem               =   0x0002,     // Accessible by sub-types only in this Assembly.
    mdAssem                     =   0x0003,     // Accessibly by anyone in the Assembly.
    mdFamily                    =   0x0004,     // Accessible only by type and sub-types.    
    mdFamORAssem                =   0x0005,     // Accessibly by sub-types anywhere, plus anyone in assembly.
    mdPublic                    =   0x0006,     // Accessibly by anyone who has visibility to this scope.    
    // end member access mask

    // method contract attributes.
    mdStatic                    =   0x0010,     // Defined on type, else per instance.
    mdFinal                     =   0x0020,     // Method may not be overridden.
    mdVirtual                   =   0x0040,     // Method virtual.
    mdHideBySig                 =   0x0080,     // Method hides by name+sig, else just by name.

    // vtable layout mask - Use this mask to retrieve vtable attributes.
    mdVtableLayoutMask          =   0x0100,
    mdReuseSlot                 =   0x0000,     // The default.
    mdNewSlot                   =   0x0100,     // Method always gets a new slot in the vtable.
    // end vtable layout mask

    // method implementation attributes.
    mdCheckAccessOnOverride     =   0x0200,     // Overridability is the same as the visibility.
    mdAbstract                  =   0x0400,     // Method does not provide an implementation.
    mdSpecialName               =   0x0800,     // Method is special.  Name describes how.
    
    // interop attributes
    mdPinvokeImpl               =   0x2000,     // Implementation is forwarded through pinvoke.
    mdUnmanagedExport           =   0x0008,     // Managed method exported via thunk to unmanaged code.

    // Reserved flags for runtime use only.
    mdReservedMask              =   0xd000,
    mdRTSpecialName             =   0x1000,     // Runtime should check name encoding.
    mdHasSecurity               =   0x4000,     // Method has security associate with it.
    mdRequireSecObject          =   0x8000,     // Method calls another method containing security code.

} CorMethodAttr;

// Assembly attr bits, used by DefineAssembly.
typedef enum CorAssemblyFlags
{
    afPublicKey             =   0x0001,     // The assembly ref holds the full (unhashed) public key.
    
    afPA_None               =   0x0000,     // Processor Architecture unspecified
    afPA_MSIL               =   0x0010,     // Processor Architecture: neutral (PE32)
    afPA_x86                =   0x0020,     // Processor Architecture: x86 (PE32)
    afPA_IA64               =   0x0030,     // Processor Architecture: Itanium (PE32+)
    afPA_AMD64              =   0x0040,     // Processor Architecture: AMD X64 (PE32+)
    afPA_ARM                =   0x0050,     // Processor Architecture: ARM (PE32)
    afPA_ARM64              =   0x0060,     // Processor Architecture: ARM64 (PE32+)
    afPA_NoPlatform         =   0x0070,      // applies to any platform but cannot run on any (e.g. reference assembly), should not have "specified" set
    afPA_Specified          =   0x0080,     // Propagate PA flags to AssemblyRef record
    afPA_Mask               =   0x0070,     // Bits describing the processor architecture
    afPA_FullMask           =   0x00F0,     // Bits describing the PA incl. Specified
    afPA_Shift              =   0x0004,     // NOT A FLAG, shift count in PA flags <--> index conversion

    afEnableJITcompileTracking   =  0x8000, // From "DebuggableAttribute".
    afDisableJITcompileOptimizer =  0x4000, // From "DebuggableAttribute".
    afDebuggableAttributeMask    =  0xc000,

    afRetargetable          =   0x0100,     // The assembly can be retargeted (at runtime) to an
                                            //  assembly from a different publisher.

    afContentType_Default         = 0x0000, 
    afContentType_WindowsRuntime  = 0x0200, 
    afContentType_Mask            = 0x0E00, // Bits describing ContentType
} CorAssemblyFlags;

// FieldDef attr bits, used by DefineField.
typedef enum CorFieldAttr
{
    // member access mask - Use this mask to retrieve accessibility information.
    fdFieldAccessMask           =   0x0007,
    fdPrivateScope              =   0x0000,     // Member not referenceable.
    fdPrivate                   =   0x0001,     // Accessible only by the parent type.
    fdFamANDAssem               =   0x0002,     // Accessible by sub-types only in this Assembly.
    fdAssembly                  =   0x0003,     // Accessibly by anyone in the Assembly.
    fdFamily                    =   0x0004,     // Accessible only by type and sub-types.
    fdFamORAssem                =   0x0005,     // Accessibly by sub-types anywhere, plus anyone in assembly.
    fdPublic                    =   0x0006,     // Accessibly by anyone who has visibility to this scope.
    // end member access mask

    // field contract attributes.
    fdStatic                    =   0x0010,     // Defined on type, else per instance.
    fdInitOnly                  =   0x0020,     // Field may only be initialized, not written to after init.
    fdLiteral                   =   0x0040,     // Value is compile time constant.
    fdNotSerialized             =   0x0080,     // Field does not have to be serialized when type is remoted.

    fdSpecialName               =   0x0200,     // field is special.  Name describes how.

    // interop attributes
    fdPinvokeImpl               =   0x2000,     // Implementation is forwarded through pinvoke.

    // Reserved flags for runtime use only.
    fdReservedMask              =   0x9500,
    fdRTSpecialName             =   0x0400,     // Runtime(metadata internal APIs) should check name encoding.
    fdHasFieldMarshal           =   0x1000,     // Field has marshalling information.
    fdHasDefault                =   0x8000,     // Field has default.
    fdHasFieldRVA               =   0x0100,     // Field has RVA.
} CorFieldAttr;

// MethodImpl attr bits, used by DefineMethodImpl.
typedef enum CorMethodImpl
{
	// code impl mask
	miCodeTypeMask      =   0x0003,   // Flags about code type.   
	miIL                =   0x0000,   // Method impl is IL.   
	miNative            =   0x0001,   // Method impl is native.     
	miOPTIL             =   0x0002,   // Method impl is OPTIL 
	miRuntime           =   0x0003,   // Method impl is provided by the runtime.
	// end code impl mask

	// managed mask
	miManagedMask       =   0x0004,   // Flags specifying whether the code is managed or unmanaged.
	miUnmanaged         =   0x0004,   // Method impl is unmanaged, otherwise managed.
	miManaged           =   0x0000,   // Method impl is managed.
	// end managed mask

	// implementation info and interop
	miForwardRef        =   0x0010,   // Indicates method is defined; used primarily in merge scenarios.
	miPreserveSig       =   0x0080,   // Indicates method sig is not to be mangled to do HRESULT conversion.

	miInternalCall      =   0x1000,   // Reserved for internal use.

	miSynchronized      =   0x0020,   // Method is single threaded through the body.
	miNoInlining        =   0x0008,   // Method may not be inlined.                                      
	miMaxMethodImplVal  =   0xffff,   // Range check value    
} CorMethodImpl;

typedef enum CorPropertyAttr
{
    prSpecialName           =   0x0200,     // property is special.  Name describes how.

    // Reserved flags for Runtime use only.
    prReservedMask          =   0xf400,
    prRTSpecialName         =   0x0400,     // Runtime(metadata internal APIs) should check name encoding.
    prHasDefault            =   0x1000,     // Property has default

    prUnused                =   0xe9ff,
} CorPropertyAttr;

// Event attr bits, used by DefineEvent.
typedef enum CorEventAttr
{
    evSpecialName           =   0x0200,     // event is special.  Name describes how.

    // Reserved flags for Runtime use only.
    evReservedMask          =   0x0400,
    evRTSpecialName         =   0x0400,     // Runtime(metadata internal APIs) should check name encoding.
} CorEventAttr;

// ManifestResource attr bits, used by DefineManifestResource.
typedef enum CorManifestResourceFlags
{
	mrVisibilityMask = 0x0007,
	mrPublic = 0x0001,     // The Resource is exported from the Assembly.
	mrPrivate = 0x0002,     // The Resource is private to the Assembly.
} CorManifestResourceFlags;

typedef enum CorMethodSemanticsAttr
{
	msSetter = 0x0001,
	msGetter = 0x0002,
	msOther = 0x0004,
	msAddOn = 0x0008,
	msRemoveOn = 0x0010,
	msFire = 0x0020,
} CorMethodSemanticsAttr;

//*****************************************************************************
//
// Element type for Cor signature
//
//*****************************************************************************

typedef enum CorElementType
{
	ELEMENT_TYPE_END            = 0x0,  
	ELEMENT_TYPE_VOID           = 0x1,  
	ELEMENT_TYPE_BOOLEAN        = 0x2,  
	ELEMENT_TYPE_CHAR           = 0x3,  
	ELEMENT_TYPE_I1             = 0x4,  
	ELEMENT_TYPE_U1             = 0x5, 
	ELEMENT_TYPE_I2             = 0x6,  
	ELEMENT_TYPE_U2             = 0x7,  
	ELEMENT_TYPE_I4             = 0x8,  
	ELEMENT_TYPE_U4             = 0x9,  
	ELEMENT_TYPE_I8             = 0xa,  
	ELEMENT_TYPE_U8             = 0xb,  
	ELEMENT_TYPE_R4             = 0xc,  
	ELEMENT_TYPE_R8             = 0xd,  
	ELEMENT_TYPE_STRING         = 0xe,  

	// every type above PTR will be simple type 
	ELEMENT_TYPE_PTR            = 0xf,      // PTR <type>   
	ELEMENT_TYPE_BYREF          = 0x10,     // BYREF <type> 

	// Please use ELEMENT_TYPE_VALUETYPE. ELEMENT_TYPE_VALUECLASS is deprecated.
	ELEMENT_TYPE_VALUETYPE      = 0x11,     // VALUETYPE <class Token> 
	ELEMENT_TYPE_CLASS          = 0x12,     // CLASS <class Token>
	ELEMENT_TYPE_VAR	        = 0x13,	    // number

	ELEMENT_TYPE_ARRAY          = 0x14,     // MDARRAY <type> <rank> <bcount> <bound1> ... <lbcount> <lb1> ...  
	ELEMENT_TYPE_GENERICINST    = 0x15,     // <type> <type-arg-count> <type-1> \x{2026} <type-n>

	ELEMENT_TYPE_TYPEDBYREF     = 0x16,     // This is a simple type.   

	ELEMENT_TYPE_I              = 0x18,     // native integer size  
	ELEMENT_TYPE_U              = 0x19,     // native unsigned integer size 
	ELEMENT_TYPE_FNPTR          = 0x1B,     // FNPTR <complete sig for the function including calling convention>
	ELEMENT_TYPE_OBJECT         = 0x1C,     // Shortcut for System.Object
	ELEMENT_TYPE_SZARRAY        = 0x1D,     // Shortcut for single dimension zero lower bound array
											// SZARRAY <type>
	ELEMENT_TYPE_MVAR	        = 0x1E,	    // number

	// This is only for binding
	ELEMENT_TYPE_CMOD_REQD      = 0x1F,     // required C modifier : E_T_CMOD_REQD <mdTypeRef/mdTypeDef>
	ELEMENT_TYPE_CMOD_OPT       = 0x20,     // optional C modifier : E_T_CMOD_OPT <mdTypeRef/mdTypeDef>

	// This is for signatures generated internally (which will not be persisted in any way).
	ELEMENT_TYPE_INTERNAL       = 0x21,     // INTERNAL <typehandle>

	// Note that this is the max of base type excluding modifiers   
	ELEMENT_TYPE_MAX            = 0x22,     // first invalid element type   


	ELEMENT_TYPE_MODIFIER       = 0x40, 
	ELEMENT_TYPE_SENTINEL       = 0x01 | ELEMENT_TYPE_MODIFIER, // sentinel for varargs
	ELEMENT_TYPE_PINNED         = 0x05 | ELEMENT_TYPE_MODIFIER,

	// For internal usage only

	ELEMENT_TYPE_TYPE = 0x50,
	ELEMENT_TYPE_TAGGED_OBJECT = 0x51,
	ELEMENT_TYPE_ENUM = 0x55
} CorElementType;

typedef struct IMAGE_COR_VTABLEFIXUP
{
	ULONG       RVA;                    // Offset of v-table array in image.
	USHORT      Count;                  // How many entries at location.
	USHORT      Type;                   // COR_VTABLE_xxx type of entries.
} IMAGE_COR_VTABLEFIXUP;

#endif