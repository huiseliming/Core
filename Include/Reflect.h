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
#define CLASS(...)     __attribute__((annotate("Meta" __VA_OPT__(",") #__VA_ARGS__)))
#define STRUCT(...)    __attribute__((annotate("Meta" __VA_OPT__(",") #__VA_ARGS__)))
#define ENUM(...)      __attribute__((annotate("Meta" __VA_OPT__(",") #__VA_ARGS__)))
#define PROPERTY(...)  __attribute__((annotate("Meta" __VA_OPT__(",") #__VA_ARGS__)))
#define FUNCTION(...)  __attribute__((annotate("Meta" __VA_OPT__(",") #__VA_ARGS__)))
#else
#define CLASS(...)
#define STRUCT(...)
#define ENUM(...)
#define PROPERTY(...)
#define FUNCTION(...)
#endif

#define GENERATED_META_BODY()                        \
public:                                              \
static FClass* StaticClass();                        \
static FMeta* StaticMeta();                          \
static UInt32 MetaId;

#ifdef COMPILE_REFLECTOR
#define STRING_TYPE std::string
#else
#define STRING_TYPE const char *
#endif // COMPILE_REFLECTOR

typedef void               Void;
typedef bool               Bool;
typedef char               Int8;
typedef unsigned char      UInt8;
typedef short              Int16;
typedef unsigned short     UInt16;
typedef int                Int32;
typedef unsigned int       UInt32;
typedef long long          Int64;
typedef unsigned long long UInt64;
typedef float              Float;
typedef double             Double;

typedef void* (*FPNew)();
typedef void  (*FPDelete)(void* O);
typedef void  (*FPConstructor)(void* O);
typedef void  (*FPDestructor)(void* O);

struct CClass;
struct FProperty;

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

typedef CRange<UInt32> FUInt32Range;


enum EClassFlag :UInt32 {
	ECF_NoneFlag                = 0x00000000,
	ECF_DefaultConstructorExist = 0x00000001,
	ECF_DefaultDestructorExist  = 0x00000002,
};

enum EFunctionFlag :UInt32 {
	EFF_NoneFlag   = 0x00000000,
	EFF_MemberFlag = 0x00000001,
	EFF_StaticFlag = 0x00000002,
};

enum EPropertyFlag : UInt32
{
	EPF_NoneFlag                 = 0x00000000,

	EPF_BoolFlag                 = 0x00000001,
	EPF_Int8Flag                 = 0x00000002,
	EPF_Int16Flag                = 0x00000004,
	EPF_Int32Flag                = 0x00000008,
	EPF_Int64Flag                = 0x00000010,
	EPF_UInt8Flag                = 0x00000020,
	EPF_UInt16Flag               = 0x00000040,
	EPF_UInt32Flag               = 0x00000080,
	EPF_UInt64Flag               = 0x00000100,
	EPF_FloatFlag                = 0x00000200,
	EPF_DoubleFlag               = 0x00000400,
	EPF_StringFlag               = 0x00000800,
	EPF_ClassFlag                = 0x00001000,
	EPF_EnumFlag			     = 0x00002000,

	EPF_ArrayFlag                = 0x00010000,
	EPF_VectorFlag               = 0x00020000,
	EPF_MapFlag                  = 0x00040000,

	EPF_PointerFlag              = 0x10000000,
	EPF_ReferenceFlag            = 0x20000000,
	EPF_ConstValueFlag           = 0x40000000,
	EPF_ConstPointerFlag         = 0x80000000,

	EPF_IntegerMaskBitFlag       = EPF_Int8Flag | EPF_Int16Flag | EPF_Int32Flag | EPF_Int64Flag | EPF_UInt8Flag | EPF_UInt16Flag | EPF_UInt32Flag | EPF_UInt64Flag,
	EPF_FloatingPointMaskBitFlag = EPF_FloatFlag | EPF_DoubleFlag,
	EPF_QualifierMaskBitFlag     = 0xF0000000,
	EPF_TypeMaskBitFlag          = ~EPF_QualifierMaskBitFlag,
};

#ifdef CORE_MODULE
// disable warning 4251
#pragma warning(push)
#pragma warning (disable: 4251)
#endif

