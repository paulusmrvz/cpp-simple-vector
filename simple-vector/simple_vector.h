#pragma once

#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <cassert>

class ReserveProxyObj {
public:
	explicit ReserveProxyObj(size_t capacity_to_reserve)
		: capacity_to_reserve_(capacity_to_reserve) {}

	ReserveProxyObj(const ReserveProxyObj&) = delete;
	ReserveProxyObj& operator=(const ReserveProxyObj&) = delete;

	size_t GetCapacity() const {
		return capacity_to_reserve_;
	}
private:
	size_t capacity_to_reserve_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
	return ReserveProxyObj(capacity_to_reserve);
}

// Но я прошу простить отсутствие реализации ArrayPtr. Я его не реализовывал в самый первый раз, ещё месяцев 4 назад,
// а сейчас я опаздываю и не хотелось бы тратить время на реализацию еще одной структуры,
// которая имеет связь с SimpleVector "as-is"
template <typename Type>
class SimpleVector {
public:
	using Iterator = Type*;
	using ConstIterator = const Type*;

	SimpleVector() noexcept = default;

	explicit SimpleVector(size_t size) {
		data_ = new Type[size];
		size_ = capacity_ = size;
	}

	SimpleVector(size_t size, const Type& value) {
		// Если выкидывает исключение bad_alloc - то память не выделяется,
		// значит никаких утечек быть не может
		// и так со всеми new и placement-new, которые ниже
		data_ = new Type[size];
		size_ = capacity_ = size;
		try {
			for (size_t iter{}; iter < size; ++iter) {
				data_[iter] = value;
			}
		}
		catch (...) {
			delete[] data_;
			throw;
		}
	}

	SimpleVector(std::initializer_list<Type> init) {
		data_ = new Type[init.size()];
		size_ = capacity_ = init.size();
		try {
			std::copy(init.begin(), init.end(), data_);
		}
		catch (...) {
			delete[] data_;
			throw;
		}
	}

	SimpleVector(const ReserveProxyObj reserve)
		: SimpleVector() {
		Reserve(reserve.GetCapacity());
	}

	SimpleVector(const SimpleVector& other) {
		data_ = new Type[other.capacity_];
		size_ = other.size_;
		capacity_ = other.capacity_;
		try {
			std::copy(other.data_, other.data_ + size_, data_);
		}
		catch (...) {
			delete[] data_;
			throw;
		}
	}

