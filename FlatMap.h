/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
******************************************************************************/

#ifndef __FLATMAP_H__
#define __FLATMAP_H__

#include "sys/sys_defines.h"

struct idLess {
	template<class A, class B> ID_FORCE_INLINE bool operator() (const A &a, const B &b) const {
		return a < b;
	}
};

template<class Key, class Value> struct idKeyVal {
	Key key;
	Value value;
};

/**
 * Flat map is a container which stores key-value pairs in an array sorted by key.
 * It supports most of the features of std::map.
 * The primary differences are:
 *   1) Flat map is more memory-efficient and cache friendly.
 *   2) Flat map is expected to be faster for small sizes.
 *   3) Modifying methods of flat map take linear time, so it can be orders of magnitude slower for large sizes.
 *
 * idTFlatMap is template by type of underlying array container,
 * which can be: idList, idStaticList, idFlexList.
 * Use idFlatMap typedef by default.
 * 
 * BEWARE: Do NOT use when size can exceed ~1000 elements!
 */

template<class Container, class Key, class Value, class Cmp = idLess>
class idTFlatMap {
public:
	typedef idKeyVal<Key, Value> KeyVal;
	typedef idTFlatMap<Container, Key, Value, Cmp> ThisFlatMap;

	//BEWARE: direct access to underlying container can break flat map!
	Container container;
	Cmp comp;

	int Num() const { return container.Num(); }
	int Size() const { return container.Size() + (sizeof(ThisFlatMap) - sizeof(container)); }
	const KeyVal *Ptr() const { return container.Ptr(); }

	void Clear(bool freeBuffer = true) {
		container.Clear(freeBuffer);
	}
	void Reserve(int count) {
		container.Reserve(count);
	}
	void Swap(ThisFlatMap &other) {
		container.Swap(other.container);
	}

	//returns index of first element which is greater or equal, in range [0..N]  (think of std::lower_bound)
	int FirstGE(const Key &key) const {
		int pos = -1;
		int span = container.Num() + 1;
		while (span > 1) {	//answer in (pos; pos + span]
			int step = span >> 1;
			pos = (comp(container[pos + step].key, key) ? pos + step : pos);
			span -= step;
		}
		return pos + 1;
	}
	//inserts new element at already found position (found e.g. using FirstGe method)
	void Insert(const Key &key, const Value &value, int pos) {
		assert(pos >= 0 && pos <= container.Num());
		assert(pos == 0 || comp(container[pos - 1].key, key) == true);
		assert(pos == container.Num() || comp(container[pos].key, key) == false);
		container.Insert(KeyVal{key, value}, pos);
	}
	//removes element by index (found e.g. using FindIndex method)
	void RemoveIndex(int pos) {
		container.RemoveIndex(pos);
	}

	//returns pointer to the element with given key, or NULL if not present
	KeyVal *Find(const Key &key) const {
		int pos = FirstGE(key);
		if (pos < container.Num() && comp(key, container[pos].key) == false)
			return const_cast<KeyVal*>(&container[pos]);
		return NULL;
	}
	//returns index of the element with given key, or -1 if not present
	int FindIndex(const Key &key) const {
		int pos = FirstGE(key);
		if (pos < container.Num() && comp(key, container[pos].key) == false)
			return pos;
		return -1;
	}
	//returns value assigned to specified key, or passed "default" if not found
	Value Get(const Key &key, const Value &default = Value()) const {
		int pos = FirstGE(key);
		if (pos < container.Num() && comp(key, container[pos].key) == false)
			return container[pos].value;
		return default;
	}

	//returns reference to the value assigned to specified key
	//if not present, then inserts the key with default-constructed value beforehand
	Value &operator[](const Key &key) {
		int pos = FirstGE(key);
		if (pos < container.Num() && comp(key, container[pos].key) == false)
			return container[pos].value;
		Insert(key, Value(), pos);
		return container[pos].value;
	}
	//assign specified value for the given key, overwrite value if already present
	//returns true if key was NOT present; index of modified element also written to *ppos
	bool Set(const Key &key, const Value &value, int *ppos = nullptr) {
		int pos = FirstGE(key);
		if (ppos)
			*ppos = pos;
		if (pos < container.Num() && comp(key, container[pos].key) == false) {
			container[pos].value = value;
			return false;
		}
		Insert(key, value, pos);
		return true;
	}
	//assign specified value for the given key, but only if such key is not present yet
	//returns true if key was NOT present; index of modified element also written to *ppos
	bool AddIfNew(const Key &key, const Value &value, int *ppos = nullptr) {
		int pos = FirstGE(key);
		if (ppos)
			*ppos = pos;
		if (pos < container.Num() && comp(key, container[pos].key) == false)
			return false;
		Insert(key, value, pos);
		return true;
	}

	//removes element with specified key if it exists
	//returns true if such element was removed; its position is stored to *ppos
	bool Remove(const Key &key, int *ppos = nullptr) {
		int pos = FirstGE(key);
		if (ppos)
			*ppos = pos;
		if (pos < container.Num() && comp(key, container[pos].key) == false) {
			RemoveIndex(pos);
			return true;
		}
		return false;
	}

	//for C++11 range-based for
	KeyVal *begin() { return container.begin(); }
	KeyVal *end() { return container.end(); }
	const KeyVal *begin() const { return container.begin(); }
	const KeyVal *end() const { return container.end(); }
};

template<class Key, class Value> using idFlatMap = idTFlatMap<idList<idKeyVal<Key, Value>>, Key, Value>;

#endif /* !__FLATMAP_H__ */