struct CORE_API FMeta
{
#ifdef COMPILE_REFLECTOR
public:
	/// LLVM-style RTTI
	enum EMetaKind {
		EMK_Meta,
		EFK_Interface,
		EFK_Enum,
		EFK_Class,
		EFK_Property,
		EFK_Function,
	};
protected:
	const EMetaKind Kind;

public:
	EMetaKind GetKind() const { return Kind; }
	FMeta(const char* InName, UInt32 InFlag = 0, EMetaKind Kind = EMK_Meta)
		: Name(InName)
		, Flag(InFlag)
		, Kind(Kind)
	{}
#else
	FMeta(const char* InName, UInt32 InFlag = 0)
		: Name(InName)
		, Flag(InFlag)
	{}
#endif
	FMeta(const FMeta&) = delete;
	FMeta& operator=(const FMeta&) = delete;
	FMeta(FMeta&&) = delete;
	FMeta& operator=(FMeta&&) = delete;
	virtual ~FMeta() {}


	UInt32 Id{ UINT32_MAX };
	STRING_TYPE Name;
	UInt32 Flag{ 0 };
	std::vector<STRING_TYPE> Alias;
	std::unordered_map<std::string, STRING_TYPE> MetaData;

	bool HasFlag(UInt32 InFlag)    { return Flag & InFlag; }
	void AddFlag(UInt32 InFlag)    { Flag = Flag | InFlag; }
	void RemoveFlag(UInt32 InFlag) { Flag = Flag & !InFlag; }

#ifdef COMPILE_REFLECTOR
	bool IsRootForCodeGenerator{false};
	std::string DeclaredFile;
	std::string DeclaredFileOnlyNameNoSuffix;
	bool IsReflectionDataCollectionCompleted{ false };
	bool IsForwardDeclared{ false };
#endif // COMPILE_REFLECTOR
};

struct CORE_API FFunction : public FMeta
{
#ifdef COMPILE_REFLECTOR
	FFunction(const char* InName, UInt32 InFlag = ECF_NoneFlag, EMetaKind Kind = EFK_Function)
		: FMeta(InName, InFlag, Kind)
	{}

	static bool classof(const FMeta* F) {
		return F->GetKind() == EFK_Function;
	}
#else
	FFunction(const char* InName)
		: FMeta(InName, ECF_NoneFlag)
	{}
#endif
	void* InvokePtr{ nullptr };
	const CClass* OwnerClass{ nullptr }; //  Owner {Class | Struct} 
};


struct CORE_API FInterface : public FMeta
{
#ifdef COMPILE_REFLECTOR
	FInterface(const char* InName, UInt32 InFlag = ECF_NoneFlag, EMetaKind Kind = EFK_Interface)
		: FMeta(InName, InFlag, Kind)
	{}

	static bool classof(const FMeta* F) {
		return F->GetKind() == EFK_Interface;
	}
#else
	FInterface(const char* InName)
		: FMeta(InName, ECF_NoneFlag)
	{}
#endif
	std::vector<FFunction> Functions;
};

struct CORE_API FClass : public FMeta
{
#ifdef COMPILE_REFLECTOR
	FClass(const char* InName, UInt32 InFlag = ECF_NoneFlag, EMetaKind Kind = EFK_Class)
		: FMeta(InName, InFlag, Kind)
	{
#ifdef COMPILE_REFLECTOR
		IsRootForCodeGenerator = true;
#endif
	}

	static bool classof(const FMeta* F) {
		return F->GetKind() == EFK_Class;
	}
#else
	FClass(const char* InName)
		: FMeta(InName, ECF_NoneFlag)
	{}
#endif
	size_t Size{ 0 };
	std::vector<FProperty*> Properties;
	std::vector<FFunction> Functions;

#ifdef COMPILE_REFLECTOR
	std::string ParentClassName;
	std::vector<std::string> InterfaceNames;
#endif
	FClass* Parent{nullptr};
	std::vector<FInterface*> Interfaces;

	// CAST RANGE [CastStart, CastEnd)
	FUInt32Range CastRange;

	FPNew         New        { nullptr };
	FPDelete      Delete     { nullptr };
	FPConstructor Constructor{ nullptr };
	FPDestructor  Destructor { nullptr };
};

struct CORE_API FEnum : public FMeta
{
#ifdef COMPILE_REFLECTOR
	FEnum(const char* InName, UInt32 InFlag = 0, EMetaKind Kind = EFK_Enum)
		: FMeta(InName, InFlag, Kind)
	{
#ifdef COMPILE_REFLECTOR
		IsRootForCodeGenerator = true;
#endif
	}

