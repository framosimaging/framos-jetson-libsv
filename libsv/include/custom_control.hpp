#pragma once

#include "generic_control.hpp"
#include <functional>

class CustomControl : public GenericControl {

    public:
        explicit CustomControl(uint32_t id, std::string name, int64_t minValue, int64_t maxValue, int64_t stepValue, int64_t defaultValue, 
        std::function<int64_t()> getCallback, std::function<bool(int64_t)> setCallback)
        : GenericControl(id, name, minValue, maxValue, stepValue, defaultValue), 
        getCallback(getCallback), setCallback(setCallback) {} 
        
        explicit CustomControl(uint32_t id, std::string name, ControlMenu menu, std::function<int64_t()> getCallback, std::function<bool(int64_t)> setCallback) 
        : GenericControl(id, name, 0, FinalMenuIndex(menu), 1, 0, menu), 
        getCallback(getCallback), setCallback(setCallback) {}

        int64_t Get() override { return getCallback(); }
        bool SetValue(int64_t val) override { return setCallback(val); }        


    private:
        std::function<int64_t()> getCallback;
        std::function<bool(int64_t)> setCallback;
};