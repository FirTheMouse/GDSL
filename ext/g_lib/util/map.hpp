#pragma once
#include "../util/list.hpp"
#include "../core/q_object.hpp"

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
    //    std::vector<int> ints;
    //    ints.push_back(1);
    //    volatile int b = ints[3];
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

    uint32_t hashString(const std::string& str) {
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