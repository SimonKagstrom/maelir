#include "mainwindow.hh"

#include "i_display.hh"
#include "ui_mainwindow.h"

namespace
{

struct ImageImpl : public Image
{
    std::vector<uint16_t> rgb565_data;
};

#include <png.h>
std::shared_ptr<ImageImpl> Vobb(const char *fn)
{
    FILE* fp = fopen(fn, "r");

    png_structp pngptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop pnginfo = png_create_info_struct(pngptr);

    png_init_io(pngptr, fp);
    png_set_palette_to_rgb(pngptr);


    png_bytepp rows;
    png_read_png(pngptr, pnginfo, PNG_TRANSFORM_IDENTITY, NULL);
    rows = png_get_rows(pngptr, pnginfo);

    auto w = png_get_image_width(pngptr, pnginfo);
    auto h = png_get_image_height(pngptr, pnginfo);

    auto img = std::make_shared<ImageImpl>();
    img->rgb565_data.resize(w * h);
    auto p = img->rgb565_data.data();

    for (auto i = 0; i < h; i++)
    {
        for (auto j = 0; j < h * 3; j += 3)
        {
            auto r = rows[i][j];
            auto g = rows[i][j + 1];
            auto b = rows[i][j + 2];

            auto rgb565 = __builtin_bswap16((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));

            *p = rgb565;
            p++;
        }
    }

    img->height = h;
    img->width = w;
    img->data = img->rgb565_data;

    return img;
}

std::shared_ptr<ImageImpl> tile0;
std::shared_ptr<ImageImpl> tile1;

}


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

    m_scene = std::make_unique<QGraphicsScene>();
    m_display = std::make_unique<DisplayQt>(m_scene.get());
    m_ui->displayGraphicsView->setScene(m_scene.get());


    auto tile0 = Vobb("/Users/ska/projects/maelir/tmp/tile0000.png");
    auto tile1 = Vobb("/Users/ska/projects/maelir/tmp/tile0001.png");

    m_display->Blit(*tile0, {0, 0, tile0->width, tile0->height}, {0, 0});
    m_display->Blit(*tile1, {0, 0, tile1->width, tile1->height}, {240, 0});
    m_display->Flip();
}

MainWindow::~MainWindow()
{
    delete m_ui;
}
