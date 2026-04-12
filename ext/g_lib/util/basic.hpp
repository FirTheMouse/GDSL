#pragma once
#include <iostream>
#include <random>
#include <chrono>

template<typename... Args>
void print(Args&&... args) {
  (std::cout << ... << args) << std::endl;
}

template<typename... Args>
void printnl(Args&&... args) {
  (std::cout << ... << args);
}


static inline std::string green(const std::string& text) {
  return "\x1b[32m"+text+"\x1b[0m";
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


static inline std::string rgb(const std::string& text, int r, int g, int b) {
  return "\x1b[38;2;"+std::to_string(r)+";"+std::to_string(g)+";"+std::to_string(b)+"m"+text+"\x1b[0m";
}
static inline std::string bg(const std::string& text, int r, int g, int b) {
  return "\x1b[48;2;"+std::to_string(r)+";"+std::to_string(g)+";"+std::to_string(b)+"m"+text+"\x1b[0m";
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


