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
            
            // Auto startup：无操作一段时间后自动进入 Bangboo，适合注意力漂移（刷手机等）后回来再看一眼。
            // 流程：last_input_time 被任意按键/摇晃更新 → 无操作超过 (delay - HINT_LEAD) 时先显示「即将进入」→ 到点执行 createAndStartApp。
            bool auto_startup_enabled = true;
            uint32_t auto_startup_delay = 10000;  // 无操作 10 秒后自动进 App
            uint32_t last_input_time = 0;         // 最后一次有操作的时间（按键或摇晃都会更新）
            std::string auto_startup_app_name = "Bangboo";
            bool auto_startup_hint_visible = false;  // 由 _update_menu 根据剩余时间设置，供状态栏渲染「即将进入 xxx」
            static constexpr uint32_t AUTO_STARTUP_HINT_LEAD_MS = 3000;  // 提前几秒在状态栏出提示，让过渡更顺
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
