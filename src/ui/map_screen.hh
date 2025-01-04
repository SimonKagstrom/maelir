#include "ui.hh"

class UserInterface::MapScreen : public ScreenBase
{
public:
    MapScreen(UserInterface& parent);

    void OnPosition(const GpsData& position) final;

    void Activate() final;

    void Update() final;

private:
    struct RouteLine
    {
        RouteLine(lv_obj_t* parent)
            : lv_line(lv_line_create(parent))
        {
        }

        lv_obj_t* lv_line;
        std::vector<lv_point_precise_t> points;
    };

    struct TileAndPosition
    {
        std::unique_ptr<ITileHandle> tile;
        lv_obj_t* image {nullptr};

        TileAndPosition(std::unique_ptr<ITileHandle> tile, lv_obj_t* image)
            : tile(std::move(tile))
            , image(image)
        {
        }

        ~TileAndPosition()
        {
            lv_obj_delete(image);
        }
    };

    Point PositionToMapCenter(const Point& pixel_position) const;
    void DrawMapTiles(const Point& position);

    void DrawBoat();
    void DrawSpeedometer();
    void DrawRoute();


    UserInterface& m_parent;
    std::unique_ptr<Image> m_boat_data;

    std::unique_ptr<RouteLine> m_route_line;

    // Freed via the screen deleter
    lv_obj_t* m_boat;
    lv_obj_t* m_speedometer_scale;
    lv_obj_t* m_speedometer_arc;

    // Global pixel position of the left corner of the map
    Point m_map_position {0, 0};
    Point m_map_position_zoomed_out {0, 0};

    etl::vector<TileAndPosition, kTileCacheSize> m_tiles;
};
