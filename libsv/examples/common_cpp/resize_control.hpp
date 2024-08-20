#pragma once

#include "sv/sv.h"

namespace common 
{
    enum ResizeValue { Vga = 0, Hd, FullHd, None };

    class ResizeControl : public IControl {

        public:
            ResizeControl();
            ~ResizeControl();
            uint32_t GetID() override;
            const char* GetName() override;
            int64_t Get() override;
            bool Set(int64_t val) override;
            int64_t GetMinValue() override;
            int64_t GetMaxValue() override;
            int64_t GetStepValue() override;
            int64_t GetDefaultValue() override;
            MenuEntryList GetMenuEntries() override;
            bool IsMenu() override;
        
        private:
            int64_t value;
            int64_t min;
            int64_t max;
            int64_t defaultValue;
            MenuEntryList entries;
    };
}