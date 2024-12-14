#pragma once

#include "gps_data.hh"
#include "mapeditor_graphicsview.hh"
#include "router.hh"

#include <QFile>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QMainWindow>
#include <QMouseEvent>
#include <etl/list.h>
#include <unordered_set>
#include <yaml-cpp/yaml.h>

/*
 * From  https://stackoverflow.com/questions/70175131/qt-how-to-implement-a-hash-function-for-qcolor
 *
 * Since we use a std::unordered_set for land colors
 */
template <>
struct std::hash<QColor>
{
    std::size_t operator()(const QColor& c) const noexcept
    {
        return std::hash<unsigned int> {}(c.rgba());
    }
};

template <>
struct std::hash<std::pair<int, int>>
{
    std::size_t operator()(const std::pair<int, int>& c) const noexcept
    {
        return std::hash<int64_t> {}(static_cast<int64_t>(c.first) << 32 | c.second);
    }
};

namespace Ui
{
class MainWindow;
}

class MapEditorMainWindow : public QMainWindow
{
    Q_OBJECT

    friend class MapEditorGraphicsView;

public:
    explicit MapEditorMainWindow(const QString& map_name,
                                 const QString& out_yaml,
                                 QWidget* parent = nullptr);
    ~MapEditorMainWindow() final;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:

private:
    bool FilterMouse(QObject* obj, QEvent* event);
    std::pair<int, int> GetMapCoordinates(QPoint pos);
    void RightClickContextMenu(QPoint mouse_position, QPoint map_posititon);
    void SetGpsPosition(double longitude, double latitude, int x, int y);
    void CalculateLand();
    void UpdateRoutingInformation();
    void AddExtraLand(int x, int y);
    void AddExtraWater(int x, int y);
    void AddSkipTile(int x, int y);
    unsigned CountLandPixels(QImage& image);

    void LoadYaml(const char* filename);
    void SaveYaml();

    Ui::MainWindow* m_ui {nullptr};

    std::unique_ptr<QGraphicsScene> m_scene;
    std::unique_ptr<QImage> m_map;
    const QString m_map_name;
    const QString m_out_yaml;
    QGraphicsPixmapItem* m_pixmap;

    uint32_t m_gps_map_width {0};
    uint32_t m_gps_map_height {0};

    std::optional<Point> m_home_position;

    // Colors for land (for route planning)
    std::unordered_set<QColor> m_land_colors;
    // "Extra land", marked manually
    std::unordered_set<std::pair<int, int>> m_extra_land;
    // "Extra water", marked manually
    std::unordered_set<std::pair<int, int>> m_extra_water;
    // "Skip tiles", marked manually
    std::unordered_set<std::pair<int, int>> m_skip_tiles;

    etl::list<Point, 2> m_wanted_route;

    bool m_panning {false};
    QPoint m_last_mouse_pos {0, 0};

    std::vector<bool> m_land_mask;
    std::vector<bool> m_all_land_tiles;
    std::vector<uint32_t> m_land_mask_uint32;

    std::vector<MapGpsRasterTile> m_gps_positions;

    std::unique_ptr<Router<kTargetCacheSize>> m_router;

    std::span<const IndexType> m_current_route;
};
