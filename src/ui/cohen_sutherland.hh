#pragma once

#include "hal/i_display.hh"

namespace cs
{

// Cohen-Sutherland region codes
constexpr int kInside = 0b0000;
constexpr int kLeft = 0b0001;
constexpr int kRight = 0b0010;
constexpr int kBottom = 0b0100;
constexpr int kTop = 0b1000;

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

// Check if the line between (x0, y0) and (x1, y1) clips the display area
bool
LineClipsDisplay(int x0, int y0, int x1, int y1)
{
    int code0 = ComputeRegionCode(x0, y0);
    int code1 = ComputeRegionCode(x1, y1);
    bool accept = false;

    while (true)
    {
        if ((code0 | code1) == 0)
        {
            // Both endpoints are kInside the display area
            accept = true;
            break;
        }
        else if (code0 & code1)
        {
            // Both endpoints share an outside region, so the line is outside the display area
            break;
        }
        else
        {
            // Some segment of the line is kInside the display area
            int code_out;
            int x, y;

            code_out = code1 > code0 ? code1 : code0;

            if (code_out & kTop)
            {
                x = x0 + (x1 - x0) * (hal::kDisplayHeight - y0) / (y1 - y0);
                y = hal::kDisplayHeight;
            }
            else if (code_out & kBottom)
            {
                x = x0 + (x1 - x0) * (0 - y0) / (y1 - y0);
                y = 0;
            }
            else if (code_out & kRight)
            {
                y = y0 + (y1 - y0) * (hal::kDisplayWidth - x0) / (x1 - x0);
                x = hal::kDisplayWidth;
            }
            else if (code_out & kLeft)
            {
                y = y0 + (y1 - y0) * (0 - x0) / (x1 - x0);
                x = 0;
            }

            if (code_out == code0)
            {
                x0 = x;
                y0 = y;
                code0 = ComputeRegionCode(x0, y0);
            }
            else
            {
                x1 = x;
                y1 = y;
                code1 = ComputeRegionCode(x1, y1);
            }
        }
    }

    return accept;
}

} // namespace cs
