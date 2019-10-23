//
//
// Thread safe container.
//
// Author: Burlutsky Stas.
//


#pragma once

#include <mutex>
#include <map>
#include <string>

template <typename key, typename value>
class ThsMap
{
public:

	typedef std::map<key, value> ThsMapType;

	void Add(const key& _key, const value& _value)
	{
		std::unique_lock<std::mutex> mtxlocker(m_mutex);
		m_map[_key] = _value;
	}

	value& operator[](const key& _key)
	{
		std::unique_lock<std::mutex> mtxlocker(m_mutex);
		value& val = m_map[_key];
		return val;
	}

	// Если такая пара присутствует, то вернуть значение для ключа.
	bool getValueIfPresent(const key& _key, value& _value)
	{
		bool result = false;
		std::unique_lock<std::mutex> mtxlocker(m_mutex);
		if (result = (m_map.count(_key) != 0))
		{
			_value = m_map[_key];
		}
		return result;
	}

	// Задать новое соответствие ключу, если такой имеется.
	bool setValueIfPresent(const key& _key, const value& _value)
	{
		bool result = false;
		std::unique_lock<std::mutex> mtxlocker(m_mutex);
		if (result = (m_map.count(_key) != 0))
		{
			m_map[_key] = _value;
		}
		return result;
	}

	// Free memory of the object if it was added earlier.
	void AddPtr(const key& _key, const value& _value)
	{
		std::unique_lock<std::mutex> mtxlocker(m_mutex);
		if (m_map.count(_key) != 0)
		{
			if (m_map[_key] != _value)
			{
				delete m_map[_key];
			}
		}
		m_map[_key] = _value;
	}

	void Clear()
	{
		std::unique_lock<std::mutex> mtxlocker(m_mutex);
		m_map.clear();
	}

	void ClearPtrs()
	{
		std::unique_lock<std::mutex> mtxlocker(m_mutex);
		for (ThsMapType::iterator i = m_map.begin(); i != m_map.end(); ++i)
		{
			delete i->second;
		}
		m_map.clear();
	}

	bool Present(const key& _key) const
	{
		std::unique_lock<std::mutex> mtxlocker(m_mutex);
		bool present = m_map.count(_key) != 0;
		return present;
	}

	void RemovePtr(const key& _key)
	{
		std::unique_lock<std::mutex> mtxlocker(m_mutex);
		if (m_map.count(_key) != 0)
		{
			delete m_map[_key];
			m_map.erase(_key);
		}
	}

	void Remove(const key& _key)
	{
		std::unique_lock<std::mutex> mtxlocker(m_mutex);
		m_map.erase(_key);
	}

	int Size()
	{
		std::unique_lock<std::mutex> mtxlocker(m_mutex);
		int size = m_map.size();
		return size;
	}

	value Get(const key& _key)
	{
		std::unique_lock<std::mutex> mtxlocker(m_mutex);
		value _val = m_map[_key];
		return _val;
	}

	void GetCopyOfData(ThsMapType& _stlMap)
	{
		std::unique_lock<std::mutex> mtxlocker(m_mutex);
		_stlMap = m_map;
	}

	// To use original container.
	ThsMapType& GetStlMap()
	{
		return m_map;
	}

	void Lock()
	{
		m_mutex.lock();
	}

	void Unlock()
	{
		m_mutex.unlock();
	}

private:
	mutable std::mutex m_mutex;
	ThsMapType m_map;
};

