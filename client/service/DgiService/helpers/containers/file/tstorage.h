//
//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//
//

#pragma once

#include <string>
#include <mutex>
#include <list>
#include <vector>
#include "mapper.h"

#pragma warning( disable : 4290 )


template <class T, class Predicate>
class TStorage
{

public:
	typedef unsigned long CounterType;
	static const int ElementNotFound = -1;

	typedef std::vector<T> EntriesList;
	
	TStorage(std::string _file) :m_mapper(_file)
	{
		m_mapper.load();
		if (m_mapper.fileSize() == 0)
		{
			m_mapper.unload();
			m_mapper.load(sizeof(CounterType));
			setCountOfEntries(0);
		}
	}

	virtual ~TStorage()
	{
		close();
	}

	bool push_back(const T& _entry)
	{
		bool added = false;

		if (m_mapper.getMappedData())
		{
			CounterType count = getCountOfEntries();
			if (added = m_mapper.increaseFileSize(sizeof(T)))
			{
				// Copy data to new allocated memory block.
				memcpy((BYTE*)((BYTE*)m_mapper.getMappedData() + sizeof(CounterType) + count * sizeof(T)),
					&_entry,
					sizeof(T));

				setCountOfEntries(count + 1);
			}
		}

		return added;
	}

	void remove_first(const T& _entry, CounterType _startSearchPos = 0)
	{
		auto size = getCountOfEntries();
		for (CounterType i = 0; i < size;)
		{
			if (at(i) == _entry)
			{
				remove_pos(i);
				--size;
			}
			else
			{
				++i;
			}
		}
	}

	void remove_if(Predicate _condition)
	{
		auto size = getCountOfEntries();
		for (CounterType i = 0; i < size;)
		{
			if (_condition(at(i)))
			{
				remove_pos(i);
				--size;
			}
			else
			{
				++i;
			}
		}
	}

	template <class Container, class UnaryPredicate>
	void remove_if(Container& _storage, UnaryPredicate _pred, int _startSearchPos = 0)
	{
		auto size = _storage.count();
		for (auto i = _startSearchPos; i < size;)
		{
			if (_pred(_storage.at(i)))
			{
				_storage.remove_pos(i);
				--size;
			}
			else
			{
				++i;
			}
		}
	}

	void remove_pos(CounterType _pos)
	{
		auto size = getCountOfEntries();
		if (_pos < size)
		{
			T* removingElement = getFirstEntry() + _pos;

			// Удаляется не последний элемент.
			if (removingElement != getLastEntry())
			{
				// Сдвигаем все элементы после удаляемого, на одну позицию.
				move(_pos, _pos + 1, size - 1);
			}

			// Уменьшаем итоговый размер файла-контейнера.
			setCountOfEntries(size - 1);
			m_mapper.cutSize(sizeof(T) * (size - 1) + sizeof(CounterType));
		}
	}

	// Provides access to an element at specific position.
	// Complexity: O(1)
	T& at(CounterType _pos) const
	{
		CounterType size = count();
		if (_pos < size)
		{
			return getFirstEntry()[_pos];
		}
		else
		{
			throw std::out_of_range("bad index has passed");
		}
	}

	// Returns index of found element otherwise -1 if element is not found.
	int find_if(Predicate _condition, CounterType _searchStartPosition = 0) const
	{
		auto size = count();
		if (_searchStartPosition >= size)
		{
			return ElementNotFound;
		}
		
		T* pEntry = getFirstEntry();
		for (auto i = _searchStartPosition; i < size; ++i, ++pEntry)
		{
			if (_condition(*pEntry))
			{
				return i;
			}
		}

		return ElementNotFound;
	}

	// Returns true if data was copied to output list.
	bool get_if(EntriesList& _outListEntries, Predicate _condition, CounterType _searchStartPosition = 0) const
	{
		bool dataWasCopied = false;
		auto size = count();
		if (_searchStartPosition >= size)
		{
			return false;
		}

		T* pEntry = getFirstEntry();
		for (auto i = _searchStartPosition; i < size; ++i, ++pEntry)
		{
			if (_condition(*pEntry))
			{
				_outListEntries.push_back(*pEntry);
				dataWasCopied = true;
			}
		}

		return dataWasCopied;
	}

	bool update(T& _findOldEntry, T& _new) throw()
	{
		bool updated = false;

		CounterType count = getCountOfEntries();
		T* pEntry = getFirstEntry();
		for (size_t i = 0; i < count; ++i, ++pEntry)
		{
			if (updated = (*pEntry == _findOldEntry))
			{
				*pEntry = _new;
			}
		}

		return updated;
	}

	void fill_vector(CounterType _count, std::vector<T>& _entries) const
	{
		auto count = getCountOfEntries();
		if (count == 0 || _count == 0)
		{
			return;
		}
		else if (count < _count)
		{
			_count = count;
		}

		T* pEntry = getFirstEntry();
		for (unsigned long i = 0; i < count; ++i, ++pEntry)
		{
			_entries.push_back(*pEntry);
		}
	}

