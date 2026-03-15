#pragma once
#include <iostream>
#include <stdlib.h>

#include <initializer_list>
#include <string>
#include <functional>
#include "../util/basic.hpp"

#define DISABLE_BOUNDS_CHECK 1

struct d_object {};


template <typename T>
class list : public d_object {
protected:
    T* ptr;
    size_t size_;
    size_t capacity_;
public:
    list() {
        size_ = 0;
        capacity_ = 0;
        ptr = nullptr;
    }

    list(size_t cap) {
        size_ = 0;
        capacity_ = cap;
        ptr = (capacity_ > 0) ? new T[capacity_] : nullptr;
    }

    list(size_t cap,T def) {
        size_ = cap;
        capacity_ = cap;
        ptr = (capacity_ > 0) ? new T[capacity_] : nullptr;
        for(int i = 0;i<cap;i++) {
            ptr[i] = def;
        }
    }

    list(std::initializer_list<T> values) {
    size_ = 0;
    capacity_ = values.size();
    ptr = (capacity_ > 0) ? new T[capacity_] : nullptr;
    for (const T& value : values) {
        push(value);
    }
    }

    list(list&& other) noexcept {
        ptr = other.ptr;
        size_ = other.size_;
        capacity_ = other.capacity_;

        other.ptr = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    list(const list& other) noexcept {
        size_ = other.size_;
        capacity_ = other.capacity_;

        if (capacity_ > 0) {
        ptr = new T[capacity_];
        for (size_t i = 0; i < size_; ++i) {
            ptr[i] = other.ptr[i];
        }
        } else {
            ptr = nullptr;
        }
    }

    list<T>& operator=(const list<T>& other) {
    if (this != &other) {
        destroy();
        size_ = other.size_;
        capacity_ = other.capacity_;
        if (capacity_ > 0) {
            ptr = new T[capacity_];
            for (size_t i = 0; i < size_; ++i) {
                ptr[i] = other.ptr[i];
            }
        } else {
            ptr = nullptr;
        }
    }
    return *this;
    }

    list<T>& operator=(list<T>&& other) noexcept {
        if (this == &other) return *this; // self-assignment safety
    
        // Steal data
        T* oldPtr = ptr;
        ptr = other.ptr;
        size_ = other.size_;
        capacity_ = other.capacity_;
    
        // Reset the other
        other.ptr = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    
        // Only delete after stealing to avoid alias corruption
        if (oldPtr) delete[] oldPtr;
    
        return *this;
    }
    

    ~list() {
        destroy();
    }

    inline size_t length() const {return size_;}
    inline size_t size() const {return size_;}
    inline size_t space() const { return capacity_; }
    inline size_t capacity() const { return capacity_; }
    inline bool empty() const {return length()==0;}
    inline T& last() {return ptr[size_-1];}
    inline T& first() {return ptr[0];}

    //This implmentation is flawed, or at least, OpenGL doesn't play nice with it
    T* data() noexcept { return ptr; }
    const T* data() const noexcept { return ptr; }

    inline T* begin() {return ptr;}
    inline T* end() {return ptr+size_;}

    inline T* begin() const {return ptr;}
    inline T* end() const {return ptr+size_;}

    void pushAll(const list<T>& input) {
        for(int i = 0;i<input.size_;i++)
        {
            push(input[i]);
        }
    }

    // void printAll() const {
    //     for(size_t i = 0;i<size;i++)
    //     {
    //         std::cout << ptr[i] << std::endl;
    //     }
    // }
   
    /// @brief Pushes all values of the input to this list
    list<T>&  operator<<(const list<T>& input) {
        for(size_t i = 0;i<input.size_;i++)
        {
            push(input.get(i));
        }
        return *this;
    }

    /// @brief Pushes all values of this list to the output
    void operator>>(list<T>& output) {
        for(size_t i = 0;i<size_;i++)
        {
            output.push(get(i));
        }
    }
    
    template <typename... Args>
    void pushAll(Args... args) {
        (push(args),...);
    }

    void destroy() {
        if (ptr) {
        delete[] ptr;
        ptr = nullptr;
        }
        size_ = 0;
        capacity_ = 0;
    }

    void clear() {
        if constexpr (std::is_trivially_destructible_v<T>) {
            size_ = 0;
        } else {
            for (size_t i = 0; i < size_; ++i) {
                ptr[i].~T();
            }
            size_ = 0;
        }
    }

    void merge(list<T>& input) {
        for(size_t i = 0;i<input.size_;i++)
        {
            push(input.get(i));
        }
        input.destroy();
    }

    /// @brief Merges the input to this list, erasing the input
    void operator<=(list<T>& input) {
        for(size_t i = 0;i<input.size_;i++)
        {
            push(input.get(i));
        }
        input.destroy();
    }

 /// @brief Merges this list to the output, erasing it
    void operator>=(list<T>& output) {
        for(size_t i = 0;i<size_;i++)
        {
            output.push(get(i));
        }
        destroy();
    }

    void operator>=(list<T>&& output) {
        for(size_t i = 0;i<size_;i++)
        {
            output.push(get(i));
        }
        destroy();
    }
    
    template <typename Func>
    void forEach(Func&& function)
    {
        for(size_t i=0;i<size_;i++)
        {
            function(ptr[i]);
        }
    }

    template <typename Func>
     /// @brief Executes a function on each of the items in the list
    void operator()(Func&& function)
    {
        for(size_t i=0;i<size_;i++)
        {
            function(ptr[i]);
        }
    }

    /// @brief Returns the index of a value in the list, similar to keylist, returns -1 if not found
    int find(const T& v) {
        for(int i=0;i<size_;i++) {
            if(ptr[i] == v) return i;
       }
       return -1;
    }

    /// @brief Returns a single index matching the function! -1 if nothing is found
    int find_if(std::function<bool(const T&)> pred) {
        for (int i = 0; i < size_; ++i) {
            if (pred(ptr[i])) return i;
        }
        return -1;
    }

    /// @brief Removes a single value from the list based on the function
    void erase_if (std::function<bool(const T&)> pred) {
        int f = find_if(pred);
        if(f!=-1) {removeAt(f);}
    }

    /// @brief Removes a value from the list based on type rather than just index, uses find
    void erase(const T& v) {
        int f = find(v);
        if(f!=-1) {removeAt(f);}
    }


    template<typename TT>
    void insert(TT&& value,size_t index)
    {
        if (index > size_) {
            if(index==0&&size_==0) {
                push(value);
                return;
            } else {
                throw std::out_of_range("list::insert::212 Insert index, "+std::to_string(index)+" out of size: "+std::to_string(size_));
            }
        }
        push(value);
        for (std::size_t i = size_ - 1; i > index; --i) {
            ptr[i] = std::move(ptr[i - 1]);
        }
        ptr[index] = std::forward<TT>(value);
    }

    template<typename TT>
    inline void push(TT&& value) {
        if (size_ >= capacity_) 
        {
        capacity_ = capacity_ == 0 ? 4 : capacity_ * 2; 
        T* newPtr = new T[capacity_];
        for (size_t i = 0; i < size_; ++i) {
             newPtr[i] = std::move(ptr[i]);
        }
        delete[] ptr;
        ptr = newPtr;
        }
        new (&ptr[size_]) T(std::forward<TT>(value));
        ++size_;
    }

    /// @brief a conditonal push that ensures the list does not already have an entry first
    template<typename TT>
    void push_if_absent(TT&& value) {
        if(!has(value)) push(value);
    }


    void reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) return;
        
