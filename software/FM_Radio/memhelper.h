#ifndef MEMHELPER_H
#define MEMHELPER_H

#include <stdint.h>

template <class T>
class Ref_Wrap {
protected:
    uint16_t index; //Index of current value

	virtual void _write(T *val, const uint16_t &index) = 0;
	virtual T* _read(T *val, const  uint16_t &index) = 0;

public:
	Ref_Wrap(uint16_t index) : index(index) {}

	inline uint16_t getAddress() {
		return index;
	}
};


template <class T>
class Val_Wrap : public Ref_Wrap<T> {
protected:

public:
	Val_Wrap(const uint16_t &index) : Ref_Wrap<T>(index) {}
	const T* read(T *val) { return _read(val, 0); }
	void write(const T &val) { _write(&val, 0); }

	inline T read() {
		T val;
		read(&val);
		return val;
	}
};

template <class T>
class Arr_Wrap : public Ref_Wrap<T> {
protected:
	const uint16_t size;

public:
	Arr_Wrap(uint16_t index, uint16_t size) : Ref_Wrap<T>(index), size(size) {}

	T* read(const uint16_t &pos, T *val) {
		return _read(val, pos);
	}
	T read(const uint16_t &pos) {
		T val;
		//val = *(read(pos, &val));
		read(pos, &val);
		return val;
	}
	inline void write(const uint16_t &pos, const T &val) {
		_write(&val, pos);
	}

	inline uint16_t getSize() {
		return size;
	}

	virtual void setValues(const uint8_t &val, const uint16_t &num, const uint16_t &offset = 0) = 0;
	virtual void readRange(const uint16_t &offset, uint8_t* values, const uint16_t &size) = 0;
	virtual void writeRange(const uint16_t &offset, uint8_t* values, const uint16_t &size) = 0;
};


// Value wrappers

template <class T>
class Int_Val : public Val_Wrap<T> {
protected:
	T m_val;

	inline void _write(T *val, const uint16_t &index) { m_val = *val; }
	inline T* _read(T *val, const uint16_t &index) { return *val = m_val, val; }

public:
	Int_Val() : Val_Wrap<T>(0) {}
};



template <class T>
class Int_Arr : public Arr_Wrap<T> {
protected:
	T *m_val;

	inline void _write(T *val, const uint16_t &idx) {
		//*(m_val + idx) = *val;
		memcpy(reinterpret_cast<uint8_t*>(&(m_val[idx])), reinterpret_cast<uint8_t*>(val), sizeof(T));
	}
	inline T* _read(T *val, const uint16_t &idx) {
		//*val = *(m_val + idx);
		memcpy(reinterpret_cast<uint8_t*>(val), reinterpret_cast<uint8_t*>(&(m_val[idx])), sizeof(T));
		return val;
	}

public:
	Int_Arr(uint16_t size, T *ptr) : Arr_Wrap<T>(0, size) {
		m_val = ptr;
		this->index = m_val;
	}
	Int_Arr(uint16_t size) : Arr_Wrap<T>(0, size) {
		m_val = new T[size];
		this->index = m_val;
	}

	inline void setValues(const uint8_t &val, const uint16_t &num, const uint16_t &offset = 0) {
		memset(m_val + offset, val, num);
	}

	inline void readRange(const uint16_t &offset, uint8_t* values, const uint16_t &size) {
		memcpy(values, m_val + offset, size);
	}

	inline void writeRange(const uint16_t &offset, uint8_t* values, const uint16_t &size) {
		memcpy(m_val + offset, values, size);
	}

	~Int_Arr() {
		delete[] m_val;
	}
};


#endif
