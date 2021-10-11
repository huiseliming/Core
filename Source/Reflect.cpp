#include "Reflect.h"

FClassTable::FClassTable(){
    Classes.push_back(nullptr);
#ifdef COMPILE_REFLECTOR
#define REG_CLS(Name, Class, ClassId)\
    Class->Id = ClassId;\
    Classes.push_back(Class);\
    NameToId.insert(std::make_pair(Name, ClassId));\
    IdCounter++;

#define TPD_CLS(Name, Class, ClassId)\
    NameToId.insert(std::make_pair(Name, ClassId));

    REG_CLS("FVoid", const_cast<FClass*>(FVoid::GetClass())   , 1);
    REG_CLS("FBool", const_cast<FClass*>(FBool::GetClass())   , 2);
    REG_CLS("FInt8", const_cast<FClass*>(FInt8::GetClass())   , 3);
    REG_CLS("FUint8", const_cast<FClass*>(FUint8::GetClass())  ,4);
    REG_CLS("FInt16", const_cast<FClass*>(FInt16::GetClass())  ,5);
    REG_CLS("FUint16", const_cast<FClass*>(FUint16::GetClass()),6);
    REG_CLS("FInt32", const_cast<FClass*>(FInt32::GetClass())  ,7);
    REG_CLS("FUint32", const_cast<FClass*>(FUint32::GetClass()),8);
    REG_CLS("FInt64", const_cast<FClass*>(FInt64::GetClass())  ,9);
    REG_CLS("FUint64", const_cast<FClass*>(FUint64::GetClass()),10);
    REG_CLS("FFloat", const_cast<FClass*>(FFloat::GetClass())  ,11);
    REG_CLS("FDouble", const_cast<FClass*>(FDouble::GetClass()),12);

    TPD_CLS("Void", const_cast<FClass*>(FVoid::GetClass())   , 1);
    TPD_CLS("Bool", const_cast<FClass*>(FBool::GetClass())   , 2);
    TPD_CLS("Int8", const_cast<FClass*>(FInt8::GetClass())   , 3);
    TPD_CLS("Uint8", const_cast<FClass*>(FUint8::GetClass())  ,4);
    TPD_CLS("Int16", const_cast<FClass*>(FInt16::GetClass())  ,5);
    TPD_CLS("Uint16", const_cast<FClass*>(FUint16::GetClass()),6);
    TPD_CLS("Int32", const_cast<FClass*>(FInt32::GetClass())  ,7);
    TPD_CLS("Uint32", const_cast<FClass*>(FUint32::GetClass()),8);
    TPD_CLS("Int64", const_cast<FClass*>(FInt64::GetClass())  ,9);
    TPD_CLS("Uint64", const_cast<FClass*>(FUint64::GetClass()),10);
    TPD_CLS("Float", const_cast<FClass*>(FFloat::GetClass())  ,11);
    TPD_CLS("Double", const_cast<FClass*>(FDouble::GetClass()),12);


    TPD_CLS("void", const_cast<FClass*>(FVoid::GetClass()),1);
    TPD_CLS("bool", const_cast<FClass*>(FBool::GetClass()),2);
    TPD_CLS("char", const_cast<FClass*>(FInt8::GetClass()),3);
    TPD_CLS("unsigned char", const_cast<FClass*>(FUint8::GetClass()),4);
    TPD_CLS("short", const_cast<FClass*>(FInt16::GetClass()),5);
    TPD_CLS("unsigned short", const_cast<FClass*>(FUint16::GetClass()),6);
    TPD_CLS("int", const_cast<FClass*>(FInt32::GetClass()),7);
    TPD_CLS("unsigned int", const_cast<FClass*>(FUint32::GetClass()),8);
    TPD_CLS("long long", const_cast<FClass*>(FInt64::GetClass()),9);
    TPD_CLS("unsigned long long", const_cast<FClass*>(FUint64::GetClass()),10);
    TPD_CLS("float", const_cast<FClass*>(FFloat::GetClass()),11);
    TPD_CLS("double", const_cast<FClass*>(FDouble::GetClass()),12);

    TPD_CLS("_Bool", const_cast<FClass*>(FVoid::GetClass()), 2);
#undef REG_CLS
#undef TPD_CLS
#endif
}

FClassTable* GClassTable = &FClassTable::Get();

FClassTable& FClassTable::Get()
{
    static std::function<FClassTable* ()> ClassTableInitializer = []() -> FClassTable* {
    	static FClassTable ClassTable;
    	return &ClassTable;
    };
    static FClassTable* ClassTablePtr = ClassTableInitializer();
    return *ClassTablePtr;
}

FClass* FClassTable::GetClass(const char* ClassName)
{
    auto NameToIdIterator = NameToId.find(ClassName);
    if (NameToIdIterator != NameToId.end())
    	return Classes[NameToIdIterator->second];
    return nullptr;
}

FClass* FClassTable::GetClass(Uint32 ClassId)
{
    if (ClassId < Classes.size())
    	return Classes[ClassId];
    return nullptr;
}

uint32_t FClassTable::RegisterClassToTable(const char* ClassName, FClass* Class)
{
    assert(Class != nullptr);
    assert(std::end(NameToId) == NameToId.find(ClassName));

    if (Class->Id == 0) {
#ifdef COMPILE_REFLECTOR
        assert(strcmp(Class->Name.c_str(), ClassName) == 0);
#else
        assert(strcmp(Class->Name, ClassName) == 0);
#endif
        Class->Id = IdCounter++;
        Classes.push_back(Class);
    	NameToId.insert(std::make_pair(ClassName, Class->Id));
    }
    else
    {
#ifdef COMPILE_REFLECTOR
        Class->Alias.push_back(ClassName);
#endif
    	NameToId.insert(std::make_pair(ClassName, Class->Id));
    }
    return Class->Id;
}

void FClassTable::Initialize()
{
    while (!DeferredRegisterList.empty())
    {
        DeferredRegisterList.front()();
        DeferredRegisterList.pop_front();
    }
}

Uint32 FVoid::ClassId = 0;
#ifdef COMPILE_REFLECTOR
#define DEFINE_BUILT_IN_CLASS_MEMBER(VarName) Uint32 F##VarName::ClassId = 0;
#else
#define DEFINE_BUILT_IN_CLASS_MEMBER(VarName) \
Uint32 F##VarName::ClassId = 0;\
static TClassAutoRegister<F##VarName> F##VarName##ClassAutoRegister;
#endif

DEFINE_BUILT_IN_CLASS_MEMBER(Bool);
DEFINE_BUILT_IN_CLASS_MEMBER(Int8);
DEFINE_BUILT_IN_CLASS_MEMBER(Uint8);
DEFINE_BUILT_IN_CLASS_MEMBER(Int16);
DEFINE_BUILT_IN_CLASS_MEMBER(Uint16);
DEFINE_BUILT_IN_CLASS_MEMBER(Int32);
DEFINE_BUILT_IN_CLASS_MEMBER(Uint32);
DEFINE_BUILT_IN_CLASS_MEMBER(Int64);
DEFINE_BUILT_IN_CLASS_MEMBER(Uint64);
DEFINE_BUILT_IN_CLASS_MEMBER(Float);
DEFINE_BUILT_IN_CLASS_MEMBER(Double);

#undef DEFINE_BUILT_IN_CLASS_MEMBER
