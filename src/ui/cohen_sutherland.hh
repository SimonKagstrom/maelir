#pragma once

#include "hal/i_display.hh"

namespace cs
{

// Cohen-Sutherland region codes
constexpr int kInside = 0; // 0000
constexpr int kLeft = 1;   // 0001
constexpr int kRight = 2;  // 0010
constexpr int kBottom = 4; // 0100
constexpr int kTop = 8;    // 1000

// Compute the region code for a point (x, y)
int
ComputeRegionCode(int x, int y)
{
    int code = kInside;

    if (x < 0)
    {
        code |= kLeft;
    }
    else if (x > hal::kDisplayWidth)
    {
        code |= kRight;
    }
    if (y < 0)
    {
        code |= kBottom;
    }
    else if (y > hal::kDisplayHeight)
    {
        code |= kTop;
    }

    return code;
}

bool
PointClipsDisplay(int x, int y)
{
    return ComputeRegionCode(x, y) == kInside;
}

// Check if the line between (x1, y1) and (x2, y2) clips the display area
bool
LineClipsDisplay(int x1, int y1, int x2, int y2)
{
    int code1 = ComputeRegionCode(x1, y1);
    int code2 = ComputeRegionCode(x2, y2);

    while (true)
    {
        if ((code1 | code2) == 0)
        {
            // Both endpoints are kInside the display area
            return true;
        }
        else if (code1 & code2)
        {
            // Both endpoints share an outside region, so the line is outside the display area
            return false;
        }
        else
        {
            // Some segment of the line is kInside the display area
            int code_out;
            int x, y;

            if (code1 != 0)
                code_out = code1;
            else
                code_out = code2;

            if (code_out & kTop)
            {
                x = x1 + (x2 - x1) * (hal::kDisplayHeight - y1) / (y2 - y1);
                y = hal::kDisplayHeight;
            }
            else if (code_out & kBottom)
            {
                x = x1 + (x2 - x1) * (0 - y1) / (y2 - y1);
                y = 0;
            }
            else if (code_out & kRight)
            {
                y = y1 + (y2 - y1) * (hal::kDisplayWidth - x1) / (x2 - x1);
                x = hal::kDisplayWidth;
            }
            else if (code_out & kLeft)
            {
                y = y1 + (y2 - y1) * (0 - x1) / (x2 - x1);
                x = 0;
            }

            if (code_out == code1)
            {
                x1 = x;
                y1 = y;
                code1 = ComputeRegionCode(x1, y1);
            }
            else
            {
                x2 = x;
                y2 = y;
                code2 = ComputeRegionCode(x2, y2);
            }
        }
    }
}

} // namespace cs