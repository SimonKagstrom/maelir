#pragma once

#include "timer_manager.hh"
#include "ui.hh"

class UserInterface::MapScreen : public ScreenBase
{
public:
    MapScreen(UserInterface& parent);

    void OnPosition(const GpsData& position) final;

    void OnInput(hal::IInput::Event event) final;

    void Update() final;

private:
    enum class State
    {
        kMap,
        kInitialOverviewMap,
        kFillOverviewMapTiles,
        kOverviewMap,
        kSelectDestination,
        kDestinationSelected,
        kAdjustGps,
        kGpsAdjusted,

        kValueCount,
    };

    enum class Mode
    {
        kMap,
        kZoom2,
        kZoom4,

        kValueCount,
    };


    // A route segment in display coordinates, clipped to the display
    struct RouteSegment
    {
        Point from;
        Point to;
        bool passed;
    };

    Point PositionToMapCenter(const Point& pixel_position) const;
    void DrawMapTiles(uint16_t* dst, const Point& position);

    // Called from LV_EVENT_DRAW_MAIN to paint directly into the LVGL render buffer
    void DrawBackground(uint16_t* dst);

    void DrawBoat();
    void DrawSpeedometer();
    void DrawRoute();
    void DrawDestinationCrosshair();

    bool DrawsMapDirectly() const;
    Point ToDisplay(const Point& point) const;

    void PrepareInitialZoomedOutMap();
    void FillZoomedOutMap();
    void DrawZoomedTile(const Point& position);

    void RunStateMachine();

    void OnInputSelectDestination(hal::IInput::Event event);
    void OnInputAdjustGps(hal::IInput::Event event);
    void OnInputViewMap(hal::IInput::Event event);

    std::unique_ptr<Image> m_boat_data;
    std::unique_ptr<Image> m_crosshair_data;

    std::vector<RouteSegment> m_route_segments;
    bool m_show_route {true};

    // Freed via the screen deleter
    lv_obj_t* m_boat;
    lv_obj_t* m_speedometer_scale;
    lv_obj_t* m_speedometer_arc;
    lv_obj_t* m_crosshair;

    lv_obj_t* m_indicators;
    lv_obj_t* m_indicators_shadow;

    lv_obj_t* m_trip_computer_box;
    lv_obj_t* m_trip_computer_label;

    // Global pixel position of the left corner of the map
    Point m_map_position {0, 0};
    Point m_map_position_zoomed_out {0, 0};
    State m_state {State::kMap};
    int32_t m_zoom_level {1};

    // Enough space for all tiles at zoom level 4, offset
    etl::vector<Point, (hal::kDisplayWidth / kTileSize) * 4 * (hal::kDisplayHeight / kTileSize) * 4>
        m_zoomed_out_map_tiles;

    // The zoomed out maps are filled in over several frames, so they need a separate buffer
    std::unique_ptr<uint16_t[]> m_static_map_buffer;

    Mode m_mode {Mode::kMap};

    os::TimerHandle m_select_timer;

    // Position selection data
    Point m_crosshair_position {0, 0};
};
