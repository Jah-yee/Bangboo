/**
 * @file menu.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-05
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "../launcher.h"
#include "lgfx/v1/lgfx_fonts.hpp"
#include "menu_render_callback.hpp"
#include "spdlog/spdlog.h"

using namespace MOONCAKE::APPS;

void Launcher::_create_menu()
{
    spdlog::info("create menu");

    // Create menu and render callback
    _data.menu = new SMOOTH_MENU::Simple_Menu(HAL::GetCanvas()->width(), HAL::GetCanvas()->height());
    _data.menu_render_cb = new LauncherRenderCallBack;

    // Setup
    _data.menu->setRenderCallback(_data.menu_render_cb);
    _data.menu->setFirstItem(1);

    // Set selector anim, in this launcher case, is the icon's moving anim (fixed selector)
    auto cfg_selector = _data.menu->getSelector()->config();
    cfg_selector.animPath_x = LVGL::ease_out;
    cfg_selector.animTime_x = 300;
    _data.menu->getSelector()->config(cfg_selector);

    // Set menu open anim
    auto cfg_menu = _data.menu->getMenu()->config();
    cfg_menu.animPath_open = LVGL::ease_out;
    cfg_menu.animTime_open = 1000;
    _data.menu->getMenu()->config(cfg_menu);

    // Record when menu open animation ends to block START during opening
    _data.menu_open_end_time = HAL::Millis() + cfg_menu.animTime_open;

    // Allow selector go loop
    _data.menu->setMenuLoopMode(true);

    // Get installed app list
    spdlog::info("installed apps num: {}", mcAppGetFramework()->getAppRegister().getInstalledAppNum());
    int i = 0;
    for (const auto& app : mcAppGetFramework()->getAppRegister().getInstalledAppList())
    {
        // Pass the launcher
        if (app->getAddr() == getAppPacker())
            continue;

        // spdlog::info("app: {} icon: {}", app->getAppName(), app->getAppIcon());
        spdlog::info("push app: {} into menu", app->getAppName());

        // Push items into menu, use icon pointer as the item user data
        _data.menu->getMenu()->addItem(app->getAppName(),
                                       THEME_APP_ICON_GAP + i * (THEME_APP_ICON_WIDTH + THEME_APP_ICON_GAP),
                                       THEME_APP_ICON_MARGIN_TOP,
                                       THEME_APP_ICON_WIDTH,
                                       THEME_APP_ICON_HEIGHT,
                                       app->getAppIcon());
        i++;
    }

    // 状态栏用同一套数据：时钟字符串 + 是否显示「即将进入 xxx」。callback 里只读这两个指针，本帧先改上面逻辑再 update，所以能完整交付。
    ((LauncherRenderCallBack*)_data.menu_render_cb)->setClock(&_data.clock);
    ((LauncherRenderCallBack*)_data.menu_render_cb)->setAutoStartHintVisible(&_data.auto_startup_hint_visible);

    // Setup anims
    ((LauncherRenderCallBack*)_data.menu_render_cb)->statusBarAnim.setAnim(LVGL::ease_out, 0, 24, 600);
    ((LauncherRenderCallBack*)_data.menu_render_cb)->statusBarAnim.resetTime(HAL::Millis() + 1000);

    ((LauncherRenderCallBack*)_data.menu_render_cb)->bottomPanelAnim.resetTime(HAL::Millis());
    ((LauncherRenderCallBack*)_data.menu_render_cb)->bottomPanelAnim.setAnim(LVGL::ease_out, 240, 210, 600);
}

void Launcher::_update_menu()
{
    
    if ((HAL::Millis() - _data.menu_update_count) > _data.menu_update_interval)
    {
        // 检查按钮输入
        bool any_button_pressed = false;

        // Update navigation - 适应新的三键配置
        // SELECT 键向前导航
        if (HAL::GetButton(GAMEPAD::BTN_SELECT))
        {
            any_button_pressed = true;
            if (!_data.menu_wait_button_released)
            {
                HAL::PlayWavFile("/system_audio/Klick.wav");
                _data.menu->goLast();
                _data.menu_wait_button_released = true;
            }
        }

        // RIGHT 键向后导航
        else if (HAL::GetButton(GAMEPAD::BTN_RIGHT))
        {
            any_button_pressed = true;
            if (!_data.menu_wait_button_released)
            {
                HAL::PlayWavFile("/system_audio/Klick.wav");
                _data.menu->goNext();
                _data.menu_wait_button_released = true;
            }
        }

        // START 键打开应用
        else if (HAL::GetButton(GAMEPAD::BTN_START) && HAL::Millis() >= _data.menu_open_end_time)
        {
            any_button_pressed = true;
            auto selected_item = _data.menu->getSelector()->getTargetItem();
            // spdlog::info("select: {} try create", selected_item);

            // Skip launcher
            selected_item++;
            // Get packer, apps are arranged by install order, so simply use index is ok
            auto app_packer = mcAppGetFramework()->getInstalledAppList()[selected_item];
            // spdlog::info("try create app: {}", app_packer->getAppName());
            // Try create and start app
            if (mcAppGetFramework()->createAndStartApp(app_packer))
            {
                HAL::PlayWavFile("/system_audio/Enter.wav");
                spdlog::info("app: {} opened", app_packer->getAppName());
                closeApp();
            }
            else
                spdlog::error("open app: {} failed", app_packer->getAppName());
        }

        // Unlock if no button is pressing
        else
        {
            _data.menu_wait_button_released = false;
        }

        // 有按键/摇晃就刷新「最后操作时间」，并关掉「即将进入」提示，避免误以为马上要跳转
        if (any_button_pressed)
        {
            _data.last_input_time = HAL::Millis();
            _data.auto_startup_hint_visible = false;
        }

        // 自动启动逻辑：只依赖 idle 时长，不碰其它模块。hint 标志给本帧后面的 menu->update() 里渲染用。
        uint32_t idle_ms = HAL::Millis() - _data.last_input_time;
        const uint32_t hint_threshold = _data.auto_startup_delay - Data_t::AUTO_STARTUP_HINT_LEAD_MS;

        if (_data.auto_startup_enabled && idle_ms > _data.auto_startup_delay)
        {
            // 到点：关掉提示，执行进入 App，成功后 closeApp 返回
            _data.auto_startup_hint_visible = false;
            auto app_list = mcAppGetFramework()->getInstalledAppList();
            for (size_t i = 1; i < app_list.size(); i++)
            {
                if (app_list[i]->getAppName() == _data.auto_startup_app_name)
                {
                    if (mcAppGetFramework()->createAndStartApp(app_list[i]))
                    {
                        spdlog::info("app: {} auto opened after {}s of inactivity", app_list[i]->getAppName(), _data.auto_startup_delay / 1000);
                        closeApp();
                        return;
                    }
                    break;
                }
            }
            _data.last_input_time = HAL::Millis();
        }
        else if (_data.auto_startup_enabled && idle_ms >= hint_threshold)
        {
            // 进入「提示窗口」：还剩几秒就自动进，状态栏本帧起会显示「即将进入 Bangboo」
            _data.auto_startup_hint_visible = true;
        }
        else
        {
            _data.auto_startup_hint_visible = false;
        }

        // 下面 update 会调 renderCallback，里面按 auto_startup_hint_visible 决定状态栏显示时钟还是「即将进入 Bangboo」
        _data.menu->update(HAL::Millis());

        // Push frame buffer
        // HAL::RenderFpsPanel();
        HAL::CanvasUpdate();

        _data.menu_update_count = HAL::Millis();
    }
}

void Launcher::_destroy_menu()
{
    spdlog::info("destroy menu");
    delete _data.menu;
    delete _data.menu_render_cb;
}
