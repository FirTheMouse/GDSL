#pragma once

#include <future>
#include <deque>
#include <mutex>
#include <cmath>
#include <atomic>
#include <chrono>
#include <random>
#include <functional>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <string>
#include <sstream>
#include <fstream>
#include <initializer_list>
#include <stdlib.h>
#include <any>
#include <thread>
template<typename... Args>
void print(Args&&... args) {
  (std::cout << ... << args) << std::endl;
}

template<typename... Args>
void printnl(Args&&... args) {
  (std::cout << ... << args);
}


static inline std::string lime(const std::string& text) {
  return "\x1b[92m"+text+"\x1b[0m";
}
static inline std::string green(const std::string& text) {
  return "\x1b[32m"+text+"\x1b[0m";
}
static inline std::string pine(const std::string& text) {
  return "\x1b[2;32m"+text+"\x1b[0m";
}
static inline std::string yellow(const std::string& text) {
  return "\x1b[33m"+text+"\x1b[0m";
}
static inline std::string red(const std::string& text) {
  return "\x1b[31m"+text+"\x1b[0m";
}
static inline std::string blue(const std::string& text) {
  return "\x1b[34m"+text+"\x1b[0m";
}
static inline std::string magenta(const std::string& text) {
  return "\x1b[35m"+text+"\x1b[0m";
}
static inline std::string cyan(const std::string& text) {
  return "\x1b[36m"+text+"\x1b[0m";
}
static inline std::string white(const std::string& text) {
  return "\x1b[37m"+text+"\x1b[0m";
}
static inline std::string gray(const std::string& text) {
  return "\x1b[90m"+text+"\x1b[0m";
}

static inline std::string bold_str(const std::string& text) {
  return "\x1b[1m"+text+"\x1b[0m";
}
static inline std::string dim_str(const std::string& text) {
  return "\x1b[2m"+text+"\x1b[0m";
}
static inline std::string underline_str(const std::string& text) {
  return "\x1b[4m"+text+"\x1b[0m";
}

static inline std::string italic_str(const std::string& s) {
  return "\033[3m" + s + "\033[23m";
}


static inline std::string rgb(const std::string& text, int r, int g, int b) {
  return "\x1b[38;2;"+std::to_string(r)+";"+std::to_string(g)+";"+std::to_string(b)+"m"+text+"\x1b[0m";
}
static inline std::string bg(const std::string& text, int r, int g, int b) {
  return "\x1b[48;2;"+std::to_string(r)+";"+std::to_string(g)+";"+std::to_string(b)+"m"+text+"\x1b[0m";
}


std::string to_bin(uint32_t n) {
  std::string s = "";
  for(int i = 31; i >= 0; i--) {
      s += ((n >> i) & 1) ? '1' : '0';
  }
  return s;
}

std::string to_hex(uint32_t n) {
  const char digits[] = "0123456789ABCDEF";
  std::string s = "0x";
  for(int i = 28; i >= 0; i -= 4) {
      s += digits[(n >> i) & 0b1111];
  }
  return s;
}

static std::string ftime(double t) 
{
  if(t >= 100000000) {
      return red(std::to_string(t/1000000000.0)+"s");
  } else if(t >= 100000) {
      return yellow(std::to_string(t/1000000.0)+"ms");
  } else { 
      return  green(std::to_string(t/1000.0)+"ns");
  } 
}

namespace Log {
  class Line {
      public:
      Line() {}
  
      ~Line() {}

      std::string label_;
      std::chrono::steady_clock::time_point start_;
      double total_time_ = 0.0;

      void start() {
          start_ = std::chrono::steady_clock::now();
      }

    double time_ns() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();
        return (double)duration;
    }

    double time_s() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration<double>(end - start_).count();
        return duration;
    }

      double end() {
          auto end = std::chrono::steady_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();
          total_time_ += duration;
          return (double)duration;
      }
      
  };
}

inline float randf(float min, float max)
{
    // One engine per thread; seeded once from real entropy.
    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);                  // [min, max) – max exclusive by default
}

inline int randi(int min, int max)
{
    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<int> dist(min, max); // inclusive both ends
    return dist(rng);
}

inline void clamp(float& value, float min, float max) {
  value = std::max(min,std::min(max,value));
}



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

    void insertAll(const list<T>& input, size_t index) {
        for(int i = input.size_-1;i>=0;i--) {
            insert(input[i],index);
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


class q_object {
    protected:
        mutable std::atomic<int> refCount{0};
        std::atomic<bool> tombstone{true};
    
    public:
        q_object() {}
        virtual ~q_object() {}
        // Explicitly delete copy operations
        q_object(const q_object&) = delete;
        q_object& operator=(const q_object&) = delete;

        // Properly implement move operations
        q_object(q_object&& other) noexcept 
            : refCount(other.refCount.load()), tombstone(other.tombstone.load()) {
            // Reset the moved-from object
            other.refCount.store(0);
            other.tombstone.store(false);
        }

        q_object& operator=(q_object&& other) noexcept {
            if (this != &other) {
                refCount.store(other.refCount.load());
                tombstone.store(other.tombstone.load());
                // Reset the moved-from object
                other.refCount.store(0);
                other.tombstone.store(false);
            }
            return *this;
        }

        void stop() {tombstone.store(false);}
        void resurrect() {tombstone.store(true);}
        bool isActive() {return tombstone.load();}
    
        void retain() { ++refCount; }
        virtual void release() {
            if (refCount.fetch_sub(1) == 1) {
                delete this;
            }
        }

        int getRefCount() const {
            return refCount.load();
        }
    };
    
    template<typename T>
    class g_ptr {
        //static_assert(std::is_base_of<Object, T>::value, "T must inherit from Object");
    
        T* ptr = nullptr;
    
    public:
        g_ptr() = default;
    
        g_ptr(T* raw) : ptr(raw) {
            if (ptr) ptr->retain();
        }
    
        g_ptr(const g_ptr<T>& other) : ptr(other.ptr) {
            if (ptr) ptr->retain();
        }
    
        g_ptr(g_ptr<T>&& other) noexcept : ptr(other.ptr) {
            other.ptr = nullptr;
        }
    
        ~g_ptr() {
            if (ptr) ptr->release();
        }
    
        g_ptr<T>& operator=(const g_ptr<T>& other) {
            if (this != &other) {
                if (ptr) ptr->release();
                ptr = other.ptr;
                if (ptr) ptr->retain();
            }
            return *this;
        }
    
        g_ptr<T>& operator=(g_ptr<T>&& other) noexcept {
            if (this != &other) {
                if (ptr) ptr->release();
                ptr = other.ptr;
                other.ptr = nullptr;
            }
            return *this;
        }
    
        T* operator->() const { return ptr; }
        T& operator*() const { return *ptr; }
        T* getPtr() const { return ptr; }

        friend bool operator==(const g_ptr<T>& lhs, const g_ptr<T>& rhs) {
            return lhs.ptr == rhs.ptr;
        }
        
        friend bool operator!=(const g_ptr<T>& lhs, const g_ptr<T>& rhs) {
            return lhs.ptr != rhs.ptr;
        }

        explicit operator bool() const { return ptr != nullptr; }

        template<typename U>
        operator g_ptr<U>() const {
            static_assert(std::is_base_of<U, T>::value, "Can only convert to base types");
            return g_ptr<U>(ptr);
        }

    };
    
    template<typename T, typename... Args>
    g_ptr<T> make(Args&&... args) {
        static_assert(std::is_base_of<q_object, T>::value, "make<T>: T must derive from Object");
        T* obj = new T(std::forward<Args>(args)...);
        return g_ptr(obj);
    }

    template<typename T, typename U>
    g_ptr<T> g_dynamic_pointer_cast(const g_ptr<U>& from) {
        static_assert(std::is_base_of<q_object, U>::value, "U must inherit from Object");
        static_assert(std::is_base_of<q_object, T>::value, "T must inherit from Object");
        
        if (!from) return g_ptr<T>(nullptr);
        
        T* casted = dynamic_cast<T*>(from.getPtr());
        if (casted) {
            return g_ptr<T>(casted);
        } else {
            return g_ptr<T>(nullptr);
        }
   }

   template<typename T, typename U>
   g_ptr<T> as(const g_ptr<U>& from) {
        return g_dynamic_pointer_cast<T>(from);
   }




template<typename K,typename V>
struct entry
{
    K key;
    V value;
    entry() = default;
    entry(const K& k, const V& v) : key(k), value(v) {}
    entry(K&& k, V&& v) : key(std::move(k)), value(std::move(v)) {}
    entry(const entry& other) = default;
    entry(entry&& other) noexcept = default;
    entry& operator=(const entry& other) = default;
    entry& operator=(entry&& other) noexcept = default;
};


template<typename K,typename V>
class keylist : public list<entry<K,V>>
{
private:

public:
    using base = list<entry<K, V>>;

    //Add more control here in the future, with conventions for const and such and r/l
    template<typename KK, typename VV>
    void put(KK&& key, VV&& value) {
        *this << entry<K,V>(std::forward<KK>(key), std::forward<VV>(value));
    }

    template<typename EE>
    void put(EE&& e) {
        *this << std::forward<EE>(e);
    }

    V& get(const K& key){
       for(entry<K,V>& e : *this){
            if(e.key == key) return e.value;
       }
       //This is so that ASAN can trace the error origin 
       std::vector<int> ints;
       ints.push_back(1);
       volatile int b = ints[3];
       throw std::runtime_error("map::43 key not found ");
    }

    list<V> getAll(const K& key){
        list<V> l;
        for(entry<K,V>& e : *this){
             if(e.key == key) l << e.value;
        }
        return l;
     }

    list<V> allValues() {
        list<V> l;
        for(entry<K,V>& e : *this){
             l << e.value;
        }
        return l;
     }

    template<typename VV>
    V& getOrDefault(const K& key,VV&& fallback){
       for(entry<K,V>& e : *this){
            if(e.key == key) return e.value;
       }
       return fallback;
    }

    bool hasKey(const K& key){
       for(entry<K,V>& e : *this){
            if(e.key == key) return true;
       }
       return false;
    }

    list<K> keySet() {  
        list<K> result;
        for(entry<K,V>& e : *this) result << e.key;
        return result;
    }

    list<entry<K,V>> entrySet() {
        list<entry<K,V>> result;
        for(entry<K,V>& e : *this) {
            result << e;
        }
        return result;
    }
    
    template<typename KK, typename VV>
    bool set(KK& key, VV&& value){
       for(entry<K,V>& e : *this){
            if(e.key == key) {
             e = entry<K,V>(std::forward<KK>(key), std::forward<VV>(value));
             return true;
            }
       }
       return false;
    }

    // V& operator[](const K& key) {
    // return get(key);
    // }

     bool has(const K& key){
       for(entry<K,V>& e : *this){
            if(e.key == key) return true;
       }
       return false;
    }

    bool remove(const K& key) {
    for (size_t i = 0; i < this->length(); ++i) {
        if (this->base::operator[](i).key == key) {
            this->removeAt(i);
            return true;
        }
    }
    return false;
    }
};

static inline uint32_t hashString(const std::string& str) {
    uint32_t hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

static inline uint32_t mix32(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return (uint32_t)x;
}

template<typename K,typename V>
class map
{
private:
   list<keylist<K,V>> buckets;
   size_t size_;
   size_t capacity;
public:
    map()
    {
        size_ = 0;
        capacity = 8;
        for(int i=0;i<capacity;i++)
        {
            buckets.push(keylist<K,V>());
        }
    }

    size_t size() {return size_;}


    uint32_t mix96(uint32_t a, uint32_t b, uint32_t c) {
        uint64_t combined = ((uint64_t)a << 32) | b;
        return mix32(combined) ^ mix32((uint64_t)c);
    }

    template<typename T>
    uint32_t hashT(const T& val)
    {
        if constexpr (std::is_same_v<T,std::string>) {
            return hashString(val);
        }
        else if constexpr (std::is_same_v<T,const char*>) {
            return hashString(string(val));
        }
        else if constexpr (std::is_array_v<T> && std::is_same_v<std::remove_extent_t<T>, char>) {
            return hashString(string(val));
        }
        else if constexpr (std::is_same_v<T,int>) {
            return val;
        } else if constexpr (std::is_pointer_v<T>) {
            auto addr = (std::uintptr_t)val;
            addr >>= 3; 
            return mix32((uint64_t)addr);
        } else if constexpr (sizeof(T)==12) {
            const uint32_t* parts = reinterpret_cast<const uint32_t*>(&val);
            return mix96(parts[0], parts[1], parts[2]);
        } else if constexpr (sizeof(T)==8) {
            return mix32((uint64_t)val);
        }
        else {
            return val;
        }
    };

    template<class U>
    uint32_t hashT(const g_ptr<U>& p)
    {
        return hashT(p.getPtr());
    }

    void put(const K& key,V value)
    {
        if(size_>=(capacity*2))
        {
            capacity = capacity*2;
            list<keylist<K,V>> newBuckets(capacity);
            for(int i=0;i<capacity;i++)
            {   
                newBuckets.push(keylist<K,V>());
            }
            for(keylist<K,V>& old : buckets)
            {
                // for(const auto& e : old.entrySet()) {
                //     newBuckets[(hashT(e.key)%capacity)].put(e.key,e.value);
                // }
                //For some reason the lambda is faster in testing
                old([&](const entry<K,V>& e){newBuckets[(hashT(e.key)%capacity)].put(e.key,e.value);});
            }
            buckets = std::move(newBuckets);

        }
        buckets[hashT(key)%capacity].put(key,value);
        size_++;
    }
    

    bool set(const K& key,V value) {
        return buckets.get(hashT(key)%capacity).set(key,value);
    }

    // keylist<K, V> b = buckets[hashT(key)%capacity];
    // if(b.hasKey(key)) return b.get(key);
    // else {throw}
    V& get(const K& key)
    {
       return buckets.get(hashT(key)%capacity).get(key);
    }

    keylist<K, V>& getBucket(const K& key) {
        return buckets.get(hashT(key)%capacity);
    }
    

    //Returns all values in the map
    list<V> getAll(){
        list<V> l;
        for(auto b : buckets) {
            for(auto v : b.allValues()) {
                l << v; }
        }
        return l;
     }

     //Reuturns all values associated with a key
     list<V> getAll(const K& key){
       return buckets.get(hashT(key)%capacity,"map::getAll::210").getAll(key);
     }



    template<typename VV>
    V& getOrDefault(const K& key,VV&& fallback)
    {
       return buckets[hashT(key)%capacity].getOrDefault(key,fallback);
    }

    bool hasKey(const K& key)
    {
       return buckets.get(hashT(key)%capacity).hasKey(key);
    }

    V& getOrPut(const K& key,const V& fallback) {
        if(!hasKey(key)) {
            put(key,fallback);
        }
        return get(key);
    }

    V& getOr(const K& key,std::function<V()> func) {
        if(!hasKey(key)) {
            put(key,func());
        }
        return get(key);
    }

    V& operator[](const K& key) {
        return getOrPut(key,V());
    }

    list<K> keySet() {  
        list<K> result;
        for(keylist<K,V>& e : buckets) e.keySet()>=result;
        return result;
    }

    list<entry<K,V>> entrySet() {
        list<entry<K,V>> result;
        for(const keylist<K,V>& e : buckets){
            for(int i=0;i<e.length();i++){
                result << e[i];
            }
        } 
        return result;
    }

    // list<entry<K,V>> entrySet() {
    //     list<entry<K,V>> result;
    //     for(int bucket_idx = 0; bucket_idx < buckets.length(); bucket_idx++) {
    //         keylist<K,V>& bucket = buckets[bucket_idx];
    //         for(int i = 0; i < bucket.length(); i++){
    //             result << bucket[i];
    //         }
    //     } 
    //     return result;
    // }

    void clear()
    {
        buckets([](keylist<K,V> keyl){keyl.base::destroy();});
        buckets.destroy();
        size_=0;
        capacity = 8;
        for(int i=0;i<capacity;i++)
        {
            buckets.push(keylist<K,V>());
        }
    }

    bool remove(const K& key) {
        int hash = (hashT(key)%capacity);
        if(hash<buckets.length())
            return buckets.get(hash).remove(key);
        else
            return false;
    }

    void debugMap() {
        buckets([](keylist<K,V> keyl){keyl([](entry<K,V> e){std::cout << e.key << std::endl;});});
    }
};
inline list<std::string> split_str(const std::string& s,char delimiter)
{
    list<std::string> toReturn;
    int last = 0;
    for(int i=0;i<s.length();i++)
    {
        if(s.at(i)==delimiter) {
            toReturn << s.substr(last,i-last);
            last = i+1;
        }
    }
    if(last<s.length())
    {
        toReturn << s.substr(last,s.length()-last);
    }
    return toReturn;
}

class Data{
public:
    Data() {}

    map<std::string,std::any> notes;

    template<typename T = std::string>
    void add(const std::string& label,T info)
    {
        notes.put(label,std::any(info));
    }

    template<typename T = std::string>
    T get(const std::string& label)
    {
        #if !DISABLE_BOUNDS_CHECK
            if(!has(label)) std::cerr << "Data does not have label " << label <<"\n";
        #endif
        return std::any_cast<T>(notes.get(label));
    }

    bool has(const std::string& label)
    {
        return notes.hasKey(label);
    }

    bool check(const std::string& label)
    {
        if(!has(label)) return false;
        try {
            return get<bool>(label);
        }
        catch(std::exception e)
        {
            print("data::check::59 Attempted to check a non-bool in data");
            //Or just return false?
            return false;
        }
    }

    bool toggle(const std::string& label) {
        if(!has(label)) set<bool>(label,true);
        bool toReturn = !get<bool>(label);
        set<bool>(label,toReturn);
        return toReturn;
    }

    void flagOn(const std::string& label) {set<bool>(label,true);}
    void flagOff(const std::string& label) {set<bool>(label,false);}

    template<typename T>
    void set(const std::string& label,T info) {
        if(!notes.set(label,info))
            add<T>(label,info);
    }

    template<typename T = int>
    T inc(const std::string& label,T by)
    {
        if(has(label)) {set<T>(label,get<T>(label)+by);}
        else {add<T>(label,by);}
        return get<T>(label);
    }

    void debugData() {
        notes.debugMap();
    }
    
    /// @brief Scans through based on provided list, returns all missing labels
    list<std::string> validate(list<std::string> toCheck)
    {
        list<std::string> toReturn;
        for(auto s : toCheck) if(!has(s)) toReturn << s;
        return toReturn;
    }
};

inline std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Could not open file: " + filename);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
  }
  
  inline void writeFile(const std::string& filename, const std::string& contents) {
    std::ofstream file(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file) throw std::runtime_error("Could not open file for writing: " + filename);
  
    file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!file) throw std::runtime_error("Failed while writing file: " + filename);
  }
  
  inline void editTextFile(  const std::string& filename, const std::function<void(std::string&)>& editor) {
    std::string text = readFile(filename);
    editor(text);
    writeFile(filename, text);
  }

inline std::ifstream openReadStream(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if(!in) throw std::runtime_error("Can't read from file: " + path);
    return std::move(in);
}

inline std::ofstream openWriteStream(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if(!out) throw std::runtime_error("Can't write to file: " + path);
    return std::move(out);
}


  template<typename T>
  inline void write_raw(std::ostream& out, const T& val) {
      out.write(reinterpret_cast<const char*>(&val), sizeof(T));
  }

  inline void write_string(std::ostream& out, const std::string& s) {
      uint32_t len = s.length();
      write_raw(out, len);
      out.write(s.data(), len);
  }

  template<typename T>
  inline T read_raw(std::istream& in) {
      T val;
      in.read(reinterpret_cast<char*>(&val), sizeof(T));
      return val;
  }

  inline std::string read_string(std::istream& in) {
      uint32_t len = read_raw<uint32_t>(in);
      std::string s(len, '\0');
      in.read(s.data(), len);
      return s;
  }

  std::vector<uint8_t> readFileBytes(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if(!file) { print(red("readFileBytes: failed to open "+path)); return {}; }
    size_t size = file.tellg();
    file.seekg(0);
    std::vector<uint8_t> bytes(size);
    file.read((char*)bytes.data(), size);
    return bytes;
}

void writeFileBytes(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream file(path, std::ios::binary);
    if(!file) { print(red("writeFileBytes: failed to open "+path)); return; }
    file.write((const char*)bytes.data(), bytes.size());
}

void writeHex(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream file(path);
    for(size_t i = 0; i < bytes.size(); i++) {
        if(i % 16 == 0) file << "\n" << std::hex << std::setw(8) << std::setfill('0') << i << ":  ";
        file << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i] << " ";
        if(i % 16 == 15) {
            file << " | ";
            for(size_t j = i-15; j <= i; j++)
                file << (char)(std::isprint(bytes[j]) ? bytes[j] : '.');
        }
    }
    file << "\n";
}

#ifdef _WIN32

#else
    #include <dirent.h>
#endif

inline list<std::string> listFilesInDirectory(const std::string& path) {
    list<std::string> files;
    #ifdef _WIN32
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA((path+"\\*").c_str(), &fd);
        if(h == INVALID_HANDLE_VALUE) { print(red("listFilesInDirectory: failed to open "+path)); return files; }
        do {
            if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                files << fd.cFileName;
        } while(FindNextFileA(h, &fd));
        FindClose(h);
    #else
        DIR* dir = opendir(path.c_str());
        if(!dir) { print(red("listFilesInDirectory: failed to open "+path)); return files; }
        struct dirent* entry;
        while((entry = readdir(dir)) != nullptr) {
            if(entry->d_type == DT_REG && entry->d_name[0] != '.')
                files << entry->d_name;
        }
        closedir(dir);
    #endif
    return files;
}




class Type;

class Object : virtual public q_object {    
    public:
        uint32_t sidx = 0;
        std::atomic<bool> recycled{false};
        Type* type_ = nullptr;

        Object() {

        }
        virtual ~Object() {}

        Object(Object&& other) noexcept 
        : q_object(std::move(other)) {}

        Object& operator=(Object&& other) noexcept {
            if (this != &other) {
                q_object::operator=(std::move(other));
            }
            return *this;
        }
    };




inline std::string add_commas(int num) {
    std::string str = std::to_string(num);
    int insert_position = str.length() - 3;
    
    while(insert_position > 0) {
        str.insert(insert_position, ",");
        insert_position -= 3;
    }
    
    return str;
  }
  
  inline void indent_multiline(std::string& str, const std::string& pad) {
    size_t pos = 0;
    while((pos = str.find('\n', pos)) != std::string::npos) {
        str.replace(pos, 1, "\n" + pad);
        pos += pad.length() + 1;
    }
  }

    inline void strip_whitespace(std::string& s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](char c) {
            return c == ' ' || c == '\n' || c == '\r' || c == '\t';
        }), s.end());
    }
  
  inline std::string wrap_str(const std::string& s,const std::string& c) {
    return c+s+c;
  }
  
  inline std::string trim_str(const std::string& s,const char c) {
    if (s.size() >= 2 && s.front() == c && s.back() == c)
        return s.substr(1, s.size() - 2);
    return s; 
  }


    std::string pad_str(const std::string& s, uint32_t width) {
        std::string to_return = s;
        while(width>to_return.length()) to_return+=" ";
        return to_return;
    }

    std::string center_pad(const std::string& s, uint32_t width) {
        if(s.length() >= width) return s;
        uint32_t total_pad = width - s.length();
        uint32_t left_pad = total_pad / 2;
        uint32_t right_pad = total_pad - left_pad;
        return std::string(left_pad, ' ') + s + std::string(right_pad, ' ');
    }
    std::string center_pad_known(const std::string& s, uint32_t s_visible_len, uint32_t width) {
        if(s_visible_len >= width) return s;
        uint32_t total_pad = width - s_visible_len;
        uint32_t left_pad = total_pad / 2;
        uint32_t right_pad = total_pad - left_pad;
        return std::string(left_pad, ' ') + s + std::string(right_pad, ' ');
    }

    uint32_t digit_count(uint32_t n) {
        if(n == 0) return 1;
        uint32_t digits = 0;
        while(n > 0) { n /= 10; digits++; }
        return digits;
    }

    bool is_str_num(const std::string& tocheck) {for(auto c : tocheck) {if(!std::isdigit(c)) return false;} return true;}

    std::string escape_string(const std::string& content, bool compact_spaces = true) {
        std::string escaped;
        int space_count = 0;
        for(char c : content) {
            if(compact_spaces) {
                if(c == ' ') {
                    if(space_count==1) {escaped.pop_back(); escaped += "..."; space_count++; continue;}
                    else if(space_count>1) {continue;}
                    else {space_count++;}
                } else {
                    space_count = 0;
                }
            }
            switch(c) {
                case '\n': escaped += "\\n"; break;
                case '\t': escaped += "\\t"; break;
                case '\r': escaped += "\\r"; break;
                //case '"':  escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                default:   escaped += c; break;
            }
        }
        return escaped;
    }

    std::string strip_ansi(const std::string& s) {
        std::string out;
        bool in_escape = false;
        for(int i = 0; i < s.length(); i++) {
            if(s[i] == '\033') { in_escape = true; continue; }
            if(in_escape) {
                if(s[i] == 'm') in_escape = false;
                continue;
            }
            out += s[i];
        }
        return out;
    }
    
    std::string html_escape_string(const std::string& content) {
        std::string escaped;
        int space_count = 0;
        for(char c : content) {
            switch(c) {
                case '<': escaped += "&lt;"; break;
                case '>': escaped += "&gt;"; break;
                case '&': escaped += "&amp;"; break;
                case '\'': escaped += "&apos;"; break;
                case '"': escaped+="&quot;"; break;
                default:   escaped += c; break;
            }
        }
        return escaped;
    }
  
namespace sgen {
    struct namebase {
        namebase() {}
        explicit namebase(const list<list<std::string>>& _opts) : opts(_opts) {}
        explicit namebase(const std::string& seed) {
            list<std::string> lines = split_str(seed,',');
            for(const auto& l : lines) {
                opts << split_str(l,'|');
            }
        }
        list<list<std::string>> opts;
    };

    const namebase STANDARD("Ja|Be|Ma|Cer|Le,ck|de|ly|th|ch|un|el");
    const namebase TRUE_RANDOM("a|A|b|B|c|C|d|D|e|E|f|F|g|G|h|H|i|I|j|J|k|K|l|L|m|M|n|N|o|O|p|P|q|Q|r|R|s|S|t|T|u|U|v|V|w|W|x|X|y|Y|z|Z|0|1|2|3|4|5|6|7|8|9|_|+|-|*|/|=|<|>|!|&|^|.|,|:|;|(|)|[|]|{|}|\"|#|@|~|`|\\");
    const namebase RANDOM(
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z,"
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z,"
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z,"
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z");
    const namebase AVAL_WEST_TAMOR_FIRST(
        "Bu|Ahm|He|Ol|Mo|In|Bir|Ba|Tu," 
        "|||||||||||||||||||ha|ck|a|ch," 
        "el|ba|ak|ael|he|med");

    const namebase AVAL_CENTRAL_FIRST_MALE(
        "Al|Ed|Da|Ro|Wil|Tho|Hen|Mar|Reg|Cla|Luc|Aug,"  
        "||||||||||||an|ar|er|or|ald|ric|vid|lan|den|bert|tor|mon,"
        "us|d|n|rt|mer|son|ard|ton|las|ius|mond|iel");

    const namebase AVAL_WESTERN_FIRST_MALE(
        "Jo|Al|Con|Se|Sok|Va|Wel|Eg," 
        "|||||||||rgo|ra|ell|ber,"
        "der|us|ard|rk|on|th|n|l|vid");

    const namebase AVAL_CENTRAL_FIRST_FEMALE(
        "My|Al|Se|Ma|Eg|Cha|Sha|Tha," 
        "|||||ri|ex|il,"
        "|||na|der|ra|us|da|na|et");
    const namebase AVAL_CENTRAL_LAST(
        "Copper|Silver|Iron|Wood|High|Low|Swift|Old|New|Red|White|Black|Green|Blue|Yellow," 
        "paw|tail|fang|talon|wing|feather|river|hill|heart|claw|hall");

    inline std::string randsgen(const namebase& g) {
        std::string result;
        for(const auto& s : g.opts) 
            result.append(s.rand());
        return result;
    }
    
    inline std::string randsgen(const std::string& line) {
        list<std::string> lines = split_str(line,',');
        std::string result;
        for(const auto& l : lines) {
            list<std::string> sub = split_str(l,'|');
            std::string app = sub.rand();
            result.append(app);
        }
        return result;
    }
}

inline std::string rands() {
    return sgen::randsgen(sgen::TRUE_RANDOM);
}


namespace Log {

// Provides the time it takes for a function to run, not avereged over iterations
inline double time_function(int ITERATIONS,std::function<void(int)> process) {
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0;i<ITERATIONS;i++) {
        process(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    return (double)time.count();
}

struct Comparison_ {
    Comparison_() {}
    Comparison_(int a, int b, int c, int d) : a_table(a), a_row(b), b_table(c), b_row(d) {}
    int a_table, a_row;
    int b_table, b_row;
};


inline void run_rig(list<list<std::function<void(int)>>> f_table,list<list<std::string>> s_table,list<Comparison_> comps,bool warm_up,int PROCESS_ITERATIONS,int C_ITS) {
    list<list<double>> t_table;

    for(int c=0;c<f_table.length();c++) {
        t_table.push(list<double>{});
        for(int r=0;r<f_table[c].length();r++) {
            t_table[c].push(0.0);
        }
    }

    for(int m = 0;m<(warm_up?2:1);m++) {
        int C_ITERATIONS = m==0?warm_up?1:C_ITS:C_ITS;

        for(int c=0;c<t_table.length();c++) {
            for(int r=0;r<t_table[c].length();r++) {
                t_table[c][r]=0.0;
            }
        }

        for(int i = 0;i<C_ITERATIONS;i++)
        {
            for(int c=0;c<f_table.length();c++) {
                for(int r=0;r<f_table[c].length();r++) {
                    // if(r==0) print("Running: ",s_table[c][r]);
                    double time = time_function(PROCESS_ITERATIONS,f_table[c][r]);
                    t_table[c][r]+=time;
                }
            }
        }
        if(warm_up) {
        print("-------------------------");
        print(m==0 ? "      ==COLD==" : "       ==WARM==");
        }
        print("-------------------------");
        for(int c=0;c<t_table.length();c++) {
            for(int r=0;r<t_table[c].length();r++) {
                t_table[c][r]/=C_ITERATIONS;
                print(s_table[c][r],": ",t_table[c][r]," ns (",t_table[c][r] / PROCESS_ITERATIONS," ns per operation)");
            }
            print("-------------------------");
        }
        for(auto v : comps) {
            double factor = t_table[v.a_table][v.a_row]/t_table[v.b_table][v.b_row];
            std::string sfs;
            double tolerance = 5.0;
            if (std::abs(factor - 1.0) < tolerance/100.0) {
                sfs = "around the same as ";
            } else if (factor > 1.0) {
                double percentage = (factor - 1.0) * 100.0;
                sfs = std::to_string(percentage) + "% slower than ";
            } else {
                double percentage = (1.0/factor - 1.0) * 100.0;
                sfs = std::to_string(percentage) + "% faster than ";
            }
            print("Factor [",s_table[v.a_table][v.a_row],"/",s_table[v.b_table][v.b_row],
            "]: ",factor," (",s_table[v.a_table][v.a_row]," is ",sfs,s_table[v.b_table][v.b_row],")");
        }
        print("-------------------------");

    }
}

// A helper for using the benchmarking tools to reduce boilerplate
struct rig {
private:

    list<list<std::function<void(int)>>> f_table;
    list<list<std::string>> s_table;
    list<Comparison_> comps;
    map<std::string,std::pair<int,int>> processes;
public:
    //Adds another table, tables are isolated test blocks
    void add_table() {
        f_table << list<std::function<void(int)>>{};
        s_table << list<std::string>{};
    }

    /// @brief Add a process to run
    /// @param process_name Name of the process, used for lookup and display when run
    /// @param process The function to time and run, the int argument is the process iteration, assuming PROCESS_ITERATIONS is not 1
    /// @param table Default value is 0, this can be used to split processes into distinct blocks when run
    void add_process(const std::string& process_name,std::function<void(int)> process,int table = 0) {
        while(f_table.length() <= table) add_table();
        processes.put(process_name,std::make_pair(table,(int)f_table.get(table).length()));
        s_table.get(table) << process_name;
        f_table.get(table) << process;
    }

    /// @brief Add a comparison to be printed
    /// @param a Process to compare against
    /// @param b Process to compare to a
    void add_comparison(const std::string& a,const std::string& b) {
        try {
            std::pair<int,int> ap = processes.get(a);
            std::pair<int,int> bp = processes.get(b);
            comps << Comparison_(ap.first,ap.second,bp.first,bp.second);
        } catch(std::exception e) {
            if(!processes.hasKey(a)) {
                print("rig::110 Unable to add comparison to rig: ",a," was never added as a process");
            }
            if(!processes.hasKey(b)) {
                print("rig::110 Unable to add comparison to rig: ",b," was never added as a process");
            }
        }
    }

    /// @brief Run the rig and print out the results of the benchmark
    /// @param C_ITS How many iterations of the processes there should be, this contributes to the averege
    /// @param warm_up Whether or not to do a cold run to warm up the cache
    /// @param PROCESS_ITERATIONS How many times each process should run, not part of the averege
    void run(int C_ITS,bool warm_up = false,int PROCESS_ITERATIONS = 1) {
        run_rig(f_table,s_table,comps,warm_up,PROCESS_ITERATIONS,C_ITS);
    }
};


struct SeqLine : public q_object
{
    SeqLine() {};
    SeqLine(const std::string _label, bool _is_log) {
        label = _label;
        is_log = _is_log;
        Log::Line new_timer; new_timer.start();
        timer = new_timer;
    }

    Log::Line timer;
    double start_ns = 0.0;
    std::string label = "";
    g_ptr<SeqLine> parent = nullptr;
    list<g_ptr<SeqLine>> children;
    bool is_log = true;

    std::string get_indent() {
        if(!parent) return "";
        int depth = 0;
        g_ptr<SeqLine> cursor = this;
        while(cursor->parent) { depth++; cursor = cursor->parent; }
        std::string indent(depth * 3, ' ');
        return indent;
    }

    std::string to_string() {
        std::string indent = get_indent();
        std::string to_return = "";
        if(is_log) {
            to_return.append(indent+label+"\n");
        } else {
            to_return.append(indent+label+" [time: " + ftime(timer.total_time_)+"]\n");
            for(auto& child : children) {
                to_return.append(child->to_string());
            }
        }
        return to_return;
    }
};

class Span : public Object
{
public:
    Span() {line_root = make<SeqLine>("Root",false);};

    map<std::string, Log::Line> timers;
    map<std::string, int> counters;
    bool print_on_line_end = false;
    bool log_everything = false;

    void start_timer(const std::string &label) {
        if (timers.hasKey(label)) {
            Log::Line &timer = timers.get(label);
            timer.start();
        } else {
            Log::Line timer;
            timer.start();
            timers.put(label, timer);
        }
    }

    double end_timer(const std::string &label) {
        if (timers.hasKey(label)) {
            Log::Line &timer = timers.get(label);
            return timer.end();
        }
        return 0.0;
    }
    double get_time(const std::string &label) {
        if (timers.hasKey(label)) {return timers.get(label).total_time_;}
        else {return -1.0;}
    }
    inline std::string timer_string(const std::string &label) {return label + ": " + ftime(get_time(label));}
    void print_timers() {for (auto label : timers.keySet()) {print(timer_string(label));}}

    inline void increment(const std::string &label, int by = 1) {counters.getOrPut(label, 0) += by;}
    inline int get_count(const std::string &label) {return counters.getOrDefault(label, 0);}
    void print_counters() {for (auto label : counters.keySet())  { print(label, ": ", get_count(label)); }}


    g_ptr<SeqLine> line_root = nullptr;
    g_ptr<SeqLine> on_line = nullptr;

    void restart() {
        std::function<void(g_ptr<SeqLine>)> break_cycles = [&](g_ptr<SeqLine> node) {
            for(auto& child : node->children) break_cycles(child);
            node->parent = nullptr;
        };
        break_cycles(line_root);
        line_root = make<SeqLine>("Root",false);
        on_line = nullptr;
        line_root->timer.start();
    }
    void restart_root_time() {line_root->timer.start();}
    double get_root_time() {return line_root->timer.time_ns();}
    double end_root_time() {return line_root->timer.end();}

    g_ptr<SeqLine> get_last_line() {
        if(on_line) return on_line;
        else return line_root;
    }

    void add_line(const std::string& label) {
        g_ptr<SeqLine> parent = get_last_line();
        parent->children << make<SeqLine>(label,false);
        SeqLine* child = parent->children.last().getPtr();
        child->parent = parent;
        child->start_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            child->timer.start_ - line_root->timer.start_
        ).count();
        on_line = parent->children.last();

        if(log_everything)
            print(label);
    }

    double end_line() 
    {
        double time = 0.0;
        if(!on_line) return time;
        time = on_line->timer.end();
        if(print_on_line_end)
            std::cout << on_line->to_string() << std::flush;
        if(on_line->parent) {
            on_line = on_line->parent;
        }
        return time;
    }

    template<typename... Args>
    void log(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        if(log_everything)
            std::cout << oss.str() << std::endl;
        std::string indent = get_last_line()->get_indent();
        indent += "  > "; //Extra space to distinquish from header
        std::string msg = indent+oss.str();
        indent_multiline(msg,indent);
        g_ptr<SeqLine> new_log = make<SeqLine>(msg,true);
        get_last_line()->children << new_log;
    }

    void print_all() {
        line_root->timer.end();
        print(line_root->to_string());
    }

    void newline(const std::string& label) {
        add_line(label);
    }

    double endline() {
        return end_line();
    }

    std::string print_as_flamechart() {
        if(line_root->children.empty()) return "";
        line_root->timer = line_root->children[0]->timer;
        double root_duration = line_root->timer.total_time_;
        double W = 2400.0, row_h = 24.0;
        
        int max_depth = 0;
        std::function<void(SeqLine*, int)> get_depth = [&](SeqLine* n, int d) {
            max_depth = std::max(max_depth, d);
            for(auto& c : n->children) if(!c->is_log) get_depth(c.getPtr(), d+1);
        };
        get_depth(line_root.getPtr(), 0);
        
        double total_h = (max_depth + 1) * row_h;
        
        std::string svg = "<svg xmlns='http://www.w3.org/2000/svg' width='" + std::to_string((int)W) + 
            "' height='" + std::to_string((int)total_h) + 
            "' viewBox='0 0 " + std::to_string((int)W) + " " + 
            std::to_string((int)total_h) + "'>\n";
        
        std::function<void(SeqLine*, int)> render = [&](SeqLine* n, int depth) {
            double x = (n->start_ns / root_duration) * W;
            double w = std::max((n->timer.total_time_ / root_duration) * W, 1.0);
            double y = total_h - (depth + 1) * row_h; // flip: depth 0 at bottom
            int hue = (int)(x * 0.3) % 360;
            std::string label = html_escape_string(strip_ansi(n->label));
            svg += "<rect x='" + std::to_string(x) + "' y='" + std::to_string(y) +
                   "' width='" + std::to_string(w) + "' height='" + std::to_string(row_h-1) +
                   "' fill='hsl(" + std::to_string(hue) + ",60%,65%)' stroke='white' stroke-width='0.5'>" +
                   "<title>" + label + " [" + html_escape_string(strip_ansi(ftime(n->timer.total_time_))) + "]</title></rect>\n";
            if(w > 15) {
                svg += "<text x='" + std::to_string(x+2) + "' y='" + std::to_string(y + row_h*0.65) +
                       "' font-size='10' fill='black'>" + label.substr(0, (int)(w/5.5)) + "</text>\n";
            }
            for(auto& c : n->children) if(!c->is_log) render(c.getPtr(), depth+1);
        };
        render(line_root.getPtr(), 0);
        svg += "</svg>";
        return svg;
    }
};

}



    

//Controls for the compiler printing, for debugging
#define PRINT_ALL 0

g_ptr<Log::Span> span = nullptr;
static inline void newline(const std::string& label) {
    #if PRINT_ALL
    if(!span) span = make<Log::Span>();
    span->add_line(label);
    #endif
}
static inline double endline() {
    #if PRINT_ALL
    return span->end_line();
    #else
    return 0;
    #endif
}

template<typename... Args>
static inline void log(Args&&... args) {
    #if PRINT_ALL
    if(!span) span = make<Log::Span>();
    span->log(std::forward<Args>(args)...);
    #endif
}

std::string ptr_to_string(uint64_t addr) {
    uint64_t varied = addr >> 4;
    
    const char* profiles[] = {"CGOQD", "IHLTFE", "AVWXZK", "BPRSM"};
    int prof = varied & 0x3;
    
    const char* group = profiles[prof];
    int letter_idx = (varied >> 2) % strlen(group);
    char letter = group[letter_idx];
    
    uint64_t tiebreaker = (varied >> 6) & 0xFFF;
    std::string tbstr = std::to_string(tiebreaker);
    std::reverse(tbstr.begin(), tbstr.end());
    
    return std::string({letter}) + "-" + tbstr + "-" + letter;
}

std::string ptr_to_string(void* ptr) {
    return ptr_to_string((uint64_t)ptr);
}

struct IdPool {
    list<int> ids;
    int top = 0;
    
    void init(list<int> available) {
        ids = available;
        top = available.length();
    }
    
    int alloc() {
        if(top == 0) return -1;
        return ids[--top];
    }
    
    void free(int id) {
        if(id != -1) 
            ids[top++] = id;
    }
};








#define ACORN_DEBUG 1

namespace Acorn {

    bool ERROR_FLAG = false;
    std::string ERROR_MSG = "";

    template<typename... Args>
    void throw_error(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        ERROR_MSG = oss.str();
        ERROR_FLAG = true;
        print(red("ERROR: "),ERROR_MSG);
    }

    #if ACORN_DEBUG
        #define DEBUG_ONLY(x) x
    #else
        #define DEBUG_ONLY(x)
    #endif

    uint32_t hashBytes(const void* data, uint32_t size) {
        uint32_t hash = 5381;
        const uint8_t* bytes = (const uint8_t*)data;
        for(uint32_t i = 0; i < size; i++) {
            hash = ((hash << 5) + hash) + bytes[i];
        }
        return hash;
    }

    // struct Ptr {
    //     Ptr() {}
    //     Ptr(uint32_t _pool, uint32_t _idx, uint32_t _sidx, uint16_t _unit) : pool(_pool), idx(_idx), sidx(_sidx), unit(_unit) {}
    //     Ptr(uint32_t _pool, uint32_t _idx, uint32_t _sidx) : pool(_pool), idx(_idx), sidx(_sidx) {}
    //     uint32_t pool = 0; //Pool it's at
    //     uint32_t idx = 0; //Column
    //     uint32_t sidx = 0; //Row
        
    //     uint16_t unit = 0;

    //     inline bool operator==(const Ptr& other) const {return pool == other.pool && idx == other.idx && sidx == other.sidx;}
    //     inline bool operator!=(const Ptr& other) const {return !(*this == other);}
    // };


    // struct Ptr {
    //     Ptr(uint32_t _unit, uint32_t _pool, uint32_t _idx, uint32_t _sidx) {
    //         memset(this, 0, sizeof(Ptr));
    //         unit = _unit; pool = _pool; idx = _idx; sidx = _sidx;
    //     }
    //     Ptr(uint32_t _pool, uint32_t _idx, uint32_t _sidx) {
    //         memset(this, 0, sizeof(Ptr));
    //         pool = _pool; idx = _idx; sidx = _sidx;
    //     }
    //     Ptr(void* _cache, uint32_t _pool, uint32_t _idx, uint32_t _sidx) {
    //         memset(this, 0, sizeof(Ptr));
    //         pool = _pool; idx = _idx; sidx = _sidx;
    //         cache = _cache; cachelevel = 3;
    //     }
    //     Ptr(void* _cache, uint32_t _idx, uint32_t _sidx) {
    //         memset(this, 0, sizeof(Ptr));
    //         idx = _idx; sidx = _sidx;
    //         cache = _cache; cachelevel = 2;
    //     }
    //     Ptr(void* _cache, uint32_t _sidx) {
    //         memset(this, 0, sizeof(Ptr));
    //         sidx = _sidx;
    //         cache = _cache; cachelevel = 1;
    //     }
    //     Ptr() { memset(this, 0, sizeof(Ptr)); }

    //     uint8_t region;
    //     uint16_t zone;

    //     uint8_t cachelevel;
    //     union {
    //         struct { 
    //             uint32_t unit; 
    //             uint32_t device; 
    //         };
    //         void* cache;
    //     };

    //     uint32_t pool;
    //     uint32_t idx;
    //     uint32_t sidx;

    //     inline bool operator==(const Ptr& other) const {
    //         return pool == other.pool && idx == other.idx && sidx == other.sidx && 
    //             zone == other.zone && region == other.region &&
    //             cachelevel == other.cachelevel && 
    //             (cachelevel == 0 ? (unit == other.unit && device == other.device) : (cache == other.cache));
    //     }
    //     inline bool operator!=(const Ptr& other) const {return !(*this == other);}
    // };



    //Cachelevels
    //0=no cache (full resolution)
    //1=sidx valid, cache is a col
    //2=idx valid, cache is a ColCol
    //3=pool valid, cache is a ColColCol

    struct Ptr {
        Ptr(uint32_t _device, uint32_t _unit, uint32_t _pool, uint32_t _idx, uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            device = _device; unit = _unit; pool = _pool; idx = _idx; sidx = _sidx;
        }
        Ptr(uint32_t _unit, uint32_t _pool, uint32_t _idx, uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            unit = _unit; pool = _pool; idx = _idx; sidx = _sidx;
        }
        Ptr(uint32_t _pool, uint32_t _idx, uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            pool = _pool; idx = _idx; sidx = _sidx;
        }
        Ptr(uint32_t _idx, uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            idx = _idx; sidx = _sidx;
        }
        Ptr(uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            sidx = _sidx;
        }
        Ptr(void* _cache, uint32_t _pool, uint32_t _idx, uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            pool = _pool; idx = _idx; sidx = _sidx;
            cache = _cache; cachelevel = 3;
        }
        Ptr(void* _cache, uint32_t _idx, uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            idx = _idx; sidx = _sidx;
            cache = _cache; cachelevel = 2;
        }
        Ptr(void* _cache, uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            sidx = _sidx;
            cache = _cache; cachelevel = 1;
        }
        Ptr() { memset(this, 0, sizeof(Ptr)); }
        
        union {
            struct { 
                uint32_t region;
                uint32_t zone;
            };
            void* cache;
        };
        uint16_t unknown16;
        uint8_t specialization; 
        uint8_t cachelevel;
        uint32_t device; 
        uint32_t unit; 
        uint32_t pool;
        uint32_t idx;
        uint32_t sidx;

        inline bool operator==(const Ptr& other) const {
            return pool == other.pool && idx == other.idx && sidx == other.sidx && 
                unit == other.unit && device == other.device &&
                cachelevel == other.cachelevel && 
                (cachelevel == 0 ? (zone == other.zone && region == other.region) : (cache == other.cache));
        }
        inline bool operator!=(const Ptr& other) const {return !(*this == other);}
    };
    static_assert(sizeof(Ptr)==32," Size of Ptr must be 32 for cross platform");

    struct Ptr4 {
        Ptr4() {}
        Ptr4(uint32_t _midx, Ptr p) : midx(_midx), ptr(p) {}
        uint32_t midx = 0;
        Ptr ptr;
    };

    static const Ptr deadptr;
    static Ptr dead_ref;

    struct QCol {
        QCol() {}
        QCol(const QCol& o) {
            storage = nullptr;
            size = 0;
            capacity = 0;
            if(o.storage && o.size > 0) {
                resize(o.size);
                memcpy(storage, o.storage, o.size);
            }
        }
        QCol(QCol&& o) {
            storage = o.storage;
            size = o.size;
            capacity = o.capacity;
            o.storage = nullptr;
            o.size = 0;
            o.capacity = 0;
        }
        QCol& operator=(const QCol& o) {
            if(this == &o) return *this;
            if(storage) delete[] storage;
            size = o.size;
            capacity = o.capacity;
            if(o.storage && o.capacity > 0) {
                storage = new uint8_t[o.capacity];
                memcpy(storage, o.storage, o.size);
            } else {
                storage = nullptr;
            }
            return *this;
        }
        QCol& operator=(QCol&& o) {
            if(this == &o) return *this;
            if(storage) delete[] storage;
            storage = o.storage;
            size = o.size;
            capacity = o.capacity;
            o.storage = nullptr;
            o.size = 0;
            o.capacity = 0;
            return *this;
        }
        ~QCol() {
            if(storage) {
                delete[] storage;
                storage = nullptr;
            }
        }

        uint8_t* storage = nullptr;
        uint32_t size = 0;
        uint32_t capacity = 0;
        
        inline bool empty() {return size==0;}
        void reserve(uint32_t new_capacity) {
            if(new_capacity <= capacity) return;
            
            uint8_t* newPtr = new uint8_t[new_capacity];
            if(storage) memcpy(newPtr, storage, size);
            delete[] storage;
            storage = newPtr;
            capacity = new_capacity;
        }
        void resize(uint32_t new_size) {
            if (new_size > capacity) {
                reserve(new_size);
            }
            size = new_size;
        }
        void push(const void* element, uint32_t width) {
            uint32_t old_size = size;
            uint32_t new_size = size+width;
            if(new_size>=capacity) {
                reserve(new_size*2);
            }
            memcpy(&storage[old_size], element, width);
            size = new_size;
        }
        void push_default(uint32_t width) {
            size_t old_size = size;
            resize(old_size + width);
            memset(&storage[old_size], 0, width);
        }
        void insert(uint32_t index, const void* element, uint32_t width) {
            uint32_t byte_pos = index * width;
            uint32_t new_size = size + width;
            if(new_size >= capacity) reserve(new_size * 2);
            memmove(&storage[byte_pos + width], &storage[byte_pos], size - byte_pos);
            memcpy(&storage[byte_pos], element, width);
            size = new_size;
        }
        inline void* qget(uint32_t offset) const {
            DEBUG_ONLY(if(offset>=size) {throw_error(red("col:qget "),"offset ",offset," out of bounds for size ",size);return nullptr;})
            return &storage[offset];
        }
        inline void* operator[](uint32_t index) {return qget(index);}
        inline void qset(uint32_t offset, const void* element, uint32_t width) {memcpy(&storage[offset], element, width);}
        void removeAt(uint32_t index, uint32_t width) {
            size_t byte_start = index * width;
            for(size_t i = byte_start; i < size - width; i++) {
                storage[i] = storage[i + width];
            }
            resize(size - width);
        }
        void clear() {size = 0;}
        void pop(void* out, uint32_t width) {
            memcpy(out, qget((size/width - 1) * width), width);
            resize(size - width);
        }
    };

    struct QString : QCol {
        QString() {}
        QString(QCol q) : QCol(q) {}
        QString(const QString& o) : QCol(o) {}
        QString(QString&& o) : QCol(std::move(o)) {}
        QString& operator=(const QString& o) {
            if(this == &o) return *this;
            QCol::operator=(o);
            return *this;
        }
        QString& operator=(QString&& o) {
            if(this == &o) return *this;
            QCol::operator=(std::move(o));
            return *this;
        }
        ~QString() {}
        
        char& at(uint32_t idx) {return *(char*)qget(idx);}
        char& operator[](uint32_t idx) {return *(char*)qget(idx);}
        void push(char c) {QCol::push((void*)&c,1);}
        uint32_t length() {return size;}

        void operator=(const std::string& s) {clear(); for(char c : s) push(c);}
        void operator=(const char* s) {clear(); while(*s) push(*s++);}
        bool operator==(const std::string& s) {
            if(size != s.length()) return false;
            return memcmp(storage, s.data(), size) == 0;
        }
        bool operator==(const char* s) {
            uint32_t len = strlen(s);
            if(size != len) return false;
            return memcmp(storage, s, size) == 0;
        }
        std::string to_std() {
            if(!storage) return "";
            return std::string((char*)storage, size);
        }
    };
    std::ostream& operator<<(std::ostream& os, QString& s) {
        if(s.storage) os.write((const char*)s.storage, s.size);
        return os;
    }


    struct CCol : QCol {
        CCol() {}
        CCol(uint32_t _size) : element_size(_size) {}
        CCol(QCol q) : QCol(q) {}
        CCol(const CCol& o) : QCol(o) {
            element_size = o.element_size;
            tag = o.tag;
            hash = o.hash;
            index = o.index;
            cachelevel = o.cachelevel;
            live = o.live;
        }
        CCol(CCol&& o) : QCol(std::move(o)) {
            element_size = o.element_size;
            tag = o.tag;
            hash = o.hash;
            index = o.index;
            cachelevel = o.cachelevel;
            live = o.live;
        }
        CCol& operator=(CCol&& o) {
            if(this == &o) return *this;
            QCol::operator=(std::move(o));
            element_size = o.element_size;
            tag = o.tag;
            hash = o.hash;
            index = o.index;
            cachelevel = o.cachelevel;
            live = o.live;
            return *this;
        }
        uint32_t element_size = 1;
        uint32_t tag = 0;
        uint32_t hash = 0;
        uint32_t index = 0;
        uint8_t cachelevel = 0;
        bool live = true;

        inline uint32_t length() const {if(element_size==0||size==0) {return 0;} else {return size / element_size;}}
        void push(const void* element) {
            QCol::push(element,element_size);
        }
        void operator<<(const void* element) {push(element);}
        void push_default() {QCol::push_default(element_size);}
        void insert(uint32_t index, const void* element) {
            QCol::insert(index, element, element_size);
        }
        
        inline void* sget(uint32_t index) const {
            DEBUG_ONLY(if(index*element_size>=size) {throw_error(red("col:sget "),"index ",index," out of bounds for size ",size,", elment size is ",element_size," tag is ",tag);return nullptr;})
            return &storage[index * element_size];
        }
        inline void* iget(uint32_t index, uint32_t offset) {
            DEBUG_ONLY(if(index*element_size+offset>=size) {throw_error(red("col:iget "),"index ",index," plus offset ",offset," out of bounds for size ",size);return nullptr;})
            return &storage[index * element_size + offset];
        }
        inline void set(uint32_t index, const void* element) {memcpy(&storage[index * element_size], element, element_size);}
        inline void iset(uint32_t index, uint32_t offset, const void* element, uint32_t width) {memcpy(&storage[index * element_size + offset], element, element_size);}
        void removeAt(uint32_t index) {QCol::removeAt(index,element_size);}
        void pop(void* out) {QCol::pop(out,element_size);}
    };

    struct QCellCol : QCol {
        QCellCol() {}
        QCellCol(QCol q) : QCol(q) {}
        QCellCol(const QCellCol& o) : QCol() {
            for(uint32_t i = 0; i < o.length(); i++) {
                CCol copy(o.get(i)); 
                push(copy);
                copy.storage = nullptr;
            }
        }
        QCellCol& operator=(QCellCol&& o) {
            if(this == &o) return *this;
            // destruct existing embedded CCols
            for(uint32_t i = 0; i < length(); i++) get(i).~CCol();
            if(storage) delete[] storage;
            storage = o.storage;
            size = o.size;
            capacity = o.capacity;
            o.storage = nullptr;
            o.size = 0;
            o.capacity = 0;
            return *this;
        }
        QCellCol& operator=(const QCellCol& o) {
            if(this == &o) return *this;
            for(uint32_t i = 0; i < length(); i++) get(i).~CCol();
            if(storage) delete[] storage;
            storage = nullptr; size = 0; capacity = 0;
            for(uint32_t i = 0; i < o.length(); i++) {
                CCol copy(o.get(i));
                push(copy);
                copy.storage = nullptr;
            }
            return *this;
        }
        ~QCellCol() {
            if(!storage) return;
            for(uint32_t i = 0; i < length(); i++) {
                get(i).~CCol();
            }
        }
        CCol& get(uint32_t idx) const {return *(CCol*)qget(idx*sizeof(CCol));}
        CCol& operator[](uint32_t idx) {return *(CCol*)qget(idx*sizeof(CCol));}
        void push(CCol c) {QCol::push((void*)&c,sizeof(CCol)); c.storage = nullptr;}
        uint32_t length() const {return size/sizeof(CCol);}
    };

    struct Col : CCol {
        Col() {}
        Col(uint32_t _size) :  CCol(_size) {}
        Col(const Col& o) : CCol(o), heterogenous(o.heterogenous), label(o.label), cells(o.cells) {}
        Col(CCol q) : CCol(q) {}
        Col& operator=(Col&& o) {
            if(this == &o) return *this;
            CCol::operator=(std::move(o)); 
            heterogenous = o.heterogenous;
            label = std::move(o.label);
            cells = std::move(o.cells);
            free = std::move(o.free);
            return *this;
        }
        bool heterogenous = false;
        QString label;
        QCellCol cells;
        list<uint32_t> free;
        
        inline void* get(uint32_t index) {
            if(heterogenous) {
                return qget(index);
            } else {
                return sget(index);
            }
        }
        inline void* operator[](uint32_t index) {return get(index);}
        inline void* last() {return qget(size-(element_size));}

        void qput(const void* element, const void* key, uint32_t key_size, uint32_t key_tag) {
            CCol c;
            c.element_size = key_size; 
            c.tag = key_tag;
            c.hash = hashBytes(key, key_size);
            c.index = length();
            c.push(key);
            push(element);
            cells.push(c);
        }
        void* get(const void* key, uint32_t size) {
            uint32_t h = hashBytes(key, size);
            for(int i = 0; i < cells.length(); i++) {
                CCol& c = cells[i];
                if(c.hash == h) {
                    if(memcmp(c.storage, key, size) == 0) { //Collison check against the stored key
                        return sget(c.index);
                    }
                }
            }
            return nullptr;
        }
        uint32_t getidx(const void* key, uint32_t size) {
            uint32_t h = hashBytes(key, size);
            for(int i = 0; i < cells.length(); i++) {
                CCol& c = cells[i];
                if(c.hash == h) {
                    if(memcmp(c.storage, key, size) == 0) { //Collison check against the stored key
                        return c.index;
                    }
                }
            }
            return 0;
        }
        bool hasKey(const void* key, uint32_t size) {
            uint32_t h = hashBytes(key, size);
            for(int i = 0; i < cells.length(); i++) {
                CCol& c = cells[i];
                if(c.hash == h && memcmp(c.storage, key, size) == 0) return true;
            }
            return false;
        }

        void put(const std::string& str, const void* element, uint32_t tag = 0) {qput(element,str.data(),str.length(),tag);}
        void* get(const std::string& str) {return get(str.data(), str.length());}
        bool hasKey(const std::string& str) {return hasKey(str.data(), str.length());}
        void put(uint64_t u64, const void* element, uint32_t tag = 0) {qput(element,(void*)&u64,8,tag);}
        void* get(uint64_t u64) {return get((void*)&u64, 8);}
        bool hasKey(uint64_t u64) {return hasKey((void*)&u64, 8);}
        void put(Ptr p, const void* element, uint32_t tag = 0) {qput(element, (void*)&p, sizeof(Ptr), tag);}
        void* get(Ptr p) {return get((void*)&p, sizeof(Ptr));}
        bool hasKey(Ptr p) {return hasKey((void*)&p, sizeof(Ptr));}
    };

    //Convience for ergonomic white/blacklist things
    struct _lookup {
        _lookup(list<std::string> init, bool _default_state) 
        : default_state(_default_state)  {
            for(auto s : init) {
                lookup[s] = !default_state;
            }
        }

        map<std::string,bool> lookup;
        bool default_state;

        bool operator[](const std::string& key) {
            return lookup.getOrDefault(key,default_state);
        }
    };

    uint32_t add_column(Col& col, size_t size = 0, uint32_t tag = 0) {
        Col ncol(size);
        ncol.tag = tag;
        col.push((void*)&ncol);
        return col.length()-1;
    }

    
    uint32_t note_value(Col& col, const std::string& key, uint32_t size, uint32_t tag) {
        uint32_t at = add_column(col, size, tag);
        (*(Col*)col.sget(at)).label = key;
        return at;
    }

   



    static void write_qcol(std::ostream& out, QCol& col) {
        write_raw<uint32_t>(out, col.size);
        out.write((const char*)col.storage, col.size);
    }

    static QCol read_qcol(std::istream& in) {
        QCol col;
        uint32_t size = read_raw<uint32_t>(in);
        col.resize(size);
        in.read((char*)col.storage, col.size);
        return col;
    }

    static void write_ccol(std::ostream& out, CCol& col) {
        //print("write_ccol size: ", col.size, " storage: ", (void*)col.storage);
        write_qcol(out,col);
        write_raw<uint32_t>(out, col.element_size);
        write_raw<uint32_t>(out, col.tag);
        write_raw<uint32_t>(out, col.hash);
        write_raw<bool>(out, col.live);
    }

    static CCol read_ccol(std::istream& in) {
        CCol col = read_qcol(in);
        col.element_size = read_raw<uint32_t>(in);
        col.tag = read_raw<uint32_t>(in);
        col.hash = read_raw<uint32_t>(in);
        col.live = read_raw<bool>(in);
        return col;
    }

    static void write_qcellcol(std::ostream& out, QCellCol& cells) {
        write_raw<uint32_t>(out, cells.length());
        //print("Writting qcellcol, count: ", cells.length(), " stream pos: ", out.tellp());
        for(uint32_t i = 0; i < cells.length(); i++) {
            write_ccol(out, cells.get(i));
        }
    }
    
    static QCellCol read_qcellcol(std::istream& in) {
        QCellCol cells;
        uint32_t count = read_raw<uint32_t>(in);
        //print("Reading qcellcol, count: ", count, " stream pos: ", in.tellg());
        for(uint32_t i = 0; i < count; i++) {
            CCol c = read_ccol(in);
            // print("Key bytes: ", c.size, " storage: ", (void*)c.storage," esize ",c.element_size," tag ",c.tag);
            // for(uint32_t b = 0; b < c.size; b++) print((char)c.storage[b]);
            cells.push(c);
            c.storage = nullptr;
        }
        return cells;
    }

    static void write_col(std::ostream& out, Col& col) {
        write_ccol(out,col);
        write_raw<bool>(out, col.heterogenous);
        write_qcellcol(out, col.cells);
        write_qcol(out,col.label);
        write_raw<uint32_t>(out,col.free.length());
        for(int i=0;i<col.free.length();i++) {
            write_raw<uint32_t>(out,col.free[i]);
        }
    }

    static Col read_col(std::istream& in) {
        Col col = read_ccol(in);
        col.heterogenous = read_raw<bool>(in);
        col.cells = read_qcellcol(in);
        col.label = read_qcol(in);
        uint32_t freelen = read_raw<uint32_t>(in);
        for(int i=0;i<freelen;i++) {
            col.free << read_raw<uint32_t>(in);
        }
        return col;
    }

    static void write_col_header(std::ostream& out, Col& col) {
        //write_raw<uint32_t>(out, col.size);
        write_raw<uint32_t>(out, col.element_size);
        write_raw<uint32_t>(out, col.tag);
        write_raw<uint32_t>(out, col.hash);
        write_raw<bool>(out, col.live);
        write_raw<bool>(out, col.heterogenous);
        write_qcellcol(out, col.cells);
        write_qcol(out,col.label);
        write_raw<uint32_t>(out,col.free.length());
        for(int i=0;i<col.free.length();i++) {
            write_raw<uint32_t>(out,col.free[i]);
        }
    }

    static Col read_col_header(std::istream& in) {
        Col col;
        // uint32_t size = read_raw<uint32_t>(in);
        // col.resize(size);
        col.element_size = read_raw<uint32_t>(in);
        col.tag = read_raw<uint32_t>(in);
        col.hash = read_raw<uint32_t>(in);
        col.live = read_raw<bool>(in);
        col.heterogenous = read_raw<bool>(in);
        col.cells = read_qcellcol(in);
        col.label = read_qcol(in);
        uint32_t freelen = read_raw<uint32_t>(in);
        for(int i=0;i<freelen;i++) {
            col.free << read_raw<uint32_t>(in);
        }
        return col;
    }
}


#define NAMED_PTRS 1

namespace Acorn {   
    static int _ctx_dummy_index = 0;
    class Unit;
    struct Node;
    struct Value;

    struct ColCol : Col {
        ColCol() : Col(sizeof(Col)) {}
        ColCol(Col c) : Col(c) {}
        ColCol(const ColCol& o) : Col(sizeof(Col)) {
            element_size = o.element_size;
            tag = o.tag;
            heterogenous = o.heterogenous;
            label = QCol(o.label);
            free = o.free;
            for(uint32_t i = 0; i < o.length(); i++) {
                Col copy(*(Col*)o.sget(i));
                push(copy);
            }
        }
        ColCol& operator=(ColCol&& o) {
            if(this == &o) return *this;
            if(storage && element_size != 0) {
                for(uint32_t i = 0; i < length(); i++) get(i).~Col();
            }
            Col::operator=(std::move(o));
            return *this;
        }
        ~ColCol() {
            if(!storage || element_size == 0) return;
            for(uint32_t i = 0; i < length(); i++) {
                get(i).~Col();
            }
        }

        Col& get(uint32_t idx) {return *(Col*)Col::sget(idx);}
        void set(uint32_t idx, Col val) {
            get(idx).~Col();
            Col::set(idx,(void*)&val);
            val.storage = nullptr;
            val.label.storage = nullptr;
            // for(uint32_t i = 0; i < val.cells.length(); i++) {
            //     val.cells.get(i).storage = nullptr;
            // }
            val.cells.storage = nullptr;
        }
        Col& operator[](uint32_t idx) {return *(Col*)Col::sget(idx);}
        void push(Col t) {
            Col::push((void*)&t);
            t.storage = nullptr;
            t.label.storage = nullptr;
            t.cells.storage = nullptr;
        }
        void put(const std::string& key, Col t) {
            Col::put(key,(void*)&t); //This should probably have tag string id for display later, may require reordering how we register the ids
            t.storage = nullptr;
            t.label.storage = nullptr;
            t.cells.storage = nullptr;
        }
    };

     //Standard column create, use pooling means it will try to find a dead column first, tag sensitive means it will also ensure the column tag matches
     uint32_t create_column(ColCol& col, uint32_t size, uint32_t tag, bool use_pooling = true, bool tag_sensitive = false) {
        if(use_pooling&&!col.free.empty()) {
            //Lazy version
            uint32_t idx = col.free.pop();
            Col& ncol = col[idx];
            ncol.clear(); ncol.element_size = size; ncol.tag = tag;
            ncol.live = true;
            return idx; 
            
            // for(int i=0;i<col.free.length();i++) {
            //     Col& ncol = col[col.free[i]];
            //     if(!ncol.live&&ncol.element_size==size&&(!tag_sensitive||ncol.tag==tag)) {
            //         ncol.clear();
            //         ncol.live = true;
            //         return col.free[i];
            //     }
            // }
        }
        add_column(col,size,tag);
        return col.length()-1;
    }
    //Creates a column from pool and intilizes it's memory if empty
    uint32_t push_column(ColCol& col, uint32_t size, uint32_t tag) {
        uint32_t at = create_column(col,size,tag);
        Col& ncol = *(Col*)col.sget(at);
        if(ncol.size<size) {
            ncol.resize(size);
        }
        return at;
    }
    static void recycle_column(ColCol& col, uint32_t id) {
       Col* c = ((Col*)col.sget(id));
       if(c) {
        c->live = false;
        col.free.push(id);
       } else {
        print(red("UNABLE TO RECYLE COL AT "+std::to_string(id)));
       }
    }
    
    struct ColColCol : Col {
        ColColCol() : Col(sizeof(ColCol)) {}
        ColColCol(Col c) : Col(c) {}
        ColColCol(const ColColCol& o) : Col(sizeof(ColCol)) {
            element_size = o.element_size;
            tag = o.tag;
            heterogenous = o.heterogenous;
            label = QCol(o.label);
            for(uint32_t i = 0; i < o.length(); i++) {
                ColCol copy(*(ColCol*)o.sget(i));
                push(copy);
            }
        }
        ColColCol& operator=(ColColCol&& o) {
            if(this == &o) return *this;
            if(storage && element_size != 0) {
                for(uint32_t i = 0; i < length(); i++) get(i).~ColCol();
            }
            ColColCol::operator=(std::move(o));
            return *this;
        }
        ~ColColCol() {
            if(!storage || element_size == 0) return;
            for(uint32_t i = 0; i < length(); i++) {
                get(i).~ColCol();
            }
        }
        ColCol& get(uint32_t idx) {return *(ColCol*)Col::sget(idx);}
        void set(uint32_t idx, ColCol val) {
            get(idx).~ColCol();
            Col::set(idx,(void*)&val);
            val.storage = nullptr;
            val.label.storage = nullptr;
            // for(uint32_t i = 0; i < val.cells.length(); i++) {
            //     val.cells.get(i).storage = nullptr;
            // }
            val.cells.storage = nullptr;
        }
        ColCol& operator[](uint32_t idx) {return *(ColCol*)Col::sget(idx);}
        void push(ColCol t) {
            Col::push((void*)&t);
            t.storage = nullptr;
            t.label.storage = nullptr;
            t.cells.storage = nullptr;
        }
        void insert(uint32_t index, ColCol t) {
            CCol::insert(index,(void*)&t);
            t.storage = nullptr;
            t.label.storage = nullptr;
            t.cells.storage = nullptr;
        }
    };

    inline ColColCol& cache_as_unit(const Ptr& p) { 
        DEBUG_ONLY(if(p.cachelevel != 3) throw_error("cache_as_unit called on Ptr with cachelevel ", p.cachelevel);)
        return *(ColColCol*)p.cache; 
    }
    inline ColCol& cache_as_pool(const Ptr& p) { 
        DEBUG_ONLY(if(p.cachelevel != 2) throw_error("cache_as_pool called on Ptr with cachelevel ", p.cachelevel);)
        return *(ColCol*)p.cache; 
    }
    inline Col& cache_as_col(const Ptr& p) { 
        DEBUG_ONLY(if(p.cachelevel != 1) throw_error("cache_as_col called on Ptr with cachelevel ", p.cachelevel);)
        return *(Col*)p.cache; 
    }

    std::string Ptr_to_string(Ptr p, int print_level = 3) {
        if(p.cachelevel > 0 && print_level > p.cachelevel)  return "CANNOT PRINT LEVEL "+std::to_string(print_level)+" ON PTR WITH CACHELEVEL "+std::to_string(p.cachelevel);

        switch(print_level) {
            case 7: return std::to_string(p.region)+"|"+std::to_string(p.zone)+"|"+std::to_string(p.device)+"|"+std::to_string(p.unit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 6: return std::to_string(p.zone)+"|"+std::to_string(p.device)+"|"+std::to_string(p.unit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 5: return std::to_string(p.device)+"|"+std::to_string(p.unit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 4: return std::to_string(p.unit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 3: return std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 2: return std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 1: return std::to_string(p.sidx);
            case 0: return std::to_string(p.region)+"|"+std::to_string(p.zone)+"|"+std::to_string(p.device)+"|"+std::to_string(p.unit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            default: return "INVALID PRINT LEVEL FOR PTR_TO_STRING "+std::to_string(print_level);
        }
    }
    Ptr string_to_Ptr(const std::string& s) {
        auto l = split_str(s,'|');
        if(l.length()==2) {
            Ptr p(std::stoi(l[0]),std::stoi(l[1]));
            p.cachelevel = 2;
            return p;
        } else if(l.length()==3) {
            Ptr p(std::stoi(l[0]),std::stoi(l[1]),std::stoi(l[2]));
            p.cachelevel = 3;
            return p;
        } else if(l.length()==4) {
            Ptr p(std::stoi(l[0]),std::stoi(l[1]),std::stoi(l[2]),std::stoi(l[3]));
            p.cachelevel = 4;
            return p;
        } else if(l.length()==5) {
            Ptr p(std::stoi(l[0]),std::stoi(l[1]),std::stoi(l[2]),std::stoi(l[3]),std::stoi(l[4]));
            p.cachelevel = 5;
            return p;
        } else {
            print(red("Unable to convert "+s+" to a Ptr")); 
            return deadptr;
        }
    }
    uint8_t string_to_cachelevel(const std::string& s) {
        uint8_t to_return = 0;
        for(auto& c : s) if(c=='|') to_return++;
        if(to_return!=0) to_return+=1;
        return to_return;
    }

    list<g_ptr<Unit>> units;
    static std::mutex units_mutex;

    ColColCol& init_first_unit();

    ColColCol& global = init_first_unit();

    static ColColCol col3_ref;
    inline ColColCol& resolve_to_unit(const Ptr& ptr);
    static ColCol col2_ref;
    inline ColCol& resolve_to_pool(const Ptr& ptr) {
        switch(ptr.cachelevel) {
            case 0: case 3: {return resolve_to_unit(ptr)[ptr.pool];}
            case 2: return *(ColCol*)ptr.cache;
            default: 
                throw_error("Can not resolve Ptr ",Ptr_to_string(ptr)," to pool because it's cachelevel ",(uint32_t)ptr.cachelevel," is too low");
            return col2_ref;
        }
    }
    static Col col1_ref;
    inline Col& resolve_to_col(const Ptr& ptr) {
        switch(ptr.cachelevel) {
            case 0: case 3: case 2: {return resolve_to_pool(ptr)[ptr.idx];}
            case 1: return *(Col*)ptr.cache;
            default: 
                throw_error("Can not resolve Ptr ",Ptr_to_string(ptr)," to col because it's cachelevel ",(uint32_t)ptr.cachelevel," is too low");
            return col1_ref;
        }
    }
    inline void* resolve_ptr(const Ptr& ptr) {
        return resolve_to_col(ptr)[ptr.sidx];
    }
    inline void* resolve_ptr(Ptr ptr, const uint32_t& idx) {ptr.idx = idx; return resolve_ptr(ptr);}
    inline Col& resolve_to_col(Ptr ptr, const uint32_t& idx) {ptr.idx = idx; return resolve_to_col(ptr);}


    inline Ptr get_ticket_from_unit(Ptr p, uint32_t type_id, uint32_t size, uint32_t tag);

    struct string : Ptr {
        string() {}
        string(Ptr p) : Ptr(p) {}
        inline Col& col() {return resolve_to_col(*this); }
        inline char at(uint32_t idx) {return *(char*)col()[idx];}
        inline uint32_t length() {return col().length();}
        inline void push(char c) { col().push(&c); }
        inline void push(const char* s, uint32_t len) {for(uint32_t i = 0; i < len; i++) col().push(&s[i]);}
        inline void push(const std::string& s) { push(s.data(), s.length()); }
        inline void operator=(const std::string& s){ col().clear(); push(s);}
        inline void operator=(string s){ col().clear(); push((const char*)s.col().storage, s.length());}
        inline void operator=(const char* s) { col().clear(); push(s, strlen(s)); }
        inline char& operator[](uint32_t idx) { return *(char*)col().get(idx); }
        std::string to_std() {Col& c = col(); DEBUG_ONLY(if(ERROR_FLAG) {return ERROR_MSG;}); return std::string((char*)c.storage, length());}
        inline int find(string look_for, int start_at, int nth_of = 1) {
            int found_at = -1;
            for(int i=start_at;i<col().length();i++) {
                bool match = true;
                for(int s=0;s<look_for.length();s++) {
                    if(*(char*)col()[i+s]!=look_for[s]) {
                        match = false;
                        break;
                    }
                }
                if(match) {
                    if(--nth_of<=0) {
                        found_at = i; break;
                    }
                }
            }
            return found_at;
        }
    };
    string get_global_string_ticket();

    struct type_and_value {
        uint32_t type;
        Ptr value;
    };

    uint32_t offsets_col = 0;
    uint32_t tags_col = offsets_col + 1;
    uint32_t sizes_col = tags_col + 1;
    uint32_t labels_col = sizes_col + 1;
    uint32_t subtags_col = labels_col + 1;
    uint32_t subsizes_col = subtags_col + 1;
    uint32_t ptrs_col = subsizes_col + 1;

    uint32_t overloads_col = ptrs_col + 1;

    bool is_live(Ptr p) {return (p.pool!=0||p.idx!=0);}
    inline Col& global_resolve_to_col(const Ptr& ptr, const uint32_t& idx) {return global[ptr.pool][idx];}

    struct _layout {
        _layout() {}
        _layout(Ptr p) : impl(p) {}
        Ptr impl = deadptr;
        map<std::string,uint32_t> label_to_index;
        uint32_t total_size = 0;

        list<uint32_t> offsets;
        list<uint32_t> tags;
        list<uint32_t> sizes;
        list<std::string> labels;

        list<uint32_t> subtags;
        list<uint32_t> subsizes;
        list<Ptr> ptrs;

        Col overloads = Col(sizeof(type_and_value));

        void add_overload(uint64_t key, uint32_t type, Ptr value) {
            type_and_value tnv{type, value};
            overloads.put(key,(void*)&tnv);
            if(is_live(impl)) {
                resolve_to_col(impl,impl.idx+overloads_col).put(key,(void*)&tnv);
            } else {
                throw_error("Unable to add overload to layout because it's implmentation is dead");
            }
        }
        bool has_overload(uint64_t key) {
            return overloads.hasKey(key);
        }
        type_and_value get_overload(uint64_t key) {
            return *(type_and_value*)overloads.get(key);
        }


        uint32_t add_prop(uint32_t tag, uint32_t size, const std::string& label, uint32_t subtag = 0, uint32_t subsize = 0, Ptr ptr = deadptr) {
            label_to_index.put(label,offsets.length());
            offsets << total_size;
            tags << tag;
            sizes << size;
            labels << label;
            subtags << subtag;
            subsizes << subsize;
            ptrs << ptr;

            resolve_to_col(impl,impl.idx+offsets_col).push((void*)&total_size);
            resolve_to_col(impl,impl.idx+tags_col).push((void*)&tag);
            resolve_to_col(impl,impl.idx+sizes_col).push((void*)&size);
            string label_ptr = get_global_string_ticket();
            label_ptr = label;
            resolve_to_col(impl,impl.idx+labels_col).push((void*)&label_ptr);
            resolve_to_col(impl,impl.idx+subtags_col).push((void*)&subtag);
            resolve_to_col(impl,impl.idx+subsizes_col).push((void*)&subsize);
            resolve_to_col(impl,impl.idx+ptrs_col).push((void*)&ptr);

            uint32_t old_size = total_size;
            total_size += size;
            return old_size;
        }
    };

    uint32_t undefined_id = 0;
    uint32_t stages_id = 1;
    uint32_t ptr_id = 2;

    uint32_t add_type() {
        uint32_t at = global.length();
        ColCol to_return;
        global.push(to_return);
        return at;
    }

    uint32_t init_handler_type() {
        ColCol t;
        uint32_t at = global.length();
        note_value(t,"UNDEFINED",0,0);
        t.get(0).push_default(); //UNDEFINED cell
        note_value(t,"stages",sizeof(Ptr),ptr_id);
        t.get(1).put("Layouts",(void*)&deadptr); //Layouts label
        note_value(t,"ptr",sizeof(Ptr),ptr_id);
        t.get(2).push_default(); //Layout of Ptr
        global.push(t);
        return at;
    }

    uint32_t handler_type_id = init_handler_type();
    
    uint32_t init_layout_type() {
        ColCol t;
        uint32_t at = global.length();
        global.push(t);
        return at;
    }
    uint32_t layout_type_id = init_layout_type(); 

    uint32_t global_reg_id(const std::string& label) {
        uint32_t at = global[handler_type_id].length();
        note_value(global[handler_type_id],label,sizeof(Ptr),ptr_id);
        global[handler_type_id][at].push_default();
        return at;
    }

    uint32_t prefix_ptr_id = global_reg_id("prefix_ptr"); uint32_t suffix_ptr_id = global_reg_id("suffix_ptr");
    uint32_t float_id = global_reg_id("float"); uint32_t prefix_float_id = global_reg_id("prefix_float"); uint32_t suffix_float_id = global_reg_id("suffix_float");
    uint32_t int_id = global_reg_id("int"); uint32_t prefix_int_id = global_reg_id("prefix_int"); uint32_t suffix_int_id = global_reg_id("suffix_int");
    uint32_t bool_id = global_reg_id("bool"); uint32_t prefix_bool_id = global_reg_id("prefix_bool"); uint32_t suffix_bool_id = global_reg_id("suffix_bool");
    uint32_t string_id = global_reg_id("string"); uint32_t prefix_string_id = global_reg_id("prefix_string"); uint32_t suffix_string_id = global_reg_id("suffix_string");
    uint32_t char_id = global_reg_id("char"); uint32_t prefix_char_id = global_reg_id("prefix_char"); uint32_t suffix_char_id = global_reg_id("suffix_char");
    uint32_t ptr4_id = global_reg_id("ptr4"); uint32_t prefix_ptr4_id = global_reg_id("prefix_ptr4"); uint32_t suffix_ptr4_id = global_reg_id("suffix_ptr4");
    uint32_t duck_id = global_reg_id("duck"); uint32_t prefix_duck_id = global_reg_id("prefix_duck"); uint32_t suffix_duck_id = global_reg_id("suffix_duck");
    size_t list_id = global_reg_id("list");
    size_t map_id = global_reg_id("map");
    size_t weakptr_id = global_reg_id("weakptr");
    size_t col_id = global_reg_id("col");
    
    uint32_t silenced_id = global_reg_id("SILENCED");
    uint32_t any_id = global_reg_id("any");
    uint32_t null_id = global_reg_id("null");
    size_t identifier_id = global_reg_id("IDENTIFIER");
    size_t object_id = global_reg_id("OBJECT");
    size_t literal_id = global_reg_id("LITERAL");
    
    size_t node_id = global_reg_id("node"); size_t prefix_node_id = global_reg_id("prefix_node"); size_t suffix_node_id = global_reg_id("suffix_node");
    size_t value_id = global_reg_id("value"); size_t prefix_value_id = global_reg_id("prefix_value"); size_t suffix_value_id = global_reg_id("suffix_value");
    size_t context_id = global_reg_id("context"); size_t prefix_context_id = global_reg_id("prefix_context"); size_t suffix_context_id = global_reg_id("suffix_context");

    size_t var_decl_id = global_reg_id("VAR_DECL");
    size_t func_call_id = global_reg_id("FUNC_CALL");
    size_t lambda_id = global_reg_id("LAMBDA");
    size_t function_id = global_reg_id("function"); size_t prefix_function_id = global_reg_id("prefix_function"); size_t suffix_function_id = global_reg_id("suffix_function");
    size_t method_call_id = global_reg_id("METHOD_CALL");
    size_t method_id = global_reg_id("METHOD");
    size_t func_decl_id = global_reg_id("FUNC_DECL");
    size_t type_decl_id = global_reg_id("TYPE_DECL");

    uint32_t sub_pass = global_reg_id("SUB_PASS");
    uint32_t direct_pass = global_reg_id("DIRECT_PASS");
    uint32_t travel_pass = global_reg_id("TRAVEL_PASS");
    uint32_t resolving_pass = global_reg_id("RESOLVING_PASS");

    size_t tombstone_col = 0; 
    size_t refs_col = 0;

    Ptr global_add_layout_to_col(uint32_t type) {
        Ptr p((uint32_t)0,layout_type_id,note_value(global[layout_type_id],std::to_string(type)+" Offsets",4,int_id),0);
        note_value(global[layout_type_id],"Tags",4,int_id);
        note_value(global[layout_type_id],"Sizes",4,int_id);
        note_value(global[layout_type_id],"Labels",sizeof(Ptr),string_id);
        note_value(global[layout_type_id],"Subtags",4,int_id);
        note_value(global[layout_type_id],"Subsizes",4,int_id);
        note_value(global[layout_type_id],"Ptrs",sizeof(Ptr),ptr_id);
        note_value(global[layout_type_id],"Overloads",sizeof(Ptr4),ptr4_id);
        global[handler_type_id][type].set(0,(void*)&p);
        return p;
    }

    uint32_t make_store_type() {
        uint32_t at = add_type();
        return at;
    }
    
    uint32_t node_type_offset = 0;
    uint32_t node_sub_type_offset = 0;
    uint32_t node_name_offset = 0;
    uint32_t x_offset = 0;
    uint32_t y_offset = 0;
    uint32_t z_offset = 0;
    uint32_t node_value_offset = 0;
    uint32_t node_children_offset = 0;
    uint32_t node_quals_offset = 0;
    uint32_t node_node_table_offset = 0;
    uint32_t node_value_table_offset = 0;
    uint32_t node_scopes_offset = 0;
    uint32_t parent_offset = 0;
    uint32_t owner_offset = 0;
    uint32_t in_scope_offset = 0;
    uint32_t resolved_offset = 0;
    uint32_t node_opt_str_offset = 0;
    uint32_t mute_offset = 0;

    uint32_t value_type_offset = 0;
    uint32_t value_sub_type_offset = 0;
    uint32_t value_data_offset = 0;
    uint32_t address_offset = 0;
    uint32_t reg_offset = 0;
    uint32_t loc_offset = 0;
    uint32_t size_offset = 0;
    uint32_t sub_size_offset = 0;
    uint32_t value_quals_offset = 0;
    uint32_t value_sub_values_offset = 0;
    uint32_t type_scope_offset = 0;
    uint32_t store_offset = 0;

    uint32_t context_node_offset = 0;
    uint32_t context_qual_offset = 0;
    uint32_t context_left_offset = 0;
    uint32_t context_out_offset = 0;
    uint32_t context_root_offset = 0;
    uint32_t context_result_offset = 0;
    uint32_t context_value_offset = 0;
    uint32_t context_index_offset = 0;
    uint32_t context_state_offset = 0;
    uint32_t context_flag_offset = 0;
    uint32_t context_sub_offset = 0;
    uint32_t context_source_offset = 0;
    uint32_t context_pass_offset = 0;
    uint32_t context_parent_offset = 0;

    uint32_t node_total_size = 0;
    uint32_t value_total_size = 0;
    uint32_t context_total_size = 0;

    uint32_t init_node_type();
    uint32_t init_value_type();
    uint32_t init_context_type();

    uint32_t name_store_id = make_store_type();
    uint32_t node_type_id = init_node_type();
    uint32_t value_type_id = init_value_type();
    uint32_t context_type_id = init_context_type();
    uint32_t children_store_id = make_store_type();
    uint32_t quals_store_id = make_store_type();
    uint32_t node_table_store_id = make_store_type(); 
    uint32_t value_table_store_id = make_store_type(); 
    uint32_t scopes_store_id = make_store_type(); 
    uint32_t opt_str_store_id = make_store_type();
    uint32_t data_store_id = make_store_type();
    uint32_t sub_value_store_id = make_store_type();

    string get_global_string_ticket() {
        Ptr ticket((uint32_t)0,name_store_id,create_column(global[name_store_id],1,char_id),0);
        return ticket;
    }

    Ptr global_add_template(uint32_t for_type) {
        Ptr p = global_add_layout_to_col(for_type);
        return p;
    }

    uint32_t init_node_type() {
        uint32_t at = add_type();
        ColCol& t = global[at];
        _layout ntemp(global_add_template(node_id)); //Node template
        node_type_offset = ntemp.add_prop(int_id,4,"type");
        node_sub_type_offset = ntemp.add_prop(int_id,4,"sub_type");
        node_name_offset = ntemp.add_prop(string_id,sizeof(Ptr),"name",char_id,1);
        x_offset = ntemp.add_prop(float_id,4,"x");
        y_offset = ntemp.add_prop(float_id,4,"y");
        z_offset = ntemp.add_prop(float_id,4,"z");
        node_value_offset = ntemp.add_prop(value_id,sizeof(Ptr),"value");
        node_children_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"children",node_id,sizeof(Ptr));
        node_quals_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"quals",node_id,sizeof(Ptr));
        node_node_table_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"node_table",node_id,sizeof(Ptr));
        node_value_table_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"value_table",value_id,sizeof(Ptr));
        node_scopes_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"scopes",node_id,sizeof(Ptr));
        parent_offset = ntemp.add_prop(node_id,sizeof(Ptr),"parent");
        owner_offset = ntemp.add_prop(node_id,sizeof(Ptr),"owner");
        in_scope_offset = ntemp.add_prop(node_id,sizeof(Ptr),"in_scope");
        resolved_offset = ntemp.add_prop(bool_id,1,"resolved");
        node_opt_str_offset = ntemp.add_prop(string_id,sizeof(Ptr),"opt_str");
        mute_offset = ntemp.add_prop(bool_id,1,"mute");
        node_total_size = ntemp.total_size;
        return at;
    }

    uint32_t init_value_type() {
        uint32_t at = add_type();
        ColCol& t = global[at];

        _layout vtemp(global_add_template(value_id)); //Value template
        value_type_offset = vtemp.add_prop(int_id,4,"type");
        value_sub_type_offset = vtemp.add_prop(int_id,4,"sub_type");
        value_data_offset = vtemp.add_prop(ptr_id,sizeof(Ptr),"data");
        address_offset = vtemp.add_prop(int_id,4,"address");
        reg_offset = vtemp.add_prop(int_id,4,"reg");
        loc_offset = vtemp.add_prop(int_id,4,"loc");
        size_offset = vtemp.add_prop(int_id,4,"size");
        sub_size_offset = vtemp.add_prop(int_id,4,"sub_size");
        value_quals_offset = vtemp.add_prop(ptr_id,sizeof(Ptr),"quals",node_id,sizeof(Ptr));
        value_sub_values_offset = vtemp.add_prop(ptr_id,sizeof(Ptr),"sub_values",value_id,sizeof(Ptr));
        type_scope_offset = vtemp.add_prop(node_id,sizeof(Ptr),"type_scope");
        store_offset = vtemp.add_prop(ptr_id,sizeof(Ptr),"store");
        value_total_size = vtemp.total_size;
        return at;
    }

    uint32_t init_context_type() {
        uint32_t at = add_type();
        ColCol& t = global[at];
        _layout ctemp(global_add_template(context_id)); //Context template
        context_node_offset = ctemp.add_prop(node_id,sizeof(Ptr),"node");
        context_qual_offset = ctemp.add_prop(node_id,sizeof(Ptr),"qual");
        context_left_offset = ctemp.add_prop(node_id,sizeof(Ptr),"left");
        context_out_offset = ctemp.add_prop(node_id,sizeof(Ptr),"out");
        context_root_offset = ctemp.add_prop(node_id,sizeof(Ptr),"root");
        context_result_offset = ctemp.add_prop(ptr_id,sizeof(Ptr),"result",node_id,sizeof(Ptr));
        context_value_offset = ctemp.add_prop(value_id,sizeof(Ptr),"value");
        context_index_offset = ctemp.add_prop(int_id,4,"index");
        context_state_offset = ctemp.add_prop(int_id,4,"state");
        context_flag_offset = ctemp.add_prop(bool_id,1,"flag");
        context_sub_offset = ctemp.add_prop(context_id,sizeof(Ptr),"sub");
        context_source_offset = ctemp.add_prop(string_id,sizeof(Ptr),"source",char_id,1);
        context_pass_offset = ctemp.add_prop(int_id,4,"pass");
        context_parent_offset = ctemp.add_prop(context_id,sizeof(Ptr),"parent");
        context_total_size = ctemp.total_size;
        return at;
    }


    template<typename T>
    struct col_Ptr : Ptr {
        col_Ptr(uint32_t _unit, uint32_t _pool, uint32_t _idx)  : Ptr(_unit,_pool,_idx,0) {}

        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)) {throw_error("Attempted ",log_msg," but col_ptr was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline Col& col()                    {DEBUG_ONLY(if(safety_check("col_ptr:col")){static Col d; return d;}) return resolve_to_col(*this);}
        inline uint32_t length()             {DEBUG_ONLY(if(safety_check("col_ptr:length")){return 0;}) return col().length();}
        inline bool empty()                  {DEBUG_ONLY(if(safety_check("col_ptr:empty")){return true;}) return col().empty();}
        inline void removeAt(uint32_t idx)   {DEBUG_ONLY(if(safety_check("col_ptr:removeAt")){return;}) col().removeAt(idx);}
        inline void clear()                  {DEBUG_ONLY(if(safety_check("col_ptr:clear")){return;}) col().clear();}
    
        inline T get(uint32_t idx)           {DEBUG_ONLY(if(safety_check("col_ptr:get")){return T(deadptr);}) void* ptr = col().get(idx); DEBUG_ONLY(if(safety_check("col_ptr:get:cast")){return T(deadptr);}) return T(*(Ptr*)ptr);}
        inline T operator[](uint32_t idx)    {return get(idx);}
        inline T last()                      {DEBUG_ONLY(if(safety_check("col_ptr:last")){return T(deadptr);}) return get(length()-1);}
    
        inline T take(uint32_t idx)          {DEBUG_ONLY(if(safety_check("col_ptr:take")){return T(deadptr);}) T val = get(idx); removeAt(idx); return val;}
        inline T pop()                       {DEBUG_ONLY(if(safety_check("col_ptr:pop")){return T(deadptr);}) Ptr p; col().pop(&p); return T(p);}
        inline void push(T t)                {DEBUG_ONLY(if(safety_check("col_ptr:push")){return;}) Ptr p(t); col().push(&p);}
        inline void operator<<(T t)          {push(t);}
        inline void insert( uint32_t idx, T t){DEBUG_ONLY(if(safety_check("col_ptr:insert")){return;}) Ptr p(t); col().insert(idx,&p);}
    
        inline bool hasKey(const std::string& key) {DEBUG_ONLY(if(safety_check("col_ptr:hasKey")){return false;}) return col().hasKey(key);}
        inline T get(const std::string& key) {DEBUG_ONLY(if(safety_check("col_ptr:get_key")){return T(deadptr);}) return T(*(Ptr*)col().get(key));}
        inline T operator[](const std::string& key) {return get(key);}
        inline void put(const std::string& key, T t) {DEBUG_ONLY(if(safety_check("col_ptr:put")){return;}) col().put(key, (void*)&t);}
    };
    using node_col  = col_Ptr<Node>;
    using value_col = col_Ptr<Value>;

    struct Value : public Ptr {
        Value() {}
        Value(Ptr p) : Ptr(p) {}

        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)) {throw_error("Attempted ",log_msg," but value was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline uint32_t  type()                {DEBUG_ONLY(if(safety_check("value:type:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(value_type_offset);}
        inline void      type(uint32_t t)      {DEBUG_ONLY(if(safety_check("value:type:set")){return;}) resolve_to_col(*this).qset(value_type_offset,(void*)&t,4);}
        inline uint32_t  sub_type()            {DEBUG_ONLY(if(safety_check("value:sub_type:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(value_sub_type_offset);}
        inline void      sub_type(uint32_t st) {DEBUG_ONLY(if(safety_check("value:sub_type:set")){return;}) resolve_to_col(*this).qset(value_sub_type_offset,(void*)&st,4);}
        
        inline Ptr&      data_ptr()            {DEBUG_ONLY(if(safety_check("value:data_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(value_data_offset);}
        inline void      data_ptr(Ptr ptr)     {DEBUG_ONLY(if(safety_check("value:data_ptr:set")){return;}) resolve_to_col(*this).qset(value_data_offset,(void*)&ptr,sizeof(Ptr));}
        inline Col&      data_col()            {Ptr p = data_ptr(); return resolve_to_col(p);}
        
        inline uint32_t  address()             {DEBUG_ONLY(if(safety_check("value:address:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(address_offset);}
        inline void      address(uint32_t v)   {DEBUG_ONLY(if(safety_check("value:address:set")){return;}) resolve_to_col(*this).qset(address_offset,(void*)&v,4);}
        inline int       reg()                 {DEBUG_ONLY(if(safety_check("value:reg:get")){return -1;}) return *(int*)resolve_to_col(*this).qget(reg_offset);}
        inline void      reg(int i)            {DEBUG_ONLY(if(safety_check("value:reg:set")){return;}) resolve_to_col(*this).qset(reg_offset,(void*)&i,4);}
        inline int       loc()                 {DEBUG_ONLY(if(safety_check("value:loc:get")){return -1;}) return *(int*)resolve_to_col(*this).qget(loc_offset);}
        inline void      loc(int i)            {DEBUG_ONLY(if(safety_check("value:loc:set")){return;}) resolve_to_col(*this).qset(loc_offset,(void*)&i,4);}
        
        inline uint32_t  size()                {DEBUG_ONLY(if(safety_check("value:size:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(size_offset);}
        inline void      size(uint32_t s)      {DEBUG_ONLY(if(safety_check("value:size:set")){return;}) resolve_to_col(*this).qset(size_offset,(void*)&s,4);}
        inline uint32_t  sub_size()            {DEBUG_ONLY(if(safety_check("value:sub_size:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(sub_size_offset);}
        inline void      sub_size(uint32_t s)  {DEBUG_ONLY(if(safety_check("value:sub_size:set")){return;}) resolve_to_col(*this).qset(sub_size_offset,(void*)&s,4);}
        
        inline Ptr&      quals_ptr()           {DEBUG_ONLY(if(safety_check("value:quals_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(value_quals_offset);}
        inline Col&      quals_col()           {Ptr& p = quals_ptr(); return resolve_to_col(p);}
        inline node_col  quals()               {return (node_col&)quals_ptr();}
        
        inline Ptr&       sub_values_ptr()     {DEBUG_ONLY(if(safety_check("value:sub_values_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(value_sub_values_offset);}
        inline Col&       sub_values_col()     {Ptr& p = sub_values_ptr(); return resolve_to_col(p);}
        inline value_col  sub_values()         {return (value_col&)sub_values_ptr();}
        
        inline Node      type_scope();
        inline void      type_scope(Ptr o)     {DEBUG_ONLY(if(safety_check("value:type_scope:set")){return;}) resolve_to_col(*this).qset(type_scope_offset,(void*)&o,sizeof(Ptr));}

        inline Ptr&      store_ptr()           {DEBUG_ONLY(if(safety_check("value:store:get")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(store_offset);}
        inline Col&      store_col()           {Ptr& p = store_ptr(); return resolve_to_col(p);}
        inline ColCol&   store_pool()          {Ptr& p = store_ptr(); return resolve_to_pool(p);}
        inline ColColCol& store_unit()         {Ptr& p = store_ptr(); return resolve_to_unit(p);}
        inline void      store(Ptr p)          {DEBUG_ONLY(if(safety_check("value:store:set")){return;}) resolve_to_col(*this).qset(store_offset,(void*)&p,sizeof(Ptr));}
    
        inline void setup(uint32_t _type, uint32_t _size, uint32_t _address = 0) {
            type(_type); size(_size); address(_address);
        }
    
        inline Ptr init_data() {
            Ptr dataptr = get_ticket_from_unit(*this,data_store_id,size(),type());
            resolve_to_col(dataptr).push_default();
            resolve_to_col(*this).qset(value_data_offset,(void*)&dataptr,sizeof(Ptr));
            return dataptr;
        }
    
        inline void set(void* data) {
            DEBUG_ONLY(if(safety_check("value:set")){return;})
            Ptr dataptr = data_ptr();
            if(!is_live(dataptr)) {
                dataptr = init_data();
            }
            if(resolve_to_col(dataptr).heterogenous) {
                resolve_to_col(dataptr).qset(dataptr.sidx, data, size());
            } else {
                resolve_to_col(dataptr).set(dataptr.sidx, data);
            }
        }
    
        inline void* get() {
            DEBUG_ONLY(if(safety_check("value:get")){return nullptr;})
            Ptr dataptr = data_ptr();
            return resolve_to_col(dataptr).get(dataptr.sidx);
        }

        inline void* sget() {
            DEBUG_ONLY(if(safety_check("value:sget")){return nullptr;})
            Ptr dataptr = data_ptr();
            return resolve_to_col(dataptr).sget(dataptr.sidx);
        }
        
        inline void* qget() {
            DEBUG_ONLY(if(safety_check("value:qget")){return nullptr;})
            Ptr dataptr = data_ptr();
            return resolve_to_col(dataptr).qget(dataptr.sidx);
        }

        inline void copy(Value o, bool is_deep) { //Do we make the deep copying happen here or in acorn-compiler?
            Col& src = resolve_to_col(o);
            Col& dst = resolve_to_col(*this);
            memcpy(dst.storage, src.storage, value_total_size);
            if(is_deep) {
                if(is_live(o.data_ptr())&&!resolve_to_col(o.data_ptr()).empty()) {
                    init_data();
                    set(o.get());
                }
                Ptr qualsptr = get_ticket_from_unit(*this, quals_store_id, sizeof(Ptr), ptr_id);
                Col& new_quals = resolve_to_col(qualsptr);
                Col& old_quals = o.quals_col();
                new_quals.reserve(old_quals.size);
                memcpy(new_quals.storage, old_quals.storage, old_quals.size);
                new_quals.size = old_quals.size;
                resolve_to_col(*this).qset(value_quals_offset,(void*)&qualsptr,sizeof(Ptr));
                Ptr subvalsptr = get_ticket_from_unit(*this, sub_value_store_id, sizeof(Ptr), ptr_id);
                Col& new_subvals = resolve_to_col(subvalsptr);
                Col& old_subvals = o.sub_values_col();
                new_subvals.reserve(old_subvals.size);
                memcpy(new_subvals.storage, old_subvals.storage, old_subvals.size);
                new_subvals.size = old_subvals.size;
                resolve_to_col(*this).qset(value_sub_values_offset,(void*)&subvalsptr,sizeof(Ptr));
            }
        }

        int find_qual(uint32_t q_id);
        Node get_qual(uint32_t q_id);
    
        bool has_qual(uint32_t q_id) {
            return find_qual(q_id)!=-1;
        }

        uint32_t count_qual(uint32_t q_id);
    };
    
    struct QNode : public Col {
        inline uint32_t& type()                   {return *(uint32_t*)&storage[node_type_offset];}
        inline void      type(uint32_t t)         {memcpy(&storage[node_type_offset],(void*)&t,4);}
        inline uint32_t& sub_type()               {return *(uint32_t*)&storage[node_sub_type_offset];}
        inline void      sub_type(uint32_t st)    {memcpy(&storage[node_sub_type_offset],(void*)&st,4);}
    };



    struct Node : public Ptr {
        Node() {}
        Node(Ptr p) : Ptr(p) {}
    
        inline QNode& toQ() {return (QNode&)resolve_to_col(*this);}

        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)||resolve_to_col(*this).empty()) {throw_error("Attempted ",log_msg," but node "+Ptr_to_string(*this)+" was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline uint32_t  type()                {DEBUG_ONLY(if(safety_check("node:type:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(node_type_offset);}
        inline void      type(uint32_t t)      {DEBUG_ONLY(if(safety_check("node:type:set")){return;}) resolve_to_col(*this).qset(node_type_offset,(void*)&t,4);}
        inline uint32_t  sub_type()            {DEBUG_ONLY(if(safety_check("node:sub_type:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(node_sub_type_offset);}
        inline void      sub_type(uint32_t st) {DEBUG_ONLY(if(safety_check("node:sub_type:set")){return;}) resolve_to_col(*this).qset(node_sub_type_offset,(void*)&st,4);}
        
        inline Ptr&      name_ptr()            {DEBUG_ONLY(if(safety_check("node:name_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(node_name_offset);}
        inline Col&      name_col()            {Ptr& p = name_ptr(); return resolve_to_col(p);}
        inline string    name()                {return string(name_ptr());}
        inline void      name(std::string s)   {DEBUG_ONLY(if(safety_check("node:name:set")){return;}) name() = s;}
        
        inline float     x()                   {DEBUG_ONLY(if(safety_check("node:x:get")){return -1.0f;}) return *(float*)resolve_to_col(*this).qget(x_offset);}
        inline void      x(float v)            {DEBUG_ONLY(if(safety_check("node:x:set")){return;}) resolve_to_col(*this).qset(x_offset,(void*)&v,4);}
        inline float     y()                   {DEBUG_ONLY(if(safety_check("node:y:get")){return -1.0f;}) return *(float*)resolve_to_col(*this).qget(y_offset);}
        inline void      y(float v)            {DEBUG_ONLY(if(safety_check("node:y:set")){return;}) resolve_to_col(*this).qset(y_offset,(void*)&v,4);}
        inline float     z()                   {DEBUG_ONLY(if(safety_check("node:z:get")){return -1.0f;}) return *(float*)resolve_to_col(*this).qget(z_offset);}
        inline void      z(float v)            {DEBUG_ONLY(if(safety_check("node:z:set")){return;}) resolve_to_col(*this).qset(z_offset,(void*)&v,4);}
        
        inline Ptr       value_ptr()           {DEBUG_ONLY(if(safety_check("node:value_ptr")){return deadptr;}) return *(Ptr*)resolve_to_col(*this).qget(node_value_offset);}
        inline Value     value()               {return Value(value_ptr());}
        inline void      value(Ptr ptr)        {DEBUG_ONLY(if(safety_check("node:value:set")){return;}) resolve_to_col(*this).qset(node_value_offset,(void*)&ptr,sizeof(Ptr));}
        
        inline Ptr&      children_ptr()        {DEBUG_ONLY(if(safety_check("node:children_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(node_children_offset);}
        inline Col&      children_col()        {Ptr& p = children_ptr(); return resolve_to_col(p);}
        inline node_col  children()            {return (node_col&)children_ptr();}
        inline void      children(node_col l)  {DEBUG_ONLY(if(safety_check("node:children:set")){return;}) resolve_to_col(*this).qset(node_children_offset,(void*)&l,sizeof(Ptr));}
        
        inline Ptr&      quals_ptr()           {DEBUG_ONLY(if(safety_check("node:quals_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(node_quals_offset);}
        inline Col&      quals_col()           {Ptr& p = quals_ptr(); return resolve_to_col(p);}
        inline node_col  quals()               {return (node_col&)quals_ptr();}
    
        inline Ptr&        node_table_ptr()    {DEBUG_ONLY(if(safety_check("node:node_table_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(node_node_table_offset);}
        inline Col&        node_table_col()    {Ptr& p = node_table_ptr(); return resolve_to_col(p);}
        inline node_col    node_table()        {return (node_col&)node_table_ptr();}
        
        inline Ptr&        value_table_ptr()   {DEBUG_ONLY(if(safety_check("node:value_table_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(node_value_table_offset);}
        inline Col&        value_table_col()   {Ptr& p = value_table_ptr(); return resolve_to_col(p);}
        inline value_col   value_table()       {return (value_col&)value_table_ptr();}
        
        inline Ptr&      scopes_ptr()          {DEBUG_ONLY(if(safety_check("node:scopes_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(node_scopes_offset);}
        inline Col&      scopes_col()          {Ptr& p = scopes_ptr(); return resolve_to_col(p);}
        inline node_col  scopes()              {return (node_col&)scopes_ptr();}
        
        inline Ptr&  parent_ptr()              {DEBUG_ONLY(if(safety_check("node:parent_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(parent_offset);}
        inline Node  parent()                  {return Node(parent_ptr());}
        inline void  parent(Ptr p)             {DEBUG_ONLY(if(safety_check("node:parent:set")){return;}) resolve_to_col(*this).qset(parent_offset,(void*)&p,sizeof(Ptr));}
        
        inline Ptr&  owner_ptr()               {DEBUG_ONLY(if(safety_check("node:owner_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(owner_offset);}
        inline Node  owner()                   {return Node(owner_ptr());}
        inline void  owner(Ptr p)              {DEBUG_ONLY(if(safety_check("node:owner:set")){return;}) resolve_to_col(*this).qset(owner_offset,(void*)&p,sizeof(Ptr));}
        
        inline Ptr&  in_scope_ptr()            {DEBUG_ONLY(if(safety_check("node:in_scope_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(in_scope_offset);}
        inline Node  in_scope()                {return Node(in_scope_ptr());}
        inline void  in_scope(Ptr p)           {DEBUG_ONLY(if(safety_check("node:in_scope:set")){return;}) resolve_to_col(*this).qset(in_scope_offset,(void*)&p,sizeof(Ptr));}
                
        inline Ptr&   opt_str_ptr()            {DEBUG_ONLY(if(safety_check("node:opt_str_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(node_opt_str_offset);}
        inline Col&   opt_str_col()            {Ptr& p = opt_str_ptr(); return resolve_to_col(p);}
        inline string opt_str()                {return string(opt_str_ptr());}
        
        inline bool  mute()                    {DEBUG_ONLY(if(safety_check("node:mute:get")){return false;}) return *(bool*)resolve_to_col(*this).qget(mute_offset);}
        inline void  mute(bool b)              {DEBUG_ONLY(if(safety_check("node:mute:set")){return;}) resolve_to_col(*this).qset(mute_offset,(void*)&b,1);}
    
        inline bool  resolved()                {DEBUG_ONLY(if(safety_check("node:resolved:get")){return false;}) return *(bool*)resolve_to_col(*this).qget(resolved_offset);}
        inline void  resolved(bool b)          {DEBUG_ONLY(if(safety_check("node:resolved:set")){return;}) resolve_to_col(*this).qset(resolved_offset,(void*)&b,1);}
    
        inline void copy(Node o) {
            Col& src = resolve_to_col(o);
            Col& dst = resolve_to_col(*this);
            memcpy(dst.storage, src.storage, node_total_size);
        }

        int find_qual(uint32_t q_id) {
            for(int i=0;i<quals().length();i++){ 
                if(quals()[i].type()==q_id) {return i;}
            }
            return -1;
        } 
    
        int find_qual_in_value(uint32_t q_id) {
            if(is_live(value()))
                return value().find_qual(q_id);
            return -1;
        }
        
        bool has_qual(size_t q_id, bool check_value = true) {
            if(check_value&&find_qual_in_value(q_id)!=-1) return true;
            if(find_qual(q_id)!=-1) return true;
            return false;
        }

        uint32_t count_qual(size_t q_id, bool check_value = true) {
            uint32_t count = 0;
            if(check_value&&is_live(value())) {
                count += value().count_qual(q_id);
            }
            for(int i=0;i<quals().length();i++){ 
                if(quals()[i].type()==q_id) count++;
            }
            return count;
        }
    };

    inline Node Value::type_scope() {return Node(*(Ptr*) resolve_to_col(*this).qget(type_scope_offset));}
    int Value::find_qual(uint32_t q_id) {
        for(int i=0;i<quals().length();i++){ 
            if(quals()[i].type()==q_id) {return i;}
        }
        return -1;
    } 
    Node Value::get_qual(uint32_t q_id) {
        int q_at = find_qual(q_id);
        if(q_at!=-1) {
            return quals()[q_at];
        }
        return deadptr;
    } 
    uint32_t Value::count_qual(uint32_t q_id) {
        uint32_t count = 0;
        for(int i=0;i<quals().length();i++){ 
            if(quals()[i].type()==q_id) count++;
        }
        return count;
    }

    struct Context : public Ptr {
        Context() {}
        Context(Ptr p) : Ptr(p) {}
    
        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)) {throw_error("Attempted ",log_msg," but context was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline Ptr&     node_ptr()           {DEBUG_ONLY(if(safety_check("context:node:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_node_offset);}
        inline Node     node()               {return Node(node_ptr());}
        inline void     node(Ptr p)          {DEBUG_ONLY(if(safety_check("context:node:set")){return;}) resolve_to_col(*this).qset(context_node_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     qual_ptr()           {DEBUG_ONLY(if(safety_check("context:qual:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_qual_offset);}
        inline Node     qual()               {return Node(qual_ptr());}
        inline void     qual(Ptr p)          {DEBUG_ONLY(if(safety_check("context:qual:set")){return;}) resolve_to_col(*this).qset(context_qual_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     left_ptr()           {DEBUG_ONLY(if(safety_check("context:left:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_left_offset);}
        inline Node     left()               {return Node(left_ptr());}
        inline void     left(Ptr p)          {DEBUG_ONLY(if(safety_check("context:left:set")){return;}) resolve_to_col(*this).qset(context_left_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     out_ptr()            {DEBUG_ONLY(if(safety_check("context:out:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_out_offset);}
        inline Node     out()                {return Node(out_ptr());}
        inline void     out(Ptr p)           {DEBUG_ONLY(if(safety_check("context:out:set")){return;}) resolve_to_col(*this).qset(context_out_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     root_ptr()           {DEBUG_ONLY(if(safety_check("context:root:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_root_offset);}
        inline Node     root()               {return Node(root_ptr());}
        inline void     root(Ptr p)          {DEBUG_ONLY(if(safety_check("context:root:set")){return;}) resolve_to_col(*this).qset(context_root_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     value_ptr()          {DEBUG_ONLY(if(safety_check("context:value:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_value_offset);}
        inline Value    value()              {return Value(value_ptr());}
        inline void     value(Ptr p)         {DEBUG_ONLY(if(safety_check("context:value:set")){return;}) resolve_to_col(*this).qset(context_value_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     result_ptr()         {DEBUG_ONLY(if(safety_check("context:result:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_result_offset);}
        inline Col&     result_col()         {Ptr& p = result_ptr(); return resolve_to_col(p);}
        inline node_col result()             {return (node_col&)result_ptr();}
        inline void     result(Ptr p)        {DEBUG_ONLY(if(safety_check("context:result:set")){return;}) resolve_to_col(*this).qset(context_result_offset,(void*)&p,sizeof(Ptr));}
    
        inline int&     index()              {DEBUG_ONLY(if(safety_check("context:index:get")){return _ctx_dummy_index;}) return *(int*)resolve_to_col(*this).qget(context_index_offset);}
        inline void     index(int i)         {DEBUG_ONLY(if(safety_check("context:index:set")){return;}) resolve_to_col(*this).qset(context_index_offset,(void*)&i,4);}
    
        inline Ptr&     sub_ptr()            {DEBUG_ONLY(if(safety_check("context:sub:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_sub_offset);}
        inline Context  sub()                {return Context(sub_ptr());}
        inline void     sub(Ptr p)           {DEBUG_ONLY(if(safety_check("context:sub:set")){return;}) resolve_to_col(*this).qset(context_sub_offset,(void*)&p,sizeof(Ptr));}

        inline Ptr&     parent_ptr()         {DEBUG_ONLY(if(safety_check("context:parent:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_parent_offset);}
        inline Context  parent()             {return Context(parent_ptr());}
        inline void     parent(Ptr p)        {DEBUG_ONLY(if(safety_check("context:parent:set")){return;}) resolve_to_col(*this).qset(context_parent_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     source_ptr()         {DEBUG_ONLY(if(safety_check("context:source:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_source_offset);}
        inline Col&     source_col()         {Ptr& p = source_ptr(); return resolve_to_col(p);}
        inline string   source()             {return string(source_ptr());}
        inline void     source(Ptr p)        {DEBUG_ONLY(if(safety_check("context:source:set")){return;}) resolve_to_col(*this).qset(context_source_offset,(void*)&p,sizeof(Ptr));}
        inline void     source(std::string s){DEBUG_ONLY(if(safety_check("context:source:set")){return;}) source() = s;}
    
        inline uint32_t state()              {DEBUG_ONLY(if(safety_check("context:state:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(context_state_offset);}
        inline void     state(uint32_t s)    {DEBUG_ONLY(if(safety_check("context:state:set")){return;}) resolve_to_col(*this).qset(context_state_offset,(void*)&s,4);}

        inline uint32_t pass()              {DEBUG_ONLY(if(safety_check("context:pass:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(context_pass_offset);}
        inline void     pass(uint32_t s)    {DEBUG_ONLY(if(safety_check("context:pass:set")){return;}) resolve_to_col(*this).qset(context_pass_offset,(void*)&s,4);}
    
        inline bool     flag()               {DEBUG_ONLY(if(safety_check("context:flag:get")){return false;}) return *(bool*)resolve_to_col(*this).qget(context_flag_offset);}
        inline void     flag(bool b)         {DEBUG_ONLY(if(safety_check("context:flag:set")){return;}) resolve_to_col(*this).qset(context_flag_offset,(void*)&b,1);}
    };

    bool init_type_pool() {
        global[handler_type_id].label = "handlers";
        global[layout_type_id].label = "layouts";
        global[node_type_id].label = "nodes";
        global[value_type_id].label = "values";
        global[context_type_id].label = "contexts";
        global[name_store_id].label = "names";
        global[children_store_id].label = "children";
        global[quals_store_id].label = "quals";
        global[node_table_store_id].label = "node table";
        global[value_table_store_id].label = "value table";
        global[scopes_store_id].label = "scopes";
        global[opt_str_store_id].label = "opt_str";
        global[data_store_id].label = "data";
        global[sub_value_store_id].label = "sub_value";
        return true;
    }
    bool type_pool_intilized = init_type_pool();


    using Handler = std::function<void(Context&)>;

    struct Stage : q_object {
        Stage() {
            default_function = [](Context& ctx){};
        }

        map<uint32_t,Handler> handlers;
        Handler default_function;

        std::string label;

        bool has(uint32_t key){
            return handlers.hasKey(key);
        }

        Handler& run(uint32_t key){
            return handlers.getOrDefault(key,default_function);
        }

        Handler& getOrDefault(uint32_t key, Handler& fallback){
            return handlers.getOrDefault(key,fallback);
        }

        Handler& operator[](uint32_t key) {
            return handlers[key];
        }
    };

    struct Watcher {
        Watcher(){};
        Watcher(std::string _label) : label(_label) {};
        std::string label = "";
        Handler stagestart = nullptr;
        Handler prefix = nullptr;
        Handler suffix = nullptr;
        Handler stagend = nullptr;
        Handler logger = nullptr;
        Handler passstart = nullptr;
    };  

    static void write_TypeCol(std::ostream& out, ColCol& type) {
        write_raw<uint32_t>(out, type.length());
        write_col_header(out, type);
        for(int c = 0; c < type.length(); c++) {
            Col& col = type[c];
            write_col(out, col);
        }
    }
    
    static ColCol read_TypeCol(std::istream& in) {
        uint32_t len = read_raw<uint32_t>(in);
        ColCol type = read_col_header(in);
        for(uint32_t i = 0; i < len; i++) {
            Col col = read_col(in);
            type.push(col);
        }
        return type;
    }

    static void write_TypeTypeCol(std::ostream& out, ColColCol& col) {
        write_raw<uint32_t>(out, col.length());
        write_col_header(out, col);
        for(int i = 0; i < col.length(); i++) {
            write_TypeCol(out,col[i]);
        }
    }

    static ColColCol read_TypeTypeCol(std::istream& in) {
        uint32_t len = read_raw<uint32_t>(in);
        ColColCol col = read_col_header(in);
        for(uint32_t p = 0; p < len; p++) {
            ColCol cc = read_TypeCol(in);
            col.push(cc);
        }
        return col;
    }

    static void write_ColColList(std::ostream& out, list<ColCol*> cols) {
        write_raw<uint32_t>(out, cols.length());
        for(int i=0;i<cols.length();i++) {
            write_TypeCol(out,*cols[i]);
        }
    }
    static list<ColCol> read_ColColList(std::ifstream& in) {
        list<ColCol> to_return;
        uint32_t len = read_raw<uint32_t>(in);
        for(int i=0;i<len;i++) {
            to_return << read_TypeCol(in);
        }
        return to_return;
    }

    class Unit : public q_object {
        public:
        Stage* active_stage;
        list<Watcher> watchers;
        g_ptr<Log::Span> uspan = make<Log::Span>();

        bool running = true; 
        Context unit_ctx = deadptr;

        uint16_t derive_uid(bool init_layouts) {
            uid = (uint16_t)units.length();

            if(init_layouts) {
                ColCol& h = global[handler_type_id];
                for(int i = 0; i < h.length(); i++) {
                    Col& handler_col = h[i];
        
                    labels.put(i,handler_col.label.to_std());
                    labels_lookup.put(handler_col.label.to_std(),i);
        
                    if(handler_col.empty()) continue;
                    Ptr lptr = *(Ptr*)handler_col.sget(0);
                    if(!is_live(lptr)) continue;
                    _layout l(lptr);
            
                    Col& offsets_c  = resolve_to_col(lptr, lptr.idx + offsets_col);
                    Col& tags_c     = resolve_to_col(lptr, lptr.idx + tags_col);
                    Col& sizes_c    = resolve_to_col(lptr, lptr.idx + sizes_col);
                    Col& labels_c   = resolve_to_col(lptr, lptr.idx + labels_col);
                    Col& subtags_c  = resolve_to_col(lptr, lptr.idx + subtags_col);
                    Col& subsizes_c = resolve_to_col(lptr, lptr.idx + subsizes_col);
                    Col& ptrs_c     = resolve_to_col(lptr, lptr.idx + ptrs_col);
                    //l.overloads     = resolve_to_col(lptr, lptr.idx + overloads_col);
            
                    uint32_t count = offsets_c.size / sizeof(uint32_t);
                    for(uint32_t f = 0; f < count; f++) {
                        uint32_t offset  = *(uint32_t*)offsets_c.sget(f);
                        uint32_t tag     = *(uint32_t*)tags_c.sget(f);
                        uint32_t size    = *(uint32_t*)sizes_c.sget(f);
                        Ptr      label_p = *(Ptr*)labels_c.sget(f);
                        uint32_t subtag  = *(uint32_t*)subtags_c.sget(f);
                        uint32_t subsize = *(uint32_t*)subsizes_c.sget(f);
                        Ptr      ptr     = *(Ptr*)ptrs_c.sget(f);
                        
                        std::string label_str = string(label_p).to_std();
                        l.label_to_index.put(label_str, l.offsets.length());
                        l.offsets  << offset;
                        l.tags     << tag;
                        l.sizes    << size;
                        l.labels   << label_str;
                        l.subtags  << subtag;
                        l.subsizes << subsize;
                        l.ptrs     << ptr;
                    }
                    l.total_size = l.offsets.length() > 0 
                        ? l.offsets.last() + l.sizes.last() 
                        : 0;
                    l.impl.unit = uid;
                    layouts.put(i,l);
                }
            }

            std::lock_guard<std::mutex> lock(units_mutex);
            units << this;
            return (uint16_t)units.length()-1;
        }

        Unit() : types(global), uid(derive_uid(true)) {init();}
        Unit(const ColColCol& starter) : types(starter), uid(derive_uid(false)) {init();}
        Unit(bool do_not_init) {}

        void setup_standard_watchers() {
            Watcher def("core");
            def.stagestart = [this](Context& ctx){
                if(active_stage) {
                    newline(active_stage->label);
                }
            };
            def.prefix = [this](Context& ctx){
                newline(active_stage->label+": "+node_info(ctx.node()));
            };
            def.suffix = [this](Context& ctx){
                if(ERROR_FLAG) {log(red("Marked "+Ptr_as_string(ctx.node())+" as error")); mark_error(ctx.node());}
                log(green("After: "),node_info(ctx.node()));
                endline();
            };
            def.stagend = [this](Context& ctx){
                endline();
            };
            watchers << def;
        }

        void setup_uspan_standard_watchers() {
            Watcher def("uspan_core");
            def.stagestart = [this](Context& ctx){
                if(active_stage) {
                    uspan->newline(active_stage->label);
                }
            };
            def.passstart = [this](Context& ctx){
                if(ctx.pass()!=0) {
                    uspan->newline(labels[ctx.pass()]+" over "+std::to_string(ctx.result().length())+" nodes");
                }
            };
            def.prefix = [this](Context& ctx){
                if(is_live(ctx.qual())) {
                    uspan->newline(active_stage->label+": "+labels[ctx.qual().type()]+" in "+ctx.node().name().to_std());
                } else {
                    uspan->newline(active_stage->label+": "+node_basic_info(ctx.node()));
                }
            };
            def.suffix = [this](Context& ctx){
                //if(ERROR_FLAG) {uspan->log(red("Marked "+Ptr_as_string(ctx.node())+" as error")); mark_error(ctx.node());}
                //uspan->log(green("After: "),node_info(ctx.node()));
                uspan->endline();
            };
            def.stagend = [this](Context& ctx){
                uspan->endline();
            };
            watchers << def;
        }
        

        map<uint32_t, std::string> labels;
        map<std::string,uint32_t> labels_lookup;
        map<uint32_t,_layout> layouts;
        map<std::string, g_ptr<Stage>> stages;
        uint32_t next_id = 0;
        uint16_t uid;

        Node unit_root = deadptr;
        std::string unit_label = "";

        ColColCol types;
        ColCol& operator[](uint16_t index) {return types[index];}

        virtual void init() {
           
        }
        virtual Node process(std::string path) {return deadptr;}
        virtual void run(Node root) {}

        inline Ptr get_ticket(uint32_t type_id, uint32_t size, uint32_t tag) {
            Ptr ticket(&types,type_id,create_column(types[type_id],size,tag,true),0);
            return ticket;
        }

        inline Ptr get_ticket(ColCol* pool, uint32_t size, uint32_t tag) {
            Ptr ticket(pool,create_column(*pool,size,tag,true),0);
            return ticket;
        }

        inline Ptr get_ticket(Ptr storeptr, uint32_t size, uint32_t tag) {
            if(storeptr.cachelevel==0) {
                Ptr ticket(storeptr.unit,storeptr.pool,create_column(resolve_to_pool(storeptr),size,tag,true),0);
                return ticket;
            } else {
                Ptr ticket(storeptr.cache,storeptr.pool,create_column(resolve_to_pool(storeptr),size,tag,true),0);
                return ticket;
            }
        }

        list<Ptr> marked_ptrs;
        map<uint64_t,std::function<void(std::string&)>> ptr_colors;
        inline uint64_t Ptr_to_key(Ptr p) {
            return ((uint64_t)p.pool << 32) | (uint64_t)p.idx;
        }
        inline Ptr key_to_Ptr(uint64_t key) {
            return Ptr{(uint32_t)(key >> 32), (uint32_t)(key & 0xFFFFFFFF), 0};
        }
        uint64_t make_overload_key(uint32_t root, uint32_t right) {
            return ((uint64_t)root << 32) | right;
        }
        inline std::pair<uint32_t,uint32_t> decode_key(uint64_t key) {
            return std::make_pair<uint32_t,uint32_t>((uint32_t)(key >> 32), (uint32_t)(key & 0xFFFFFFFF));
        }

        std::string Ptr_as_string(Ptr p) {
            if(ERROR_FLAG) {
                return red("ERROR_ACTIVE:"+Ptr_to_string(p));
            } else {
                if(p.cachelevel==0) {
                    if(p.unit>=units.length()) {
                        return red("UNIT_OUT_OF_BOUNDS:"+Ptr_to_string(p));
                    }
                } else {
                    if(!p.cache) return red("PTR_CACHE_MISSING");
                }
                if(p.pool>=resolve_to_unit(p).length()) {
                    return red("POOL_OUT_OF_BOUNDS("+std::to_string(resolve_to_unit(p).length())+"):"+Ptr_to_string(p));
                } else if(p.idx>=resolve_to_pool(p).length()) {
                    return red("IDX_OUT_OF_BOUNDS("+std::to_string(resolve_to_pool(p).length())+"):"+Ptr_to_string(p));
                } else { //This keeps popping up on sidx 0 on accident
                    // if(resolve_to_col(p).heterogenous) {
                    //     if(p.sidx>=resolve_to_col(p).size) {
                    //         return red("SIDX_OUT_OF_BOUNDS("+std::to_string(resolve_to_col(p).size)+"):"+Ptr_to_string(p));
                    //     }
                    // } else {
                    //     if(p.sidx>=resolve_to_col(p).length()) {
                    //         return red("SIDX_OUT_OF_BOUNDS("+std::to_string(resolve_to_col(p).length())+"):"+Ptr_to_string(p));
                    //     }
                    // }
                } 
            }
            if(marked_ptrs.has(p)) {
                return red(Ptr_to_string(p));
            }

            //ADD CACHE LEVELS HERE LATER!!!
            #if NAMED_PTRS
                std::string plabel = resolve_to_pool(p).label.empty()?std::to_string(p.pool):resolve_to_pool(p).label.to_std();
                std::string pidx = resolve_to_col(p).label.empty()?std::to_string(p.idx):resolve_to_col(p).label.to_std();
                std::string pstring = std::to_string(p.unit)+"|"+plabel+"|"+pidx+"|"+std::to_string(p.sidx)+"";
                uint64_t key = Ptr_to_key(p);
            
                if(ptr_colors.hasKey(key)) {ptr_colors.get(key)(pstring);}
                return pstring;
            #else
                return Ptr_to_string(p);
            #endif
        }

        void mark_error(Ptr ptr) {marked_ptrs << ptr;}

        void print_layout(_layout& l) {
            for(int i=0;i<l.offsets.length();i++) {
                print(i,": ",labels[i],": ",l.offsets[i],", ",labels[l.tags[i]],", ",labels[l.subtags[i]],"[",l.sizes[i],"]");
            }
            // for(auto e : l.overload.entrySet()) {
            //     auto keyl = decode_key(e.key);
            //     print(labels[keyl.first]," ",labels[keyl.second],"(",keyl.second,"): ",labels[e.value.type]);
            // }
        }


        uint32_t reg_id(const std::string& label) {
            //print("Registering: ",label," LEN: ",types[handler_type_id].length()," GLOBAL LEN: ",global[handler_type_id].length());
            uint32_t at = types[handler_type_id].length();
            note_value(types[handler_type_id],label,sizeof(Ptr),ptr_id);
            types[handler_type_id][at].push_default();
            labels[at] = label;
            labels_lookup[label] = at;
            //print("Registered: ",label," LEN: ",types[handler_type_id].length()," GLOBAL LEN: ",global[handler_type_id].length());
            return at;
        }

        Ptr add_layout_to_col(uint32_t type) {
            Ptr p(uid,layout_type_id,note_value(types[layout_type_id],std::to_string(type)+" Offsets",4,int_id),0);
            note_value(types[layout_type_id],"Tags",4,int_id);
            note_value(types[layout_type_id],"Sizes",4,int_id);
            note_value(types[layout_type_id],"Labels",sizeof(Ptr),string_id);
            note_value(types[layout_type_id],"Subtags",4,int_id);
            note_value(types[layout_type_id],"Subsizes",4,int_id);
            note_value(types[layout_type_id],"Ptrs",sizeof(Ptr),ptr_id);
            note_value(types[layout_type_id],"Overloads",sizeof(Ptr4),ptr4_id);
            types[handler_type_id][type].set(0,(void*)&p);
            return p;
        }

        Ptr add_template(uint32_t for_type) {
            Ptr p = add_layout_to_col(for_type);
            return p;
        }

        std::string make_wrapper_for_layout(_layout& l, const std::string& name) {
            std::string s = "";
            std::string pad = "     ";
            s+="struct "+name+" : Ptr {\n";
            s += pad+name+"() {}\n";
            s += pad+name+"(Ptr p) : Ptr(p) {}\n";
            for(int i=0;i<l.offsets.length();i++) {
                s+="\n";
                std::string type = labels[l.tags[i]];
                std::string label = l.labels[i];
                uint32_t offset = l.offsets[i];
                uint32_t size = l.sizes[i];

                bool is_compound = false; //Not sure what this was meant to do 
                //layouts.hasKey(l.tags[i]);
                bool is_ptr = l.tags[i]==ptr_id||l.tags[i]==string_id;

                if(is_ptr) {
                    s += pad+pad_str("inline "+pad_str("Ptr&",12)+" "+label+"_ptr()",32)+"{return *(Ptr*)resolve_to_col(*this).qget(sidx+"+std::to_string(offset)+"); }\n";
                    s += pad+pad_str("inline "+pad_str("Col&",12)+" "+label+"_col()",32)+"{return resolve_to_col("+label+"_ptr());}\n";
                    s += pad+pad_str("inline "+pad_str("void",12)+" "+label+"(Ptr p)",32)+"{resolve_to_col(*this).qset(sidx+"+std::to_string(offset)+", (void*)&p, "+std::to_string(size)+"); }\n";
                } else if(is_compound) {
                    s += pad+pad_str("inline "+pad_str(type,12)+" "+label+"()",32)+"{return {pool, idx, sidx+"+std::to_string(offset)+"}; }\n";
                    s += pad+pad_str("inline "+pad_str("void",12)+" "+label+"("+type+" t)",32)+"{resolve_to_col(*this).qset(sidx+"+std::to_string(offset)+", types[t.pool][t.idx].qget(t.sidx), "+std::to_string(size)+"); }\n";
                } else {
                    s += pad+pad_str("inline "+pad_str(type,12)+" "+label+"()",32)+"{return *("+type+"*)resolve_to_col(*this).qget(sidx+"+std::to_string(offset)+"); }\n";
                    s += pad+pad_str("inline "+pad_str("void",12)+" "+label+"("+type+" t)",32)+"{resolve_to_col(*this).qset(sidx+"+std::to_string(offset)+", (void*)&t, "+std::to_string(size)+"); }\n";
                }
            }
            s+="};";
            return s;
        }

        Node make_node(uint32_t type = 0, uint32_t sub_type = 0, std::string name = "", float x = -1.0f, float y = -1.0f, float z = -1.0f,
            Value value = deadptr, Ptr childrenptr = deadptr, Ptr qualsptr = deadptr, Ptr nodetableptr = deadptr, 
            Ptr valuetableptr = deadptr, Ptr scopesptr = deadptr, Ptr parent = deadptr, Ptr owner = deadptr, 
            Ptr in_scope = deadptr, std::string opt_str = "", bool mute = false, bool resolved = false) 
        {
            Node n;
            n.pool = node_type_id;
            n.idx = push_column(types[node_type_id], node_total_size, node_id);
            n.sidx = 0;
            n.cache = &types;
            n.cachelevel = 3;
            Col& col = types[node_type_id][n.idx];
            col.heterogenous = true;
    
            col.qset(node_type_offset,(void*)&type,4);
            col.qset(node_sub_type_offset,(void*)&sub_type,4);
    
            Ptr nameptr = get_ticket(name_store_id,sizeof(char),char_id);
            col.qset(node_name_offset, (void*)&nameptr,sizeof(Ptr));
            for(auto c : name) types[nameptr.pool][nameptr.idx].push((void*)&c);
    
            col.qset(x_offset, (void*)&x,4);
            col.qset(y_offset, (void*)&y,4);
            col.qset(z_offset, (void*)&z,4);
    
            col.qset(node_value_offset, (void*)&value,sizeof(Ptr));
        
            if(!is_live(childrenptr)) {childrenptr = get_ticket(children_store_id,sizeof(Ptr),ptr_id);}
            col.qset(node_children_offset, (void*)&childrenptr,sizeof(Ptr));

        
            if(!is_live(qualsptr)) qualsptr = get_ticket(quals_store_id,sizeof(Ptr),ptr_id);
            col.qset(node_quals_offset, (void*)&qualsptr,sizeof(Ptr));
            
            if(!is_live(nodetableptr)) nodetableptr = get_ticket(node_table_store_id,sizeof(Ptr),ptr_id);
            col.qset(node_node_table_offset, (void*)&nodetableptr,sizeof(Ptr));
        
            if(!is_live(valuetableptr)) valuetableptr = get_ticket(value_table_store_id,sizeof(Ptr),ptr_id);
            col.qset(node_value_table_offset, (void*)&valuetableptr,sizeof(Ptr));
    
            if(!is_live(scopesptr)) scopesptr = get_ticket(scopes_store_id,sizeof(Ptr),ptr_id);
            col.qset(node_scopes_offset, (void*)&scopesptr,sizeof(Ptr));
            
            col.qset(parent_offset, (void*)&parent,sizeof(Ptr));
            col.qset(owner_offset, (void*)&owner,sizeof(Ptr));
            col.qset(in_scope_offset, (void*)&in_scope,sizeof(Ptr));
    
            Ptr optstrptr = get_ticket(opt_str_store_id,sizeof(char),char_id);
            col.qset(node_opt_str_offset, (void*)&optstrptr,sizeof(Ptr));
            for(auto c : opt_str) types[optstrptr.pool][optstrptr.idx].push((void*)&c);
    
            col.qset(mute_offset, (void*)&mute,1);
            col.qset(resolved_offset, (void*)&resolved,1);
    
            return n;
        }
    
        Node make_node(uint32_t type, std::string name, Value value, Ptr in_scope) {
            return make_node(type,0,name,-1.0f,-1.0f,-1.0f,value,deadptr,deadptr,deadptr,deadptr,deadptr,deadptr,deadptr,in_scope);
        }
    
        void recycle_column(Ptr p) {
            Acorn::recycle_column(resolve_to_pool(p), p.idx);
        }
        
        void recycle_value(Value v, bool recycle_data = true) {
            if(is_live(v)&&resolve_to_col(v).live) {
                for(int i=0;i<v.quals().length();i++) {
                    recycle_node(v.quals()[i]);
                }
                recycle_column(v.quals_ptr());
    
                for(int i=0;i<v.sub_values().length();i++) {
                    recycle_value(v.sub_values()[i]);
                }
                recycle_column(v.sub_values_ptr());
                
                if(recycle_data) {
                    recycle_column(v.data_ptr());
                }
            }
        }
    
        //Recycles everything
        void recycle_node(Node n) {
            //print("Recycling: ",node_info(n));
            if(is_live(n)&&resolve_to_col(n).live) {
                //print("CHILDREN");
                //print("CHILDREN LEN: ",types[6].length());
                for(int i=0;i<n.children().length();i++) {
                    recycle_node(n.children()[i]);
                }
                //print("CHILDREN LEN: ",types[6].length());
                //print("CHILDREN PTR: ",Ptr_to_string(n.children_ptr()));
                recycle_column(n.children_ptr());
                //print("SCOPES");
                for(int i=0;i<n.scopes().length();i++) {
                    recycle_node(n.scopes()[i]);
                }
                //print("SCOPES PTR: ",Ptr_to_string(n.scopes_ptr()));
                recycle_column(n.scopes_ptr());
                //print("QUALS");
                for(int i=0;i<n.quals().length();i++) {
                    recycle_node(n.quals()[i]);
                }
                //print("QUALS PTR: ",Ptr_to_string(n.quals_ptr()));
                recycle_column(n.quals_ptr());
                //print("NAME PTR ",Ptr_to_string(n.name_ptr()));
                recycle_column(n.name_ptr());
                //print("VALUE ",Ptr_to_string(n.value()));
                recycle_value(n.value());
                //print("VALUE TABLE PTR ",Ptr_to_string(n.value_table_ptr()));
                recycle_column(n.node_table_ptr());
                //print("NODE TABLE PTR ",Ptr_to_string(n.node_table_ptr()));
                recycle_column(n.value_table_ptr());
                //print("OPT STR PTR ",Ptr_to_string(n.opt_str_ptr()));
                recycle_column(n.opt_str_ptr());
                recycle_column(n);
            }
        }
    
        //Doesn't recycle the value or children or scopes or quals
        void soft_recycle_node(Node n) {
            recycle_column(n.name_ptr());
            recycle_column(n.children_ptr());
            recycle_column(n.quals_ptr());
            recycle_column(n.node_table_ptr());
            recycle_column(n.value_table_ptr());
            recycle_column(n.scopes_ptr());
            recycle_column(n.opt_str_ptr());
            recycle_column(n);
        }
    
        Value make_value(uint32_t type = 0, uint32_t size = 0, uint32_t address = 0, uint32_t sub_type = 0, 
            uint32_t sub_size = 0, Ptr type_scope = deadptr, Ptr data = deadptr, Ptr quals = deadptr, 
            Ptr sub_values = deadptr, Ptr store = deadptr, int reg = -1, int loc = -1) 
        {
            Value v;
            v.pool = value_type_id;
            v.idx = push_column(types[value_type_id], value_total_size, value_id);
            v.sidx = 0;
            v.unit = uid;
            Col& col = types[value_type_id][v.idx];
            col.heterogenous = true;
        
            col.qset(value_type_offset, (void*)&type, 4);
            col.qset(value_sub_type_offset, (void*)&sub_type, 4);
            col.qset(size_offset, (void*)&size, 4);
            col.qset(sub_size_offset, (void*)&sub_size, 4);
            col.qset(address_offset, (void*)&address, 4);
            col.qset(reg_offset, (void*)&reg, 4);
            col.qset(loc_offset, (void*)&loc, 4);
        
            col.qset(value_data_offset, (void*)&data, sizeof(Ptr));
            col.qset(type_scope_offset, (void*)&type_scope, sizeof(Ptr));
            col.qset(store_offset, (void*)&store, sizeof(Ptr));
        
            if(!is_live(quals)) quals = get_ticket(quals_store_id, sizeof(Ptr), ptr_id);
            col.qset(value_quals_offset, (void*)&quals, sizeof(Ptr));
        
            if(!is_live(sub_values)) {sub_values = get_ticket(sub_value_store_id, sizeof(Ptr), ptr_id);}
            col.qset(value_sub_values_offset, (void*)&sub_values, sizeof(Ptr));
        
            return v;
        }
        Context make_context(Ptr result = deadptr, Ptr source = deadptr, uint32_t pass = 0, Context parent = deadptr) {
            Context c;
            c.pool = context_type_id;
            c.idx = push_column(types[context_type_id], context_total_size, context_id);
            c.sidx = 0;
            c.unit = uid;
            Col& col = types[context_type_id][c.idx];
            col.heterogenous = true;
        
            Ptr dead_node = deadptr;
            Ptr dead_value = deadptr;
        
            col.qset(context_node_offset,   (void*)&dead_node,  sizeof(Ptr));
            col.qset(context_qual_offset,   (void*)&dead_node,  sizeof(Ptr));
            col.qset(context_left_offset,   (void*)&dead_node,  sizeof(Ptr));
            col.qset(context_out_offset,    (void*)&dead_node,  sizeof(Ptr));
            col.qset(context_root_offset,   (void*)&dead_node,  sizeof(Ptr));
            col.qset(context_value_offset,  (void*)&dead_value, sizeof(Ptr));
            col.qset(context_sub_offset,    (void*)&dead_node,  sizeof(Ptr));
            col.qset(context_parent_offset, (void*)&parent,  sizeof(Ptr));
        
            if(!is_live(result)) result = get_ticket(children_store_id, sizeof(Ptr), ptr_id);
            col.qset(context_result_offset, (void*)&result, sizeof(Ptr));
            
            if(!is_live(source)) source = get_ticket(name_store_id, sizeof(char), char_id);
            col.qset(context_source_offset, (void*)&source, sizeof(Ptr));
        
            uint32_t zero = 0; bool f = false;
            col.qset(context_index_offset,  (void*)&zero, 4);
            col.qset(context_state_offset,  (void*)&zero, 4);
            col.qset(context_pass_offset,   (void*)&pass, 4);
            col.qset(context_flag_offset,   (void*)&f,    1);
        
            return c;
        }
    
        void recycle_context(Context ctx) {
            recycle_column(ctx);
        }
        void deep_recycle_context(Context ctx) {
            // recycle_column(ctx.result_ptr()); //Because this is often somebodies children
            // recycle_column(ctx.source_ptr());
            recycle_column(ctx);
        }

        std::string tag_to_str(uint32_t tag, void* data) {
            DEBUG_ONLY(if(ERROR_FLAG) {return "ERROR";})
            if(tag==int_id) {
                return std::to_string(*(int*)data);
            } else if(tag==float_id) {
                return std::to_string(*(float*)data);
            } else if(tag==bool_id) {
                return (*(bool*)data?"true":"false");
            } else if(tag==char_id) {
                return std::string(1,*(char*)data);
            } else if(tag==string_id) {
                Ptr ptr = *(Ptr*)data;
                if(ptr.pool>=types.length()||ptr.idx>=types[ptr.pool].length()) {
                    return red("STRING ERROR "+std::to_string(ptr.pool)+"|"+std::to_string(ptr.idx)+"|"+std::to_string(ptr.sidx));
                }
                std::string content = string(ptr).to_std();
                return Ptr_as_string(ptr)+"> \""+escape_string(content)+"\"";
            } else if(tag==ptr_id) {
                return Ptr_to_string(*(Ptr*)data);
            } else if(tag==ptr_id||tag==node_id||tag==value_id||tag==context_id||tag==function_id) {
                return Ptr_as_string(*(Ptr*)data);
            } else if(tag==ptr4_id) {
                Ptr4 p = *(Ptr4*)data;
                std::string s = labels[p.midx]+"> "+Ptr_as_string(p.ptr);
                return s;
            } else if(tag==list_id||tag==col_id) {
                Ptr ptr = *(Ptr*)data;
                std::string s = Ptr_as_string(ptr)+"> [";
                Col& col = resolve_to_col(ptr);
                for(int i=0;i<col.length();i++) {
                    s+=tag_to_str(col.tag,col[i]);
                    if(i<col.length()-1) {s+=", ";}
                }
                s+="]";
                return s;
            } else if(tag==map_id) {
                Ptr ptr = *(Ptr*)data;
                std::string s = Ptr_as_string(ptr)+"> [";
                Col& col = resolve_to_col(ptr);
                // for(auto e : col.cells.entrySet()) {
                //     s+="{"+e.key+": "+tag_to_str(col.tag,col[e.value])+"}";
                // }
                s+="]";
                return s;
            } else {
                return "(add tag_to_str for "+labels[tag]+")";
            }
        }
        
        std::string disassemble(uint32_t instr) {
            if(instr==0) return "NULL";
            if(instr==0xD65F03C0) return "ret";

            uint32_t op9 = (instr >> 23) & 0b111111111;
            uint32_t op7 = (instr >> 24) & 0b11111110;

            if(op9 == 0b010100101) { // MOVZ sf=0
                int rd    = instr & 0b11111;
                int imm16 = (instr >> 5) & 0b1111111111111111;
                return "movz " + std::to_string(rd) + " " + std::to_string(imm16) + " 0";
            }
            if(op9 == 0b110100101) { // MOVZ sf=1
                int rd    = instr & 0b11111;
                int imm16 = (instr >> 5) & 0b1111111111111111;
                return "movz " + std::to_string(rd) + " " + std::to_string(imm16) + " 1";
            }


            if((instr & 0b01111111100000000000000000000000) == 0b01110010100000000000000000000000) {
                int rd    =  instr & 0b11111;
                int imm16 = (instr >> 5) & 0b1111111111111111;
                int hw    = (instr >> 21) & 0b11;
                int sf    = (instr >> 31) & 1;
                return "movk " + std::to_string(rd) + " " + std::to_string(imm16) + " " + std::to_string(hw*16) + " " + std::to_string(sf);
            }

            if((instr & 0b01111111001000000000001111100000) == 0b00101010000000000000001111100000) {
                int rd = instr & 0b11111;
                int rm = (instr >> 16) & 0b11111;
                int sf = (instr >> 31) & 1;
                return "mov " + std::to_string(rd) + " " + std::to_string(rm) + " " + std::to_string(sf);
            }

            if((instr & 0b01111111111000000000000000000000) == 0b00001011000000000000000000000000) {
                int rd = instr & 0b11111;
                int rn = (instr >> 5) & 0b11111;
                int rm = (instr >> 16) & 0b11111;
                int sf = (instr >> 31) & 1;
                return "add " + std::to_string(rd) + " " + std::to_string(rn) + " " + std::to_string(rm) + " " + std::to_string(sf);
            }

            if((instr & 0b01111111111000000000000000000000) == 0b00011011000000000000000000000000) {
                int rd = instr & 0b11111;
                int rn = (instr >> 5) & 0b11111;
                int rm = (instr >> 16) & 0b11111;
                int sf = (instr >> 31) & 1;
                return "mul " + std::to_string(rd) + " " + std::to_string(rn) + " " + std::to_string(rm) + " " + std::to_string(sf);
            }

            if((instr & 0b11111111110000000000000000000000) == 0b10111001010000000000000000000000) {
                int rt     = instr & 0b11111;
                int rn     = (instr >> 5) & 0b11111;
                int offset = ((instr >> 10) & 0b111111111111) * 4;
                return "ldr32 " + std::to_string(rt) + " " + std::to_string(rn) + " " + std::to_string(offset);
            }

            if((instr & 0b11111111110000000000000000000000) == 0b11111001010000000000000000000000) {
                int rt     = instr & 0b11111;
                int rn     = (instr >> 5) & 0b11111;
                int offset = ((instr >> 10) & 0b111111111111) * 8;
                return "ldr " + std::to_string(rt) + " " + std::to_string(rn) + " " + std::to_string(offset);
            }

            if((instr & 0b11111111110000000000000000000000) == 0b10111001000000000000000000000000) {
                int rt     = instr & 0b11111;
                int rn     = (instr >> 5) & 0b11111;
                int offset = ((instr >> 10) & 0b111111111111) * 4;
                return "str32 " + std::to_string(rt) + " " + std::to_string(rn) + " " + std::to_string(offset);
            }

            if((instr & 0b11111111110000000000000000000000) == 0b11111001000000000000000000000000) {
                int rt     = instr & 0b11111;
                int rn     = (instr >> 5) & 0b11111;
                int offset = ((instr >> 10) & 0b111111111111) * 4;
                return "str " + std::to_string(rt) + " " + std::to_string(rn) + " " + std::to_string(offset);
            }



            return "   ?   ";
        }

        std::string print_columnar_table(list<list<std::string>> lines) {
            //print("Printing columar with ",lines.length()," lines ");
            list<uint32_t> widths;
            uint32_t longest_row = 0;
            for(int l=0;l<lines.length();l++) {
                list<std::string>& line = lines[l];
                uint32_t widest_row = 0;
                for(int i=0;i<line.length();i++) {
                    if(i==0&&line[i].empty()) {line[i] = std::to_string(l);}
                    if(line[i].length()>widest_row) {widest_row = line[i].length();}
                }
                widths << widest_row;
                if(line.length()>longest_row) {longest_row = line.length();}
            }

            std::string to_return = "";
            int lpadlen = digit_count(longest_row)+1;
            for(int r=0;r<longest_row;r++) {
                if(r==1) {
                    for(int l=0;l<lines.length();l++) {
                        to_return+=std::string(widths[l]+lpadlen+3,'-')+"<|>";
                    }
                    to_return+="\n";
                }

                for(int l=0;l<lines.length();l++) {
                    std::string line = "";
                    //print("On line ",l," row ",r);
                    if(lines[l].length()>r) {line = lines[l][r];}
                    //print("Line: ",line);
                    std::string rownum = std::to_string(r-1); //Minus 1 because indexes start at 0                    
                    if(r==0) { //If a header
                        std::string column = std::to_string(l);
                        if(line==column) {
                            to_return += center_pad(line, widths[l]+lpadlen+3) + " | ";
                        } else {
                            to_return+=std::string(lpadlen-column.length(),' ')+column+" : ";
                            to_return += center_pad(line, widths[l]) + " | ";
                        }
                    } else {
                        if(!line.empty()) {
                            to_return+=std::string(lpadlen-rownum.length(),' ')+rownum+" : ";
                            std::string padding(widths[l]-line.length(),' ');
                            to_return += line+padding+" | ";
                        } else {
                            to_return += center_pad("X",widths[l]+lpadlen+3)+" | ";
                        }
                    }

                }
                to_return+="\n";

                if(longest_row>1&&r==longest_row-1) {
                    for(int l=0;l<lines.length();l++) {
                        to_return+=std::string(widths[l]+lpadlen+3,'=')+"/ \\";
                    }
                }
            }
            return to_return;
        }

        list<list<std::string>> type_to_lines(ColCol& t) {
            list<list<std::string>> lines;
            list<uint32_t> dtypes;
            for(int c=0;c<t.length();c++) {
                Col& col = t[c];
                list<std::string> subline;
                subline << col.label.to_std()+(col.live?"":" [FREE]");
                //print("Pushed label ",subline[0]);
                if(col.heterogenous) {
                    if(layouts.hasKey(col.tag)) {
                        _layout& l = layouts.get(col.tag);
                        for(int o=0;o<l.offsets.length();o++) {
                            std::string line = "";
                            line+=pad_str(l.labels[o]+": ",12);
                            line+=tag_to_str(l.tags[o],col.qget(l.offsets[o]));
                            subline << line;
                        }
                    } else {
                        print(red("core::type_to_string unable to print heteregenous column of type "+labels[col.tag]+" because no layout was found"));
                    }
                } else {
                    for(int r=0;r<col.length();r++) {
                        std::string line = "";
                        //print("Line ",lines.length()," Subline ",subline.length());
                        //print("Row ",r," Column ",c," Tag ",labels[col.tag],"(",col.tag,")");
                        if(col.cells.length()>r) {
                            if(col.cells[r].tag==string_id) {
                                line += "["+((QString&)col.cells[r]).to_std()+"] ";
                            } else {
                                line += "["+labels[col.cells[r].tag]+"?] ";
                            }
                        }
                        line += tag_to_str(col.tag,col[r]);
                        //print("Result: ",line);
                        subline << line;
                    }
                }
                //print("Pushed ",subline.length()," sublines");
                lines << subline;
            }
            //print("Returned ",lines.length()," lines");
            return lines;
        }

        std::string type_to_string(ColCol& t) {
            return print_columnar_table(type_to_lines(t));
        }

        list<list<std::string>> TypeCol_to_lines(ColCol& t) {
            list<list<std::string>> lines;
            for(int c=0;c<t.length();c++) {
                Col& col = t[c];
                list<std::string> subline;
                subline << col.label.to_std();
                if(col.heterogenous) {
                    if(layouts.hasKey(col.tag)) {
                        _layout& l = layouts.get(col.tag);
                        for(int o=0;o<l.offsets.length();o++) {
                            std::string line = "";
                            line+=pad_str(l.labels[o]+": ",12);
                            line+=tag_to_str(l.tags[o],col.qget(l.offsets[o]));
                            subline << line;
                        }
                    } else {
                        print(red("core::TypeCol_to_lines unable to print heteregenous column of type "+labels[col.tag]+" because no layout was found"));
                    }
                } else {
                    for(int r=0;r<col.length();r++) {
                        std::string line = "";
                        if(col.cells.length()>r) {
                            if(col.cells[r].tag==string_id) {
                                line += "["+((QString&)col.cells[r]).to_std()+"] ";
                            } else {
                                line += "["+labels[col.cells[r].tag]+"?] ";
                            }
                        }
                        line += tag_to_str(col.tag,col[r]);
                        subline << line;
                    }
                }
                lines << subline;
            }
            return lines;
        }

        void print_column(Col& col) {
            print("COL: ",col.label," TAG: ",labels[col.tag]," [",std::to_string(col.length()),"]");
            if(col.heterogenous) {
                if(layouts.hasKey(col.tag)) {
                    _layout& l = layouts.get(col.tag);
                    for(int o=0;o<l.offsets.length();o++) {
                        std::string line = "";
                        line+=pad_str(l.labels[o]+": ",12);
                        line+=tag_to_str(l.tags[o],col.qget(l.offsets[o]));
                        print(line);
                    }
                } else {
                    print(red("core::print_column unable to print heteregenous column of type "+labels[col.tag]+" because no layout was found"));
                }
            } else {
                for(int i=0;i<col.length();i++) {
                    print(i,": ",tag_to_str(col.tag,col[i]));
                }
            }
        }

        void dump_pool(ColCol& pool, uint32_t index, bool clear_dump) {
            if(clear_dump) writeFile("mixos-acorn/tests/printout.txt","");
            std::string to_print = "";
            to_print += "TYPE "+std::to_string(index)+" "+pool.label.to_std()+(pool.tag!=0?" ["+labels[pool.tag]+"]":"")+":\n";
            to_print += type_to_string(pool);
            to_print += "\n\n\n";
            editTextFile("mixos-acorn/tests/printout.txt",[to_print](std::string& source){
                source+=to_print;
            });
        }

        void dump_unit(bool clear_dump) {
            if(clear_dump) writeFile("mixos-acorn/tests/printout.txt","");

            for(int t=0;t<types.length();t++) {
                std::string to_print = "";
                to_print += "TYPE "+std::to_string(t)+" "+types[t].label.to_std()+":\n";
                //print("PRINTING: ",t);
                to_print += type_to_string(types[t]);
                to_print += "\n\n\n";
                // print("COMMITING: ",t);
                // print("TEXT: ",to_print);
                editTextFile("mixos-acorn/tests/printout.txt",[to_print](std::string& source){
                    source+=to_print;
                });
            }
        }

        std::string value_info(Value value) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(value)),red(" while another error was active")); return "";})

            std::string to_return = "";
            to_return += cyan("["+Ptr_as_string(value)+"]")+
            + "(type: " + green(labels[value.type()])
            + (value.reg()!=-1?", reg: "+std::to_string(value.reg()):"");
            if(is_live(value.data_ptr())) { //For post-mortems we want to see the adress, so it needs to be computed first, before the error
                std::string ptr_addr = Ptr_as_string(value.data_ptr());
                if(resolve_to_col(value.data_ptr()).empty()) {
                    to_return += ", value: "+gray("empty")+" @"+ptr_addr;
                } else {
                    std::string tagstr = tag_to_str(value.type(),value.get());
                    if(tagstr.length()>50) tagstr = tagstr.substr(0,50); //Disable this to disable truncation of large values
                    to_return += ", value: "+gray(tagstr)+" @"+ptr_addr;
                }
                DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_to_string(value)),red(" but the value was invalid")); return to_return;})
            }
            to_return += (value.sub_type()!=0?", sub_type: "+labels[value.sub_type()]:"")
            + (is_live(value.type_scope())?", type_scope: "+blue(Ptr_as_string(value.type_scope())):"")
            + (value.size()!=0?", size: "+std::to_string(value.size()):"")
            + (value.sub_size()!=0?", sub_size: "+std::to_string(value.sub_size()):"")
            + (value.address()!=0?", address: "+std::to_string(value.address()):"")
            + (value.loc()!=-1?", loc: "+std::to_string(value.loc()):"")
            + (is_live(value.store_ptr())?", store: "+Ptr_as_string(value.store_ptr()):"")
            + (!value.sub_values().empty()?", sub: "+std::to_string(value.sub_values().length()):"");
            if(!value.quals().empty()) {
                to_return += ", Quals: ";
                for(int i=0;i<value.quals().length();i++) {
                    to_return += labels[value.quals()[i].type()]+(i!=value.quals().length()-1?", ":"");
                }
            }
            to_return += ")";
            return to_return;
        }

        std::string node_basic_info(Node node) {
            std::string to_return = labels[node.type()]+ (node.name().length()==0?"":" "+node.name().to_std()+" ");
            return to_return;
        }
        
        std::string node_info(Node node) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(node)),red(" while another error was active")); return "";})

            std::string to_return = "";
            to_return += blue(Ptr_as_string(node)+" ")
            + labels[node.type()]
            + (node.sub_type()==0?"":":"+labels[node.sub_type()])
            + (node.name().length()==0?"":" "+green(escape_string(node.name().to_std()))+" ") 
            + (is_live(node.value())?value_info(node.value()):"")
            + (node.x()!=-1.0f?"("+std::to_string((int)node.x())+","+std::to_string((int)node.y())+")":"")
            + (!node.children().empty()?"[C:"+std::to_string(node.children().length())+"]":"")
            + (!node.scopes().empty()?"[S:"+std::to_string(node.scopes().length())+"]":"")
            + (is_live(node.owner())?"[O:"+blue(Ptr_as_string(node.owner()))+"]":"")
            + (is_live(node.in_scope())?"{"+node.in_scope().name().to_std()+"}":"");
            if(!node.quals().empty()) {
                to_return += "[Q: ";
                for(int i=0;i<node.quals().length();i++) {
                    if(node.quals()[i].mute()) {
                        to_return += italic_str(Ptr_as_string(node.quals()[i])+">"+labels[node.quals()[i].type()]);
                    } else {
                        to_return += Ptr_as_string(node.quals()[i])+">"+labels[node.quals()[i].type()];
                    }
                    to_return+=(i!=node.quals().length()-1?", ":"");
                }
                to_return += "]";
            }
            return to_return;
        }

        #define ACORN_MUTE_TABLES 1

        std::string node_to_string(Node node, int depth = 0, int index = 0, bool print_sub_scopes = false, std::string sigil = "") {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(node)),red(" while another error was active")); return "";})
            std::string indent(depth * 2, ' ');
            std::string to_return = "";
            
            to_return += indent + sigil + std::to_string(index) + ": " + node_info(node);
        
            #if !ACORN_MUTE_TABLES 
            if(node.value_table().length()>0) {
                to_return += "\n" + indent + "   Value table:";
                QCellCol cells = node.value_table().col().cells;
                for(int i=0;i<cells.length();i++) {
                    to_return += "\n" + indent + "     Key: "+((QString&)cells[i]).to_std()+" | "+value_info(node.value_table().get(cells.get(i).index));
                }
            }
            if(node.node_table().length()>0) {
                to_return += "\n" + indent + "   Node table:";
                QCellCol cells = node.node_table().col().cells;
                for(int i=0;i<cells.length();i++) {
                    to_return += "\n" + indent + "     Key: "+((QString&)cells[i]).to_std()+" | "+node_info(node.node_table().get(cells.get(i).index));
                }
            }
            #endif
        


            // if(!node->opt_str.empty()) {
            //     to_return +=  "\n" + indent + "  Opt_str: " + node->opt_str;
            // }

            if(!node.children().empty()) {
                for(int i=0;i<node.children().length();i++) {
                    if(is_live(node.children()[i])) {
                        if(node.children()[i].idx==node.idx) {
                            to_return+="\n "+indent+red("  self refrence");
                        } else {
                            to_return += "\n " + node_to_string(node.children()[i], depth + 1, i, print_sub_scopes,"c");
                        }
                    }
                    else {
                        to_return += "\n" + indent + "[NULL CHILD] "+node_info(node.children()[i]);
                    }
                }
            }

            if(!node.scopes().empty()) {
                //to_return +=  "\n" + indent + "   Scopes: " + std::to_string(node.scopes().length());
                int i = 0;
                for(int s=0;s<node.scopes().length();s++) {
                    Node scope = node.scopes()[s];
                    if(scope.owner().idx==node.idx) {
                        to_return += "\n " + node_to_string(scope, depth + 1, s, print_sub_scopes,"s");
                    }
                    else {
                        to_return += "\n"+indent+"  s"+std::to_string(s)+": "+node_info(scope);
                    }
                }
            }
        
            return to_return;
        }

        Node scan_for_node(const std::string& label, Node from) {
            for(int i=0;i<from.children().length();i++) {
                //print("CHECKING: ",from.children()[i].name().to_std());
                if(from.children()[i].name().to_std()==label) {
                    return from.children()[i];
                }
                Node found = scan_for_node(label,from.children()[i]);
                if(is_live(found)) {
                    return found;
                }
            }
            for(int i=0;i<from.scopes().length();i++) {
                Node found = scan_for_node(label,from.scopes()[i]);
                if(is_live(found)) {
                    return found;
                }
            }
            return deadptr;
        }

        void test_acorn() {
            Node n = make_node();
            Node m = make_node();

            print("UNITS: ",units.length());
            print("TYPES: ",types.length());

            n.name("NODE N"); m.name("NODE M");
            print("NODES WITH 2 NODES: ",types[node_type_id].length());
            print("NAMES WITH 2 NODES: ",types[name_store_id].length());
            print("CHILDREN WITH 2 NODES: ",types[children_store_id].length());
            recycle_node(n);
            print("NODES AFTER RECYCLE: ",types[node_type_id].length());
            print("NAMES AFTER RECYCLE: ",types[name_store_id].length());
            print("CHILDREN AFTER RECYCLE: ",types[children_store_id].length());
            Node c = make_node();
            print("NODES WITH 2 NODES: ",types[node_type_id].length());
            print("NAMES WITH 2 NODES: ",types[name_store_id].length());
            print("CHILDREN WITH 2 NODES: ",types[children_store_id].length());
            c.name("C");
            print(c.name().to_std());
        }

      

        Node copy_as_token(Node node) {
            Node copy = make_node(node.type(),0,node.name().to_std(),node.x(),node.y(),node.z());
            copy.mute(true);
            for(int i=0;i<node.quals().length();i++) {
                Node q = node.quals()[i];
                if(q.mute()) {copy.quals() << q;}
            }
            return copy;
        }

        Node turn_into_token(Node node) {
            node.mute(true);
            return node;
        }

        void walk_nodenet(Node root, std::function<void(Node)> func) {
            func(root);
            for(int i=0;i<root.children().length();i++) walk_nodenet(root.children()[i],func);
            for(int i=0;i<root.quals().length();i++) walk_nodenet(root.quals()[i],func);
            for(int i=0;i<root.scopes().length();i++) if(root.scopes()[i].owner()==root) walk_nodenet(root.scopes()[i],func);
            if(is_live(root.value())) {
                for(int i=0;i<root.value().quals().length();i++) walk_nodenet(root.value().quals()[i],func);
            }
        }
    
        Stage& reg_stage(std::string label) {
            g_ptr<Stage> new_stage = make<Stage>();
            new_stage->label = label;
            stages.put(label,new_stage);
            types[handler_type_id][stages_id].put(label,(void*)&deadptr);
            return *new_stage.getPtr();
        }

        map<uint32_t,Handler> value_printers; 

        std::string value_as_string(Value v) {
            Context ctx = make_context(); ctx.value(v);
            if(value_printers.hasKey(v.type())) {
                value_printers[v.type()](ctx);
            } else {
                return "(add value printer for "+labels[v.type()]+")";
            }
            std::string str = ctx.source().to_std();
            deep_recycle_context(ctx);
            return str;
        }

        std::string value_as_string(Ptr dataptr) {
            if(!is_live(dataptr)) return "";
            Value v = make_value(resolve_to_col(dataptr).tag,0,0,0,0,deadptr,dataptr);
            std::string to_return = value_as_string(v);
            recycle_value(v,false);
            return to_return;

        }
    

        std::string GLOBAL_MSG = "";
        void log_to_watcher(Context& ctx, const std::string& msg) {
            GLOBAL_MSG = msg;
            for(auto& w : watchers) {if(w.logger) w.logger(ctx);}
            GLOBAL_MSG = "";
        }
    
        void start_logged_stage(Stage& stage) {
            active_stage = &stage;
            DEBUG_ONLY(for(auto& w : watchers) {if(w.stagestart) w.stagestart((Context&)dead_ref);})
        }
        void end_logged_stage() {
            DEBUG_ONLY(for(auto& w : watchers) {if(w.stagend) w.stagend((Context&)dead_ref);})
        }

        void start_stage(Stage& stage) {
            active_stage = &stage;
        }
    
        void start_stage(g_ptr<Stage> stage_ptr) {
            start_stage(*stage_ptr.getPtr());
        }

        void start_stage(Stage* stage_ptr) {
            start_stage(*stage_ptr);
        }

        inline void invoke_in(Stage* stage, Context& ctx) {
            Stage* old_stage = active_stage;
            active_stage = stage;
            standard_process(ctx);
            active_stage = old_stage;
        }
        inline void invoke_in(Stage& stage, Context& ctx) {
            invoke_in(&stage,ctx);
        }

        uint32_t standard_travel_pass(Node root, Context sub = deadptr);
        uint32_t resume_travel_pass(Context ctx);

        void suspend() {running = false;}
        void resume() {running = true;}

        inline void standard_process(Context& ctx, uint32_t type) {
            DEBUG_ONLY(for(auto& w : watchers) {if(w.prefix) w.prefix(ctx);})
            while(!running) {
                std::this_thread::sleep_for(std::chrono::nanoseconds(100));
            }
            active_stage->run(type)(ctx);
            DEBUG_ONLY(for(auto& w : watchers) {if(w.suffix) w.suffix(ctx);})
        }

        inline void standard_process(Context& ctx) {
            standard_process(ctx,ctx.node().type());
        }
    
        void process_node(Context& ctx, Node node) {
            Node saved_node = ctx.node();
            Node saved_qual = ctx.qual();
            Context saved_sub = ctx.sub();
            ctx.node(node);
            standard_process(ctx);
            ctx.node(saved_node);
            ctx.sub(saved_sub);
            ctx.qual(saved_qual);
        }

        //Sets the root as the previous ctx.node
        void sub_process_node(Context& ctx, Node node) {
            Node saved_node = ctx.node();
            Node saved_qual = ctx.qual();
            Context saved_sub = ctx.sub();
            Node saved_root = ctx.root();
            ctx.root(ctx.node());
            ctx.node(node);
            standard_process(ctx);
            ctx.node(saved_node);
            ctx.sub(saved_sub);
            ctx.root(saved_root);
            ctx.qual(saved_qual);
        }
    
        void process_node(Context& ctx, Node node, Node left) {
            Node saved_node = ctx.node();
            Node saved_left = ctx.left();
            Context saved_sub = ctx.sub();
            ctx.node(node);
            ctx.left(left);
            standard_process(ctx);
            ctx.node(saved_node);
            ctx.left(saved_left);
            ctx.sub(saved_sub);
        }
    
        void process_node(Node node, Node left) {
            Context ctx = make_context();
            process_node(ctx,node,left);
            deep_recycle_context(ctx);
        }
    
        void standard_sub_process_node(Node root) {
            Context ctx = make_context();
            ctx.node(root);
            standard_sub_process(ctx);
            deep_recycle_context(ctx);
        }

        void standard_sub_process(Context& ctx) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a sub_process while an error was flagged")); return;})
            node_col children = ctx.node().children();
            Context sub_ctx = make_context(children,ctx.source_ptr());
            sub_ctx.root(ctx.node());
            sub_ctx.sub(ctx.sub());

            int& i = sub_ctx.index();
            while(i < sub_ctx.result().length()) {
                if(i==0) {
                    process_node(sub_ctx, sub_ctx.result().get(i));
                } else {
                    process_node(sub_ctx, sub_ctx.result().get(i), sub_ctx.result().get(i-1));
                }

                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})

                i++;
            }
            ctx.flag(sub_ctx.flag());
            recycle_context(sub_ctx);
        }

        void backwards_sub_process(Context& ctx) { 
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a backwards sub_process while an error was flagged")); return;})
            node_col children = ctx.node().children();
            Context sub_ctx = make_context(children,ctx.source_ptr());
            sub_ctx.root(ctx.node());
            sub_ctx.sub(ctx.sub());
            int& i = sub_ctx.index();
            i = children.length()-1;
            while(i >= 0) {
                if(i==children.length()-1) {
                    process_node(sub_ctx, sub_ctx.result().get(i));
                } else {
                    process_node(sub_ctx, sub_ctx.result().get(i), sub_ctx.result().get(i+1));
                }
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                i--;
            }
            ctx.flag(sub_ctx.flag());
            recycle_context(sub_ctx);
        }

        //resolve_to_col(sub_ctx).qset(context_source_offset,(void*)&ctx.source_ptr(),sizeof(Ptr));

        void fire_quals(Context& ctx, Value value) {
            Value saved_value = ctx.value();
            Node saved_qual = ctx.qual();
            ctx.value(value);
            for(int q=0;q<value.quals().length();q++) {
                Node qual = value.quals()[q];
                if(qual.mute()) continue;
                ctx.qual(qual);
                //if(active_stage->has(qual.type()+1))
                standard_process(ctx,qual.type()+1);
            }
            ctx.value(saved_value);
            ctx.qual(saved_qual);
        }
        void fire_quals(Context& ctx, Node node) {
            Node saved_node = ctx.node();
            Node saved_qual = ctx.qual();
            ctx.node(node);
            for(int q=0;q<node.quals().length();q++) {
                Node qual = node.quals()[q];
                if(qual.mute()) continue;
                ctx.qual(qual);
                standard_process(ctx,qual.type()+2);
            }
            ctx.node(saved_node);
            ctx.qual(saved_qual);
        }

        void standard_qual_process(Context& ctx) {
            for(int n=0;n<2;n++) {
                Context sub_ctx = make_context(n==0?ctx.node().quals():ctx.node().value().quals());
                int& i = sub_ctx.index();
                sub_ctx.root(ctx.node());
                sub_ctx.sub(ctx.sub());
                while(i < sub_ctx.result().length()) {
                    sub_ctx.qual(sub_ctx.result().get(i));
                    if(n==0) {
                        sub_ctx.node(ctx.node());
                    } else {
                        sub_ctx.value(ctx.node().value());
                    }
                    standard_process(sub_ctx,sub_ctx.qual().type());
                    sub_ctx.left(sub_ctx.result().get(i));
                    i++;
                }
                recycle_context(sub_ctx);
            }
        }
    
        void standard_direct_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a direct pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Direct pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children);
            int& i = ctx.index();
            ctx.root(root);
            while(i < ctx.result().length()) {
                ctx.node(ctx.result().get(i));
                standard_process(ctx);
                ctx.left(ctx.result().get(i));
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                i++;
            }
    
            node_col scopes = root.scopes();
            for(int i = 0; i<scopes.length(); i++) {
                standard_direct_pass(scopes.get(i));
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
            }
            endline();
            deep_recycle_context(ctx);
        }

        void standard_resolve_child_scopes(Node node) {
            for(int c = 0; c < node.children().length(); c++) {
                Node child = node.children()[c];
                if(!child.scopes().empty()) {
                    standard_sub_process_node(child);
                    for(int s = 0; s < child.scopes().length(); s++) {
                        if(child.scopes()[s].owner()==child) {
                            standard_resolving_pass(child.scopes()[s]);
                        }
                    }
                }
                standard_resolve_child_scopes(child);
            }
        }

        void standard_resolving_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a resolving pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Resolving pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children);
            int& i = ctx.index();
            ctx.root(root);
            while(i < ctx.result().length()) {    //Process all nodes with scopes first (like any declerations)
                if(!ctx.result()[i].scopes().empty()) {
                    ctx.node(ctx.result()[i]);
                    standard_process(ctx);
                    DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                    ctx.left(ctx.result()[i]);
                }
                i++;
            }
            i = 0;
            while(i < ctx.result().length()) {    //Then process nodes without scopes
                if(ctx.result()[i].scopes().empty()) {
                    ctx.node(ctx.result()[i]);
                    standard_process(ctx);
                    DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                    ctx.left(ctx.result()[i]);
                }
                i++;
            }
            i = 0;
            while(i < ctx.result().length()) {    //Then the children of nodes with scopes
                if(!ctx.result()[i].scopes().empty()) {
                    standard_sub_process_node(ctx.result()[i]);
                }
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                i++;
            }
            i = 0;
            while(i < ctx.result().length()) {    //Then finnaly the subscopes
                standard_resolve_child_scopes(ctx.result()[i]); //Processing closures and such, any child containing it's own scopes
                if(!ctx.result()[i].scopes().empty()) {
                    for(int s = 0;s<ctx.result()[i].scopes().length();s++) {
                        if(ctx.result()[i].scopes()[s].owner()==ctx.result()[i]) {
                            standard_resolving_pass(ctx.result()[i].scopes()[s]);
                            DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                        }
                    }
                }
                i++;
            }
            endline();
            deep_recycle_context(ctx);
        }

        void standard_backwards_child_scopes(Node node) {
            for(int c = 0; c < node.children().length(); c++) {
                Node child = node.children()[c];
                if(!child.scopes().empty()) {
                    standard_sub_process_node(child);
                    for(int s = 0; s < child.scopes().length(); s++) {
                        if(child.scopes()[s].owner()==child) {
                            standard_backwards_pass(child.scopes()[s]);
                        }
                    }
                }
                standard_backwards_child_scopes(child);
            }
        }

        void standard_backwards_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a backwards pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Backwards pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children);
            int& i = ctx.index();
            ctx.root(root);
            i = ctx.result().length()-1;
            while(i >= 0) {
                ctx.node(ctx.result().get(i));
                standard_process(ctx);
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                node_col scopes = ctx.result().get(i).scopes();
                standard_backwards_child_scopes(ctx.result().get(i));
                for(int s = 0; s<scopes.length(); s++) {
                    if(is_live(scopes.get(s).owner())&&scopes.get(s).owner().idx==ctx.result().get(i).idx) {
                        memory_backwards_pass(scopes.get(s));
                        DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                    }
                }
                ctx.left(ctx.result().get(i));
                i--;
            }
            endline();
            deep_recycle_context(ctx);
        }

        void standard_memory_backwards_child_scopes(Node node) {
            for(int c = 0; c < node.children().length(); c++) {
                Node child = node.children()[c];
                if(!child.scopes().empty()) {
                    standard_sub_process_node(child);
                    for(int s = 0; s < child.scopes().length(); s++) {
                        if(child.scopes()[s].owner()==child) {
                            memory_backwards_pass(child.scopes()[s]);
                        }
                    }
                }
                standard_memory_backwards_child_scopes(child);
            }
        }

        void memory_backwards_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a memory backwards pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Backwards pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children);
            int& i = ctx.index();
            ctx.root(root);
            i = ctx.result().length()-1;
            while(i >= 0) {
                ctx.node(ctx.result().get(i));
                standard_memory_backwards_child_scopes(ctx.result().get(i));
                node_col scopes = ctx.result().get(i).scopes();
                for(int s = 0; s<scopes.length(); s++) {
                    if(is_live(scopes.get(s).owner())&&scopes.get(s).owner()==ctx.result().get(i)) {
                        memory_backwards_pass(scopes.get(s));
                        DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                    }
                }
                standard_process(ctx);
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                ctx.left(ctx.result().get(i));
                i--;
            }
            endline();
            deep_recycle_context(ctx);
        }   


        inline void normalize_Col_tags(Col& col, map<uint32_t,uint32_t>& fixup) {
            if(fixup.hasKey(col.tag)) {
                //print("Fixup ",col.tag," as ",fixup[col.tag]);
                col.tag = fixup.get(col.tag);
            }
        }
        inline void normalize_ColCol_tags(ColCol& col, map<uint32_t,uint32_t>& fixup) {
            normalize_Col_tags(col,fixup);
            for(int i=0;i<col.length();i++) {
                normalize_Col_tags(col[i],fixup);
            }
        }
        inline void normalize_ColColCol_tags(ColColCol& col, map<uint32_t,uint32_t>& fixup) {
            normalize_Col_tags(col,fixup);
            for(int i=0;i<col.length();i++) {
                normalize_ColCol_tags(col[i],fixup);
            }
        }

        map<uint32_t,uint32_t> remap_ids_by_label(map<uint32_t,std::string>& ids) {
            map<uint32_t,uint32_t> fixup;
            for(auto e : ids.entrySet()) {
                uint32_t old_id = e.key;
                std::string label = e.value;
                if(labels_lookup.hasKey(label)) {
                    uint32_t new_id = labels_lookup.get(label);
                    if(old_id != new_id) {
                        //print("Registered id fixup from ",old_id,"[",label,"] to ",new_id,"[",labels[new_id],"]");
                        fixup.put(old_id, new_id);
                    }
                } else {
                    //print("Label lookup does not have key ",label);
                }
            }
            return fixup;
        }

        enum norm_ops {
            NORM_IDS = 0
        };
    
        void normalize(std::ifstream& in, list<void*> data, uint8_t level) {
            uint32_t pass_count = read_raw<uint32_t>(in);
            for(uint32_t p=0;p<pass_count;p++) {
                uint32_t block_op = read_raw<uint32_t>(in);
                switch(block_op) {
                    case NORM_IDS: {
                        uint32_t count = read_raw<uint32_t>(in);
                        print("Normalizing ",count," ids");
                        map<uint32_t,std::string> block_ids;
                        for(uint32_t j=0;j<count;j++) {
                            uint32_t id = read_raw<uint32_t>(in);
                            std::string label = read_string(in);
                            block_ids.put(id,label);
                            //print(label,": ",id," [",labels[id],"]");
                        }
                        map<uint32_t,uint32_t> fixup = remap_ids_by_label(block_ids);
                        if(level==0) { //Col
                            for(int i=0;i<data.length();i++) {
                                Col& col = *(Col*)data[i];
                                normalize_Col_tags(col,fixup);
                            }
                        } else if(level==1) { //ColCol
                            for(int i=0;i<data.length();i++) {
                                ColCol& col = *(ColCol*)data[i];
                                normalize_ColCol_tags(col,fixup);
                            }
                        } else if(level==2) { //ColColCol
                            for(int i=0;i<data.length();i++) {
                                ColColCol& col = *(ColColCol*)data[i];
                                normalize_ColColCol_tags(col,fixup);
                            }
                        }
                    }
                    break;

                    default:
                    print(red("core:normalize unrecognized block opperation: "+std::to_string(block_op)));
                    break;
                }
            }
        }

        void write_normalize_trailer(std::ostream& out,list<uint32_t> passes) {
            uint32_t pass_count = passes.length();
            write_raw(out, pass_count);
            for(int p=0;p<pass_count;p++) {
                switch(passes[p]) {
                    case NORM_IDS: {
                        write_raw<uint32_t>(out, NORM_IDS);
                        write_raw<uint32_t>(out, labels.entrySet().length());
                        for(auto e : labels.entrySet()) {
                            write_raw<uint32_t>(out, e.key);
                            write_string(out, e.value);
                        }
                    }
                    break;

                    default:
                    print(red("core:write_normalize unrecognized pass: "+std::to_string(passes[p])));
                    break;
                }
            }
        }


        void save_acorn(const std::string& path) {
            auto out = openWriteStream(path);
            write_TypeTypeCol(out,types);
    
        }
        void load_acorn(const std::string& path) {
            auto in = openReadStream(path);
            types = read_TypeTypeCol(in);
            init();
            ERROR_FLAG = false;
        }
    };

    template<typename T>
    inline g_ptr<T> make_unit() {
        g_ptr<T> u = make<T>();
        return u;
    }

    inline uint16_t make_unit(const ColColCol& starter) {
        g_ptr<Unit> u = make<Unit>(starter);
        return u->uid;
    }

   
    inline ColColCol& resolve_to_unit(const Ptr& ptr) {
        switch(ptr.cachelevel) {
            case 0: {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit]).types;}
            case 3: return *(ColColCol*)ptr.cache;
            default: 
                throw_error("Can not resolve Ptr ",Ptr_to_string(ptr)," to unit because it's cachelevel ",(uint32_t)ptr.cachelevel," is too low");
            return col3_ref;
        }
    }
    // inline void* resolve_ptr(const Ptr& ptr) {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit])[ptr.pool][ptr.idx][ptr.sidx];}
    // inline void* resolve_ptr(const Ptr& ptr, const uint32_t& idx) {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit])[ptr.pool][idx].get(ptr.sidx);}
    // inline Ptr& resolve_to_ptr(const Ptr& ptr) {std::lock_guard<std::mutex> lock(units_mutex); return *(Ptr*)(*units[ptr.unit])[ptr.pool][ptr.idx].get(ptr.sidx);}
    // inline Ptr& resolve_to_ptr(const Ptr& ptr, const uint32_t& idx) {std::lock_guard<std::mutex> lock(units_mutex); return *(Ptr*)(*units[ptr.unit])[ptr.pool][idx].get(ptr.sidx);}
    // inline Col& resolve_to_col(const Ptr& ptr) {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit])[ptr.pool][ptr.idx];}
    // inline Col& resolve_to_col(const Ptr& ptr, const uint32_t& idx) {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit])[ptr.pool][idx];}
    // inline ColCol& resolve_to_pool(const Ptr& ptr) {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit])[ptr.pool];}
    // inline ColColCol& resolve_to_unit(const Ptr& ptr) {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit]).types;}
    // inline Col& to_col(const Ptr& ptr) {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit])[ptr.pool][ptr.idx];}

    inline Ptr get_ticket_from_unit(Ptr p, uint32_t type_id, uint32_t size, uint32_t tag) {
        if(p.cachelevel==3) {
            Ptr ticket(p.cache,type_id,create_column(resolve_to_unit(p)[type_id],size,tag,true),0);
            return ticket;
        } else if(p.cachelevel==0) {
            std::lock_guard<std::mutex> lock(units_mutex);
            return (*units[p.unit]).get_ticket(type_id,size,tag);
        }
        return deadptr;
    }

    std::ostream& operator<<(std::ostream& os, Acorn::string& s) {
        os.write((const char*)s.col().storage, s.length());
        return os;
    }

    //Returns true if flagged for a return/break
    uint32_t Unit::standard_travel_pass(Node root, Context sub) {
        DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a travel pass while an error was flagged")); return true;})
        node_col children = root.children();
        newline("Travel pass over "+std::to_string(children.length())+" nodes");
        Context ctx = make_context(children,is_live(sub)?sub.source_ptr():deadptr,travel_pass,unit_ctx);
        ctx.root(root);
        ctx.sub(sub);
        unit_ctx = ctx;
        DEBUG_ONLY(for(auto& w : watchers) {if(w.passstart) w.passstart(ctx);})
        while(unit_ctx.index() < unit_ctx.result().length()) {
            unit_ctx.node(unit_ctx.result().get(unit_ctx.index()));
            standard_process(unit_ctx);
            Node nleft = unit_ctx.result().get(unit_ctx.index());
            unit_ctx.left(nleft);
            DEBUG_ONLY(if(ERROR_FLAG) {endline(); return true;})
            if(unit_ctx.state()>0) { //This is the return/break process
                DEBUG_ONLY(for(auto& w : watchers) {if(w.suffix) w.suffix(ctx);})
                endline();
                uint32_t state = unit_ctx.state();
                unit_ctx = unit_ctx.parent();
                deep_recycle_context(ctx);
                return state;
            }
            unit_ctx.index()++;
        }
        DEBUG_ONLY(for(auto& w : watchers) {if(w.suffix) w.suffix(ctx);})
        endline();
        unit_ctx = unit_ctx.parent();
        deep_recycle_context(ctx);
        return 0;
    }
    
    //To prove a point about continuations and Seaside
    uint32_t Unit::resume_travel_pass(Context ctx) {
        int& i = ctx.index();
        while(i < ctx.result().length()) {
            ctx.node(ctx.result().get(i));
            standard_process(ctx);
            Node nleft = ctx.result().get(i);
            ctx.left(nleft);
            DEBUG_ONLY(if(ERROR_FLAG) {endline(); return true;})
            if(ctx.state()>0) { //This is the return/break process
                endline();
                deep_recycle_context(ctx);
                return ctx.state();
            }
            i++;
        }
        endline();
        deep_recycle_context(ctx);
        return 0;
    }

    ColColCol& init_first_unit() {
        g_ptr<Unit> u = make<Unit>(false);
        units << u;
        return u->types;
    }
}

template<typename... Args>
void print_and_pause(float time, Args&&... args) {
  (std::cout << ... << args) << std::endl;
  Log::Line l; l.start();
  while(l.time_s()<time) {std::this_thread::sleep_for(std::chrono::nanoseconds(100));}
}

class Thread : public q_object
{
public:
std::atomic<float> tps{0.3f};
std::chrono::steady_clock::time_point lst = std::chrono::steady_clock::now();
bool logSPS = false;
std::string name;
std::thread impl;

Thread(std::string _name = "undefined") : name(_name) {}
~Thread() {
    end();
}

private:
    std::atomic<bool> runningSlice;
    std::atomic<bool> shouldStopThread;
    float sliceTime = 0;
    std::atomic<float> sliceSpeed{0.016f};

std::function<void()> onRun = nullptr;

std::mutex taskQueueMutex;
std::deque<std::function<void()>> taskQueue;

void simulationLoop() {
auto lastSliceTime = std::chrono::steady_clock::now();
auto SPSOutput = std::chrono::steady_clock::now();
    int sliceCounter = 0;
    while (!shouldStopThread) {
        auto currentTime = std::chrono::steady_clock::now();
        float delta = std::chrono::duration<float>(currentTime - lastSliceTime).count();
        if (runningTurn && !runningSlice && delta >= sliceSpeed) {
            runningSlice = true;
            if(onRun) onRun();
            slice++;
            runningSlice = false;
            float fallback = sliceSpeed;
            tps = tps>sliceSpeed ? fallback : delta;
            sliceCounter++;
            lastSliceTime = currentTime;
            lst = currentTime;
        }
        else if(!runningTurn) tps = 0.0f;

        if(std::chrono::duration<float>(currentTime - SPSOutput).count()>=1.0f)
        {
            if(logSPS)
            {
            print(name," SPS ",sliceCounter);
            }
            sliceCounter=0;
            SPSOutput = currentTime;
        }
        
        // Don't burn CPU waiting
        std::this_thread::sleep_for(std::chrono::nanoseconds(100));
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

public:

int id;

void run(std::function<void()> toRun,float speed = -1) {
    shouldStopThread = false;
    if(speed>0)
        setSpeed(speed);
    onRun = toRun;
    impl = std::thread(&Thread::simulationLoop, this);
}

void run_blocking(std::function<void()> func) {
    shouldStopThread = false;
    impl = std::thread([this, func]() {
        func();
    });
}

void pause() {
    runningTurn = false;
}

void start() {
    runningTurn = true;
}

std::atomic<int> slice;
std::atomic<bool> runningTurn;

void end() {
    shouldStopThread = true;
    if (impl.joinable()) {
        impl.join();
    }
}

void setSpeed(float speed)
{
    if(speed<=0.0f) {runningTurn = false; sliceSpeed.store(0);}
    else {sliceSpeed.store(speed); runningTurn = true;}
}

float getSpeed() {
    return sliceSpeed.load();
}

void waitForIdle() {
    while(runningSlice) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(100));
    }
}

void queueTask(std::function<void()> func) {
    std::lock_guard<std::mutex> lock(taskQueueMutex);
    taskQueue.push_back(func);
}

void queueAndWait(std::function<void()> func) {
std::promise<void> done;
auto future = done.get_future();

queueTask([&done, func]() {
    func(); 
    done.set_value();
});

future.get(); // Wait until it's done
}

void flushTasks() {
std::deque<std::function<void()>> localQueue;
{
    std::lock_guard<std::mutex> lock(taskQueueMutex);
    std::swap(localQueue, taskQueue);
}

auto startTime = std::chrono::steady_clock::now();
while (!localQueue.empty())
    // std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count() < sliceSpeed)
{
    auto task = localQueue.front();
    localQueue.pop_front(); 

    task();
}
if (!localQueue.empty()) std::cerr << "[WARN] Sim task flush incomplete!" << std::endl;

// Push any unprocessed tasks back into the main queue
{
    std::lock_guard<std::mutex> lock(taskQueueMutex);
    while (!localQueue.empty()) {
        taskQueue.push_back(std::move(localQueue.front()));
        localQueue.pop_front();
    }
}
}
};
#ifdef _WIN32
    struct TerminalLantern {};
#else
    #include <termios.h>
    #include <unistd.h>
    #include <csignal>
    struct TerminalLantern {
        termios old_termios;
        
        TerminalLantern() {
            tcgetattr(STDIN_FILENO, &old_termios);
            termios raw = old_termios;
            raw.c_lflag &= ~(ECHO | ICANON); //Disable echo and line buffering
            raw.c_cc[VMIN] = 1;  //Read one char at a time
            raw.c_cc[VTIME] = 0; //No timeout
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        }
        
        ~TerminalLantern() {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_termios); //Restore on destruction
        }
    };
#endif





char read_key() {
    #ifdef _WIN32
        return ' ';
    #else
        char c;
        read(STDIN_FILENO, &c, 1);
        return c;
    #endif
}


constexpr const char* strip_path(const char* path) {
    const char* last = path;
    for(const char* p = path; *p; p++) {
        if(*p == '/' || *p == '\\') last = p+1;
    }
    return last;
}

//q = -100
//^ = 2
//v = -2
//> = 1
//< = -1
//s = 3
//f = 4
int read_arrow() {
    #ifdef _WIN32
        return 0;
    #else
        char c = read_key();
        if(c == '\x1b') {
            char seq[2];
            read(STDIN_FILENO, &seq[0], 1);
            read(STDIN_FILENO, &seq[1], 1);
            if(seq[0]=='[') {
                if(seq[1]=='C') return 1; //>
                if(seq[1]=='D') return -1; //<
                if(seq[1]=='A') return 2; //^
                if(seq[1]=='B') return -2; //v
            }
        }
        if(c=='q') return -100; // quit
        if(c=='s') return 3;
        if(c=='f') return 4;
        return 0;
    #endif
}

namespace Acorn {

    #ifdef _WIN32
        void setup_signals() {}
    #else
        void signal_handler(int signal) {
            print("\nRECIVED SIGNAL: ",signal);
            if(ERROR_FLAG) {
                std::abort();
            }
            ERROR_FLAG = true;
            ERROR_MSG = "Console interrupt";
        }

        void setup_signals() {
            struct sigaction sa;
            sa.sa_handler = signal_handler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGINT, &sa, nullptr);
        }
    #endif

    struct Blackfeather_Unit : public virtual Unit {
        Blackfeather_Unit(uint16_t _uid) : Unit(_uid) {init();}
        Blackfeather_Unit() {init();}
        
        void init() override {
            setup_signals();
        }

        #define LOG_W(ctx, msg) DEBUG_ONLY(log_to_watcher(ctx, std::string(msg) + " [" + strip_path(__FILE__) + ":" + std::to_string(__LINE__) + "]"))

        void stamp_onto_page(Node node, list<std::string>& lines) {
            if(node.x()>=0.0f&&node.y()>=0.0f) {
                // print("STAMPING: ",node_info(node));
                int x = (int)node.x();
                int y = (int)node.y();
                while(y>=lines.length()) {lines << "";}
                while((x+node.name().length())>=lines[y].length()) lines[y]+=" ";
                for(char c : node.name().to_std()) lines[y][x++] = c;
                // for(auto l : lines) {
                //     print(escape_string(l,false));
                // }
            }
            for(int i=0;i<node.children().length();i++) stamp_onto_page(node.children()[i],lines);
            for(int i=0;i<node.quals().length();i++) stamp_onto_page(node.quals()[i],lines);
            for(int i=0;i<node.scopes().length();i++) stamp_onto_page(node.scopes()[i],lines);
            if(is_live(node.value())) {
                for(int i=0;i<node.value().quals().length();i++) stamp_onto_page(node.value().quals()[i],lines);
            }
        }
        std::string nodenet_to_string(Node root) {
            list<std::string> lines;
            stamp_onto_page(root,lines);
            std::string out = "";
            for(auto l : lines) {
                out+=l+"\n";
            }
            return out;
        }

        std::string idx_to_color(const std::string& num, int idx) {
            float hue = fmod(idx * 137.508f, 360.0f);
            float s = 0.8f, v = 0.9f;
            
            float c = v * s;
            float x = c * (1.0f - fabs(fmod(hue / 60.0f, 2.0f) - 1.0f));
            float m = v - c;
            float r,g,b;
            if(hue<60)       {r=c;g=x;b=0;}
            else if(hue<120) {r=x;g=c;b=0;}
            else if(hue<180) {r=0;g=c;b=x;}
            else if(hue<240) {r=0;g=x;b=c;}
            else if(hue<300) {r=x;g=0;b=c;}
            else             {r=c;g=0;b=x;}
            return rgb(num, (int)((r+m)*255), (int)((g+m)*255), (int)((b+m)*255));
        }
    
        //Remember to preserve and reverse the x/y of each node after
        void collect_stamps_by_data(Node node, list<Node>& nodes, map<uint64_t,bool>& visited, map<uint64_t,std::pair<float,float>>& reversions) {
            uint64_t key = Ptr_to_key(node);
            if(visited.getOrDefault(key, false)) return;
            visited.put(key, true);
            if(is_live(node.value())&&is_live(node.value().data_ptr())) {

                if((node.x()<0.0f||node.y()<0.0f)&&!node.quals().empty()) {
                    Node q = node.quals()[0];
                    reversions.put(key,std::make_pair<float,float>(node.x(),node.y()));
                    node.x(q.x());
                    node.y(q.y());
                }
                int x = (int)node.x();
                int y = (int)node.y();
                if(x>=0.0f&&y>=0.0f) {
                    int insert_at = nodes.length();
                    for(int i=0;i<nodes.length();i++) {
                        int ny = (int)nodes[i].y();
                        int nx = (int)nodes[i].x();
                        if(y<ny||(y==ny&&x<nx)) {
                            insert_at = i;
                            break;
                        }
                    }
                    nodes.insert(node, insert_at);
                }
            }
            for(int i=0;i<node.children().length();i++) collect_stamps_by_data(node.children()[i],nodes,visited,reversions);
            for(int i=0;i<node.scopes().length();i++) if(node.scopes()[i].owner()==node) collect_stamps_by_data(node.scopes()[i],nodes,visited,reversions);
        }
        void collect_stamps_unsorted(Node node, list<Node>& nodes) {
            if(node.x()>=0.0f&&node.y()>=0.0f) {
                int x = (int)node.x();
                int y = (int)node.y();
                nodes << node;
            }
            for(int i=0;i<node.children().length();i++) collect_stamps_unsorted(node.children()[i],nodes);
            for(int i=0;i<node.quals().length();i++) collect_stamps_unsorted(node.quals()[i],nodes);
            for(int i=0;i<node.scopes().length();i++) if(node.scopes()[i].owner()==node) collect_stamps_unsorted(node.scopes()[i],nodes);
            if(is_live(node.value())) {
                for(int i=0;i<node.value().quals().length();i++) collect_stamps_unsorted(node.value().quals()[i],nodes);
            }
        }
        void collect_stamps(Node node, list<Node>& nodes, map<uint64_t,bool>& visited) {
            uint64_t key = Ptr_to_key(node);
            if(visited.getOrDefault(key, false)) return;
            visited.put(key, true);
            
            if(node.x()>=0.0f&&node.y()>=0.0f) {
                int x = (int)node.x();
                int y = (int)node.y();
                int insert_at = nodes.length();
                for(int i=0;i<nodes.length();i++) {
                    int ny = (int)nodes[i].y();
                    int nx = (int)nodes[i].x();
                    if(y<ny||(y==ny&&x<nx)) {
                        insert_at = i;
                        break;
                    }
                }
                nodes.insert(node, insert_at);
            }
            for(int i=0;i<node.children().length();i++) collect_stamps(node.children()[i],nodes,visited);
            for(int i=0;i<node.quals().length();i++) collect_stamps(node.quals()[i],nodes,visited);
            for(int i=0;i<node.scopes().length();i++) if(node.scopes()[i].owner()==node) collect_stamps(node.scopes()[i],nodes,visited);
            if(is_live(node.value())) {
                for(int i=0;i<node.value().quals().length();i++) collect_stamps(node.value().quals()[i],nodes,visited);
            }
        }

        struct Stamper {
            Stamper() {}
            Stamper(std::function<std::string(Node,list<int>&)> _format, std::function<list<Node>(Node)> _collect) 
            : format(_format), collect(_collect)
            {}
            std::function<std::string(Node,list<int>&)> format;
            std::function<list<Node>(Node)> collect;
        };

        list<std::string> fstamp(Node root, Stamper stamper) {
            list<std::string> lines;
            list<int> offsets;
            list<Node> stamps = stamper.collect(root);
            for(int i=0;i<stamps.length();i++) {
                Node stamp = stamps[i];
                float px = stamp.x();
                float py = stamp.y();

                std::string to_stamp = stamper.format(stamp,offsets);
                int x = (int)stamp.x();
                int y = (int)stamp.y();
                while(y>=lines.length()) {lines << "";}
                while((x+to_stamp.length())>=lines[y].length()) lines[y]+=" ";
                for(char c : to_stamp) lines[y][x++] = c;
 
                stamp.x(px); //Because some stampers will modify the position of the stamp, we need to restore it after
                stamp.y(py); 
            }
            return lines;
        }
        std::string fnodenet_to_string(Node root, Stamper stamper) {
            list<std::string> lines = fstamp(root,stamper);
            std::string out = "";
            for(auto l : lines) {
                out+=l+"\n";
            }
            return out;
        }

        std::string fmultiline_nodenet(Node root,list<Stamper> stampers) {
            list<list<std::string>> stamps;
            size_t len = 0;
            for(auto s : stampers) {
                list<std::string> stamp = fstamp(root,s);
                if(stamp.length()>len) len = stamp.length();
                stamps << stamp;
            }
            std::string out = "";
            for(int i=0;i<len;i++) {
                for(auto s : stamps) {
                    if(i<s.length()&&!s[i].empty()) {
                        out+=s[i]+"\n";
                    }
                }
            }
            return out;
        }

        struct Flipbook : q_object {
            Flipbook() {}
            Flipbook(std::string _label) : label(_label) {}
            std::string label = "";
            std::ofstream out;
            size_t len = 0;
            void open() {
                std::string path = "mixos-acorn/flipbooks/"+label;
                out.open(path, std::ios::binary);
            }
            void close() {
                out.close();
            }
            void add_page(const std::string& page) {
                write_string(out,page);
                len++;
            }
            list<std::string> pages() {
                list<std::string> to_return;
                std::string path = "mixos-acorn/flipbooks/"+label;
                std::ifstream in(path, std::ios::binary);
                for(int i=0;i<len;i++) {
                    to_return << read_string(in);
                }
                in.close();
                return to_return;
            }
        };
        list<g_ptr<Flipbook>> flipbooks;

        g_ptr<Flipbook> get_flipbook(const std::string& label) {
            for(int i=0;i<flipbooks.length();i++) {
                if(flipbooks[i]->label==label) return flipbooks[i];
            }
            return nullptr;
        }
      
        void setup_stamp_res_flipbook() {
            Watcher w("stamp_res");
            w.stagestart = [this](Context& ctx){
                g_ptr<Flipbook> b = make<Flipbook>("stamp_res_"+active_stage->label+"_"+unit_label);
                b->open();
                flipbooks << b;
            };
            w.prefix = [this](Context& ctx) {
                g_ptr<Flipbook> b = get_flipbook("stamp_res_"+active_stage->label+"_"+unit_label);    
                b->add_page(
                    "\n\n\n\n\n\n"+
                    fnodenet_to_string(unit_root,Stamper{[&ctx](Node n, list<int>& offsets){
                        std::string s1 = n.name().to_std();
                        std::string s2 = s1;
                        if(n==ctx.node()) {
                            s2 = blue(s2);
                        } else {
                            if(n.resolved()) {
                                s2 = gray(s2);
                            } else {
                                s2 = white(s2);
                            }
                        }
                        while((int)n.y()>=offsets.length()) {offsets<<0;}
                        n.x(n.x()+offsets[(int)n.y()]);
                        offsets[(int)n.y()]+=s2.length()-s1.length();
                        s1 = s2;
                        return s1;
                },[this](Node n){
                        list<Node> stamps;
                        map<uint64_t,bool> visited;
                        collect_stamps(n,stamps,visited);
                        return stamps;
                }})+blue(std::to_string(b->len+1)));
            };
            w.suffix = [this](Context& ctx) {
                ctx.node().resolved(true);
                for(int i=0;i<ctx.node().quals().length();i++)  {
                    Node q = ctx.node().quals()[i]; 
                    if(q.mute()) {q.resolved(true);}
                }
            };
            w.stagend = [this](Context& ctx){
                g_ptr<Flipbook> b = get_flipbook("stamp_res_"+active_stage->label+"_"+unit_label);
                if(b) b->close();
                walk_nodenet(unit_root,[](Node n){n.resolved(false);});
            };
            watchers << w;
        }

        void setup_trace_res_flipbook() {
            Watcher w("trace_res");
            w.stagestart = [this](Context& ctx){
                g_ptr<Flipbook> b = make<Flipbook>("trace_res_"+active_stage->label+"_"+unit_label);
                b->open();
                flipbooks << b;
            };
            w.prefix = [this](Context& ctx) {                
                g_ptr<Flipbook> b = get_flipbook("trace_res_"+active_stage->label+"_"+unit_label);   
                if(b) {
                    ptr_colors[Ptr_to_key(ctx.node())] = [](std::string& s){s = white("> "+s);}; //Ptr on
                    b->add_page("\n\n\n\n\n\n\n"+node_to_string(unit_root)+"\n"+blue(std::to_string(b->len+1)));
                    ptr_colors[Ptr_to_key(ctx.node())] = [](std::string& s){s = pine("~ "+s);}; //Ptr resolving
                }
            };
            w.suffix = [this](Context& ctx) {
                g_ptr<Flipbook> b = get_flipbook("trace_res_"+active_stage->label+"_"+unit_label);      
                if(b) {
                    ptr_colors[Ptr_to_key(ctx.node())] = [](std::string& s){s = green("> "+s);}; //Ptr finished resolving
                    b->add_page("\n\n\n\n\n\n\n"+node_to_string(unit_root)+"\n"+blue(std::to_string(b->len+1)));
                    for(int i=0;i<ctx.node().quals().length();i++)  {
                        Node q = ctx.node().quals()[i]; 
                        if(q.mute()) {
                            ptr_colors[Ptr_to_key(q)] = [](std::string& s){s = gray(s);};
                        }
                    }
                    ptr_colors[Ptr_to_key(ctx.node())] = [](std::string& s){s = gray(". "+s);}; //Ptr resolved
                }
            };
            w.logger = [this](Context& ctx) {
                g_ptr<Flipbook> b = get_flipbook("trace_res_"+active_stage->label+"_"+unit_label);  
                if(b) {
                    b->add_page("\n\n\n\n\n\n\n"+node_to_string(is_live(ctx.root())?ctx.root():ctx.node())+"\n"+blue(std::to_string(b->len+1))+": "+GLOBAL_MSG);
                }
            };
            w.stagend = [this](Context& ctx){
                g_ptr<Flipbook> b = get_flipbook("trace_res_"+active_stage->label+"_"+unit_label);
                if(b) {
                    b->close();
                    ptr_colors.clear();
                }
            };
            watchers << w;
        }

        void clear_terminal() {
            print("\033[3J\033[H");
        }
        void enter_alt_screen() { print("\033[?1049h"); }
        void exit_alt_screen()  { print("\033[?1049l"); }

        void print_page_at(const std::string& page, int offset) {
            print("\033[H\033[2J");
            list<std::string> lines = split_str(page, '\n');
            int terminal_height = 40; // or query with TIOCGWINSZ
            for(int i = offset; i < std::min((size_t)(offset + terminal_height), lines.length()); i++) {
                print(lines[i]);
            }
        }

        void navigate_flipbook(g_ptr<Flipbook> flipbook) {
            list<std::string> book = flipbook->pages();
            if(book.empty()) {print("Flipbook ",flipbook->label," is empty"); return;}
            int on_page = 0;
            TerminalLantern lantern;
            std::string next = book[on_page];
            int line_offset = 0;
            enter_alt_screen();
            while(true) {
                clear_terminal();
                print_page_at(next,line_offset);
                int key = read_arrow();
                if(key == 1 && on_page < book.length()-1) {on_page++; next = book[on_page];} //>
                if(key == -1 && on_page > 0) {on_page--; next = book[on_page];} //<
                if(key == -2) {if(line_offset<book.length()) line_offset++;} //v
                if(key == 2) {if(line_offset>0) line_offset--;} //^
                // if(key == -2) {if(on_page-5 > 0) {on_page-=5;} else {on_page=0;} next = book[on_page];} //v
                // if(key == 2) {if(on_page+5 < book.length()-1) {on_page+=5;} else {on_page=book.length()-1;} next = book[on_page];} //^
                if(key == -100) break;
            }
            exit_alt_screen();
            print("Exited navigation");
        }


        float flip_speed = 0.07f;
        int flip_pages(list<std::string> book, int start_page = 0, int flip_to = -1) {
            if(book.empty()) return 0;
            if(flip_to==-1) flip_to = book.length()-1;
            int on_page = start_page;
            Log::Line l; l.start();
            while(on_page!=flip_to) {
                if(ERROR_FLAG) break;
                if(l.time_s()>=flip_speed) {
                    std::string p = book[on_page];
                    print(p);
                    if(flip_to>on_page) {
                        on_page++;
                    } else if(flip_to<on_page) {
                        on_page--;
                    }
                    if(on_page>=book.length()||on_page<=0) {
                        break;
                    }
                    l.start();
                }
            }
            ERROR_FLAG = false;
            return on_page;
        }


        bool silence_blackfeather = false;
        void launch_blackfeather(list<Node> roots) {
            if(silence_blackfeather) return;
            std::string line;
            Node on_node = deadptr;
            while(std::getline(std::cin, line)) {
                if(line.empty()) break;
                if(line == "exit") break;
                if(line == "cont") break;

                for(int i=0;i<line.length();i++) {
                    if(line.at(i)=='|') {
                        if(line.at(i+1)==' ') {line.erase(i+1,1);}
                        if(line.at(i-1)==' ') {line.erase(i-1,1); i--;}
                    }
                }

                list<std::string> piped_cmds = split_str(line,'|');
                std::string mode = "";
                bool echo = false;
                bool is_invalid = false;
                for(auto pcmd : piped_cmds) {
                    list<std::string> cmds = split_str(pcmd,' ');
                    if(pcmd.empty()) {is_invalid = true; continue;}

                    if(mode.empty()) { //This is to let us pipe without constantly redeclaring the first scope
                        mode=cmds[0];
                    } else {
                        cmds.insert(mode,0);
                    }

                    if(cmds[0]=="trace") {
                        if(cmds.length()==1) {is_invalid = true; continue;}
                        if(cmds[1]=="count") {
                            print(roots.length()); 
                        } else if(cmds[1]=="print") {
                            if(cmds.length()==2) {is_invalid = true; continue;}
                            if(cmds[2]=="all") {
                                for(auto r : roots) {
                                    print(node_to_string(r));
                                }
                            }  else {
                                if(!is_str_num(cmds[2])) {is_invalid = true; continue;}
                                int root_id = std::stoi(cmds[2]);
                                if(root_id<roots.length()) {
                                    print(node_to_string(roots[root_id]));
                                } else {
                                    print(red("ROOT INDEX OUT OF BOUNDS"));
                                }
                            }
                            echo = true;
                        } else if(cmds[1]=="stamp") {
                            if(cmds.length()==2) {is_invalid = true; continue;}
                            if(cmds[2]=="all") {
                                for(auto r : roots) {
                                    print(nodenet_to_string(r));
                                }
                            } else if(cmds[2]=="test") {
                                print(fnodenet_to_string(roots[0],Stamper{[](Node n, list<int>& offsets){
                                    std::string to_return = n.name().to_std();
                                    if(n.mute()) {
                                        std::string nreturn = gray(to_return);
                                        while((int)n.y()>=offsets.length()) {offsets<<0;}
                                        n.x(n.x()+offsets[(int)n.y()]);
                                        offsets[(int)n.y()]+=nreturn.length()-to_return.length();
                                        to_return = nreturn;
                                    }
                                    return to_return;
                                },[this](Node n){
                                    list<Node> stamps;
                                    map<uint64_t,bool> visited;
                                    collect_stamps(n,stamps,visited);
                                    return stamps;
                                }}));
                            } else {
                                if(!is_str_num(cmds[2])) {is_invalid = true; continue;}
                                int root_id = std::stoi(cmds[2]);
                                if(root_id<roots.length()) {
                                    print(nodenet_to_string(roots[root_id]));
                                } else {
                                    print(red("ROOT INDEX OUT OF BOUNDS"));
                                }
                            }
                            
                            echo = true;
                        } else if(cmds[1]=="live") {
                            for(auto r : roots) {
                                //print(nodenet_to_lifetimes(r));
                                print(fmultiline_nodenet(r,{
                                    Stamper{[this](Node n, list<int>& offsets){
                                        while((int)n.y()>=offsets.length()) {offsets<<0;}
                                        n.x(n.x()+offsets[(int)n.y()]);
                                        return n.name().to_std();
                                    },[this](Node n){
                                        list<Node> stamps;
                                        map<uint64_t,bool> visited;
                                        collect_stamps(n,stamps,visited);
                                        return stamps;
                                    }},
                                    Stamper{[this](Node n, list<int>& offsets){
                                        std::string to_return = "";
                                        if(is_live(n.value())&&is_live(n.value().data_ptr())) {
                                            int idx = n.value().data_ptr().idx;
                                            to_return = std::to_string(idx);
                                            std::string nreturn = idx_to_color(std::to_string(idx),idx);
                                            uint32_t padlen = n.name().length();
                                            if((n.x()<0.0f||n.y()<0.0f)&&!n.quals().empty()) {
                                                Node q = n.quals()[0];
                                                n.x(q.x()); n.y(q.y());
                                                padlen = q.name().length();
                                            }
                                            while((int)n.y()>=offsets.length()) {offsets<<0;}
                                            n.x(n.x()+offsets[(int)n.y()]);
                                            offsets[(int)n.y()]+=nreturn.length()-to_return.length();

                                            uint32_t visible_len = std::to_string(idx).length();
                                            nreturn = center_pad_known(nreturn, visible_len, padlen);

                                            to_return = nreturn;
                                        }   
                                        return to_return;
                                    },[this](Node n){
                                        list<Node> stamps;
                                        map<uint64_t,bool> visited;
                                        map<uint64_t,std::pair<float,float>> reversions;
                                        collect_stamps_by_data(n,stamps,visited,reversions);
                                        for(auto e : reversions.entrySet()) {
                                            Node n(key_to_Ptr(e.key));
                                            n.x(e.value.first);
                                            n.y(e.value.second);
                                        }
                                        return stamps;
                                    }},
                                }));
                            }
                            echo = true;
                        }
                    } else if(cmds[0]=="span") {
                        if(cmds.length()==1) {is_invalid = true; continue;}
                        if(cmds[1]=="print") {
                            if(cmds.length()==2) {is_invalid = true; continue;}
                            if(cmds[2]=="all") {
                                span->print_all();
                            } else {
                                //Nothing here yet
                            }
                            echo = true;
                        }
                    } else if(cmds[0]=="unit") {
                        if(cmds.length()==1) {is_invalid = true; continue;}
                        if(cmds[1]=="print"||cmds[1]=="dump") {
                            bool do_print = cmds[1]=="print";
                            if(do_print) echo = true;
                            if(cmds.length()>2) {
                                uint32_t addr0 = 0;
                                if(is_str_num(cmds[2])) {
                                    addr0 = std::stoi(cmds[2]);
                                } else {
                                    for(int i=0;i<types.length();i++) {
                                        if(types[i].label==cmds[2]) {
                                            addr0 = i; break;
                                        }
                                    }
                                }
                                ColCol& ptype = types[addr0];
                                if(cmds.length()==3) {
                                    if(do_print) {
                                        print(type_to_string(ptype));
                                    } else {
                                        writeFile("mixos-acorn/tests/printout.txt",type_to_string(ptype));
                                    }
                                } else {
                                    uint32_t addr1 = 0;
                                    if(is_str_num(cmds[3])) {
                                        addr1 = std::stoi(cmds[3]);
                                    } else {
                                        if(cmds[3]=="on") {addr1 = on_node.idx;}
                                    }
                                    if(cmds.length()==4) {
                                        if(do_print) {
                                            print(type_to_string(ptype));
                                        } else {
                                            list<list<std::string>> plines = type_to_lines(ptype);
                                            list<std::string> tline = plines[addr1];
                                            writeFile("mixos-acorn/tests/printout.txt","");
                                            editTextFile("mixos-acorn/tests/printout.txt",[tline,this](std::string& src){
                                                list<list<std::string>> col = {tline};
                                                src = print_columnar_table(col);
                                            });
                                        }
                                    } else {
                                        uint32_t addr2 = std::stoi(cmds[4]);
                                    }
                                }
                            } else {
                                if(!do_print) {
                                    dump_unit(true);
                                }
                            }
                        }
                    } else if(cmds[0]=="node") {
                        if(cmds.length()==1) {is_invalid = true; continue;}
                        if(is_str_num(cmds[1])) {
                            uint32_t node_id = std::stoi(cmds[1]);
                            Ptr nptr(&types,node_type_id,node_id,0);
                            on_node = (Node&)nptr;
                            print("on_node: ",node_info(on_node));
                        } else {
                            if(cmds[1]=="print") {
                                if(cmds.length()>2) {
                                    _layout& ntemp = layouts[node_id];
                                    uint32_t idx = ntemp.label_to_index[cmds[2]];
                                    print(tag_to_str(ntemp.tags[idx],resolve_to_col(on_node).qget(ntemp.offsets[idx])));
                                } else {
                                    print(node_to_string(on_node));
                                    echo = true;
                                }
                            } else {
                                if(cmds[1]=="in_scope") {
                                    on_node = on_node.in_scope();
                                } else if(cmds[1]=="owner") {
                                    on_node = on_node.owner();
                                } else if(cmds[1]=="child") {
                                    uint32_t cidx = std::stoi(cmds[2]);
                                    on_node = on_node.children()[cidx];
                                } else if(cmds[1]=="qual") {
                                    uint32_t qidx = std::stoi(cmds[2]);
                                    on_node = on_node.quals()[qidx];
                                } else if(cmds[1]=="scope") {
                                    uint32_t scidx = std::stoi(cmds[2]);
                                    on_node = on_node.scopes()[scidx];
                                }
                                print("on_node: ",node_info(on_node));
                            }
                        }
                    } else if(cmds[0]=="flipbook"||cmds[0]=="flip"||cmds[0]=="f") {
                        if(cmds.length()==1) {is_invalid = true; continue;}
                        if(cmds[1]=="list") {
                            print("Listing ",flipbooks.length()," flipbooks");
                            for(int i=0;i<flipbooks.length();i++) {
                                print(i,": ",flipbooks[i]->label);
                            }
                        } else if(cmds[1]=="play"||cmds[1]=="open") {
                            if(cmds.length()>2) {
                                print("Running ",cmds[1],"-",cmds[2]);
                                g_ptr<Flipbook> b = nullptr;
                                if(is_str_num(cmds[2])) {
                                    int f_idx = std::stoi(cmds[2]);
                                    if(f_idx<flipbooks.length()&&f_idx>=0) b = flipbooks[f_idx];
                                } else {
                                    get_flipbook(cmds[2]);
                                }
                                if(b) {              
                                    if(cmds[1]=="play") {
                                        print("Playing: ",b->label);
                                        print("Pages: ",b->len);
                                        flip_pages(b->pages());
                                        echo = true;
                                    }
                                    if(cmds[1]=="open") {
                                        print("Opened: ",b->label);
                                        print("Pages: ",b->len);
                                        navigate_flipbook(b);
                                        echo = true;
                                    }
                                } else {
                                    print("Could not find flipbook ",cmds[2]);
                                }
                            }
                        } else if(cmds[1]=="all") {
                            for(auto b : flipbooks) {
                                print_and_pause(0.7f,"\n\n\n\n\n\n",b->label," pages: ",b->len,"\n\n\n\n\n\n");
                                flip_pages(b->pages());
                            }
                        } else if(cmds[1]=="speed") {
                            if(cmds.length()>2) {
                                if(is_str_num(cmds[2])) {
                                    flip_speed = std::stof(cmds[2]);
                                }
                            } else {
                                print("Flip speed: ",flip_speed);
                            }   
                        } else {
                            is_invalid = true;
                        }
                    } else {
                        is_invalid = true;
                    }
                }
                if(echo) {
                    print("[",line,"]");
                }
                if(is_invalid) {
                    print(red("Invalid command: "+line));
                }
            }
        }

        void launch_blackfeather(Node root) {
            list<Node> roots = {root};
            launch_blackfeather(roots);
        }

        void launch_blackfeather() {
            list<Node> roots = {unit_root};
            launch_blackfeather(roots);
        }
    };
}
namespace Acorn {
    struct Compiler_Unit : public virtual Blackfeather_Unit {


        Compiler_Unit(uint16_t _uid) : Unit(_uid)  { init(); }
        Compiler_Unit() {init();}

        uint32_t setup_unit_data() {
            uint32_t idx = types.length();
            ColCol unitdata;
            types.push(unitdata);
            return idx;
        }

        uint32_t unitdata_col = setup_unit_data();
        uint32_t global_value_table_idx = note_value(types[unitdata_col], "global_values", sizeof(Ptr), value_id);
        uint32_t global_node_table_idx = note_value(types[unitdata_col], "global_nodes", sizeof(Ptr), node_id);
        
        
        Stage& a_handlers = reg_stage("assembling");
        Stage& s_handlers = reg_stage("scoping");
        Stage& t_handlers = reg_stage("typing");
    
        Stage& d_handlers = reg_stage("discovering");
        Stage& r_handlers = reg_stage("resolving");
        Stage& e_handlers = reg_stage("evaluating");
    
        Stage& m_handlers = reg_stage("modeling");
        Stage& i_handlers = reg_stage("inspecting");
        Stage& x_handlers = reg_stage("executing");

        map<std::string,Value> keywords;
        //Qual handlers which act on the value
        size_t to_prefix_id(size_t id) {return id+1;}
        //Qual handlers which act on the node
        size_t to_suffix_id(size_t id) {return id+2;}

        void a_pass_resolve_keywords(node_col nodes, int context = -1) {
            for(int i=0;i<nodes.length();i++) {
                Node node = nodes[i];
                log("Keyword resolving ",node_info(node));
                if(keywords.hasKey(node.name().to_std())) {
                    for(Value v : keywords.getAll(node.name().to_std())) {
                        if(is_live(v)) {
                            if(v.reg()==-1||v.reg()==context) { //By default is -1
                                node.value(make_value()); //Make a value to copy into
                                node.value().copy(v,true);
                                node.value().reg(-1); //Reset to -1 for cleanliness
                            }
                        }
                    }
                }

                if(is_live(node.value())) {
                    context =  node.value().sub_type();
                }

                a_pass_resolve_keywords(node.children(), context);

                //No scopes! To work with precompiling passes and also because there shouldn't be any scopes in a stage
                // for(int s = 0;s<node.scopes().length();s++) {
                //     a_pass_resolve_keywords(node.scopes()[s].children(), context);
                // }
            }
        };


        Value make_qual_value(const std::string& f, uint32_t size = 0) {
            uint32_t id = reg_id(f);
            uint32_t prefix_id = reg_id(f);
            uint32_t suffix_id = reg_id(f);
            Value val = make_value(id,size);
            val.sub_type(id);
            return val;
        }

        uint32_t register_qual_ids(const std::string& f) {
            uint32_t id = reg_id(f);
            uint32_t prefix_id = reg_id(f);
            uint32_t suffix_id = reg_id(f);
            return id;
        }

        void add_type_stamping_handler(uint32_t type) {
            t_handlers[to_prefix_id(type)] = [](Context& ctx){
                if(ctx.value().sub_type() == 0) {
                    ctx.value().sub_type(ctx.qual().sub_type());
                    ctx.value().type(ctx.qual().type());
                    ctx.value().size(ctx.qual().value().size());
                    if(is_live(ctx.qual().value().type_scope()))
                        ctx.value().type_scope(ctx.qual().value().type_scope());
                }
            };
            // r_handlers[to_prefix_id(type)] = [](Context& ctx){
            //     if(is_live(ctx.value())&&ctx.value().quals()[0]==ctx.qual()) {
            //         ctx.value().type(ctx.qual().value().type());
            //         ctx.value().size(ctx.qual().value().size());
            //     }
            // };
        }

        Value make_type_value(const std::string& f, size_t size = 0) {
            Value val = make_qual_value(f,size);
            add_type_stamping_handler(val.type());
            return  val;
        }

        uint32_t add_qual(const std::string& f, uint32_t size = 0) {
            Value val = make_qual_value(f,size);
            keywords.put(f,val);
            return val.type();
        }


        void register_type_initilizers(uint32_t prefix_type) {
            r_handlers[prefix_type] = [this](Context& ctx){
                if(ctx.value().size()==0&&layouts.hasKey(ctx.value().type())) {
                    ctx.value().size(layouts.get(ctx.value().type()).total_size);
                }
            };
            x_handlers[prefix_type] = [this](Context& ctx){
                if(is_live(ctx.value())&&ctx.value().quals()[0].type()!=ptr_id) { //Beause Ptrs store subtypes in their quals
                    ctx.value().data_col().push_default();
                    ctx.value().data_col().heterogenous = true;
                }
            }; 
        }
        uint32_t make_type(const std::string& f, uint32_t size = 0) {
            Value val = make_type_value(f,size);
            keywords.put(f,val);
            register_type_initilizers(to_prefix_id(val.type()));
            return val.type();
        }

        void register_type(const std::string& label, uint32_t type, uint32_t size) {
            Value val = make_value(type,size); val.sub_type(type);
            add_type_stamping_handler(type);
            keywords.put(label,val);
        }

        Value register_value(const std::string& name, uint32_t size = 0,uint32_t type = 0) {
            Value v = make_value(type,size);
            v.sub_type(reg_id(name));
            return v;
        }

        uint32_t make_keyword(const std::string& name, uint32_t size = 0, std::string type_name = "", uint32_t sub_type = 0) {
            Value val = register_value(type_name==""?name:type_name,size,sub_type);
            keywords.put(name,val);
            return val.sub_type();
        }

        void add_alias(const std::string& name, uint32_t type) {
            keywords.put(name,keywords.get(labels[type]));
        }



        map<std::string,uint32_t> tokenized_keywords;
        map<char,bool> char_is_split;

        map<char, Handler> tokenizer_functions;
        map<uint32_t, Handler> tokenizer_state_functions;
        Handler tokenizer_default_function = nullptr;

        float at_x = 0.0f;
        float at_y = 0.0f;

        size_t make_tokenized_keyword(const std::string& token_name, size_t default_id = 0) {
            size_t id = default_id;
            if(default_id==0) {
                id = reg_id(token_name);
            }
            tokenized_keywords.put(token_name,id);
            return id;
        }

        map<uint32_t,map<uint32_t,uint32_t>> token_combos;
        inline uint32_t combine_tokens(char a, char b, char c = '\0', char d = '\0') {
            return  (uint32_t(uint8_t(a)) << 24) |
                    (uint32_t(uint8_t(b)) << 16) |
                    (uint32_t(uint8_t(c)) << 8)  |
                    (uint32_t(uint8_t(d)));
        }
        uint32_t add_token_combo(const std::string& f, char a, char b, char c = '\0', char d = '\0') {
            uint32_t id = reg_id(f);
            token_combos[a].put(combine_tokens(a,b,c,d),id);
            return id;
        }
        int find_token_combo(Context& ctx) {
            char a = ctx.source().at(ctx.index());
            if(!token_combos.hasKey(a)) return 0;

            char c[4] = {a,'\0','\0','\0'};
            for(int i=1;i<4;i++) { //To safely fill without outrunning the length of source
                if(ctx.index()+i < ctx.source().length()) {
                    c[i] = ctx.source().at(ctx.index()+i);
                }
            }

            uint32_t zero = 0;
            map<uint32_t,uint32_t>& combos = token_combos.get(a);
            for(int i=3;i>0;i--) {
                uint32_t key = combine_tokens(c[0],c[1],c[2],c[3]);
                uint32_t result = combos.getOrDefault(key,zero);

                // printnl(" ",i,": ");
                // for(int m=0;m<4;m++) {
                //     printnl(c[m],"[",std::to_string(m),"]");
                // }
                // print(" : ",labels[result]);

                if(result!=0) {
                    ctx.node().type(result);
                    return i;
                }
                c[i] = '\0';
            }
            return 0;
        }

        size_t add_token(char c, const std::string& f) {
            size_t id = reg_id(f);
            tokenizer_functions[c] = [this,id,c](Context& ctx) {
                ctx.node(make_node(0,0,"",at_x,at_y));
                int to_skip = find_token_combo(ctx);
                if(to_skip!=0) { 
                    ctx.node().name().push(c);
                    for(int i=0;i<to_skip;i++) {
                        ctx.index()++;
                        if(ctx.index()<ctx.source().length()) {
                            at_x+=1.0f;
                            ctx.node().name().push(ctx.source().at(ctx.index()));
                        }
                    }
                } else {
                    ctx.node().type(id);
                    ctx.node().name().push(c);
                }
                if(tokenizer_state_functions.hasKey(ctx.node().type())) {
                    ctx.state(ctx.node().type());
                }
                ctx.result().push(ctx.node());
            };
            char_is_split.put(c, true);
            return id;
        }


        list<size_t> discard_types;

        size_t to_decl_id(size_t id) {return id+1;}
        size_t to_unary_id(size_t id) {return id+2;}
        
        size_t lparen_id = add_token('(',"LPAREN");
        size_t rparen_id = add_token(')',"RPAREN");
        size_t comma_id = add_token(',',"COMMA");
        size_t lbracket_id = add_token('[', "LBRACKET");
        size_t rbracket_id = add_token(']', "RBRACKET");
        size_t lbrace_id = add_token('{', "LBRACE");
        size_t rbrace_id = add_token('}', "RBRACE");

        size_t in_alpha_id = reg_id("IN_ALPHA");
        size_t in_digit_id = reg_id("IN_DIGIT");
        size_t end_id = add_token(';',"END"); //Can commonly be changed to be a line return
        size_t quote_id = add_token('"',"QUOTE");
        size_t comment_id = reg_id("COMMENT");
        size_t single_quote_id = add_token('\'',"SINGLE_QUOTE");

        Node tokenize(const std::string& code) {
            Node root = make_node();
            root.name("ROOT");
            node_col result = root.children();
            uint32_t state = 0;
            at_x = 0.0f;
            at_y = 0.0f;
            Context ctx = make_context(result);
            int& index = ctx.index();

            ctx.source(code);
            ctx.root(root);

            #if PRINT_ALL
            newline("tokenize pass");
            #endif

            if(!tokenizer_default_function) {
                print("GDSL::tokenize warning! No defined default function, please define one");
            }

            while (index<code.length()) {
                char c = code.at(index);
                Handler* func = nullptr;
                if(ctx.state()!=0&&tokenizer_state_functions.hasKey(ctx.state())) {
                    func = &tokenizer_state_functions.get(ctx.state());
                } else {
                    func = &tokenizer_functions.getOrDefault(c,tokenizer_default_function);
                }

                if(func) {
                    (*func)(ctx);
                }

                at_x += 1.0f;
                ++index;
            }  

            #if PRINT_ALL
            int i = 0;
            for(int t=0;t<result.length();t++) {
                log(i++," ",labels[result.get(t).type()],": ",result.get(t).name());
            }
            endline();
            #endif
            deep_recycle_context(ctx);
            return root;
        }

        void init_tokenizer() {
            // Literals_Unit::init();
            char_is_split.put(' ',true);
            tokenizer_state_functions.put(in_alpha_id,[this](Context& ctx) {
                char c = ctx.source().at(ctx.index());
                if(char_is_split.getOrDefault(c,false)) {
                    ctx.state(0); 
                    at_x-=1.0f;
                    --ctx.index();
                    ctx.node().type(tokenized_keywords.getOrDefault(ctx.node().name().to_std(),ctx.node().type()));
                    return;
                } else {
                    ctx.node().name().push(c);
                    if(ctx.index()+1==ctx.source().length()) {
                        ctx.state(0); 
                        ctx.node().type(tokenized_keywords.getOrDefault(ctx.node().name().to_std(),ctx.node().type()));
                    }
                }
            });
    
            tokenizer_state_functions.put(in_digit_id,[this](Context& ctx) {
                char c = ctx.source().at(ctx.index());
                if(c=='.') {
                    ctx.node().type(float_id);
                } else if(c=='|') {
                    ctx.node().type(ptr_id);
                } else if(char_is_split.getOrDefault(c,false)) {
                    ctx.state(0); 
                    at_x-=1.0f;
                    --ctx.index();
                    return;
                } else if(std::isalpha(c)) {
                    ctx.state(in_alpha_id);
                }
                ctx.node().name().push(c);
            });


            tokenizer_functions[' '] = [this](Context& ctx) {
                //Just skip
            };
            tokenizer_functions['\t'] = [this](Context& ctx) {
                //Just skip
            };
            tokenizer_functions['\n'] = [this](Context& ctx) {
                at_y += 1.0f;
                at_x = -1.0f;
            };
    
            tokenizer_default_function = [this](Context& ctx) {
                char c = ctx.source().at(ctx.index());
                if(std::isalpha(c)) {
                    ctx.state(in_alpha_id);
                    ctx.node(make_node(identifier_id,0,std::string(1,c),at_x,at_y));
                    ctx.result().push(ctx.node());
                }
                else if(std::isdigit(c)) {
                    ctx.state(in_digit_id);
                    ctx.node(make_node(int_id,0,std::string(1,c),at_x,at_y));
                    ctx.result().push(ctx.node());
                }  else {
                    print("tokenize:default_function missing handling for char: ",c);
                }

                int to_skip = find_token_combo(ctx);
                if(to_skip!=0) {
                    ctx.state(0);
                    for(int i=0;i<to_skip;i++) {
                        ctx.index()++;
                        if(ctx.index()<ctx.source().length()) {
                            at_x+=1.0f;
                            ctx.node().name().push(ctx.source().at(ctx.index()));
                        }
                    }
                }
            };

            // tokenizer_default_function = [this](Context& ctx) {
            //     char c = ctx.source().at(ctx.index());
            //     int to_skip = find_token_combo(ctx);
            //     if(to_skip!=0) {
            //         ctx.node(make_node(0,0,"",at_x,at_y));
            //         ctx.node().name().push(c);
            //         for(int i=0;i<to_skip;i++) {
            //             ctx.index()++;
            //             if(ctx.index()<ctx.source().length()) {
            //                 at_x+=1.0f;
            //                 ctx.node().name().push(ctx.source().at(ctx.index()));
            //             }
            //         }
            //         ctx.result().push(ctx.node());
            //     } else {
            //         if(std::isalpha(c)) {
            //             ctx.state(in_alpha_id);
            //             ctx.node(make_node(identifier_id,0,std::string(1,c),at_x,at_y));
            //             ctx.result().push(ctx.node());
            //         } else if(std::isdigit(c)) {
            //             ctx.state(in_digit_id);
            //             ctx.node(make_node(int_id,0,std::string(1,c),at_x,at_y));
            //             ctx.result().push(ctx.node());
            //         }  else {
            //             print("tokenize:default_function missing handling for char: ",c);
            //         }
            //     }               
            // };
        }



        bool find_value_in_scope(Node node) {
            if(node.in_scope().value_table().hasKey(node.name().to_std())) {
                node.value(node.in_scope().value_table().get(node.name().to_std()));
                return true;
            }
            value_col vcol(uid, unitdata_col, global_value_table_idx);
            if(vcol.hasKey(node.name().to_std())) {
                node.value(vcol.get(node.name().to_std()));
                return true;
            }
            return false;
        }


        bool find_node_in_scope(Node node) {
            if(node.in_scope().node_table().hasKey(node.name().to_std())) {
                if(node.scopes().length()>0) node.scopes().clear();
                node.scopes().push(node.in_scope().node_table().get(node.name().to_std()));
                return true;
            }
            node_col ncol(uid, unitdata_col, global_node_table_idx);
            if(ncol.hasKey(node.name().to_std())) {
                node.scopes().push(ncol.get(node.name().to_std()));
                return true;
            }
            return false;
        }

        // Value distribute_value(QNode& node, const std::string& label, Value val) {
        //     if(node.value_table().hasKey(label)) {
        //         Value table_value = node.value_table().get(label);
        //         if(table_value.type() == 0) {
        //             table_value.copy(val);
        //             val = table_value;
        //         }
        //     } else {
        //         node.value_table().put(label, val);
        //     }
        //     for(int c = 0;c<node.children().length();c++) {
        //         QNode& child = node.children()[c].toQ();
        //         if(!child.scopes().empty()) {
        //             for(int s = 0;s<child.scopes().length();s++) {
        //                 QNode& scope = child.scopes().get(s).toQ();
        //                 if(scope.owner().idx==child.idx) {
        //                     val = distribute_value(scope,label,val);
        //                 }
        //             }
        //         }
        //     }
        //     return val;
        // }

        uint32_t hoisted_id = add_qual("hoisted");
        uint32_t global_qual = add_qual("global");

        Value distribute_value_children(Node node, const std::string& label, Value val) {
            for(int c = 0;c<node.children().length();c++) {
                Node child = node.children()[c];
                for(int s = 0;s<child.scopes().length();s++) {
                    Node scope = child.scopes().get(s);
                    if(scope.owner().idx==child.idx) {
                        val = distribute_value(scope,label,val,0);
                    }
                }
                val = distribute_value_children(child, label, val);
            }
            return val;
        }

        Value distribute_value(Node node, const std::string& label, Value val, int initial_hoist) {
            if(initial_hoist!=0) {
                Node climb = node;
                for(int i = 0; i < initial_hoist && is_live(climb.owner()); i++) {
                    climb = climb.owner().in_scope();
                }
                return distribute_value(climb, label, val, 0);
            }

            if(node.value_table().hasKey(label)) {
                Value table_value = node.value_table().get(label);
                if(table_value.type() == 0) {
                    table_value.copy(val,false);
                    val = table_value;
                }
            } else {
                node.value_table().put(label, val);
            }
            for(int c = 0;c<node.children().length();c++) {
                Node child = node.children()[c];
                if(!child.scopes().empty()) {
                    for(int s = 0;s<node.children()[c].scopes().length();s++) {
                        Node scope = child.scopes().get(s);
                        if(scope.owner().idx==child.idx) {
                            val = distribute_value(scope,label,val,0);
                        }
                    }
                }
                val = distribute_value_children(child,label,val);
            }
            return val;
        }

        Node distribute_node_children(Node node, const std::string& label, Node carry) {
            for(int c = 0;c<node.children().length();c++) {
                Node child = node.children()[c];
                for(int s = 0;s<child.scopes().length();s++) {
                    Node scope = child.scopes().get(s);
                    if(scope.owner().idx==child.idx) {
                        carry = distribute_node(scope,label,carry,0);
                    }
                }
                carry = distribute_node_children(child, label, carry);
            }
            return carry;
        }

        Node distribute_node(Node node, const std::string& label, Node carry, int inital_hoist) {
            if(inital_hoist!=0) {
                Node climb = node;
                for(int i = 0; i < inital_hoist && is_live(climb.owner()); i++) {
                    climb = climb.owner().in_scope();
                }
                return distribute_node(climb, label, carry, 0);
            }

            if(node.node_table().hasKey(label)) {
                Node table_node = node.node_table().get(label);
                if(table_node.name().length()==0) {
                    table_node.copy(carry);
                    carry = table_node;
                }
            } else {
                node.node_table().put(label, carry);
            }

            for(int c = 0;c<node.children().length();c++) {
                Node child = node.children()[c];
                if(!child.scopes().empty()) {
                    for(int s = 0;s<node.children()[c].scopes().length();s++) {
                        Node scope = child.scopes().get(s);
                        if(scope.owner().idx==child.idx) {
                            carry = distribute_node(scope,label,carry,0);
                        }
                    }
                }
                carry = distribute_node_children(child,label,carry);
            }
            return carry;
        }
 
        //Logging for the deep copy node
        #define LOG_DCN 0
        #if LOG_DCN
            #define UDCN(x) x
        #else
            #define UDCN(x)
        #endif

        //Make this cleaner later, probably when I do the normalization update and get more equipment
        //Experiment with a version that doesn't copy *evrything*, this is the biggest performance problem  right now in TwigSnap
        void deep_copy_node(Node n, Node o, map<uint32_t,Value>& value_alias_table, map<uint32_t,Node>& node_alias_table) {
            UDCN(uspan->newline("Deep copying "+node_info(o));)
            UDCN(uspan->newline("Initial fields");)
            n.type(o.type());
            n.sub_type(o.sub_type());
            n.name(o.name().to_std());
            n.x(o.x());
            n.y(o.y());
            n.z(o.z());
            n.mute(o.mute());
            n.resolved(o.resolved());
            UDCN(uspan->endline();)
            UDCN(uspan->newline("Copy children");)
            n.children().clear();
            for(int i = 0; i < o.children().length(); i++) {
                if(n.type()==equals_id&&i==0&&o.children()[0].in_scope()!=o.children()[1].in_scope()) {
                    n.children() << o.children()[i]; //For function calls, don't deep copy the left argumnet.
                    //This again needs to be fxied up as *local* function calls do need this copy for aliasing
                    //Add to list of things to fix when normalization rolls around
                } else {
                    Node newc = make_node();
                    deep_copy_node(newc, o.children()[i], value_alias_table, node_alias_table);
                    n.children() << newc;
                }
            }
            UDCN(uspan->endline();)
            UDCN(uspan->newline("Copy quals");)
            n.quals().clear();
            for(int i = 0; i < o.quals().length(); i++) {
                if(o.quals()[i].mute()) {
                    n.quals() << o.quals()[i];
                } else {
                    Node newq = make_node();
                    deep_copy_node(newq, o.quals()[i], value_alias_table, node_alias_table);
                    n.quals() << newq;
                }
            }
            UDCN(uspan->endline();)
            UDCN(uspan->newline("Copy value");)
            if(value_alias_table.hasKey(o.value().idx)) {
                Value aliased = value_alias_table.get(o.value().idx);
                n.value(aliased);
            } else {
                if(is_live(o.value())) {
                    if(!o.has_qual(global_qual)) {
                        if(!is_live(n.value())) {
                            n.value(make_value());
                        }
                        deep_copy_value(n.value(),o.value());
                    } else {
                        n.value(o.value());
                    }
                }
            }
            UDCN(uspan->endline();)
            UDCN(uspan->newline("Copy scopes");)
            n.scopes().clear();
            for(int i = 0; i < o.scopes().length(); i++) {
                //print("Deciding to alias scope ",o.scopes()[i].idx);
                //print(node_to_string(o));
                if(node_alias_table.hasKey(o.scopes()[i].idx)) {
                    Node aliased = node_alias_table.get(o.scopes()[i].idx);
                    //print("Aliasing as ",aliased.idx);
                    n.scopes() << aliased;
                } else if(o.scopes()[i].owner()==o) {
                    Node news = make_node();
                    n.scopes() << news;
                    //print("Deep copying as ",news.idx);
                    if(n.type()==func_decl_id) {
                        n.value().type_scope(n.scopes()[i]);
                        n.scopes()[i].owner(n);
                        value_alias_table.put(o.value().idx, n.value());
                        node_alias_table.put(o.scopes()[i].idx, n.scopes()[i]);
                        //print("Put ",o.scopes()[i].idx," node alias for : ",node_info(n.scopes()[i]));
                    }
                    deep_copy_node(news, o.scopes()[i], value_alias_table, node_alias_table);
                    news.owner(n);
                } else {
                    //print("Leaving untouched");
                    n.scopes() << o.scopes()[i];
                }
            }
            UDCN(uspan->endline();)
            UDCN(uspan->newline("Resolve tables");)
            resolve_to_col(n).qset(node_value_table_offset,
                resolve_to_col(o).qget(node_value_table_offset), sizeof(Ptr));
            resolve_to_col(n).qset(node_node_table_offset,
                resolve_to_col(o).qget(node_node_table_offset), sizeof(Ptr));
        
            n.parent(o.parent_ptr());
            n.owner(o.owner_ptr());
            n.in_scope(o.in_scope_ptr());
            n.opt_str() = o.opt_str().to_std();

            if(n.type()==var_decl_id) {
                value_alias_table.put(o.value().idx, n.value());
            } 
            UDCN(uspan->endline();)
            UDCN(uspan->endline();)
        }
        

        Node value_to_qual(Value val, std::string name = "", float x = -1.0f, float y = -1.0f) {
            Node to_return = make_node(val.type(),val.sub_type(),name,x,y,0.0f,val);
            return to_return;
        }

        map<uint32_t,int> left_binding_power;
        map<uint32_t,int> right_binding_power;
        void set_binding_powers(uint32_t id, int lbp, int rbp) {
            left_binding_power[id] = lbp;
            right_binding_power[id] = rbp;
        }

        map<char,bool> registered_opperators;
        list<uint32_t>  registered_opperator_ids;
        size_t add_binary_operator(char c, const std::string& f, int lbp, int rbp, int use_id = -1) {
            size_t id = use_id;
            if(id==-1) {
                id = add_token(c,f);
            }
            set_binding_powers(id,lbp,rbp);
            registered_opperators[c] = true;
            registered_opperator_ids << id;
            size_t decl_id = reg_id(f+"_decl");
            size_t unary_id = reg_id(f+"_unary");

            Handler handler = [this,decl_id,unary_id,c](Context& ctx){
                node_col children = ctx.node().children();
                standard_sub_process(ctx); //This causes us to double distribute because if the left term becomes a var decl from a user defined type it distirbutes itself, we don't overwritte though so its just wasted compute, not a bug
                
                ctx.node().quals() << copy_as_token(ctx.node());
                ctx.node().x(-1.0f); ctx.node().y(-1.0f);
                
                if(children.length() == 2) {
                    Node type_term = children[0];
                    Node id_term = children[1];

                    ctx.node().name(type_term.name().to_std()+c+id_term.name().to_std());
                    //May need to commit the decls as tokens, check the stamp later when it isn't almost midnight
                    if(type_term.type()==var_decl_id||(is_live(type_term.value())&&type_term.value().type()==type_term.type())) {
                        ctx.node().type(decl_id);
                        ctx.node().value(make_value());
                        ctx.node().value().copy(type_term.value(),true);
                        ctx.node().value().quals().push(value_to_qual(type_term.value()));
                        ctx.node().name(id_term.name().to_std());
                        if(id_term.value().type()==0) { //If this is a decleration
                            ctx.node().value().sub_type(0);
                            ctx.node().value(distribute_value(ctx.node().in_scope(), ctx.node().name().to_std(),ctx.node().value(),ctx.node().count_qual(hoisted_id)));
                            ctx.node().children().clear();
                        } else { //If this is a reinterpret like for a cast
                            ctx.node().children().removeAt(0);
                        }
    
                        
                    }
                } else if(children.length() == 1) {
                    Node type_term = children[0];
                    ctx.node().name(c+type_term.name().to_std());
                    ctx.node().type(unary_id);
                    if(is_live(type_term.value())) {
                        if(!is_live(ctx.node().value())) ctx.node().value(make_value());
                        ctx.node().value().copy(type_term.value(),true);
                    }
                } 
            };
            t_handlers[id] = handler;
            t_handlers[unary_id] = handler;

            Handler shandler = [this](Context& ctx){
                if(!ctx.node().children().empty()) {
                    standard_direct_pass(ctx.node());
                }

                if(ctx.index()+1>=ctx.result().length()) {
                    return;
                }

                Node right = ctx.result()[ctx.index()+1];
                if(right.type()==lbrace_id) {
                    if(!ctx.node().children().empty()) {
                        ctx.node().children().last().children() << ctx.result().take(ctx.index()+1);
                    } else {
                        ctx.node().children() << ctx.result().take(ctx.index()+1);
                    }
                }                
            };
            s_handlers[id] = shandler;
            s_handlers[unary_id] = shandler;
    
            return id;
        }

        size_t register_binary_operator(int use_id, int lbp, int rbp) {
            return add_binary_operator(' ',labels[use_id],lbp,rbp,use_id);
        }

        size_t plus_id = add_binary_operator('+',"PLUS", 4, 6);
        size_t dash_id = add_binary_operator('-',"DASH", 4, 5);
        size_t rangle_id = add_binary_operator('>',"RANGLE", 2, 3);
        size_t langle_id = add_binary_operator('<',"LANGLE", 2, 3);
        size_t bang_id = add_binary_operator('!',"BANG", 2, 3);
        size_t equals_id = add_binary_operator('=', "EQUALS", 1, 1);
        size_t star_id = add_binary_operator('*',"STAR", 5, 7);
        size_t slash_id = add_binary_operator('/',"SLASH", 4, 5);
        size_t caret_id = add_binary_operator('^',"CARET", 8, 4);
        size_t amp_id = add_binary_operator('&',"AMPERSAND", 4, 8);
        size_t dot_id = add_binary_operator('.', "DOT", 8, 9);
        size_t pipe_id = add_binary_operator('|', "PIPE", 9, 8);
        uint32_t qmark_id = add_binary_operator('?',"QMARK",1,3);
        uint32_t property_id = add_binary_operator(':',"COLON",5,6);
        uint32_t hash_id = add_binary_operator('#',"HASH",5,6);

        uint32_t  add_binding_token_combo(const std::string& f, int lbp, int rbp, char a, char b, char c = '\0', char d = '\0') {
            uint32_t id = add_token_combo(f,a,b,c,d);
            set_binding_powers(id,lbp,rbp);
            Handler shandler = [this](Context& ctx){ //This is nessecary for closures
                if(!ctx.node().children().empty()) {
                    standard_direct_pass(ctx.node());
                }

                if(ctx.index()+1>=ctx.result().length()) {
                    return;
                }

                Node right = ctx.result()[ctx.index()+1];
                if(right.type()==lbrace_id) {
                    if(!ctx.node().children().empty()) {
                        ctx.node().children().last().children() << ctx.result().take(ctx.index()+1);
                    } else {
                        ctx.node().children() << ctx.result().take(ctx.index()+1);
                    }
                }                
            };
            s_handlers[id] = shandler;
            return id;
        }

        uint32_t plus_plus_id = add_binding_token_combo("PLUS_PLUS",2,-1,'+','+');
        uint32_t plus_plus_plus_id = add_binding_token_combo("PLUS_PLUS_PLUS",2,-1,'+','+','+');
        uint32_t plus_equals_plus_id = add_binding_token_combo("PLUS_EQUALS_PLUS",2,-1,'+','=','+');
        uint32_t plus_equals_id = add_binding_token_combo("PLUS_EQUALS",2,3,'+','=');

        uint32_t langle_langle_id = add_binding_token_combo("LANGLE_LANGLE",8,9,'<','<');
        uint32_t rangle_rangle_id = add_binding_token_combo("RANGLE_RANGLE",8,9,'>','>');

        uint32_t equals_equals_id =  add_binding_token_combo("EQUALS_EQUALS",2,3,'=','=');
        uint32_t bang_equals_id =  add_binding_token_combo("BANG_EQUALS",2,3,'!','=');
        uint32_t langle_equals_id =  add_binding_token_combo("LANGLE_EQUALS",2,3,'<','=');
        uint32_t rangle_equals_id =  add_binding_token_combo("RANGLE__EQUALS",2,3,'>','=');
        uint32_t amp_amp_id =  add_binding_token_combo("AMP_AMP",1,1,'&','&');
        uint32_t pipe_pipe_id =  add_binding_token_combo("PIPE_PIPE",1,1,'|','|');

        uint32_t random_combo_id = add_token_combo("RANDOM",'|','*','^','+');

        uint32_t arguments_id = reg_id("ARGUMENTS"); //For function calls

        void init_stage_a() {
            discard_types.push_if_absent(undefined_id);
            discard_types.push_if_absent(end_id);
            discard_types.push_if_absent(lparen_id);
            discard_types.push_if_absent(rparen_id);
            discard_types.push_if_absent(lbrace_id);
            discard_types.push_if_absent(rbrace_id);
            discard_types.push_if_absent(lbracket_id);
            discard_types.push_if_absent(rbracket_id);
            //discard_types.push_if_absent(rbrace_id);
            discard_types.push_if_absent(comma_id);

            discard_types.push_if_absent(return_id);

            registered_opperators['['] = true; //Does this belong here? Possibly not, possibly yes
            registered_opperator_ids.push_if_absent(lbracket_id);
            //It's so we can overload on lbrackets

            a_handlers.default_function = [this](Context& ctx) {
                int left_bp = left_binding_power.getOrDefault(ctx.node().type(), -1);
                int right_bp = right_binding_power.getOrDefault(ctx.node().type(), -1);

                if(left_bp == -1 && right_bp == -1) return;
                
                if(is_live(ctx.left()) && left_bp > 0 && !discard_types.has(ctx.left().type())) {
                    int left_left_bp = left_binding_power.getOrDefault(ctx.left().type(), -1);
                    int left_right_bp = right_binding_power.getOrDefault(ctx.left().type(), -1);
    
                    bool right_associative = right_bp < left_bp; //lbp > rbp means right assoc
                    bool should_steal = left_bp > (right_associative ? left_right_bp : left_left_bp);

                    //LOG_W(ctx,"ON "+ctx.node().name().to_std());
                    
                    if(!ctx.left().children().empty()) {
                        if(ctx.left().children().length()==1) {
                            should_steal = true;
                        }
                        else if(discard_types.has(ctx.left().children().last().type())) {
                            goto otter;
                        }
                    }
    
                    //LOG_W(ctx,"SHOULD STEAL "+std::to_string((int)should_steal));
                    if(left_right_bp!=-1 && should_steal) {
                        if(ctx.left().children().length()>1) {
                            //LOG_W(ctx,"TAKING: "+node_info(ctx.left().children().last()));
                            ctx.node().children() << ctx.left().children().pop();
                            //LOG_W(ctx,"TOOK: "+node_info(ctx.node().children().last()));
                            if(ERROR_FLAG) return;
                        }
                        ctx.left().children() << ctx.result().take(ctx.index());
                    } else {
                        ctx.node().children() << ctx.left();
                        ctx.result().removeAt(ctx.index() - 1);
                    }
                } else {
                    otter:
                    if(!discard_types.has(ctx.node().type())) {
                        if(ctx.node().name().length()==1) { //Only single char opperators can be made unary like this
                            ctx.node().type(to_unary_id(ctx.node().type()));
                        }
                    }
                    ctx.index()++;
                }

                //LOG_W(ctx,"MIDDLE "+ctx.node().name().to_std());
                
                if(right_bp != -1 && ctx.index() < ctx.result().length()) {
                    Node next = ctx.result().get(ctx.index());
                    int next_lbp = left_binding_power.getOrDefault(next.type(), -1);
                    if(next_lbp == -1 && !discard_types.has(next.type())) { //It's an atom so we grab it
                        ctx.node().children() << ctx.result().take(ctx.index());
                    } 
                }
                ctx.index()--;

                //LOG_W(ctx,"AFTER "+ctx.node().name().to_std());
            };
    
            for(int m = 0; m<2; m++) {
                uint32_t open_id = m==0?rparen_id:rbracket_id;
                uint32_t close_id = m==0?lparen_id:lbracket_id;

                left_binding_power.put(close_id,10);

                a_handlers[open_id] = [this,close_id](Context& ctx) {
                    ctx.result().removeAt(ctx.index());
                    int i = ctx.index()-1;
                    list<Node> gathered;
                    while(i>=0) {
                        Node on = ctx.result().get(i);
                        node_col on_from = ctx.result();
                        while(!on.children().empty()&&on.type()!=close_id) {
                            on_from = on.children();
                            on = on.children().last();
                        }
                        Node on_left = deadptr; //This logic was added just to handle the lambda arguments case, it's subject to future correction as nessecary
                        node_col left_from = ctx.result();
                        if(i>0) {
                            on_left = ctx.result().get(i-1);
                            while(!on_left.children().empty()&&on_left.type()!=lbracket_id&&on_left.type()!=lparen_id) {
                                left_from = on_left.children();
                                on_left = on_left.children().last();
                            }
                        }
                        if(on.type()==close_id) {
                            gathered.reverse();

                            if(close_id!=lparen_id||(ctx.result().length()<i+1&&ctx.result().get(i+1).type()!=rbracket_id)) { //So we can call functions inside brackets like arr[stoi(s)];
                                if(is_live(on_left)&&on_left.type()==lbracket_id) { //For lambda arguments and lambda calls from indexed lists like arr[i](args)
                                    for(auto g : gathered) on.children() << g;
                                    if(close_id==lparen_id) {
                                        on.type(arguments_id);
                                        on_left.children().push(on_from==ctx.result()?ctx.result().take(i):on_from.pop());
                                        ctx.index(i+(on_from==ctx.result()?1:0)); //I'm not sure if this is the right index or not, it should be noticble if it causes issues though
                                    } else if(close_id==lbracket_id) { //For nested arrays like arr[0][1][2]
                                        on.children().insert(0, left_from==ctx.result()?ctx.result().take(i-1):left_from.pop());
                                        ctx.index(i-1);
                                    }
                                    break;
                                }
                            }


                            bool was_given_children = false;
                            if(on.children().empty()) {
                                for(auto g : gathered)
                                    on.children() << g;
                                was_given_children = true;
                            }
                            if(close_id!=lbracket_id) { //Brackets remain after a gather
                                Node token_on = copy_as_token(on);

                                if(!on.children().empty()) {
                                    on.copy(on.children().take(0));
                                    if(was_given_children) {
                                        for(int g=1;g<gathered.length();g++) {
                                            on.children() << gathered[g];
                                        }
                                    }
                                }

                                on.quals() << token_on; //Copy the lparen
                            }
                            on.quals() << turn_into_token(ctx.node()); //Copy the rparen

                            if(!was_given_children) {
                                if(on.children().empty()||close_id==lbracket_id) {
                                    for(auto g : gathered)
                                        on.children() << g;
                                } else { //This case if for things like int main(int a), where we want the gathered to go under main, not int
                                    for(auto g : gathered)
                                        on.children().last().children() << g;
                                }
                            }
                            ctx.index(i);
                            break;
                        } else {
                            gathered << ctx.result().take(i);
                            i--;
                        }
                    }
                    if(i < 0) {
                        ctx.result().push(ctx.node()); //Return the rparen to carry the error
                        print(red("rparen:a_handler unmatched closing paren"));
                    }
                };
            }
    
            a_handlers[identifier_id] = [this](Context& ctx){
                if(is_live(ctx.left()) && ctx.left().type() == identifier_id) {
                    while(ctx.index() < ctx.result().length() && ctx.result().get(ctx.index()).type() == identifier_id) {
                        ctx.left().children() << ctx.result().take(ctx.index());
                    }
                    ctx.index()--;
                } 
            };
        }

        uint32_t scope_id = reg_id("scope");
        uint32_t type_scope_id = reg_id("type_scope");


        void place_node_in_scope(Node node, Node insc) {
            node.in_scope(insc);
            for(int i=0;i<node.children().length();i++) {
                place_node_in_scope(node.children()[i],insc);
            }
        }

        void init_stage_s() {
            s_handlers[rbrace_id] = [this](Context& ctx){
                ctx.result().removeAt(ctx.index());
                int i = ctx.index()-1;
                list<Node> gathered;
                while(i>=0) {
                    Node on = ctx.result().get(i);
                    Node was_on = ctx.root(); //Storing the root for cases where we want to notify once children are gathered
                    while(!on.children().empty()&&on.type()!=lbrace_id) {
                        was_on = on;
                        on = on.children().last(); //Descend to the found lbrace
                    }
                    if(on.type()!=lbrace_id&&!on.scopes().empty()) {
                        for(int i=0;i<on.scopes().length();i++) {
                            on = on.scopes()[i];
                            while(!on.children().empty()&&on.type()!=lbrace_id) {
                                was_on = on;
                                on = on.children().last(); //Descend to the found lbrace
                            }
                            if(on.type()==lbrace_id) break;
                        }
                    }
                    if(on.type()==lbrace_id) {
                        on.quals().push(copy_as_token(on));
                        on.quals().push(turn_into_token(ctx.node()));
                        on.type(scope_id); //Turn it into a scope and hand over the contents
                        on.x(-1.0f); on.y(-1.0f);
                        gathered.reverse();
                        for(auto g : gathered) on.children() << g;

                        for(int c=0;c<was_on.children().length();c++) { //Promote to a scope
                            if(was_on.children()[c].idx==on.idx) {
                                Node newscope = was_on.children().take(c);
                                was_on.scopes() << newscope;
                                newscope.owner(was_on);
                                for(int n=0;n<newscope.children().length();n++){
                                   place_node_in_scope(newscope.children()[n],newscope);
                                }
                                break;
                            }
                        }
                        ctx.index(i);
                        break;
                    } else {
                        gathered << ctx.result().take(i);
                        i--;
                    }
                }
                if(i < 0) {
                    ctx.result().push(ctx.node()); //Return the rbrace to carry the error
                    print(red("rbrace:s_handler unmatched closing brace"));
                }
            };
            s_handlers[lbrace_id] = [this](Context& ctx){}; //Do nothing

            s_handlers.default_function = [this](Context& ctx){
                if(!ctx.node().children().empty()) {
                    standard_direct_pass(ctx.node());
                }
                
                if(ctx.index()+1>=ctx.result().length()) return;

                Node right = ctx.result()[ctx.index()+1];
                if(right.type()==lbrace_id) {
                    ctx.node().children() << ctx.result().take(ctx.index()+1);
                }

            };
        }

    void resolve_node_literal(Context& ctx, void* val, uint32_t tag, uint32_t size) {
            standard_sub_process(ctx);
            ctx.node().type(literal_id);
            Value value = make_value(tag,size);
            // value.set(val);
            ctx.node().value(value);
        }

        void init_literals() {
            value_printers[object_id] = [this](Context& ctx) {ctx.source(Ptr_as_string(ctx.value().data_ptr()));};
            value_printers[ptr_id] = [this](Context& ctx) {Ptr p = *(Ptr*)ctx.value().get(); ctx.source(Ptr_to_string(p,p.cachelevel));};
            value_printers[float_id] = [](Context& ctx) {ctx.source(std::to_string(*(float*)ctx.value().get()));};
            value_printers[int_id] = [](Context& ctx) {void* p = ctx.value().get(); DEBUG_ONLY(if(ERROR_FLAG) {return;}) ctx.source(std::to_string(*(int*)p));};
            value_printers[char_id] = [](Context& ctx) {ctx.source(std::string(1,*(char*)ctx.value().get()));};
            value_printers[bool_id] = [](Context& ctx) {ctx.source((*(bool*)ctx.value().get()) ? "TRUE" : "FALSE");};
            value_printers[string_id] = [this](Context& ctx) {void* p = ctx.value().get(); DEBUG_ONLY(if(ERROR_FLAG) {return;}) ctx.source() = *(string*)p;};
            value_printers[node_id] = [this](Context& ctx) {ctx.source(node_to_string((Node&)(*(Ptr*)ctx.value().get())));};
            value_printers[value_id] = [this](Context& ctx) {ctx.source(value_info((Value&)(*(Ptr*)ctx.value().get())));};
            value_printers[context_id] = [this](Context& ctx) {Context context = (Context&)(*(Ptr*)ctx.value().get()); std::string src = "Source ptr of "+Ptr_as_string(context)+": "+Ptr_as_string(context.source_ptr()); ctx.source(src);};
                
            t_handlers[ptr_id] = [this](Context& ctx) {
                Ptr p = string_to_Ptr(ctx.node().name().to_std());
                resolve_node_literal(ctx,(void*)&p,ptr_id,sizeof(Ptr));
            }; 

            t_handlers[float_id] = [this](Context& ctx) {
                float stof = std::stof(ctx.node().name().to_std());
                resolve_node_literal(ctx,(void*)&stof,float_id,4);
            }; 
    
            t_handlers[int_id] = [this](Context& ctx) {
                int stoi = std::stoi(ctx.node().name().to_std());
                resolve_node_literal(ctx,(void*)&stoi,int_id,4);
            }; 
    
            t_handlers[bool_id] = [this](Context& ctx) {
                bool stob = ctx.node().name().to_std() == "true" ? true : false;
                resolve_node_literal(ctx,(void*)&stob,bool_id,1);
            }; 

            t_handlers[char_id] = [this](Context& ctx) {
                char stob = ctx.node().name()[0];
                resolve_node_literal(ctx,(void*)&stob,char_id,1);
            }; 
    
            t_handlers[string_id] = [this](Context& ctx) {
                Ptr ptr = ctx.node().name_ptr();
                resolve_node_literal(ctx,(void*)&ptr,string_id,sizeof(Ptr));
            }; 
        }

        void resolve_identifier(Context& ctx) {
            Node node = ctx.node();
            
            Value decl_value = make_value();
            bool found_a_value = find_value_in_scope(ctx.node());
            // if(is_live(node.value())) decl_value = node.value();
            // else decl_value = make_value();
            bool is_qualifier = is_live(node.value()) && node.value().type()!=0 && node.value().sub_type() != 0; 
            //We count it as a qualifer if it has a fully valid value to stamp

            int root_idx = -1;
            if(is_qualifier) {
                if(is_live(ctx.node().value())) {
                    decl_value.quals() << value_to_qual(node.value(),node.name().to_std(),node.x(),node.y());
                }
                for(int i = 0; i < node.children().length(); i++) {
                    Node c = node.children()[i];
                    find_value_in_scope(c); //Process forward and consume other qualifers
                    if(c.type()!=identifier_id) {break;}

                    if(is_live(c.value())&&c.value().type()!=0&&c.value().type()!=duck_id) {
                        decl_value.quals() << value_to_qual(c.value(),c.name().to_std(),c.x(),c.y());
                    } else {
                        root_idx = i;
                        break;
                    }
                }
                if(root_idx!=-1) {
                    Node root = node.children()[root_idx];
                    node.name(root.name().to_std());
                    node.x(root.x());
                    node.y(root.y());
                    for(int i = root_idx+1; i < node.children().length(); i++) {
                        Node c = node.children()[i];
                        find_value_in_scope(c);
                        if(is_live(c.value())&&c.value().type()!=0&&c.value().type()!=duck_id) {
                            node.quals() << value_to_qual(c.value(),c.name().to_std(),c.x(),c.y());
                        } 
                    }
                    node.children(node.children().take(root_idx).children());
                }
            }
            
            if(node.scopes().empty()) { //Defer, the r_stage will do this later for scoped nodes
                standard_sub_process(ctx);
            }

            //For builtin functions and such
            if(keywords.hasKey(node.name().to_std())) {
                if(node.value().sub_type()!=0) {
                    node.type(node.value().sub_type());
                    node.value().sub_type(0);
                    return;
                }
            }

            // recycle_value(node.value()); //Figure out how to deal with lifetimes like this later
            node.value(decl_value);

            fire_quals(ctx, decl_value);

            bool has_scope = !node.scopes().empty();
            bool has_type_scope = is_live(node.value().type_scope());
            bool has_sub_type = node.value().sub_type() != 0;
            
            if(has_scope) {
                node.scopes()[0].owner(node);
                node.scopes()[0].name(node.name().to_std());
                if(has_sub_type) {
                    node.type(func_decl_id);
                    node.scopes()[0] = distribute_node(node.in_scope(),node.name().to_std(),node.scopes()[0],node.count_qual(hoisted_id));
                    node.value().type_scope(node.scopes()[0]);
                    node.value().sub_type(0);
                    node.value(distribute_value(node.in_scope(),node.name().to_std(),node.value(),node.count_qual(hoisted_id)));
                    if(node.in_scope().type()==type_scope_id) {
                        std::string nname = node.name().to_std();
                        bool has_opp = false;
                        for(auto c : nname) {if(registered_opperators.getOrDefault(c,false)) {has_opp = true; break;}}
                        if(!has_opp) {nname=".\""+nname+"\"";} 
                        overload_type(node.in_scope().owner().value().type(),nname,method_call_id,node.value());

                        Node star = make_node(star_id);
                        Node type_term = make_node(identifier_id,node.in_scope().owner().name().to_std(),deadptr,node.scopes()[0]);
                        Node id_term = make_node(identifier_id,"this",deadptr,node.scopes()[0]);
                        star.children().push(type_term);
                        star.children().push(id_term);
                        node.children().insert(0,star);
                    }
                    for(int c=0;c<node.children().length();c++) {
                        place_node_in_scope(node.children()[c],node.scopes()[0]);
                    }
                } else {
                    node.type(type_decl_id);
                    node.value(make_type_value(node.name().to_std(),0));
                    node.value().type_scope(node.scopes()[0]);
                    node.value(distribute_value(node.in_scope(),node.name().to_std(),node.value(), node.count_qual(hoisted_id)));
                    node.scopes()[0].type(type_scope_id);
                    add_template(node.value().type());
                    r_handlers[to_prefix_id(node.value().type())] = [this](Context& ctx){
                        if(ctx.value().size()==0&&layouts.hasKey(ctx.value().type())) {
                            ctx.value().size(layouts.get(ctx.value().type()).total_size);
                        }
                    };
                    x_handlers[to_prefix_id(node.value().type())] = [this](Context& ctx){
                        if(is_live(ctx.value())&&ctx.value().quals()[0]==ctx.qual()) {
                            ctx.value().data_col().push_default();
                            ctx.value().data_col().heterogenous = true;
                        }
                    }; 
                }
            } else {
                has_scope = find_node_in_scope(node); //To distinquish func_calls from object identifiers
                if(has_sub_type) {
                    node.type(var_decl_id);
                    //Shadowing will need a pass later, better coordination with how distirbute value returns and such.
                    //A noted other issue beyond the duck typing collisions is that for loops are polluting sibling scopes
                    if(find_value_in_scope(node)) { //If this node was duck typed
                        if(node.value().type()==duck_id) {
                            node.value().copy(decl_value,false);
                        }   
                    } else {
                        if(node.in_scope().type()==type_scope_id) {
                            node.in_scope().value_table().put(node.name().to_std(), decl_value); //So we don't distribute into function bodies, we need to alias later via this, as it's per instance
                            layouts[node.in_scope().owner().value().type()].add_prop(node.value().type(),node.value().size(),node.name().to_std(),0,0,decl_value);
                        } else {
                            node.value(distribute_value(node.in_scope(), node.name().to_std(), decl_value, node.count_qual(hoisted_id)));
                        }
                    }
                    node.value().sub_type(0);
                } else if(has_scope) {
                    node.type(func_call_id);
                    find_value_in_scope(node); //Retrive our return value (could probably just do 'found_a_value' skips decl set...)
                    if(is_live(node.value().type_scope()))
                        node.scopes()[0] = node.value().type_scope(); //Swap to the type scope
                    // if(!node->children.empty()) {
                    //     node->name.append("(");
                    //     for(auto c : node->children) {node->name.append(c->name+(c!=node->children.last()?",":")"));}
                    // }
                } else if(found_a_value) { //If we already had a value and nothing interesting happened to us, reclaim it
                    find_value_in_scope(node);
                    if(node.value().type()==function_id) {
                        node.type(func_call_id);
                    }
                } else {                                   
                    if(node.in_scope().value_table().hasKey("this")) { //The has check is so we don't inject this on the names of declared variables at the top
                        bool children_has_node = false;
                        for(int i=0;i<node.in_scope().children().length();i++){
                            if(node.in_scope().children()==node) {children_has_node=true; break;}
                        }

                        if(node.in_scope().owner().type()==func_decl_id&&!children_has_node) { //These are arguments in the function decleration, the children_has_node check is becuase the args are in the scope of the decleration but not actually in it's children list
                            
                        } else {
                            Node climb = node;
                            while(is_live(climb)&&climb.type()!=func_decl_id) { 
                                climb = climb.in_scope().owner();
                            }
                            if(is_live(climb)&&climb.type()==func_decl_id) { //Checking if this is a member of the method or not
                                if(climb.in_scope().value_table().hasKey(node.name().to_std())) {
                                    //node.value().copy(climb.in_scope().value_table().get(node.name().to_std()),true);

                                    Node accessor = make_node(dot_id);
                                    place_node_in_scope(accessor,node.in_scope());
                                    Node star = make_node(star_id);
                                    Node this_node = make_node(identifier_id,"this",node.in_scope().value_table().get("this"),node.in_scope());
                                    star.children().push(this_node);
                                    accessor.children().push(star);
                                    accessor.children().push(node);
                                    ctx.result().removeAt(ctx.index()); ctx.result().insert(ctx.index(),accessor); 
                                    process_node(ctx,star); //Because this won't get processed again like the scope children do, it's up to us to resolve it here
                                } else {
                                    //Just a plain identifer, not a member
                                }
                            } else {
                               //It isn't in a method, something gave it this as a glitch probably
                            }
                        }
                    } else {
                        //No clue what this could be, duck type it
                        if(ctx.root().type()==equals_id&&ctx.root().children().length()>1&&ctx.root().children()[1]==node) {
                            //print(yellow("DUCK EQUALS: "),node_to_string(ctx.root()));
                            node.type(var_decl_id);
                            decl_value.type(duck_id);
                            node.value(distribute_value(node.in_scope(), node.name().to_std(), decl_value, node.count_qual(hoisted_id)));
                        } else {
                            //print(magenta("NO  DUCK EQUALS: "),node_info(node));
                        }
                    }
                }
            }
        }

        bool is_node_opperator(Node n) {
            for(int i=0;i<n.name().length();i++) {
                if(registered_opperators[n.name().at(i)]) return true;
            }
            return false;
        }


        void overload_type(uint32_t type, const std::string& instr, uint32_t overload_to, Value value = deadptr) {
            if(!layouts.hasKey(type)) {
                layouts.put(type,_layout(add_template(type)));
            }
            Node expr = tokenize(instr);
            Stage* old_stage = active_stage;
            start_stage(a_handlers);
            standard_direct_pass(expr);
            a_pass_resolve_keywords(expr.children());
            start_stage(old_stage);

            // print(node_to_string(expr));

            uint32_t root_type = 0; 
            uint32_t right_type = 0;
            if(!expr.children().empty()) {
                Node op = expr.children()[0];
                root_type = op.type();
                if(op.name().length()==1&&instr.length()>1&&instr.find(op.name().to_std())==0&&op.type()!=lbracket_id) {
                    root_type-=2; //Convert to normal version if it's on the left side, so +string parses as plus_unary, but is actually just normal plus
                    //Only single char ops can be unary form so token combos don't need this
                }

                if(!op.children().empty()) {
                    if(!is_live(op.children()[0].value())) {
                        right_type = op.children()[0].type();
                        if(right_type==string_id) {
                            right_type = hashString(op.children()[0].name().to_std());
                        }
                    } else {
                        right_type = op.children()[0].value().type();
                    }
                }
            }
            layouts.get(type).add_overload(make_overload_key(root_type,right_type),overload_to,value);
            recycle_node(expr);
        }
        uint32_t overload_type(uint32_t type, const std::string& instr, const std::string& f, Value value = deadptr) {
            uint32_t id = reg_id(f);
            overload_type(type,instr,id,value);
            return id;
        }

        uint32_t overload_type(uint32_t type, const std::string& instr, const std::string& f, Value value, Handler xhandler) {
            uint32_t id = reg_id(f);
            overload_type(type,instr,id,value);
            x_handlers[id] = xhandler;
            return id;
        }

        void what_I_see(Context& ctx) {
            print(bold_str(ctx.node().name().to_std()),": I see my root is ",green(ctx.root().name().to_std()),", my value type is ",blue(labels[ctx.node().value().type()])," and to my left is ",yellow(is_live(ctx.left())?ctx.left().name().to_std():"nothing"));
        }

        void resolve_overload(Context& ctx) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to resolve overloads while another error was flagged")); return;})
            if(!is_node_opperator(ctx.root())) return;
            //LOG_W(ctx," resolving overloads");
            // print("RESOLVING: ",node_to_string(ctx.node()));
            standard_sub_process(ctx); //Consider not doing this
            if(ctx.index()==0&&is_live(ctx.node().value())) { //If we're the left term
                if(layouts.hasKey(ctx.node().value().type())) {
                    _layout& l = layouts[ctx.node().value().type()];
                    uint32_t right_type = 0;
                    bool has_overload = false;
                    uint64_t typekey = 0;
                    if(ctx.result().length()>1) {
                        ctx.index() = 1; //Because process node will just blindly carry index
                        process_node(ctx,ctx.result().get(1));
                        ctx.index() = 0;
                        if(is_live(ctx.result().get(1).value())) {
                            //print("Deriving value from right_type");
                            right_type = ctx.result().get(1).value().type();
                        } else {
                            log(yellow("resolve_overload: right term has a dead value: "),node_info(ctx.result().get(1)));
                            //print(span->on_line->parent->to_string());
                        }
                    } else {
                        typekey = make_overload_key(ctx.root().type(),0);
                        has_overload = l.has_overload(typekey);
                    }

                    if(right_type!=0) {
                        typekey = make_overload_key(ctx.root().type(),right_type);
                        has_overload = l.has_overload(typekey);
                        if(!has_overload) {
                            //print("False overload, checking any");
                            typekey = make_overload_key(ctx.root().type(),any_id);
                            has_overload = l.has_overload(typekey);
                        }
                    } else if(!has_overload) {
                        if(ctx.result().length()>1) {
                            right_type = hashString(ctx.result().get(1).name().to_std());
                            //print("Overloading name: ",ctx.result().get(1).name().to_std());
                        }
                        if(right_type!=0) {
                            typekey = make_overload_key(ctx.root().type(),right_type);
                            has_overload = l.has_overload(typekey);
                        }
                    }
                    //print("Has overload: ",has_overload?"Yes":"No");
                    if(has_overload) {
                        type_and_value tnv = l.get_overload(typekey);
                        ctx.root().type(tnv.type);
                        if(is_live(tnv.value)) {
                            Value& value = (Value&)tnv.value;
                            if((value.type()!=0)) {
                                ctx.root().value(make_value(value.type(),value.size(),value.address(),value.sub_type(),value.sub_size(),value.type_scope()));
                            } else {
                                if(ctx.node().value().sub_type()==0) {
                                    fire_quals(ctx,ctx.node().value());
                                } 

                                ctx.root().value(make_value(
                                    ctx.node().value().sub_type(),
                                    ctx.node().value().sub_size()
                                ));
                                if(ctx.node().value().quals().length()>1) {
                                    for(int i=1;i<ctx.node().value().quals().length();i++) {
                                        ctx.root().value().quals() << ctx.node().value().quals()[i];
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        uint32_t static_qual = add_qual("static");

        void sync_args(Context& ctx) {
            if(!ctx.node().scopes().empty()) {
                DEBUG_ONLY(if(ctx.node().children().length()!=ctx.node().scopes()[0].owner().children().length()) {throw_error("Wrong number of arguments for function: ",node_to_string(ctx.node())); return;})
                for(int i = 0; i < ctx.node().children().length(); i++) {
                    Node arg = ctx.node().children()[i];
                    if(arg.type()==equals_id) continue;
                    Node param = ctx.node().scopes()[0].owner().children()[i];
                    Node assignment = make_node(equals_id);
                    assignment.children().push(param);
                    assignment.children().push(arg);
                    ctx.node().children().col().set(i,(void*)&assignment);
                }
            }
        }

        void gather_all_values_in_scope(value_col& subvals, Node scope) {
            for(int i=0;i<scope.children().length();i++) {
                Node c = scope.children()[i];
                if(is_live(c.value())) {
                    for(int n=0;n<subvals.length();n++) {
                        if(subvals.get(n).idx==c.value().idx) goto skipatom;
                    }
                    subvals.push(c.value());
                }
                skipatom:
                gather_all_values_in_scope(subvals,c);
            }
            for(int s=0;s<scope.scopes().length();s++) {
                Node subscope = scope.scopes()[s];
                if(!is_live(subscope.owner())||subscope.owner().idx==scope.idx) {
                    gather_all_values_in_scope(subvals,subscope);
                }
            }
        }

        //Add another row to each data column for function calls
        int descend_call_scope(Context& ctx, Node scope) {
            Value sv = scope.value();
            int loc = sv.loc()+1;
            sv.loc(loc);
            value_col subvals = sv.sub_values();
            if(subvals.empty()) {gather_all_values_in_scope(subvals,scope);}
            for(int i=0;i<subvals.length();i++) {
                Value sval = subvals.get(i); //Static values just stay where they are, they don't descend and ascend
                if(is_live(sval.data_ptr())&&!sval.has_qual(static_qual)) { //This ceremony is becuse if we just did col.push(col.get(0) it would invalidate the column as we push thus breaking the get, so we have to save as temps
                    Ptr dataptr = sval.data_ptr();
                    if(dataptr.pool!=data_store_id) continue;
                    uint32_t elem_size = resolve_to_col(dataptr).element_size;
                    list<uint8_t> temp(elem_size);
                    if(resolve_to_col(dataptr).empty()) {
                        if(resolve_to_col(dataptr).element_size == 0) continue; //Skip uninitialized cols
                        resolve_to_col(dataptr).push_default();
                    }
                    memcpy(temp.data(), resolve_to_col(dataptr).get((uint32_t)0), elem_size);
                    if(resolve_to_col(dataptr).length() <= loc) {
                        //These shouldn't be getting out of sync in the first place, in the future investigate this deeper
                        int depth_check = 0;
                        //This could be due to us trying this on other func calls, perhaps replace loc with the return of the data ptr to make this work?
                        while(resolve_to_col(dataptr).length() <= loc && depth_check++ < 100) {
                            resolve_to_col(dataptr).push(temp.data());
                        }
                        if(depth_check>=90) {
                            print(red("Infinite loop in loc catchup on "+Ptr_as_string(dataptr)+": this shouldn't even be happening in the first place!"));
                        }
                    } else {
                        resolve_to_col(dataptr).set(loc, temp.data());
                    }
                    dataptr.sidx = loc;
                    resolve_to_col(sval).qset(value_data_offset,(void*)&dataptr,sizeof(Ptr));
                } else {
                    log(yellow(Ptr_as_string(sval)+" is not live, and can not be descended"));
                }
            }
            return loc;
        }

        void ascend_call_scope(Node scope) {
            Value sv = scope.value();
            int loc = sv.loc()-1;
            sv.loc(loc);
            value_col subvals = sv.sub_values();
            for(int i=0;i<subvals.length();i++) {
                Value sval = subvals.get(i);
                if(is_live(sval.data_ptr())&&!sval.has_qual(static_qual)) {
                    Ptr newptr = sval.data_ptr();
                    if(newptr.pool!=data_store_id) continue;
                    newptr.sidx = loc;
                    resolve_to_col(sval).qset(value_data_offset,(void*)&newptr,sizeof(Ptr));
                }
            }
        }

        void desync_args(Node root) {
            if(is_live(root.value().data_ptr())) {
                Ptr newptr = root.value().data_ptr();
                newptr.sidx = newptr.sidx-1;
                resolve_to_col(root.value()).qset(value_data_offset,(void*)&newptr,sizeof(Ptr));
            }
            for(int i=0;i<root.children().length();i++) {
                desync_args(root.children()[i]);
            }
        }
        void resync_args(Node root) {
            if(is_live(root.value().data_ptr())) {
                Ptr newptr = root.value().data_ptr();
                newptr.sidx = newptr.sidx+1;
                resolve_to_col(root.value()).qset(value_data_offset,(void*)&newptr,sizeof(Ptr));
            }
            for(int i=0;i<root.children().length();i++) {
                resync_args(root.children()[i]);
            }
        }

        void call_func(Context& ctx, Node scope) {
            list<list<uint8_t>> temps;
            for(int i=0;i<ctx.node().children().length();i++) {
                Node rightterm = ctx.node().children()[i].children()[1];
                process_node(ctx, rightterm);
                Value rv = rightterm.value();
                list<uint8_t> snap; snap.resize(rv.size());
                memcpy(snap.data(), rv.get(), rv.size());
                temps << snap;
            }
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("ABORTING FUNCTION CALL BEFORE DESCENT")); return;})
            //print("RUNNING: ",node_to_string(ctx.node()));
            int stack_depth = descend_call_scope(ctx,scope);
            DEBUG_ONLY(if(stack_depth>500) {throw_error("Stack overflow on function call: ",node_info(ctx.node())); return;})
            for(int i=0;i<ctx.node().children().length();i++) {
                Node leftterm = ctx.node().children()[i].children()[0];
                leftterm.value().set(temps[i].data());
            }
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("ABORTING FUNCTION CALL BEFORE PASS")); return;})
            if(!standard_travel_pass(scope,ctx.sub())) { //If the return didn't already ascend
                ascend_call_scope(scope);
            }
        }

        void assign(Value lv, Value rv) {
            if(rv.data_col().empty()) return; //No data to copy

            Ptr lp = lv.data_ptr();
            Ptr rp = rv.data_ptr();

            //print("Equals: ",node_to_string(ctx.node()));

            if(!is_live(lp)) return; //Normally caused by something being delcared but never used, and thus missed by the m pass
            DEBUG_ONLY(if(!is_live(rp)) {throw_error("right term of equals is invalid"); return;})

            Col& lcol = resolve_to_col(lp);

            uint32_t subtype = 0; uint32_t subsize = 0; uint32_t alias = ptr_id;

            Col& rcol = resolve_to_col(rp);
            if(rcol.tag==string_id) {subtype = char_id; subsize = 1; alias = string_id;}
            else if(rv.sub_type()!=0) {subtype = rv.sub_type(); subsize = rv.sub_size(); alias = rv.type();}
            // else if(rcol.tag==ptr_id&&!rcol.empty()) {
            //     //Alias through one of it's pointers to discern what's at that location
            //     print(node_to_string(ctx.node()));
            //     print("(Implment later) Checking column through: ",Ptr_as_string(*(Ptr*)rcol[0]));
            // }   

            if(lcol.tag==ptr_id&&rcol.tag==ptr_id) { //Figure out better proper Ptr assignment later
                //This is just a kludge for now because I'm testing normalization in TwigSnap 
                if(resolve_to_col(lp).heterogenous) {
                    resolve_to_col(lp).qset(lp.sidx,resolve_ptr(rp),rv.size());
                } else {
                    resolve_to_col(lp).set(lp.sidx,resolve_ptr(rp));
                }
                return;
            }

            Ptr subp = deadptr;
            if(lcol.tag==ptr_id||lcol.tag==string_id) {
                if(!lcol.empty()) {
                    subp = *(Ptr*)lcol.get(lp.sidx); //The Ptr currently stored to the other collection
                    if(subtype==0||subsize==0&&is_live(subp)) { //Free the subptr if we're realiasing to a scalar
                        //print("Recycling subp");
                        recycle_column(subp);
                    } else {
                        if(is_live(subp)) {
                            //print("Resetting subp");
                            Col& subcol = resolve_to_col(subp);
                            subcol.clear(); subcol.element_size = subsize; subcol.tag = subtype;
                        } else {
                            //print("Regnerating subp");
                            subp = get_ticket(lp.pool,subsize,subtype);
                            resolve_to_col(lp).set(lp.sidx,(void*)&subp);
                        }
                    }
                } else if(subtype!=0&&subsize!=0) {
                    //print("Replacing subp");
                    subp = get_ticket(lp.pool,subsize,subtype);
                    resolve_to_col(lp).push((void*)&subp);
                }
            }
            Col& col = resolve_to_col(lp); //Realias because the push may have invalidated it earlier
            if(subtype!=0&&subsize!=0) { //If right is a pointer to a collection
                if(col.tag!=alias) {
                    //print("Realiasing");
                    col.element_size = sizeof(Ptr); col.tag=alias;
                    lv.size(sizeof(Ptr)); lv.type(alias);
                    col.clear();
                    subp = get_ticket(lp.pool,subsize,subtype);
                    resolve_to_col(lp).push((void*)&subp);
                } else {
                    //print("Replacing");
                }
                Ptr dataptr  = *(Ptr*)rv.get();
                Col& datacol = resolve_to_col(dataptr); //Copy over the data to it's new position
                Col& subcol = resolve_to_col(subp);
                subcol.clear();
                for(int i=0;i<datacol.length();i++) {
                    subcol.push(datacol[i]);
                }
            } else { //If we're the direct value in the store pool
                if(col.element_size!=rv.size()||col.tag!=rv.type()||lv.type()!=rv.type()) {
                    // print("Clearing and pushing");
                    col.clear();
                    col.element_size = rv.size(); col.tag=rv.type();
                    lv.size(rv.size()); lv.type(rv.type());
                    col.push(rv.get());
                } else if(col.empty()) {
                    //print("Pushing");
                    col.push(rv.get());
                } else {
                    //print("Setting");
                    if(rv.data_ptr().sidx>=rv.data_col().length()) {
                        print(yellow("core:assign right value data pointer sub index is out of bounds: "),Ptr_as_string(rv.data_ptr()));
                    } else {
                        col.set(lp.sidx,rv.get());
                    }
                }
            }
        }

        void deep_copy_value(Value v, Value o) {
            v.copy(o,true); //This already does 90% of the work, all we're really doign here is marshaling ptr reallocation
            //assign(v,o); //This was causing problems, come back later and revise this.
        }


        Node instantiate_template_scope(Node call, Node decl, Context& ctx, bool args_already_synced = false) {
            Node new_scope = make_node(decl.scopes()[0].type(), 0, decl.name().to_std());

            if(is_live(decl.scopes()[0].value())) {
                new_scope.value(make_value());
                new_scope.value().copy(decl.scopes()[0].value(), true);
            }
            new_scope.owner(call);
        
            for(int i = 0; i < decl.scopes()[0].quals().length(); i++) {
                new_scope.quals() << decl.scopes()[0].quals()[i];
            }
        
            map<uint32_t, Value> value_alias_table;
            map<uint32_t, Node> node_alias_table;
        
            Stage* oldstage = active_stage;
            start_stage(x_handlers); //Because we're trying to derive the value, this may not be the right long term solution though
            //This was a bit of an accident born from how things were working in Webcorn's standard_gather_from_scope
            //And an anomaly with FUNC_DECLs revelead when trying to make templating work
            if(args_already_synced) { //Because they've already been turned into the assignment form by sync_args
                for(int i = 0; i < call.children().length(); i++) {
                    Node c = call.children()[i];
                    process_node(ctx, c.children()[1]);
                    Value newv = make_value();
                    deep_copy_value(newv,c.children()[1].value());
                    value_alias_table.put(c.children()[0].value().idx, newv);
                }
            } else {
                node_col decl_args = decl.children();
                for(int i=0;i<decl_args.length();i++) { //For lambdas and such where we have arguments within the children body
                    if(decl_args[i].type()==arguments_id){
                        for(int j=0;j<decl_args.length();j++) {
                            if(j!=i) {
                                Value newv = make_value();
                                deep_copy_value(newv,decl_args[j].value());
                                value_alias_table.put(decl_args[j].value().idx, newv);
                            }   
                        }
                        decl_args = decl_args[i].children(); 
                        break;
                    }
                } 
                for(int i = 0; i < call.children().length(); i++) { 
                    process_node(ctx, call.children()[i]);
                    if(i < decl_args.length()) {
                        value_alias_table.put(decl_args[i].value().idx, call.children()[i].value());
                    }
                }
            }
            active_stage = oldstage;
        
            for(int i = 0; i < decl.scopes()[0].children().length(); i++) {
                Node copy = make_node();
                copy.in_scope(new_scope);
                deep_copy_node(copy, decl.scopes()[0].children()[i], value_alias_table, node_alias_table);
                new_scope.children() << copy;
            }
        
            return new_scope;
        }

        Node instantiate_function(Node func, Context& ctx){
            Node new_scope = make_node(func.scopes()[0].type(), 0, func.name().to_std());
            if(is_live(func.scopes()[0].value())) {
                new_scope.value(make_value());
                new_scope.value().copy(func.scopes()[0].value(), true);
            }
            for(int i = 0; i < func.scopes()[0].quals().length(); i++) {
                new_scope.quals() << func.scopes()[0].quals()[i];
            }
        
            map<uint32_t, Value> value_alias_table;
            map<uint32_t, Node> node_alias_table;
            
            Node puppet = make_node(func_decl_id);
            puppet.value(func.value());
            if(!puppet.value().sub_values().empty()) {
                value_alias_table[puppet.value().sub_values()[0].idx] = puppet.value().sub_values()[0];
            }
            puppet.scopes() << new_scope;
            new_scope.owner(puppet);

            node_col decl_args = func.children();
            for(int i=0;i<decl_args.length();i++) { //For lambdas and such where we have arguments within the children body
                if(decl_args[i].type()==arguments_id){ //This is handeling the copying of the captures and finding the arguments
                    for(int j=0;j<decl_args.length();j++) {
                        if(j!=i) {
                            if(decl_args[j].type()==to_unary_id(amp_id)) {
                                if(!decl_args[j].children().empty()) {
                                    if(is_live(decl_args[j].children()[0].value())) {
                                        Value orig = decl_args[j].children()[0].value();
                                        value_alias_table[orig.idx] = orig;
                                    }
                                } else {
                                    value_col vtable = func.scopes()[0].value_table();
                                    for(int e=0;e<vtable.length();e++) {
                                        value_alias_table[vtable[e].idx] = vtable[e];
                                    }
                                }
                            } else if(is_live(decl_args[j].value())) {
                                Value newv = make_value();
                                deep_copy_value(newv,decl_args[j].value());
                                value_alias_table[decl_args[j].value().idx] = newv;
                            }
                        }   
                    }
                    decl_args = decl_args[i].children(); 
                    break;
                }
            } 
            if(decl_args!=func.children()) { //Copying the arguments in so that calls have a puppet to bind to
                for(int i=0;i<decl_args.length();i++) { 
                    Node copy = make_node();
                    deep_copy_node(copy,decl_args[i],value_alias_table,node_alias_table);
                    puppet.children() << copy;
                    value_alias_table[decl_args[i].value().idx] = puppet.children()[i].value();
                }
            }
        
            for(int i = 0; i < func.scopes()[0].children().length(); i++) {
                Node copy = make_node();
                copy.in_scope(new_scope);
                deep_copy_node(copy, func.scopes()[0].children()[i], value_alias_table, node_alias_table);
                new_scope.children() << copy;
            }
            return puppet;
        }

        void instantiate_template(Node call, Node decl, Context& ctx, bool args_already_synced = false) {
            Node new_scope = instantiate_template_scope(call, decl, ctx, args_already_synced);
            call.scopes_col().set(0, (void*)&new_scope);
            new_scope.owner(call);
        }

        // void instantiate_template(Node call, Node decl, Context& ctx, bool args_already_synced = false) {
        //     if(!call.scopes().empty()) {
        //         Node new_scope = make_node(decl.scopes()[0].type(), 0, decl.name().to_std());
        //         call.scopes_col().set(0,(void*)&new_scope);
                
        //         if(is_live(decl.scopes()[0].value())) {
        //             call.scopes()[0].value(make_value());
        //             call.scopes()[0].value().copy(decl.scopes()[0].value(),true);
        //         }
        //         call.scopes()[0].owner(call);
                
        //         for(int i = 0; i < decl.scopes()[0].quals().length(); i++) {
        //             call.scopes()[0].quals() << decl.scopes()[0].quals()[i];
        //         }

        //         map<uint32_t,Value> value_alias_table;
        //         map<uint32_t,Node> node_alias_table;
        
        //         Stage* oldstage = active_stage;
        //         start_stage(x_handlers); //Because we're trying to derive the value, this may not be the right long term solution though
        //         //This was a bit of an accident born from how things were working in Webcorn's standard_gather_from_scope
        //         //And an anomaly with FUNC_DECLs revelead when trying to make templating work
        //         if(args_already_synced) { //Because they've already been turned into the assignment form by sync_args
        //             for(int i = 0; i < call.children().length(); i++) {
        //                 Node c = call.children()[i];
        //                 process_node(ctx, c.children()[1]);
        //                 value_alias_table.put(c.children()[0].value().idx, c.children()[1].value());
        //             }
        //         } else {
        //             for(int i = 0; i < call.children().length(); i++) {
        //                 process_node(ctx, call.children()[i]);
        //                 if(i < decl.children().length()) {
        //                     value_alias_table.put(decl.children()[i].value().idx, call.children()[i].value());
        //                 }
        //             }
        //         }
        //         active_stage = oldstage;
        
        //         for(int i = 0; i < decl.scopes()[0].children().length(); i++) {
        //             Node copy = make_node();
        //             copy.in_scope(call.scopes()[0]);
        //             //print("Deep copying:\n",node_to_string(decl.scopes()[0].children()[i],2));
        //             deep_copy_node(copy, decl.scopes()[0].children()[i], value_alias_table, node_alias_table);
        //             //print("Deep copy done, copy:\n",node_to_string(copy,2));
        //             call.scopes()[0].children() << copy;
        //         }
        //     } else {
        //         print("CALL HAS NO SCOPE");
        //     }
        // }



        uint32_t add_function(const std::string& f, Handler x_handler, uint32_t size = 0, uint32_t return_type = 0) {
            Value val = register_value(f,size,return_type);
            keywords.put(f,val);
            uint32_t id = val.sub_type();
            if(return_type!=0) {
                r_handlers[id] = [this](Context& ctx) {
                    standard_sub_process(ctx);
                    resolve_overload(ctx);
                };
            }
            x_handlers[id] = x_handler;
            return id;
        }

        std::string children_to_string(Context& ctx, node_col children) {
            std::string to_print = "";
            for(int i=0;i<ctx.node().children().length();i++) {
                Node c = ctx.node().children()[i];
                process_node(ctx,c);
                to_print += value_as_string(c.value());
            }
            return to_print;
        }

        uint32_t print_id = add_function("print",[this](Context& ctx){ 
            print(children_to_string(ctx,ctx.node().children()));
        });
        uint32_t return_id = make_tokenized_keyword("return");

        uint32_t true_id = add_function("true",[this](Context& ctx){
            bool t = true;
            ctx.node().value().set((void*)&t);
        },1,bool_id);
        uint32_t false_id = add_function("false",[this](Context& ctx){
            bool f = false;
            ctx.node().value().set((void*)&f);
        },1,bool_id);

        void init() override {
            init_literals();
            init_tokenizer();
            init_stage_a();
            init_stage_s();

            register_type("int",int_id,4);
            register_type("float",float_id,4);
            register_type("bool",bool_id,1);
            register_type("string",string_id,sizeof(Ptr));
            register_type("Node",node_id,sizeof(Ptr));
            register_type("Value",value_id,sizeof(Ptr));
            register_type("Context",context_id,sizeof(Ptr));
            register_type("Ptr",ptr_id,sizeof(Ptr));
            register_type("func",function_id,sizeof(Ptr));
            value_printers[function_id] = [this](Context& ctx){ctx.source(Ptr_as_string(*(Ptr*)ctx.value().get()));};
            register_type("duck",duck_id,0);
            value_printers[duck_id] = [this](Context& ctx){ctx.source()="QUACK!";};

            set_binding_powers(random_combo_id,8,9);

            t_handlers[identifier_id] = [this](Context& ctx){resolve_identifier(ctx);};
            t_handlers[equals_id] = [this](Context& ctx){standard_sub_process(ctx);};

            t_handlers.default_function = [this](Context& ctx){if(ctx.node().scopes().empty()) {standard_sub_process(ctx);}}; //Because resolving passes will already cover the sub process for scoped nodes
            r_handlers.default_function = [this](Context& ctx){standard_sub_process(ctx);};
            x_handlers.default_function = [this](Context& ctx){standard_sub_process(ctx);};

            r_handlers[func_decl_id] = [this](Context& ctx) {
                fire_quals(ctx,ctx.node().value());
                Node scope = ctx.node().scopes()[0];
                if(!is_live(scope.value())) {
                    scope.value(make_value()); 
                    scope.value().loc(0); //Set location for stack depth
                }
            };
            x_handlers[func_decl_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node().value());
            };

            t_handlers[lbracket_id] = [this](Context& ctx){
                if(!ctx.node().scopes().empty()) {
                    ctx.node().type(lambda_id);
                    ctx.node().value(make_value(function_id,sizeof(Ptr)));
                    Node scope = ctx.node().scopes()[0];
                    if(!is_live(scope.value())) {
                        scope.value(make_value()); 
                        scope.value().loc(0); //Set location for stack depth
                    }
                } else {
                    standard_sub_process(ctx);
                }
            };  
            r_handlers[func_call_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                resolve_overload(ctx);
                fire_quals(ctx,ctx.node().value());
                sync_args(ctx);
                //instantiate_template(ctx.node(),ctx.node().value().type_scope().owner(),ctx);
            };
            x_handlers[lambda_id] = [this](Context& ctx){
                Node puppet = instantiate_function(ctx.node(),ctx); 
                ctx.node().value().set((void*)&puppet);
                //print("Returned puppet: ",node_to_string(puppet));
            };
            x_handlers[func_call_id] = [this](Context& ctx) {
                if(ctx.node().value().type()==function_id) { //Because active function values store their return in their sub value, if this gets confused consider using a qual minted by returns on function types
                    if(ctx.node().has_qual(lparen_id)) {
                        Node func = ((Node&)*(Ptr*)ctx.node().value().get());
                        Value decl_val = func.value();
                        Node func_scope = func.scopes()[0];
                        if(!decl_val.sub_values().empty()) {
                            ctx.node().value(decl_val.sub_values()[0]);
                        }
                        if(ctx.node().scopes().empty()) { //We're a lambda being called for the first time
                            ctx.node().scopes() << func_scope;
                            sync_args(ctx);
                        } else {
                            ctx.node().scopes().col().set(0,(void*)&func_scope);
                        }
                    } else {
                        return;
                    }
                }

                Node scope = ctx.node().scopes()[0];
                //print("Calling function");
                call_func(ctx,scope);
                //print("Call succeded");
            };


            t_handlers[return_id] = [this](Context& ctx){
                if(ctx.index()+1<ctx.result().length()) {
                    ctx.node().children() << ctx.result().take(ctx.index()+1);
                }
                standard_sub_process(ctx);
            };
            r_handlers[return_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node climb = ctx.node();
                while(is_live(climb)&&climb.type()!=func_decl_id&&climb.type()!=lambda_id) { //WARNING: we need to make sure things like in blocks which also use returns are safe with this! 
                    //This might try to bind to some random function via climbing when it does this, so when metaprogramming becomes visible in the compielr, add gaurds.
                    climb = climb.in_scope().owner();
                }
                ctx.node().parent(climb);
                if(is_live(ctx.node().parent())) {
                    if(ctx.node().parent().type()==lambda_id) {
                        if(!ctx.node().children().empty()) {
                            ctx.node().value(make_value());
                            ctx.node().value().copy(ctx.node().children()[0].value(),true);
                            ctx.node().parent().value().sub_values().push(ctx.node().value());
                        }
                    } else {
                        ctx.node().value(ctx.node().parent().value());
                    }
                }
            };  
            x_handlers[return_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(is_live(ctx.node().parent())) {
                    if(!ctx.node().children().empty()) {
                        void* snap = ctx.node().children()[0].value().get();
                        ascend_call_scope(ctx.node().parent().scopes()[0]);
                        ctx.node().value().set(snap);
                    } else {
                        ascend_call_scope(ctx.node().parent().scopes()[0]);
                    }
                }
                ctx.state(1);
                return;
            };

            r_handlers[equals_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx);
            };
            x_handlers[equals_id] = [this](Context& ctx){
                if(ctx.node().children().length()==2) {
                    backwards_sub_process(ctx);
                    DEBUG_ONLY(if(ERROR_FLAG){log(red("Attempted to execute equals while another error was flagged")); return;})
                    Node left = ctx.node().children()[0];
                    Node right = ctx.node().children()[1];
                    Value rv = right.value();
                    Value lv = left.value();
                    assign(lv,rv);
                    // DEBUG_ONLY(if(left.value().size()!=right.value().size()) {throw_error("Mismatched sizes for assignment from:\n",node_to_string(ctx.node())); return;})
                    
                    // //Col& lcol = resolve_to_col(lp);
                    // if(resolve_to_col(lp).heterogenous) {
                    //     resolve_to_col(lp).qset(lp.sidx,resolve_ptr(rp),right.value().size());
                    // } else {
                    //     resolve_to_col(lp).set(lp.sidx,resolve_ptr(rp));
                    // }
                }
            };

            //Stricter equals with no duck typing or reassignment
                // x_handlers[equals_id] = [this](Context& ctx){
                //     if(ctx.node().children().length()==2) {
                //         backwards_sub_process(ctx);
                //         DEBUG_ONLY(if(ERROR_FLAG){log(red("Attempted to execute equals while another error was flagged")); return;})
                //         Node left = ctx.node().children()[0];
                //         Node right = ctx.node().children()[1];

                //         Ptr lp = left.value().data_ptr();
                //         Ptr rp = right.value().data_ptr();

                        

                //         if(!is_live(lp)) return; //Normally caused by something being delcared but never used, and thus missed by the m pass
                //         DEBUG_ONLY(if(!is_live(rp)) {throw_error("right term of equals is invalid"); return;})
                //         DEBUG_ONLY(if(left.value().size()!=right.value().size()) {throw_error("Mismatched sizes for assignment from:\n",node_to_string(ctx.node())); return;})
                        
                //         //Col& lcol = resolve_to_col(lp);
                //         if(resolve_to_col(lp).heterogenous) {
                //             resolve_to_col(lp).qset(lp.sidx,resolve_ptr(rp),right.value().size());
                //         } else {
                //             resolve_to_col(lp).set(lp.sidx,resolve_ptr(rp));
                //         }
                //     }
                // };

            make_tokenized_keyword("any",any_id);
            make_tokenized_keyword("null",null_id);

            r_handlers[to_unary_id(bang_id)] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(bool_id,1));
                resolve_overload(ctx);
            };
            x_handlers[to_unary_id(bang_id)] = [this](Context& ctx){
                standard_sub_process(ctx);
                bool b = *(bool*)ctx.node().children()[0].value().get();
                b = !b;
                ctx.node().value().set((void*)&b);
            };

            // add_gather_token('#', hash_id, identifier_id, identifier_id); //REMEMBER TO FIX ## AS WELL LATER!

            r_handlers[identifier_id] = [this](Context& ctx){
                resolve_overload(ctx);
            };
            r_handlers[literal_id] = r_handlers[identifier_id];

            r_handlers[dot_id] = [this](Context& ctx){
                if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(ctx.node().type()==dot_id) {
                    Node left = ctx.node().children()[0];
                    Node right = ctx.node().children()[1];
                    uint32_t ltype = left.value().type();
                    if(layouts.hasKey(ltype)) {
                        _layout& layout = layouts.get(ltype);
                        std::string prop = right.name().to_std();
                        if(layout.label_to_index.hasKey(prop)) {
                            uint32_t index = layout.label_to_index.get(prop);
                            if(is_live(layout.ptrs[index])) { //If we were handed a full value just copy that over (why not just always use this though... mark for later)
                                ctx.node().value(make_value()); ctx.node().value().copy(layout.ptrs[index],true);
                            } else {
                                ctx.node().value(make_value(layout.tags[index], layout.sizes[index], layout.offsets[index], layout.subtags[index], layout.subsizes[index]));
                            }
                        } else {
                            print(red("r_handlers::dot_id layout of "+labels[ltype]+" does not have prop "+prop));
                            // print(red("root is: "+labels[ctx.root().type()])); 
                        }
                        //right.value(ctx.node().value());
                    } else {
                        print(red("r_handlers::dot_id no layout found for type "+labels[ltype]));
                    }

                    //This is mean to be for inline get syntax like children(0), probably going to be replaced with a proper overload in the future
                    if(right.type()==identifier_id&&!right.children().empty()) { //Can replace with QValue in the future for an optimization
                        Value value = ctx.node().value();
                        value.type(value.sub_type()); value.sub_type(0);
                        value.size(value.sub_size()); value.sub_size(0);
                        //right.type(temp_get_id);
                    }

                    resolve_overload(ctx); //Going around a second time
                } else if(ctx.node().type()==method_call_id) { //Turn into a function call
                    ctx.node().type(func_call_id);
                    Node amp = make_node(to_unary_id(amp_id));
                    amp.value(make_value(ptr_id,sizeof(Ptr)));
                    Node match_this = make_node(identifier_id,"match_this",ctx.node().children()[0].value(),ctx.node().in_scope());
                    amp.children().push(match_this);
                    process_node(ctx,amp); //Resolve this
                    node_col args = ctx.node().children()[1].children();
                    ctx.node().children(args);
                    ctx.node().children().insert(0,amp);
                    ctx.node().scopes().push(ctx.node().value().type_scope());
                    sync_args(ctx);
                    ctx.node().value(ctx.node().value().type_scope().owner().value());
                }
            };
            x_handlers[dot_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                Value value = ctx.node().value();
                if(right.type()==identifier_id) {
                    Ptr ptr = deadptr;
                    uint32_t rvt = left.value().type();
                    if(rvt==ptr_id||rvt==node_id||rvt==value_id||rvt==context_id||rvt==string_id) {
                        void* p = left.value().get();
                        DEBUG_ONLY(if(ERROR_FLAG) {return;});
                        ptr = *(Ptr*)p;
                    } else {
                        ptr = left.value().data_ptr();
                    }
                    ptr.sidx = value.address();
                    if(!right.children().empty()) { //This is a bit jank, I would prefer to find a way to get = working instead
                        if(resolve_to_col(ptr).heterogenous) {
                            resolve_to_col(ptr).qset(ptr.sidx,(void*)&right.children()[0].value().data_ptr(),right.children()[0].value().size());
                        } else {
                            resolve_to_col(ptr).set(ptr.sidx,(void*)&right.children()[0].value().data_ptr());
                        }
                    } else {
                        //if(is_assignment.getOrDefault(ctx.root().type(),false)&&is_live(ctx.left())) {
                            value.data_ptr(ptr); //Setting the data pointer itself
                        // } else {
                        //     value.set(resolve_ptr(ptr)); //Setting what the data_ptr points to
                        // }
                    }
                }
            };


            tokenizer_state_functions[quote_id] = [this](Context& ctx) {
                char c = ctx.source().at(ctx.index());

                if(ctx.node().quals().empty()) {
                    Node open_token = copy_as_token(ctx.node());
                    ctx.node().quals() << open_token;
                    ctx.node().type(string_id);
                    ctx.node().name().col().clear();
                    ctx.node().x(at_x);
                    ctx.node().y(at_y);
                }

                if(c == '"') {
                    ctx.state(0);
                    Node closer = copy_as_token(ctx.node().quals()[0]);
                    closer.x(at_x); closer.y(at_y);
                    ctx.node().quals() << closer;
                } else if(c == '\\' && ctx.index()+1<ctx.source().length()) {
                    char next = ctx.source().at(ctx.index() + 1);
                    switch(next) {
                        case 'n':  ctx.node().name().push('\n'); break;
                        case 't':  ctx.node().name().push('\t'); break;
                        case 'r':  ctx.node().name().push('\r'); break;
                        case '"':  ctx.node().name().push('"');  break;
                        case '\\': ctx.node().name().push('\\'); break;
                        default:   ctx.node().name().push(next); break;
                    }
                    at_x += 1.0f;
                    ctx.index()++;
                } else if(c == '\n') {
                    at_y += 1.0f;
                    at_x = -1.0f;
                } else {
                    ctx.node().name().push(c);
                }
            };

            //Add handeling as a single char later if we only ever iterate over one item, for now this is just another way to make strings (for TwigSnap)
            tokenizer_state_functions[single_quote_id] = [this](Context& ctx) {
                char c = ctx.source().at(ctx.index());

                if(ctx.node().quals().empty()) {
                    Node open_token = copy_as_token(ctx.node());
                    ctx.node().quals() << open_token;
                    ctx.node().type(string_id);
                    ctx.node().name().col().clear();
                    ctx.node().x(at_x);
                    ctx.node().y(at_y);
                }

                if(c == '\'') {
                    ctx.state(0);
                    Node closer = copy_as_token(ctx.node().quals()[0]);
                    closer.x(at_x); closer.y(at_y);
                    ctx.node().quals() << closer;
                } else if(c == '\\' && ctx.index()+1<ctx.source().length()) {
                    char next = ctx.source().at(ctx.index() + 1);
                    switch(next) {
                        case 'n':  ctx.node().name().push('\n'); break;
                        case 't':  ctx.node().name().push('\t'); break;
                        case 'r':  ctx.node().name().push('\r'); break;
                        case '"':  ctx.node().name().push('"');  break;
                        case '\'':  ctx.node().name().push('\'');  break;
                        case '\\': ctx.node().name().push('\\'); break;
                        default:   ctx.node().name().push(next); break;
                    }
                    at_x += 1.0f;
                    ctx.index()++;
                } else if(c == '\n') {
                    at_y += 1.0f;
                    at_x = -1.0f;
                } else {
                    ctx.node().name().push(c);
                }
            };
 
            r_handlers[langle_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(bool_id,1));
            };
            x_handlers[langle_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                int result =      
                    *(int*)p1
                    <
                    *(int*)p2
                ;
                ctx.node().value().set((void*)&result);
            };

            r_handlers[rangle_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(bool_id,1));
            };
            x_handlers[rangle_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                int result =      
                    *(int*)p1
                    >
                    *(int*)p2
                ;
                ctx.node().value().set((void*)&result);
            };


            // r_handlers[plus_equals_id] = [this](Context& ctx){
            //     if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
            //     standard_sub_process(ctx);
            //     resolve_overload(ctx);
            // };

            r_handlers[plus_id] = [this](Context& ctx){
                if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(int_id,4));
            };
            x_handlers[plus_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                int result =      
                    *(int*)p1
                    +
                    *(int*)p2
                ;
                ctx.node().value().set((void*)&result);
            };

            r_handlers[dash_id] = [this](Context& ctx){
                if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(int_id,4));
            };
            x_handlers[dash_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                int result =      
                    *(int*)p1
                    -
                    *(int*)p2
                ;
                ctx.node().value().set((void*)&result);
            };

            r_handlers[star_id] = [this](Context& ctx){
                if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(int_id,4));
            };
            x_handlers[star_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                int result =      
                    *(int*)p1
                    *
                    *(int*)p2
                ;
                ctx.node().value().set((void*)&result);
            };

            r_handlers[slash_id] = [this](Context& ctx){
                if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(int_id,4));
            };
            x_handlers[slash_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                int result =      
                    *(int*)p1
                    /
                    *(int*)p2
                ;
                ctx.node().value().set((void*)&result);
            };

            r_handlers[to_unary_id(dash_id)] = [this](Context& ctx){
                if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(int_id,4));
            };
            x_handlers[to_unary_id(dash_id)] = [this](Context& ctx){
                int neg = -(*(int*)ctx.node().children()[0].value().get());
                ctx.node().value().set((void*)&neg);
            };

            x_handlers[make_tokenized_keyword("root_name")] = [this](Context& ctx){
                if(ctx.node().children().empty()) {
                    ctx.node().value(make_value(string_id,sizeof(Ptr)));
                    ctx.node().value().set((void*)&ctx.root().name_ptr());
                } else {
                    ctx.root().name() = ctx.node().children()[0].name();
                }
            };
        }
    };
}

#define LOBOTOMIZE_M_STAGE 1

namespace Acorn {
    struct Acorn_Script : public virtual Compiler_Unit {
        Acorn_Script(uint16_t _uid) : Unit(_uid) {init();}
        Acorn_Script() {init();}

        //For my sanity
        string resolve_string_ticket(Node n) {
            if(is_live(n.value())) {
                if(is_live(*(Ptr*)n.value().get())) {
                    return (string&)*(Ptr*)n.value().get();
                } else {
                    Ptr p = get_ticket(name_store_id,1,char_id); 
                    n.value().set((void*)&p);
                    return (string&)p;
                }
            }
            return deadptr;
        }

        Ptr resolve_ticket_by_ptr(Ptr p, Col& col, uint32_t index, uint32_t width, uint32_t size, uint32_t type) {
            if(is_live(p)) {
                return p;
            } else {
                p = get_ticket(data_store_id,size,type); 
                col.qset(index, (void*)&p, width);
                return p;
            }
        }


        Ptr resolve_ticket(Node n,  uint32_t size, uint32_t type) {
            if(is_live(n.value())) {
                if(is_live(*(Ptr*)n.value().get())) {
                    return *(Ptr*)n.value().get();
                } else {
                    Ptr p = get_ticket(data_store_id,size,type); 
                    n.value().set((void*)&p);
                    return p;
                }
            }
            return deadptr;
        }

        uint32_t test_id = reg_id("TEST");
        Stage& n_handlers = reg_stage("naming"); 
        
        uint32_t labels_id = make_tokenized_keyword("labels");

        uint32_t node_block_id = reg_id("node_block");
        uint32_t invoke_stage_id = make_keyword("invoke_stage");
        uint32_t in_id = make_keyword("in");
        uint32_t on_id = make_tokenized_keyword("on");
        uint32_t precompiling_id = reg_id("PRECOMPILING");

        uint32_t ctx_id = make_tokenized_keyword("ctx");
        uint32_t lctx_id = make_tokenized_keyword("lctx");

        uint32_t while_id = make_tokenized_keyword("while");
        uint32_t for_id = make_tokenized_keyword("for");
        uint32_t if_id = make_tokenized_keyword("if");
        uint32_t else_id = make_tokenized_keyword("else");

        uint32_t read_file_id = make_tokenized_keyword("read_file");
        uint32_t write_file_id = add_function("write_file",[this](Context& ctx){
            standard_sub_process(ctx);
            string path = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            string contents = (string&)*(Ptr*)ctx.node().children()[1].value().get();
            DEBUG_ONLY(if(ERROR_FLAG){return;})
            writeFile(path.to_std(),contents.to_std());
        });
        uint32_t compile_id = make_tokenized_keyword("compile");

        uint32_t live_qual = register_qual_ids("live");
        uint32_t gatekeeper_qual = register_qual_ids("gatekeeper");
        uint32_t assigned_qual = register_qual_ids("assigned");
        uint32_t constant_qual = register_qual_ids("constant");

        uint32_t to_string_id = make_tokenized_keyword("to_string");
        uint32_t from_string_id = add_function("from_string",[this](Context& ctx){
            standard_sub_process(ctx);
            string str = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            Node n = compile_literal(str.to_std());
            print("From string: ",node_info(n));
            ctx.node().value(n.value());
        },0,duck_id);
        uint32_t to_type_id = make_tokenized_keyword("to_type");
        uint32_t DEBUG_ROOT_id = make_tokenized_keyword("DEBUG_ROOT");

        uint32_t get_ticket_id = add_function("get_ticket",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t pool = *(uint32_t*)ctx.node().children()[0].value().get();
            Value v = ctx.node().children()[1].value();
            Ptr ticket = get_ticket(pool,v.size(),v.type());
            if(is_live(v.data_ptr())) {
                resolve_to_col(ticket).push(v.get());
            }
            ctx.node().value().set((void*)&ticket);
        },sizeof(Ptr),ptr_id);
        uint32_t get_ticket_of_type_id = add_function("get_ticket_of_type",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t pool = *(uint32_t*)ctx.node().children()[0].value().get();
            Value v = ctx.node().children()[1].value();
            Ptr ticket = get_ticket(pool,v.size(),v.type());
            ctx.node().value().set((void*)&ticket);
        },sizeof(Ptr),ptr_id);

        uint32_t ptr_take_id = reg_id("PTR_TAKE");
        uint32_t ptr_push_id = reg_id("PTR_PUSH");
        uint32_t ptr_length_id = reg_id("PTR_LENGTH");
        uint32_t ptr_clear_id = overload_type(ptr_id,".\"clear\"","PTR_CLEAR",deadptr,[this](Context& ctx){
            standard_sub_process(ctx);
            Node left = ctx.node().children()[0];
            Node right = ctx.node().children()[1];
            Col& col = resolve_to_col(*(Ptr*)left.value().get());
            col.clear();
        });
        // uint32_t string_append_id = reg_id("STRING_APPEND");
        uint32_t string_substr_id = reg_id("STRING_SUBSTR");
        uint32_t string_slice_id = reg_id("STRING_SLICE");
        uint32_t string_find_id = reg_id("STRING_FIND");
        uint32_t string_find_from_id = reg_id("STRING_FIND_FROM");

        uint32_t break_id = add_function("break",[this](Context& ctx){
            ctx.state(2);
        });
        uint32_t continue_id = add_function("continue",[this](Context& ctx){
            ctx.state(3);
        });

        uint32_t suspend_unit_id = add_function("suspend_unit",[this](Context& ctx){print("Suspended unit"); suspend();});

        uint32_t ptr_get_id = overload_type(ptr_id,".\"get\"","PTR_GET",make_value(0),[this](Context& ctx){ //No value means take the subsize and subtype 
            standard_sub_process(ctx);
            Node left = ctx.node().children()[0];
            Node right = ctx.node().children()[1];
            Value cv = right.value();
            void* lv = left.value().get();
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("No left value in ptr get")); return;});
            Ptr ptr = *(Ptr*)lv;
            Col& col = resolve_to_col(ptr);
            if(!right.children().empty()) {
                cv = right.children()[0].value();
            }
            if(cv.type()==int_id) {
                int index = *(int*)cv.get();
                if(index<col.length()) {
                    Value value = ctx.node().value();
                    ptr.sidx = index;
                    value.data_ptr(ptr);
                } else {
                    print(red("ptr_get:x_handler index "+std::to_string(index)+" out of bounds on "+Ptr_as_string(ptr)));
                }
            } else if(cv.type()==string_id||cv.type()==ptr_id||cv.type()==node_id) {
                Col& ccol = resolve_to_col(*(Ptr*)cv.get());
                ptr.sidx = col.getidx(ccol.storage,ccol.size);
                ctx.node().value().data_ptr(ptr);
            }
        });
        uint32_t ptr_idxget_id = overload_type(ptr_id,"[any]","PTR_IDXGET",make_value(0),[this](Context& ctx){ //No value means take the subsize and subtype 
            standard_sub_process(ctx);
            Node left = ctx.node().children()[0];
            Node right = ctx.node().children()[1];
            Value cv = right.value();
            void* lv = left.value().get();
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("No left value in ptridx get")); return;});
            Ptr ptr = *(Ptr*)lv;
            Col& col = resolve_to_col(ptr);
            if(cv.type()==int_id) {
                int index = *(int*)cv.get();
                if(index<col.length()) {
                    Value value = ctx.node().value();
                    ptr.sidx = index;
                    value.data_ptr(ptr);
                } else {
                    print(red("ptr_idxget:x_handler index "+std::to_string(index)+" out of bounds on "+Ptr_as_string(ptr)));
                }
            } else if(cv.type()==string_id||cv.type()==ptr_id||cv.type()==node_id) {
                Col& ccol = resolve_to_col(*(Ptr*)cv.get());
                ptr.sidx = col.getidx(ccol.storage,ccol.size);
                ctx.node().value().data_ptr(ptr);
            }
        });
        uint32_t ptr_put_id = overload_type(ptr_id,".\"put\"","PTR_PUT",deadptr,[this](Context& ctx){
            standard_sub_process(ctx);
            Node left = ctx.node().children()[0];
            Node right = ctx.node().children()[1];
            DEBUG_ONLY(if(ERROR_FLAG) {return;})
            void* lv = left.value().get();
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("No left value in ptr put")); return;});
            Ptr ptr = *(Ptr*)lv;
            Col& col = resolve_to_col(ptr);
            Value keyv = right.children()[0].value();
            Value elv = right.children()[1].value();
            
            void* key = nullptr;
            uint32_t key_size = 0;
            if(keyv.type()==string_id||keyv.type()==ptr_id||keyv.type()==node_id) {
                Col& keycol = resolve_to_col(*(Ptr*)keyv.get());
                key = keycol.storage;
                key_size = keycol.size;
            }
            col.qput(elv.get(),key,key_size,keyv.type());
        });
        uint32_t ptr_has_id = overload_type(ptr_id,".\"has\"","PTR_HAS",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            Node left = ctx.node().children()[0];
            Node right = ctx.node().children()[1];
            DEBUG_ONLY(if(ERROR_FLAG) {return;})
            void* lv = left.value().get();
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("No left value in ptr put")); return;});
            Ptr ptr = *(Ptr*)lv;
            Col& col = resolve_to_col(ptr);
            Value keyv = right.children()[0].value();
            
            void* key = nullptr;
            uint32_t key_size = 0;
            if(keyv.type()==string_id||keyv.type()==ptr_id||keyv.type()==node_id) {
                Col& keycol = resolve_to_col(*(Ptr*)keyv.get());
                key = keycol.storage;
                key_size = keycol.size;
            }
            bool has_thing = col.hasKey(key,key_size);
            ctx.node().value().set((void*)&has_thing);
        });
        uint32_t ptr_qset_id = overload_type(ptr_id,".\"qset\"","PTR_QSET",deadptr,[this](Context& ctx){
            standard_sub_process(ctx);
            Node left = ctx.node().children()[0];
            Node right = ctx.node().children()[1];
            DEBUG_ONLY(if(ERROR_FLAG) {return;})
            void* lv = left.value().get();
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("No left value in ptr put")); return;});
            Ptr ptr = *(Ptr*)lv;
            Col& col = resolve_to_col(ptr);
            void* data = right.children()[0].value().get();
            int width  = *(int*)right.children()[1].value().get();
            col.qset(ptr.sidx,data,width);
        });
        uint32_t ptr_set_id = overload_type(ptr_id,".\"set\"","PTR_SET",deadptr,[this](Context& ctx){
            standard_sub_process(ctx);
            Node left = ctx.node().children()[0];
            Node right = ctx.node().children()[1];
            DEBUG_ONLY(if(ERROR_FLAG) {return;})
            void* lv = left.value().get();
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("No left value in ptr put")); return;});
            Ptr ptr = *(Ptr*)lv;
            Col& col = resolve_to_col(ptr);
            void* data = right.children()[1].value().get();
            uint32_t index = 0;
            Value cv = right.children()[0].value();
            if(cv.type()==int_id) {
                index = *(int*)cv.get();
                if(index>=col.length()) {
                    print(red("ptr_set:x_handler index "+std::to_string(index)+" out of bounds on "+Ptr_as_string(ptr)));
                }
            }
            else if(cv.type()==string_id||cv.type()==ptr_id||cv.type()==node_id) {
                void* key = nullptr;
                uint32_t key_size = 0;
                Col& keycol = resolve_to_col(*(Ptr*)cv.get());
                key = keycol.storage;
                key_size = keycol.size;
                index = col.getidx(key,key_size);
            }
            col.set(index,data);
        });

        uint32_t ptr_label_id = overload_type(ptr_id,".\"label\"","PTR_LABEL",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            Ptr p = *(Ptr*)ctx.node().children()[0].value().get();
            if(!ctx.node().children()[1].children().empty()) {
                string label = (string&)*(Ptr*)ctx.node().children()[1].children()[0].value().get();
                resolve_to_col(p).label = label.to_std();
            } else {
                string output = resolve_string_ticket(ctx.node());
                output = resolve_to_col(p).label.to_std();
            }
        });

        uint32_t check_equality_int = overload_type(int_id,"==int","CHECK_EQUALITY_INT",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            bool result = (*(int*)ctx.node().children()[0].value().get()==*(int*)ctx.node().children()[1].value().get());
            ctx.node().value().set((void*)&result);
        });
        uint32_t check_noequality_int = overload_type(int_id,"!=int","CHECK_NOEQUALITY_INT",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            bool result = (*(int*)ctx.node().children()[0].value().get()!=*(int*)ctx.node().children()[1].value().get());
            ctx.node().value().set((void*)&result);
        });

        uint32_t check_equality_string = overload_type(string_id,"==string","CHECK_EQUALITY_STRING",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            string l(*(Ptr*)ctx.node().children()[0].value().get());
            string r(*(Ptr*)ctx.node().children()[1].value().get());
            bool result = false;
            if(l.length()!=r.length()) {ctx.node().value().set((void*)&result); return;}
            for(int i=0;i<l.length();i++) {
                if(l.at(i)!=r.at(i)) {
                    ctx.node().value().set((void*)&result);
                    return;
                }
            }
            result = true;
            ctx.node().value().set((void*)&result);
        });

        uint32_t check_lessthan_or_equalsto_int = overload_type(int_id,"<=int","CHECK_LEQ_INT",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            bool result = (*(int*)ctx.node().children()[0].value().get()<=*(int*)ctx.node().children()[1].value().get());
            ctx.node().value().set((void*)&result);
        });
        uint32_t check_greaterthan_or_equalsto_int = overload_type(int_id,">=int","CHECK_GEQ_INT",make_value(bool_id,1),[this](Context& ctx){
            x_handlers.run(check_lessthan_or_equalsto_int)(ctx);
            bool result = !*(bool*)ctx.node().value().get();
            ctx.node().value().set((void*)&result);
        });
        uint32_t check_lessthan_int = overload_type(int_id,"<int","CHECK_LT_INT",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            DEBUG_ONLY(if(ERROR_FLAG) {return;});
            bool result = (*(int*)ctx.node().children()[0].value().get()<*(int*)ctx.node().children()[1].value().get());
            ctx.node().value().set((void*)&result);
        });
        uint32_t check_greaterthan_int = overload_type(int_id,">int","CHECK_GT_INT",make_value(bool_id,1),[this](Context& ctx){
            x_handlers.run(check_lessthan_int)(ctx);
            DEBUG_ONLY(if(ERROR_FLAG) {return;});
            bool result = !*(bool*)ctx.node().value().get();
            ctx.node().value().set((void*)&result);
        });

        uint32_t increment_int = overload_type(int_id,"++int","INCREMENT_INT",make_value(int_id,4),[this](Context& ctx){
            standard_sub_process(ctx);
            ctx.node().value(ctx.node().children()[0].value());
            int inced = *(int*)ctx.node().value().get()+1;
            if(ctx.node().value().data_col().heterogenous) {
                ctx.node().value().data_col().qset(ctx.node().value().data_ptr().sidx,(void*)&inced,ctx.node().value().size());
            } else {
                ctx.node().value().set((void*)&inced);
            }
        });

        uint32_t valueGetStr_id = overload_type(value_id,".\"getStr\"","VALUE_GETSTR",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            Value v = (Value&)*(Ptr*)ctx.node().children()[0].value().get();
            ctx.node().value(v);
        });
        uint32_t valueGetNode_id = overload_type(value_id,".\"getNode\"","VALUE_GETNODE",make_value(node_id,sizeof(Ptr)),[this](Context& ctx){
            standard_sub_process(ctx);
            Node v = (Value&)*(Ptr*)ctx.node().children()[0].value().get();
            ctx.node().value(v);
        });
        uint32_t valueGetInt_id = overload_type(value_id,".\"getInt\"","VALUE_GETINT",make_value(int_id,4),[this](Context& ctx){
            standard_sub_process(ctx);
            Value v = (Value&)*(Ptr*)ctx.node().children()[0].value().get();
            ctx.node().value(v);
        });
        uint32_t value_set_id = overload_type(value_id,".\"set\"","VALUE_SET",deadptr,[this](Context& ctx){
            standard_sub_process(ctx);
            Value v = (Value&)*(Ptr*)ctx.node().children()[0].value().get();
            void* d = ctx.node().children()[1].children()[0].value().get();
            v.set(d);
        });

        uint32_t node_asid_id = overload_type(node_id,".\"asID\"","NODE_ASID",make_value(int_id,4),[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t id = (*(Ptr*)ctx.node().children()[0].value().get()).idx;
            ctx.node().value().set((void*)&id);
        });

        uint32_t std_sub_proccess_id = add_function("subprocess",[this](Context& ctx){
            standard_sub_process(ctx);
            Context c = (Context&)*(Ptr*)ctx.node().children()[0].value().get();
            standard_sub_process(c);
        });

        //Investigate why this isn't working later
        // uint32_t context_doat = overload_type(context_id,".\"p\"","DOAT",deadptr,[this](Context& ctx){
        //     standard_sub_process(ctx);
        //     Context context = (Context&)*(Ptr*)ctx.node().children()[0].value().get();
        //     print("Source ptr of ",Ptr_as_string(context),": ",Ptr_as_string(context.source_ptr()));
        // });


        uint32_t string_equals_id = overload_type(string_id,"=string","STRING_EQUALS",deadptr,[this](Context& ctx){
            standard_sub_process(ctx);
            string l = resolve_string_ticket(ctx.node().children()[0]);
            string r = resolve_string_ticket(ctx.node().children()[1]);
            l = r;
        });

        uint32_t string_append_id = overload_type(string_id,"+string","STRING_APPEND",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            string l = resolve_string_ticket(ctx.node().children()[0]);
            string r = resolve_string_ticket(ctx.node().children()[1]);
            string output = resolve_string_ticket(ctx.node());
            output.col().clear(); output.push(l.to_std()); output.push(r.to_std());
        });

        uint32_t string_func_append_id = overload_type(string_id,".\"append\"","STRING_FUNC_APPEND",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            Node left = ctx.node().children()[0]; Node right = ctx.node().children()[1];
            DEBUG_ONLY(if(ERROR_FLAG) return;)
            Node right_child = ctx.node().children()[1].children()[0];
            DEBUG_ONLY(if(ERROR_FLAG) return;)
            void* lv = left.value().get(); void* rv = right_child.value().get();
            DEBUG_ONLY(if(ERROR_FLAG) return;)
            string l(*(Ptr*)lv);
            string r(*(Ptr*)rv);
            l.push(r.to_std());
            ctx.node().value(ctx.node().children()[0].value());
        });        

        uint32_t string_append_eq_id = overload_type(string_id,"+=string","STRING_APPEND_EQ",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            string l(*(Ptr*)ctx.node().children()[0].value().get());
            string r(*(Ptr*)ctx.node().children()[1].value().get());
            l.push(r.to_std());
            ctx.node().value(ctx.node().children()[0].value());
        });

        uint32_t split_str_id = add_function("split_str",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string str = ((string&)(*(Ptr*)ctx.node().children()[0].value().get())).to_std();
            std::string delimiter = ((string&)(*(Ptr*)ctx.node().children()[1].value().get())).to_std();
            Ptr p = resolve_ticket(ctx.node(),sizeof(Ptr),string_id);
            Col& data = resolve_to_col(p);
            list<std::string> split = split_str(str, delimiter.at(0));
            while(data.length() > split.length()) {
                recycle_column(*(Ptr*)data.last());
                data.removeAt(data.length()-1);
            }
            for(int i=0;i<split.length();i++) {
                if(i<data.length()) {
                    string s = (string&)*(Ptr*)data[i];
                    s = split[i];
                } else {
                    string s = get_ticket(name_store_id,1,char_id);
                    s = split[i];
                    data.push((void*)&s);
                }
            }
            ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);


        uint32_t colsize_id = add_function("_colsize",[this](Context& ctx){
            Node left = ctx.node().children()[0];
            
        },4,int_id);

        uint32_t ptr_size_id = add_function("ptr_size",[this](Context& ctx){
            uint32_t s = sizeof(Ptr);
            ctx.node().value().set((void*)&s);
        },4,int_id);

        uint32_t make_value_id = add_function("make_value",[this](Context& ctx){
            Value v = make_value();
            if(ctx.node().children().length()==2) {
                standard_sub_process(ctx);
                int type = *(int*)ctx.node().children()[0].value().get();
                int size = *(int*)ctx.node().children()[1].value().get();
                v.type(type); v.size(size);
                v.init_data();
            }
            ctx.node().value().set((void*)&v);
        },sizeof(Ptr),value_id);

        uint32_t recycle_id = add_function("recycle",[this](Context& ctx){
            standard_sub_process(ctx);
            Node to_recycle = (Node&)(*(Ptr*)ctx.node().children()[0].value().get());
            recycle_node(to_recycle);
        });

        uint32_t stoi_id = add_function("stoi",[this](Context& ctx){
            standard_sub_process(ctx);
            string s = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            int stoid = std::stoi(s.to_std());
            ctx.node().value().set((void*)&stoid);
        },4,int_id);

        uint32_t string_to_Ptr_id = add_function("string_to_Ptr",[this](Context& ctx){
            standard_sub_process(ctx);
            string s = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            Ptr p = string_to_Ptr(s.to_std());
            ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);


        uint32_t make_unit_id = add_function("make_unit",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t unitid = (uint32_t)make_unit<Unit>()->uid;
            ctx.node().value().set((void*)&unitid);
        },4,int_id);

        uint32_t randi_id = add_function("randi",[this](Context& ctx){
            standard_sub_process(ctx);
            int min = *(int*)ctx.node().children()[0].value().get();
            int max = *(int*)ctx.node().children()[1].value().get();
            int rand = randi(min,max);
            ctx.node().value().set((void*)&rand);
        },4,int_id);

        void e_stage_assignment_handler(Context& ctx) {
            Node left = ctx.node().children()[0];
            Node right = ctx.node().children()[1];
            if(!is_live(left.value())||!is_live(right.value())) {
                log(red("Unable to do e stage assignment handler because one of the values is missing"));
                return;
            }
            ctx.node().quals().push((make_node(live_qual))); //This liveness thing is kludgy but I'm tired and just trying to get ctx.source = "wub" to work
            process_node(ctx,left);
            if(left.has_qual(live_qual)) { //Transfer liveness
                if(!right.has_qual(live_qual)) {
                    right.value().quals().push(make_node(live_qual));
                }
            } else {
                ctx.node().quals().pop(); //Kill it
            }

            int const_at = left.value().find_qual(constant_qual);
            if(const_at==-1) { //A value is constant if it's only asigned once
                if(!left.has_qual(assigned_qual)) {
                    left.value().quals().push(make_node(constant_qual));
                }
            } else {
                left.value().quals().removeAt(const_at);
            }

            if(!left.has_qual(assigned_qual)) {
                left.value().quals().push(make_node(assigned_qual));
            }
        };
        void e_stage_binary_handler(Context& ctx) {
            Node left = ctx.node().children()[0];
            Node right = ctx.node().children()[1];
            if(!is_live(left.value())||!is_live(right.value())) {
                log(red("Unable to do e stage binary handler because one of the values is missing"));
                return;
            }
            if(ctx.node().has_qual(live_qual)) {
                if(!right.has_qual(live_qual)) {
                    right.value().quals().push(make_node(live_qual));
                }
                if(!left.has_qual(live_qual)) {
                    left.value().quals().push(make_node(live_qual));
                }
            } else {
                if(left.has_qual(live_qual)) { //Transfer liveness
                    ctx.node().quals().push((make_node(live_qual)));
                    if(!right.has_qual(live_qual)) {
                        right.value().quals().push(make_node(live_qual));
                    }
                } else if(right.has_qual(live_qual)) {
                    ctx.node().quals().push((make_node(live_qual)));
                    left.value().quals().push(make_node(live_qual));
                }
            }
        };
        void e_scoped_handler(Context& ctx) {
            if(!ctx.node().scopes().empty()) {
                Node scope = ctx.node().scopes()[0];
                for(int i=0;i<scope.children().length();i++) {
                    if(scope.children()[i].has_qual(live_qual)) {
                        ctx.node().quals().push(make_node(live_qual));
                        break;
                    }
                }
            }
            if(ctx.node().has_qual(live_qual)) {
                for(int i=0;i<ctx.node().children().length();i++) {
                    Node c = ctx.node().children().get(i);
                    if(is_live(c.value())&&!c.has_qual(live_qual)) {
                        c.value().quals().push(make_node(live_qual));
                    } else if(!is_live(c.value())) {
                        c.quals().push(make_node(live_qual));
                    }
                }
            }
            standard_sub_process(ctx);
        }

        void e_pass_prune_dead(node_col nodes) {
            for(int i=nodes.length()-1;i>=0;i--) {
                Node node = nodes[i];
                log("Pruning: ",node_info(node));
                e_pass_prune_dead(node.children());
                for(int s = 0;s<node.scopes().length();s++) {
                    if(node.scopes()[s].owner()==node)
                        e_pass_prune_dead(node.scopes()[s].children());
                }
                if(!node.has_qual(live_qual)) {
                    log(yellow("Recycled: "+Ptr_as_string(nodes.get(i))));
                    recycle_node(nodes.get(i));
                    nodes.removeAt(i);
                }
            }
        };

        uint32_t bumpid = add_function("BUMP",[](Context& ctx){});

        void m_stage_assignment_handler(Context& ctx) {
            standard_sub_process(ctx); //Not sure what to do with this yet
        };
        bool should_allocate_data(Value v) {
            return (is_live(v)&&!is_live(v.data_ptr())&&v.type()!=0);
        }
        bool should_recycle_data(Value v) {
            return (is_live(v)&&is_live(v.data_ptr())&&v.type()!=0);
        }
        void allocate_data(Value v) {
            v.data_ptr(get_ticket(data_store_id,v.size(),v.type()));
            if(v.data_col().empty()) {
                v.data_col().push_default();
            }
        }

        uint32_t ptr_push_op_id = overload_type(ptr_id,"<<any","PTR_PUSH_OP",deadptr,[this](Context& ctx){
            standard_sub_process(ctx);
            Node left = ctx.node().children()[0];
            Node right = ctx.node().children()[1];
            void* lv = left.value().get();
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("No left value in ptr push op")); return;});
            Col& col = resolve_to_col(*(Ptr*)lv);
            col.push(right.value().get());
        });

        uint32_t precompile_brace = add_token_combo("precompile_brace",'#','#');
        uint32_t comment_brace = add_token_combo("comment_brace",'/','/');

        void init() override {
            register_type("list",ptr_id,sizeof(Ptr));

            overload_type(ptr_id,".\"push\"",ptr_push_id);
            overload_type(ptr_id,".\"take\"",ptr_take_id,make_value());
            overload_type(ptr_id,".\"length\"",ptr_length_id,make_value(int_id,4));

            //overload_type(string_id,"+string",string_append_id,make_value(string_id,sizeof(Ptr),0,char_id,1));
            overload_type(string_id,".\"length\"",ptr_length_id,make_value(int_id,4));
            overload_type(string_id,".\"clear\"",ptr_clear_id);
            overload_type(string_id,".\"substr\"",string_substr_id,make_value(string_id,sizeof(Ptr),0,char_id,1));
            overload_type(string_id,".\"slice\"",string_slice_id,make_value(string_id,sizeof(Ptr),0,char_id,1));
            overload_type(string_id,".\"find\"",string_find_id,make_value(int_id,4));

            overload_type(string_id,"|*^+int",reg_id("THRONGLIZE"),make_value(ptr_id,sizeof(Ptr),0,int_id,4));

            
            x_handlers[ptr_take_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                Value cv = right.value();
                Col& col = resolve_to_col(*(Ptr*)left.value().get());
                if(!right.children().empty()) {
                    cv = right.children()[0].value();
                }
                if(cv.type()==int_id) {
                    int index = *(int*)cv.get();
                    ctx.node().value().set(col.get((uint32_t)index));
                    col.removeAt(index);
                }
            };
            x_handlers[ptr_push_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                if(!right.children().empty()) {
                    right = right.children()[0];
                }
                void* lv = left.value().get();
                DEBUG_ONLY(if(ERROR_FLAG) {log(red("No left value in ptr push")); return;});
                Col& col = resolve_to_col(*(Ptr*)lv);
                col.push(right.value().get());
            };
            x_handlers[ptr_length_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                Col& col = resolve_to_col(*(Ptr*)left.value().get());
                int len = col.length();
                ctx.node().value().set((void*)&len);
            };
            x_handlers[string_substr_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                if(!is_live(ctx.node().value().data_ptr())||!is_live(*(Ptr*)ctx.node().value().get())) {
                    Ptr ticket = get_ticket(data_store_id,1,char_id);
                    ctx.node().value().set((void*)&ticket);
                }
                string target(*(Ptr*)ctx.node().value().get());
                Ptr ptr = *(Ptr*)left.value().get();
                int from = *(int*)right.children()[0].value().get();
                int to = target.length()-from;
                if(right.children().length()>1) {
                    to = *(int*)right.children()[1].value().get();
                }
                target.col().clear();
                for(int i=from;i<from+to;i++) {
                    target.push(*(char*)resolve_to_col(ptr)[i]);
                }
            };
            x_handlers[string_slice_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                if(!is_live(ctx.node().value().data_ptr())||!is_live(*(Ptr*)ctx.node().value().get())) {
                    Ptr ticket = get_ticket(data_store_id,1,char_id);
                    ctx.node().value().set((void*)&ticket);
                }
                string target(*(Ptr*)ctx.node().value().get());
                Ptr ptr = *(Ptr*)left.value().get();
                int from = 0;
                int to = 0; //Add some deffensive checking here later
                if(right.children()[0].value().type()==string_id) {
                    string refstr(*(Ptr*)right.children()[0].value().get());
                    string fromstr(ptr);
                    from = fromstr.find(refstr,0,*(int*)right.children()[1].value().get())+1;
                    to = fromstr.find(refstr,0,*(int*)right.children()[2].value().get());
                } else {
                    from = *(int*)right.children()[0].value().get();
                    to = *(int*)right.children()[1].value().get();
                }
                target.col().clear();
                for(int i=from;i<to;i++) {
                    target.push(*(char*)resolve_to_col(ptr)[i]);
                }
            };
            x_handlers[string_find_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                Ptr ptr = *(Ptr*)left.value().get();
                Col& tcol = resolve_to_col(ptr);
                string refstr(*(Ptr*)right.children()[0].value().get());
                int start_at = 0;
                if(right.children().length()>1) {
                    start_at = *(int*)right.children()[1].value().get();
                }
                int nth_of = 1;
                if(right.children().length()>2) {
                    nth_of = *(int*)right.children()[2].value().get();
                }
                int found_id = string(ptr).find(refstr,start_at,nth_of);
                ctx.node().value().set((void*)&found_id);
            };

            add_function("resolve_as_Ptr",[this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value().set(resolve_ptr(*(Ptr*)ctx.node().children()[0].value().get()));
            },sizeof(Ptr),ptr_id);

            add_function("resolve_as_Node",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr resolved = *(Ptr*)ctx.node().children()[0].value().get();
                ctx.node().value().set(resolve_ptr(resolved));
            },sizeof(Ptr),node_id);

            add_function("cast_to_Node",[this](Context& ctx){
                standard_sub_process(ctx);
                Node n = (Node&)*(Ptr*)ctx.node().children()[0].value().get();
                if(n.cachelevel==3) n.cache = &types; //Figure out later why this isnt' working with string_to_Ptr
                ctx.node().value().set((void*)&n);
            },sizeof(Ptr),node_id);

            add_function("makePtr3",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr& p = *(Ptr*)ctx.node().value().get();
                uint32_t pool = *(int*)ctx.node().children()[0].value().get();
                uint32_t idx = *(int*)ctx.node().children()[1].value().get();
                uint32_t sidx = *(int*)ctx.node().children()[2].value().get();
                p.pool = pool; p.idx = idx; p.sidx = sidx;
                p.cachelevel = 3; p.cache = &types;
            },sizeof(Ptr),ptr_id);

            add_function("value_as_string",[this](Context& ctx){
                standard_sub_process(ctx);
                string output = resolve_string_ticket(ctx.node());
                output = value_as_string(*(Ptr*)ctx.node().children()[0].value().get());
            },sizeof(Ptr),string_id);

            add_function("is_live",[this](Context& ctx){
                standard_sub_process(ctx);
                bool b = false;
                if(ctx.node().children()[0].value().type()==ptr_id) {
                    b = is_live(*(Ptr*)ctx.node().children()[0].value().get());
                }
                ctx.node().value().set((void*)&b);
            },1,bool_id);

            add_function("call_owner_as_string",[this](Context& ctx){
                //We *don't* standard sub process because we don't want to emit to source
                Node n = ctx.node().children()[0];
                Ptr ownerptr = n.scopes()[0].owner();
                string output = resolve_string_ticket(ctx.node());
                output = Ptr_to_string(ownerptr,ownerptr.cachelevel);
            },sizeof(Ptr),string_id);

            add_function("call_owner",[this](Context& ctx){
                //We *don't* standard sub process because we don't want to emit to source
                Node n = ctx.node().children()[0];
                Node owner = n.scopes()[0].owner();
                ctx.node().value().set((void*)&owner);
            },sizeof(Ptr),node_id);
            

            Handler discard = [this](Context& ctx){
                if(ctx.index()>0) {
                    ctx.index()--;
                    ctx.result().get(ctx.index()).quals() << turn_into_token(ctx.result().take(ctx.index()+1));
                } else if(is_live(ctx.root())) {
                    ctx.root().quals() << turn_into_token(ctx.result().take(ctx.index()));
                }
            };
            t_handlers[end_id] = discard;
            t_handlers[comma_id] = discard;
            t_handlers[comment_id] = discard;

            a_handlers[make_tokenized_keyword("register")] = [this](Context& ctx){
                ctx.node().quals() << turn_into_token(ctx.result().take(ctx.index()+1));
                make_tokenized_keyword(ctx.node().quals().last().name().to_std());
            };
            a_handlers[make_tokenized_keyword("newstage")] = [this](Context& ctx){
                reg_stage(ctx.result().take(ctx.index()+1).name().to_std());
            };

            add_function("set_binding_powers",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t type = *(int*)ctx.node().children()[0].value().get();
                int lbp = *(int*)ctx.node().children()[1].value().get();
                int rbp = *(int*)ctx.node().children()[2].value().get();
                set_binding_powers(type,lbp,rbp);
                print("Set the bidning powers for ",labels[type]," to ",lbp," ",rbp);
            });

            //Revist this idea later once we have proper closures
            add_function("add_function",[this](Context& ctx){
                string label = resolve_string_ticket(ctx.node().children()[0]);
                Node n = (Node&)*(Ptr*)ctx.node().children()[1].value().get();
                add_function(label.to_std(),[this,n](Context& ctx) mutable {
                    // g_ptr<Stage> old_stage = active_stage;

                    // start_stage(x_handlers);
                    // standard_travel_pass(this_node.scopes()[0],ctx);
                    // start_stage(old_stage);
                    
                });
            });

            add_function("C0",[this](Context& ctx){
                Node c0 = ctx.sub().node().children()[0];
                ctx.node().value().set((void*)&c0);
            },sizeof(Ptr),node_id);
            add_function("C1",[this](Context& ctx){
                Node c1 = ctx.sub().node().children()[1];
                ctx.node().value().set((void*)&c1);
            },sizeof(Ptr),node_id);
    
            overload_type(node_id,".\"c0\"","NODE_c0",make_value(node_id,sizeof(Ptr)),[this](Context& ctx){
                standard_sub_process(ctx);
                Node n = (Node&)*(Ptr*)ctx.node().children()[0].value().get();
                Node c0 = n.children()[0];
                ctx.node().value().set((void*)&c0);
            });
            overload_type(node_id,".\"c1\"","NODE_c1",make_value(node_id,sizeof(Ptr)),[this](Context& ctx){
                standard_sub_process(ctx);
                Node n = (Node&)*(Ptr*)ctx.node().children()[0].value().get();
                Node c1 = n.children()[1];
                ctx.node().value().set((void*)&c1);
            });

            r_handlers[qmark_id] = [this](Context& ctx){
                standard_sub_process(ctx); //Because the second child is a : so  we just grab it's left value, so that other things know how to resolve against the qmark
                Value lv = ctx.node().children()[1].children()[0].value();
                ctx.node().value(lv); //Just the same for now, in the future we can be a bit fancier
                resolve_overload(ctx);
            };
            x_handlers[qmark_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(*(bool*)ctx.node().children()[0].value().get()) {
                    ctx.node().value(ctx.node().children()[1].children()[0].value());
                }
                else {
                    ctx.node().value(ctx.node().children()[1].children()[1].value());
                }
            };


            //Temporary kludge methods until Ptrs become proper heterogenous systems
            add_function("getpool",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr p = *(Ptr*)ctx.node().children()[0].value().get();
                ctx.node().value().set((void*)&p.pool);
            },4,int_id);
            add_function("setpool",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr& p = *(Ptr*)ctx.node().children()[0].value().get();
                int val = *(int*)ctx.node().children()[1].value().get();
                p.pool = val;
            });
            add_function("getidx",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr p = *(Ptr*)ctx.node().children()[0].value().get();
                ctx.node().value().set((void*)&p.idx);
            },4,int_id);
            add_function("setidx",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr& p = *(Ptr*)ctx.node().children()[0].value().get();
                int val = *(int*)ctx.node().children()[1].value().get();
                p.idx = val;
            });
            add_function("getsidx",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr p = *(Ptr*)ctx.node().children()[0].value().get();
                ctx.node().value().set((void*)&p.sidx);
            },4,int_id);
            add_function("setsidx",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr& p = *(Ptr*)ctx.node().children()[0].value().get();
                int val = *(int*)ctx.node().children()[1].value().get();
                p.sidx = val;
            });

            uint32_t ptr_setsidx_id = add_binding_token_combo("PTR_SETSIDX_OP",8,2,'|','S','=');
            t_handlers[ptr_setsidx_id] = [this](Context& ctx){
                standard_sub_process(ctx); 
                ctx.node().value(make_value(ptr_id,sizeof(Ptr)));  
            };
            x_handlers[ptr_setsidx_id] = [this](Context& ctx){
                standard_sub_process(ctx); 
                Ptr p = *(Ptr*)ctx.node().children()[0].value().get();
                if(!is_live(ctx.node().value().data_ptr())) {ctx.node().value().init_data();}
                Ptr& output = *(Ptr*)ctx.node().value().get();
                int val = *(int*)ctx.node().children()[1].value().get();
                p.sidx = val;
                output = p;
            };





            add_function("newline",[this](Context& ctx){
                uspan->newline(children_to_string(ctx,ctx.node().children()));
            });
            add_function("log",[this](Context& ctx){
                uspan->log(children_to_string(ctx,ctx.node().children()));
            });
            add_function("udump",[this](Context& ctx){
                uspan->print_all();
            });
            add_function("ussw",[this](Context& ctx){ //uspan_setup_standard_watcher
                setup_uspan_standard_watchers();
            });
            add_function("ursw",[this](Context& ctx){ //uspan_remove_standard_watcher
                for(int i=0;i<watchers.length();i++) {if(watchers[i].label=="uspan_core") watchers.removeAt(i); break;}
            });
            add_function("ures",[this](Context& ctx){ //uspan_restart
                uspan->restart();
            });
            add_function("utg",[this](Context& ctx){ //uspan_time_get
                string output = resolve_string_ticket(ctx.node());
                output = ftime(uspan->get_root_time());
            },sizeof(Ptr),string_id);
            add_function("uts",[this](Context& ctx){ //uspan_time_start
                uspan->restart_root_time();
            });
            add_function("ute",[this](Context& ctx){ //uspan_time_end
                string output = resolve_string_ticket(ctx.node());
                output = ftime(uspan->end_root_time());
            },sizeof(Ptr),string_id);

            add_function("tstart",[this](Context& ctx){ //uspan_time_start
                uspan->start_timer(children_to_string(ctx,ctx.node().children()));
            });
            add_function("ttime",[this](Context& ctx){ //uspan_time_end
                string output = resolve_string_ticket(ctx.node());
                output = ftime(uspan->get_time(children_to_string(ctx,ctx.node().children())));
            },sizeof(Ptr),string_id);
            add_function("tstop",[this](Context& ctx){ //uspan_time_end
                string output = resolve_string_ticket(ctx.node());
                output = ftime(uspan->end_timer(children_to_string(ctx,ctx.node().children())));
            },sizeof(Ptr),string_id);

            tokenizer_state_functions[comment_brace] = [this](Context& ctx) {
                char c = ctx.source().at(ctx.index());
                if(c == '/' && (ctx.index()+1<ctx.source().length()&&ctx.source().at(ctx.index()+1)=='/')) {
                    ctx.node().type(comment_id);
                    ctx.state(0);
                    ctx.result().removeAt(ctx.index());
                    ctx.index()++;
                    ctx.node().name().push("//");
                } else if(c == '\n') {
                    at_y += 1.0f;
                    at_x = -1.0f;
                    ctx.node().type(comment_id);
                    ctx.state(0);
                } else {
                    ctx.node().name().push(c);
                }
            };


            tokenizer_state_functions[precompile_brace] = [this](Context& ctx) {
                char c = ctx.source().at(ctx.index());

                if(ctx.node().quals().empty()) {
                    Node open_token = copy_as_token(ctx.node());
                    ctx.node().quals() << open_token;
                    ctx.node().type(precompiling_id);
                    ctx.node().name().col().clear();
                    ctx.node().x(at_x);
                    ctx.node().y(at_y);
                }

                if(c == '#'&&(ctx.index()+1<ctx.source().length()&&ctx.source().at(ctx.index()+1)=='#')) {
                    Node closer = copy_as_token(ctx.node().quals()[0]);
                    closer.x(at_x); closer.y(at_y);
                    ctx.node().quals() << closer;

                    ctx.state(0);
                    //ctx.result().removeAt(ctx.index());
                    ctx.index()++;
                    at_x+=1.0f;

                    std::string oldsrc = ctx.source().to_std(); //Remember to just fix the source in context (when I'm not trying to ship a prototype)

                    // list<Watcher> watcher_daycare; //We don't log things like precompiling stages (for now)
                    // watcher_daycare << watchers;
                    // watchers.clear();

                    std::string oldlabel = unit_label;
                    unit_label = oldlabel+"pc";

                    Node root = process(ctx.node().name().to_std());
                    ctx.node().name().col().clear(); //To avoid stinking up the nodenet and memory dump
                    compile(root,false);
                    start_logged_stage(x_handlers);
                    standard_travel_pass(root);
                    end_logged_stage();

                    unit_label = oldlabel;
                    //watchers << watcher_daycare; //Restore watchers

                    ctx.node().scopes() << root;

                    ctx.source(oldsrc);
                } else if(c=='\n') {
                    at_y += 1.0f; at_x = -1.0f;
                    ctx.node().name().push(c);
                }
                else {
                    ctx.node().name().push(c);
                }
            };


            


            x_handlers[make_tokenized_keyword("as_data")] = [this](Context& ctx){
                Value rv = ctx.node().children()[0].value();
                ctx.node().value(make_value(ptr_id,sizeof(Ptr)));
                ctx.node().value().set((void*)&rv.data_ptr());
            };

            r_handlers[var_decl_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node().value());
            };
            r_handlers[to_unary_id(star_id)] = [this](Context& ctx){ //To get size data and such from the type
                standard_sub_process(ctx);
                fire_quals(ctx,ctx.node().value());
                resolve_overload(ctx);
            };


            r_handlers[to_decl_id(star_id)] = [this](Context& ctx){
                if(ctx.node().value().type()!=ptr_id) {
                    ctx.node().value().type(ptr_id);
                    ctx.node().value().size(sizeof(Ptr));
                    ctx.node().value().quals().insert(0,make_node(ptr_id,"Ptr",make_value(ptr_id,sizeof(Ptr)),ctx.node().in_scope()));
                    fire_quals(ctx,ctx.node().value());
                    standard_sub_process(ctx);
                    resolve_overload(ctx);
                }
            };
            x_handlers[to_decl_id(star_id)] = [this](Context& ctx){
                if(!ctx.node().children().empty()) {
                    standard_sub_process(ctx);
                    Node child = ctx.node().children()[0];
                    Ptr p = *(Ptr*)child.value().get();
                    ctx.node().value().set((void*)&p);
                }
            };
            r_handlers[to_unary_id(amp_id)] = [this](Context& ctx){
                if(ctx.node().children().length()!=1) return;
                if(ctx.node().value().type()!=ptr_id) {
                    ctx.node().value().type(ptr_id);
                    ctx.node().value().size(sizeof(Ptr));
                    ctx.node().value().quals().insert(0,make_node(ptr_id,"Ptr",make_value(ptr_id,sizeof(Ptr)),ctx.node().in_scope()));
                    fire_quals(ctx,ctx.node().value());
                    standard_sub_process(ctx);
                    resolve_overload(ctx);
                }
            };

            x_handlers[to_unary_id(amp_id)] = [this](Context& ctx){
                if(ctx.node().children().length()!=1) return;
                standard_sub_process(ctx);
                Node child = ctx.node().children()[0];
                Ptr p = child.value().data_ptr();
                ctx.node().value().set((void*)&p);
            };
            x_handlers[to_unary_id(star_id)] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node child = ctx.node().children()[0];
                Ptr p = *(Ptr*)child.value().get();
                ctx.node().value().data_ptr(p);
                ctx.node().value().type(resolve_to_col(p).tag);
                ctx.node().value().size(resolve_to_col(p).element_size);
            };


            x_handlers[var_decl_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node().value());
            };
            r_handlers[prefix_ptr_id] = [this](Context& ctx){
                if(is_live(ctx.value())) {
                    if(ctx.value().quals().length()>1) {
                        int i = 0;
                        while(i<ctx.value().quals().length()) {
                            Node q = ctx.value().quals()[i];
                            if(q.type()==ptr_id&&i<ctx.value().quals().length()-1) {
                                Node left = ctx.value().quals()[i+1];
                                ctx.value().sub_type(left.value().type());
                                ctx.value().sub_size(left.value().size());
                                break;
                            }
                            i++;
                        }
                    } else {
                        //It's just a normal Ptr
                        // print(red("prefix_ptr_id::r_handler missing type it points to!"));
                        // print(node_to_string(ctx.node()));
                    }
                }
            };
            x_handlers[prefix_ptr_id] = [this](Context& ctx){
                if(is_live(ctx.value())) {
                    if(ctx.value().sub_type()!=0) {
                        Ptr ticket = get_ticket(data_store_id,ctx.value().sub_size(),ctx.value().sub_type());
                        ctx.value().set((void*)&ticket);
                    }
                }
            };
            x_handlers[prefix_string_id] = [this](Context& ctx){
                if(is_live(ctx.value())&&ctx.value().quals()[0]==ctx.qual()) {
                    Ptr ticket = get_ticket(name_store_id,1,char_id);
                    ctx.value().set((void*)&ticket);
                }
            };
            x_handlers[prefix_node_id] = [this](Context& ctx){
                if(is_live(ctx.value())&&ctx.value().quals()[0]==ctx.qual()&&!is_live(ctx.value().data_ptr())) {
                    Node n = make_node();
                    ctx.node().value().init_data();
                    ctx.value().set((void*)&n);
                }
            };
            x_handlers[prefix_value_id] = [this](Context& ctx){
                if(is_live(ctx.value())&&ctx.value().quals()[0]==ctx.qual()) {
                    Value v = make_value();
                    ctx.node().value().init_data();
                    ctx.value().set((void*)&v);
                }
            };
            x_handlers[prefix_context_id] = [this](Context& ctx){
                if(is_live(ctx.value())&&ctx.value().quals()[0]==ctx.qual()) {
                    Context c = make_context();
                    ctx.node().value().init_data();
                    ctx.value().set((void*)&c);
                }
            };

            x_handlers[literal_id] = [this](Context& ctx){
                std::string name = ctx.node().name().to_std();
                uint32_t vtype = ctx.node().value().type();
                if(vtype==int_id) {
                    int i = std::stoi(name); ctx.node().value().set((void*)&i);
                } else if(vtype==float_id) {
                    float f = std::stof(name); ctx.node().value().set((void*)&f);
                } else if(vtype==ptr_id) {
                    Ptr p = string_to_Ptr(name); 
                    if(p.cachelevel==3) {p.cache = &types;} //Add fillins for other caches somehow later
                    ctx.node().value().set((void*)&p);
                } else if(vtype==string_id) { //This is a race condition
                    Ptr p = get_ticket(name_store_id,1,char_id); string s(p); s = name; ctx.node().value().set((void*)&p);
                } else if(vtype==bool_id) {
                    bool b = (name=="true"||name=="1");
                    ctx.node().value().set((void*)&b);
                } else if(vtype==char_id) {
                    char c = name.empty() ? '\0' : name[0];
                    ctx.node().value().set((void*)&c);
                } else if(vtype==null_id) { //For future use
                    Ptr dead = deadptr;
                    ctx.node().value().set((void*)&dead);
                } else {
                    throw_error("No way to handle value type ",labels[vtype],"!");
                }
            };

            r_handlers[to_unary_id(hash_id)] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(int_id,4));
                resolve_overload(ctx);
            };
            x_handlers[to_unary_id(hash_id)] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node c = ctx.node().children()[0];
                std::string search_for = c.name().to_std();
                uint32_t type = 0;
                type = labels_lookup.getOrDefault(c.name().to_std(),type);
                ctx.node().value().set((void*)&type);
            };

            int l_lbp = 5; int l_rbp = 3;
            set_binding_powers(hash_id,l_lbp,l_rbp);
            //Double check what the precedence for these should be later
            uint32_t label_type_combo = add_binding_token_combo("LABEL_TYPE",l_lbp,l_rbp,'L','#');
            r_handlers[label_type_combo] = [this](Context& ctx){
                standard_sub_process(ctx); 
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));  
                resolve_overload(ctx); 
            };
            x_handlers[label_type_combo] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node c = ctx.node().children()[0];
                string output = resolve_string_ticket(ctx.node());
                uint32_t type = *(int*)c.value().get();
                output = labels[type];
            };

            uint32_t node_type_combo = add_binding_token_combo("NODE_TYPE",l_lbp,l_rbp,'N','#');
            r_handlers[node_type_combo] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(int_id,4));
                resolve_overload(ctx);
            };
            x_handlers[node_type_combo] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node c = ctx.node().children()[0];
                uint32_t type = c.type();
                ctx.node().value().set((void*)&type);
            };
            uint32_t label_node_type_combo = add_binding_token_combo("LABEL_NODE_TYPE",l_lbp,l_rbp,'S','N','#');
            r_handlers[label_node_type_combo] = [this](Context& ctx){
                standard_sub_process(ctx); 
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));  
                resolve_overload(ctx); 
            };
            x_handlers[label_node_type_combo] = [this](Context& ctx){
                Node c = ctx.node().children()[0];
                string output = resolve_string_ticket(ctx.node());
                uint32_t type = c.type();
                output = labels[type];
            };
            uint32_t value_type_combo = add_binding_token_combo("VALUE_TYPE",l_lbp,l_rbp,'V','#');
            r_handlers[value_type_combo] = [this](Context& ctx){                 
                standard_sub_process(ctx);
                ctx.node().value(make_value(int_id,4));
                resolve_overload(ctx);
            };
            x_handlers[value_type_combo] = [this](Context& ctx){
                Node c = ctx.node().children()[0];
                uint32_t type = 0;
                if(is_live(c.value())) {
                    type = c.value().type();
                }
                ctx.node().value().set((void*)&type);
            };
            uint32_t label_value_type_combo = add_binding_token_combo("LABEL_VALUE_TYPE",l_lbp,l_rbp,'S','V','#');
            r_handlers[label_value_type_combo] = [this](Context& ctx){
                standard_sub_process(ctx); 
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));  
                resolve_overload(ctx); 
            };
            x_handlers[label_value_type_combo] = [this](Context& ctx){
                Node c = ctx.node().children()[0];
                string output = resolve_string_ticket(ctx.node());
                uint32_t type = 0;
                if(is_live(c.value())) {
                    type = c.value().type();
                }
                output = labels[type];
            };

            x_handlers[to_prefix_id(global_qual)] = [this](Context& ctx){
                if(ctx.node().type()==var_decl_id) {
                    value_col vcol(uid,unitdata_col,global_value_table_idx);
                    vcol.put(ctx.node().name().to_std(),ctx.value());
                } else if(ctx.node().type()==func_decl_id) {
                    node_col ncol(uid,unitdata_col,global_node_table_idx);
                    ncol.put(ctx.node().name().to_std(),ctx.node().scopes()[0]);
                    value_col vcol(uid,unitdata_col,global_value_table_idx);
                    vcol.put(ctx.node().name().to_std(),ctx.value());
                }
            };
           
            r_handlers[ctx_id] = [this](Context& ctx){
                ctx.node().value(make_value(context_id,sizeof(Ptr)));
                ctx.node().value().quals().push(make_node(live_qual));
            };
            x_handlers[ctx_id] = [this](Context& ctx){
                if(is_live(ctx.sub())) {
                    Context sctx = ctx.sub();
                    ctx.node().value().set((void*)&sctx);
                } else {
                    ctx.node().value().set((void*)&ctx);
                }
            };
            r_handlers[lctx_id] = [this](Context& ctx){
                ctx.node().value(make_value(context_id,sizeof(Ptr)));
                ctx.node().value().quals().push(make_node(live_qual));
            };
            x_handlers[lctx_id] = [this](Context& ctx){
                ctx.node().value().set((void*)&ctx);
            };

            r_handlers[to_string_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(string_id,sizeof(Ptr)));
                ctx.node().value().init_data();
                Ptr ticket = get_ticket(data_store_id,1,char_id);
                ctx.node().value().set((void*)&ticket);
                resolve_overload(ctx);
            };
            x_handlers[to_string_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                string str(*(Ptr*)ctx.node().value().get());
                str = value_as_string(ctx.node().children()[0].value());
            };

            r_handlers[labels_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));
                Ptr ticket = get_ticket(data_store_id,1,char_id);
                ctx.node().value().set((void*)&ticket);
                resolve_overload(ctx);
            };
            x_handlers[labels_id] = [this](Context& ctx){
                if(!ctx.node().children().empty()) {
                    standard_sub_process(ctx);
                    string label(*(Ptr*)ctx.node().value().get());
                    uint32_t p = *(uint32_t*)ctx.node().children()[0].value().get();
                    label = labels[p]; 
                }
            };

            r_handlers[to_type_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(int_id,4));
                ctx.node().value().init_data();
                resolve_overload(ctx);
            };
            x_handlers[to_type_id] = [this](Context& ctx){
                //Add caching for this later
                standard_sub_process(ctx);
                std::string search_for = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
                for(auto e : labels.entrySet()) {
                    if(e.value == search_for) {
                        ctx.node().value().set((void*)&e.key);
                        return;
                    }
                }
            };

            // x_handlers[make_tokenized_keyword("make_value")] = [this](Context& ctx){
            //     standard_sub_process(ctx);
            //     int type = *(int*)ctx.node().children()[0].value().get();
            //     int size = *(int*)ctx.node().children()[1].value().get();
            //     ctx.sub().node().value(make_value(type,size));
            //     ctx.sub().node().value().init_data();
            // };


            s_handlers[string_id] = [this](Context& ctx){
                if(ctx.index()+1>=ctx.result().length()) return;

                Node right = ctx.result()[ctx.index()+1];
                if(right.type()==lbrace_id) {
                    ctx.node().children() << ctx.result().take(ctx.index()+1);
                    ctx.node().quals() << copy_as_token(ctx.node().children().last());
                    ctx.node().type(node_block_id);
                    ctx.node().children().last().name(ctx.node().name().to_std());
                    ctx.node().children().last().x(-1.0f); ctx.node().children().last().y(-1.0f);
                }
            };
            t_handlers[node_block_id] = [this](Context& ctx){
                for(auto e : labels.entrySet()) {
                    if(e.value==ctx.node().name().to_std()) {
                        ctx.node().sub_type(e.key);
                        break;
                    }
                }
                if(ctx.node().sub_type()==0) {
                    print(red("node_block:t_handler unrecognized node type: "+ctx.node().name().to_std()));
                }
            };
            x_handlers[node_block_id] = [this](Context& ctx){
                ctx.state(standard_travel_pass(ctx.node().scopes()[0]));
            };

            x_handlers[make_tokenized_keyword("test")] = [this](Context& ctx){
                print("THIS SHOULD NOT PRINT");
            };

            r_handlers[in_id] = [this](Context& ctx){
                if(!ctx.node().children().empty()&&is_live(ctx.node().in_scope())&&is_live(ctx.node().in_scope().owner())) {
                    ctx.node().quals() << copy_as_token(ctx.node());
                    ctx.node().x(-1.0f); ctx.node().y(-1.0f);
                    ctx.node().name("in "+ctx.node().children()[0].name().to_std()+" "+labels[ctx.node().in_scope().owner().sub_type()]);
                    if(!ctx.node().scopes().empty()) {
                        ctx.node().scopes()[0].name(ctx.node().name().to_std());
                    }
                }
            };
            x_handlers[in_id] = [this](Context& ctx){
                Node this_node = ctx.node();
                uint32_t target_type = ctx.node().in_scope().owner().sub_type();
                std::string stage_name = ctx.node().children()[0].name().to_std();
                if(!stages.hasKey(stage_name)) {
                    print(red("in_id:x_handler unknown stage "+stage_name));
                    return;
                }
                g_ptr<Stage> stage = stages.get(stage_name);
                (*stage)[target_type] = [this,this_node](Context& ctx) mutable {
                    g_ptr<Stage> old_stage = active_stage;

                    start_stage(x_handlers);
                    standard_travel_pass(this_node.scopes()[0],ctx);
                    start_stage(old_stage);
                    
                };

                uint32_t stage_id = *(uint32_t*)types[handler_type_id][stages_id].get(stage_name);
                while(types[handler_type_id][target_type].length()<=stage_id) types[handler_type_id][target_type].push_default();
                Node target_scope = this_node.scopes()[0];
                types[handler_type_id][target_type].set(stage_id,(void*)&target_scope);
            };

            n_handlers[on_id] = [this](Context& ctx){ //Add a proper n_take_right later, like we had in GDSL
                ctx.index()++;
                while(ctx.result().get(ctx.index()).type()!=lbrace_id) {
                    ctx.node().children().push(ctx.result().take(ctx.index()));
                }
            };
            x_handlers[on_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t target_type = ctx.node().in_scope().owner().sub_type();
                string instr = resolve_string_ticket(ctx.node().children()[0]);

                Value v = deadptr;
                if(ctx.node().children().length()>1) {
                    v = ctx.node().children()[1].value();
                }

                Node this_node = ctx.node();
                overload_type(target_type, instr.to_std(), instr.to_std()+"_overload", v, [this, this_node](Context& ctx) mutable {
                    g_ptr<Stage> old_stage = active_stage;
                    start_stage(x_handlers);
                    standard_travel_pass(this_node.scopes()[0], ctx);
                    start_stage(old_stage);
                });
            };


            //This implicit scoping doesn't work yet, add it as a proper feature later
            //To work, the s handlers for rbrace need to properly descend scopes so they can work with implictly scoped nodes containing lbraces
            //Like else if
            s_handlers[if_id] = [this](Context& ctx){
                if(ctx.index()+1>=ctx.result().length()) return;

                Node right = ctx.result()[ctx.index()+1];

                if(right.type()==lbrace_id) {
                    ctx.node().children() << ctx.result().take(ctx.index()+1);
                } else if(is_live(right)) {
                    ctx.index()++;
                    process_node(ctx,right);
                    ctx.index()--;
                    Node newscope = make_node(scope_id,ctx.node().name().to_std(),deadptr,deadptr);
                    ctx.node().scopes() << newscope;
                    newscope.owner(ctx.node());
                    place_node_in_scope(right,newscope);
                    newscope.children() << ctx.result().take(ctx.index()+1);
                }
            };
            s_handlers[else_id] = s_handlers[if_id];

            x_handlers[if_id] = [this](Context& ctx) {
                process_node(ctx, ctx.node().children()[0]);
                DEBUG_ONLY(if(ERROR_FLAG) {return;});
                if(*(bool*)ctx.node().children()[0].value().get()) {
                    ctx.state(standard_travel_pass(ctx.node().scopes()[0],ctx.sub()));
                }
                else if(ctx.node().scopes().length()>1) {
                    ctx.state(standard_travel_pass(ctx.node().scopes()[1],ctx.sub()));
                }
            };
            t_handlers[else_id] = [this](Context& ctx) {
                if(ctx.index()>0) {
                    Node left_if = deadptr;      
                    if(ctx.left().type()==if_id) {
                        left_if = ctx.left();
                        while(left_if.scopes().length()==2) { //To chain else ifs
                            Node lscope = left_if.scopes()[1];
                            if(!lscope.children().empty()) {
                                if(lscope.children()[0].type()==if_id) {
                                    left_if = lscope.children()[0];
                                }
                            }
                        }
                    }
                    if(is_live(left_if)) {
                        ctx.node().scopes()[0].owner(left_if);
                        left_if.scopes() << ctx.node().scopes()[0];
                        left_if.quals() << turn_into_token(ctx.node());
                        ctx.result().removeAt(ctx.index());
                        ctx.index()--;
                    } 
                }
            };

            r_handlers[while_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                ctx.node().value(make_value());
            };
            x_handlers[while_id] = [this](Context& ctx) {
                while(true) {
                    DEBUG_ONLY(if(ERROR_FLAG){log(red("Attempted to execute while while another error was flagged")); return;})
                    process_node(ctx, ctx.node().children()[0]);
                    if(!(*(bool*)ctx.node().children()[0].value().get()))break;
                    uint32_t result = standard_travel_pass(ctx.node().scopes()[0], ctx.sub());
                    if(result > 0) {
                        uint32_t kind = result % 4;
                        if(kind == 2 || kind == 3) {//Break or continue
                            result -= 4;//Consume one magnitude
                            if(result >= 4) ctx.state(result);//If it still has magnitude, propagate up
                            if(kind == 2) break; //Otherwise break away
                            //Continue will fall through here, after consuming the magnitude
                        } else { //If it's a return, pass it up
                            ctx.state(result);
                            break;
                        }
                    }
                }
            };         

            r_handlers[for_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                ctx.node().scopes()[0].value(make_value());
            };
            x_handlers[for_id] = [this](Context& ctx) {
                process_node(ctx, ctx.node().children()[0]);
                Node scope = ctx.node().scopes()[0];
                scope.value().loc(0);
                while(true) {
                    process_node(ctx, ctx.node().children()[1]);
                    DEBUG_ONLY(if(ERROR_FLAG) {return;})
                    if(!(*(bool*)ctx.node().children()[1].value().get()))break;
                    scope.value().loc(scope.value().loc()+1); //This starts at 1 because function calls also start it at one
                    uint32_t result = standard_travel_pass(scope, ctx.sub());
                    process_node(ctx, ctx.node().children()[2]);
                    if(result > 0) {
                        uint32_t kind = result % 4;
                        if(kind == 2 || kind == 3) {//Break or continue
                            result -= 4;//Consume one magnitude
                            if(result >= 4) ctx.state(result);//If it still has magnitude, propagate up
                            if(kind == 2) break; //Otherwise break away
                            continue;
                        } else { //If it's a return, pass it up
                            ctx.state(result);
                            break;
                        }
                    }
                }
            };  

            r_handlers[read_file_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));
                Ptr ticket = get_ticket(name_store_id,1,char_id);
                ctx.node().value().set((void*)&ticket);
                resolve_overload(ctx);
            };
            x_handlers[read_file_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = *(Ptr*)ctx.node().value().get();
                string output(ptr);
                Ptr cptr = *(Ptr*)ctx.node().children()[0].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                output = readFile(string(cptr).to_std());
            };

            r_handlers[compile_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(node_id,sizeof(Ptr)));
                Ptr ticket = make_node();
                ctx.node().value().set((void*)&ticket);
                resolve_overload(ctx);
            };
            x_handlers[compile_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node& node = (Node&)(*(Ptr*)ctx.node().value().get());
                std::string source = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
                Node root = process(source);
                compile(root);
                start_logged_stage(x_handlers);
                node.copy(root);
                ctx.node().scopes() << root;
                root.owner(ctx.node());
                end_logged_stage();
            };

            // t_handlers[precompiling_id] = [this](Context& ctx){
            //     Node croot = ctx.node().scopes()[0];
            //     Col& vt = croot.value_table().col();
            //     for(int i=0;i<vt.length();i++) {
            //         ctx.root().value_table().put(((QString&)vt.cells[i]).to_std(),*(Value*)vt[i]);
            //     }
            // };
            x_handlers[precompiling_id] = [this](Context& ctx){ctx.node().scopes()[0].owner(ctx.node());}; //To restore visibility

            e_handlers.default_function = [this](Context& ctx){
                if(!ctx.node().scopes().empty()&&ctx.node().scopes()[0].owner()==ctx.node()) {
                    e_scoped_handler(ctx);
                    return;
                }

                if(!is_live(ctx.node().value())) {ctx.node().quals().push(make_node(live_qual));} //For debug things and such

                if(is_node_opperator(ctx.node())) {
                    if(ctx.node().children().length()==2) {
                        e_stage_binary_handler(ctx);
                    } else if(ctx.node().children().length()==1) {
                        ctx.node().value((ctx.node().children()[0].value())); 
                    }
                }
                standard_sub_process(ctx);
            };
            e_handlers[var_decl_id] = [this](Context& ctx){}; //Doing nothing
            e_handlers[equals_id] = [this](Context& ctx){
                e_stage_assignment_handler(ctx);
                standard_sub_process(ctx);
            };
            e_handlers[func_call_id] = [this](Context& ctx){
                if(ctx.node().has_qual(live_qual)) {
                    for(int i=0;i<ctx.node().children().length();i++) {
                        Node c = ctx.node().children().get(i);
                        if(is_live(c.value())&&!c.has_qual(live_qual)) {
                            c.value().quals().push(make_node(live_qual));
                        }
                    }
                }
                standard_sub_process(ctx);
            };
            e_handlers[return_id] = [this](Context& ctx){
                if(ctx.node().has_qual(live_qual)) {
                    for(int i=0;i<ctx.node().children().length();i++) {
                        Node c = ctx.node().children().get(i);
                        if(is_live(c.value())&&!c.has_qual(live_qual)) {
                            c.value().quals().push(make_node(live_qual));
                        }
                    }
                }
                standard_sub_process(ctx);
            };
            e_handlers[print_id] = [this](Context& ctx){
                ctx.node().value().quals().push(make_node(gatekeeper_qual));
                ctx.node().value().quals().push(make_node(live_qual));
                value_col subvals = ctx.node().value().sub_values();
                gather_all_values_in_scope(subvals,ctx.node());
                fire_quals(ctx,ctx.node().value());
                standard_sub_process(ctx);
            };
            e_handlers[to_prefix_id(gatekeeper_qual)] = [this](Context& ctx){
                for(int i=0;i<ctx.value().sub_values().length();i++) {
                    Value v = ctx.value().sub_values().get(i);
                    if(!v.has_qual(live_qual)) {v.quals().push(make_node(live_qual));}
                }
            };

            m_handlers.default_function = [this](Context& ctx){
                if(is_live(ctx.node().value())) {
                    if(should_allocate_data(ctx.node().value())) {
                        allocate_data(ctx.node().value());
                    }
                    for(int i=ctx.node().children().length()-1;i>=0;i--) {
                        Node child = ctx.node().children()[i];
                        if(m_handlers.has(child.type())) {
                            process_node(ctx,child);
                        } else {
                            if(is_live(child.value())) {
                                if(should_allocate_data(child.value())) {
                                    allocate_data(child.value());
                                }
                            }
                            process_node(ctx,child);
                        }
                    }
                    #if !LOBOTOMIZE_M_STAGE
                    for(int i=ctx.node().children().length()-1;i>=0;i--) {
                        Node child = ctx.node().children()[i];
                        if(!m_handlers.has(child.type())) {
                            if(should_recycle_data(child.value())) {
                                recycle_column(child.value().data_ptr());
                            }
                        }
                    }
                    #endif
                } else {
                    backwards_sub_process(ctx);
                }
            };
            m_handlers[var_decl_id] = [this](Context& ctx){
                #if !LOBOTOMIZE_M_STAGE
                if(should_recycle_data(ctx.node().value())) {
                    recycle_column(ctx.node().value().data_ptr());
                }
                #endif
            };
            m_handlers[identifier_id] = [this](Context& ctx){
                if(should_allocate_data(ctx.node().value())) {
                    allocate_data(ctx.node().value());
                }
            };



            a_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==A STAGE==");
                print(node_to_string(ctx.root()));
            };
            t_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==T STAGE==");
                print(node_to_string(ctx.root()));
            };
            r_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==R STAGE==");
                print(node_to_string(ctx.root()));
            };
            e_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==E STAGE==");
                print(node_to_string(ctx.root()));
            };
            m_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==M STAGE==");
                print(node_to_string(ctx.root()));
            };
            x_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==X STAGE==");
                print(node_to_string(ctx.node().in_scope()));
            };

            x_handlers[make_tokenized_keyword("MISTAKE")] = [this](Context& ctx){
                print("==X STAGE==");
                print(node_to_string(ctx.node().in_scope()));
            };

            e_handlers[make_tokenized_keyword("LBF_E")] = [this](Context& ctx){print("Launching blackfeather in e stage"); launch_blackfeather(unit_root);};
            x_handlers[make_tokenized_keyword("LBF_X")] = [this](Context& ctx){print("Launching blackfeather in x stage"); launch_blackfeather(unit_root);};


            x_handlers[invoke_stage_id] = [this](Context& ctx){
                std::string stage_name = ctx.node().name().to_std();
                if(!stages.hasKey(stage_name)) {
                    if(!stages.hasKey(stage_name+"ing")) {
                        print(red("invoke_stage_id:x_handler unknown stage "+stage_name));
                        return;
                    } else {
                        stage_name+="ing";
                    }
                }
                g_ptr<Stage> stage = stages.get(stage_name);
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                if(left.value().type()==node_id) {
                    left = Node(*(Ptr*)left.value().get());
                }
                ctx.node(left);
                stage->run(left.type())(ctx);
            };
        }

        void lemmatize_stages() {
            for(auto e : stages.entrySet()) {
                std::string label = e.key;
                if(e.key.length()>3 && e.key.substr(e.key.length()-3)=="ing") {
                    label = e.key.substr(0,e.key.length()-3);
                }
                if(!keywords.hasKey(label)) { //to stop double registraion
                    add_alias(label,invoke_stage_id);
                }
            }
        }

        virtual Node process(std::string path) override {
            Node root = tokenize(path);
            unit_root = root;
            return root;
        }

        bool post_mortem_printed = false;
        void post_mortem(Node root) {
            if(post_mortem_printed) return; 
            else post_mortem_printed = true;

            print(red("FINISHED WITH ERROR: "),ERROR_MSG);
            ERROR_FLAG = false;
            launch_blackfeather(root);
            ERROR_FLAG = true;
        }

        void deresolve_nodes(Node root) {
            walk_nodenet(root,[](Node n){n.resolved(false);});
        }

        Node compile_literal(const std::string& literal) {
            Node root = tokenize(literal);
            Node n = root.children()[0];
            if(root.children().length()>1) {
                n.type(string_id);
                for(int i=1;i<root.children().length();i++) {
                    n.name().push(" "+root.children()[i].name().to_std());
                }
            } else {
                if(n.type()==identifier_id) {
                    n.type(string_id);
                }
            }
            Context ctx = make_context(); ctx.node(n);
            t_handlers.run(n.type())(ctx);
            m_handlers.run(n.type())(ctx);
            x_handlers.run(n.type())(ctx);
            return n;
        }

        void print_stage_header(const std::string& label) {print_and_pause(0.7f,"\n\n\n\n\n\n\n\n=="+label+" STAGE==\n\n\n\n\n\n\n\n");} 

        void compile(Node root, bool prune = true) {
            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(a_handlers);
            standard_direct_pass(root);
            end_logged_stage();

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            lemmatize_stages();
            newline("Resolving keywords");
                a_pass_resolve_keywords(root.children());
            endline();
            for(int i=0;i<root.children().length();i++) {
                place_node_in_scope(root.children()[i],root);
            }

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(n_handlers);
            standard_direct_pass(root);
            end_logged_stage();

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(s_handlers);
            standard_direct_pass(root);
            end_logged_stage();
            
            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(t_handlers);
            standard_resolving_pass(root);
            end_logged_stage();

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(r_handlers);
            standard_resolving_pass(root);
            end_logged_stage();

            //No E stage for now while testing
            // DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            // start_logged_stage(e_handlers);
            // memory_backwards_pass(root);
            // endline();

            // if(prune) {
            //     newline("Pruning dead nodes");
            //         e_pass_prune_dead(root.children());
            //     endline();
            // }

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(m_handlers);
            memory_backwards_pass(root);
            end_logged_stage();
        }
    


        virtual void run(Node root) override {
            compile(root);

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            //print(node_to_string(root,0,0,true));

            start_logged_stage(x_handlers);
            standard_travel_pass(root);
            end_logged_stage();

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            //dump_unit(true);

            //launch_blackfeather(root);
        }


    };
}
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSE_SOCKET(fd) closesocket(fd)
    #define READ_SOCKET(fd, buf, len) recv(fd, buf, len, 0)
    #define WRITE_SOCKET(fd, buf, len) send(fd, buf, len, 0)
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <arpa/inet.h>

    #include <mach/mach.h>

    #define _UUID_T
    typedef unsigned char uuid_t[16];

    #define CLOSE_SOCKET(fd) ::close(fd)
    #define READ_SOCKET(fd, buf, len) ::read(fd, buf, len)
    #define WRITE_SOCKET(fd, buf, len) ::write(fd, buf, len)
#endif



size_t current_memory_usage() {
    #ifdef _WIN32
        return 0;
    #else
        struct mach_task_basic_info info;
        mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
        task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &size);
        return info.resident_size; // current RSS in bytes
    #endif
}

namespace Acorn {
    struct Webcorn_Core : public virtual Acorn_Script {
        Webcorn_Core(uint16_t _uid) : Unit(_uid) {init();}
        Webcorn_Core() {init();}



        uint32_t session_id = make_type("Session");
        uint32_t init_session_type() {
            ColCol col;
            col.label = "Sessions";
            uint32_t id = types.length();
            types.push(col);

            _layout stemp(add_template(session_id));
            stemp.add_prop(string_id,sizeof(Ptr),"username",char_id,1);
            stemp.add_prop(string_id,sizeof(Ptr),"userpath",char_id,1);
            stemp.add_prop(int_id,4,"timestamp");
            stemp.add_prop(int_id,4,"ip");
            layouts.put(session_id,stemp);
            value_printers[session_id] = [this](Context& ctx) {ctx.source("SESSION:"+Ptr_as_string(*(Ptr*)ctx.value().get()));};
            //writeFile("mixos-acorn/tests/printout.txt",make_wrapper_for_layout(stemp,"Session"));
            return id;
        }
        uint32_t session_col = init_session_type();

        uint32_t datasheet_id = reg_id("datahsheet");
        uint32_t metadatasheet_id = reg_id("metadatasheet");
        uint32_t notesheet_id = reg_id("notesheet");
        uint32_t scriptsheet_id = reg_id("scriptsheet");
        uint32_t storesheet_id = reg_id("storesheet");
        uint32_t formsheet_id = reg_id("formsheet");

        struct Session : Ptr {
            Session() {}
            Session(Ptr p) : Ptr(p) {}
       
            inline Ptr&         username_ptr(){return *(Ptr*)resolve_to_col(*this).qget(sidx+0); }
            inline Col&         username_col(){return resolve_to_col(username_ptr());}
            inline void         username(Ptr p){resolve_to_col(*this).qset(sidx+0, (void*)&p, 32); }
            inline string       username() {return (string&)username_ptr();}
       
            inline Ptr&         userpath_ptr(){return *(Ptr*)resolve_to_col(*this).qget(sidx+32); }
            inline Col&         userpath_col(){return resolve_to_col(userpath_ptr());}
            inline void         userpath(Ptr p){resolve_to_col(*this).qset(sidx+32, (void*)&p, 32); }
            inline string       userpath() {return (string&)userpath_ptr();}
       
            inline int          timestamp() {return *(int*)resolve_to_col(*this).qget(sidx+64); }
            inline void         timestamp(int t){resolve_to_col(*this).qset(sidx+64, (void*)&t, 4); }
       
            inline int          ip()        {return *(int*)resolve_to_col(*this).qget(sidx+68); }
            inline void         ip(int t)   {resolve_to_col(*this).qset(sidx+68, (void*)&t, 4); }
       };

        struct qeue_request {
            qeue_request() {}
            qeue_request(std::string _session, int _fd, std::string _message, std::string _unitcode) : 
            session(_session), fd(_fd), message(_message), unitcode(_unitcode) {}
            std::string session = "";
            int fd = 0; 
            std::string message = "";
            std::string unitcode = "";
        };
        struct Server : q_object {
            g_ptr<Thread> thread = nullptr;
            uint16_t unit = 0;
            list<qeue_request> requests;
            Session session = deadptr;
            bool authourized = false;

            uint32_t getfd() {std::lock_guard<std::mutex> lock(units_mutex); return units[unit]->types.index;}
            std::string getlabel() {std::lock_guard<std::mutex> lock(units_mutex); return units[unit]->types.label.to_std();}
            uint32_t gethash() {std::lock_guard<std::mutex> lock(units_mutex); return units[unit]->types.hash;}
            void setfd(uint32_t fd) {std::lock_guard<std::mutex> lock(units_mutex); units[unit]->types.index = fd;}
            void setlabel(const std::string& label) {std::lock_guard<std::mutex> lock(units_mutex); units[unit]->types.label = label; }
            void sethash(const std::string& hashstr) {std::lock_guard<std::mutex> lock(units_mutex); units[unit]->types.hash = hashBytes(hashstr.data(), hashstr.length());}
            void sethash(uint32_t hash) {std::lock_guard<std::mutex> lock(units_mutex); units[unit]->types.hash = hash;}

            bool needshelp() {std::lock_guard<std::mutex> lock(units_mutex); return !units[unit]->types.live;}
        };
    
        list<g_ptr<Server>> servers;
        std::mutex servers_mutex;


        std::string generate_token() {
            #ifdef _WIN32
                return "";
            #else
                unsigned char buf[32];
                int fd = open("/dev/urandom", O_RDONLY);
                read(fd, buf, 32);
                ::close(fd);
                std::string token = "";
                const char* hex = "0123456789abcdef";
                for(int i = 0; i < 32; i++) {
                    token += hex[buf[i] >> 4];
                    token += hex[buf[i] & 0xf];
                }
                return token;
            #endif
        }
        uint32_t generate_token_id = add_function("generate_token",[this](Context& ctx){
            string output = resolve_string_ticket(ctx.node());
            output = generate_token();
        },sizeof(Ptr),string_id);


        std::string extract_cookie(const std::string& request, const std::string& name) {
            std::string cookie_header = "Cookie: ";
            size_t start = request.find(cookie_header);
            if(start == std::string::npos) return "";
            start += cookie_header.length();
            size_t end = request.find("\r\n", start);
            std::string cookies = request.substr(start, end - start);
            
            std::string search = name + "=";
            size_t pos = cookies.find(search);
            if(pos == std::string::npos) return "";
            pos += search.length();
            size_t pos_end = cookies.find(";", pos);
            return cookies.substr(pos, pos_end - pos);
        }
        uint32_t get_cookie_id = add_function("get_cookie",[this](Context& ctx){
            std::string request = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
            std::string name = string(*(Ptr*)ctx.node().children()[1].value().get()).to_std();
            string output = resolve_string_ticket(ctx.node());
            output = extract_cookie(request,name);
        },sizeof(Ptr),string_id);

        void cry(const std::string& message) {
            types.label = message;
            types.live = false;
            print(red("Cried for help"));
            while(!types.live) {
                std::this_thread::sleep_for(std::chrono::nanoseconds(100));
            }
            print(green("Cries answered"));
        }

        uint32_t validate_login_id = add_function("validate_login",[this](Context& ctx){
            std::string body = ctx.sub().source().to_std();

            std::string username = "";
            std::string password = "";
            size_t u = body.find("username=");
            size_t p = body.find("password=");
            if(u != std::string::npos) username = body.substr(u+9, body.find("&", u) - u - 9);
            if(p != std::string::npos) password = body.substr(p+9, body.find("&", p) - p - 9);
        
            std::string role = "";
            if(username=="employee" && password=="pass123") role = "employee";
            if(username=="manager"  && password=="pass456") role = "manager";
            if(username=="Fir" && password!="NULL") role = "admin";
            if(username=="Reed" && password!="NULL") role = "admin";

            print(yellow("Validating a login")," ",username," ",password);
        
            if(!role.empty()) {
                cry("SESSION:"+username);
                std::string token = types.label.to_std();

                ctx.sub().source() = "HTTP/1.1 302 Found\r\n"
                    "Set-Cookie: session=" + token + "; HttpOnly\r\n"
                    "Location: /\r\n"
                    "Content-Length: 0\r\n"
                    "\r\n";
            } else {
                std::string body = "invalid";
                ctx.sub().source() = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: " + std::to_string(body.length()) + "\r\n"
                    "\r\n" + body;
            }
        });

        void copy_session(Session s, Session o) {
            if(is_live(o.username_ptr())) { 
                if(!is_live(s.username_ptr())) {
                    s.username(get_ticket(data_store_id,1,char_id));
                }
                s.username() = o.username();
            }
            if(is_live(o.userpath_ptr())) { 
                if(!is_live(s.userpath_ptr())) {
                    s.userpath(get_ticket(data_store_id,1,char_id));
                }
                s.userpath() = o.userpath();
            }
            s.timestamp(o.timestamp());
            s.ip(o.ip());
        }

        map<std::string,bool> distributed_tokens;

        void save_sheet(uint32_t idx, const std::string& path) {
            uint32_t sheetpool = find_sheet_pools_start(idx);
            auto out = openWriteStream(path);
            types[sheetpool].label =  path.substr(path.find_last_of('/')+1);
            write_raw<uint32_t>(out,sheetpool);
            list<ColCol*> sheet = gather_sheet_pools(sheetpool);
            write_ColColList(out,sheet);
            write_normalize_trailer(out,{NORM_IDS});
            out.close();
        }
        uint32_t load_sheet(const std::string& path) {
            uint32_t sheetpool = 0;
            bool found = false;
            std::string label = path.substr(path.find_last_of('/')+1);
            for(int p=0;p<types.length();p++) {
                if(types[p].tag==datasheet_id&&types[p].label==label) {
                    sheetpool = p; found = true; break;
                }   
            }
            if(!found) {
                auto in = openReadStream(path);
                print("Loading ",label);
                uint32_t saved_sheetpool = read_raw<uint32_t>(in);
                list<ColCol> loadsheet = read_ColColList(in);
                print("Loaded, adding to unit and normalizing");
                list<void*> to_normalize; for(int i=0;i<loadsheet.length();i++) to_normalize << (void*)&loadsheet[i];
                normalize(in,to_normalize,1);

                sheetpool = types.length();
                print("Sheetpool ",sheetpool," saved sheetpool ",saved_sheetpool);
                for(int p=0;p<loadsheet.length();p++) {
                    for(int c=0;c<loadsheet[p].length();c++) {
                        Col& col = loadsheet[p][c];
                        if(col.heterogenous) {
                            //Add a scan over the layout and normalization for Ptr members in the future if needed
                        } else if(col.tag==ptr_id||col.tag==string_id) {
                            for(int r=0;r<col.length();r++) {
                               Ptr ptr = *(Ptr*)col[r];
                               if(is_live(ptr)) {
                                    if(ptr.cachelevel==3) {
                                        ptr.cache = &types;
                                    } else if(ptr.cachelevel==0) {
                                        ptr.unit = uid;
                                    }
                                    uint32_t oldpool = ptr.pool;
                                    ptr.pool = sheetpool + (ptr.pool - saved_sheetpool);
                                    //print("Normalized ",oldpool," to ",ptr.pool);
                                    col.set(r,(void*)&ptr);
                               }
                            }
                        }
                    }
                    types.push(loadsheet[p]);
                }   
                print("Unit normalized");
            }
            return sheetpool;
        };

        void manage_sessions(const std::string& unitcode) {
            while(true) {
                g_ptr<Webcorn_Core> unit = nullptr;
                qeue_request queued;
                bool has_queued = false;
                g_ptr<Server> server = nullptr;
                {
                    std::lock_guard<std::mutex> lock(servers_mutex);
                    for(int i=0;i<servers.length();i++) {
                        if(servers[i]->needshelp()) {
                            unit = as<Webcorn_Core>(units[servers[i]->unit]);
                            server = servers[i];
                            break;
                        } 
                        if(servers[i]->getfd()==0 && !servers[i]->requests.empty()) {
                            queued = servers[i]->requests.take(0);
                            has_queued = true;
                            server = servers[i];
                            break;
                        }
                    }
                }
                if(unit) {
                    std::string fullreq = unit->types.label.to_std();
                    print("Unit ",unit->uid," has aksed for ",fullreq);
                    list<std::string> req = split_str(fullreq,':');
                    std::string cmd = req[0];
                    std::string arg = req.length()>1?req[1]:"";
                    if(cmd=="SESSION") {
                        if(session_col==0) {
                            print(red("webcorn:manage_sessions no valid session column in the main unit! Ensure a session manager was started"));
                        } else {
                            ColCol& sessions = types[session_col];
                            std::string token = "";
                            print(green("Logging in unit "+std::to_string(unit->uid)+" for "+arg));
                            uint32_t seshid = 0;
                            Session o;
                            if(sessions.hasKey(arg)) {
                                uint32_t seshid = sessions.getidx(arg.data(),arg.length());
                                token = sessions[seshid].label.to_std();
                                Ptr optr(&types,session_col,seshid,0);
                                o = optr;
                                print("Retrived token ",token," for ",arg);
                            } else {
                                token = generate_token();
                                distributed_tokens.put(token, true);
                                Col newsession;
                                newsession.label = token;
                                newsession.index = server->unit;
                                newsession.heterogenous = true;
                                _layout& l = layouts.get(session_id);
                                newsession.tag = session_id; newsession.element_size = l.total_size;
                                newsession.push_default();
                                seshid = sessions.length();
                                sessions.put(arg,newsession); //Do not use the sessions col refrence after this point
                                Ptr optr(&types,session_col,seshid,0);
                                o = optr;
                                o.username(get_ticket(name_store_id,1,char_id));
                                o.username() = arg;
                                o.userpath(get_ticket(name_store_id,1,char_id));
                                o.userpath() = "mixos-acorn/web/thistle/users/"+arg+"/";
                                uint32_t ts = (uint32_t)std::time(nullptr);
                                o.timestamp(ts);
                                server->authourized = true;
                            }
                            server->session = o; //Give it its session
                            unit->types.label = token;
                            server->sethash(token);

                            ColCol& unitdata = unit->types[unitdata_col];
                            value_col unit_global_values(unit->uid, unitdata_col, global_value_table_idx);
                            if(unit_global_values.hasKey("session")) {
                                Value seshv = unit_global_values.get("session");
                                Session s = seshv.data_ptr();
                                unit->copy_session(s,o);
                                print("Session coppied to unit ",unit->uid);
                            }
                            distributed_tokens.put(token,true);
                        }
                        unit->types.live = true;
                    } else if(cmd=="FILE") {
                        if(session_col==0) {
                            print(red("webcorn:manage_sessions:FILE no valid session column in the main unit! Ensure a session manager was started"));
                        } else {
                            if(server->authourized) {
                                if(arg=="LOAD") {
                                    _layout& l = layouts.get(session_id);
                                    ColCol& sessions = types[session_col];
                                    string username = server->session.username();
                                    std::string path = "mixos-acorn/web/thistle/users/"+username.to_std()+"/"+req[2]; //Add a bounds check for this later
                                    print("Loading ",path);
                                    uint32_t sheetpool = unit->load_sheet(path);
                                    unit->types.label = std::to_string(sheetpool);
                                } else if(arg=="SAVE") {
                                    _layout& l = layouts.get(session_id);
                                    ColCol& sessions = types[session_col];
                                    string username = server->session.username();
                                    std::string path = "mixos-acorn/web/thistle/users/"+username.to_std()+"/"+req[3]; //Add a bounds check for this later
                                    print("Saving ",path);
                                    unit->save_sheet(std::stoi(req[2]),path); //And a bounds check for this
                                } else {
                                    print(red("webcorn:manage_sessions:FILE unrecognized argument: "+arg));
                                }
                            } else {
                                unit->types.label = "";
                                print(red("webcorn:manage_sessions:FILE server is not authourized, please log in"));
                            }
                        }
                    } else {
                        print(red("webcorn:manage_sessions unrecognized command: "+cmd));
                    }
                    unit->types.live = true;
                }   
                else if(has_queued) {
                    print(green("Fuffiled a qeued request"));
                    server->setlabel(queued.message);
                    server->setfd(queued.fd);
                } 
                else {
                    std::this_thread::sleep_for(std::chrono::nanoseconds(100));
                }
            }
        }
        uint32_t start_session_manager_id = add_function("start_session_manager",[this](Context& ctx){
            std::string unitcode = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
            g_ptr<Server> new_server = make<Server>();
            new_server->thread = make<Thread>();
            new_server->unit = uid;
            new_server->sethash("session_manager");
            servers << new_server;
            new_server->thread->run_blocking([this, unitcode]() mutable {
                manage_sessions(unitcode);
            });
        });

        uint32_t dispatch_unit_id = add_function("dispatch_unit",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string session = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
            int server_fd = *(int*)ctx.node().children()[1].value().get();
            std::string message = string(*(Ptr*)ctx.node().children()[2].value().get()).to_std();
            std::string unitcode = string(*(Ptr*)ctx.node().children()[3].value().get()).to_std();

            uint32_t sessionhash = 0;
            if(!session.empty()) {
                if(distributed_tokens.hasKey(session)) {
                    print(green("Looking for session "+session));
                    sessionhash = hashBytes(session.data(),session.length());
                }
            }

            g_ptr<Server> server = nullptr;
            {
                std::lock_guard<std::mutex> lock(servers_mutex);
                if(sessionhash) {
                    for(int i=0;i<servers.length();i++) {
                        if(servers[i]->gethash()==sessionhash) {
                            server = servers[i];
                            print("Retrived session ",sessionhash," server unit ",server->unit);
                            break;
                        }
                    }
                    if(!server) {
                        print(red("webcorn:dispatch_unit no server matches the session "+session+", something has gone wrong with the session manager!"));
                        return;
                    }   
                    if(server->getfd()!=0) {
                        print(yellow("Retrived server is busy, qeueing a request instead"));
                        qeue_request req(session,server_fd,message,unitcode);
                        server->requests << req;
                        return;
                    }
                } else {
                    for(int i=0;i<servers.length();i++) {
                        if(servers[i]->gethash()==0 && servers[i]->getfd()==0) {
                            server = servers[i];
                            print("Found avaliable server unit ",server->unit);
                            break;
                        }
                    }
                    if(!server) {
                        g_ptr<Server> new_server = make<Server>();
                        new_server->thread = make<Thread>();
                        g_ptr<Webcorn_Core> webcorn = make_unit<Webcorn_Core>();
                        new_server->unit = webcorn->uid;
                        new_server->sethash(0);
                        servers << new_server;
                        server = new_server;
                        uint16_t uid = webcorn->uid;
                        print("Dispatched a new server unit ",uid);
                        new_server->thread->run_blocking([webcorn, unitcode]() mutable {
                            webcorn->run(webcorn->process(unitcode));
                        });
                    } 
                }
            }
            server->setlabel(message);
            server->setfd(server_fd);
            print("Server fd is now ",server->getfd());
        });


        bool is_websocket_upgrade(const std::string& request) {
            return request.find("Upgrade: websocket") != std::string::npos;
        }
        
        std::string get_websocket_key(const std::string& request) {
            size_t pos = request.find("Sec-WebSocket-Key: ");
            if(pos == std::string::npos) return "";
            pos += 19;
            size_t end = request.find("\r\n", pos);
            return request.substr(pos, end - pos);
        }
        
        std::string sha1_base64(const std::string& input) {
            // SHA1 constants
            uint32_t h0 = 0x67452301;
            uint32_t h1 = 0xEFCDAB89;
            uint32_t h2 = 0x98BADCFE;
            uint32_t h3 = 0x10325476;
            uint32_t h4 = 0xC3D2E1F0;
        
            // Pre-processing: adding padding
            std::string msg = input;
            uint64_t bit_len = input.size() * 8;
            msg += (char)0x80;
            while(msg.size() % 64 != 56) msg += (char)0x00;
            for(int i = 7; i >= 0; i--) msg += (char)((bit_len >> (i*8)) & 0xFF);
        
            // Process each 512-bit chunk
            for(size_t chunk = 0; chunk < msg.size(); chunk += 64) {
                uint32_t w[80];
                for(int i = 0; i < 16; i++) {
                    w[i] = ((uint8_t)msg[chunk+i*4]   << 24) |
                           ((uint8_t)msg[chunk+i*4+1] << 16) |
                           ((uint8_t)msg[chunk+i*4+2] << 8)  |
                           ((uint8_t)msg[chunk+i*4+3]);
                }
                for(int i = 16; i < 80; i++) {
                    uint32_t n = w[i-3]^w[i-8]^w[i-14]^w[i-16];
                    w[i] = (n<<1)|(n>>31); // left rotate 1
                }
        
                uint32_t a=h0, b=h1, c=h2, d=h3, e=h4;
        
                for(int i = 0; i < 80; i++) {
                    uint32_t f, k;
                    if(i<20)      { f=(b&c)|((~b)&d); k=0x5A827999; }
                    else if(i<40) { f=b^c^d;          k=0x6ED9EBA1; }
                    else if(i<60) { f=(b&c)|(b&d)|(c&d); k=0x8F1BBCDC; }
                    else          { f=b^c^d;          k=0xCA62C1D6; }
        
                    uint32_t temp = ((a<<5)|(a>>27)) + f + e + k + w[i];
                    e=d; d=c; c=(b<<30)|(b>>2); b=a; a=temp;
                }
        
                h0+=a; h1+=b; h2+=c; h3+=d; h4+=e;
            }
        
            // Produce 20-byte digest
            uint8_t digest[20];
            for(int i=0;i<4;i++) {
                digest[i]    = (h0>>(24-i*8))&0xFF;
                digest[i+4]  = (h1>>(24-i*8))&0xFF;
                digest[i+8]  = (h2>>(24-i*8))&0xFF;
                digest[i+12] = (h3>>(24-i*8))&0xFF;
                digest[i+16] = (h4>>(24-i*8))&0xFF;
            }
        
            // Base64 encode
            const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string out;
            for(int i=0;i<20;i+=3) {
                uint32_t n = ((uint32_t)digest[i]<<16) | 
                             (i+1<20?(uint32_t)digest[i+1]<<8:0) | 
                             (i+2<20?(uint32_t)digest[i+2]:0);
                out += b64[(n>>18)&63];
                out += b64[(n>>12)&63];
                out += (i+1<20) ? b64[(n>>6)&63] : '=';
                out += (i+2<20) ? b64[n&63]      : '=';
            }
            return out;
        }

        std::string make_websocket_accept(const std::string& key) {
            return sha1_base64(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
        }

        std::string websocket_handshake(const std::string& accept_key) {
            return "HTTP/1.1 101 Switching Protocols\r\n"
                   "Upgrade: websocket\r\n"
                   "Connection: Upgrade\r\n"
                   "Sec-WebSocket-Accept: " + accept_key + "\r\n"
                   "\r\n";
        }

        std::string websocket_read(int fd) {
            uint8_t header[2];
            READ_SOCKET(fd, (char*)header, 2);
            
            bool masked = header[1] & 0x80;
            uint64_t length = header[1] & 0x7F;
            
            if(length == 126) {
                uint8_t ext[2];
                READ_SOCKET(fd, (char*)ext, 2);
                length = ((uint64_t)ext[0]<<8) | ext[1];
            } else if(length == 127) {
                uint8_t ext[8];
                READ_SOCKET(fd, (char*)ext, 8);
                length = 0;
                for(int i=0;i<8;i++) length = (length<<8)|ext[i];
            }
            
            uint8_t mask[4] = {0};
            if(masked) READ_SOCKET(fd, (char*)mask, 4);
            
            std::string payload(length, 0);
            READ_SOCKET(fd, payload.data(), length);
            
            if(masked) {
                for(size_t i=0;i<length;i++) {
                    payload[i] ^= mask[i%4];
                }
            }
            
            return payload;
        }

        void websocket_write(int fd, const std::string& message) {
            std::string frame;
            frame += (char)0x81; // FIN + text opcode
            
            size_t len = message.size();
            if(len <= 125) {
                frame += (char)len;
            } else if(len <= 65535) {
                frame += (char)126;
                frame += (char)((len>>8)&0xFF);
                frame += (char)(len&0xFF);
            } else {
                frame += (char)127;
                for(int i=7;i>=0;i--) frame += (char)((len>>(i*8))&0xFF);
            }
            
            frame += message;
            WRITE_SOCKET(fd, frame.data(), frame.size());
        }

        uint32_t thread_sleep_id = add_function("thread_sleep",[this](Context& ctx){std::this_thread::sleep_for(std::chrono::nanoseconds(100));});
        uint32_t unit_sleep_id = add_function("unit_sleep",[this](Context& ctx){types.live = false;});
        uint32_t unit_wake_id = add_function("unit_wake",[this](Context& ctx){types.live = true;});
        uint32_t unit_index_id = add_function("unit_index",[this](Context& ctx){ctx.node().value().set((void*)&types.index);},4,int_id);
        uint32_t unit_uid_id = add_function("unit_uid",[this](Context& ctx){uint32_t tuid = (uint32_t)uid; ctx.node().value().set((void*)&tuid);},4,int_id);
        uint32_t unit_setindex_id = add_function("unit_setindex",[this](Context& ctx){int idx = *(int*)ctx.node().children()[0].value().get(); types.index=idx;});
        uint32_t unit_label_id = add_function("unit_label",[this](Context& ctx){string s = resolve_string_ticket(ctx.node()); s = types.label.to_std();},sizeof(Ptr),string_id);

        uint32_t make_webcorn_id = add_function("make_webcorn",[this](Context& ctx){
            standard_sub_process(ctx);
            auto unit = make_unit<Webcorn_Core>();
            uint32_t unitid = (uint32_t)unit->uid;
            ctx.node().value().set((void*)&unitid);
        },4,int_id);

        uint32_t compile_unit_id = add_function("compile_unit",[this](Context& ctx){
            //print(node_to_string(ctx.node()));
            standard_sub_process(ctx);
            int unitid = *(int*)ctx.node().children()[0].value().get();
            std::string source = string(*(Ptr*)ctx.node().children()[1].value().get()).to_std();
            g_ptr<Webcorn_Core> unit = as<Webcorn_Core>(units[unitid]);
            Node root = unit->process(source);
            unit->compile(root);
            //unit->start_logged_stage(unit->x_handlers);

            // ctx.node().scopes() << root;
            // root.owner(ctx.node());

            //unit->end_logged_stage();
            ctx.node().value().set((void*)&root);
        },sizeof(Ptr),node_id);

        // uint32_t run_unit_id = add_function("run_unit",[this](Context& ctx){
        //     //print(node_to_string(ctx.node()));
        //     standard_sub_process(ctx);
        //     int unitid = *(int*)ctx.node().children()[0].value().get();
        //     g_ptr<Webcorn_Core> unit = as<Webcorn_Core>(units[unitid]);
        //     unit->start_logged_stage(unit->x_handlers);
        //     unit->standard_travel_pass(unit->unit_root);
        //     unit->end_logged_stage();
        // });

        uint32_t run_unit_id = add_function("run_unit",[this](Context& ctx){
            //print(node_to_string(ctx.node()));
            standard_sub_process(ctx);
            int unitid = *(int*)ctx.node().children()[0].value().get();
            std::string source = string(*(Ptr*)ctx.node().children()[1].value().get()).to_std();
            g_ptr<Webcorn_Core> unit = as<Webcorn_Core>(units[unitid]);
            Node root = unit->process(source);
            unit->run(root);
        });


        uint32_t properties_id = reg_id("properties");
        uint32_t inlined_id = reg_id("inlined"); uint32_t prefix_inlined_id = reg_id("prefix_inlined");  uint32_t suffix_inlined_id = reg_id("suffix_inlined"); 
        uint32_t invisible_id = reg_id("invisible"); uint32_t prefix_invisible_id = reg_id("prefix_invisible"); uint32_t suffix_invisible_id = reg_id("suffix_invisible");
        uint32_t component_id = reg_id("component"); uint32_t prefix_component_id = reg_id("prefix_component");  uint32_t suffix_component_id = reg_id("suffix_component");
        uint32_t template_qual = add_qual("template");
        uint32_t stateless_qual = add_qual("stateless");
        uint32_t find_node_id = make_tokenized_keyword("find_node");
        // uint32_t capture_id = add_function("capture",[this](Context& ctx){
        //     if(!ctx.node().scopes().empty()&&ctx.source().to_std()=="go") { //The property normally owns the scope, but if I find a way to change that...
        //         ctx.source() = "";
        //         standard_travel_pass(ctx.node().scopes()[0],ctx);
        //     } else {
        //         standard_sub_process(ctx);
        //         string output = resolve_string_ticket(ctx.node());
        //         output = "run('"+ctx.source().to_std()+"'";
        //         for(int i=0;i<ctx.node().children().length();i++) {
        //             Node arg = ctx.node().children()[i];
        //             output.push(","+string(*(Ptr*)arg.value().get()).to_std());
        //         }
        //         output.push(")");
        //     }
        // },sizeof(Ptr),string_id);

        uint32_t capture_id = make_tokenized_keyword("capture");
        
        std::string to_js_expr(std::string s) {
            std::string str = "(()=>{"+s+"})()";
            return str;
        }
        uint32_t to_js_expr_id = add_function("js_do",[this](Context& ctx){
            standard_sub_process(ctx);
            string output = resolve_string_ticket(ctx.node());
            string s = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            output = to_js_expr(s.to_std());
        },sizeof(Ptr),string_id);

        std::string escape_js_string(const std::string& s) {
            std::string out;
            for(char c : s) {
                switch(c) {
                    case '\\': out += "\\\\"; break;
                    case '\'': out += "\\'"; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default: out += c; break;
                }
            }
            return out;
        }
        std::string to_js_lit(std::string s) {
            return "'"+escape_js_string(s)+"'";
        }
        uint32_t to_js_lit_id = add_function("js_lit",[this](Context& ctx){
            standard_sub_process(ctx);
            string output = resolve_string_ticket(ctx.node());
            string s = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            output = to_js_lit(s.to_std());
        },sizeof(Ptr),string_id);


        Stage& html_handlers = reg_stage("htmlemiting");

        _lookup is_structural{{
            "id", "class", "name", "type",
            "value", "placeholder", "checked", "disabled",
            "readonly", "required", "selected", "multiple",
            "action", "method", "for", "maxlength", "min", "max", 
            "step", "href", "src", "alt", "target", "rel",
            "tabindex", "contenteditable", "draggable", "hidden",
            "onclick", "onchange", "onsubmit", "oninput",
            "onfocus", "onblur", "onkeydown", "onkeyup",
            "onmouseenter", "onmouseleave", "onload", "onmouseover",
            "role","lang","colspan", "rowspan", "scope",
            "rows", "cols", "autocorrect", "autocapitalize", "spellcheck", "wrap",
            "autocomplete", "autofocus", "enctype", "novalidate", "pattern", "size",
            "download", "controls", "autoplay", "loop", "muted", "poster",  "oncontextmenu"
        }, false};
        bool is_prop_structural(const std::string& name) {
            return is_structural[name] || name.substr(0,5) == "data-";
        }

        uint32_t escape_str_id = add_function("escape_str",[this](Context& ctx){
            standard_sub_process(ctx);
            string content = resolve_string_ticket(ctx.node().children()[0]);
            std::string escaped = html_escape_string(content.to_std());
            string output = resolve_string_ticket(ctx.node());
            output = escaped;
        },sizeof(Ptr),string_id);

        bool resolve_prop_names(Context& ctx, Node c, std::string& prop, std::string& val) {
            Node saved_root = ctx.root();
            ctx.root(c);
            std::string old_source = ctx.source().to_std();

            if(c.children()[0].type()==literal_id) {
                prop = c.children()[0].name().to_std();
            } else if(c.children()[0].value().type()==string_id) {
                process_node(ctx,c.children()[0]);
                if(!is_live(c.children()[0].value().data_ptr())) { //For templates and such where we might use an identifer
                    ctx.root(saved_root);
                    ctx.source() = old_source;
                    return true;
                }
                prop = string(*(Ptr*)c.children()[0].value().get()).to_std();
            } else {
                prop = c.children()[0].name().to_std();
            }

            if(c.children().length()>1) {
                if(c.children()[1].type()==literal_id) {
                    val = c.children()[1].name().to_std();
                } else if(c.children()[1].value().type()==string_id) {
                    process_node(ctx,c.children()[1]);
                    if(!is_live(c.children()[1].value().data_ptr())) {
                        ctx.root(saved_root);
                        ctx.source() = old_source;
                        return true;
                    }
                    val = string(*(Ptr*)c.children()[1].value().get()).to_std();
                } else {
                    val = c.children()[1].name().to_std();
                }
            } else if(!c.scopes().empty()) {
                val = "run('"+Ptr_to_string(c,c.cachelevel)+"')";
            }
            else {
                ctx.root(saved_root);
                ctx.source() = old_source;
                return true;
            }
            ctx.root(saved_root);
            ctx.source() = old_source;
            return false;
        }

        void gather_inline_props(
            Context& ctx, Node c,
            list<std::string>& structural_prop_labels, list<std::string>& structural_prop_values,
            list<std::string>& style_prop_labels, list<std::string>& style_prop_values
        ) {
            std::string prop = "";
            std::string val = "";

            if(resolve_prop_names(ctx,c,prop,val)) {return;}

            list<std::string>* prop_labels; list<std::string>* prop_values;
            if(is_prop_structural(prop)) {
                prop_labels = &structural_prop_labels; 
                prop_values = &structural_prop_values;
            } else {
                prop_labels = &style_prop_labels; 
                prop_values = &style_prop_values;
            }

            if(!prop_labels->has(prop)) {
                prop_labels->push(prop);
                prop_values->push(val);
            }
        }

        void scan_for_inline_props(
            Context& ctx, Node node,
            list<std::string>& structural_prop_labels, list<std::string>& structural_prop_values,
            list<std::string>& style_prop_labels, list<std::string>& style_prop_values
        ) {
            if(node.type()==property_id||node.type()==to_unary_id(property_id)) {
                gather_inline_props(ctx,node,structural_prop_labels,structural_prop_values,style_prop_labels,style_prop_values);
            } else if(node.type()==if_id) {
                process_node(ctx,node.children()[0]);
                if(*(bool*)node.children()[0].value().get())  {
                    for(int j=0;j<node.scopes()[0].children().length();j++) {
                        Node jc = node.scopes()[0].children()[j];
                        scan_for_inline_props(ctx,jc,structural_prop_labels,structural_prop_values,style_prop_labels,style_prop_values);
                    }
                } else if(node.scopes().length()>1) {
                    for(int j=0;j<node.scopes()[1].children().length();j++) {
                        Node jc = node.scopes()[1].children()[j];
                        scan_for_inline_props(ctx,jc,structural_prop_labels,structural_prop_values,style_prop_labels,style_prop_values);
                    }
                }
            } else {
                for(int i=0;i<node.children().length();i++) {
                    Node c = node.children()[i];
                    scan_for_inline_props(ctx,c,structural_prop_labels,structural_prop_values,style_prop_labels,style_prop_values);
                }
            }
        }

        void emit_inline_html(Context& ctx) {
            if(ctx.node().mute()) return;
            std::string s = "";
            list<std::string> structural_prop_labels; list<std::string> structural_prop_values;
            list<std::string> style_prop_labels; list<std::string> style_prop_values;
            scan_for_inline_props(ctx,ctx.node(),structural_prop_labels,structural_prop_values,style_prop_labels,style_prop_values);
            for(int i=0;i<structural_prop_labels.length();i++) {
                s += " "+structural_prop_labels[i]+"=\""+structural_prop_values[i]+"\"";
            }   
            if(!style_prop_labels.empty()) {
                s += " style=\"";
                for(int i=0;i<style_prop_labels.length();i++) {
                    s += style_prop_labels[i]+":"+style_prop_values[i]+";";
                }  
                s += "\""; 
            }
            ctx.sub().source().push(s);
        }

        std::string emit_inline_html(Context& ctx, Node node) {
            Node old_node = ctx.node();
            Node old_qual = ctx.qual(); //Consider jus removing the qual here, it can cause problems with the rerouting
            std::string old_source = ctx.source().to_std();
            ctx.source().col().clear();
            ctx.node(node);
            emit_inline_html(ctx);
            std::string to_reutrn = ctx.source().to_std();
            ctx.node(old_node);
            ctx.source() = old_source;
            ctx.qual(old_qual);
            return to_reutrn;
        }

       Node make_property(Node type, Node value, Node parent) {
            Node prop_node = make_node(property_id);
            prop_node.children().push(type);
            prop_node.children().push(value);
            //prop_node.quals() << copy_as_token(parent);
            return prop_node;
        }

        Node get_prop(Context& ctx, Node node, std::string looking_for) {
            for(int i=0;i<node.children().length();i++) {
                Node c = node.children()[i];
                if(c.type()==property_id||c.type()==to_unary_id(property_id)) {
                    std::string prop = "";
                    std::string val = "";
                    if(resolve_prop_names(ctx,c,prop,val)) {
                        continue;
                    }
                    if(prop==looking_for) {
                        return c;
                    }
                }  
            }
            return deadptr;
        }
        uint32_t get_prop_id = overload_type(component_id,".\"get_prop\"","GET_PROP",make_value(node_id,sizeof(Ptr)),[this](Context& ctx){
            Node node = ctx.node().children()[0].scopes()[0];
            process_node(ctx, ctx.node().children()[1].children()[0]);
            std::string looking_for = resolve_string_ticket(ctx.node().children()[1].children()[0]).to_std();
            Node c = get_prop(ctx,node,looking_for);
            ctx.node().value().set((void*)&c);
        });
        uint32_t set_prop_id = overload_type(component_id,".\"set_prop\"","SET_PROP",make_value(node_id,sizeof(Ptr)),[this](Context& ctx){
            Node node = ctx.node().children()[0].scopes()[0];
            process_node(ctx, ctx.node().children()[1].children()[0]);
            process_node(ctx, ctx.node().children()[1].children()[1]);
            std::string looking_for = resolve_string_ticket(ctx.node().children()[1].children()[0]).to_std();
            std::string set_to = resolve_string_ticket(ctx.node().children()[1].children()[1]).to_std();
            Node c = get_prop(ctx,node,looking_for);
            if(is_live(c)) {
                c.children()[1].name() = set_to;
            } else {
                print(yellow("webcorn:set_prop property "+looking_for+" not found while trying to set to "+set_to));
            }
        });
        uint32_t has_prop_id = overload_type(component_id,".\"has_prop\"","HAS_PROP",make_value(bool_id,1),[this](Context& ctx){
            Node node = ctx.node().children()[0].scopes()[0];
            process_node(ctx, ctx.node().children()[1].children()[0]);
            std::string looking_for = resolve_string_ticket(ctx.node().children()[1].children()[0]).to_std();
            bool has = is_live(get_prop(ctx,node,looking_for));
            ctx.node().value().set((void*)&has);
        });
        int get_propidx(Context& ctx, Node node, std::string looking_for) {
            for(int i=0;i<node.children().length();i++) {
                Node c = node.children()[i];
                if(c.type()==property_id||c.type()==to_unary_id(property_id)) {
                    std::string prop = "";
                    std::string val = "";
                    if(resolve_prop_names(ctx,c,prop,val)) {
                        continue;
                    }
                    if(prop==looking_for) {
                        return i;
                    }
                }  
            }
            return -1;
        }
        uint32_t get_propidx_id = overload_type(component_id,".\"get_propidx\"","GET_PROPIDX",make_value(int_id,4),[this](Context& ctx){
            Node node = ctx.node().children()[0].scopes()[0];
            process_node(ctx, ctx.node().children()[1].children()[0]);
            std::string looking_for = resolve_string_ticket(ctx.node().children()[1].children()[0]).to_std();
            int i = get_propidx(ctx,node,looking_for);
            ctx.node().value().set((void*)&i);
        });

        //This should definetly be replaced with a cleaner system eventually
        Node webcorn_node_scan(const std::string& label, Node from) {
            //print("SEARCHING: ",node_info(from));
            if(!from.scopes().empty()) {
                //print("SEARCHING ",from.scopes()[0].quals().length()," QUALS");
                for(int i=0;i<from.scopes()[0].children().length();i++) {
                    Node c = from.scopes()[0].children()[i];
                    //print("   LOOKING AT ",node_to_string(c));
                    if(c.type()==property_id||c.type()==to_unary_id(property_id)) {
                        std::string prop = "";
                        std::string val = "";

                        if(c.children()[0].value().type()==string_id) {
                            process_node(c.children()[0],deadptr);
                            if(!is_live(c.children()[0].value().data_ptr())) {
                                if(!is_live(c.children()[0].value().data_ptr())) {
                                    continue;
                                }
                            }
                            //print("Prop is: ",prop);
                            prop = string(*(Ptr*)c.children()[0].value().get()).to_std();
                        } else {
                            prop = c.children()[0].name().to_std();
                        }

                        if(c.children()[1].value().type()==string_id) {
                            process_node(c.children()[1],deadptr);
                            if(!is_live(c.children()[1].value().data_ptr())) {
                                if(!is_live(c.children()[1].value().data_ptr())) {
                                    continue;
                                }
                            }
                            val = string(*(Ptr*)c.children()[1].value().get()).to_std();
                            //print("Val is: ",val);
                        } else {
                            val = c.children()[1].name().to_std();
                        }

                        //print("   ",prop,":",val);
                    
                        if(prop=="id"&&val==label) return from;
                    }
                }   
            }
            for(int i=0;i<from.children().length();i++) {
                Node found = webcorn_node_scan(label,from.children()[i]);
                if(is_live(found)) {
                    return found;
                }
            }
            for(int i=0;i<from.scopes().length();i++) {
                if(from.scopes()[i].owner()==from) {
                    Node found = webcorn_node_scan(label,from.scopes()[i]);
                    if(is_live(found)) {
                        return found;
                    }
                }
            }
            return deadptr;
        }

        struct style_manager : public q_object {
            style_manager(Webcorn_Core* _unit) : unit(_unit) {}
            style_manager(Webcorn_Core* _unit, list<std::string> init) : unit(_unit) {
                for(auto s : init) add_prop(s);
            }
            Webcorn_Core* unit;
            list<Node> props;
            list<std::string> prop_names;

            void add_prop(const std::string& name, Node prop = deadptr) {
                props << prop;
                prop_names << name;
            }

            void match_prop(const std::string& name, Node prop) {
                for(int i=0;i<prop_names.length();i++) {
                    if(prop_names[i]==name) {
                        props[i] = prop;
                        return;
                    }
                }
            }

            std::string resolve_prop(Context& ctx, const std::string& name) {
                for(int i=0;i<prop_names.length();i++) {
                    if(prop_names[i]==name&&is_live(props[i])) {
                        return unit->emit_inline_html(ctx,props[i]);
                    }
                }
                return "";
            }
        };

        uint32_t find_sheet_pools_start(uint32_t sheetpool) {
            while(types[sheetpool].tag != datasheet_id) {
                print(sheetpool,": ",labels[types[sheetpool].tag]);
                if(sheetpool == 0) {
                    print(red("webcorn:find_sheet_pools_start unable to find the datasheet")); 
                    return 0;
                }
                sheetpool -= 1;
            }
            return sheetpool;
        }

        list<ColCol*> gather_sheet_pools(uint32_t sheetpool) {
            list<ColCol*> to_return;
            if(sheetpool>=types.length()) {print(red("webcorn:gather_sheet_pools sheetpool "+std::to_string(sheetpool)+" out of bounds for types length "+std::to_string(types.length()))); return to_return;}
            sheetpool = find_sheet_pools_start(sheetpool);
            for(int p=sheetpool;p<types.length();p++) {
                if(p<types.length()) {
                    to_return << &types[p];
                    if(types[p].tag==storesheet_id) {
                        break;
                    }
                } else break;
            }
            return to_return;
        }

        //ADD NORMALIZATION LATER WHEN THIS NEEDS TO WORK WITH MULTIPLE POOLS
        void delete_sheet(uint32_t sheetpool) {
            if(sheetpool>=types.length()) {print(red("webcorn:delete_sheet sheetpool "+std::to_string(sheetpool)+" out of bounds for types length "+std::to_string(types.length()))); return;}
            sheetpool = find_sheet_pools_start(sheetpool);
            while(sheetpool < types.length() && types[sheetpool].tag != storesheet_id) {
                types.removeAt(sheetpool);
            }
            if(sheetpool < types.length()) {
                types.removeAt(sheetpool);
            }
        }

        void rename_sheet(uint32_t sheetpool, const std::string& name) {
            sheetpool = find_sheet_pools_start(sheetpool);
            types[sheetpool].label = name;
        }

        bool has_sheet(list<ColCol*> sheets, uint32_t tag, uint32_t nth = 0) {
            for(int i=0;i<sheets.length();i++) {
                if(sheets[i]->tag==tag) {
                    if(nth==0) {
                        return true;
                    } else nth-=1;
                }
            }
            return false;
        }
        uint32_t find_sheetidx(list<ColCol*> sheets, uint32_t tag, uint32_t nth = 0) {
            for(int i=0;i<sheets.length();i++) {
                if(sheets[i]->tag==tag) {
                    if(nth==0) {
                        return i;
                    } else nth-=1;
                }
            }
            print(red("webcorn:find_sheetidx could not find sheet "+labels[tag]));
            return 0;
        }
        ColCol* find_sheet(list<ColCol*> sheets, uint32_t tag, uint32_t nth = 0) {
            uint32_t index = find_sheetidx(sheets,tag,nth);
            return sheets[index];
        }
        uint32_t find_sheet_id = add_function("find_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t sheetpool = *(int*)ctx.node().children()[0].value().get();
            list<ColCol*> sheets = gather_sheet_pools(sheetpool);
            uint32_t tag = *(int*)ctx.node().children()[1].value().get();
            uint32_t nth = 0;
            if(ctx.node().children().length()>2) {
                nth = *(int*)ctx.node().children()[2].value().get();
            }
            uint32_t index = sheetpool+find_sheetidx(sheets,tag,nth);
            ctx.node().value().set((void*)&index);
        },4,int_id);
        uint32_t has_sheet_id = add_function("has_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t sheetpool = *(int*)ctx.node().children()[0].value().get();
            list<ColCol*> sheets = gather_sheet_pools(sheetpool);
            uint32_t tag = *(int*)ctx.node().children()[1].value().get();
            uint32_t nth = 0;
            if(ctx.node().children().length()>2) {
                nth = *(int*)ctx.node().children()[2].value().get();
            }
            bool has = has_sheet(sheets,tag,nth);
            ctx.node().value().set((void*)&has);
        },1,bool_id);

        void dump_sheet(list<ColCol*> sheets, uint32_t baseoffset) {
            for(int s = 0;s<sheets.length();s++) {
                dump_pool(*sheets[s],s+baseoffset,s==0);
            }
        }
        void dump_sheet(uint32_t sheetpool) {
            uint32_t baseoffset = find_sheet_pools_start(sheetpool);
            dump_sheet(gather_sheet_pools(sheetpool),baseoffset);
        }

        uint32_t dump_sheet_id = add_function("dump_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t sheetpool = *(int*)ctx.node().children()[0].value().get();
            uint32_t baseoffset = find_sheet_pools_start(sheetpool);
            dump_sheet(gather_sheet_pools(sheetpool),baseoffset);
        });

        uint32_t sheets_to_CSV_id = add_function("sheets_to_CSV",[this](Context& ctx){
            std::string to_return = "";
            uint32_t sheetpool = *(int*)ctx.node().children()[0].value().get();
            list<ColCol*> sheets = gather_sheet_pools(sheetpool);
            ColCol* datasheet = find_sheet(sheets, datasheet_id);
            for(int c = 0; c < datasheet->length(); c++) {
                if(c > 0) to_return += ",";
                to_return += "\"" + datasheet->get(c).label.to_std() + "\"";
            }
            to_return += "\n";
            int lenr = datasheet->empty() ? 0 : datasheet->get(0).length();
            for(int r = 0; r < lenr; r++) {
                for(int c = 0; c < datasheet->length(); c++) {
                    if(c > 0) to_return += ",";
                    Ptr cellptr(&types, sheetpool, c, r);
                    Ptr p = *(Ptr*)resolve_ptr(cellptr);
                    if(is_live(p)) {
                        to_return += "\"" + value_as_string(p) + "\"";
                    }
                }
                to_return += "\n";
            }

            string output = resolve_string_ticket(ctx.node());
            output = to_return;
        },sizeof(Ptr),string_id);
        uint32_t sheets_to_JSON_id = add_function("sheets_to_JSON",[this](Context& ctx){
            uint32_t sheetpool = *(int*)ctx.node().children()[0].value().get();
            list<ColCol*> sheets = gather_sheet_pools(sheetpool);
            ColCol* datasheet = find_sheet(sheets, datasheet_id);
            std::string to_return = "[\n";
            int lenr = datasheet->empty() ? 0 : datasheet->get(0).length();
            for(int r = 0; r < lenr; r++) {
                to_return += "  {";
                bool first = true;
                for(int c = 0; c < datasheet->length(); c++) {
                    if(!first) to_return += ",";
                    first = false;
                    std::string label = datasheet->get(c).label.to_std();
                    Ptr cellptr(&types, sheetpool, c, r);
                    Ptr p = *(Ptr*)resolve_ptr(cellptr);
                    std::string val = "";
                    if(is_live(p)) {
                        val = value_as_string(p);
                    }
                    to_return += "\"" + label + "\":\"" + val + "\"";
                }
                to_return += "}";
                if(r < lenr - 1) to_return += ",";
                to_return += "\n";
            }
            to_return += "]";
            string output = resolve_string_ticket(ctx.node());
            output = to_return;
        },sizeof(Ptr),string_id);
        
        //Moved to TwigSnap
        uint32_t render_sheet_id = add_function("render_sheet",[this](Context& ctx){});

        uint32_t create_sheet_id = add_function("create_sheet",[this](Context& ctx){
            uint32_t sheetid = types.length();
            ColCol data_pool; data_pool.tag=datasheet_id; types.push(data_pool);
            ColCol metadata_pool; metadata_pool.tag=metadatasheet_id; types.push(metadata_pool); 
            ColCol notes_pool; notes_pool.tag=notesheet_id; types.push(notes_pool);
            ColCol scripts_pool; scripts_pool.tag=scriptsheet_id; types.push(scripts_pool);
            ColCol store_pool; store_pool.tag=storesheet_id; types.push(store_pool);
            ctx.node().value().set((void*)&sheetid);
        },4,int_id);
        uint32_t delete_sheet_id = add_function("delete_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t sheetpool = *(uint32_t*)ctx.node().children()[0].value().get();
            delete_sheet(sheetpool);
        });
        uint32_t rename_sheet_id = add_function("rename_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t sheetpool = *(uint32_t*)ctx.node().children()[0].value().get();
            string name = resolve_string_ticket(ctx.node().children()[1]);
            rename_sheet(sheetpool,name.to_std());
        });
        uint32_t add_form_id = add_function("add_form",[this](Context& ctx){
            standard_sub_process(ctx);
            int idx = *(int*)ctx.node().children()[0].value().get();
            list<ColCol*> sheets = gather_sheet_pools(idx);
            if(sheets.empty()) {print(red("webcorn::add_form sheetpool "+std::to_string(idx)+" is invalid, unable to add a form")); return;}
            uint32_t baseoffset = find_sheet_pools_start(idx);
            uint32_t storeidx = baseoffset+(sheets.length()-1);
            uint32_t oldstoreidx = storeidx;
            
            //Snapshot shape from first non-store pool
            int ncols = sheets[0]->length();
            int nrows = 0;
            if(!sheets[0]->empty()) {
                nrows = sheets[0]->get(0).length();
            }
        
            auto insert_pool = [&](uint32_t tag) {
                ColCol pool; pool.tag = tag;
                for(int c = 0; c < ncols; c++) {
                    Col col(sizeof(Ptr)); col.tag = ptr_id;
                    for(int r = 0; r < nrows; r++) col.push_default();
                    pool.push(col); 
                }
                types.insert(oldstoreidx,pool); //There's a resize risk here, so probably shouldn't use sheets past this point
                storeidx+=1;
            };
        
            //Order is inverted becuase an insert means entries come in backwards
            insert_pool(scriptsheet_id);
            insert_pool(notesheet_id);
            insert_pool(metadatasheet_id);
            insert_pool(formsheet_id);

            sheets = gather_sheet_pools(idx);

            //Normalize for the newely shifted storesheet index
            for(int p=0;p<sheets.length();p++) {
                for(int c=0;c<sheets[p]->length();c++) {
                    Col& col = sheets[p]->get(c);
                    if(col.heterogenous) {
                        //Add a scan over the layout and normalization for Ptr members in the future if needed
                    } else if(col.tag==ptr_id||col.tag==string_id) {
                        for(int r=0;r<col.length();r++) {
                           Ptr ptr = *(Ptr*)col[r];
                           if(is_live(ptr)) {
                                if(ptr.pool==oldstoreidx) {
                                    ptr.pool = storeidx;
                                }
                                col.set(r,(void*)&ptr);
                           }
                        }
                    }
                }
            }   
            // print("Added form elements");
            // dump_sheet(sheets,baseoffset);
        });

     
        uint32_t save_sheet_id = add_function("save_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t idx = *(uint32_t*)ctx.node().children()[0].value().get();
            string s(*(Ptr*)ctx.node().children()[1].value().get());
            save_sheet(idx,("mixos-acorn/web/thistle/users/fir/sheets/"+s.to_std()));
        });
        uint32_t load_sheet_id = add_function("load_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            string s(*(Ptr*)ctx.node().children()[0].value().get());
            uint32_t sheetpool = load_sheet("mixos-acorn/web/thistle/users/fir/sheets/"+s.to_std());
            ctx.node().value().set((void*)&sheetpool);
            // dump_sheet(sheetpool);
            // cry("FILE:LOAD:sheets/"+s.to_std());
            // if(types.label.empty()) {
            //     print(red("Load sheet failed!"));
            // } else {
            //     uint32_t sheetpool = std::stoi(types.label.to_std());
            //     ctx.node().value().set((void*)&sheetpool);
            // }
        },4,int_id);

        uint32_t add_column_id = add_function("add_column_to_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            int sheetpool = *(int*)ctx.node().children()[0].value().get();
            print("Adding column to ",sheetpool);
            for(auto& pool : gather_sheet_pools(sheetpool)) {
                if(pool->tag==storesheet_id) continue;
                Col ncol(sizeof(Ptr)); ncol.tag = ptr_id;
                uint32_t row_count = 0;
                if(!types[sheetpool].empty()) {
                    row_count = types[sheetpool][0].empty() ? 0 : types[sheetpool][0].length();
                } 
                for(int i=0;i<row_count;i++) {
                    ncol.push_default();
                }
                pool->push(ncol);
            }
        });
        uint32_t add_row_id = add_function("add_row_to_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            int sheetpool = *(int*)ctx.node().children()[0].value().get();
            for(auto& pool : gather_sheet_pools(sheetpool)) {
                if(pool->tag==storesheet_id) continue;
                for(int i=0;i<pool->length();i++) {
                    pool->get(i).push_default();
                }
            }
        });
        uint32_t add_row_to_col_id = add_function("add_row_to_col",[this](Context& ctx){
            standard_sub_process(ctx);
            int sheetpool = *(int*)ctx.node().children()[0].value().get();
            int column = *(int*)ctx.node().children()[1].value().get();
            types[sheetpool][column].push_default();
        });
        uint32_t remove_row_from_col_id = add_function("remove_row_from_col",[this](Context& ctx){
            standard_sub_process(ctx);
            int sheetpool = *(int*)ctx.node().children()[0].value().get();
            int column = *(int*)ctx.node().children()[1].value().get();
            Col& col = types[sheetpool][column];
            if(col.cells.length()==col.length()) col.cells.removeAt(col.cells.length()-1,sizeof(CCol));
            col.removeAt(col.length()-1);
        });

        uint32_t setcell_id = add_function("setcell",[this](Context& ctx){
            standard_sub_process(ctx);
            //uspan->newline("setcell resolving vars");
            string addr = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            list<std::string> terms = split_str(addr.to_std(),'=');
            if(string_to_cachelevel(terms[0])!=3) {print(red("webcorn:setcell ptr "+terms[0]+" is invalid, it has the wrong cache level")); return;}
            Ptr cellptr = string_to_Ptr(terms[0]); cellptr.cache = &types;
            uint32_t pooltag = resolve_to_pool(cellptr).tag;
            list<ColCol*> sheets = gather_sheet_pools(cellptr.pool);
            if(sheets.last()->tag!=storesheet_id) {print(red("webcorn:setcell ptr "+terms[0]+" is invalid, the sheets it gathered did not end with a storesheet")); return;}
            uint32_t storepoolidx = find_sheet_pools_start(cellptr.pool)+(sheets.length()-1);
            Ptr p = cellptr;
            if(cellptr.pool!=storepoolidx) {
                p = *(Ptr*)resolve_ptr(cellptr);
            }
            //uspan->endline();
            //uspan->newline("setcell compiling literal");
            Node literal = compile_literal(terms[1]);
            //uspan->endline();
            //uspan->newline("setcell interim");
            Value lv = literal.value();
            print("Setcell value: ",value_info(lv));
            if(!is_live(p)) { //If we aren't replacing a live Ptr, create a new spot for its data in the store pool
                p = get_ticket(storepoolidx,lv.size(),lv.type());
                resolve_to_col(cellptr).set(cellptr.sidx,(void*)&p);
            }
            void* data = lv.get();
            Col& tcol = resolve_to_col(p); //Where the value is stored in the store pool

            uint32_t subtype = 0; uint32_t subsize = 0; uint32_t alias = ptr_id;

            if(lv.type()==string_id) {subtype = char_id; subsize = 1; alias = string_id;}
            else if(lv.sub_type()!=0) {subtype = lv.sub_type(); subsize = lv.sub_size(); alias = lv.type();}

            Ptr subp = deadptr; //This can be optimized in the future with finer discernment, such as detecting if we're replacing a Ptr more accurately than with just alias
            //Pending a redesign of how double-hop values work on the language side
            //uspan->endline();
            //uspan->newline("setcell work");
            if(tcol.tag==ptr_id||tcol.tag==string_id) {
                if(!tcol.empty()) {
                    subp = *(Ptr*)tcol.get(p.sidx); //The Ptr currently stored to the other collection
                    if(subtype==0||subsize==0&&is_live(subp)) { //Free the subptr if we're realiasing to a scalar
                        print("Recycling subp");
                        recycle_column(subp);
                    } else {
                        if(is_live(subp)) {
                            print("Resetting subp");
                            Col& subcol = resolve_to_col(subp);
                            subcol.clear(); subcol.element_size = subsize; subcol.tag = subtype;
                        } else {
                            print("Regnerating subp");
                            subp = get_ticket(storepoolidx,subsize,subtype);
                            resolve_to_col(p).set(p.sidx,(void*)&subp);
                        }
                    }
                } else if(subtype!=0&&subsize!=0) {
                    print("Replacing subp");
                    subp = get_ticket(storepoolidx,subsize,subtype);
                    resolve_to_col(p).push((void*)&subp);
                }
            }
            Col& col = resolve_to_col(p);
            if(subtype!=0&&subsize!=0) { //If we're a pointer to a collection
                if(col.tag!=alias) {
                    print("Realiasing");
                    col.element_size = sizeof(Ptr); col.tag=alias;
                    col.clear();
                    subp = get_ticket(storepoolidx,subsize,subtype);
                    resolve_to_col(p).push((void*)&subp);
                } else {
                    print("Replacing");
                }
                Ptr dataptr  = *(Ptr*)data;
                Col& datacol = resolve_to_col(dataptr); //Copy over the data to it's new position
                Col& subcol = resolve_to_col(subp);
                subcol.clear();
                for(int i=0;i<datacol.length();i++) {
                    subcol.push(datacol[i]);
                }
            } else { //If we're the direct value in the store pool
                if(col.element_size!=lv.size()||col.tag!=lv.type()) {
                    print("Clearing and pushing");
                    col.clear();
                    col.element_size = lv.size(); col.tag=lv.type();
                    col.push(data);
                } else if(col.empty()) {
                    print("Pushing");
                    col.push(data);
                } else {
                    print("Setting");
                    col.set(p.sidx,data);
                }
            }
            //uspan->endline();
        });


        uint32_t labelcell_id = add_function("labelcell",[this](Context& ctx){
            standard_sub_process(ctx);
            string addr = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            list<std::string> terms = split_str(addr.to_std(),'=');
            if(string_to_cachelevel(terms[0])!=3) {print(red("webcorn:labelcell ptr "+terms[0]+" is invalid, it has the wrong cache level")); return;}
            Ptr cellptr = string_to_Ptr(terms[0]); cellptr.cache = &types;
            uint32_t pooltag = resolve_to_pool(cellptr).tag;
            Col& cellcol = resolve_to_col(cellptr);
            if(pooltag==storesheet_id) { //We can only label storesheet cells for now
                Node literal = compile_literal(terms[1]);
                if(literal.value().type()==string_id) { 
                    while(cellcol.cells.length()<=cellptr.sidx) {
                        CCol c; //Temporary filler
                        char defc = ' ';
                        c.element_size = 1; 
                        c.tag = string_id;
                        c.hash = hashBytes((void*)&defc, 1);
                        c.index = cellcol.cells.length();
                        c.push((void*)&defc);
                        cellcol.cells.push(c);
                    }
                    CCol& cell = cellcol.cells[cellptr.sidx];
                    string& s = (string&)*(Ptr*)literal.value().get();
                    cell.clear();
                    cell.element_size = s.length();
                    cell.hash = hashBytes(resolve_ptr(s), s.length());
                    cell.push(resolve_ptr(s));
                } else {
                    //We only suppourt string keys for now
                }
            }
        });

        uint32_t list_files_in_directory_id = add_function("list_files_in_directory",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string path = ((string&)(*(Ptr*)ctx.node().children()[0].value().get())).to_std();
            Ptr p = resolve_ticket(ctx.node(),sizeof(Ptr),string_id);
            Col& data = resolve_to_col(p);
            list<std::string> files = listFilesInDirectory(path);
            while(data.length() > files.length()) {
                recycle_column(*(Ptr*)data.last());
                data.removeAt(data.length()-1);
            }
            for(int i=0;i<files.length();i++) {
                if(i<data.length()) {
                    string s = (string&)*(Ptr*)data[i];
                    s = files[i];
                } else {
                    string s = get_ticket(name_store_id,1,char_id);
                    s = files[i];
                    data.push((void*)&s);
                }
            }
            ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);

        //Just some experiments
        uint32_t clientsidescope_id = reg_id("clientsidescope");
        uint32_t clientside_id = add_function("clientside",[this](Context& ctx){
            standard_sub_process(ctx);
            Ptr callptr = ctx.node().scopes()[0];
            string output = resolve_string_ticket(ctx.node());
            std::string out = "";
            out = "run('"+Ptr_to_string(callptr,callptr.cachelevel)+"'";
            for(int i=0;i<ctx.node().children().length();i++) {
                Node arg = ctx.node().children()[i];
                if(arg.type()==var_decl_id) continue;
                out += ","+string(*(Ptr*)arg.value().get()).to_std();
            }
            out+=");";
            output = out;
        },sizeof(Ptr),string_id); 
        uint32_t serversidescope_id = reg_id("serversidescope");
        uint32_t serverside_id = add_function("serverside",[this](Context& ctx){

        }); 

        void init() override {
            register_type("div",component_id,0);
            register_type("inlined",inlined_id,0);
            register_type("invisible",invisible_id,0);

            r_handlers[clientside_id] = [this](Context& ctx){
                if(!ctx.node().scopes().empty()) {
                    ctx.node().scopes()[0].type(clientsidescope_id);
                }
            };
            x_handlers[clientsidescope_id] = [this](Context& ctx){
                ctx.state(standard_travel_pass(ctx.node(),ctx));
            };

            x_handlers[to_unary_id(property_id)] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node().scopes().empty()&&ctx.source().to_std()=="go") {
                    ctx.source() = "";
                    standard_travel_pass(ctx.node().scopes()[0],ctx);
                }
            };
            x_handlers[property_id] = x_handlers[to_unary_id(property_id)];

            //Potential idea, experiment with later
            //The idea is having the prop emission be driven by the props, making control flows and such more first class
            //But this would have performance issues and introduce extra compleixty in the current state (not to mention it doesn't work like this, source passing breaks it)
            // html_handlers[property_id] = [this](Context& ctx){
            //     std::string prop = ""; std::string val = "";
            //     resolve_prop_names(ctx,ctx.node(),prop,val);
            //     ctx.source().push(prop+"\0"+val+"\1");
            // };

             // void emit_inline_html(Context& ctx) {
            //     if(ctx.node().mute()) return;
            //     std::string s = "";
            //     list<std::string> structural_prop_labels; list<std::string> structural_prop_values;
            //     list<std::string> style_prop_labels; list<std::string> style_prop_values;
            //     std::string old_source = ctx.source().to_std();
            //     ctx.source().col().clear();
            //     standard_sub_process(ctx);
            //     list<std::string> prop_groups  = split_str(ctx.source().to_std(),'\1');
            //     for(int i=0;i<prop_groups.length();i++) {
            //         std::string property = prop_groups[i];
            //         list<std::string> props  = split_str(property,'\0');
            //         std::string prop = ""; if(props.length()>0) prop = props[0];
            //         std::string val = ""; if(props.length()>1) val = props[1];
            //         list<std::string>* prop_labels; list<std::string>* prop_values;
            //         if(is_prop_structural(prop)) {
            //             prop_labels = &structural_prop_labels; 
            //             prop_values = &structural_prop_values;
            //         } else {
            //             prop_labels = &style_prop_labels; 
            //             prop_values = &style_prop_values;
            //         }
        
            //         if(!prop_labels->has(prop)) {
            //             prop_labels->push(prop);
            //             prop_values->push(val);
            //         }
            //     }
            //     for(int i=0;i<structural_prop_labels.length();i++) {
            //         s += " "+structural_prop_labels[i]+"=\""+structural_prop_values[i]+"\"";
            //     }   
            //     if(!style_prop_labels.empty()) {
            //         s += " style=\"";
            //         for(int i=0;i<style_prop_labels.length();i++) {
            //             s += style_prop_labels[i]+":"+style_prop_values[i]+";";
            //         }  
            //         s += "\""; 
            //     }
            //     ctx.sub().source().push(s);
            // }

            //M is just the stage when this is most viable, any earlier and we get some issues
            m_handlers[property_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node().scopes().empty()&&ctx.node().children().length()>1) { //Yield the scope
                    ctx.node().children()[1].scopes().push(ctx.node().scopes().take(0));
                    ctx.node().children()[1].scopes()[0].owner(ctx.node().children()[1]);
                }
            };
            t_handlers[capture_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));
            };
            x_handlers[capture_id] = [this](Context& ctx){
                if(!ctx.node().scopes().empty()&&ctx.node().scopes()[0].owner()!=ctx.node()) {
                    ctx.node().scopes()[0].owner(ctx.node());
                }
                if(!ctx.node().scopes().empty()&&ctx.source().to_std()=="go") {
                    ctx.source() = "";
                    standard_travel_pass(ctx.node().scopes()[0],ctx);
                } else {
                    standard_sub_process(ctx);
                    string output = resolve_string_ticket(ctx.node());
                    output = "run('"+Ptr_to_string(ctx.node(),ctx.node().cachelevel)+"'";
                    for(int i=0;i<ctx.node().children().length();i++) {
                        Node arg = ctx.node().children()[i];
                        if(arg.type()==var_decl_id) {
                            output.push(",'[VAR]'");
                        } else {
                            //print("ARG: ",node_to_string(arg));
                            output.push(","+string(*(Ptr*)arg.value().get()).to_std());
                        }
                    }
                    output.push(")");
                }
            };


            add_function("plen",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t pool = *(int*)ctx.node().children()[0].value().get();
                Ptr pptr(&types,pool,0,0);
                uint32_t plen = resolve_to_pool(pptr).length();
                ctx.node().value().set((void*)&plen);
            },4,int_id);

            add_function("clen",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t pool = *(int*)ctx.node().children()[0].value().get();
                uint32_t col = *(int*)ctx.node().children()[1].value().get();
                Ptr cptr(&types,pool,col,0);
                uint32_t clen = resolve_to_col(cptr).length();
                ctx.node().value().set((void*)&clen);
            },4,int_id);

            add_function("clabel",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t pool = 0;
                uint32_t col = 0;
                if(ctx.node().children()[0].value().type()==ptr_id) {
                    Ptr p = *(Ptr*)ctx.node().children()[0].value().get();
                    pool = p.pool; col = p.idx;
                } else {
                    pool = *(int*)ctx.node().children()[0].value().get();
                    col = *(int*)ctx.node().children()[1].value().get();
                }
                Ptr cptr(&types,pool,col,0);
                string output = resolve_string_ticket(ctx.node());
                output = resolve_to_col(cptr).label.to_std();
            },sizeof(Ptr),string_id);

            add_function("csetlabel",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t pool = 0;
                uint32_t col = 0;
                std::string label = "";
                if(ctx.node().children()[0].value().type()==ptr_id) {
                    Ptr p = *(Ptr*)ctx.node().children()[0].value().get();
                    pool = p.pool; col = p.idx;
                    label = ((string&)*(Ptr*)ctx.node().children()[1].value().get()).to_std();
                } else {
                    pool = *(int*)ctx.node().children()[0].value().get();
                    col = *(int*)ctx.node().children()[1].value().get();
                    label = ((string&)*(Ptr*)ctx.node().children()[2].value().get()).to_std();
                }
                Ptr cptr(&types,pool,col,0);
                resolve_to_col(cptr).label = label;
            });

            uint32_t ptr_celllabel_id = overload_type(ptr_id,".\"celllabel\"","PTR_CELLLABEL",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
                Ptr p = *(Ptr*)ctx.node().children()[0].value().get();
                if(p.cachelevel==3) p.cache = &types;
                if(!ctx.node().children()[1].children().empty()) {
                    string label = (string&)*(Ptr*)ctx.node().children()[1].children()[0].value().get();
                    uint32_t pooltag = resolve_to_pool(p).tag;
                    Col& cellcol = resolve_to_col(p);
                    if(pooltag==storesheet_id) { //We can only label storesheet cells for now
                        Node literal = compile_literal(label.to_std());
                        if(literal.value().type()==string_id) { 
                            while(cellcol.cells.length()<=p.sidx) {
                                CCol c; //Temporary filler
                                char defc = ' ';
                                c.element_size = 1; 
                                c.tag = string_id;
                                c.hash = hashBytes((void*)&defc, 1);
                                c.index = cellcol.cells.length();
                                c.push((void*)&defc);
                                cellcol.cells.push(c);
                            }
                            CCol& cell = cellcol.cells[p.sidx];
                            string& s = (string&)*(Ptr*)literal.value().get();
                            cell.clear();
                            cell.element_size = s.length();
                            cell.hash = hashBytes(resolve_ptr(s), s.length());
                            cell.push(resolve_ptr(s));
                        } else {
                            //We only suppourt string keys for now
                        }
                    }
                } else {
                    string output = resolve_string_ticket(ctx.node());
                    Col& cellcol = resolve_to_col(p);
                    if(cellcol.cells.length()>p.sidx) {
                        output = ((QString&)cellcol.cells[p.sidx]).to_std();
                    } else {
                        output = "";
                    }
                }
            });

            add_function("uspan_flame_chart",[this](Context& ctx){
                string output = resolve_string_ticket(ctx.node());
                output = uspan->print_as_flamechart();
            },sizeof(Ptr),string_id);


            html_handlers.default_function = [this](Context& ctx) {
                if(is_live(ctx.qual())) {
                    //print("Running qual: ",node_info(ctx.qual()),red(" for "),node_info(ctx.node()));
                    if(is_live(ctx.value())) {
                        //print("Rerouting to value");
                        x_handlers.run(to_prefix_id(ctx.qual().type()))(ctx);
                    } else {
                        //print("Rerouting to node");
                        x_handlers.run(to_suffix_id(ctx.qual().type()))(ctx);
                    }
                } else {
                    //print("Running: ",node_info(ctx.node()));
                    //print("Rerouting to X");
                    x_handlers.run(ctx.node().type())(ctx);
                }
            };


            html_handlers[func_call_id] = [this](Context& ctx){
                uint32_t prelen = ctx.source().length();
                fire_quals(ctx,ctx.node().value());
                if(ctx.node().mute()) {ctx.node().mute(false); return;}
                bool needs_closing = ctx.source().length()!=prelen;
                x_handlers.run(func_call_id)(ctx);
                if(needs_closing) {
                    std::string src = ctx.source().to_std();
                    size_t tag_start = prelen+1;
                    size_t tag_end = src.find(' ', tag_start);
                    if(tag_end == std::string::npos) tag_end = src.find('>', tag_start);
                    std::string tag = src.substr(tag_start, tag_end - tag_start);
                    ctx.source().push("</"+tag+">");
                }
            };
            html_handlers[func_decl_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node().value());
            };
            html_handlers[prefix_component_id] = [this](Context& ctx){
                if(ctx.node().type()==func_call_id) {
                    ctx.qual(deadptr); //Revoke it. Dealing with inner quals and their routing is a minefield!
                    std::string props = emit_inline_html(ctx,ctx.value().type_scope());
                    if(!props.empty()) {
                        ctx.source().push("<div "+props+">");
                    }
                } else if(ctx.node().type()==func_decl_id&&!ctx.node().has_qual(template_qual)) {
                    if(ctx.node().children().length()==0) { //Implcit case, for when a div is paramatrized and named but not qualifed as a template, don't emit it!
                        ctx.qual(deadptr); 
                        html_handlers.run(component_id)(ctx);
                    }
                }
            };
            x_handlers[prefix_component_id] = html_handlers[prefix_component_id];

            html_handlers[to_prefix_id(template_qual)] = [this](Context& ctx){
                if(ctx.node().type()==func_call_id&&!ctx.node().has_qual(stateless_qual)) {
                    //print("Instantiating: [",ctx.node().scopes().length(),"] ",node_info(ctx.node()));
                    Node active_instance = deadptr;

                    std::string path = "";
                    Node climb = ctx.node(); //Create a key by climbing the scope, to handle recursion
                    while(is_live(climb.in_scope())&&is_live(climb.in_scope().owner())) {
                        Node climb_scope = climb.in_scope();
                        if(is_live(climb_scope.value())&&climb_scope.value().loc()>0) {
                            path += "-"+std::to_string(climb_scope.value().loc());
                        }
                        climb = climb_scope.owner();
                    }
                    if(!path.empty()) { //If we've been emitted multiple times because this call is in a loop
                        if(ctx.node().scopes().length()==1) { //Push our template scope in as a copy so that scope 1 remains the place to instantiate from
                            ctx.node().scopes() << ctx.node().scopes()[0];
                        }
                        //Add shrinking by checking against the node in 0 later, we can use it to understand what the previous highest path was and cull things that don't reach that watermark at iteration 0 of a given path
                        //That's a future memory optimization.
                        if(ctx.node().scopes().hasKey(path)) {
                            active_instance = ctx.node().scopes().get(path);
                        } else {
                            active_instance = instantiate_template_scope(ctx.node(),ctx.node().scopes()[1].owner(),ctx,true);
                            ctx.node().scopes().put(path,active_instance);
                        }

                        //We need a puppet for each instnatiation to bind the arguments to, figure out how to do this.

                        // Node decl = active_instance.owner(); //Rebind the arguments
                        // for(int i=0;i<ctx.node().children().length();i++) {
                        //     Node c = ctx.node().children()[i];
                        //     Node arg = decl.children()[i];
                        //     c.children().col().set(0,(void*)&arg);
                        // }

                        ctx.node().scopes().col().set(0,(void*)&active_instance);
                    } else {
                        Node scope_owner = ctx.node().scopes()[0].owner();
                        if(scope_owner!=ctx.node()) { //If we haven't been instantiated yet
                            active_instance = instantiate_template_scope(ctx.node(),scope_owner,ctx,true);
                        } else {
                            active_instance = ctx.node().scopes()[0];
                        }
                    }
                    ctx.node().scopes().col().set(0,(void*)&active_instance); //Swap the active instance into 0 so it gets called
                }
            };
            x_handlers[to_prefix_id(template_qual)] = html_handlers[to_prefix_id(template_qual)];

            r_handlers[prefix_inlined_id] = [this](Context& ctx){
                if(ctx.node().type()==func_call_id) {
                    Node func = ctx.value().type_scope();
                    // func.owner().mute(true); //To stop it from emitting 
                    bool is_static = ctx.value().has_qual(static_qual);
                    for(int i=0;i<func.children().length();i++) {
                        if(is_static) {
                            Node copy = make_node(); //Make this a proper deep copy later (placeholder for now)
                            copy.copy(func.children()[i]);
                            ctx.result().insert(ctx.index(),copy);
                        } else {
                            ctx.result().insert(ctx.index(),func.children()[i]);
                        }
                        ctx.index()++;
                    }
                }
            };

            html_handlers[DEBUG_ROOT_id] = [this](Context& ctx) {
                print("==HTML STAGE==");
                print(node_to_string(ctx.node().in_scope()));
            };

            x_handlers[make_tokenized_keyword("gather_props")] = [this](Context& ctx){
                print(yellow("webcorn:gather_props this function is deprecated"));
            };

            x_handlers[make_tokenized_keyword("emit_inline_html")] = [this](Context& ctx){
                if(ctx.sub().node().scopes().empty()) return;
                ctx.node(ctx.sub().node().scopes()[0]);
                emit_inline_html(ctx);
            };

            x_handlers[make_tokenized_keyword("emit_contents")] = [this](Context& ctx){
                Node node = ctx.sub().node();
                if(node.scopes().length()>0) {
                    Node scope = node.scopes().get(0);
                    for(int i=0;i<scope.children().length();i++) {
                        Node child = scope.children().get(i);
                        start_stage(html_handlers);
                        process_node(ctx,child);
                        start_stage(x_handlers);
                    }
                }
            };

            uint32_t display_node_id = make_tokenized_keyword("display_node");
            r_handlers[display_node_id] = [this](Context& ctx){
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));
                Ptr ticket = get_ticket(data_store_id,1,char_id);
                string contents(ticket);
                


                ctx.node().value().set((void*)&ticket);
            };
            x_handlers[display_node_id] = [this](Context& ctx){
                string addr(*(Ptr*)ctx.node().children()[0].value().get());
                string output(*(Ptr*)ctx.node().value().get());

                

                output = ("<p>"+addr.to_std()+"</p>");
            };

            r_handlers[find_node_id] = [this](Context& ctx){
                ctx.node().value(make_value(node_id,sizeof(Ptr)));
                resolve_overload(ctx);
            };
            x_handlers[find_node_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                std::string target = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
                Node& from = (Node&)(*(Ptr*)ctx.node().children()[1].value().get());
                //print("TARGET: ",target," FROM: ",node_info(from));
                Node result = webcorn_node_scan(target,from);
                // if(!is_live(result)) {
                //     while(is_live(from.in_scope())&&is_live(from.in_scope().owner())&&from.in_scope().owner().type()==func_decl_id) {
                //         from = from.in_scope().owner();
                //     }
                //     print("NOW SEARCHING FROM: ",node_info(from));
                // }

                ctx.node().value().set((void*)&result);
                if(is_live(result)) {
                    //print("FOUND: ",node_to_string(result));
                } else {
                    print(red("COULD NOT FIND "+target));
                }
            };

            x_handlers[make_tokenized_keyword("webcorn")] = [this](Context& ctx){
                ctx.node().value(make_value(int_id,4));
                int server_fd = 6;
                ctx.node().value().set((void*)&server_fd);
            };

            auto make_int_node = [this](Context& ctx){
                ctx.node().value(make_value(int_id,4));
            };

            uint32_t socket_id = make_tokenized_keyword("socket");
            r_handlers[socket_id] = make_int_node;
            x_handlers[socket_id] = [this](Context& ctx){
                #ifdef _WIN32
                    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
                #endif
                int server_fd = socket(AF_INET, SOCK_STREAM, 0);
                if(server_fd < 0) { print(red("socket() failed")); return; }
                ctx.node().value().set((void*)&server_fd);
            };
            
            uint32_t bind_id = make_tokenized_keyword("bind");
            r_handlers[bind_id] = make_int_node;
            x_handlers[bind_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                int port = *(int*)ctx.node().children()[1].value().get();
                int opt = 1;
                setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
                struct sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = htons(port);
                addr.sin_addr.s_addr = INADDR_ANY;
                int result = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
                ctx.node().value().set((void*)&result);
            };
            
            uint32_t listen_id = make_tokenized_keyword("listen");
            r_handlers[listen_id] = make_int_node;
            x_handlers[listen_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                int result = listen(fd, 10);
                ctx.node().value().set((void*)&result);
            };
            
            uint32_t accept_id = make_tokenized_keyword("accept");
            r_handlers[accept_id] = make_int_node;
            x_handlers[accept_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(client_addr));
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
                if(client_fd == -1) { throw_error("accept failed"); return; }
                ctx.node().value().set((void*)&client_fd);
            };
            
            uint32_t read_id = make_tokenized_keyword("read");
            r_handlers[read_id] = [this](Context& ctx){
                ctx.node().value(make_value(string_id, sizeof(Ptr)));
            };
            x_handlers[read_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                char buffer[4096];
                std::string request;
                
                while(true) {
                    int bytes = READ_SOCKET(fd, buffer, sizeof(buffer)-1);
                    if(bytes <= 0) break;
                    buffer[bytes] = 0;
                    request += buffer;
                    if(bytes < (int)sizeof(buffer)-1) break;
                }
                
                size_t header_end = request.find("\r\n\r\n");
                if(header_end != std::string::npos) {
                    size_t cl_pos = request.find("Content-Length: ");
                    if(cl_pos != std::string::npos) {
                        int content_length = std::stoi(request.substr(cl_pos+16));
                        std::string body = request.substr(header_end+4);
                        while((int)body.length() < content_length) {
                            int bytes = READ_SOCKET(fd, buffer, sizeof(buffer)-1);
                            if(bytes <= 0) break;
                            buffer[bytes] = 0;
                            body += buffer;
                        }
                        request = request.substr(0, header_end+4) + body;
                    }
                }
                
                Ptr ticket = get_ticket(name_store_id, 1, char_id);
                for(auto c : request) types[name_store_id][ticket.idx].push((void*)&c);
                ctx.node().value().set((void*)&ticket);
            };
            
            uint32_t write_id = make_tokenized_keyword("write");
            r_handlers[write_id] = make_int_node;
            x_handlers[write_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                Ptr strptr = *(Ptr*)ctx.node().children()[1].value().get();
                Col& col = resolve_to_col(strptr);
                WRITE_SOCKET(fd, (const char*)col.storage, col.size);
            };
            
            uint32_t close_id = make_tokenized_keyword("close");
            r_handlers[close_id] = make_int_node;
            x_handlers[close_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                CLOSE_SOCKET(fd);
            };

            

            // x_handlers[make_tokenized_keyword("respond")] = [this](Context& ctx){
            //     int fd = *(int*)ctx.node().children()[0].value().get();
            //     string str = *(Ptr*)ctx.node().children()[1].value().get();
            //     print("RESPONDING TO:\n",str.to_std());
            //     std::string body = "<html><body> <p> hello world </p>  <body></html>";
            //     std::string response = 
            //         "HTTP/1.1 200 OK\r\n"
            //         "Content-Type: text/html\r\n"
            //         "Content-Length: " + std::to_string(body.length()) + "\r\n"
            //         "\r\n" + body;
            //     print("Response:\n",response);
            //     if(::write(fd, response.c_str(), response.length()) < 0) {
            //         print(red("server_id::x_handler write() failed"));
            //     }
            // };


            x_handlers[make_tokenized_keyword("mem_test")] = [this](Context& ctx){
                uint32_t host_before = 0;
                uint32_t host_after = 0;
                for(int t = 0; t < types.length(); t++) {
                    for(int c = 0; c < types[t].length(); c++) {
                        host_before += types[t][c].size;
                    }
                }

                int iterations = 200;
                //readFile("mixos-acorn/web/webtest.gld");
                std::string sample = 
                //"int i = 5; print(i);"; 
                "Ptr Ptr Ptr int double_nested;\n"
                "Ptr Ptr int nested;\n"
                "Ptr int nums;\n"
                "nums.push(3);\n"
                "nums.push(8);\n"
                "nested.push(nums);\n"
                "Ptr int tums;\n"
                "tums.push(12);\n"
                "tums.push(14);\n"
                "nested.push(tums);\n"
                "double_nested.push(nested);\n"
                "print(double_nested.get(0).get(0).get(0));\n"
                "print(double_nested.get(0).get(0).get(1));\n"
                "print(double_nested.get(0).get(1).get(0));\n"
                "print(double_nested.get(0).get(1).get(1));\n";
            
                list<size_t> snapshots;
                
                for(int i = 0; i < iterations; i++) {
                    size_t before = current_memory_usage();
                    
                    Log::Line total; total.start();
                    Log::Line l; l.start();
                    g_ptr<Webcorn_Core> twig = make_unit<Webcorn_Core>();
                    print("INIT TIME: ",ftime(l.end())); l.start();
                    Node root = twig->process(sample);
                    print("PROCESS TIME: ",ftime(l.end())); l.start();
                    twig->compile(root);
                    print("COMPILE TIME: ",ftime(l.end())); l.start();
                    twig->start_stage(x_handlers);
                    twig->standard_travel_pass(root);
                    print("EXECUTE TIME: ",ftime(l.end())); l.start();
                    print("TOTAL TIME: ",ftime(total.end()));

                    units.removeAt(twig->uid);
                    twig->release();

                    size_t after = current_memory_usage();
                    snapshots << after;
                    print("iter ",i,": ",before," -> ",after," (delta: ",((int64_t)after-(int64_t)before),")");
                }

                for(int t = 0; t < types.length(); t++) {
                    for(int c = 0; c < types[t].length(); c++) {
                        host_after += types[t][c].size;
                    }
                }
                print("Host pool growth: ", (int)host_after - (int)host_before);
                
                // Print overall trend
                if(snapshots.length() > 1) {
                    int64_t total_growth = (int64_t)snapshots.last() - (int64_t)snapshots[0];
                    print("Total growth over ",iterations," iterations: ",total_growth," bytes");
                    print("Average per iteration: ",total_growth/iterations," bytes");
                }
            };

            //DON"T USE UNTIL FIX BEUCASE SERVERS WORK DIFFRENTLY NOW!!!
            x_handlers[make_tokenized_keyword("fragment_highlight")] = [this](Context& ctx) {
                std::string source = ctx.sub().source().to_std();
    
                size_t first = source.find(" ");
                size_t second = source.find(" ", first + 1);
                
                std::string target = source.substr(0, first);
                std::string instruction = source.substr(first + 1, second - first - 1);
                std::string content = source.substr(second + 1);

                print("TARGET: ",target);
                print("INSTRUCTION: ",instruction);
                print("CONTENT: ",content);

                print("MEMORY USED: ",current_memory_usage());

                std::string out = "";
                g_ptr<Webcorn_Core> twig = make_unit<Webcorn_Core>();
                if(instruction=="compile") {
                    Log::Line l; l.start();
                    Node root = twig->process(content);
                    twig->compile(root);
                    double a_time = l.end(); l.start();
                    out += fnodenet_to_string(root,Stamper{[this](Node n, list<int>& offsets){
                        std::string to_return = n.name().to_std();
                        if(n.type()!=0) {
                            std::string nreturn = "<span class='"+labels[n.type()]+"'>"+to_return+"</span>";
                            while((int)n.y()>=offsets.length()) {offsets<<0;}
                            n.x(n.x()+offsets[(int)n.y()]);
                            offsets[(int)n.y()]+=nreturn.length()-to_return.length();
                            to_return = nreturn;
                        }
                        return to_return;
                    },[this](Node n){
                        list<Node> stamps;
                        map<uint64_t,bool> visited;
                        collect_stamps(n,stamps,visited);
                        return stamps;
                    }});
                    double b_time = l.end(); 
                    // l.start();
                    //print_root(root);
                    // double c_time = l.end();

                    print("A: ",ftime(a_time));
                    print("B: ",ftime(b_time));
                    //print("C: ",ftime(c_time));

                    // print(node_to_string(root));

                    // recycle_node(root); //Deal with memory managment later, like in the mem_test
                    // units.erase(twig);

                    print("POST TWIG: ",current_memory_usage());

                } else if(instruction=="end") {
                    // print("REQUEST TO END: ",target," OF ",servers.length());
                    // g_ptr<Server> to_end = get_server(target);
                    // if(to_end) {
                    //     ::close(to_end->fd); 
                    //     to_end->fd = -1;
                    //     to_end->thread->end();
                    //     servers.erase(to_end);
                    // } else {
                    //     print(red("Unable to find server "+target+" to end"));
                    // }
                } else if(instruction=="preview") {
                    Node root = twig->process(content);
                    twig->run(root);

                    int port_num = 8081;
                    // for(auto c : root->children) {
                    //     if(c->type==server_id) {
                    //         for(auto sc : c->scope()->children) {
                    //             if(sc->type==port_id) {
                    //                 port_num = sc->left()->value->get<int>();
                    //             }
                    //         }
                    //     }
                    // }
                    servers << twig->servers;
                    // servers.last()->label = target;
                    // print("SPINNING UP A NEW SERVER ON ",port_num," CALLED ",servers.last()->label);
                    out = std::to_string(port_num);
                } else if(instruction=="read") {
                    out = readFile(content);
                } else {
                    print(red("Unrecognized instruction for fragment: "+ctx.sub().source().to_std()));
                }
                ctx.sub().source() = out;
            };
            


        }
    };
}