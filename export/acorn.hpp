#pragma once

#include <future>
#include <deque>
#include <mutex>
#include <thread>
#include <cmath>
#include <atomic>
#include <chrono>
#include <random>
#include <functional>
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <fstream>
#include <initializer_list>
#include <stdlib.h>
#include <any>
#include <sys/mman.h>
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
        processes.put(process_name,std::make_pair(table,f_table.get(table).length()));
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
    std::string label = "";
    SeqLine* parent = nullptr;
    list<g_ptr<SeqLine>> children;
    bool is_log = true;

    std::string get_indent() {
        if(!parent) return "";
        int depth = 0;
        SeqLine* cursor = this;
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

    g_ptr<SeqLine> get_last_line() {
        if(on_line) return on_line;
        else return line_root;
    }

    void add_line(const std::string& label) {
        g_ptr<SeqLine> parent = get_last_line();
        parent->children << make<SeqLine>(label,false);
        parent->children.last()->parent = parent.getPtr();
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
};

}



    
//Controls for the compiler printing, for debugging
#define PRINT_ALL 1

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

    struct Ptr {
        Ptr() {}
        Ptr(uint32_t _pool, uint32_t _idx, uint32_t _sidx, uint16_t _unit) : pool(_pool), idx(_idx), sidx(_sidx), unit(_unit) {}
        Ptr(uint32_t _pool, uint32_t _idx, uint32_t _sidx) : pool(_pool), idx(_idx), sidx(_sidx) {}
        uint32_t pool = 0; //Pool it's at
        uint32_t idx = 0; //Column
        uint32_t sidx = 0; //Row
        
        uint16_t unit = 0;

        inline bool operator==(const Ptr& other) const {return pool == other.pool && idx == other.idx && sidx == other.sidx;}
        inline bool operator!=(const Ptr& other) const {return !(*this == other);}
    };

    struct Ptr4 {
        Ptr4() {}
        Ptr4(uint32_t _midx, Ptr p) : midx(_midx), ptr(p) {}
        uint32_t midx = 0;
        Ptr ptr;
    };

    static const Ptr deadptr = {0,0,0,0};
    static Ptr dead_ref = {0,0,0,0};

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
            live = o.live;
        }
        uint32_t element_size = 1;
        uint32_t tag = 0;
        uint32_t hash = 0;
        uint32_t index = 0;
        bool live = true;

        inline uint32_t length() const {return size / element_size;}
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
        bool heterogenous = false;
        QString label;
        QCellCol cells;
        
        inline void* get(uint32_t index) {
            if(heterogenous) {
                return qget(index);
            } else {
                return sget(index);
            }
        }
        inline void* operator[](uint32_t index) {return get(index);}
        inline void* last() {return get(size-1);}

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

    //Standard column create, use pooling means it will try to find a dead column first, tag sensitive means it will also ensure the column tag matches
    uint32_t create_column(Col& col, uint32_t size, uint32_t tag, bool use_pooling = true, bool tag_sensitive = false) {
        if(use_pooling) {
            for(int i=0;i<col.length();i++) {
                Col& ncol = *(Col*)col.sget(i);
                if(!ncol.live&&ncol.element_size==size&&(!tag_sensitive||ncol.tag==tag)) {
                    ncol.clear();
                    ncol.live = true;
                    return i;
                }
            }
        }
        add_column(col,size,tag);
        return col.length()-1;
    }
    //Creates a column from pool and intilizes it's memory if empty
    uint32_t push_column(Col& col, uint32_t size, uint32_t tag) {
        uint32_t at = create_column(col,size,tag);
        Col& ncol = *(Col*)col.sget(at);
        if(ncol.size<size) {
            ncol.resize(size);
        }
        return at;
    }
    static void recycle_column(Col& col, uint32_t id) {
       Col* c = ((Col*)col.sget(id));
       if(c) {
        c->live = false;
       } else {
        print(red("UNABLE TO RECYLE COL AT "+std::to_string(id)));
       }
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
    }

    static Col read_col(std::istream& in) {
        Col col = read_ccol(in);
        col.heterogenous = read_raw<bool>(in);
        col.cells = read_qcellcol(in);
        col.label = read_qcol(in);
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
            for(uint32_t i = 0; i < o.length(); i++) {
                Col copy(*(Col*)o.sget(i));
                push(copy);
            }
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
            // for(uint32_t i = 0; i < t.cells.length(); i++) {
            //     t.cells.get(i).storage = nullptr;
            // }
            t.cells.storage = nullptr;
        }
    };
    
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
            // for(uint32_t i = 0; i < t.cells.length(); i++) {
            //     t.cells.get(i).storage = nullptr;
            // }
            t.cells.storage = nullptr;
        }
    };

    std::string Ptr_to_string(Ptr p) {
        return std::to_string(p.unit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx)+"";
    }
    Ptr string_to_Ptr(const std::string& s) {
        auto l = split_str(s,'|');
        if(l.length()==4) {
            Ptr p(std::stoi(l[1]),std::stoi(l[2]),std::stoi(l[3]),std::stoi(l[0]));
            return p;
        } else {
            print(red("Unable to convert "+s+" to a Ptr")); 
            return deadptr;
        }
    }

    list<g_ptr<Unit>> units;

    ColColCol& init_first_unit();

    ColColCol& global = init_first_unit();

    inline void* resolve_ptr(const Ptr& ptr);
    inline void* resolve_ptr(const Ptr& ptr, const uint32_t& idx);
    inline Ptr& resolve_to_ptr(const Ptr& ptr);
    inline Col& resolve_to_col(const Ptr& ptr);
    inline Col& resolve_to_col(const Ptr& ptr, const uint32_t& idx);
    inline ColCol& resolve_to_pool(const Ptr& ptr);
    inline ColColCol& resolve_to_unit(const Ptr& ptr);
    inline Col& to_col(const Ptr& ptr);
    inline Ptr get_ticket_from_unit(uint16_t unit_id, uint32_t type_id, uint32_t size, uint32_t tag);

    struct string : Ptr {
        string() {}
        string(Ptr p) {pool=p.pool;idx=p.idx;sidx=p.sidx;unit = p.unit;}
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

    Ptr add_type_for_handle() {
        Ptr to_return{add_type(),0,0};
        return to_return;
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
    size_t method_call_id = global_reg_id("METHOD_CALL");
    size_t function_id = global_reg_id("FUNCTION");
    size_t method_id = global_reg_id("METHOD");
    size_t func_decl_id = global_reg_id("FUNC_DECL");
    size_t type_decl_id = global_reg_id("TYPE_DECL");

    size_t tombstone_col = 0; 
    size_t refs_col = 0;

    Ptr global_add_layout_to_col(uint32_t type) {
        Ptr p(layout_type_id,note_value(global[layout_type_id],std::to_string(type)+" Offsets",4,int_id),0);
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
        Ptr ticket(name_store_id,create_column(global[name_store_id],1,char_id),0);
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
        context_total_size = ctemp.total_size;
        return at;
    }


    template<typename T>
    struct col_Ptr : Ptr {
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
        Value(Ptr p) { pool = p.pool; sidx = p.sidx; idx = p.idx; unit = p.unit;}

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
            Ptr dataptr = get_ticket_from_unit(unit,data_store_id,size(),type());
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
            // print("SET AT: ",Ptr_as_string(dataptr));
            // print("FROM: ",tag_to_str(types[dataptr.pool][dataptr.idx].tag,types[dataptr.pool][dataptr.idx].get(dataptr.sidx)));
            // print("TO: ",tag_to_str(types[dataptr.pool][dataptr.idx].tag,data));
            resolve_to_col(dataptr).set(dataptr.sidx, data);
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

        inline void copy(Value o, bool is_deep) {
            Col& src = resolve_to_col(o);
            Col& dst = resolve_to_col(*this);
            memcpy(dst.storage, src.storage, value_total_size);
            if(is_deep) {
                if(is_live(o.data_ptr())) {
                    init_data();
                    set(o.get());
                }

                Ptr qualsptr = get_ticket_from_unit(unit, quals_store_id, sizeof(Ptr), ptr_id);
                Col& new_quals = resolve_to_col(qualsptr);
                Col& old_quals = o.quals_col();

                new_quals.reserve(old_quals.size);
                memcpy(new_quals.storage, old_quals.storage, old_quals.size);
                new_quals.size = old_quals.size;
                resolve_to_col(*this).qset(value_quals_offset,(void*)&qualsptr,sizeof(Ptr));
        
                Ptr subvalsptr = get_ticket_from_unit(unit, sub_value_store_id, sizeof(Ptr), ptr_id);
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
    };
    
    struct QNode : public Col {
        inline uint32_t& type()                   {return *(uint32_t*)&storage[node_type_offset];}
        inline void      type(uint32_t t)         {memcpy(&storage[node_type_offset],(void*)&t,4);}
        inline uint32_t& sub_type()               {return *(uint32_t*)&storage[node_sub_type_offset];}
        inline void      sub_type(uint32_t st)    {memcpy(&storage[node_sub_type_offset],(void*)&st,4);}
    };



    struct Node : public Ptr {
        Node() {}
        Node(Ptr p) { pool = p.pool; sidx = p.sidx; idx = p.idx; unit = p.unit;}
    
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

    struct Context : public Ptr {
        Context() {}
        Context(Ptr p){ pool = p.pool; sidx = p.sidx; idx = p.idx; unit = p.unit;}
    
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
    
        inline Ptr&     source_ptr()         {DEBUG_ONLY(if(safety_check("context:source:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_source_offset);}
        inline Col&     source_col()         {Ptr& p = source_ptr(); return resolve_to_col(p);}
        inline string   source()             {return string(source_ptr());}
        inline void     source(Ptr p)        {DEBUG_ONLY(if(safety_check("context:source:set")){return;}) resolve_to_col(*this).qset(context_source_offset,(void*)&p,sizeof(Ptr));}
        inline void     source(std::string s){DEBUG_ONLY(if(safety_check("context:source:set")){return;}) source() = s;}
    
        inline uint32_t state()              {DEBUG_ONLY(if(safety_check("context:state:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(context_state_offset);}
        inline void     state(uint32_t s)    {DEBUG_ONLY(if(safety_check("context:state:set")){return;}) resolve_to_col(*this).qset(context_state_offset,(void*)&s,4);}
    
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

    class Unit : public q_object {
        public:
        Stage* active_stage;
        list<Watcher> watchers;

        uint16_t derive_uid(bool init_layouts) {
            DEBUG_ONLY(
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
            )
          
            uid = (uint16_t)units.length();

            if(init_layouts) {
                ColCol& h = global[handler_type_id];
                for(int i = 0; i < h.length(); i++) {
                    Col& handler_col = h[i];
        
                    labels.put(i,handler_col.label.to_std());
        
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
                    l.impl.unit = (uint16_t)units.length()-1;
                    layouts.put(i,l);
                }
            }
            units << this;
            return (uint16_t)units.length()-1;
        }

        Unit() : types(global), uid(derive_uid(true)) {init();}
        Unit(const ColColCol& starter) : types(starter), uid(derive_uid(false)) {init();}
        Unit(bool do_not_init) {}
        

        map<uint32_t, std::string> labels;
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
            Ptr ticket(type_id,create_column(types[type_id],size,tag,true),0,uid);
            return ticket;
        }

        inline Ptr get_ticket(Ptr storeptr, uint32_t size, uint32_t tag) {
            Ptr ticket(storeptr.pool,create_column(resolve_to_pool(storeptr),size,tag,true),0,storeptr.unit);
            return ticket;
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
            } else if(p.unit>=units.length()) {
                return red("UNIT_OUT_OF_BOUNDS:"+Ptr_to_string(p));
            } else if(p.pool>=(*units[p.unit]).types.length()) {
                return red("POOL_OUT_OF_BOUNDS:"+Ptr_to_string(p));
            } else if(p.idx>=(*units[p.unit]).types[p.pool].length()) {
                return red("IDX_OUT_OF_BOUNDS("+std::to_string(types[p.pool].length())+"):"+Ptr_to_string(p));
            } else if(marked_ptrs.has(p)) {
                return red(Ptr_to_string(p));
            }

            #if NAMED_PTRS
                std::string plabel = types[p.pool].label.empty()?std::to_string(p.pool):types[p.pool].label.to_std();
                std::string pidx = types[p.pool][p.idx].label.empty()?std::to_string(p.idx):types[p.pool][p.idx].label.to_std();
                std::string pstring = ""+plabel+"|"+pidx+"|"+std::to_string(p.sidx)+"";
                uint64_t key = Ptr_to_key(p);
            
                if(ptr_colors.hasKey(key)) {ptr_colors.get(key)(pstring);}
                return pstring;
            #else
                return std::to_string(p.unit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx)+"";
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
            //print("Registered: ",label," LEN: ",types[handler_type_id].length()," GLOBAL LEN: ",global[handler_type_id].length());
            return at;
        }

        Ptr add_layout_to_col(uint32_t type) {
            Ptr p(layout_type_id,note_value(types[layout_type_id],std::to_string(type)+" Offsets",4,int_id),0,uid);
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
            s += pad+name+"(uint32_t p, uint32_t i, uint32_t s) { pool = p; idx = i; sidx = s; }\n";
            s += pad+name+"(Ptr p) { pool = p.pool; idx = p.idx; sidx = p.sidx; }\n";
            for(int i=0;i<l.offsets.length();i++) {
                s+="\n";
                std::string type = labels[l.tags[i]];
                std::string label = l.labels[i];
                uint32_t offset = l.offsets[i];
                uint32_t size = l.sizes[i];

                bool is_compound = layouts.hasKey(l.tags[i]);
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
            n.unit = uid;
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
            Acorn::recycle_column((*units[p.unit])[p.pool], p.idx);
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
        Context make_context(Ptr result = deadptr, Ptr source = deadptr) {
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
        
            if(!is_live(result)) result = get_ticket(children_store_id, sizeof(Ptr), ptr_id);
            col.qset(context_result_offset, (void*)&result, sizeof(Ptr));
            
            if(!is_live(source)) source = get_ticket(name_store_id, sizeof(char), char_id);
            col.qset(context_source_offset, (void*)&source, sizeof(Ptr));
        
            uint32_t zero = 0; bool f = false;
            col.qset(context_index_offset,  (void*)&zero, 4);
            col.qset(context_state_offset,  (void*)&zero, 4);
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
                if(ptr.pool>=types.length()) {
                    return red("STRING ERROR "+std::to_string(ptr.pool)+"|"+std::to_string(ptr.idx)+"|"+std::to_string(ptr.sidx));
                }
                std::string content = string(ptr).to_std();
                return Ptr_as_string(ptr)+"> \""+escape_string(content)+"\"";
            } else if(tag==ptr_id) {
                return Ptr_to_string(*(Ptr*)data);
            } else if(tag==ptr_id||tag==node_id||tag==value_id||tag==context_id) {
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
                return labels[tag]+"?";
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
                        std::string line = ""; //v turn this into a 'show key as string' tag eventually and replace this cruft
                        if(col.label=="stages") { //From the handler type
                            std::string cell_label = ""; //col.get_cell_label(r);
                            if(!cell_label.empty()) line+=cell_label;
                            else line+="REIMPLMENT CELL KEYS LATER";
                        } else {
                            //print("Line ",lines.length()," Subline ",subline.length());
                            //print("Row ",r," Column ",c," Tag ",labels[col.tag],"(",col.tag,")");
                            std::string result = tag_to_str(col.tag,col[r]);
                            //print("Result: ",result);
                            line+=result;
                        }
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
                        line+=tag_to_str(col.tag,col[r]);
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
                    to_return += ", value: "+gray(tag_to_str(value.type(),value.get()))+" @"+ptr_addr;
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
                    to_return += "\n" + indent + "     Key: "+std::to_string(cells.get(i).hash)+" | "+value_info(node.value_table().get(cells.get(i).index));
                }
            }
            if(node.node_table().length()>0) {
                to_return += "\n" + indent + "   Node table:";
                QCellCol cells = node.node_table().col().cells;
                for(int i=0;i<cells.length();i++) {
                    to_return += "\n" + indent + "     Key: "+std::to_string(cells.get(i).hash)+" | "+node_info(node.node_table().get(cells.get(i).index));
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

        void deep_copy_node(Node n, Node o, map<uint32_t,Value>& value_alias_table, map<uint32_t,Node>& node_alias_table) {
            n.type(o.type());
            n.sub_type(o.sub_type());
            n.name(o.name().to_std());
            n.x(o.x());
            n.y(o.y());
            n.z(o.z());
            n.mute(o.mute());
            n.resolved(o.resolved());
        
            n.children().clear();
            for(int i = 0; i < o.children().length(); i++) {
                Node newc = make_node();
                deep_copy_node(newc, o.children()[i], value_alias_table, node_alias_table);
                n.children() << newc;
            }
        
            n.quals().clear();
            for(int i = 0; i < o.quals().length(); i++) {
                Node newq = make_node();
                deep_copy_node(newq, o.quals()[i], value_alias_table, node_alias_table);
                n.quals() << newq;
            }
        
            n.scopes().clear();
            for(int i = 0; i < o.scopes().length(); i++) {
                Node news = make_node();
                deep_copy_node(news, o.scopes()[i], value_alias_table, node_alias_table);
                n.scopes() << news;
            }
        
            if(value_alias_table.hasKey(o.value().idx)) {
                Value aliased = value_alias_table.get(o.value().idx);
                n.value(aliased);
                if(is_live(aliased.type_scope())) {
                    if(!n.scopes().empty()) {
                        Node ntyscope = aliased.type_scope();
                        n.scopes().col().set(0,(void*)&ntyscope);
                    }
                    else
                        n.scopes() << aliased.type_scope();
                }
            } else {
                if(is_live(o.value())) {
                    if(!is_live(n.value())) {
                        n.value(make_value());
                    }
                    n.value().copy(o.value(),true);
                }
            }
        
            types[n.pool][n.idx].qset(node_value_table_offset,
                types[o.pool][o.idx].qget(node_value_table_offset), sizeof(Ptr));
            types[n.pool][n.idx].qset(node_node_table_offset,
                types[o.pool][o.idx].qget(node_node_table_offset), sizeof(Ptr));
        
            n.parent(o.parent_ptr());
            n.owner(o.owner_ptr());
            n.in_scope(o.in_scope_ptr());
            n.opt_str() = o.opt_str().to_std();
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

            Ptr p(0,0,false); //Just a dead pointer
            types[handler_type_id][stages_id].put(label,(void*)&p);

            return *new_stage.getPtr();
        }

        map<uint32_t,Handler> value_printers; 

        std::string value_as_string(Value v) {
            Context ctx = make_context(); ctx.value(v);
            if(value_printers.hasKey(v.type())) {
                value_printers[v.type()](ctx);
            } else {
                return labels[v.type()]+"?";
            }
            deep_recycle_context(ctx);
            return ctx.source().to_std();
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

        uint32_t standard_travel_pass(Node root, Context sub = deadptr);

        inline void standard_process(Context& ctx, uint32_t type) {
            DEBUG_ONLY(for(auto& w : watchers) {if(w.prefix) w.prefix(ctx);})
            active_stage->run(type)(ctx);
            DEBUG_ONLY(for(auto& w : watchers) {if(w.suffix) w.suffix(ctx);})
        }

        inline void standard_process(Context& ctx) {
            standard_process(ctx,ctx.node().type());
        }
    
        void process_node(Context& ctx, Node node) {
            Node saved_node = ctx.node();
            Context saved_sub = ctx.sub();
            ctx.node(node);
            standard_process(ctx);
            ctx.node(saved_node);
            ctx.sub(saved_sub);
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
            ctx.value(value);
            for(int q=0;q<value.quals().length();q++) {
                Node qual = value.quals()[q];
                if(qual.mute()) continue;
                ctx.qual(qual);
                if(active_stage->has(qual.type()+1))
                    (*active_stage)[qual.type()+1](ctx);
            }
            ctx.value(saved_value);
        }
        void fire_quals(Context& ctx, Node node) {
            Node saved_node = ctx.node();
            ctx.node(node);
            for(int q=0;q<node.quals().length();q++) {
                Node qual = node.quals()[q];
                if(qual.mute()) continue;
                ctx.qual(qual);
                if(active_stage->has(qual.type()+2))
                    (*active_stage)[qual.type()+2](ctx);
            }
            ctx.node(saved_node);
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
                if(!ctx.result()[i].scopes().empty()) {
                    for(int s = 0;s<ctx.result()[i].scopes().length();s++) {
                        if(ctx.result()[i].scopes()[s].owner().idx==ctx.result()[i].idx) {
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
                node_col scopes = ctx.result().get(i).scopes();
                for(int s = 0; s<scopes.length(); s++) {
                    if(is_live(scopes.get(s).owner())&&scopes.get(s).owner().idx==ctx.result().get(i).idx) {
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

    inline void* resolve_ptr(const Ptr& ptr) {return (*units[ptr.unit])[ptr.pool][ptr.idx].get(ptr.sidx);}
    inline void* resolve_ptr(const Ptr& ptr, const uint32_t& idx) {return (*units[ptr.unit])[ptr.pool][idx].get(ptr.sidx);}
    inline Ptr& resolve_to_ptr(const Ptr& ptr) {return *(Ptr*)(*units[ptr.unit])[ptr.pool][ptr.idx].get(ptr.sidx);}
    inline Ptr& resolve_to_ptr(const Ptr& ptr, const uint32_t& idx) {return *(Ptr*)(*units[ptr.unit])[ptr.pool][idx].get(ptr.sidx);}
    inline Col& resolve_to_col(const Ptr& ptr) {return (*units[ptr.unit])[ptr.pool][ptr.idx];}
    inline Col& resolve_to_col(const Ptr& ptr, const uint32_t& idx) {return (*units[ptr.unit])[ptr.pool][idx];}
    inline ColCol& resolve_to_pool(const Ptr& ptr) {return (*units[ptr.unit])[ptr.pool];}
    inline ColColCol& resolve_to_unit(const Ptr& ptr) {return (*units[ptr.unit]).types;}
    inline Col& to_col(const Ptr& ptr) {return (*units[ptr.unit])[ptr.pool][ptr.idx];}

    inline Ptr get_ticket_from_unit(uint16_t unit_id, uint32_t type_id, uint32_t size, uint32_t tag) {
        Ptr ticket(type_id,create_column((*units[unit_id])[type_id],size,tag),0,unit_id);
        return ticket;
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
        Context ctx = make_context(children,is_live(sub)?sub.source_ptr():deadptr);
        int& i = ctx.index();
        ctx.root(root);
        ctx.sub(sub);
        while(i < ctx.result().length()) {
            ctx.node(ctx.result().get(i));
            standard_process(ctx);
            ctx.left(ctx.result().get(i));
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
    char c;
    read(STDIN_FILENO, &c, 1);
    return c;
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
}

namespace Acorn {

    #ifdef _WIN32
        
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
                            Ptr nptr(node_type_id,node_id,0);
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
        Compiler_Unit(uint16_t _uid) : Unit(_uid) { init(); }
        Compiler_Unit() {init();}
        
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

        uint32_t add_qualifer(const std::string& f) {
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

        uint32_t make_type(const std::string& f, uint32_t size = 0) {
            Value val = make_type_value(f,size);
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
        
        size_t colon_id = add_token(':',"COLON");
        size_t lparen_id = add_token('(',"LPAREN");
        size_t rparen_id = add_token(')',"RPAREN");
        size_t comma_id = add_token(',',"COMMA");
        size_t lbracket_id = add_token('[', "LBRACKET");
        size_t rbracket_id = add_token(']', "RBRACKET");
        size_t lbrace_id = add_token('{', "LBRACE");
        size_t rbrace_id = add_token('}', "RBRACE");
        size_t hash_id = add_token('#',"HASH");

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
            };
        }



        bool find_value_in_scope(Node node) {
            if(node.in_scope().value_table().hasKey(node.name().to_std())) {
                node.value(node.in_scope().value_table().get(node.name().to_std()));
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

        Value distribute_value(Node node, const std::string& label, Value val) {
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
                            val = distribute_value(scope,label,val);
                        }
                    }
                }
            }
            return val;
        }

        Node distribute_node(Node node, const std::string& label, Node carry) {
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
                            carry = distribute_node(scope,label,carry);
                        }
                    }
                }
            }
            return carry;
        }

        Node value_to_qual(Value val, std::string name = "", float x = -1.0f, float y = -1.0f) {
            Node to_return = make_node(val.type(),val.sub_type(),name,x,y,0.0f,val);
            return to_return;
        }

        map<uint32_t,int> left_binding_power;
        map<uint32_t,int> right_binding_power;
        void set_binding_powers(uint32_t id, int lbp, int rbp) {
            left_binding_power.put(id,lbp);
            right_binding_power.put(id,rbp);
        }

        map<char,bool> registered_opperators;
        size_t add_binary_operator(char c, const std::string& f, int lbp, int rbp, int use_id = -1) {
            size_t id = use_id;
            if(id==-1) {
                id = add_token(c,f);
            }
            set_binding_powers(id,lbp,rbp);
            registered_opperators[c] = true;
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
                    if(type_term.type()==var_decl_id||(is_live(type_term.value())&&type_term.value().sub_type()==type_term.type())) {
                        ctx.node().type(decl_id);
                        ctx.node().value(make_value());
                        ctx.node().value().copy(type_term.value(),true);
                        ctx.node().value().quals().push(value_to_qual(type_term.value()));
                        ctx.node().name(id_term.name().to_std());
                        ctx.node().value().sub_type(0);
                        ctx.node().value(distribute_value(ctx.node().in_scope(), ctx.node().name().to_std(), ctx.node().value()));
                        ctx.node().children().clear();
    
                        
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

        uint32_t  add_binding_token_combo(const std::string& f, int lbp, int rbp, char a, char b, char c = '\0', char d = '\0') {
            uint32_t id = add_token_combo(f,a,b,c,d);
            set_binding_powers(id,lbp,rbp);
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

        uint32_t assign_into_id = reg_id("ASSIGN_INTO"); //For function calls

        void init_stage_a() {
            discard_types.push_if_absent(undefined_id);
            discard_types.push_if_absent(end_id);
            discard_types.push_if_absent(lparen_id);
            discard_types.push_if_absent(lbrace_id);
            discard_types.push_if_absent(comma_id);

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
                        Node was_on = on; //Storing the root for cases where we want to notify once children are gathered
                        while(!on.children().empty()&&on.type()!=close_id) {
                            on = on.children().last();
                        }
                        if(on.type()==close_id) {
                            gathered.reverse();
                            bool was_given_children = false;
                            if(on.children().empty()) {
                                for(auto g : gathered)
                                    on.children() << g;
                                was_given_children = true;
                            }
                            // g_ptr<Node> token_on = copy_as_token(on);s

                            Node token_on = copy_as_token(on);

                            if(!on.children().empty())
                                on.copy(on.children().take(0));

                            on.quals() << token_on; //Copy the lparen
                            on.quals() << turn_into_token(ctx.node()); //Copy the rparen

                            if(!was_given_children) {
                                if(on.children().empty()) {
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
            value_printers[ptr_id] = [this](Context& ctx) {ctx.source(Ptr_as_string(ctx.value().data_ptr()));};
            value_printers[float_id] = [](Context& ctx) {ctx.source(std::to_string(*(float*)ctx.value().get()));};
            value_printers[int_id] = [](Context& ctx) {void* p = ctx.value().get(); DEBUG_ONLY(if(ERROR_FLAG) {return;}) ctx.source(std::to_string(*(int*)p));};
            value_printers[char_id] = [](Context& ctx) {ctx.source(std::string(1,*(char*)ctx.value().get()));};
            value_printers[bool_id] = [](Context& ctx) {ctx.source((*(bool*)ctx.value().get()) ? "TRUE" : "FALSE");};
            value_printers[string_id] = [this](Context& ctx) {void* p = ctx.value().get(); DEBUG_ONLY(if(ERROR_FLAG) {return;}) ctx.source((*(Ptr*)p));};
            value_printers[node_id] = [this](Context& ctx) {ctx.source(node_to_string((Node&)(*(Ptr*)ctx.value().get())));};
            value_printers[value_id] = [this](Context& ctx) {ctx.source(value_info((Value&)(*(Ptr*)ctx.value().get())));};
                
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

                    if(is_live(c.value())&&c.value().type()!=0) {
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
                        if(is_live(c.value())&&c.value().type()!=0) {
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
                    node.scopes()[0] = distribute_node(node.in_scope(),node.name().to_std(),node.scopes()[0]);
                    node.value().type_scope(node.scopes()[0]);
                    node.value().sub_type(0);
                    node.value(distribute_value(node.in_scope(),node.name().to_std(),node.value()));
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
                    node.value(distribute_value(node.in_scope(),node.name().to_std(),node.value()));
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
                    if(node.in_scope().type()==type_scope_id) {
                        node.in_scope().value_table().put(node.name().to_std(), decl_value); //So we don't distribute into function bodies, we need to alias later via this, as it's per instance
                        layouts[node.in_scope().owner().value().type()].add_prop(node.value().type(),node.value().size(),node.name().to_std(),0,0,decl_value);
                    } else {
                        node.value(distribute_value(node.in_scope(), node.name().to_std(), decl_value));
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
                        //No clue what this could be
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

            uint32_t root_type = 0; 
            uint32_t right_type = 0;
            if(!expr.children().empty()) {
                Node op = expr.children()[0];
                root_type = op.type();
                if(op.name().length()==1&&instr.length()>1&&instr.find(op.name().to_std())==0) {
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
            //LOG_W(ctx," resolving overloads");
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
                            typekey = make_overload_key(ctx.root().type(),any_id);
                            has_overload = l.has_overload(typekey);
                        }
                    } else if(!has_overload) {
                        if(ctx.result().length()>1) {
                            right_type = hashString(ctx.result().get(1).name().to_std());
                        }
                        if(right_type!=0) {
                            typekey = make_overload_key(ctx.root().type(),right_type);
                            has_overload = l.has_overload(typekey);
                        }
                    }
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
        int descend_call_scope(Context& ctx) {
            Node scope = ctx.node().scopes()[0];
            Value sv = scope.value();
            int loc = sv.loc()+1;
            sv.loc(loc);
            value_col subvals = sv.sub_values();
            if(subvals.empty()) {gather_all_values_in_scope(subvals,scope);}
            for(int i=0;i<subvals.length();i++) {
                Value sval = subvals.get(i);
                if(is_live(sval.data_ptr())) { //This ceremony is becuse if we just did col.push(col.get(0) it would invalidate the column as we push thus breaking the get, so we have to save as temps
                    Ptr dataptr = sval.data_ptr();
                    uint32_t elem_size = types[dataptr.pool][dataptr.idx].element_size;
                    uint8_t temp[elem_size];
                    if(types[dataptr.pool][dataptr.idx].empty()) {
                        types[dataptr.pool][dataptr.idx].push_default();
                    }
                    memcpy(temp, types[dataptr.pool][dataptr.idx].get((uint32_t)0), elem_size);
                    if(types[dataptr.pool][dataptr.idx].length() <= loc) {
                        //These shouldn't be getting out of sync in the first place, in the future investigate this deeper
                        int depth_check = 0;
                        while(types[dataptr.pool][dataptr.idx].length() <= loc && depth_check++ < 100) types[dataptr.pool][dataptr.idx].push(temp);
                        if(depth_check>=90) {
                            print(red("Infinite loop in loc catchup on "+Ptr_as_string(dataptr)+": this shouldn't even be happening in the first place!"));
                        }
                    } else {
                        types[dataptr.pool][dataptr.idx].set(loc, temp);
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
                if(is_live(sval.data_ptr())) {
                    Ptr newptr = sval.data_ptr();
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

        void instantiate_template(Node call, Node decl, Context& ctx) {
            if(!call.scopes().empty()) {
                Node new_scope = make_node(decl.scopes()[0].type(), 0, decl.name().to_std());
                call.scopes().col().set(0,(void*)&new_scope);
                
                if(is_live(decl.scopes()[0].value())) {
                    call.scopes()[0].value().copy(decl.scopes()[0].value(),true);
                }
                call.scopes()[0].owner(call);
                
                for(int i = 0; i < decl.scopes()[0].quals().length(); i++) {
                    call.scopes()[0].quals() << decl.scopes()[0].quals()[i];
                }

                map<uint32_t,Value> value_alias_table;
                map<uint32_t,Node> node_alias_table;
        
                for(int i = 0; i < call.children().length(); i++) {
                    process_node(ctx, call.children()[i]);
                    if(i < decl.children().length()) {
                        value_alias_table.put(decl.children()[i].value().idx, call.children()[i].value());
                        node_alias_table.put(decl.children()[i].idx, call.children()[i]);
                    }
                }
        
                for(int i = 0; i < decl.scopes()[0].children().length(); i++) {
                    Node copy = make_node();
                    copy.in_scope(call.scopes()[0]);
                    deep_copy_node(copy, decl.scopes()[0].children()[i], value_alias_table, node_alias_table);
                    call.scopes()[0].children() << copy;
                }
            } else {
                print("CALL HAS NO SCOPE");
            }
        }



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

        uint32_t print_id = add_function("print",[this](Context& ctx){ 
            std::string to_print = "";
            for(int i=0;i<ctx.node().children().length();i++) {
                Node c = ctx.node().children()[i];
                process_node(ctx,c);
                to_print += value_as_string(c.value());
            }
            print(to_print);
        });
        uint32_t return_id = make_tokenized_keyword("return");

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

            set_binding_powers(random_combo_id,8,9);

            t_handlers[identifier_id] = [this](Context& ctx){resolve_identifier(ctx);};
            t_handlers[equals_id] = [this](Context& ctx){standard_sub_process(ctx);};

            t_handlers.default_function = [this](Context& ctx){if(ctx.node().scopes().empty()) {standard_sub_process(ctx);}}; //Because resolving passes will already cover the sub process for scoped nodes
            r_handlers.default_function = [this](Context& ctx){standard_sub_process(ctx);};
            x_handlers.default_function = [this](Context& ctx){standard_sub_process(ctx);};

            r_handlers[func_decl_id] = [this](Context& ctx) {
                Node scope = ctx.node().scopes()[0];
                if(!is_live(scope.value())) {
                    scope.value(make_value()); 
                    scope.value().loc(0); //Set location for stack depth
                }
            };
            x_handlers[func_decl_id] = [this](Context& ctx){};
            r_handlers[func_call_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                sync_args(ctx);
                //instantiate_template(ctx.node(),ctx.node().value().type_scope().owner(),ctx);
            };
            x_handlers[func_call_id] = [this](Context& ctx) {
                Node scope = ctx.node().scopes()[0];
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
                int stack_depth = descend_call_scope(ctx);
                DEBUG_ONLY(if(stack_depth>500) {throw_error("Stack overflow on function call: ",node_info(ctx.node())); return;})
                for(int i=0;i<ctx.node().children().length();i++) {
                    Node leftterm = ctx.node().children()[i].children()[0];
                    leftterm.value().set(temps[i].data());
                }
                DEBUG_ONLY(if(ERROR_FLAG) {log(red("ABORTING FUNCTION CALL BEFORE PASS")); return;})
                if(!standard_travel_pass(scope,ctx.sub())) { //If the return didn't already ascend
                    ascend_call_scope(ctx.node().scopes()[0]);
                }
            };

            t_handlers[return_id] = [this](Context& ctx){
                if(ctx.index()+1<ctx.result().length()) {
                    ctx.node().children() << ctx.result().take(ctx.index()+1);
                }
                standard_sub_process(ctx);
            };
            r_handlers[return_id] = [this](Context& ctx){
                Node climb = ctx.node();
                while(is_live(climb)&&climb.type()!=func_decl_id) { //WARNING: we need to make sure things like in blocks which also use returns are safe with this! 
                    //This might try to bind to some random function via climbing when it does this, so when metaprogramming becomes visible in the compielr, add gaurds.
                    climb = climb.in_scope().owner();
                }
                ctx.node().parent(climb);
                if(is_live(ctx.node().parent())) {
                    ctx.node().value(ctx.node().parent().value());
                }
                standard_sub_process(ctx);
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

                    Ptr lp = left.value().data_ptr();
                    Ptr rp = right.value().data_ptr();

                    DEBUG_ONLY(if(!is_live(lp)) {throw_error("left term of equals is invalid"); return;})
                    DEBUG_ONLY(if(!is_live(rp)) {throw_error("right term of equals is invalid"); return;})
                    DEBUG_ONLY(if(left.value().size()!=right.value().size()) {throw_error("Mismatched sizes for assignment from:\n",node_to_string(ctx.node())); return;})
                    
                    if(types[lp.pool][lp.idx].heterogenous) {
                        types[lp.pool][lp.idx].qset(lp.sidx,types[rp.pool][rp.idx][rp.sidx],right.value().size());
                    } else {
                        types[lp.pool][lp.idx].set(lp.sidx,types[rp.pool][rp.idx][rp.sidx]);
                    }
                }
            };

            make_tokenized_keyword("any",any_id);
            make_tokenized_keyword("null",null_id);

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
                        if(types[ptr.pool][ptr.idx].heterogenous) {
                            types[ptr.pool][ptr.idx].qset(ptr.sidx,(void*)&right.children()[0].value().data_ptr(),right.children()[0].value().size());
                        } else {
                            types[ptr.pool][ptr.idx].set(ptr.sidx,(void*)&right.children()[0].value().data_ptr());
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

        uint32_t test_id = reg_id("TEST");
        Stage& n_handlers = reg_stage("naming"); 
        
        uint32_t labels_id = make_tokenized_keyword("labels");

        uint32_t node_block_id = reg_id("node_block");
        uint32_t invoke_stage_id = make_keyword("invoke_stage");
        uint32_t in_id = make_keyword("in");
        uint32_t precompiling_id = reg_id("PRECOMPILING");

        uint32_t ctx_id = make_tokenized_keyword("ctx");
        uint32_t lctx_id = make_tokenized_keyword("lctx");

        uint32_t while_id = make_tokenized_keyword("while");
        uint32_t for_id = make_tokenized_keyword("for");
        uint32_t if_id = make_tokenized_keyword("if");
        uint32_t else_id = make_tokenized_keyword("else");

        uint32_t read_file_id = make_tokenized_keyword("read_file");
        uint32_t write_file_id = make_tokenized_keyword("write_file");
        uint32_t compile_id = make_tokenized_keyword("compile");

        uint32_t live_qual = add_qualifer("live");
        uint32_t gatekeeper_qual = add_qualifer("gatekeeper");
        uint32_t assigned_qual = add_qualifer("assigned");
        uint32_t constant_qual = add_qualifer("constant");

        uint32_t to_string_id = make_tokenized_keyword("to_string");
        uint32_t to_type_id = make_tokenized_keyword("to_type");
        uint32_t DEBUG_ROOT_id = make_tokenized_keyword("DEBUG_ROOT");

        uint32_t ptr_take_id = reg_id("PTR_TAKE");
        uint32_t ptr_push_id = reg_id("PTR_PUSH");
        uint32_t ptr_length_id = reg_id("PTR_LENGTH");
        uint32_t ptr_clear_id = reg_id("PTR_CLEAR");
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
            void* data = right.children()[0].value().get();
            col.set(ptr.sidx,data);
        });

        uint32_t check_equality_int = overload_type(int_id,"==int","CHECK_EQUALITY_INT",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            bool result = (*(int*)ctx.node().children()[0].value().get()==*(int*)ctx.node().children()[1].value().get());
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


        uint32_t string_equals_id = overload_type(string_id,"=string","STRING_EQUALS",deadptr,[this](Context& ctx){
            standard_sub_process(ctx);
            string l(*(Ptr*)ctx.node().children()[0].value().get());
            if(!is_live(l)) {Ptr ticket = get_ticket(name_store_id,1,char_id); l = ticket; ctx.node().children()[0].value().set((void*)&ticket);}
            string r(*(Ptr*)ctx.node().children()[1].value().get());
            l.col().clear();
            for(int i=0;i<r.length();i++) {
                l.push(r.at(i));
            }
        });

        uint32_t string_append_id = overload_type(string_id,"+string","STRING_APPEND",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            string l(*(Ptr*)ctx.node().children()[0].value().get());
            string r(*(Ptr*)ctx.node().children()[1].value().get());
            if(!is_live(ctx.node().value().data_ptr())||!is_live(*(Ptr*)ctx.node().value().get())) {
                Ptr ticket = get_ticket(name_store_id,1,char_id); ctx.node().value().set((void*)&ticket);
            }
            string o(*(Ptr*)ctx.node().value().get());
            o.col().clear();
            o.push(l.to_std()); o.push(r.to_std());
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

        uint32_t precompile_brace = add_token_combo("precompile_brace",'#','#');
        uint32_t comment_brace = add_token_combo("comment_brace",'/','/');

        void init() override {
            register_type("list",ptr_id,sizeof(Ptr));

            overload_type(ptr_id,".\"push\"",ptr_push_id);
            overload_type(ptr_id,".\"take\"",ptr_take_id,make_value());
            overload_type(ptr_id,"<<any",ptr_push_id);
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
            x_handlers[ptr_clear_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                Col& col = resolve_to_col(*(Ptr*)left.value().get());
                col.clear();
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
                    target.push(*(char*)types[ptr.pool][ptr.idx][i]);
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
                    target.push(*(char*)types[ptr.pool][ptr.idx][i]);
                }
            };
            x_handlers[string_find_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                Ptr ptr = *(Ptr*)left.value().get();
                Col& tcol = types[ptr.pool][ptr.idx];
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
                fire_quals(ctx,ctx.node().value());
            };


            r_handlers[to_decl_id(star_id)] = [this](Context& ctx){
                if(ctx.node().value().type()!=ptr_id) {
                    ctx.node().value().type(ptr_id);
                    ctx.node().value().size(sizeof(Ptr));
                    ctx.node().value().quals().insert(0,make_node(ptr_id,"Ptr",make_value(ptr_id,sizeof(Ptr)),ctx.node().in_scope()));
                    fire_quals(ctx,ctx.node().value());
                    standard_sub_process(ctx);
                }
            };
            r_handlers[to_unary_id(amp_id)] = [this](Context& ctx){
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
                    if(ctx.left().type()==if_id) {
                        ctx.node().scopes()[0].owner(ctx.left());
                        ctx.left().scopes() << ctx.node().scopes()[0];
                        ctx.left().quals() << turn_into_token(ctx.node());
                        ctx.result().removeAt(ctx.index());
                        ctx.index()--;
                    }
                }
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
            x_handlers[for_id] = [this](Context& ctx) {
                process_node(ctx, ctx.node().children()[0]);
                while(true) {
                    process_node(ctx, ctx.node().children()[1]);
                    DEBUG_ONLY(if(ERROR_FLAG) {return;})
                    if(!(*(bool*)ctx.node().children()[1].value().get()))break;
                    uint32_t result = standard_travel_pass(ctx.node().scopes()[0], ctx.sub());
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


            x_handlers[var_decl_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node().value());
            };
            r_handlers[prefix_ptr_id] = [this](Context& ctx){
                if(is_live(ctx.value())) {
                    if(ctx.value().quals().length()>1) {
                        Node left = ctx.value().quals()[1];
                        ctx.value().sub_type(left.value().type());
                        ctx.value().sub_size(left.value().size());
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
                if(is_live(ctx.value())&&ctx.value().quals()[0]==ctx.qual()) {
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
                    Ptr p = string_to_Ptr(name); ctx.node().value().set((void*)&p);
                } else if(vtype==string_id) {
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

            r_handlers[make_tokenized_keyword("MISTAKE")] = [this](Context& ctx){
                print(ctx.node().value().reg());
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
            if(n.type()==identifier_id) {n.type(string_id);}
            Context ctx = make_context(); ctx.node(n);
            t_handlers.run(n.type())(ctx);
            m_handlers.run(n.type())(ctx);
            x_handlers.run(n.type())(ctx);
            // print(node_to_string(n));
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

            launch_blackfeather(root);
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
    struct mach_task_basic_info info;
    mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
    task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &size);
    return info.resident_size; // current RSS in bytes
}

namespace Acorn {
    struct Webcorn_Core : public virtual Acorn_Script {
        Webcorn_Core(uint16_t _uid) : Unit(_uid) {init();}
        Webcorn_Core() {init();}

        struct Server : q_object {
            int fd;
            std::string label;
            g_ptr<Thread> thread;
            Unit* unit;
        };
    
        list<g_ptr<Server>> servers;
    
        g_ptr<Server> get_server(int fd) {
            for(auto& s : servers) {
                if(s->fd == fd) return s;
            }
            return nullptr;
        }
    
        g_ptr<Server> get_server(const std::string& label) {
            for(auto& s : servers) {
                if(s->label == label) return s;
            }
            return nullptr;
        }

        uint32_t property_id = reg_id("property");
        uint32_t properties_id = reg_id("properties");
        uint32_t inlined_id = reg_id("inlined"); uint32_t suffix_inlined_id = reg_id("suffix_inlined"); uint32_t prefix_inlined_id = reg_id("prefix_inlined");
        uint32_t invisible_id = reg_id("invisible"); uint32_t suffix_invisible_id = reg_id("suffix_invisible"); uint32_t prefix_invisible_id = reg_id("prefix_invisible");
        uint32_t component_id = reg_id("component"); uint32_t suffix_component_id = reg_id("suffix_component"); uint32_t prefix_component_id = reg_id("prefix_component");

        uint32_t find_node_id = make_tokenized_keyword("find_node");

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
            "download", "controls", "autoplay", "loop", "muted", "poster"
        }, false};
        bool is_prop_structural(const std::string& name) {
            return is_structural[name] || name.substr(0,5) == "data-";
        }

        void emit_inline_html(Context& ctx) {
            if(ctx.node().mute()) return;
            std::string s = "";
            list<std::string> structural_prop_labels; list<std::string> structural_prop_values;
            list<std::string> style_prop_labels; list<std::string> style_prop_values;
            for(int q=0;q<ctx.node().quals().length();q++) {
                Node qual = ctx.node().quals()[q];
                for(int i=0;i<qual.children().length();i++) {
                    Node c = qual.children()[i];
                    if(c.type()==property_id) {
                        std::string prop = "";
                        std::string val = "";

                        if(c.children()[0].value().type()==string_id) {
                            process_node(ctx,c.children()[0]);
                            if(!is_live(c.children()[0].value().data_ptr())) { //For templates and such where we might use an identifer
                                continue;
                            }
                            prop = string(*(Ptr*)c.children()[0].value().get()).to_std();
                        } else {
                            prop = c.children()[0].name().to_std();
                        }

                        if(c.children()[1].value().type()==string_id) {
                            process_node(ctx,c.children()[1]);
                            if(!is_live(c.children()[1].value().data_ptr())) {
                                continue;
                            }
                            val = string(*(Ptr*)c.children()[1].value().get()).to_std();
                        } else {
                            val = c.children()[1].name().to_std();
                        }

                        list<std::string>* prop_labels; list<std::string>* prop_values;
                        if(is_prop_structural(prop)) {
                            prop_labels = &structural_prop_labels; 
                            prop_values = &structural_prop_values;
                        } else {
                            prop_labels = &style_prop_labels; 
                            prop_values = &style_prop_values;
                        }

                        if(q==0||!prop_labels->has(prop)) {
                            prop_labels->push(prop);
                            prop_values->push(val);
                        }
                    } else {

                    }
                }
            }


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
            std::string old_source = ctx.source().to_std();
            ctx.source().col().clear();
            ctx.node(node);
            emit_inline_html(ctx);
            std::string to_reutrn = ctx.source().to_std();
            ctx.node(old_node);
            ctx.source() = old_source;
            return to_reutrn;
        }

       Node make_property(Node type, Node value, Node parent) {
            Node prop_node = make_node(property_id);
            prop_node.children().push(type);
            prop_node.children().push(value);
            //prop_node.quals() << copy_as_token(parent);
            return prop_node;
        }

        void standard_gather_from_scope(Context& ctx) {
            Node node = ctx.node();
            if(!node.scopes().empty()) {
                Node scope = node.scopes()[0];
                Node properties = make_node(properties_id);
                properties.mute(true);
                scope.quals().push(properties);
                for(int i = 0;i<scope.children().length();i++) {
                    Node c = scope.children()[i];
                    if(c.type()==func_call_id) { //Anything defined with a type and identifer becomes a function call when refrenced elsewhere
                        Node ref = c.value().type_scope();
                        if(ref.idx==0 || ref.owner().idx==0) {
                            print(red("gather_from_scope:func_call type_scope or owner is null"));
                            continue;
                        }
                        Value ref_v = ref.owner().value();
                        if(ref_v.type()==inlined_id) {
                            for(int q=0;q<ref.quals().length();q++) {
                                scope.quals().push(ref.quals()[q]);
                            }
                            // scope.quals() << copy_as_token(c);
                            scope.children().removeAt(i);
                            i--;
                        } else if(ref_v.type()==component_id) {
                            if(ref.owner().children().empty()) {
                                // scope.quals() << copy_as_token(c);
                                Node owner = ref.owner();
                                scope.children_col().set(i,(void*)&owner);
                            } else {
                                instantiate_template(c,ref.owner(),ctx);
                            }
                        } else {
                            print("ACTUAL FUNC CALL");
                            //Is an actual func call, handle as such
                        }
                    } else if(c.type() == func_decl_id) {

                    } else if(c.children().empty()) {
                        if(is_live(c.value())&&c.value().type()==inlined_id) {
                            //scope.quals() << copy_as_token(c);
                            for(int q=0;q<c.scopes()[0].quals().length();q++) {
                                scope.quals().push(c.scopes()[0].quals()[q]);
                            }
                            scope.children().removeAt(i);
                            i--;
                        }
                    } else if(c.children().length()==2&&c.type()==colon_id) {
                        properties.children().push(make_property(c.children()[0],c.children()[1],c));
                        for(int q=0;q<properties.children().last().quals().length();q++) {
                            scope.quals().push(properties.children().last().quals()[q]); //Stealing the tokens for ourselves
                        }
                        properties.children().last().quals_col().clear();
                        scope.children().removeAt(i);
                        i--;
                    }
                }
            }
        }

        Node webcorn_node_scan(const std::string& label, Node from) {
            //print("SEARCHING: ",node_info(from));
            if(!from.scopes().empty()) {
                //print("SEARCHING ",from.scopes()[0].quals().length()," QUALS");
                for(int q=0;q<from.scopes()[0].quals().length();q++) {
                    Node qual = from.scopes()[0].quals()[q];
                    //print("  LOOKING AT ",node_info(qual));
                    for(int i=0;i<qual.children().length();i++) {
                        Node c = qual.children()[i];
                        //print("   LOOKING AT ",node_to_string(c));
                        if(c.type()==property_id) {
                            std::string prop = "";
                            std::string val = "";
    
                            if(c.children()[0].value().type()==string_id) { //Figure out why the props for sheet aren't resolving so this workaround isnt' nessecary
                                if(!is_live(c.children()[0].value().data_ptr())) {
                                    process_node(c.children()[0],deadptr);
                                    if(!is_live(c.children()[0].value().data_ptr())) {
                                        continue;
                                    }
                                }
                                prop = string(*(Ptr*)c.children()[0].value().get()).to_std();
                            } else {
                                prop = c.children()[0].name().to_std();
                            }
    
                            if(c.children()[1].value().type()==string_id) {
                                if(!is_live(c.children()[1].value().data_ptr())) {
                                    process_node(c.children()[1],deadptr);
                                    if(!is_live(c.children()[1].value().data_ptr())) {
                                        continue;
                                    }
                                }
                                val = string(*(Ptr*)c.children()[1].value().get()).to_std();
                            } else {
                                val = c.children()[1].name().to_std();
                            }

                            //print("   ",prop,":",val);

                            if(prop=="id"&&val==label) return from;
                        }
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

        std::string ColColCol_to_DebugSheet(Context& ctx, Ptr ptr, uint32_t offset = 0) {
            if(resolve_to_unit(ptr).length()<=ptr.pool||ptr.pool<0) return  "<div id='"+ctx.sub().node().name().to_std()+"'><p \"style=color:red\"> OUT OF BOUNDS: "+std::to_string(ptr.pool)+"</p></div>";
            g_ptr<style_manager> styles = make<style_manager>(this);
            std::string out = "";
            out+="<div id='"+ctx.sub().node().name().to_std()+"'>";
            out+="<p>"+std::to_string(ptr.pool)+"</p>";
            out += "<table ";
            out += emit_inline_html(ctx, ctx.sub().node());
            if(!ctx.sub().node().scopes().empty()) {
                node_col props = ctx.sub().node().scopes()[0].children();
                for(int i=0;i<props.length();i++) {
                    styles->add_prop(props[i].name().to_std(),props[i].scopes()[0]);
                }    
            }
            out += ">\n";
            ColCol& rendersheet = resolve_to_pool(ptr);
            out+= "<tr "; 
            out+=styles->resolve_prop(ctx, "row_style"); 
            out+=">\n";
            for(int c = 0;c<rendersheet.length();c++) {
                out += "<th ";
                out+=styles->resolve_prop(ctx, "header_style"); 
                out+=">";
                out += rendersheet[c].label.to_std();

                if(rendersheet.tag == 0) { //If it's a store pool or direct values
                    ptr.idx = c;
                    out += "<div class='popup' style='"
                           "display:none;position:absolute;top:100%;left:0;"
                           "background:white;border:1px solid #ccc;border-radius:4px;"
                           "padding:4px;z-index:100;white-space:nowrap'>"
                           "<button onclick=\"fragthree('"+ctx.sub().node().name().to_std()+"','add_row_col','"+std::to_string(c)+"')\">+</button>"
                           "<button onclick=\"fragthree('"+ctx.sub().node().name().to_std()+"','remove_row_col','"+std::to_string(c)+"')\">-</button>"
                           "</div>";
                }

                out += "</th>";
            }
            out += "</tr>";
            
            uint32_t max_rows = 0;
            for(int c = 0;c<rendersheet.length();c++) if(rendersheet[c].length() > max_rows) max_rows = rendersheet[c].length();
            
            for(int r = 0; r < max_rows; r++) {
                ptr.sidx = r;
                out += "<tr ";
                out+=styles->resolve_prop(ctx, "row_style"); 
                out+=">";
                for(int c = 0;c<rendersheet.length();c++) {
                    Col& col = rendersheet[c];
                    std::string tostr = "";
                    if(r<col.length()) {
                        if(rendersheet.tag==0) { //This sheet stores direct values
                            tostr = tag_to_str(col.tag,col[r]);
                        } else if(rendersheet.tag==1) { //This sheet stores Ptrs
                            Ptr p = *(Ptr*)col[r]; 
                            if(is_live(p)) {
                                p.unit = ptr.unit;
                                Col& vcol = resolve_to_col(p);
                                tostr = tag_to_str(vcol.tag,vcol[p.sidx]);
                            }
                        }
                    }
                    ptr.idx = c;
                    out += "<td ";
                    out+=styles->resolve_prop(ctx, "column_style"); 
                    out+=">\n<input "; 
                    out+=styles->resolve_prop(ctx, "input_style"); 
                    out+=" value=\""+tostr+"\"";
                    out += "onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','setcell','"+Ptr_to_string(ptr)+"='+this.value)\"";
                    out += "/>";
                    if(rendersheet.tag == 0) {
                        out += "<div class='context_menu' ";
                        out += styles->resolve_prop(ctx, "context_menu_style");
                        out += ">";
                        out += "<div ";
                        out += styles->resolve_prop(ctx, "context_menu_header_style");
                        out += ">Cell " + Ptr_to_string(ptr) + "</div>";
                        out += "<div ";
                        out += styles->resolve_prop(ctx, "context_menu_body_style");
                        out += ">";
                        out += "<label ";
                        out += styles->resolve_prop(ctx, "context_menu_label_style");
                        out += ">Label</label>";
                        out += "<input ";
                        out += styles->resolve_prop(ctx, "context_menu_input_style");
                        std::string str = "";
                        if(col.cells.length()>r) {
                            //Same problem in ColColCol_to_form
                            //This is a bit dangerous and janky, in the future we'll use tags to ensure we're casting the right type of key
                            str = ((QString&)col.cells[r]).to_std(); //To retrive the key as a string
                        }
                        out += "value=\""+str+"\" onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','labelcell','"+Ptr_to_string(ptr)+"='+this.value)\"/>";
                        out += "</div></div>";
                    }
                    out+="\n</td>\n";
                }
                out += "</tr>";
            }
            out += "</table>";
            out+="</div>";
            return out;
        }

        std::string ColColCol_to_Form(Context& ctx, Ptr ptr, uint32_t offset = 0) {
            uint32_t target_pool = ptr.pool;
            if((resolve_to_unit(ptr).length()-offset)<10) return "<div id='"+ctx.sub().node().name().to_std()+"'></div>";
            ptr.pool = offset;   ColCol& sheetsheet    = resolve_to_pool(ptr);
            ptr.pool = offset+5; ColCol& datasheet     = resolve_to_pool(ptr);
            ptr.pool = offset+6; ColCol& metadatasheet = resolve_to_pool(ptr);
            ptr.pool = offset+7; ColCol& notesheet     = resolve_to_pool(ptr);
            ptr.pool = offset+8; ColCol& scriptsheet   = resolve_to_pool(ptr);
            ptr.pool = offset+9; ColCol& storesheet    = resolve_to_pool(ptr);
            ptr.pool = target_pool;
        
            g_ptr<style_manager> styles = make<style_manager>(this);
            std::string out = "<div id='"+ctx.sub().node().name().to_std()+"' ";
            out += emit_inline_html(ctx, ctx.sub().node());
            if(!ctx.sub().node().scopes().empty()) {
                node_col props = ctx.sub().node().scopes()[0].children();
                for(int i=0;i<props.length();i++) {
                    styles->add_prop(props[i].name().to_std(),props[i].scopes()[0]);
                }    
            }
            out += ">\n";

            uint32_t max_rows = 0;
            for(int c = 0;c<sheetsheet.length();c++) if(sheetsheet[c].length() > max_rows) max_rows = sheetsheet[c].length();
 
            for(int r = 0; r < max_rows; r++) {
                ptr.sidx = r;
                for(int c = 0; c < sheetsheet.length(); c++) {
                    ptr.idx = c;
                    Col& sheetcol = sheetsheet[c];
                    Col& datacol = datasheet[c];
                    Col& metacol = metadatasheet[c];
                    
                    std::string label = datacol.label.to_std();
                    Ptr metadata = *(Ptr*)metacol[r];
                    uint32_t widget_type = metadata.pool;

                    Ptr sheetptr = *(Ptr*)sheetcol[r];
                    std::string current = value_as_string(sheetptr);

                    if(datacol.length() > 0) {
                        Ptr p = *(Ptr*)datacol[r];
                        if(is_live(p)) {
                            Col& vcol = resolve_to_col(p);
                            QString sublabel = vcol.label;
                            if(sublabel.empty()) sublabel = std::to_string(c+r);

                            out += "<div ";
                            out += styles->resolve_prop(ctx, "field_style");
                            out += ">\n<label ";
                            out += styles->resolve_prop(ctx, "label_style");
                            out += ">" + sublabel.to_std() + "</label>\n";

                            if(widget_type==0) {
                                out += "<select ";
                                out += styles->resolve_prop(ctx, "select_style");
                                ptr.pool = offset; //Targeting the sheet cell itself
                                out += " onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','setcell','"+Ptr_to_string(ptr)+"='+this.value);";
                                out += "\">\n";
                                for(int v = 0; v < vcol.length(); v++) {

                                    std::string str = std::to_string(v);
                                    if(vcol.cells.length()>v) {
                                        //This is a bit dangerous and janky, in the future we'll use tags to ensure we're casting the right type of key
                                        str = ((QString&)vcol.cells[v]).to_std(); //To retrive the key as a string
                                    }
                                    p.sidx = v;
                                    std::string val = value_as_string(p);
                                    p.sidx = 0; //Could also store the selected in the sidx of p
                                    std::string selected = (val==current) ? " selected" : ""; //For now, in the future add a check against the value at 0 for this column
                                    out += "<option value='"+val+"'"+selected+">"+str+"</option>\n";
                                }
                                out += "</select>\n";
                            } else if(widget_type==1) {
                                out += "<input ";
                                out += styles->resolve_prop(ctx, "form_input_style");
                                out += " value=\""+current+"\"";
                                ptr.pool = offset; //Targeting the sheet cell itself
                                out += " onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','setcell','"+Ptr_to_string(ptr)+"='+this.value);";
                                out += "/>\n";
                            } else {

                            }
                        }
                    }
                }
            }
            out += "</div>";
            return out;
        }

        std::string ColColCol_to_Sheet(Context& ctx, Ptr ptr, uint32_t offset = 0) {
            uint32_t target_pool = ptr.pool;
            if((resolve_to_unit(ptr).length()-offset)<5) return "<table id='"+ctx.sub().node().name().to_std()+"'></table>";
            ptr.pool = offset;   ColCol& datasheet = resolve_to_pool(ptr);
            ptr.pool = offset+1; ColCol& metadatasheet = resolve_to_pool(ptr);
            ptr.pool = offset+2; ColCol& notesheet = resolve_to_pool(ptr);
            ptr.pool = offset+3; ColCol& scriptsheet = resolve_to_pool(ptr);
            ptr.pool = offset+4; ColCol& storesheet = resolve_to_pool(ptr);
            ptr.pool = offset;
            
            g_ptr<style_manager> styles = make<style_manager>(this);

            std::string out = "";
            out += "<table id='"+ctx.sub().node().name().to_std()+"' ";
            out += emit_inline_html(ctx, ctx.sub().node());
            if(!ctx.sub().node().scopes().empty()) {
                node_col props = ctx.sub().node().scopes()[0].children();
                for(int i=0;i<props.length();i++) {
                    styles->add_prop(props[i].name().to_std(),props[i].scopes()[0]);
                }    
            }
            out += ">\n";
            out+= "<tr "; 
            out+=styles->resolve_prop(ctx, "row_style"); 
            out+=">\n";
            for(int c = 0;c<datasheet.length();c++) {
                out += "<th ";
                out+=styles->resolve_prop(ctx, "header_style"); 
                out+=">";
                out += datasheet[c].label.to_std();
                out += "</th>";
            }
            out += "</tr>";
            
            uint32_t max_rows = 0;
            for(int c = 0;c<datasheet.length();c++) if(datasheet[c].length() > max_rows) max_rows = datasheet[c].length();
            
            for(int r = 0; r < max_rows; r++) {
                ptr.sidx = r;
                out += "<tr ";
                out+=styles->resolve_prop(ctx, "row_style"); 
                out+=">";
                for(int c = 0;c<datasheet.length();c++) {
                    Col& col = datasheet[c];
                    std::string tostr = "";
                    if(r<col.length()) {
                        Ptr p = *(Ptr*)col[r]; //Since the datasheet stores Ptrs
                        if(is_live(p)) {
                            tostr = value_as_string(p);
                        }
                    }
                    ptr.idx = c;
                    out += "<td ";
                    out+=styles->resolve_prop(ctx, "column_style"); 
                    out+=">\n<input "; 
                    out+=styles->resolve_prop(ctx, "input_style"); 
                    out+=" value=\""+tostr+"\""
                    + " onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','setcell','"+Ptr_to_string(ptr)+"='+this.value)\""
                    +"/>\n</td>\n";
                }
                out += "</tr>";
            }
            
            out += "</table>";
            return out;
        }

        //render_sheet(sheetid, poolid, "render as")
        //Render as options: static, sheet, form, debug
        uint32_t render_sheet_id = add_function("render_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            if(ctx.node().children().length()!=3) {print(red("Wrong number of arguments for render_sheet, expected 3")); return;}
            int sheetid = *(int*)ctx.node().children()[0].value().get();
            int poolid = *(int*)ctx.node().children()[1].value().get();
            string renderas = (string&)*(Ptr*)ctx.node().children()[2].value().get();
            print("RENDERING POOL: ",poolid," AS ",renderas," FROM ",sheetid);
            if(sheetid!=0) {
                Ptr ptr(poolid,0,0,sheetid);
                if(renderas.to_std()=="static") {
                    //ctx.sub().source().push(ColCol_to_Static(ctx,ptr));
                } else if(renderas.to_std()=="sheet") {
                    ctx.sub().source().push(ColColCol_to_Sheet(ctx,ptr));
                } else if(renderas.to_std()=="form") {
                    ctx.sub().source().push(ColColCol_to_Form(ctx,ptr));
                } else if(renderas.to_std()=="debug") {
                    ctx.sub().source().push(ColColCol_to_DebugSheet(ctx,ptr));
                } else {
                    print(red("Unrecognized render type for render_sheet "),renderas);
                }
                // units[sheetid]->dump_unit(true);
            } else {
                ctx.sub().source().push("<table id='"+ctx.sub().node().name().to_std()+"'></table>");
            }
        });

        uint32_t create_sheet_id = add_function("create_sheet",[this](Context& ctx){
            ColColCol sheet;
            ColCol data_pool; data_pool.tag=1; sheet.push(data_pool); //Tag 1 means that everything here is a Ptr to something else
            ColCol metadata_pool; metadata_pool.tag=1; sheet.push(metadata_pool); 
            ColCol notes_pool; notes_pool.tag=1; sheet.push(notes_pool);
            ColCol scripts_pool; scripts_pool.tag=1; sheet.push(scripts_pool);
            ColCol store_pool; store_pool.tag=0; sheet.push(store_pool); //Tag 0 means that direct values are stored here
            uint32_t sheetid = (uint32_t)make_unit(sheet);
            ctx.node().value().set((void*)&sheetid);
        },4,int_id);
        uint32_t add_form_id = add_function("add_form",[this](Context& ctx){
            standard_sub_process(ctx);
            int idx = *(int*)ctx.node().children()[0].value().get();
            ColColCol& sheet = (*units[idx]).types;
            
            //Snapshot shape from first non-store pool
            int ncols = 0, nrows = 0;
            for(int o = 0; o < sheet.length(); o++) {
                if(sheet[o].tag == 0) continue;
                ncols = sheet[o].length();
                nrows = ncols > 0 ? sheet[o][0].length() : 0;
                break;
            }
        
            auto make_pool = [&](uint8_t tag) {
                ColCol pool; pool.tag = tag;
                if(tag == 0) { sheet.push(pool); return; } //Store pool, empty is fine
                for(int c = 0; c < ncols; c++) {
                    Col col(sizeof(Ptr)); col.tag = ptr_id;
                    for(int r = 0; r < nrows; r++) col.push_default();
                    pool.push(col);
                }
                sheet.push(pool);
            };
        
            make_pool(1); //Data
            make_pool(1); //Metadata
            make_pool(1); //Notes
            make_pool(1); //Scripts
            make_pool(0); //Store

            ColCol& store = sheet[9];
            uint32_t a = create_column(store,4,int_id);
            bool use_corrupt = true;
            int numA = 5;
            int numB = 8;
            int numC = 12;
            if(use_corrupt) {
                store[a].put("opt",(void*)&numA,string_id);
                store[a].put("nopt",(void*)&numB,string_id);
                store[a].put("bopt",(void*)&numC,string_id);
            } else {
                store[a].push((void*)&numA);
                store[a].push((void*)&numB);
                store[a].push((void*)&numC);
            }
            Ptr to_store(9,0,0,idx);
            sheet[5][0].qset(0,(void*)&to_store,sizeof(Ptr));
            print("Added form elements");
            units[idx]->dump_unit(true);
        });
        uint32_t load_sheet_id = add_function("load_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            string s(*(Ptr*)ctx.node().children()[0].value().get());
            uint32_t sheetid = 0;
            for(int u=0;u<units.length();u++) {
                if(units[u]->types.label==s.to_std()) {
                    sheetid = u; break;
                }   
            }
            if(sheetid==0) {
                auto in = openReadStream("mixos-acorn/web/thistle/users/fir/sheets/"+s.to_std());
                print("Loading ",s.to_std());
                ColColCol loadsheet = read_TypeTypeCol(in);
                //print("Loaded: ",loadsheet.length()," sheets");

                // print("Saving");
                // auto reout = openWriteStream("savetest.wub");
                // write_TypeTypeCol(reout, loadsheet);
                // reout.close();
                // std::vector<uint8_t> b = readFileBytes("savetest.wub");
                // print("Loaded ",b.size()," bytes");
                // writeHex("mixos-acorn/web/thistle/users/fir/sheets/testload",b);
                // print("Saved");

                print("Loaded, making a unit");
                sheetid = (uint32_t)make_unit(loadsheet);
                ColColCol& sheet = units[sheetid]->types;
                print("Unit made, normalizing");
                for(int p = 0;p<sheet.length();p++) {
                    //print(p,"/",sheet.length()," [",sheet[p].tag,"]");
                    if(sheet[p].tag==1) { //Stores Ptrs, so it needs to be normalized
                        ColCol& pool = sheet[p];
                        for(int c=0;c<pool.length();c++) {
                            Col& col = pool[c];
                            for(int r=0;r<col.length();r++) {
                                //print(c,"/",pool.length(),":",r,"/",col.length());
                               Ptr ptr = *(Ptr*)col[r];
                               //print("Normalizing: ",Ptr_to_string(ptr));
                               if(is_live(ptr)) {
                                    ptr.unit = sheetid;
                                    col.set(r,(void*)&ptr);
                               }
                               //print("Normalized: ",Ptr_to_string(*(Ptr*)col[r]));
                            }
                        }   
                    }
                }
                print("Unit normalized");
            }

            print("Rendering ",sheetid);
            units[sheetid]->dump_unit(true);
            print("Set and finished");
            ctx.node().value().set((void*)&sheetid);
        },4,int_id);
        uint32_t save_sheet_id = add_function("save_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            uint16_t sheetid = *(uint16_t*)ctx.node().children()[0].value().get();
            string s(*(Ptr*)ctx.node().children()[1].value().get());
            auto out = openWriteStream("mixos-acorn/web/thistle/users/fir/sheets/"+s.to_std());
            units[sheetid]->types.label = s.to_std();
            //print("Saving: ",units[sheetid]->types.length()," sheets");
            write_TypeTypeCol(out,units[sheetid]->types);
            out.close();

            //writeHex("mixos-acorn/web/thistle/users/fir/sheets/test", readFileBytes("mixos-acorn/web/thistle/users/fir/sheets/fsa.twg"));
        });
        uint32_t add_column_id = add_function("add_column_to_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            int idx = *(int*)ctx.node().children()[0].value().get();
            int offset = *(int*)ctx.node().children()[1].value().get();
            for(int o=offset;o<(*units[idx]).types.length();o++) { //We're iterating over each of the diffrent pools in the sheet by offset
                if((*units[idx])[o].tag==0) continue; //If it's a store pool
                Col ncol(sizeof(Ptr)); ncol.tag = ptr_id;
                if(!(*units[idx])[o].empty()) { //We need to ensure there's always the same ammount of rows in each column
                    for(int i=0;i<((*units[idx])[o][0].length());i++) {
                        ncol.push_default();
                    }
                }
                (*units[idx])[o].push(ncol);
            }
        });
        uint32_t add_row_id = add_function("add_row_to_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            int idx = *(int*)ctx.node().children()[0].value().get();
            int offset = *(int*)ctx.node().children()[1].value().get();
            for(int o=offset;o<(*units[idx]).types.length();o++) { //We're iterating over each of the diffrent pools in the sheet by offset
                if((*units[idx])[o].tag==0) continue; //If it's a store pool
                for(int i=0;i<(*units[idx])[o].length();i++) {
                    (*units[idx])[o][i].push_default();
                }
            }
        });
        uint32_t add_row_to_col_id = add_function("add_row_to_col",[this](Context& ctx){
            standard_sub_process(ctx);
            int idx = *(int*)ctx.node().children()[0].value().get();
            int pool = *(int*)ctx.node().children()[1].value().get();
            int col = *(int*)ctx.node().children()[2].value().get();
            (*units[idx])[pool][col].push_default();
        });
        uint32_t removw_row_from_col_id = add_function("remove_row_from_col",[this](Context& ctx){
            standard_sub_process(ctx);
            int idx = *(int*)ctx.node().children()[0].value().get();
            int pool = *(int*)ctx.node().children()[1].value().get();
            int column = *(int*)ctx.node().children()[2].value().get();
            Col& col = (*units[idx])[pool][column];
            if(col.cells.length()==col.length()) col.cells.removeAt(col.cells.length()-1,sizeof(CCol));
            col.removeAt(col.length()-1);
        });
        uint32_t setcell_id = add_function("setcell",[this](Context& ctx){
            standard_sub_process(ctx);
            string addr = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            list<std::string> terms = split_str(addr.to_std(),'=');
            Ptr cellptr = string_to_Ptr(terms[0]);
            uint32_t pooltag = resolve_to_pool(cellptr).tag;
            if(pooltag==0) { //The tag on the pool dictates how it's values are stored
                Node literal = compile_literal(terms[1]);
                resolve_to_col(cellptr).set(cellptr.sidx,literal.value().get());
            } else if(pooltag==1) {
                Ptr p = *(Ptr*)resolve_ptr(cellptr);
                Node literal = compile_literal(terms[1]);
                Value lv = literal.value();
                if(!is_live(p)) {
                    Ptr storeptr = cellptr;
                    storeptr.pool+=4; //To get to the store pool (this could be a bit fragile)
                    p = get_ticket(storeptr,lv.size(),lv.type());
                    resolve_to_col(cellptr).set(cellptr.sidx,(void*)&p);
                }

                void* data = lv.get();
                Col& col = resolve_to_col(p); //Where the value is stored in the store pool
                if(lv.type()==string_id) {
                    if(col.tag!=string_id||col.empty()) {
                        col.clear(); 
                        col.element_size = lv.size(); col.tag=lv.type();
                        Ptr storeptr = cellptr;
                        storeptr.pool+=4;
                        Ptr charp = get_ticket(storeptr,1,char_id); //Col is unsafe to use after this
                        string str = (string&)charp;
                        string lstr = (string&)*(Ptr*)lv.get();
                        str = lstr.to_std();
                        resolve_to_col(p).push((void*)&charp);
                        return;
                    } else {
                        data = col[p.sidx];
                        string str = (string&)*(Ptr*)col[p.sidx];
                        string lstr = (string&)*(Ptr*)lv.get();
                        str = lstr.to_std();
                    }
                } 

                if(col.element_size!=lv.size()||col.tag!=lv.type()) {
                    col.clear();
                    col.element_size = lv.size(); col.tag=lv.type();
                    col.push(data);
                } else if(col.empty()) {
                    col.push(data);
                } else {
                    col.set(p.sidx,data);
                }
            }
        });
        uint32_t labelcell_id = add_function("labelcell",[this](Context& ctx){
            standard_sub_process(ctx);
            string addr = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            list<std::string> terms = split_str(addr.to_std(),'=');
            Ptr cellptr = string_to_Ptr(terms[0]);
            uint32_t pooltag = resolve_to_pool(cellptr).tag;
            Col& cellcol = resolve_to_col(cellptr);
            if(pooltag==0) { //The tag on the pool dictates how it's values are stored
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

        map<std::string,uint32_t> routes;
        map<uint32_t,Node> route_nodes;


        uint32_t div_id = make_tokenized_keyword("div");

        void init() override {
            set_binding_powers(colon_id,4,6);
            // register_type("div",component_id,0);
            register_type("inlined",inlined_id,0);
            register_type("invisible",invisible_id,0);

            n_handlers[div_id] = [this](Context& ctx){
                if(ctx.result().get(ctx.index()+1).type()!=lbrace_id) {
                    Node take = ctx.result().take(ctx.index()+1);
                    ctx.node().name(take.name().to_std()); 
                    for(int i=0;i<take.children().length();i++) {
                        ctx.node().children() << take.children()[i];
                    }
                }
            };
            t_handlers[div_id] = [this](Context& ctx) {
                Node node = ctx.node();
                ctx.node().value(make_value(component_id));
                node.scopes()[0].owner(node);
                node.scopes()[0].name(node.name().to_std());
                node.type(func_decl_id);
                node.scopes()[0] = distribute_node(node.in_scope(),node.name().to_std(),node.scopes()[0]);
                node.value().type_scope(node.scopes()[0]);
                node.value().sub_type(0);
                node.value(distribute_value(node.in_scope(),node.name().to_std(),node.value()));
                for(int c=0;c<node.children().length();c++) {
                    place_node_in_scope(node.children()[c],node.scopes()[0]);
                }
                ctx.node().type(func_decl_id);
            };

            r_handlers[func_decl_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                if(ctx.node().type()==func_call_id) {
                    if(ctx.node().value().type()!=component_id) {
                        //instantiate_template(ctx.node(),ctx.node().value().type_scope().owner(),ctx);
                        sync_args(ctx);
                    }
                } else {
                    Node scope = ctx.node().scopes()[0];
                    if(!is_live(scope.value())) {
                        scope.value(make_value()); 
                        scope.value().loc(0); //Set location for stack depth
                    }
                }
                if(ctx.node().value().type()==component_id) {
                    standard_gather_from_scope(ctx);    
                    if(!ctx.node().scopes().empty()) {
                        if(ctx.node().type()==func_decl_id&&!ctx.node().children().empty()) {
                            ctx.node().value().type(invisible_id); //For templates which we don't want to emit
                        }
                        for(int i=0;i<ctx.node().scopes().length();i++) {
                            Node s = ctx.node().scopes()[i];
                            if(ctx.node().value().type()==invisible_id) {
                                s.type(invisible_id);
                            // } else if(ctx.node()->value->type==foldable_id) {
                            //     s->type = foldable_id;
                            // } else if(ctx.node()->value->type==iframe_id) {
                            //     s->type = iframe_id;
                            } else {
                                s.type(component_id);
                            }
                        }
                    }
                }
            };
            r_handlers[func_call_id] = r_handlers[func_decl_id];

            x_handlers[make_tokenized_keyword("gather_props")] = [this](Context& ctx){
                ctx.node(ctx.sub().node());
                standard_gather_from_scope(ctx);
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

            ColCol t;
            for(int i=0;i<5;i++) {
                Col c;
                c.element_size = 4;
                c.tag = int_id;
                for(int n=0;n<8;n++) {
                    c.push((void*)&n);
                }
                t.push(c);
            }
            types.push(t);


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

            x_handlers[make_tokenized_keyword("run_server")] = [this](Context& ctx){
                standard_sub_process(ctx);
                int server_fd = *(int*)ctx.node().children()[0].value().get();
                g_ptr<Server> new_server = make<Server>();
                new_server->fd = server_fd;
                new_server->thread = make<Thread>();
                new_server->unit = this;
                servers << new_server;
                
                if(!ctx.node().scopes().empty()) {
                    Node scope = ctx.node().scopes()[0];
                    new_server->thread->run_blocking([this, scope, ctx]() mutable {
                        standard_travel_pass(scope, ctx);
                    });
                }
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
                Col& col = types[strptr.pool][strptr.idx];
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
                    servers.last()->label = target;
                    print("SPINNING UP A NEW SERVER ON ",port_num," CALLED ",servers.last()->label);
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