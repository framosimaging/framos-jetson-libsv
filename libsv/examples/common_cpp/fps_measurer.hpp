#pragma once

#include <atomic>
#include <thread>

namespace common
{
    class FpsMeasurer 
    {
        public:

            FpsMeasurer() : frameCounter(0), fps(0)
            {
                fpsThreadActive = true;
                fpsThread = std::thread(&FpsMeasurer::FpsThread, this);
            }

            ~FpsMeasurer()
            {
                fpsThreadActive = false;
                fpsCondition.notify_all();

                if (fpsThread.joinable()) {
                    fpsThread.join();
                }
            }

            void FrameReceived()
            {
                ++frameCounter;
            }

            uint32_t GetFps()
            {
                return fps;
            }

        private:
            std::atomic<bool> fpsThreadActive;
            std::thread fpsThread;
            std::mutex fpsMutex;
            std::condition_variable fpsCondition;
            std::atomic<uint32_t> frameCounter;
            std::atomic<uint32_t> fps;

            void FpsThread() 
            {
                while(fpsThreadActive) {

                    std::unique_lock<std::mutex> lock(fpsMutex);
                    fpsCondition.wait_for(lock, std::chrono::seconds(1));
                    
                    fps.exchange(frameCounter);
                    frameCounter = 0;
                }
            }
    };
}