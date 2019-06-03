#ifndef UNSORTED_MAP
#define UNSORTED_MAP

#pragma once
#include <algorithm>
#include <vector>

template <typename K, typename V>
class UnsortedMap : public std::vector<std::pair<K, V>>
{
	using UnsortedMapIt = typename std::vector<std::pair<K, V>>::iterator;
public:
	UnsortedMap() = default;
	UnsortedMap(const UnsortedMapIt& begin, const UnsortedMapIt& end)
	{
		this->assign(begin, end);
	}

	UnsortedMapIt find(const K& ref)
	{
		return std::find_if(this->begin(), this->end(), [ref](const std::pair<K, V>& vecItem)
		{
			return vecItem.first == ref;
		});
	}

	V& Find(const K& ref, bool& success)
	{
		auto it = std::find_if(this->begin(), this->end(), [&](const std::pair<K, V>& vecItem) -> bool
		{
			return vecItem.first == ref;
		});

		success = it != this->end();
		return it->second;
	}

	V& Find(const K& ref)
	{
		bool tmp;
		return Find(ref, tmp);
	}
};
#endif // !UnsortedMap
