#pragma once

#include <gccore.h>

#ifdef MYSTL
template<typename T> class List
{
protected:
	T* data;
	u32 size;

public:
	List() { data = NULL; size = 0; }

	List(const List& list)
	{
		size = list.size;
		if (size) {
			data = new T[size];
			Copy(data, list.data, size);
		} else
			data = NULL;
	}

	T* Data() { return data; }
	T* End() { return data + size; }
	u32 Size() { return size; }

	void Copy(T* dest, T* source, u32 length)
	{
		for (u32 i = 0; i < length; i++)
			dest[i] = source[i];
	}

	T* Expand(u32 by)
	{
		T* newdata = new T[size + by];
		if (data) {
			Copy(newdata, data, size);
			delete[] data;
		}
		data = newdata;
		size += by;
		return data + size - by;
	}

	T* Add(T& item)
	{
		T* ptr = Expand(1);
		*ptr = item;
		return ptr;
	}

	T* Add(T* item, u32 length)
	{
		if (!length)
			return NULL;
		T* ptr = Expand(length);
		Copy(ptr, item, length);
		return ptr;
	}

	void Delete(T* start, T* end)
	{
		T* newdata = new T[size - (end - start)];
		if (data) {
			T* newitem = newdata;
			for (T* item = data; item < End(); item++) {
				if (item >= start && item < end)
					continue;
				*newitem = *item;
				newitem++;
			}
			delete[] data;
		}
		data = newdata;
	}

	void Clear()
	{
		if (data) {
			delete[] data;
			data = NULL;
			size = 0;
		}
	}

	T& operator[](u32 index)
	{
		return data[index];
	}

	~List()
	{
		Clear();
	}
};
#else

#include <vector>
using std::vector;
template<typename T> class List
{
protected:
	vector<T> data;

public:
	T* Data() { return &data[0]; }
	T* End() { return Data() + Size(); }
	u32 Size() { return data.size(); }

	T* Add(T& item)
	{
		data.push_back(item);
		return End() - 1;
	}

	T* Add(T* item, u32 length)
	{
		for (u32 i = 0; i < length; i++)
			Add(item[i]);
		return End() - length;
	}

	void Delete(T* start, T* end)
	{
		/*
		T* newdata = new T[size - (end - start)];
		if (data) {
			T* newitem = newdata;
			for (T* item = data; item < End(); item++) {
				if (item >= start && item < end)
					continue;
				*newitem = *item;
				newitem++;
			}
			delete[] data;
		}
		data = newdata;
		*/
	}

	void Clear()
	{
		data.clear();
	}

	T& operator[](u32 index)
	{
		return data[index];
	}

	~List()
	{
		Clear();
	}
};
#endif

template<typename K, typename T> class Pair
{
protected:
	K key;
	T value;

public:
	Pair() { }
	Pair(K k, T v) { key = k; value = v; }

	K& Key() { return key; }
	T& Value() { return value; }
};

template<typename K, typename T> class Map : public List<Pair<K, T> >
{
public:
	Map() : List<Pair<K, T> >() { }

	Pair<K, T>* Add(K key, T value)
	{
		Pair<K, T> pair(key, value);
		return List<Pair<K, T> >::Add(pair);
	}

	Pair<K, T>* Add(Pair<K, T>* item, u32 length)
	{
		return List<Pair<K, T> >::Add(item, length);
	}

	T& operator[](K key)
	{
		for (Pair<K, T>* pair = List<Pair<K, T> >::Data(); pair < List<Pair<K, T> >::End(); pair++) {
			if (pair->Key() == key)
				return pair->Value();
		}

		return Add(key, T())->Value();
	}
};
