#pragma once
#include <memory>
#include "PlatformImplement.h"

namespace EQueueMode{
	enum Type
	{
		SPSC,
		MPSC,
	};
}

template<typename T, EQueueMode::Type Mode = EQueueMode::SPSC>
class TQueue
{
public:

	TQueue()
	{
		Head = Tail = new TNode();
	}

	~TQueue()
	{
		while (Tail != nullptr)
		{
			TNode* Node = Tail;
			Tail = Tail->NextNode;

			delete Node;
		}
	}

	bool Dequeue(T& OutElement)
	{
		TNode* Popped = Tail->NextNode;

		if (Popped == nullptr)
		{
			return false;
		}

		OutElement = std::move(Popped->Element);

		TNode* OldTail = Tail;
		Tail = Popped;
		Tail->Element = T();
		delete OldTail;

		return true;
	}

	bool Enqueue(const T& Element)
	{
		TNode* NewNode = new TNode(Element);

		if (NewNode == nullptr)
		{
			return false;
		}

		TNode* OldHead;

		if (Mode == EQueueMode::MPSC)
		{
			OldHead = (TNode*)FPlatformImplement::InterlockedExchangePtr((void**)&Head, NewNode);
			FPlatformImplement::InterlockedExchangePtr((void**)&OldHead->NextNode, NewNode);
		}
		else
		{
			OldHead = Head;
			Head = NewNode;
			FPlatformImplement::MemoryBarrier();
			OldHead->NextNode = NewNode;
		}
		return true;
	}

	bool Enqueue(T&& Element)
	{
		TNode* NewNode = new TNode(std::move(Element));

		if (NewNode == nullptr)
		{
			return false;
		}

		TNode* OldHead;

		if (Mode == EQueueMode::MPSC)
		{
			OldHead = (TNode*)FPlatformImplement::InterlockedExchangePtr((void**)&Head, NewNode);
			FPlatformImplement::InterlockedExchangePtr((void**)&OldHead->NextNode, NewNode);
		}
		else
		{
			OldHead = Head;
			Head = NewNode;
			FPlatformImplement::MemoryBarrier();
			OldHead->NextNode = NewNode;
		}

		return true;
	}

	void Empty()
	{
		while (Pop());
	}

	bool IsEmpty() const
	{
		return (Tail->NextNode == nullptr);
	}

	bool Peek(T& OutElement) const
	{
		if (Tail->NextNode == nullptr)
		{
			return false;
		}

		OutElement = Tail->NextNode->Element;

		return true;
	}

	T* Peek()
	{
		if (Tail->NextNode == nullptr)
		{
			return nullptr;
		}

		return &Tail->NextNode->Element;
	}

	const T* Peek() const
	{
		return const_cast<TQueue*>(this)->Peek();
	}

	bool Pop()
	{
		TNode* Popped = Tail->NextNode;

		if (Popped == nullptr)
		{
			return false;
		}

		TNode* OldTail = Tail;
		Tail = Popped;
		Tail->Element = T();
		delete OldTail;

		return true;
	}

	bool MoreThanOne() {
		TNode* FirstNode = Tail->NextNode;
		if (FirstNode)
		{
			if (FirstNode->NextNode) {
				return true;
			}
		}
		return false;
	}

private:

	struct TNode
	{
		TNode()
			: NextNode(nullptr)
		{ }

		explicit TNode(const T& InElement)
			: NextNode(nullptr)
			, Element(InElement)
		{ }

		explicit TNode(T&& InElement)
			: NextNode(nullptr)
			, Element(std::move(InElement))
		{ }

		TNode* alignas(8) NextNode;
		T Element;
	};

	alignas(8) TNode* Head;

	alignas(8) TNode* Tail;

private:

	TQueue(const TQueue&) = delete;

	TQueue& operator=(const TQueue&) = delete;
};