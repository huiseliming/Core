#pragma once
#include <list>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <cassert>
#include <functional>
#include <unordered_map>
#include "Traits.h"
#ifdef COMPILE_REFLECTOR
#include "llvm/Support/Casting.h"
#endif

#ifdef CORE_MODULE
#include "CoreApi.h"
#else
#define CORE_API
#endif

#ifdef __REFLECTOR__
#define CLASS(...)     __attribute__((annotate("Class"    __VA_OPT__(",") #__VA_ARGS__)))
#define STRUCT(...)    __attribute__((annotate("Struct"   __VA_OPT__(",") #__VA_ARGS__)))
#define ENUM(...)      __attribute__((annotate("Enum"     __VA_OPT__(",") #__VA_ARGS__)))
#define PROPERTY(...)  __attribute__((annotate("Property" __VA_OPT__(",") #__VA_ARGS__)))
#define FUNCTION(...)  __attribute__((annotate("Function" __VA_OPT__(",") #__VA_ARGS__)))
#else
#define CLASS(...)
#define STRUCT(...)
#define ENUM(...)
#define PROPERTY(...)
#define FUNCTION(...)
#endif

#define REFLECT_GENERATED_BODY() \
public:                          \
static const CMeta* StaticMeta();\
static Uint32 MetaId;


#ifdef COMPILE_REFLECTOR
#define STRING_TYPE std::string
#else
#define STRING_TYPE const char *
#endif // COMPILE_REFLECTOR

typedef void               Void;
typedef bool               Bool;
typedef char               Int8;
typedef unsigned char      Uint8;
typedef short              Int16;
typedef unsigned short     Uint16;
typedef int                Int32;
typedef unsigned int       Uint32;
typedef long long          Int64;
typedef unsigned long long Uint64;
typedef float              Float;
typedef double             Double;

typedef void* (*FPNew)();
typedef void  (*FPDelete)(void* O);
typedef void  (*FPConstructor)(void* O);
typedef void  (*FPDestructor)(void* O);

struct CClass;
struct CProperty;

// [Begin, End)
template<typename T>
struct CRange 
{
	T Begin;
	T End;
	bool InRange(T In)
	{
		return Begin <= In && In < End;
	}
};

typedef CRange<Uint32> CUint32Range;


enum EClassFlag :Uint32 {
	ECF_NoneFlag                = 0x00000000,
	ECF_DefaultConstructorExist = 0x00000001,
	ECF_DefaultDestructorExist  = 0x00000002,
};

enum EFunctionFlag :Uint32 {
	EFF_NoneFlag   = 0x00000000,
	EFF_MemberFlag = 0x00000001,
	EFF_StaticFlag = 0x00000002,
};

enum EPropertyFlag : Uint32
{
	EPF_NoneFlag                 = 0x00000000,

	EPF_BoolFlag                 = 0x00000001,
	EPF_Int8Flag                 = 0x00000002,
	EPF_Int16Flag                = 0x00000004,
	EPF_Int32Flag                = 0x00000008,
	EPF_Int64Flag                = 0x00000010,
	EPF_Uint8Flag                = 0x00000020,
	EPF_Uint16Flag               = 0x00000040,
	EPF_Uint32Flag               = 0x00000080,
	EPF_Uint64Flag               = 0x00000100,
	EPF_FloatFlag                = 0x00000200,
	EPF_DoubleFlag               = 0x00000400,
	EPF_StringFlag               = 0x00000800,
	EPF_StructFlag               = 0x00001000,
	EPF_ClassFlag                = 0x00002000,
	EPF_EnumFlag			     = 0x00004000,

	EPF_ArrayFlag                = 0x00004000,
	EPF_MapFlag                  = 0x00008000,

	EPF_PointerFlag              = 0x10000000,
	EPF_ReferenceFlag            = 0x20000000,
	EPF_ConstValueFlag           = 0x40000000,
	EPF_ConstPointerceFlag       = 0x80000000,

