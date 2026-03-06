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
#include "../../../hal/hal.h"
#include <cmath>

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

    // Setup some widget shit
    // Pass clock string pointer for redner
    ((LauncherRenderCallBack*)_data.menu_render_cb)->setClock(&_data.clock);

    // Setup anims
    ((LauncherRenderCallBack*)_data.menu_render_cb)->statusBarAnim.setAnim(LVGL::ease_out, 0, 24, 600);
    ((LauncherRenderCallBack*)_data.menu_render_cb)->statusBarAnim.resetTime(HAL::Millis() + 1000);

    ((LauncherRenderCallBack*)_data.menu_render_cb)->bottomPanelAnim.resetTime(HAL::Millis());
    ((LauncherRenderCallBack*)_data.menu_render_cb)->bottomPanelAnim.setAnim(LVGL::ease_out, 240, 210, 600);
}

// 方向性摇晃检测：用 IMU 的加速度变化判断「左晃 / 右晃」，只在 Launcher 里当菜单导航用，
// 不改 HAL，也不影响 Bangboo 等其它 App 的摇晃逻辑。一次有效晃只返回一次 -1 或 1，靠冷却保证只动一格。
int Launcher::_get_shake_navigation()
{
    HAL::UpdateImuData();
    const IMU::ImuData_t& imu = HAL::GetImuData();
    uint32_t now = HAL::Millis();

    // 用相邻两帧加速度差的模长当作「晃的强度」，带衰减，避免偶尔抖动一直挂着
    if (_data.shake_nav_last_time > 0)
    {
        float dx = imu.accelX - _data.shake_nav_last_accel_x;
        float dy = imu.accelY - _data.shake_nav_last_accel_y;
        float dz = imu.accelZ - _data.shake_nav_last_accel_z;
        float delta = std::sqrt(dx * dx + dy * dy + dz * dz);
        _data.shake_nav_intensity = _data.shake_nav_intensity * _data.SHAKE_NAV_DECAY + delta;
    }
    _data.shake_nav_last_accel_x = imu.accelX;
    _data.shake_nav_last_accel_y = imu.accelY;
    _data.shake_nav_last_accel_z = imu.accelZ;
    _data.shake_nav_last_time = now;

    // 冷却期内一律不响应，避免一次晃被当成多次
    if (now < _data.shake_nav_cooldown_until)
    {
        if (_data.shake_nav_intensity < _data.SHAKE_NAV_THRESHOLD * 0.1f)
            _data.shake_nav_intensity = 0.0f;
        return 0;
    }

    switch (_data.shake_nav_state)
    {
    case Data_t::SHAKE_NAV_IDLE:
        if (_data.shake_nav_intensity > _data.SHAKE_NAV_THRESHOLD)
        {
            _data.shake_nav_state = Data_t::SHAKE_NAV_DETECTING;
            _data.shake_nav_start_time = now;
            _data.shake_nav_sum_accel_x = 0.0f;
        }
        break;

    case Data_t::SHAKE_NAV_DETECTING:
        _data.shake_nav_sum_accel_x += imu.accelX;
        if ((now - _data.shake_nav_start_time) >= _data.SHAKE_NAV_DETECT_MS)
        {
            _data.shake_nav_state = Data_t::SHAKE_NAV_IDLE;
            _data.shake_nav_cooldown_until = now + _data.SHAKE_NAV_COOLDOWN_MS;
            _data.shake_nav_intensity = 0.0f;
            return _data.shake_nav_sum_accel_x < 0 ? -1 : 1;
        }
        // 检测超时或强度掉下去都回到空闲，避免卡死（例如无 IMU 或数据异常）
        if ((now - _data.shake_nav_start_time) >= _data.SHAKE_NAV_DETECT_TIMEOUT_MS ||
            _data.shake_nav_intensity < _data.SHAKE_NAV_THRESHOLD * 0.3f)
        {
            _data.shake_nav_state = Data_t::SHAKE_NAV_IDLE;
            _data.shake_nav_intensity = 0.0f;
        }
        break;
    }
    return 0;
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

        // 没有任何按键按下时才走这里：先放开「等按键松开」的锁，再看有没有左晃/右晃。
        // 与上面 SELECT/RIGHT 分支用同一套指令：同一 _data.menu、同一 goLast()/goNext()、同一 Klick.wav；摇一次只动一格由 _get_shake_navigation 内部冷却保证。
        else
        {
            _data.menu_wait_button_released = false;
            int shake = _get_shake_navigation();
            if (shake == -1)
            {
                any_button_pressed = true;
                HAL::PlayWavFile("/system_audio/Klick.wav");
                _data.menu->goLast();  // 与 BTN_SELECT 分支一致
            }
            else if (shake == 1)
            {
                any_button_pressed = true;
                HAL::PlayWavFile("/system_audio/Klick.wav");
                _data.menu->goNext();  // 与 BTN_RIGHT 分支一致
            }
        }

        // 更新最后输入时间
        if (any_button_pressed)
        {
            _data.last_input_time = HAL::Millis();
        }

        // 检查自动启动条件
        if (_data.auto_startup_enabled && 
            (HAL::Millis() - _data.last_input_time) > _data.auto_startup_delay)
        {
            // 查找 Bangboo 应用
            auto app_list = mcAppGetFramework()->getInstalledAppList();
            for (size_t i = 1; i < app_list.size(); i++) { // 跳过 launcher (index 0)
                if (app_list[i]->getAppName() == _data.auto_startup_app_name) {
                    if (mcAppGetFramework()->createAndStartApp(app_list[i])) {
                        spdlog::info("app: {} auto opened after 10s of inactivity", app_list[i]->getAppName());
                        closeApp();
                        return; // 启动成功后直接返回
                    }
                    break;
                }
            }
            
            // 如果没找到Bangboo应用，重置计时器以避免一直尝试
            _data.last_input_time = HAL::Millis();
        }

        // Update menu
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