	static bool classof(const FMeta* F) {
		return F->GetKind() == EFK_Enum;
	}
#else
	FEnum(const char* InName)
		: FMeta(InName)
	{}
#endif
	UInt32 Size;
#ifdef COMPILE_REFLECTOR
	std::vector<std::pair<STRING_TYPE, uint64_t>> Options;
#else
	std::unordered_map<uint64_t, STRING_TYPE> Options;
#endif
};

struct CORE_API FMetaTable {
public:
	FMetaTable();
	std::unordered_map<std::string, UInt32> NameToId;
	std::vector<FMeta*> Metas;
	std::atomic<UInt32> IdCounter{ 1 };
	std::list<std::function<void()>> DeferredRegisterList;
	std::list<std::function<void()>> StaticMetaIdInitializerList;

	static FMetaTable& Get();

	FMeta* GetMeta(const char* MetaName);
	FMeta* GetMeta(UInt32 MetaId);
	uint32_t RegisterMetaToTable(FMeta* Meta);

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
extern CORE_API FMetaTable* GMetaTable;

template<typename T>
struct TEnumClass {};

template<typename T>
struct TMetaAutoRegister {
	TMetaAutoRegister()
	{
		const FMeta* Meta = T::StaticMeta();
		FMetaTable::Get().RegisterMetaToTable(const_cast<FMeta*>(Meta));
		FMetaTable::Get().StaticMetaIdInitializerList.push_back([Meta] { T::MetaId = Meta->Id; });
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

struct CORE_API FProperty : public FMeta
{
#ifdef COMPILE_REFLECTOR
	FProperty(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1, EMetaKind Kind = EFK_Property)
		: FMeta(InName, InFlag, Kind), Offset(InOffset), Number(InNumber)
	{
	}

	static bool classof(const FMeta* F) {
		return F->GetKind() == EFK_Property;
	}
#else
	FProperty(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: FMeta(InName, InFlag), Offset(InOffset), Number(InNumber)
	{}
#endif

#ifdef COMPILE_REFLECTOR
	std::string MetaDeclaredName;
#endif

	UInt32        Offset{ 0 };
	UInt32        Number{ 1 };

	virtual void* GetPropertyAddressPtr(void* A) const { return OFFSET_VOID_PTR(A, FProperty::Offset); }

	//bool IsSimpleValueType() { return Flag == kQualifierNoFlag; }
	bool IsPointerType() { return Flag & EPF_PointerFlag; }
	bool IsReferenceType() { return Flag & EPF_ReferenceFlag; }
	bool IsConstValueType() { return Flag & EPF_ConstValueFlag; }
	bool IsConstPointerType() { return Flag & EPF_ConstPointerFlag; }

	virtual bool IsFloatingPoint() const { return false; }
	virtual bool IsInteger() const { return false; }

	virtual FMeta* GetMetaPropertyValue() const { return nullptr; }
	virtual FEnum* GetEnumPropertyValue() const { return nullptr; }
	virtual FClass* GetClassPropertyValue() const { return nullptr; }
	virtual FProperty* GetVectorDataProperty() const { return nullptr; }

	virtual Bool GetBoolPropertyValue(void const* Data) const { return false; }
	virtual std::string GetBoolPropertyValueToString(void const* Data) const { return ""; }
	virtual void SetBoolPropertyValue(void* Data, bool Value) const {}

	virtual Int64 GetSignedIntPropertyValue(void const* Data) const { return 0; }
	virtual UInt64 GetUnsignedIntPropertyValue(void const* Data) const { return 0; }
	virtual double GetFloatingPointPropertyValue(void const* Data) const { return 0.f; }
	virtual std::string GeTNumericPropertyValueToString(void const* Data) const { return ""; }
	virtual void SetIntPropertyValue(void* Data, UInt64 Value) const {}
	virtual void SetIntPropertyValue(void* Data, Int64 Value) const {}
	virtual void SetFloatingPointPropertyValue(void* Data, double Value) const {}
	virtual void SetNumericPropertyValueFromString(void* Data, char const* Value) const {}

	virtual std::string GetStringPropertyValue(void const* Data) const { return ""; }
	virtual const char* GetStringPropertyDataPtr(void const* Data) const { return ""; }
	virtual void SetStringPropertyValue(void* Data, const std::string& Value) const {}
	virtual void SetStringPropertyValue(void* Data, const char* Value) const {}
	virtual void SetStringPropertyValue(void* Data, UInt64 Value) const {}
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
struct TNumericProperty : public FProperty
{
	using TPropertyValue = TPropertyValue<CppType>;

	TNumericProperty(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: FProperty(InName, InFlag, InOffset, InNumber)
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
	virtual void SetIntPropertyValue(void* Data, UInt64 Value) const override
	{
		assert(TIsIntegral<CppType>::Value);
		TPropertyValue::SetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset), (CppType)Value);
	}
	virtual void SetIntPropertyValue(void* Data, Int64 Value) const override
	{
		assert(TIsIntegral<CppType>::Value);
		TPropertyValue::SetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset), (CppType)Value);
	}
	virtual void SetFloatingPointPropertyValue(void* Data, double Value) const override
	{
		assert(TIsFloatingPoint<CppType>::Value);
		TPropertyValue::SetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset), (CppType)Value);
	}
	virtual void SetNumericPropertyValueFromString(void* Data, char const* Value) const override
	{
#pragma warning(push)
#pragma warning (disable: 4244)
		* TPropertyValue::GetPropertyValuePtr(OFFSET_VOID_PTR(Data, FProperty::Offset)) = atoll(Value);
#pragma warning(pop)
	}
	virtual std::string GeTNumericPropertyValueToString(void const* Data) const override
	{
		return std::to_string(TPropertyValue::GetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset)));
	}
	virtual Int64 GetSignedIntPropertyValue(void const* Data) const override
	{
		assert(TIsIntegral<CppType>::Value);
		return (Int64)TPropertyValue::GetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset));
	}
	virtual UInt64 GetUnsignedIntPropertyValue(void const* Data) const override
	{
		assert(TIsIntegral<CppType>::Value);
		return (UInt64)TPropertyValue::GetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset));
	}
	virtual double GetFloatingPointPropertyValue(void const* Data) const override
	{
		assert(TIsFloatingPoint<CppType>::Value);
		return (double)TPropertyValue::GetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset));
	}

};