	EPF_IntegerMaskBitFlag       = EPF_Int8Flag | EPF_Int16Flag | EPF_Int32Flag | EPF_Int64Flag | EPF_Uint8Flag | EPF_Uint16Flag | EPF_Uint32Flag | EPF_Uint64Flag,
	EPF_FloatingPointMaskBitFlag = EPF_FloatFlag | EPF_DoubleFlag,
	EPF_TypeMaskBitFlag          = 0x0000FFFF,
	EPF_QualifierMaskBitFlag     = 0xF0000000,
};

#ifdef CORE_MODULE
// disable warning 4251
#pragma warning(push)
#pragma warning (disable: 4251)
#endif

struct CORE_API FFunction 
{
	STRING_TYPE Name{ "" };
	void* Ptr{ nullptr };
	const CClass* OwnerClass{ nullptr }; //  Owner {Class | Struct} TypeDescriptor
	Uint32 Flag{ EFF_NoneFlag }; //EFunctionFlag
};

struct CORE_API CMeta
{
#ifdef COMPILE_REFLECTOR
public:
	/// LLVM-style RTTI
	enum EMetaKind {
		EMK_Meta,
		EFK_Interface,
		EFK_EnumClass,
		EFK_Struct,
		EFK_Class,
	};
protected:
	const EMetaKind Kind;

public:
	EMetaKind GetKind() const { return Kind; }
	CMeta(const char* InName, Uint32 InFlag = 0, EMetaKind Kind = EMK_Meta)
		: Name(InName)
		, Flag(InFlag)
		, Kind(Kind)
	{}
#else
	CMeta(const char* InName, Uint32 InFlag = 0)
		: Name(InName)
		, Flag(InFlag)
	{}
#endif
	CMeta(const CMeta&) = delete;
	CMeta& operator=(const CMeta&) = delete;
	CMeta(CMeta&&) = delete;
	CMeta& operator=(CMeta&&) = delete;
	virtual ~CMeta() {}

	Uint32 Id{ 0 };
	STRING_TYPE Name;
	Uint32 Flag{ 0 };
	std::vector<STRING_TYPE> Alias;
	std::unordered_map<std::string, STRING_TYPE> Data;

	bool HasFlag(Uint32 InFlag)    { return Flag & InFlag; }
	void AddFlag(Uint32 InFlag)    { Flag = Flag | InFlag; }
	void RemoveFlag(Uint32 InFlag) { Flag = Flag & !InFlag; }

#ifdef COMPILE_REFLECTOR
	std::string DeclaredFile;
	bool IsReflectionDataCollectionCompleted{ false };
	bool IsForwardDeclared{ false };
#endif // COMPILE_REFLECTOR
};

struct CORE_API CInterface : public CMeta
{
#ifdef COMPILE_REFLECTOR
	CInterface(const char* InName, Uint32 InFlag = ECF_NoneFlag, EMetaKind Kind = EFK_Interface)
		: CMeta(InName, InFlag, Kind)
	{}

	static bool classof(const CMeta* F) {
		return F->GetKind() == EFK_Interface;
	}
#else
	CInterface(const char* InName)
		: CMeta(InName, ECF_NoneFlag)
	{}
#endif
	std::vector<FFunction> Functions;
};

struct CORE_API CStruct : public CMeta
{
#ifdef COMPILE_REFLECTOR
	CStruct(const char* InName, Uint32 InFlag = ECF_NoneFlag, EMetaKind Kind = EFK_Struct)
		: CMeta(InName, InFlag, Kind)
	{}

	static bool classof(const CMeta* F) {
		return F->GetKind() >= EFK_Struct && F->GetKind() <= EFK_Class;
	}
#else
	CStruct(const char* InName)
		: CMeta(InName, ECF_NoneFlag)
	{}
#endif
	size_t Size{ 0 };
	std::vector<std::unique_ptr<CProperty>> Properties;
	CStruct* Parent{nullptr};
	std::vector<const CInterface*> Interfaces;

