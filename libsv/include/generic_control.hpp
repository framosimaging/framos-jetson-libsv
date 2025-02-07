#pragma once

#include "sv/sv.h"
#include "control_menu.hpp"
#include <string>
#include <algorithm>

class GenericControl : public IControl
{
    public:
        explicit GenericControl(uint32_t id, std::string name, int64_t minValue, int64_t maxValue, int64_t stepValue, int64_t defaultValue, ControlMenu menu = ControlMenu()) 
        : id(id), name(name), minValue(minValue), maxValue(maxValue), stepValue(stepValue), defaultValue(defaultValue), menu(menu) {}
        virtual ~GenericControl() = default;
        uint32_t GetID() override { return id; }
        const char* GetName() override { return name.c_str(); }
        int64_t GetMinValue() override { return minValue; }
        int64_t GetMaxValue() override { return maxValue; }
        int64_t GetStepValue() override { return stepValue; }
        int64_t GetDefaultValue() override { return defaultValue; }
        MenuEntryList GetMenuEntries() override;
        bool IsMenu() override { return !menu.empty(); }
        bool Set(int64_t val) override final;
        virtual bool SetValue(int64_t val) = 0;
        void Update(ControlMenu menu);
        virtual void Update() { };
        void AddDependency(GenericControl *dependency);

    protected:
        uint32_t id;
        std::string name;
        int64_t minValue;
        int64_t maxValue;
        int64_t stepValue;
        int64_t defaultValue;
        ControlMenu menu;
        std::vector<GenericControl*> dependencies;
        int32_t FinalMenuIndex(ControlMenu menu);
};