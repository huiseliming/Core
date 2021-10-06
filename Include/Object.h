#pragma once
#include <string>
#include <vector>
#include <atomic>

struct FGCNode
{
	std::atomic<uint32_t> UseCounter;
	int32_t ArrayIndex;
	OObject* Object = nullptr;
};

std::vector<FGCNode> GCArray;

class OObject
{
public:
	OObject()
	{
			
	}

	virtual ~OObject() 
	{
	
	}

	std::string Name;
};