	// CAST RANGE [CastStart, CastEnd)
	CUint32Range CastRange;

	FPNew         New        { nullptr };
	FPDelete      Delete     { nullptr };
	FPConstructor Constructor{ nullptr };
	FPDestructor  Destructor { nullptr };
};

struct CORE_API CClass : public CStruct
{
#ifdef COMPILE_REFLECTOR
	CClass(const char* InName, Uint32 InFlag = ECF_NoneFlag, EMetaKind Kind = EFK_Class)
		: CStruct(InName, InFlag, Kind)
	{}
	static bool classof(const CMeta* F) {
		return F->GetKind() == EFK_Class;
	}

#else
	CClass(const char* InName)
		: CStruct(InName)
	{}
#endif
	std::vector<FFunction> Functions;
};

struct CORE_API CEnumClass : public CMeta
{
#ifdef COMPILE_REFLECTOR
	CEnumClass(const char* InName, Uint32 InFlag = 0, EMetaKind Kind = EFK_EnumClass)
		: CMeta(InName, InFlag, Kind)
	{}

	static bool classof(const CMeta* F) {
		return F->GetKind() == EFK_EnumClass;
	}
#else
	CEnumClass(const char* InName)
		: CMeta(InName)
	{}
#endif
	Uint32 Size;
	std::vector<std::pair<STRING_TYPE, uint64_t>> Options;
};

struct CORE_API CMetaTable {
public:
	CMetaTable();
	std::unordered_map<std::string, Uint32> NameToId;
	std::vector<CMeta*> Metas;
	std::atomic<Uint32> IdCounter{ 1 };
	std::list<std::function<void()>> DeferredRegisterList;
	std::list<std::function<void()>> StaticMetaIdInitializerList;

	static CMetaTable& Get();

	CMeta* GetMeta(const char* MetaName);
	CMeta* GetMeta(Uint32 MetaId);
	uint32_t RegisterMetaToTable(CMeta* Meta);

	/**
	 * must be called after global initialization is complete and before use,
	 * this function will defer registration
	 *
	 * exmaple:
	 * int main(int argc, const char* argv[])
	 * {
	 *     GMetaTable->Initialize();
	 *     //Do something ...
	 *     return 0;
	 * }
	**/
	void Initialize();
};

/**
 * can be used after global initialization is complete
**/
extern CORE_API CMetaTable* GMetaTable;

template<typename T>
struct TEnumClass {};

template<typename T>
struct TMetaAutoRegister {
	TMetaAutoRegister()
	{
		const CMeta* Meta = T::StaticMeta();
		CMetaTable::Get().RegisterMetaToTable(const_cast<CMeta*>(Meta));
		CMetaTable::Get().StaticMetaIdInitializerList.push_back([Meta] { T::MetaId = Meta->Id; });
	}
};

template<typename T>
struct TLifeCycle {
	static void* New() { return new T(); }
	static void Delete(void* Ptr) { delete (T*)Ptr; }
	static void Constructor(void* Ptr) { new (Ptr) T(); }
	static void Destructor(void* Ptr) { ((T const*)(Ptr))->~T(); }
};

#define OFFSET_VOID_PTR(PVoid,Offset) (void*)(((char*)(PVoid)) + Offset)

