#pragma once
#include <list>
#include <vector>
#include <memory>
#include <atomic>
#include <cassert>
#include <functional>
#include <unordered_map>
#ifdef CORE_MODULE
#include "CoreApi.h"
#else
#define CORE_API
#endif

#ifdef __REFLECTOR__
#define CLASS(...)     __attribute__((annotate("Class"    __VA_OPT__(",") #__VA_ARGS__)))
#define ENUM(...)      __attribute__((annotate("Enum"     __VA_OPT__(",") #__VA_ARGS__)))
#define PROPERTY(...)  __attribute__((annotate("Property" __VA_OPT__(",") #__VA_ARGS__)))
#define FUNCTION(...)  __attribute__((annotate("Function" __VA_OPT__(",") #__VA_ARGS__)))
#else
#define CLASS(...)
#define ENUM(...)
#define PROPERTY(...)
#define FUNCTION(...)
#endif

#define REFLECT_GENERATED_BODY() \
public: \
static const FClass* GetClass();\
static Uint32 ClassId;


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

struct FClass;


enum EClassFlag :Uint32 {
	kClassNoFlag = 0,
	kHasDefaultConstructorFlagBit = 1 << 0,
	kHasDestructorFlagBit = 1 << 1,
	kUserWritedReflectClassFlagBit = 1 << 2,
};

enum EFunctionFlag :Uint32 {
	kFunctionNoFlag = 0,
	kMemberFlagBit,
	kStaticFlagBit,
};

enum EQualifierFlag :Uint32 {
	kQualifierNoFlag = 0,
	kPointerFlagBit = 1 << 0,
	kReferenceFlagBit = 1 << 1,
	kConstValueFlagBit = 1 << 2,
	kConstPointerFlagBit = 1 << 3,

};
#ifdef CORE_MODULE
// disable warning 4251
#pragma warning(push)
#pragma warning (disable: 4251)
#endif
struct CORE_API FParameter
{
	const FClass* Class{ nullptr };
	STRING_TYPE Name{ "" };
	Uint32 Flag{ kQualifierNoFlag }; // EQualifierFlag
};

struct CORE_API FRetVal
{
	const FClass* Class{ nullptr };
	Uint32 Flag{ kQualifierNoFlag }; // EQualifierFlag
};

struct CORE_API FFunction {
	STRING_TYPE Name{ "" };
	void* Ptr{ nullptr };
	const FClass* OwnerClass{ nullptr }; //  Owner {Class | Struct} TypeDescriptor
	FRetVal Ret;
	std::vector<FParameter> Args;
	Uint32 Flag{ kFunctionNoFlag }; //EFunctionFlag
	bool IsStaticMemberFunction() { return Flag & (kMemberFlagBit | kStaticFlagBit); }
	bool IsMemberFunction() { return Flag & kMemberFlagBit; }
	bool IsStaticFunction() { return Flag & kStaticFlagBit; }
};

struct CORE_API FField
{
	STRING_TYPE Name{ "" };
	const FClass* Class{ nullptr };
	Uint32 Flag{ kQualifierNoFlag }; // EQualifierFlag
	size_t Offset{ 0 };
	size_t Number{ 1 };
	//#ifdef COMPILE_REFLECTOR
	//	STRING_TYPE ClassName {""};
	//#endif


	bool IsNoQualifierType() { return Flag == kQualifierNoFlag; }
	bool IsPointerType() { return Flag & kPointerFlagBit; }
	bool IsReferenceType() { return Flag & kReferenceFlagBit; }
	bool IsConstValueType() { return Flag & kConstValueFlagBit; }
	bool IsConstPointerType() { return Flag & kConstPointerFlagBit; }

	template<typename T>
	T& Ref(void* OwnerBaseAddress) { return *reinterpret_cast<T*>(((Uint8*)OwnerBaseAddress) + Offset); }
	template<typename T>
	T* Ptr(void* OwnerBaseAddress) { return reinterpret_cast<T*>(((Uint8*)OwnerBaseAddress) + Offset); }
	template<typename T>
	const T& CRef(void* OwnerBaseAddress) { return GetRef<T>(OwnerBaseAddress); }
	template<typename T>
	const T* CPtr(void* OwnerBaseAddress) { return GetPtr<T>(OwnerBaseAddress); }
};

// 功能描述对象

struct CORE_API FClass
{
	STRING_TYPE Name{ "" };
	size_t Size{ 0 };
	Uint32 Flag{ kClassNoFlag }; // EClassFlag
	std::vector<FField> Fields;
	std::vector<FFunction> Functions;
	std::vector<STRING_TYPE> Alias;
	std::vector<const FClass*> ParentClasses;
	Uint32 Id{ 0 };
#ifdef COMPILE_REFLECTOR
	std::string DeclaredFile;
	bool IsReflectionDataCollectionCompleted{ false };
	virtual bool IsReflectClass() { return true; }
	virtual bool IsForwardDeclaredClass() { return false; }
#endif // COMPILE_REFLECTOR

	virtual bool IsBuiltInType() { return false; }
	virtual bool IsEnumClass() { return false; }


	bool HasDefaultConstructor() { return Flag & kHasDefaultConstructorFlagBit; }
	bool HasDestructor() { return Flag & kHasDestructorFlagBit; }
	bool IsReflectGeneratedClass() { return !(Flag & kUserWritedReflectClassFlagBit); }


