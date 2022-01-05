#pragma once
#include "Reflect.h"

#define GENERATED_META_OBJECT_BODY() \
GENERATED_META_BODY()                \
virtual FClass* GetClass() override { return StaticClass(); };


class CORE_API FMetaObject
{
public:
	virtual ~FMetaObject() = default;
	virtual FClass* GetClass() { return nullptr; };
};

class CORE_API MObject : public FMetaObject
{
};
