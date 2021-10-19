#pragma once
#include <mutex>
#include <thread>
#include <set>
#include "PlatformImplement.h"

#include "Reflect.h"

class CObjectBase
{
    friend class CObjectArray;
public:
    CObjectBase(std::string InName = "")
        : Name(InName)
    {}
    ~CObjectBase() {};

    virtual CMeta* GetClass() = 0;

private:
    std::string Name;
};

#define REFLECT_OBJECT_CLASS() \
public: \
virtual CClass* GetClass() override { return (CClass*)GMetaTable->GetMeta(MetaId); }

class CLASS() CObject : public CObjectBase
{
    REFLECT_GENERATED_BODY()
    REFLECT_OBJECT_CLASS()
public:

};

class CLASS() OTestObject : public CObject
{
    REFLECT_GENERATED_BODY()
    REFLECT_OBJECT_CLASS()

};


template<typename T>
T* Cast(CObject* O)
{
    
    if (reinterpret_cast<CClass*>(GMetaTable->GetMeta(T::MetaId))->CastRange.InRange(O->GetClass()->Id))
        return (T*)O;
    return nullptr;
}

