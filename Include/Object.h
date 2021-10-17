#pragma once
#include <mutex>
#include <thread>
#include <set>
#include "PlatformImplement.h"

#include "Reflect.h"

// 多线程
// 线程上下文数组
// 多线程同步marker
// 分步sweep
// b
//TObject<>::New
/**
 *
 *
**/

enum EObjectItemFlag
{
    EOIF_NoneFlag = 0,
    EOIF_RootSetFlag = 1 << 0,
    EOIF_PendingConstructionFlag = 1 << 1,
    EOIF_PendingRecycleFlag = 1 << 2,
    EOIF_ReachableFlag = 1 << 31,
};

enum EObjectFlag
{
    EOF_NoneFlag = 0,
};

class CObjectBase
{
    friend class CObjectArray;
public:
    CObjectBase(std::string Name = "", CClass* InClass = nullptr, EObjectFlag InFlag = EOF_NoneFlag)
        : Class(InClass)
        , Flag(InFlag)
    {}


    virtual ~CObjectBase() {}

    

    CClass* GetClass() { return Class; }
    void SetClass(CClass * InClass){ Class = InClass;}

private:
    CClass* Class;
    Uint32 Flag;
    std::string Name;
    Uint32 InternalIndex;
};



class CObject : public CObjectBase
{
public:
    CObject(){}

};

struct FObjectItem
{
    Uint32 InternalIndex{ UINT32_MAX };
    CObjectBase* Object{ nullptr };
    Uint32 Flag{ 0 };
};

class CObjectArray
{
public:
    enum
    {
        NumElementsPerChunk = 64 * 1024, // 64k
    };

    CObjectArray(Uint32 PreAllocatedSize = 4 * 1024)
    {
        ObjectItems.reserve(PreAllocatedSize);
    }

    void AllocateObjectItem(CObject* Object)
    {
        Uint32 Index = -1;
        if(AvailableItemList.size() > 0)
        {
            Index = AvailableItemList.front();
            AvailableItemList.pop_front();
#ifndef NDEBUG
            assert(!AvailableItems.contains(Index));
            AvailableItems.erase(AvailableItems.find(Index));
#endif
        }
        else
        {
            Index = ObjectItems.size();
            ObjectItems.push_back(FObjectItem{ .InternalIndex = Index });
        }
        Object->InternalIndex = Index;
        ObjectItems[Index].Object = Object;
        ObjectItems[Index].Flag = EOIF_PendingConstructionFlag;
    }
    
    void FreeObjectItem(Uint32 Index)
    {
        AvailableItemList.push_back(Index);
#ifndef NDEBUG
        assert(!AvailableItems.contains(Index));
        AvailableItems.insert(Index);
#endif
        //ObjectItems[Index].Flag &= !EOIF_PendingRecycleFlag;
        ObjectItems[Index].Flag = 0;
    }

    void CollectGarbage()
    {
        for (size_t i = 0; i < ObjectItems.size(); i++)
        {
            if(ObjectItems[i].Flag | EOIF_RootSetFlag)
            {
               
            }
        }
    }

    std::vector<FObjectItem> ObjectItems;
    std::list<Uint32>  AvailableItemList;
#ifndef NDEBUG
    std::set<Uint32>  AvailableItems;
#endif

};

struct FThreadContext
{
    std::thread::id ThreadId;
    CObjectArray ObjectArray;
};

extern CORE_API std::unordered_map<std::thread::id, FThreadContext> GThreadContexts;

extern CORE_API FThreadContext& GetCurrentThreadContext();

template<typename O>
O* NewObject(std::string Name = "")
{
    CClass* Class = (CClass*)GMetaTable->GetMeta(O::MetaId);
    assert(Class);
    O* Object = (O*)Class->New();
    Object->SetClass(Class);
    GetCurrentThreadContext().ObjectArray.AllocateObjectItem(Object);
    return Object;
}



class CORE_API CLASS() OTest : public CObject
{
    REFLECT_GENERATED_BODY()

};





//template<typename T, Int32 MaxChunks = 256, Int32 ChunkedSize = 64 * 1024>
//class TChunkedFixedVector
//{
//    enum
//    {
//        MaxElements = MaxChunks * ChunkedSize,
//    };
//
//    Int32 NumElements;
//    std::atomic<Int32> NumChunks;
//    T* Data[MaxChunks];
//    T* PreAllocatedMemory;
//
//    void ExpandChunksToIndex(Int32 ElementIndex)
//    {
//        assert(ElementIndex >= MaxElements)
//            Int32 ChunkIndex = ElementIndex / ChunkedSize;
//        while (ChunkIndex < NumChunks)
//        {
//            T** DestChunk = &Data[NumChunks];
//            T* ExchangeChunk = new T[ChunkedSize];
//            if (!FPlatformImplement::InterlockedCompareExchangePointer(DestChunk, ExchangeChunk, nullptr))
//            {
//                NumChunks++;
//            }
//        }
//    }
//public:
//    TChunkedFixedVector(bool PreAllocated = false)
//        : NumElements(0)
//        , NumChunks(0)
//        , Data(nullptr)
//    {
//        Data = new T * [MaxChunks];
//        memset(Data, 0, sizeof(T*) * MaxChunks);
//        if (PreAllocated)
//        {
//            // Fully allocate all chunks as contiguous memory
//            PreAllocatedMemory = new T[MaxElements];
//            for (Int32 ChunkIndex = 0; ChunkIndex < MaxChunks; ++ChunkIndex)
//            {
//                Data[ChunkIndex] = PreAllocatedObjects + ChunkIndex * ChunkedSize;
//            }
//            NumChunks = MaxChunks;
//        }
//    }
//    ~TChunkedFixedVector() {
//        if (PreAllocatedMemory)
//        {
//            delete[] PreAllocatedMemory;
//        }
//        else
//        {
//            for (Int32 ChunkIndex = 0; ChunkIndex < NumChunks; ++ChunkIndex)
//            {
//                delete[] Data[ChunkIndex];
//            }
//        }
//        delete[] Data;
//    }
//
//    Int32 Num() const
//    {
//        return NumElements;
//    }
//
//    Int32 Capacity() const
//    {
//        return MaxElements;
//    }
//
//    bool IsValidIndex(Int32 Index) const
//    {
//        return Index < Num() && Index >= 0;
//    }
//
//    T const* GetDataPtr(Int32 Index) const
//    {
//        const Int32 ChunkIndex = Index / ChunkedSize;
//        const Int32 WithinChunkIndex = Index % ChunkedSize;
//        T* Chunk = Data[ChunkIndex];
//        return Chunk + WithinChunkIndex;
//    }
//
//    T const& operator[](Int32 Index) const
//    {
//        T const* DataPtr = GetDataPtr(Index);
//        return *DataPtr;
//    }
//
//    T& operator[](Int32 Index)
//    {
//        T* DataPtr = GetDataPtr(Index);
//        return *DataPtr;
//    }
//
//    Int32 AddRange(Int32 NumToAdd)
//    {
//        Int32 Result = NumElements;
//        ExpandChunksToIndex(Result + NumToAdd - 1);
//        NumElements += NumToAdd;
//        return Result;
//    }
//
//    Int32 AddSingle()
//    {
//        return AddRange(1);
//    }
//};
