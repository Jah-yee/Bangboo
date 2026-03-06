/**
 * @file launcher.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include <cstdint>
#include <mooncake.h>
#include <string>
#include "../utils/smooth_menu/simple_menu/simple_menu.h"

namespace MOONCAKE::APPS
{
    /**
     * @brief Launcher
     *
     */
    class Launcher : public APP_BASE
    {
    private:
        struct Data_t
        {
            // Clock
            std::string clock;
            uint32_t clock_update_count = 0;
            const uint32_t clock_update_interval = 1000;
            char string_buffer[10];

            // Menu
            SMOOTH_MENU::Simple_Menu* menu = nullptr;
            SMOOTH_MENU::SimpleMenuCallback_t* menu_render_cb = nullptr;
            uint32_t menu_update_count = 0;
            const uint32_t menu_update_interval = 10;
            // const uint32_t menu_update_interval = 0;
            bool menu_wait_button_released = false;
            // Block START until menu opening animation finishes
            uint32_t menu_open_end_time = 0;

            // App open and close anim
            LVGL::Anim_Path app_anim;
            
            // Auto startup
            bool auto_startup_enabled = true;
            uint32_t auto_startup_delay = 10000;  // 10秒无操作后自动启动
            uint32_t last_input_time = 0;  // 最后一次按钮操作时间
            std::string auto_startup_app_name = "Bangboo";  // 要自动启动的app名称

            // 长按快速滚动：左/右键按住超过一定时间后，按名单顺序快速切换 App，松手即停。
            // 单击仍为「按一下切一个」；长按先等 LONG_PRESS_MS 再以 LONG_PRESS_REPEAT_MS 间隔连续切。
            uint32_t long_press_select_hold_start = 0;   // SELECT 按下时刻，0 表示当前未按住
            uint32_t long_press_select_last_repeat = 0;  // 上次触发「滚动一步」的时刻（用于节流）
            uint32_t long_press_right_hold_start = 0;    // RIGHT 同上
            uint32_t long_press_right_last_repeat = 0;
            static constexpr uint32_t LONG_PRESS_MS = 550;        // 按住超过此时长才进入快速滚动，避免与单击误触
            static constexpr uint32_t LONG_PRESS_REPEAT_MS = 130; // 快速滚动时每隔多久切一个，类似秒表哒哒哒的节奏
        };
        Data_t _data;
        void _update_clock(bool updateNow = false);
        void _create_menu();
        void _update_menu();
        void _destroy_menu();
        void _play_app_anim(bool open);

    public:
        void onCreate() override;
        void onResume() override;
        void onRunning() override;
        void onRunningBG() override;
        void onPause() override;
        void onDestroy() override;
    };

    class Launcher_Packer : public APP_PACKER_BASE
    {
        std::string getAppName() override { return "Launcher"; }
        void* newApp() override { return new Launcher; }
        void deleteApp(void* app) override { delete (Launcher*)app; }
    };
} // namespace MOONCAKE::APPS
