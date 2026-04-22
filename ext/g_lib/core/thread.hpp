#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <cmath> 
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>
#include <thread>
#include <future>
#include <functional>
#include "../util/util.hpp"
#include "../core/q_object.hpp"

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