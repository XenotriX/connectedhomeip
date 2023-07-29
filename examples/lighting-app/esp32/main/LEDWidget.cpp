/*
 *
 *    Copyright (c) 2021-2023 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "LEDWidget.h"
#include "ColorFormat.h"
#if CONFIG_HAVE_DISPLAY
#include "ScreenManager.h"
#endif
#include "led_strip.h"

static const char * TAG = "LEDWidget";

void LEDWidget::Init(void)
{
    mState      = false;
    mBrightness = UINT8_MAX;

    mHue        = 0;
    mSaturation = 0;
    mGPIONum                       = (gpio_num_t) CONFIG_LED_GPIO;
    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE, // timer mode
        .duty_resolution = LEDC_TIMER_8_BIT,    // resolution of PWM duty
        .timer_num       = LEDC_TIMER_1,        // timer index
        .freq_hz         = 5000,                // frequency of PWM signal
        .clk_cfg         = LEDC_AUTO_CLK,       // Auto select the source clock
    };
    ledc_timer_config(&ledc_timer);
    ledc_channel_config_t ledc_channel_red = {
        .gpio_num   = GPIO_NUM_2,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_1,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config_t ledc_channel_green = {
        .gpio_num   = GPIO_NUM_4,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_1,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_1,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config_t ledc_channel_blue = {
        .gpio_num   = GPIO_NUM_5,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_2,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_1,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&ledc_channel_red);
    ledc_channel_config(&ledc_channel_green);
    ledc_channel_config(&ledc_channel_blue);
}

void LEDWidget::Set(bool state)
{
    ESP_LOGI(TAG, "Setting state to %d", state ? 1 : 0);
    if (state == mState)
        return;

    mState = state;

    DoSet();
}

void LEDWidget::Toggle()
{
    ESP_LOGI(TAG, "Toggling state to %d", !mState);
    mState = !mState;

    DoSet();
}

void LEDWidget::SetBrightness(uint8_t brightness)
{
    ESP_LOGI(TAG, "Setting brightness to %d", brightness);
    if (brightness == mBrightness)
        return;

    mBrightness = brightness;

    DoSet();
}

uint8_t LEDWidget::GetLevel()
{
    return this->mBrightness;
}

bool LEDWidget::IsTurnedOn()
{
    return this->mState;
}

void LEDWidget::SetColor(uint8_t Hue, uint8_t Saturation)
{
    if (Hue == mHue && Saturation == mSaturation)
        return;

    mHue        = Hue;
    mSaturation = Saturation;

    ESP_LOGI(TAG, "Setting color to %d, %d", mHue, mSaturation);

    DoSet();
}

void LEDWidget::DoSet(void)
{
    uint8_t brightness = mState ? mBrightness : 0;

    HsvColor_t hsv = { mHue, mSaturation, brightness };
    RgbColor_t rgb = HsvToRgb(hsv);

#if CONFIG_LED_TYPE_RMT
    if (mStrip)
    {
        HsvColor_t hsv = { mHue, mSaturation, brightness };
        RgbColor_t rgb = HsvToRgb(hsv);
        mStrip->set_pixel(mStrip, 0, rgb.r, rgb.g, rgb.b);
        mStrip->refresh(mStrip, 100);
    }
#else
    ESP_LOGI(TAG, "Setting color to %d, %d, %d", rgb.r, rgb.g, rgb.b);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, rgb.r);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, rgb.g);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, rgb.b);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
#endif // CONFIG_LED_TYPE_RMT
#if CONFIG_HAVE_DISPLAY
    if (mVirtualLEDIndex != -1)
    {
        ScreenManager::SetVLED(mVirtualLEDIndex, mState);
    }
#endif // CONFIG_HAVE_DISPLAY
}

#if CONFIG_DEVICE_TYPE_M5STACK
void LEDWidget::SetVLED(int id1)
{
    mVirtualLEDIndex = id1;
    if (mVirtualLEDIndex != -1)
    {
        ScreenManager::SetVLED(mVirtualLEDIndex, mState);
    }
}
#endif
