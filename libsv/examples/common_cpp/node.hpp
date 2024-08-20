#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <atomic>

namespace common
{
    template<class Output>
    class Node {

        public:

            Node() : nodeActive(false), nodeAlive(true), outputInitialized(false), outputReady(false)
            {
                nodeThread = std::thread(&Node::NodeThread, this);
            }

            virtual ~Node() 
            {
                nodeAlive = false;
                outputCondition.notify_all();
                threadCondition.notify_all();

                if (nodeThread.joinable()) {
                    nodeThread.join();
                }
            }

            void Start()
            {
                InitializeAction();

                ReinitializeOutput();

                nodeActive = true;
                threadCondition.notify_all();
            }

            void Stop()
            {
                std::unique_lock<std::mutex> lock(threadMutex);
                threadCondition.wait(lock, [&]{ return true; });

                DeintializeAction();

                nodeActive = false;
            }

            Output GetOutputBlocking() 
            { 
                std::unique_lock<std::mutex> lock(outputMutex);
                outputCondition.wait(lock, [&]{ return outputReady || !nodeAlive; } );

                MakeOutputAvailable();

                return availableOutput;
            }

            bool GetOutputNonBlocking(Output &output)
            {
                if (outputReady) {
                    MakeOutputAvailable();
                    output = availableOutput;
                    return true;
                }

                return false;
            }

            void ReturnOutput()
            {
                ReturnOutput(availableOutput);
            }

        protected:
            
            virtual void PerformAction(Output &output) = 0;
            
            virtual void InitializeAction()
            {

            }

            virtual void DeintializeAction()
            {

            }

            virtual void InitializeOutput(Output &output)
            {
                (void)output;
            }

            virtual void DeinitializeOutput(Output &output)
            {
                (void)output;
            }

            virtual void ReturnOutput(Output &output)
            {
                (void)output;
            }

        private:

            std::atomic<bool> nodeActive;
            std::atomic<bool> nodeAlive;
            std::thread nodeThread;
            std::mutex threadMutex;
            std::condition_variable threadCondition;

            bool outputInitialized;
            bool outputReady;
            Output availableOutput;
            Output readyOutput;
            Output activeOutput;
            std::mutex outputMutex;
            std::condition_variable outputCondition;

            void NodeThread() 
            {
                while (nodeAlive) {
        
                    std::unique_lock<std::mutex> lock(threadMutex);
                    threadCondition.wait(lock, [&]{ return nodeActive || !nodeAlive; } );
                    if (!nodeAlive) return;

                    PerformAction(activeOutput);

                    SetOutput();

                    lock.unlock();
                    threadCondition.notify_all();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }

            void SetOutput()
            {
                std::unique_lock<std::mutex> lock(outputMutex);

                if (outputReady) {
                    ReturnOutput(readyOutput);
                }

                std::swap(readyOutput, activeOutput);
                outputReady = true;
            
                lock.unlock();
                outputCondition.notify_all();
            }

            void ReinitializeOutput()
            {
                if (outputInitialized) {
                    DeinitializeOutput();
                }

                InitializeOutput();
            }

            void DeinitializeOutput()
            {
                DeinitializeOutput(availableOutput);
                DeinitializeOutput(readyOutput);
                DeinitializeOutput(activeOutput);
                outputInitialized = false;
            }

            void InitializeOutput()
            {
                InitializeOutput(availableOutput);
                InitializeOutput(readyOutput);
                InitializeOutput(activeOutput);
                outputInitialized = true;
            }

            void MakeOutputAvailable()
            {
                std::swap(availableOutput, readyOutput);
                outputReady = false;
            }
    };
}