struct FBoolProperty : public FProperty
{
	FBoolProperty(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: FProperty(InName, EPropertyFlag(InFlag | EPF_BoolFlag), InOffset, InNumber)
	{
#ifdef COMPILE_REFLECTOR
		MetaDeclaredName = "Bool";
#endif
	}
	virtual void SetBoolPropertyValue(void* Data, bool Value) const override
	{
		TPropertyValue<Bool>::SetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset), (Bool)Value);
	}
	virtual Bool GetBoolPropertyValue(void const* Data) const  override
	{
		return TPropertyValue<Bool>::GetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset));
	}
	virtual std::string GetBoolPropertyValueToString(void const* Data) const override
	{
		return TPropertyValue<Bool>::GetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset)) ? "True" : "false";
	}
};

struct FInt8Property : TNumericProperty<Int8>
{
	FInt8Property(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: TNumericProperty<Int8>(InName, EPropertyFlag(InFlag | EPF_Int8Flag), InOffset, InNumber)
	{
#ifdef COMPILE_REFLECTOR
		MetaDeclaredName = "Int8";
#endif
	}
};

struct FInt16Property : TNumericProperty<Int16>
{
	FInt16Property(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: TNumericProperty<Int16>(InName, EPropertyFlag(InFlag | EPF_Int16Flag), InOffset, InNumber)
	{
#ifdef COMPILE_REFLECTOR
		MetaDeclaredName = "Int16";
#endif
	}
};

struct FInt32Property : TNumericProperty<Int32>
{
	FInt32Property(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: TNumericProperty<Int32>(InName, EPropertyFlag(InFlag | EPF_Int32Flag), InOffset, InNumber)
	{
#ifdef COMPILE_REFLECTOR
		MetaDeclaredName = "Int32";
#endif
	}
};

struct FInt64Property : TNumericProperty<Int64>
{
	FInt64Property(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: TNumericProperty<Int64>(InName, EPropertyFlag(InFlag | EPF_Int64Flag), InOffset, InNumber)
	{
#ifdef COMPILE_REFLECTOR
		MetaDeclaredName = "Int64";
#endif
	}
};

struct FUInt8Property : TNumericProperty<UInt8>
{
	FUInt8Property(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: TNumericProperty<UInt8>(InName, EPropertyFlag(InFlag | EPF_UInt8Flag), InOffset, InNumber)
	{
#ifdef COMPILE_REFLECTOR
		MetaDeclaredName = "UInt8";
#endif
	}
};

struct FUInt16Property : TNumericProperty<UInt16>
{
	FUInt16Property(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: TNumericProperty<UInt16>(InName, EPropertyFlag(InFlag | EPF_UInt16Flag), InOffset, InNumber)
	{
#ifdef COMPILE_REFLECTOR
		MetaDeclaredName = "UInt16";
#endif
	}
};

struct FUInt32Property : TNumericProperty<UInt32>
{
	FUInt32Property(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: TNumericProperty<UInt32>(InName, EPropertyFlag(InFlag | EPF_UInt32Flag), InOffset, InNumber)
	{
#ifdef COMPILE_REFLECTOR
		MetaDeclaredName = "UInt32";
#endif
	}
};

struct FUInt64Property : TNumericProperty<UInt64>
{
	FUInt64Property(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: TNumericProperty<UInt64>(InName, EPropertyFlag(InFlag | EPF_UInt64Flag), InOffset, InNumber)
	{
#ifdef COMPILE_REFLECTOR
		MetaDeclaredName = "UInt64";
#endif
	}
};

struct FFloatProperty : TNumericProperty<Float>
{
	FFloatProperty(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: TNumericProperty<Float>(InName, EPropertyFlag(InFlag | EPF_FloatFlag), InOffset, InNumber)
	{
#ifdef COMPILE_REFLECTOR
		MetaDeclaredName = "Float";
#endif
	}
};

struct FDoubleProperty : TNumericProperty<Double>
{
	FDoubleProperty(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: TNumericProperty<Double>(InName, EPropertyFlag(InFlag | EPF_DoubleFlag), InOffset, InNumber)
	{
#ifdef COMPILE_REFLECTOR
		MetaDeclaredName = "Double";
#endif
	}
};

struct FStringProperty : public FProperty
{
	FStringProperty(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: FProperty(InName, EPropertyFlag(InFlag | EPF_StringFlag), InOffset, InNumber)
	{
#ifdef COMPILE_REFLECTOR
		MetaDeclaredName = "std::string";
#endif
	}

	virtual std::string GetStringPropertyValue(void const* Data) const override
	{
		return TPropertyValue<std::string>::GetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset));
	}

