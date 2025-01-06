#pragma once

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

        kValueCount,
    };

    enum class Mode
    {
        kMap,
        kZoom2,
        kZoom4,

        kValueCount,
    };


    struct RouteLine
    {
        RouteLine(lv_obj_t* parent)
            : lv_line(lv_line_create(parent))
        {
        }

        lv_obj_t* lv_line;
        std::vector<lv_point_precise_t> points;
    };

    Point PositionToMapCenter(const Point& pixel_position) const;
    void DrawMapTiles(const Point& position);

    void DrawBoat();
    void DrawSpeedometer();
    void DrawRoute();

    void PrepareInitialZoomedOutMap();
    void FillZoomedOutMap();
    void DrawZoomedTile(const Point& position);

    void RunStateMachine();

    UserInterface& m_parent;
    std::unique_ptr<Image> m_boat_data;

    std::unique_ptr<RouteLine> m_route_line;

    // Freed via the screen deleter
    lv_obj_t* m_background;
    lv_obj_t* m_boat;
    lv_obj_t* m_speedometer_scale;
    lv_obj_t* m_speedometer_arc;

    // Global pixel position of the left corner of the map
    Point m_map_position {0, 0};
    Point m_map_position_zoomed_out {0, 0};
    State m_state {State::kMap};

    etl::vector<Point, ((hal::kDisplayWidth * hal::kDisplayHeight) / kTileSize) * 4>
        m_zoomed_out_map_tiles;

    std::unique_ptr<uint8_t[]> m_static_map_buffer;
    std::unique_ptr<Image> m_static_map_image;

    Mode m_mode {Mode::kMap};
};
