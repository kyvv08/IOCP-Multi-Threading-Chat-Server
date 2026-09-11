#pragma once
#include "Common/Types.h"
#include "Memory/Memory.h"

/*-------------------
    StlAllocator
--------------------*/
template<typename T>
class StlAllocator
{
public:
	using value_type = T;

	StlAllocator() = default;

	template<typename Other>
	StlAllocator(const StlAllocator<Other>&) {}

	T* allocate(size_t count)
	{
		const int32 size = static_cast<int32>(count * sizeof(T));
		return static_cast<T*>(PoolAllocator::Alloc(size));
	}

	void deallocate(T* ptr, size_t /*count*/)
	{
		PoolAllocator::Release(ptr);
	}

	template<typename Other>
	bool operator==(const StlAllocator<Other>&) const { return true; }

	template<typename Other>
	bool operator!=(const StlAllocator<Other>&) const { return false; }
};

/*------------------------
    STL Container Aliases
-------------------------*/
template<typename Type>
using Vector = std::vector<Type, StlAllocator<Type>>;

template<typename Type>
using List = std::list<Type, StlAllocator<Type>>;

template<typename Type>
using Deque = std::deque<Type, StlAllocator<Type>>;

template<typename Key, typename Type, typename Pred = std::less<Key>>
using Map = std::map<Key, Type, Pred, StlAllocator<std::pair<const Key, Type>>>;

template<typename Key, typename Pred = std::less<Key>>
using Set = std::set<Key, Pred, StlAllocator<Key>>;

template<typename Type>
using Queue = std::queue<Type, Deque<Type>>;

template<typename Type>
using Stack = std::stack<Type, Deque<Type>>;

template<typename Key, typename Type, typename Hasher = std::hash<Key>, typename KeyEq = std::equal_to<Key>>
using HashMap = std::unordered_map<Key, Type, Hasher, KeyEq, StlAllocator<std::pair<const Key, Type>>>;

template<typename Key, typename Hasher = std::hash<Key>, typename KeyEq = std::equal_to<Key>>
using HashSet = std::unordered_set<Key, Hasher, KeyEq, StlAllocator<Key>>;

using String = std::basic_string<char, std::char_traits<char>, StlAllocator<char>>;
using WString = std::basic_string<wchar_t, std::char_traits<wchar_t>, StlAllocator<wchar_t>>;
