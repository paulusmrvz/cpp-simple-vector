#pragma once

#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <utility>

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

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(size_t size) {
        data_ = new Type[size]{};
        size_ = capacity_ = size;
        std::uninitialized_default_construct_n(data_, size);
    }

    SimpleVector(size_t size, const Type& value) {
        data_ = new Type[size];
        size_ = capacity_ = size;
        std::uninitialized_fill_n(data_, size, value);
    }

    SimpleVector(std::initializer_list<Type> init) {
        data_ = new Type[init.size()];
        size_ = capacity_ = init.size();
        std::uninitialized_copy(std::make_move_iterator(init.begin()), std::make_move_iterator(init.end()), data_);
    }

    SimpleVector(const ReserveProxyObj& reserve)
        : SimpleVector() {
        Reserve(reserve.GetCapacity());
    }

    SimpleVector(ReserveProxyObj&& reserve)
        : SimpleVector() {
        Reserve(reserve.GetCapacity());
    }

    SimpleVector(const SimpleVector& other) {
        data_ = new Type[other.capacity_];
        size_ = other.size_;
        capacity_ = other.capacity_;
        std::uninitialized_copy(other.data_, other.data_ + size_, data_);
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
            Clear();
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
        std::destroy_n(data_, size_);
        delete[] data_;
    }

    void PushBack(const Type& item) {
        if (size_ == capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : 2 * capacity_;
            Type* new_data = new Type[new_capacity];
            std::uninitialized_move_n(data_, size_, new_data);
            std::destroy_n(data_, size_);
            delete[] data_;
            data_ = new_data;
            capacity_ = new_capacity;
        }
        new(data_ + size_) Type(item);
        ++size_;
    }

    void PushBack(Type&& item) {
        if (size_ == capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : 2 * capacity_;
            Type* new_data = new Type[new_capacity];
            std::uninitialized_move_n(data_, size_, new_data);
            std::destroy_n(data_, size_);
            delete[] data_;
            data_ = new_data;
            capacity_ = new_capacity;
        }
        new(data_ + size_) Type(std::move(item));
        ++size_;
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        ptrdiff_t index = pos - data_;
        if (size_ == capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : 2 * capacity_;
            Type* new_data = new Type[new_capacity];
            std::uninitialized_move_n(data_, index, new_data);
            new(new_data + index) Type(value);
            std::uninitialized_move_n(data_ + index, size_ - index, new_data + index + 1);
            std::destroy_n(data_, size_);
            delete[] data_;
            data_ = new_data;
            capacity_ = new_capacity;
        }
        else {
            std::uninitialized_move_n(data_ + index, size_ - index, data_ + index + 1);
            data_[index] = value;
        }
        ++size_;
        return data_ + index;
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        ptrdiff_t index = pos - data_;
        if (size_ == capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : 2 * capacity_;
            Type* new_data = new Type[new_capacity];
            std::uninitialized_move_n(data_, index, new_data);
            new(new_data + index) Type(std::move(value));
            std::uninitialized_move_n(data_ + index, size_ - index, new_data + index + 1);
            std::destroy_n(data_, size_);
            delete[] data_;
            data_ = new_data;
            capacity_ = new_capacity;
        }
        else {
            std::uninitialized_move_n(data_ + index, size_ - index, data_ + index + 1);
            data_[index] = std::move(value);
        }
        ++size_;
        return data_ + index;
    }

    void PopBack() noexcept {
        std::destroy_at(data_ + size_ - 1);
        --size_;
    }

    Iterator Erase(ConstIterator pos) {
        ptrdiff_t index = pos - data_;
        std::move(data_ + index + 1, data_ + size_, data_ + index);
        std::destroy_at(data_ + size_ - 1);
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
        return data_[index];
    }

    const Type& operator[](size_t index) const noexcept {
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
        std::destroy_n(data_, size_);
        size_ = 0;
    }

    void Resize(size_t new_size) {
        if (new_size > capacity_) {
            Type* new_data = new Type[new_size];
            std::uninitialized_copy_n(std::make_move_iterator(data_), size_, new_data);

            std::destroy_n(data_, size_);

            delete[] data_;

            data_ = new_data;
            capacity_ = new_size;
        }
        if (new_size > size_) {
            std::uninitialized_value_construct_n(data_ + size_, new_size - size_);
        }
        else if (new_size < size_) {
            std::destroy_n(data_ + new_size, size_ - new_size);
        }
        size_ = new_size;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) {
            return;
        }

        Type* new_data = new Type[new_capacity];
        std::uninitialized_move_n(data_, size_, new_data);
        std::destroy_n(data_, size_);
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
    for (std::size_t i = 0; i < lhs.GetSize(); ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    const std::size_t n = std::min(lhs.GetSize(), rhs.GetSize());
    for (std::size_t i = 0; i < n; ++i) {
        if (lhs[i] < rhs[i]) {
            return true;
        }
        else if (lhs[i] > rhs[i]) {
            return false;
        }
    }
    return lhs.GetSize() < rhs.GetSize();
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