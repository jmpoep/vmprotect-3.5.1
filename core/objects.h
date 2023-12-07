/**
 * Support of object hierarchy.
 */

#ifndef OBJECTS_H
#define OBJECTS_H

std::string string_format(const char *format, ...);

// TODO: find more appropriate place for {
enum OperandSize : uint8_t {
	osByte,
	osWord,
	osDWord,
	osQWord,
	osTByte,
	osOWord,
	osXMMWord,
	osYMMWord,
	osFWord,
	osDefault = 0xff
};

enum MessageType {
	mtInformation,
	mtWarning,
	mtError,
	mtAdded,
	mtChanged,
	mtDeleted,
	mtScript,
};
// }

class IObject
{
public:
	virtual ~IObject() {}
	virtual int CompareWith(const IObject &) const { throw std::runtime_error("Abstract method"); }
};

class AddressableObject : public IObject
{
public:
	AddressableObject() : address_(0) {}
	AddressableObject(const AddressableObject &src) : address_(src.address_) {}
	AddressableObject &operator=(const AddressableObject &src) { address_ = src.address_; return *this; }

	using IObject::CompareWith;
	int CompareWith(const AddressableObject &other) const;
	uint64_t address() const { return address_; }
	void set_address(uint64_t address) { address_ = address; } 

protected:
	uint64_t address_;
};

template <typename Object>
class ObjectList : public IObject
{
	//not implemented
	ObjectList &operator=(const ObjectList &);
public:
	explicit ObjectList() : IObject() {}
	explicit ObjectList(const ObjectList &src) : IObject(src) {}
	virtual ~ObjectList() { clear(); }
	virtual void clear()
	{
		while (!v_.empty()) {
			delete v_.back();
		}
	}
	void Delete(size_t index)
	{
		if (index >= v_.size())
			throw std::runtime_error("subscript out of range");
		delete v_[index]; 
	}
	size_t count() const { return v_.size(); }
	Object *item(size_t index) const
	{ 
		if (index >= v_.size())
			throw std::runtime_error("subscript out of range");
		return v_[index]; 
	}
	void resize(size_t size) {
		v_.resize(size);
	}
	Object *last() const { return v_.empty() ? NULL : *v_.rbegin(); }
	static bool CompareObjects(const Object *obj1, const Object *obj2) { return obj1->CompareWith(*obj2) < 0; }
	void Sort() { std::sort(v_.begin(), v_.end(), CompareObjects); }
	typedef typename std::vector<Object*>::const_iterator const_iterator;
	typedef typename std::vector<Object*>::iterator iterator;
	size_t IndexOf(const Object *obj) const
	{
		const_iterator it = std::find(v_.begin(), v_.end(), obj);
		return (it == v_.end()) ? -1 : it - v_.begin();
	}

	size_t IndexOf(const Object *obj, size_t index) const
	{
		const_iterator it = std::find((v_.begin()+index), v_.end(), obj);
		return (it == v_.end()) ? -1 : it - v_.begin();
	}
	void SwapObjects(size_t i, size_t j) { std::swap(v_[i], v_[j]); }
	virtual void AddObject(Object *obj) { v_.push_back(obj); }
	virtual void InsertObject(size_t index, Object *obj) { v_.insert(v_.begin() + index, obj); }
	virtual void RemoveObject(Object *obj)
	{ 
		for (size_t i = count(); i > 0; i--) {
			if (item(i - 1) == obj) {
				erase(i - 1);
				break;
			}
		}
	}
	void erase(size_t index) { v_.erase(v_.begin() + index); }
	void assign(const std::list<Object*> &src) 
	{ 
		v_.clear(); 
		for (typename std::list<Object*>::const_iterator it = src.begin(); it != src.end(); it++) {
			v_.push_back(*it);
		}
	}

	const_iterator begin() const { return v_.begin(); }
	const_iterator end() const { return v_.end(); }

	iterator _begin() { return v_.begin(); }
	iterator _end() { return v_.end(); }

protected:
	void Reserve(size_t count) { v_.reserve(count); }
	std::vector<Object*> v_;
};

class Data
{
public:
	Data() {}
	Data(const std::vector<uint8_t> &src) { m_vData = src; }
	void PushByte(uint8_t value)   { m_vData.push_back(value);	}
	void PushDWord(uint32_t value) { PushBuff(&value, sizeof(value)); }
	void PushQWord(uint64_t value) { PushBuff(&value, sizeof(value)); }
	void PushWord(uint16_t value) { PushBuff(&value, sizeof(value)); }
	void PushBuff(const void *value, size_t nCount)	{ m_vData.insert(m_vData.end(), reinterpret_cast<const uint8_t *>(value), reinterpret_cast<const uint8_t *>(value) + nCount); }
	void InsertByte(size_t pos, uint8_t value) { m_vData.insert(m_vData.begin() + pos, value); }
	void InsertBuff(size_t pos, const void *buff, size_t nCount) { m_vData.insert(m_vData.begin() + pos, reinterpret_cast<const uint8_t *>(buff), reinterpret_cast<const uint8_t *>(buff) + nCount); }
	uint32_t ReadDWord(size_t nPosition) const { return *reinterpret_cast<const uint32_t *>(&m_vData[nPosition]); }
	void WriteDWord(size_t nPosition, uint32_t dwValue) { *reinterpret_cast<uint32_t *>(&m_vData[nPosition]) = dwValue; }
	size_t size() const { return m_vData.size(); }
	void clear() { m_vData.clear(); }
	bool empty() const { return m_vData.empty(); }
	void resize(size_t size) { m_vData.resize(size); }
	void resize(size_t size, uint8_t value) { m_vData.resize(size, value); }
	const uint8_t *data() const { return m_vData.data(); }
	const uint8_t &operator[](size_t pos) const
	{ 
		if (pos >= m_vData.size())
			throw std::runtime_error("subscript out of range");		
		return m_vData[pos]; 
	}
	uint8_t &operator[](size_t pos)
	{ 
		if (pos >= m_vData.size())
			throw std::runtime_error("subscript out of range");		
		return m_vData[pos]; 
	}
	bool operator < (const Data &right) const
	{
		return (size() != right.size()) ? (size() < right.size()) : (memcmp(data(), right.data(), size()) < 0);
	}
private:
	std::vector<uint8_t> m_vData;
};

class canceled_error : public std::runtime_error
{
public:
	explicit canceled_error(const std::string &message)
		: runtime_error(message) {}
};

class abort_error : public std::runtime_error
{
public:
	explicit abort_error(const std::string &message)
		: runtime_error(message) {}
};

#endif
