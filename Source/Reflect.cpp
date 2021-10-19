#include <algorithm>
#include <map>
#include "Reflect.h"
#ifdef CORE_MODULE
#include "Logger.h"
#include <chrono>
#endif // CORE_MODULE

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
#ifdef CORE_MODULE
    std::chrono::steady_clock::time_point Start = std::chrono::steady_clock::now();
#endif // CORE_MODULE
    // complete deferred register
    while (!DeferredRegisterList.empty())
    {
        DeferredRegisterList.front()();
        DeferredRegisterList.pop_front();
    }

    // runtime calc LLVM-style rtti
    struct FNode {
        FNode() { Struct = nullptr; }
        ~FNode() { std::for_each(Childs.begin(), Childs.end(), [](std::map<CStruct*, FNode*>::reference Ref) { delete Ref.second; }); }
        CStruct* Struct;
        std::map<CStruct*, FNode*> Childs;
    };

    std::map<CStruct*, FNode*> FindMap;
    std::map<CStruct*, FNode*> Root;
    std::list<FNode*> DeferredList;
    std::list<CMeta*> OtherMeta;
    for (size_t i = 1; i < Metas.size(); i++)
    {
        CMeta* Meta = Metas[i];
        CStruct* Struct = dynamic_cast<CStruct*>(Meta);
        if (!Struct)
        {
            OtherMeta.push_back(Meta);
            continue;
        }
        FNode* Node = new FNode();
        Node->Struct = Struct;
        
        if (Struct->Parent == nullptr) 
        {
            Root.insert(std::make_pair(Struct, Node));
        }
        else
        {
            auto Iterator= FindMap.find(Struct->Parent);
            if (Iterator != std::end(FindMap)) Iterator->second->Childs.insert(std::make_pair(Struct, Node));
            else DeferredList.push_back(Node);
        }
        FindMap.insert(std::make_pair(Struct, Node));
    }
    // deferred match
    while(!DeferredList.empty())
    {
        for (auto Iterator = DeferredList.begin(); Iterator != DeferredList.end();)
        {
            auto TargetIterator = FindMap.find((*Iterator)->Struct->Parent);
            if (TargetIterator != std::end(FindMap))
            {
                TargetIterator->second->Childs.insert(std::make_pair((*Iterator)->Struct, (*Iterator)));
                Iterator = DeferredList.erase(Iterator);
            }
            else
            {
                Iterator++;
            }
        }
    }
    // remap id
    std::map<Uint32, Uint32> RemapId;
    Uint32 CurrentMetaId = 1;
    for (auto Iterator = OtherMeta.begin(); Iterator != OtherMeta.end(); Iterator++)
    {
        RemapId.insert(std::make_pair((*Iterator)->Id, CurrentMetaId));
        (*Iterator)->Id = CurrentMetaId;
        Metas[CurrentMetaId] = (*Iterator);
        CurrentMetaId++;
    }

    std::function<void(FNode*)> CalculateCastRange = [&](FNode* Node)
    {
        Node->Struct->CastRange.Begin = CurrentMetaId;
        RemapId.insert(std::make_pair(Node->Struct->Id, CurrentMetaId));
        Node->Struct->Id = CurrentMetaId;
        Metas[CurrentMetaId] = Node->Struct;
        CurrentMetaId++;
        for (auto Iterator = Node->Childs.begin(); Iterator != Node->Childs.end(); Iterator++)
        {
            CalculateCastRange(Iterator->second);
        }
        Node->Struct->CastRange.End = CurrentMetaId;
    };
    std::for_each(Root.begin(), Root.end(), [&](std::map<CStruct*, FNode*>::reference Ref) { 
        CalculateCastRange(Ref.second);; 
        delete Ref.second;
    });

    std::for_each(NameToId.begin(), NameToId.end(), [&] (std::map<std::string, Uint32>::reference Ref) {
        Ref.second = RemapId.find(Ref.second)->second;
    });

    // Initialize StaticMetaId
    while (!StaticMetaIdInitializerList.empty())
    {
        StaticMetaIdInitializerList.front()();
        StaticMetaIdInitializerList.pop_front();
    }
#ifdef CORE_MODULE
    std::chrono::steady_clock::time_point End = std::chrono::steady_clock::now();
    GLogger->Log(ELogLevel::kDebug, "GMetaTable Initialize in {:f} seconds", std::chrono::duration<double>(End - Start).count());
#endif // CORE_MODULE
}