        T* newPtr = new T[new_capacity];
        for (size_t i = 0; i < size_; ++i) {
            newPtr[i] = std::move(ptr[i]);
        }
        
        delete[] ptr;
        ptr = newPtr;
        capacity_ = new_capacity;
    }

    void resize(size_t new_size) {
        if (new_size > capacity_) {
            reserve(new_size);
        }
        size_ = new_size;
    }

    void shrink_to_fit() {
        if (size_ == capacity_ || capacity_ == 0) return;
        
        if (size_ == 0) {
            delete[] ptr;
            ptr = nullptr;
            capacity_ = 0;
            return;
        }
        
        T* newPtr = new T[size_];
        for (size_t i = 0; i < size_; ++i) {
            newPtr[i] = std::move(ptr[i]);
        }
        
        delete[] ptr;
        ptr = newPtr;
        capacity_ = size_;
    }

    /// @brief Pushes a value to the list
    /// @param value the value to be pushed
    list<T>& operator<<(T value) {
        push(value);
        return *this;
    }

    T pop() {
        if (size_ == 0) throw std::out_of_range("ERROR: List is empty");

        T val = ptr[size_ - 1];
        --size_;
        return val;
    }

    void removeAt(size_t index) {
        if (index >= size_) throw std::out_of_range("ERROR: Remove index out of bounds!");
        for (size_t i = index; i + 1 < size_; ++i) {
            ptr[i] = ptr[i + 1];
        }
        --size_;
    }

    T take(size_t index) {
        T val = ptr[index];
        removeAt(index);
        return val;
    }

    // T& rand() {return }
    
    inline T& get(size_t index) {
        #if !DISABLE_BOUNDS_CHECK
        if(index >= size_) {
            throw std::out_of_range("Util 265: List out of Bounds");
        }
        #endif
        return ptr[index];
    }

    inline const T& get(size_t index) const {
        if(index >= size_) {
            throw std::out_of_range("Util 268: List out of Bounds");
        }
        return ptr[index];
    }

    inline T& get(size_t index,const std::string& from) {
        if(index >= size_) {
            throw std::out_of_range("Util 275: List out of Bounds from \n  "+from);
        }
        return ptr[index];
    }

    inline T& operator[](size_t index) {
    #if !DISABLE_BOUNDS_CHECK
    if(index >= size_) {
        throw std::out_of_range("Util 265: List out of Bounds");
    }
    #endif
      return ptr[index];
    }

    inline const T& operator[](size_t index) const {
    #if !DISABLE_BOUNDS_CHECK
        if(index >= size_) {
            throw std::out_of_range("Util 268: List out of Bounds");
        }
    #endif
        return ptr[index];
    }

    inline T& rand() {
        return ptr[randi(0,size_-1)];
    }

    inline T& rand() const {
        return ptr[randi(0,size_-1)];
    }


    inline void shuffle() {
        for(int i = size_ - 1; i > 0; i--) {
            int j = randi(0, i);
            T& temp = ptr[i];
            ptr[i] = ptr[j];
            ptr[j] = temp;
        }
    }

    //This is crude and temporary
    inline void sort(std::function<bool(T,T)> func) {
        std::vector<T> temp_vec;
        for(int i=0;i<size_;i++) {
            temp_vec.push_back(ptr[i]);
        }
        std::sort(temp_vec.begin(), temp_vec.end(), func);
        clear();
        for(int i=0;i<temp_vec.size();i++) {
            push(temp_vec[i]);
        }
    }
    
    inline void reverse() {
        T* left = ptr;
        T* right = ptr + size_;
        
        if (left >= right) return;
        --right;
        
        while (left < right) {
            std::swap(*left++, *right--);
        }
    }

    inline void swap(size_t from, size_t to) {
        std::swap(ptr[from], ptr[to]);
    }

    template<typename TT>
    /// @brief Compares two lists and returns true if they are equivalent.
    bool operator==(list<TT>& other) {
        if(other.size_!=size_) return false;
        for(size_t i=0;i<size_;i++)
        {
           if(other[i]!=ptr[i]) return false;
        }
        return true;
    }

    template<typename TT>
    /// @brief Compares two lists and returns true if they are equivalent.
    bool operator==(const list<TT>& other) const {
        if(other.size_!=size_) return false;
        for(size_t i=0;i<size_;i++)
        {
           if(other[i]!=ptr[i]) return false;
        }
        return true;
    }

    template<typename TT>
    /// @brief Compares two lists and returns false if they are equivalent.
    bool operator!=(list<TT>& other) {
        return !(*this == other);
    }

    /// @brief Returns if the list contains an instance of a given value
    bool has(T search) const {
        for(size_t i=0;i<size_;i++)
        {
            if(ptr[i]==search) return true;
        }
        return false;
    }

};
template <typename T>
list(std::initializer_list<T>) -> list<T>;



    // template<typename... Args>
    // void emplace(Args&&... args) {
    //     if (size >= capacity) {
    //         capacity = capacity == 0 ? 4 : capacity * 2;
    //         T* newPtr = new T[capacity];
    //         for (size_t i = 0; i < size; ++i) {
    //             newPtr[i] = std::move(ptr[i]);
    //         }
    //         delete[] ptr;
    //         ptr = newPtr;
    //     }
    //     ptr[size++] = T(std::forward<Args>(args)...); // Constructs in-place
    // }