struct CORE_API CProperty : public CMeta
{
	CProperty(const char* InName, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: CMeta(InName, InFlag), Offset(InOffset), Number(InNumber)
	{}

	Uint32        Offset{ 0 };
	Uint32        Number{ 1 };

	virtual void* GetPropertyAddressPtr(void* A) const { return OFFSET_VOID_PTR(A, CProperty::Offset); }

	//bool IsSimpleValueType() { return Flag == kQualifierNoFlag; }
	bool IsPointerType() { return Flag & EPF_PointerFlag; }
	bool IsReferenceType() { return Flag & EPF_ReferenceFlag; }
	bool IsConstValueType() { return Flag & EPF_ConstValueFlag; }
	bool IsConstPointerType() { return Flag & EPF_ConstPointerceFlag; }

	virtual bool IsFloatingPoint() const { return false; }
	virtual bool IsInteger() const { return false; }

	virtual CMeta* GetMetaPropertyMeta() const { return nullptr; }

	virtual Bool GetBoolPropertyValue(void const* Data) const { return false; }
	virtual std::string GetBoolPropertyValueToString(void const* Data) const { return ""; }
	virtual void SetBoolPropertyValue(void* Data, bool Value) const {}

	virtual Int64 GetSignedIntPropertyValue(void const* Data) const { return 0; }
	virtual Uint64 GetUnsignedIntPropertyValue(void const* Data) const { return 0; }
	virtual double GetFloatingPointPropertyValue(void const* Data) const { return 0.f; }
	virtual std::string GetNumericPropertyValueToString(void const* Data) const { return ""; }
	virtual void SetIntPropertyValue(void* Data, Uint64 Value) const {}
	virtual void SetIntPropertyValue(void* Data, Int64 Value) const {}
	virtual void SetFloatingPointPropertyValue(void* Data, double Value) const {}
	virtual void SetNumericPropertyValueFromString(void* Data, char const* Value) const {}

	virtual std::string GetStringPropertyValue(void const* Data) const { return ""; }
	virtual const char* GetStringPropertyDataPtr(void const* Data) const { return ""; }
	virtual void SetStringPropertyValue(void* Data, std::string& Value) const {}
	virtual void SetStringPropertyValue(void* Data, const char* Value) const {}
	virtual void SetStringPropertyValue(void* Data, Uint64 Value) const {}
	virtual void SetStringPropertyValue(void* Data, Int64 Value) const {}
	virtual void SetStringPropertyValue(void* Data, double Value) const {}
};

template<typename CppType>
struct TPropertyValue
{
	enum
	{
		CPPSize = sizeof(CppType),
		CPPAlignment = alignof(CppType)
	};

	/** Convert the address of a value of the property to the proper type */
	static CppType const* GetPropertyValuePtr(void const* A)
	{
		return (CppType const*)(A);
	}
	/** Convert the address of a value of the property to the proper type */
	static CppType* GetPropertyValuePtr(void* A)
	{
		return (CppType*)(A);
	}
	/** Get the value of the property from an address */
	static CppType const& GetPropertyValue(void const* A)
	{
		return *GetPropertyValuePtr(A);
	}
	/** Get the default value of the cpp type, just the default constructor, which works even for things like in32 */
	static CppType GetDefaultPropertyValue()
	{
		return CppType();
	}
	/** Get the value of the property from an address, unless it is NULL, then return the default value */
	static CppType GetOptionalPropertyValue(void const* B)
	{
		return B ? GetPropertyValue(B) : GetDefaultPropertyValue();
	}
	/** Set the value of a property at an address */
	static void SetPropertyValue(void* A, CppType const& Value)
	{
		*GetPropertyValuePtr(A) = Value;
	}
	/** Initialize the value of a property at an address, this assumes over uninitialized memory */
	static CppType* InitializePropertyValue(void* A)
	{
		return new (A) CppType();
	}
	/** Destroy the value of a property at an address */
	static void DestroyPropertyValue(void* A)
	{
		GetPropertyValuePtr(A)->~CppType();
	}
};


template <typename CppType>
struct TNumericProperty : public CProperty
{
	using TPropertyValue = TPropertyValue<CppType>;