	virtual void* New() { return nullptr; }
	virtual void Delete(void* Object) { }
	virtual void Constructor(void* ConstructedObject) { }
	virtual void Destructor(void* DestructedObject) { }

	virtual void CopyConstructor(void* ConstructedObject, void* CopyedObject) { }
	virtual void MoveConstructor(void* ConstructedObject, void* MoveedObject) { }
	virtual void CopyAssignmentOperator(void* LObject, void* RObject) {}
	virtual void MoveAssignmentOperator(void* LObject, void* RObject) {}
};


#ifdef COMPILE_REFLECTOR
struct FNonReflectClass : public FClass
{
	virtual bool IsReflectClass() { return false; }
};

struct FForwardDeclaredClass : public FClass
{
	bool IsForwardDeclaredClass() { return true; }
};

#endif // COMPILE_REFLECTOR

struct CORE_API FEnumClass : public FClass
{
	virtual ~FEnumClass() {}
	virtual bool IsEnumClass() { return true; }
	virtual const char* ToString(Uint64 In) { return "?*?*?"; };
	std::vector<std::pair<STRING_TYPE, uint64_t>> Options;
};

struct CORE_API FClassTable {
public:
	FClassTable();
	std::unordered_map<std::string, int32_t> NameToId;
	std::vector<FClass*> Classes;
	std::atomic<int32_t> IdCounter{ 1 };
	std::list<std::function<bool()>> DeferredRegisterList;

	static FClassTable& Get();

	FClass* GetClass(const char* ClassName);
	FClass* GetClass(Uint32 ClassId);
	uint32_t RegisterClassToTable(const char* TypeName, FClass* Class);

	/**
	 * must be called after global initialization is complete and before use,
	 * this function will defer registration
	 *
	 * exmaple:
	 * int main(int argc, const char* argv[])
	 * {
	 *     GClassTable->Initialize();
	 *     //Do something ...
	 *     return 0;
	 * }
	**/
	void Initialize();
};

/**
 * can be used after global initialization is complete
**/
#ifndef COMPILE_REFLECTOR
extern CORE_API FClassTable* GClassTable;
#endif

#pragma pack (push,1)

struct CORE_API FVoid
{
	static const FClass* GetClass()
	{
		static_assert(sizeof(FVoid) == 1);
		static std::function<FClass* ()> ClassInitializer = []() -> FClass* {
			static struct FVoidClass : public FClass {
				bool IsBuiltInType()           override { return true; }
				void* New()                    override { return nullptr; }
				void Delete(void* Object)      override { }
				void Constructor(void* Object) override { }
				void Destructor(void* Object)  override { }
			} Class{};
			Class.Name = "FVoid";
			Class.Size = 0;
			Class.Flag = kClassNoFlag;
			return &Class;
		};
		static FClass* VoidClass = ClassInitializer();
		return VoidClass;
	}
	static Uint32 ClassId;
};

#define DEFINE_BUILT_IN_CLASS(VarName, BuiltInType)                                                    \
struct CORE_API F##VarName                                                                             \
{                                                                                                      \
	static const FClass* GetClass()                                                                    \
	{                                                                                                  \
		static_assert(sizeof(F##VarName) == sizeof(BuiltInType));                                      \
		static std::function<FClass* ()> ClassInitializer = []() -> FClass* {                          \
			static struct F##VarNameClass : public FClass {                                            \
				bool IsBuiltInType()                      override { return true; }                    \
				void* New()                               override { return new BuiltInType; }         \
				void Delete(void* Object)                 override { delete (BuiltInType*)Object; }    \
				void Constructor(void* ConstructedObject) override { }                                 \
				void Destructor(void* DestructedObject)   override { }                                 \
			} Class{};                                                                                 \
			Class.Name = "F" #VarName;                                                                 \
			Class.Size = sizeof(BuiltInType);                                                          \
			Class.Flag = kClassNoFlag;                                                                 \
			return &Class;                                                                             \
		};                                                                                             \
		static FClass* VarName##Class = ClassInitializer();                                            \
		return VarName##Class;                                                                         \
	}                                                                                                  \
	static Uint32 ClassId;                                                                             \
	BuiltInType VarName;                                                                               \
};                                                                                                     \



DEFINE_BUILT_IN_CLASS(Bool, bool);
DEFINE_BUILT_IN_CLASS(Int8, char);
DEFINE_BUILT_IN_CLASS(Uint8, unsigned char);
DEFINE_BUILT_IN_CLASS(Int16, short);
DEFINE_BUILT_IN_CLASS(Uint16, unsigned short);
DEFINE_BUILT_IN_CLASS(Int32, int);
DEFINE_BUILT_IN_CLASS(Uint32, unsigned int);
DEFINE_BUILT_IN_CLASS(Int64, long long);
DEFINE_BUILT_IN_CLASS(Uint64, unsigned long long);
DEFINE_BUILT_IN_CLASS(Float, float);
DEFINE_BUILT_IN_CLASS(Double, double);

#undef DEFINE_BUILT_IN_CLASS

#pragma pack(pop)

#ifdef CORE_MODULE
#pragma warning(pop)
#endif

template<typename T>
struct TEnum {

};

template<typename T>
struct TClassAutoRegister {
	TClassAutoRegister()
	{
		const FClass* Class = T::GetClass();
		T::ClassId = FClassTable::Get().RegisterClassToTable(Class->Name, const_cast<FClass*>(Class));
	}
};

//template<typename TA, typename TB>
//bool IsA(TB* B)
//{
//	return B->IsA<TA>();
//}
