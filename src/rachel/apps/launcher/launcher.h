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
            
            // 自动启动：无操作一段时间后自动进入 Bangboo，适合注意力漂移（刷手机等）后回来再看一眼。
            // 流程：last_input_time 被任意按键/摇晃更新 → 无操作超过 (delay - HINT_LEAD) 时先显示「即将进入」→ 到点执行 createAndStartApp。
            bool auto_startup_enabled = true;
            uint32_t auto_startup_delay = 60000;  // 无操作多久后自动进 App（60 秒，避免 10 秒太短误触）
            uint32_t last_input_time = 0;         // 最后一次有操作的时间（按键或摇晃都会更新）
            std::string auto_startup_app_name = "Bangboo";
            bool auto_startup_hint_visible = false;  // 由 _update_menu 根据剩余时间设置，供状态栏渲染「即将进入 xxx」
            static constexpr uint32_t AUTO_STARTUP_HINT_LEAD_MS = 3000;  // 提前几秒在状态栏出提示，让过渡更顺

            // 方向性摇晃当作「左键/右键」用：只在 Launcher 里生效，不碰 HAL，也不影响其他 App。
            // 左晃 = 上一项（等同 SELECT），右晃 = 下一项（等同 RIGHT），一次晃只动一个格子。
            enum ShakeNavState_t { SHAKE_NAV_IDLE, SHAKE_NAV_DETECTING };
            ShakeNavState_t shake_nav_state = SHAKE_NAV_IDLE;
            uint32_t shake_nav_start_time = 0;
            uint32_t shake_nav_cooldown_until = 0;
            float shake_nav_intensity = 0.0f;
            float shake_nav_last_accel_x = 0.0f, shake_nav_last_accel_y = 0.0f, shake_nav_last_accel_z = 0.0f;
            uint32_t shake_nav_last_time = 0;
            float shake_nav_sum_accel_x = 0.0f;  // 检测窗内 accelX 累加，用来区分左晃（负）和右晃（正）
            static constexpr float SHAKE_NAV_THRESHOLD = 2.8f;   // 强度超过这个才算一次晃
            static constexpr uint32_t SHAKE_NAV_DETECT_MS = 140;   // 持续这么久就出结果，稍短一点让反应更跟手，仍能防误触
            static constexpr uint32_t SHAKE_NAV_COOLDOWN_MS = 480;   // 触发后这段时间内不再响应；略短一点方便连续晃两次时更顺
            static constexpr uint32_t SHAKE_NAV_DETECT_TIMEOUT_MS = 1500; // 检测超时，防止卡在 DETECTING
            static constexpr float SHAKE_NAV_DECAY = 0.80f;           // 强度衰减，避免残留
        };
        Data_t _data;
        void _update_clock(bool updateNow = false);
        void _create_menu();
        void _update_menu();
        void _destroy_menu();
        void _play_app_anim(bool open);
        /** 检测本帧是否有「左晃/右晃」可当作一次菜单滑动。返回值：-1 左晃(上一项)，0 无，1 右晃(下一项)。内部有冷却，一次只产出一个方向。 */
        int _get_shake_navigation();

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
