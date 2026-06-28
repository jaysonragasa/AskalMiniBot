#pragma once
#include "Interfaces.h"
#include <Preferences.h>

class PreferencesConfig : public IConfigRepository {
public:
    PreferencesConfig();
    ~PreferencesConfig() override = default;

    void begin() override;
    ServoConfig getServoConfig(int index) override;
    void setServoConfig(int index, const ServoConfig& config) override;

private:
    void loadDefaults();
    
    Preferences preferences;
    ServoConfig servoConfigs[4];
};