	// That is very useful thing.
	// It helps to get copy of all data and work with that like you want.
	void toVector(std::vector<T>& _entries) const
	{
		fill_vector(count(), _entries);
	}

	std::vector<T> toVector() const
	{
		std::vector<T> result;
		toVector(result);
		return result;
	}

	bool clear()
	{
		setCountOfEntries(0);
		bool success = m_mapper.cutSize(sizeof(CounterType));
		return success;
	}

	// Complexity: O(1)
	bool empty() const
	{
		return count() == 0;
	}

	// Complexity: O(1)
	CounterType count() const
	{
		CounterType count = getCountOfEntries();
		return count;
	}

	// Complexity: O(N)
	CounterType count(const T& _entry) const
	{
		CounterType count = 0, size = getCountOfEntries();
		T* pEntry = getFirstEntry();
		for (CounterType i = 0; i < count; ++i, ++pEntry)
		{
			if (*pEntry == _entry)
			{
				count++;
			}
		}

		return count;
	}

	// Complexity: O(N)
	CounterType count_if(Predicate _condition) const
	{
		CounterType count = 0, size = getCountOfEntries();
		T* pEntry = getFirstEntry();
		for (CounterType i = 0; i < size; ++i, ++pEntry)
		{
			if (_condition(*pEntry))
			{
				count++;
			}
		}

		return count;
	}

	bool isPresent(Predicate _condition) const
	{
		CounterType count = 0, size = getCountOfEntries();
		T* pEntry = getFirstEntry();
		for (CounterType i = 0; i < size; ++i, ++pEntry)
		{
			if (_condition(*pEntry))
			{
				return true;
			}
		}

		return false;
	}

	void close()
	{
		m_mapper.unload();
	}

	std::string get_file_path() const
	{
		return m_mapper.getFilePath();
	}

	bool isOpened() const
	{
		return m_mapper.isLoaded();
	}

	bool reload()
	{
		if (m_mapper.flush())
		{
			m_mapper.unload();

			if (m_mapper.load())
			{
				return true;
			}
		}
		
		return false;
	}

	bool flush()
	{
		return m_mapper.flush();
	}

	void lock() const
	{
		m_mutex.lock();
	}

	void unlock() const
	{
		m_mutex.unlock();
	}

private:

	inline void* getRawPtr() const
	{
		void* ptr = m_mapper.getMappedData();
		return ptr;
	}

	inline CounterType getCountOfEntries() const
	{
		CounterType count = 0;
		if (m_mapper.getMappedData())
		{
			count = *((CounterType*)m_mapper.getMappedData());
		}
		return count;
	}

	void setCountOfEntries(CounterType _count)
	{
		if (m_mapper.getMappedData())
		{
			*((CounterType*)m_mapper.getMappedData()) = _count;
		}
	}

	inline T* getFirstEntry() const
	{
		if (getCountOfEntries())
		{
			return (T*)((BYTE*)m_mapper.getMappedData() + sizeof(CounterType));
		}

		return nullptr;
	}

	inline T* getLastEntry() throw()
	{
		CounterType count = getCountOfEntries();
		if (count)
		{
			return getFirstEntry() + (count - 1);
		}

		return getFirstEntry();
	}

	void move(CounterType _toPos, CounterType _fromPos, CounterType _endPos)
	{
		auto entries = getFirstEntry();
		for (auto pos = _fromPos; pos <= _endPos; ++pos)
		{
			entries[_toPos + (pos - _fromPos)] = entries[pos];
		}
	}

	whlp::Mapper m_mapper;
	mutable std::mutex m_mutex;

	// All instances are not copyable.
	TStorage(const TStorage& _rhs);
	TStorage& operator=(const TStorage& _rhs);
};

//
// Additional algorithms for - mapped file container (mfc).
//
namespace mfc_algs
{
	static const int NotFound = -1;

	//
	// Возвращает номер позиции первого элемента, удовлетворяющего условиям поиска.
	//
	// Данный алгоритм очень удобно использовать, когда существующий предикат,
	// который является частью типа контейнера, не подходит для выборки. 
	//
	template <class Container, class UnaryPredicate>
	int find_if(const Container& _storage, const UnaryPredicate& _pred, int _startSearchPos = 0)
	{
		auto size = _storage.count();
		for (size_t i = _startSearchPos; i < size; ++i)
		{
			try
			{
				if (_pred(_storage.at(i)))
				{
					return i;
				}
			} catch (std::out_of_range&)
			{
				return mfc_algs::NotFound;
			}
		}

		return mfc_algs::NotFound;
	}

	template <class Container, class UnaryPredicate>
	void remove_if(Container& _storage, UnaryPredicate _pred, size_t _startSearchPos = 0)
	{
		auto size = _storage.count();
		for (size_t i = _startSearchPos; i < size;)
		{
			if ( _pred(_storage.at(i)) )
			{
				_storage.remove_pos(i);
				--size;
			} else {
				++i;
			}
		}
	}
}


//
// Fake selector - It doesn't do anything! Returns false.
// It helps when you use TStorage for storing only one element (structure, object).
//
template <class T>
class NoSelector
{
public:
	bool operator()(const T& _entry) const
	{
		return false;
	}
};