	SimpleVector(SimpleVector&& other)
		: data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
		other.data_ = nullptr;
		other.size_ = 0;
		other.capacity_ = 0;
	}


	SimpleVector& operator=(const SimpleVector& rhs) {
		if (this != &rhs) {
			SimpleVector(rhs).swap(*this);
		}
		return *this;
	}

	SimpleVector& operator=(SimpleVector&& rhs) {
		if (this != &rhs) {
			delete[] data_;
			data_ = rhs.data_;
			size_ = rhs.size_;
			capacity_ = rhs.capacity_;
			rhs.data_ = nullptr;
			rhs.size_ = 0;
			rhs.capacity_ = 0;
		}
		return *this;
	}

	~SimpleVector() {
		delete[] data_;
	}

	void PushBack(const Type& item) {
		if (size_ == capacity_) {
			size_t new_capacity = capacity_ == 0 ? 1 : 2 * capacity_;
			Type* new_data = new Type[new_capacity];
			try {
				std::copy(data_, data_ + size_, new_data);
			}
			catch (...) {
				delete[] new_data;
				delete[] data_;
				throw;
			}
			delete[] data_;
			data_ = new_data;
			capacity_ = new_capacity;
		}
		data_[size_] = item;
		++size_;
	}

	void PushBack(Type&& item) {
		if (size_ == capacity_) {
			size_t new_capacity = capacity_ == 0 ? 1 : 2 * capacity_;
			Type* new_data = new Type[new_capacity];
			try {
				std::move(data_, data_ + size_, new_data);
			}
			catch (...) {
				delete[] new_data;
				delete[] data_;
				throw;
			}
			delete[] data_;
			data_ = new_data;
			capacity_ = new_capacity;
		}
		data_[size_] = item;
		++size_;
	}

	Iterator Insert(ConstIterator pos, const Type& value) {
		ptrdiff_t index = pos - data_;
		assert(index <= size_ && index >= 0);
		if (size_ == capacity_) {
			size_t new_capacity = capacity_ == 0 ? 1 : 2 * capacity_;
			Type* new_data = new Type[new_capacity];
			try {
				std::copy(data_, data_ + index, new_data);
			}
			catch (...) {
				delete[] new_data;
				delete[] data_;
				throw;
			}

			new_data[index] =  value;

			try {
				std::copy(data_ + index, data_ + size_, new_data + index + 1);
			}
			catch (...) {
				delete[] new_data;
				delete[] data_;
				throw;
			}

			delete[] data_;

			data_ = new_data;
			capacity_ = new_capacity;
		}
		else {
			for (size_t i = size_; i > index; --i) {
				data_[i] = std::move(data_[i - 1]);
			}

			data_[index] = value;
		}

		++size_;
		return data_ + index;
	}

	Iterator Insert(ConstIterator pos, Type&& value) {
		ptrdiff_t index = pos - data_;
		assert(index <= size_ && index >= 0);
		if (size_ == capacity_) {
			size_t new_capacity = capacity_ == 0 ? 1 : 2 * capacity_;
			Type* new_data = new Type[new_capacity];

			try {
				std::move(data_, data_ + index, new_data);
			}
			catch (...) {
				delete[] new_data;
				delete[] data_;
				throw;
			}

			new_data[index] = std::move(value);

			try {
				std::move(data_ + index, data_ + size_, new_data + index + 1);
			}
			catch (...) {
				delete[] new_data;
				delete[] data_;
				throw;
			}

			delete[] data_;

			data_ = new_data;
			capacity_ = new_capacity;
		}
		else {
			for (size_t i = size_; i > index; --i) {
				data_[i] = std::move(data_[i - 1]);
			}

			data_[index] = std::move(value);
		}
		++size_;
		return data_ + index;
	}

	void PopBack() noexcept {
		assert(size_ > 0);
		--size_;
	}

	Iterator Erase(ConstIterator pos) {
		ptrdiff_t index = pos - data_;
		assert(size_ > 0);
		assert(index <= size_ && index >= 0);
		std::move(data_ + index + 1, data_ + size_, data_ + index);
		--size_;
		return data_ + index;
	}

	void swap(SimpleVector& other) noexcept {
		std::swap(data_, other.data_);
		std::swap(size_, other.size_);
		std::swap(capacity_, other.capacity_);
	}

	size_t GetSize() const noexcept {
		return size_;
	}

	size_t GetCapacity() const noexcept {
		return capacity_;
	}

	bool IsEmpty() const noexcept {
		return size_ == 0;
	}

	Type& operator[](size_t index) noexcept {
		assert(index < size_);
		return data_[index];
	}

	const Type& operator[](size_t index) const noexcept {
		assert(index < size_);
		return data_[index];
	}

	Type& At(size_t index) {
		if (index >= size_) {
			throw std::out_of_range("index out of range");
		}
		return data_[index];
	}

	const Type& At(size_t index) const {
		if (index >= size_) {
			throw std::out_of_range("index out of range");
		}
		return data_[index];
	}

	void Clear() noexcept {
		size_ = 0;
	}

	void Resize(size_t new_size) {
		if (new_size > capacity_) {
			Type* new_data = new Type[new_size];
			try {
				std::move(data_, data_ + size_, new_data);
			}
			catch (...) {
				delete[] new_data;
				delete[] data_;
				throw;
			}

			delete[] data_;

			data_ = new_data;
			capacity_ = new_size;
		}

		size_ = new_size;
	}

	void Reserve(size_t new_capacity) {
		if (new_capacity <= capacity_) {
			return;
		}

		Type* new_data = new Type[new_capacity];
		try {
			std::move(data_, data_ + size_, new_data);
		}
		catch (...) {
			delete[] new_data;
			delete[] data_;
			throw;
		}

		delete[] data_;

		data_ = new_data;
		capacity_ = new_capacity;
	}

	Iterator begin() noexcept {
		return data_;
	}

	Iterator end() noexcept {
		return data_ + size_;
	}

	ConstIterator begin() const noexcept {
		return data_;
	}

	ConstIterator end() const noexcept {
		return data_ + size_;
	}

	ConstIterator cbegin() const noexcept {
		return data_;
	}

	ConstIterator cend() const noexcept {
		return data_ + size_;
	}
private:
	Type* data_{};
	size_t size_{};
	size_t capacity_{};
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	if (lhs.GetSize() != rhs.GetSize()) {
		return false;
	}
	return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return !(lhs < rhs);
}