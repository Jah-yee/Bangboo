/**
 * @file rcb_align_left.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-10
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "render_callbacks.h"
#include "../../../../../assets/theme/theme.h"
#include "../../../../../../hal/hal.h"

using namespace SYSTEM::UI;

void SelectMenuRenderCb_AlignLeft::renderCallback(const std::vector<SMOOTH_MENU::Item_t*>& menuItemList,
                                                  const SMOOTH_MENU::RenderAttribute_t& selector,
                                                  const SMOOTH_MENU::RenderAttribute_t& camera)
{
    // Clear
    HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);

    // Render items（统一浅色；Quit 项在未选中时加白色描边框，选中时仅用下方 selector 高亮）
    HAL::GetCanvas()->setTextColor(THEME_COLOR_LIGHT, THEME_COLOR_BLACK);
    HAL::GetCanvas()->setTextSize(1);
    HAL::GetCanvas()->setTextDatum(textdatum_t::top_left);
    int quit_index = -1;
    for (int i = 0; i < menuItemList.size(); i++)
    {
        if (menuItemList[i]->tag == "Quit")
            quit_index = i;
        HAL::GetCanvas()->drawString(menuItemList[i]->tag.c_str(), menuItemList[i]->x, menuItemList[i]->y - camera.y);
    }

    // Quit 未选中时：在 Quit 项外画一层白色圆角描边，便于一眼看到退出入口
    const int pad = 2;
    if (quit_index >= 0 && selector.targetItem != quit_index)
    {
        const auto* q = menuItemList[quit_index];
        int rx = q->x - pad;
        int ry = q->y - camera.y - pad;
        int rw = q->width + pad * 2;
        int rh = q->height + pad * 2;
        HAL::GetCanvas()->setColor(TFT_WHITE);
        HAL::GetCanvas()->drawRoundRect(rx, ry, rw, rh, 4);
    }

    // Render selector（选中项的白底高亮，Quit 被选中时也走这里）
    HAL::GetCanvas()->setColor(TFT_WHITE);
    HAL::GetCanvas()->fillSmoothRoundRectInDifference(selector.x,
                                                      selector.y - camera.y +
                                                          (menuItemList[selector.targetItem]->height - selector.height) / 2,
                                                      selector.width,
                                                      selector.height,
                                                      6);
}