	virtual const char* GetStringPropertyDataPtr(void const* Data) const override
	{
		return TPropertyValue<std::string>::GetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset)).c_str();
	}

	virtual void SetStringPropertyValue(void* Data, const std::string& Value) const override
	{
		TPropertyValue<std::string>::SetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset), Value);
	}

	virtual void SetStringPropertyValue(void* Data, const char* Value) const override
	{
		*TPropertyValue<std::string>::GetPropertyValuePtr(OFFSET_VOID_PTR(Data, FProperty::Offset)) = Value;
	}

	virtual void SetStringPropertyValue(void* Data, UInt64 Value) const override
	{
		TPropertyValue<std::string>::SetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset), std::to_string(Value));
	}

	virtual void SetStringPropertyValue(void* Data, Int64 Value) const override
	{
		TPropertyValue<std::string>::SetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset), std::to_string(Value));
	}

	virtual void SetStringPropertyValue(void* Data, double Value) const override
	{
		TPropertyValue<std::string>::SetPropertyValue(OFFSET_VOID_PTR(Data, FProperty::Offset), std::to_string(Value));
	}


};

struct FMetaProperty : public FProperty
{
protected:
	FMetaProperty(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: FProperty(InName, InFlag, InOffset, InNumber)
		, Meta(nullptr)
	{}
public:
	FMeta* Meta;
	virtual FMeta* GetMetaPropertyValue() const { return Meta; }
};

struct FClassProperty : public FMetaProperty
{
	FClassProperty(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: FMetaProperty(InName, EPropertyFlag(InFlag | EPF_ClassFlag), InOffset, InNumber)
	{}
	virtual FClass* GetClassPropertyValue() const override { return reinterpret_cast<FClass*>(Meta); }
};

struct FEnumProperty : public FMetaProperty
{
	FEnumProperty(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: FMetaProperty(InName, EPropertyFlag(InFlag | EPF_EnumFlag), InOffset, InNumber)
	{}

	virtual FEnum* GetEnumPropertyValue() const override { return reinterpret_cast<FEnum*>(Meta); }
};

struct FContainerProperty : public FProperty
{
	FContainerProperty(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: FProperty(InName, EPropertyFlag(InFlag), InOffset, InNumber)
	{}
};

struct FVectorProperty : public FContainerProperty
{
	FVectorProperty(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: FContainerProperty(InName, EPropertyFlag(InFlag | EPF_VectorFlag), InOffset, InNumber)
	{}

	virtual void* Data(void const* InBaseAddress, UInt32 Offset = 0) { return nullptr; }
	virtual void PushBack(void const* InBaseAddress, void* InData) {}
	virtual UInt32 Size(void const* InBaseAddress) { return 0; }

	virtual void Reserve(void const* InBaseAddress, UInt32 InSize) {}
	virtual void Resize(void const* InBaseAddress, UInt32 InSize) {}
	virtual void Remove(void const* InBaseAddress, UInt32 InIndex) {}

	virtual FProperty* GetVectorDataProperty() const override { return reinterpret_cast<FProperty*>(DataProperty); }
	FProperty* DataProperty;
};

template<typename T>
struct TVectorProperty : public FVectorProperty
{
	TVectorProperty(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
		: FVectorProperty(InName, EPropertyFlag(InFlag), InOffset, InNumber)
	{}
	
	virtual void* Data(void const* InBaseAddress, UInt32 Offset = 0) override
	{
		std::vector<T>* VectorPtr = (std::vector<T>*)OFFSET_VOID_PTR(InBaseAddress, FProperty::Offset);
		return ((T*)(VectorPtr->data())) + Offset;
	}

	virtual void PushBack(void const* InBaseAddress, void* InData) override
	{
		std::vector<T>* VectorPtr = (std::vector<T>*)OFFSET_VOID_PTR(InBaseAddress, FProperty::Offset);
		T* Data = (T*)InData;
		VectorPtr->push_back(*Data);
	}

	virtual UInt32 Size(void const* InBaseAddress) override
	{
		std::vector<T>* VectorPtr = (std::vector<T>*)OFFSET_VOID_PTR(InBaseAddress, FProperty::Offset);
		return VectorPtr->size();
	}

	virtual void Reserve(void const* InBaseAddress, UInt32 InSize) override
	{
		std::vector<T>* VectorPtr = (std::vector<T>*)OFFSET_VOID_PTR(InBaseAddress, FProperty::Offset);
		VectorPtr->reserve(InSize);
	}

	virtual void Resize(void const* InBaseAddress, UInt32 InSize) override
	{
		std::vector<T>* VectorPtr = (std::vector<T>*)OFFSET_VOID_PTR(InBaseAddress, FProperty::Offset);
		VectorPtr->resize(InSize);
	}

	virtual void Remove(void const* InBaseAddress, UInt32 InIndex) override 
	{
		std::vector<T>* VectorPtr = (std::vector<T>*)OFFSET_VOID_PTR(InBaseAddress, FProperty::Offset);
		VectorPtr->erase(VectorPtr->begin() + InIndex);
	}
};

//
//template<typename Key, typename Value, typename Map = std::unordered_map<Key, Value>>
//struct TMapProperty : public FContainerProperty
//{
//	TMapProperty(const char* InName = "", EPropertyFlag InFlag = EPF_NoneFlag, UInt32 InOffset = 0, UInt32 InNumber = 1)
//		: FContainerProperty(InName, EPropertyFlag(InFlag | EPF_MapFlag), InOffset, InNumber)
//	{}
//
//	void* GetKeyDataPtr(void const* InBaseAddress)
//	{
//		Map* VectorPtr = (Map*)InBaseAddress;
//		return VectorPtr->data();
//	}
//
//	void* GetValueDataPtr(void const* InBaseAddress, void* KeyType)
//	{
//		Map* VectorPtr = (Map*)InBaseAddress;
//		return VectorPtr.find();
//	}
//};

#ifdef CORE_MODULE
#pragma warning(pop)
#endif

#undef OFFSET_VOID_PTR
