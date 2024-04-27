#pragma once

#include <algorithm>
#include <cassert>
#include <utility>
#include <initializer_list>
#include "array_ptr.h"

class ReserveProxyObj {
   public:
   size_t capacity_;
   ReserveProxyObj(size_t capacity) { capacity_ = capacity; }
};

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit  SimpleVector(size_t size)
        : SimpleVector(size, Type()) {      
    }

    SimpleVector(size_t size, const Type& value) 
        : ptr_(size) 
        , size_(size)
        , capacity_(size)  
    {
        ExchangeEmptyWithValues(begin(), end(), value);
    }

    SimpleVector(std::initializer_list<Type> init) 
        : ptr_ (ArrayPtr<Type>(init.size()))
        , size_(init.size())
        , capacity_(init.size())
    {
        std::copy(init.begin(), init.end(), begin());
    }
        
    SimpleVector(ReserveProxyObj reserve) : SimpleVector(reserve.capacity_) {
        size_ = 0;
    }
    
    SimpleVector(const SimpleVector& other) {
        SimpleVector vector(other.size_);
        std::copy(other.begin(), other.end(), vector.begin());
        swap(vector);
    }
        
     SimpleVector(SimpleVector&& other) {
        ptr_ = std::move(other.ptr_);
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            if (rhs.IsEmpty()) {
                Clear();
                return *this;
            }
            SimpleVector tmp(rhs.GetSize());
            std::copy(rhs.begin(), rhs.end(), tmp.begin());
            tmp.capacity_ = rhs.GetCapacity();
            swap(tmp);
        }
        return *this;
    }
    
    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this != &rhs) {
            if (rhs.IsEmpty()) {
                Clear();
                return *this;
            }
            SimpleVector tmp(rhs.size_);
            std::move(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()), tmp.begin());
            std::exchange(tmp.capacity_, rhs.capacity_);
            swap(tmp);
        }
        return *this;
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
        return ptr_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return ptr_[index];
    }

    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index bigger than size");
        }
        else {
            return ptr_[index];
        } 
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Index bigger than size");
        }
        else {
            return ptr_[index];
        } 
    }

    void Clear() noexcept {
        size_ = 0;
    }

  void Resize(size_t new_size) {
        if (size_ > new_size) {
            size_ = new_size;
        } else if (capacity_ >= new_size) {
            ExchangeEmptyWithValues(end(), begin() + new_size, Type());
            size_ = new_size;
        } else {
            ArrayPtr<Type> new_ptr(new_size * 2);
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), &new_ptr[0]);
            ptr_.swap(new_ptr);
            capacity_ = new_size * 2;
            size_ = new_size;
        }
    }
    
    void PushBack(const Type& item) {
        if (size_ < capacity_) {
            ptr_[size_] = item;
            size_++;
        }
        else if (size_ == capacity_) {
            Resize(size_+ 1);
            ptr_[size_ - 1] = item;
        }
        else {
            size_t index = size_;
            size_t new_cap = capacity_ * 2;
            Resize(new_cap);
            ptr_[index] = item;
        }
    }

    void PushBack(Type&& item) {
        if (size_ < capacity_) {
            std::exchange(ptr_[size_], std::move(item));
            size_++;
        }
        else if (size_ == capacity_) {
            Resize(size_+ 1);
            std::exchange(ptr_[size_ - 1], std::move(item));
        }
        else {
            size_t index = size_;
            size_t new_cap = capacity_ * 2;
            Resize(new_cap);
            std::exchange(ptr_[index], std::move(item));
        }
    }
    
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= cbegin());
        assert(pos <= cend());
        int index = (distance(cbegin(), pos)); 
        if (size_ + 1 <= capacity_) {
            std::copy(begin() + index, end(), begin() + index + 1);
            ptr_[index] = value;
            size_++;
            return &ptr_[index];
        }
        else {
            if (capacity_ == 0) {
                capacity_ = 1;
            }
            ArrayPtr<Type> new_v(capacity_ * 2);
            std::copy(cbegin(), pos, &new_v[0]);
            std::copy_backward(pos, cend(), &new_v[size_ + 1]);
            new_v[index] = value;
            ++size_;
            capacity_ *= 2;
            ptr_.swap(new_v); 
            return &ptr_[index];
        }
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= cbegin());
        assert(pos <= cend());
        int index = (std::distance(cbegin(), pos));
        if (size_ + 1 <= capacity_) {
            std::move(std::make_move_iterator(begin() + index), std::make_move_iterator(end()), begin() + index + 1);
            std::exchange(ptr_[index], std::move(value));
            size_++;
            return &ptr_[index];
        }
        else {
            if (capacity_ == 0) {
                capacity_++;
            }
            ArrayPtr<Type> new_v(capacity_ * 2);
            std::move(std::make_move_iterator(begin()), std::make_move_iterator(begin() + index), &new_v[0]);
            std::move_backward(std::make_move_iterator(begin() + index), std::make_move_iterator(end()), &new_v[size_ + 1] + index + 1);
            std::exchange(new_v[index], std::move(value));
            ++size_;
            capacity_ *= 2;
            ptr_.swap(new_v); 
            return &ptr_[index];
        }
    }
    
    void PopBack() noexcept {
        if (size_ > 0) {
            --size_;
        }
    }
    
    void Reserve(size_t new_capacity) {
        if (capacity_ >= new_capacity) {
            return;
        }
        size_t temp = size_;
        ArrayPtr<Type> new_v(new_capacity);
        std::copy(cbegin(), cend(), &new_v[0]);
        size_ = temp;
        capacity_ = new_capacity;
        ptr_.swap(new_v); 
    }

    Iterator Erase(ConstIterator pos) {
        assert(pos >= cbegin());
        assert(pos < cend());
        Iterator mutable_pos = begin() + (pos - cbegin());
        std::copy(std::make_move_iterator(mutable_pos + 1), std::make_move_iterator(end()), mutable_pos);
        --size_;
        return mutable_pos;
    }

    void swap(SimpleVector& other) noexcept {
        ptr_.swap(other.ptr_);
        std::swap(other.capacity_, capacity_);
        std::swap(other.size_, size_);
    }
    
    Iterator begin() noexcept {
        return ptr_.Get();
    }
    
    Iterator end() noexcept {
        return ptr_.Get() + size_;
    }
    
    ConstIterator begin() const noexcept {
        return ptr_.Get();
    }

    ConstIterator end() const noexcept {
        return ptr_.Get() + size_;
    }

    ConstIterator cbegin() const noexcept {
        return ptr_.Get();
    }

    ConstIterator cend() const noexcept {
        return ptr_.Get() + size_;
    }
    
    private:
    
    ArrayPtr<Type> ptr_;
    size_t size_ = 0;
    size_t capacity_ = 0;
    
    void ExchangeEmptyWithValues(const Iterator begin, const Iterator end, Type value) {
        auto it = begin;
        while (it != end) {
            std::exchange(*it, std::move(value));
            it++;
        }
    }  
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
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
    return (rhs < lhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}
    
ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}; 