	TNumericProperty(const char* InName, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: CProperty(InName, InFlag, InOffset, InNumber)
	{
	}

	virtual bool IsFloatingPoint() const override
	{
		return TIsFloatingPoint<CppType>::Value;
	}
	virtual bool IsInteger() const override
	{
		return TIsIntegral<CppType>::Value;
	}
	virtual void SetIntPropertyValue(void* Data, Uint64 Value) const override
	{
		assert(TIsIntegral<CppType>::Value);
		TPropertyValue::SetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset), (CppType)Value);
	}
	virtual void SetIntPropertyValue(void* Data, Int64 Value) const override
	{
		assert(TIsIntegral<CppType>::Value);
		TPropertyValue::SetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset), (CppType)Value);
	}
	virtual void SetFloatingPointPropertyValue(void* Data, double Value) const override
	{
		assert(TIsFloatingPoint<CppType>::Value);
		TPropertyValue::SetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset), (CppType)Value);
	}
	virtual void SetNumericPropertyValueFromString(void* Data, char const* Value) const override
	{
#pragma warning(push)
#pragma warning (disable: 4244)
		* TPropertyValue::GetPropertyValuePtr(OFFSET_VOID_PTR(Data, CProperty::Offset)) = atoll(Value);
#pragma warning(pop)
	}
	virtual std::string GetNumericPropertyValueToString(void const* Data) const override
	{
		return std::to_string(TPropertyValue::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset)));
	}
	virtual Int64 GetSignedIntPropertyValue(void const* Data) const override
	{
		assert(TIsIntegral<CppType>::Value);
		return (Int64)TPropertyValue::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset));
	}
	virtual Uint64 GetUnsignedIntPropertyValue(void const* Data) const override
	{
		assert(TIsIntegral<CppType>::Value);
		return (Uint64)TPropertyValue::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset));
	}
	virtual double GetFloatingPointPropertyValue(void const* Data) const override
	{
		assert(TIsFloatingPoint<CppType>::Value);
		return (double)TPropertyValue::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset));
	}

};


struct CBoolProperty : public CProperty
{
	CBoolProperty(const char* InName, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: CProperty(InName, EPropertyFlag(InFlag | EPF_BoolFlag), InOffset, InNumber)
	{}
	virtual void SetBoolPropertyValue(void* Data, bool Value) const override
	{
		TPropertyValue<Bool>::SetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset), (Bool)Value);
	}
	virtual Bool GetBoolPropertyValue(void const* Data) const  override
	{
		return TPropertyValue<Bool>::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset));
	}
	virtual std::string GetBoolPropertyValueToString(void const* Data) const override
	{
		return TPropertyValue<Bool>::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset)) ? "True" : "false";
	}
};

