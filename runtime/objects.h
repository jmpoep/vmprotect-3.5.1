#ifndef OBJECTS_H
#define OBJECTS_H

template<class T>
class vector
{
public:
	vector() : data_(NULL), total_(0), size_(0) {}
	~vector() { delete [] data_; }
	size_t size() const { return size_; }
	bool empty() const { return size_ == 0; }
	const T &operator[] (size_t index) const { return data_[index]; }
	T &operator[] (size_t index) {return data_[index]; }
	void push_back(const T &t) 
	{
		if (size_ == total_) { 
			// no free space
			size_t new_total = (total_ == 0) ? 1 : total_ * 2;
			T *new_data = new T[new_total];
			for (size_t i = 0; i < size_; i++) {
				new_data[i] = data_[i];
			}
			delete [] data_;
			data_ = new_data;
			total_ = new_total;
		}
		data_[size_++] = t;
	}
	void pop_back()
	{
		if (size_)
			size_--;
	}
	
	void erase(size_t pos)
	{
		if (pos >= size_) 
			return; // error
		for (size_t i = pos; i < size_ - 1; i++) {
			data_[i] = data_[i + 1];
		}
		size_--;
	}

	template <typename X>
	size_t find(const X &v) const
	{
		for (size_t i = 0; i < size_; i++) {
			if (data_[i] == v) 
				return i;
		}
		return -1;
	}

	void clear()
	{
		size_ = total_ = 0;
		delete [] data_;
		data_ = NULL;
	}
private:
	T *data_;
	size_t total_;
	size_t size_;
};

class CriticalSection
{
public:
	CriticalSection(CRITICAL_SECTION &critical_section);
	~CriticalSection();
	static void Init(CRITICAL_SECTION &critical_section);
	static void Free(CRITICAL_SECTION &critical_section);
private:
	CRITICAL_SECTION &critical_section_;
};

#endif