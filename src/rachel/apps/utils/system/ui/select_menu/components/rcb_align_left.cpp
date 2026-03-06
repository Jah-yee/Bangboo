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

    // Render items（Settings 等使用本 callback；Quit 项用红色突出，便于一眼看到）
    HAL::GetCanvas()->setTextSize(1);
    HAL::GetCanvas()->setTextDatum(textdatum_t::top_left);
    for (int i = 0; i < menuItemList.size(); i++)
    {
        if (menuItemList[i]->tag == "Quit")
            HAL::GetCanvas()->setTextColor(THEME_COLOR_RED, THEME_COLOR_BLACK);
        else
            HAL::GetCanvas()->setTextColor(THEME_COLOR_LIGHT, THEME_COLOR_BLACK);
        HAL::GetCanvas()->drawString(menuItemList[i]->tag.c_str(), menuItemList[i]->x, menuItemList[i]->y - camera.y);
    }

    // Render selector
    HAL::GetCanvas()->setColor(TFT_WHITE);
    HAL::GetCanvas()->fillSmoothRoundRectInDifference(selector.x,
                                                      selector.y - camera.y +
                                                          (menuItemList[selector.targetItem]->height - selector.height) / 2,
                                                      selector.width,
                                                      selector.height,
                                                      6);
}