struct CInt8Property : TNumericProperty<Int8>
{
	CInt8Property(const char* InName, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: TNumericProperty<Int8>(InName, EPropertyFlag(InFlag | EPF_Int8Flag), InOffset, InNumber)
	{}
};

struct CInt16Property : TNumericProperty<Int16>
{
	CInt16Property(const char* InName, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: TNumericProperty<Int16>(InName, EPropertyFlag(InFlag | EPF_Int16Flag), InOffset, InNumber)
	{}
};

struct CInt32Property : TNumericProperty<Int32>
{
	CInt32Property(const char* InName, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: TNumericProperty<Int32>(InName, EPropertyFlag(InFlag | EPF_Int32Flag), InOffset, InNumber)
	{}
};

struct CInt64Property : TNumericProperty<Int64>
{
	CInt64Property(const char* InName, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: TNumericProperty<Int64>(InName, EPropertyFlag(InFlag | EPF_Int64Flag), InOffset, InNumber)
	{}
};

struct CUint8Property : TNumericProperty<Uint8>
{
	CUint8Property(const char* InName, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: TNumericProperty<Uint8>(InName, EPropertyFlag(InFlag | EPF_Uint8Flag), InOffset, InNumber)
	{}
};

struct CUint16Property : TNumericProperty<Uint16>
{
	CUint16Property(const char* InName, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: TNumericProperty<Uint16>(InName, EPropertyFlag(InFlag | EPF_Uint16Flag), InOffset, InNumber)
	{}
};

struct CUint32Property : TNumericProperty<Uint32>
{
	CUint32Property(const char* InName, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: TNumericProperty<Uint32>(InName, EPropertyFlag(InFlag | EPF_Uint32Flag), InOffset, InNumber)
	{}
};

struct CUint64Property : TNumericProperty<Uint64>
{
	CUint64Property(const char* InName, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: TNumericProperty<Uint64>(InName, EPropertyFlag(InFlag | EPF_Uint64Flag), InOffset, InNumber)
	{}
};

struct CFloatProperty : TNumericProperty<Float>
{
	CFloatProperty(const char* InName, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: TNumericProperty<Float>(InName, EPropertyFlag(InFlag | EPF_FloatFlag), InOffset, InNumber)
	{}
};

struct CDoubleProperty : TNumericProperty<Double>
{
	CDoubleProperty(const char* InName, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: TNumericProperty<Double>(InName, EPropertyFlag(InFlag | EPF_DoubleFlag), InOffset, InNumber)
	{}
};

struct CStringProperty : public CProperty
{
	CStringProperty(const char* InName, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: CProperty(InName, EPropertyFlag(InFlag | EPF_StringFlag), InOffset, InNumber)
	{}

	virtual std::string GetStringPropertyValue(void const* Data) const override
	{
		return TPropertyValue<std::string>::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset));
	}

	virtual const char* GetStringPropertyDataPtr(void const* Data) const override
	{
		return TPropertyValue<std::string>::GetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset)).c_str();
	}

	virtual void SetStringPropertyValue(void* Data, std::string& Value) const override
	{
		TPropertyValue<std::string>::SetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset), Value);
	}

	virtual void SetStringPropertyValue(void* Data, const char* Value) const override
	{
		*TPropertyValue<std::string>::GetPropertyValuePtr(OFFSET_VOID_PTR(Data, CProperty::Offset)) = Value;
	}

	virtual void SetStringPropertyValue(void* Data, Uint64 Value) const
	{
		TPropertyValue<std::string>::SetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset), std::to_string(Value));
	}

	virtual void SetStringPropertyValue(void* Data, Int64 Value) const
	{
		TPropertyValue<std::string>::SetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset), std::to_string(Value));
	}

	virtual void SetStringPropertyValue(void* Data, double Value) const
	{
		TPropertyValue<std::string>::SetPropertyValue(OFFSET_VOID_PTR(Data, CProperty::Offset), std::to_string(Value));
	}


};

struct CMetaProperty : public CProperty
{
protected:
	CMetaProperty(const char* InName, CMeta* InMeta, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: CProperty(InName, InFlag, InOffset, InNumber)
		, Meta(InMeta)
	{}
public:
	CMeta* Meta;

	virtual CMeta* GetMetaPropertyMeta() const { return Meta; }
};

struct CStructProperty : public CMetaProperty
{
	CStructProperty(const char* InName, CStruct* InStruct, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: CMetaProperty(InName, InStruct, EPropertyFlag(InFlag | EPF_StructFlag), InOffset, InNumber)
	{}
};

struct CClassProperty : public CMetaProperty
{
	CClassProperty(const char* InName, CClass* InClass, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: CMetaProperty(InName, InClass, EPropertyFlag(InFlag | EPF_ClassFlag), InOffset, InNumber)
	{}
};

struct CEnumProperty : public CMetaProperty
{
	CEnumProperty(const char* InName, CEnumClass* InEnum, EPropertyFlag InFlag, Uint32 InOffset, Uint32 InNumber = 1)
		: CMetaProperty(InName, InEnum, EPropertyFlag(InFlag | EPF_EnumFlag), InOffset, InNumber)
	{}
};


#ifdef CORE_MODULE
#pragma warning(pop)
#endif

#undef OFFSET_VOID_PTR
