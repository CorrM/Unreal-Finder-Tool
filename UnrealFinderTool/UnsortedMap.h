#ifndef UNSORTED_MAP
#define UNSORTED_MAP

#pragma once
#include <algorithm>
#include <vector>


template <typename K, typename V>
class UnsortedMap : public std::vector<std::pair<K, V>>
{
	using UnsortedMapItem = std::pair<K, V>;
	using UnsortedMapIt = typename std::vector<std::pair<K, V>>::iterator;
public:
	UnsortedMap() = default;
	UnsortedMap(const UnsortedMapIt& begin, const UnsortedMapIt& end)
	{
		this->assign(begin, end);
	}

	UnsortedMapIt find(const K& obj)
	{
		return std::find_if(this->begin(), this->end(), [&obj](const std::pair<K, V>& vecItem)
		{
			return vecItem.first == obj;
		});
	}

	V* Find(const K& obj, bool& success)
	{
		auto it = std::find_if(this->begin(), this->end(), [&obj](const std::pair<K, V>& vecItem) -> bool
		{
			return vecItem.first == obj;
		});

		success = it != this->end();
		if (success)
			return &it->second;
		return nullptr; // if code hit this point then maybe there a problem need to solve :D
	}

	V* Find(const K& ref)
	{
		bool tmp;
		return Find(ref, tmp);
	}
};
#endif // !UnsortedMap
