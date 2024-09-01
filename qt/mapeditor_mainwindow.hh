#pragma once

#include "mapeditor_graphicsview.hh"

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
namespace Ui
{
class MainWindow;
}

class MapEditorMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MapEditorMainWindow(std::unique_ptr<QImage> map,
                                 const QString& map_name,
                                 const QString& out_yaml,
                                 QWidget* parent = nullptr);
    ~MapEditorMainWindow() final;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:

private:
    struct GpsToPixel
    {
        double longitude;
        double latitude;
        int x;
        int y;
    };

    struct MapPositionData
    {
        double longitude_at_pixel_0;
        double latitude_at_pixel_0;
        double longitude_per_pixel;
        double latitude_per_pixel;

        unsigned longitude_pixel_size;
        unsigned latitude_pixel_size;
    };

    bool FilterMouse(QObject* obj, QEvent* event);
    std::pair<int, int> GetMapCoordinates(QPoint pos);
    void RightClickContextMenu(QPoint mouse_position, QPoint map_posititon);
    void SetGpsPosition(double longitude, double latitude, int x, int y);
    void LoadYaml(const char* filename);
    void SaveYaml();

    Ui::MainWindow* m_ui {nullptr};

    std::unique_ptr<QGraphicsScene> m_scene;
    std::unique_ptr<QImage> m_map;
    QString m_map_name;
    QString m_out_yaml;
    QGraphicsPixmapItem* m_pixmap;

    etl::list<GpsToPixel, 2> m_positions;
    std::optional<MapPositionData> m_map_position_data;

    // Colors for land (for route planning)
    std::unordered_set<QColor> m_land_colors;

    bool m_panning {false};
    QPoint m_last_mouse_pos {0, 0};
};
