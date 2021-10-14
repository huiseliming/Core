#include "Reflect.h"

CMetaTable::CMetaTable(){
    Metas.push_back(nullptr);
}

CMetaTable* GMetaTable = &CMetaTable::Get();

CMetaTable& CMetaTable::Get()
{
    static std::function<CMetaTable* ()> MetaTableInitializer = []() -> CMetaTable* {
    	static CMetaTable MetaTable;
    	return &MetaTable;
    };
    static CMetaTable* MetaTablePtr = MetaTableInitializer();
    return *MetaTablePtr;
}

CMeta* CMetaTable::GetMeta(const char* MetaName)
{
    auto NameToIdIterator = NameToId.find(MetaName);
    if (NameToIdIterator != NameToId.end())
    	return Metas[NameToIdIterator->second];
    return nullptr;
}

CMeta* CMetaTable::GetMeta(Uint32 MetaId)
{
    if (MetaId < Metas.size())
    	return Metas[MetaId];
    return nullptr;
}

uint32_t CMetaTable::RegisterMetaToTable(CMeta* Meta)
{
    assert(Meta != nullptr);
    assert(std::end(NameToId) == NameToId.find(Meta->Name));
    if (Meta->Id == 0) {
        Meta->Id = IdCounter++;
        Metas.push_back(Meta);
    	NameToId.insert(std::make_pair(Meta->Name, Meta->Id));
    }
    else
    {
#ifdef COMPILE_REFLECTOR
        Meta->Alias.push_back(Meta->Name);
#endif
    	NameToId.insert(std::make_pair(Meta->Name, Meta->Id));
    }
    return Meta->Id;
}

void CMetaTable::Initialize()
{
    while (!DeferredRegisterList.empty())
    {
        DeferredRegisterList.front()();
        DeferredRegisterList.pop_front();
    }
}
