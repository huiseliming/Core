#include <algorithm>
#include <map>
#include "Reflect.h"
#ifdef CORE_MODULE
#include "Logger.h"
#include <chrono>
#endif // CORE_MODULE

FMetaTable::FMetaTable() {
	Metas.push_back(nullptr);
}

FMetaTable* GMetaTable = &FMetaTable::Get();

FMetaTable& FMetaTable::Get()
{
	static FMetaTable MetaTable;
	return MetaTable;
}

FMeta* FMetaTable::GetMeta(const char* MetaName)
{
	auto NameToIdIterator = NameToId.find(MetaName);
	if (NameToIdIterator != NameToId.end())
		return Metas[NameToIdIterator->second];
	return nullptr;
}

FMeta* FMetaTable::GetMeta(Uint32 MetaId)
{
	if (MetaId < Metas.size())
		return Metas[MetaId];
	return nullptr;
}

uint32_t FMetaTable::RegisterMetaToTable(FMeta* Meta)
{
	assert(Meta != nullptr);
	assert(std::end(NameToId) == NameToId.find(Meta->Name));
	if (Meta->Id == UINT32_MAX) {
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

void FMetaTable::Initialize()
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
		~FNode() { std::for_each(Childs.begin(), Childs.end(), [](std::map<FClass*, FNode*>::reference Ref) { delete Ref.second; }); }
		FClass* Struct;
		std::map<FClass*, FNode*> Childs;
	};

	std::map<FClass*, FNode*> FindMap;
	std::map<FClass*, FNode*> Root;
	std::list<FNode*> DeferredList;
	std::list<FMeta*> OtherMeta;
	for (size_t i = 1; i < Metas.size(); i++)
	{
		FMeta* Meta = Metas[i];
		FClass* Struct = dynamic_cast<FClass*>(Meta);
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
			auto Iterator = FindMap.find(Struct->Parent);
			if (Iterator != std::end(FindMap)) Iterator->second->Childs.insert(std::make_pair(Struct, Node));
			else DeferredList.push_back(Node);
		}
		FindMap.insert(std::make_pair(Struct, Node));
	}
	// deferred match
	while (!DeferredList.empty())
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
	std::for_each(Root.begin(), Root.end(), [&](std::map<FClass*, FNode*>::reference Ref) {
		CalculateCastRange(Ref.second);;
		delete Ref.second;
		});

	std::for_each(NameToId.begin(), NameToId.end(), [&](std::map<std::string, Uint32>::reference Ref) {
		Ref.second = RemapId.find(Ref.second)->second;
		});

	// Initialize StatiFMetaId
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