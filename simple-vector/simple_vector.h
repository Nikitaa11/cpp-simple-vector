#pragma once

#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <algorithm>
#include "array_ptr.h"

// Класс-обертка для резервирования памяти
class ReserveProxyObj {
public:
    explicit ReserveProxyObj(size_t capacity)
        : capacity_to_reserve_(capacity) {
    }

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

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) : SimpleVector(size, Type{}) {
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        : items_(size > 0 ? ArrayPtr<Type>(size) : ArrayPtr<Type>())
        , size_(size)
        , capacity_(size) {
        if (size > 0) {
            std::fill(items_.Get(), items_.Get() + size, value);
        }
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
        : items_(init.size() > 0 ? ArrayPtr<Type>(init.size()) : ArrayPtr<Type>())
        , size_(init.size())
        , capacity_(init.size()) {
        if (init.size() > 0) {
            std::copy(init.begin(), init.end(), items_.Get());
        }
    }

    // Конструктор с резервированием памяти
    explicit SimpleVector(ReserveProxyObj capacity)
        : items_(capacity.capacity_to_reserve_ > 0 ? ArrayPtr<Type>(capacity.capacity_to_reserve_) : ArrayPtr<Type>())
        , size_(0)
        , capacity_(capacity.capacity_to_reserve_) {
    }

    // Конструктор копирования
    SimpleVector(const SimpleVector& other)
        : items_(other.capacity_ > 0 ? ArrayPtr<Type>(other.capacity_) : ArrayPtr<Type>())
        , size_(other.size_)
        , capacity_(other.capacity_) {
        if (other.capacity_ > 0) {
            std::copy(other.items_.Get(), other.items_.Get() + other.size_, items_.Get());
        }
    }

    // Конструктор перемещения
    SimpleVector(SimpleVector&& other) noexcept {
        swap(other);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            if (rhs.IsEmpty()) {
                Clear();
                return *this;
            }
            SimpleVector<Type> tmp(rhs);
            swap(tmp);
        }
        return *this;
    }

    // Оператор присваивания перемещением
    SimpleVector& operator=(SimpleVector&& rhs) noexcept {
        if (this != &rhs) {
            swap(rhs);
        }
        return *this;
    }

    // Деструктор
    ~SimpleVector() = default;

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (items_.Get() == nullptr) {
            throw std::out_of_range("Vector is empty");
        }
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& At(size_t index) const {
        if (items_.Get() == nullptr) {
            throw std::out_of_range("Vector is empty");
        }
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size <= capacity_) {
            size_ = new_size;
            return;
        }

        ArrayPtr<Type> new_items(new_size);
        if (items_.Get()) {
            for (size_t i = 0; i < size_; ++i) {
                new_items[i] = std::move(items_[i]);
            }
        }
        for (size_t i = size_; i < new_size; ++i) {
            new_items[i] = Type{};
        }
        items_ = std::move(new_items);
        size_ = new_size;
        capacity_ = new_size;
    }

    // Возвращает итератор на начало массива
    Iterator begin() noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    Iterator end() noexcept {
        return items_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    ConstIterator begin() const noexcept {
        return items_.Get();
    }

    // Возвращает константный итератор на элемент, следующий за последним
    ConstIterator end() const noexcept {
        return items_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    ConstIterator cbegin() const noexcept {
        return items_.Get();
    }

    // Возвращает константный итератор на элемент, следующий за последним
    ConstIterator cend() const noexcept {
        return items_.Get() + size_;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (size_ == capacity_) {
            size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_items(new_capacity);
            if (items_.Get()) {
                std::copy(items_.Get(), items_.Get() + size_, new_items.Get());
            }
            items_ = std::move(new_items);
            capacity_ = new_capacity;
        }
        items_[size_++] = item;
    }

    // Добавляет элемент в конец вектора с использованием move-семантики
    void PushBack(Type&& item) {
        if (size_ == capacity_) {
            size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_items(new_capacity);
            if (items_.Get()) {
                std::move(items_.Get(), items_.Get() + size_, new_items.Get());
            }
            items_ = std::move(new_items);
            capacity_ = new_capacity;
        }
        items_[size_++] = std::move(item);
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= begin() && pos <= end());
        size_t index = pos - begin();
        if (size_ == capacity_) {
            size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_items(new_capacity);
            std::copy(items_.Get(), items_.Get() + index, new_items.Get());
            new_items[index] = value;
            std::copy(items_.Get() + index, items_.Get() + size_, new_items.Get() + index + 1);
            items_ = std::move(new_items);
            capacity_ = new_capacity;
        } else {
            std::move_backward(items_.Get() + index, items_.Get() + size_, items_.Get() + size_ + 1);
            items_[index] = value;
        }
        ++size_;
        return items_.Get() + index;
    }

    // Вставляет значение value в позицию pos с использованием move-семантики
    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= begin() && pos <= end());
        size_t index = pos - begin();
        if (size_ == capacity_) {
            size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_items(new_capacity);
            std::move(items_.Get(), items_.Get() + index, new_items.Get());
            new_items[index] = std::move(value);
            std::move(items_.Get() + index, items_.Get() + size_, new_items.Get() + index + 1);
            items_ = std::move(new_items);
            capacity_ = new_capacity;
        } else {
            std::move_backward(items_.Get() + index, items_.Get() + size_, items_.Get() + size_ + 1);
            items_[index] = std::move(value);
        }
        ++size_;
        return items_.Get() + index;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if (size_ > 0) {
            --size_;
        }
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());
        size_t index = pos - begin();
        std::move(items_.Get() + index + 1, items_.Get() + size_, items_.Get() + index);
        --size_;
        return items_.Get() + index;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Резервирует память для хранения как минимум new_capacity элементов
    void Reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) {
            return;
        }
        ArrayPtr<Type> new_items(new_capacity);
        if (items_.Get()) {
            std::copy(items_.Get(), items_.Get() + size_, new_items.Get());
        }
        items_ = std::move(new_items);
        capacity_ = new_capacity;
    }

private:
    ArrayPtr<Type> items_;
    size_t size_ = 0;
    size_t capacity_ = 0;
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