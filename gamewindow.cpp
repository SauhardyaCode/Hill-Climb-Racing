#include <iostream>
#include "gamewindow.h"
#include "./ui_gamewindow.h"

GameWindow::GameWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GameWindow)
    , isGridVisible(false)
{
    ui->setupUi(this);
}

GameWindow::~GameWindow()
{
    delete ui;
}

void GameWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    QPixmap pixmap;

    // Ensure pixmap matches the real, rendered canvas size
    if (pixmap.isNull() || pixmap.size() != ui->gameCanvas->size()) {
        pixmap = QPixmap(ui->gameCanvas->size());
        pixmap.fill(Qt::white);
        ui->gameCanvas->setPixmap(pixmap);
        canvasWidth = ui->gameCanvas->width();
        canvasHeight = ui->gameCanvas->height();
        updateScreen();
    }
}

void GameWindow::updateScreen(){
    int cellSize = ui->zoomSet->value();
    if (cellSize <= 0 || canvasWidth <= 0 || canvasHeight <= 0) return;

    QPixmap pixmap(canvasWidth, canvasHeight);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);

    // Origin screen pixels
    int centrePixelX = canvasWidth/2;
    int centrePixelY = canvasHeight/2;

    // Cartesian grid bounds on X and Y
    int maxCartesianX = (canvasWidth - centrePixelX) / cellSize + 1;
    int minCartesianX = -centrePixelX / cellSize - 1;
    int maxCartesianY = centrePixelY / cellSize + 1;
    int minCartesianY = -(canvasHeight - centrePixelY) / cellSize - 1;

    // Grid Lines
    if (isGridVisible) {
        ui->gridBtn->setText("Hide Grid");
        QPen gridPen(QColor(220, 220, 220), 1);
        painter.setPen(gridPen);

        // Vertical grid lines
        for (int x = centrePixelX; x < canvasWidth; x += cellSize)
            painter.drawLine(x, 0, x, canvasHeight);
        for (int x = centrePixelX; x >= 0; x -= cellSize)
            painter.drawLine(x, 0, x, canvasHeight);

        // Horizontal grid lines
        for (int y = centrePixelY; y < canvasHeight; y += cellSize)
            painter.drawLine(0, y, canvasWidth, y);
        for (int y = centrePixelY; y >= 0; y -= cellSize)
            painter.drawLine(0, y, canvasWidth, y);

        // X and Y Axes
        QColor axisColor(180, 180, 180);
        for (int x=minCartesianX; x<=maxCartesianX; x++){
            // Cartesian key -> screen pixels
            int pixelLeft = centrePixelX + (x * cellSize);
            int pixelTop = centrePixelY - cellSize;
            painter.fillRect(pixelLeft, pixelTop, cellSize, cellSize, axisColor);
        }
        for (int y=minCartesianY; y<=maxCartesianY; y++){
            // Cartesian key -> screen pixels
            int pixelLeft = centrePixelX;
            int pixelTop = centrePixelY - ((y + 1) * cellSize);
            painter.fillRect(pixelLeft, pixelTop, cellSize, cellSize, axisColor);
        }
    } else{
        ui->gridBtn->setText("Show Grid");
    }

    // Fill Pixels (from pixelMap)
    for (int cartesianX = minCartesianX; cartesianX <= maxCartesianX; ++cartesianX) {
        for (int cartesianY = minCartesianY; cartesianY <= maxCartesianY; ++cartesianY) {
            QPair<int, int> cell(cartesianX, cartesianY);

            if (pixelMap.contains(cell)) {
                // Cartesian key -> screen pixels
                int pixelLeft = centrePixelX + (cartesianX * cellSize);
                int pixelTop = centrePixelY - ((cartesianY + 1) * cellSize);

                painter.fillRect(pixelLeft, pixelTop, cellSize, cellSize, pixelMap[cell]);
            }
        }
    }

    ui->gameCanvas->setPixmap(pixmap);
}
void GameWindow::on_gridBtn_clicked()
{
    isGridVisible = !isGridVisible;
    updateScreen();
}


void GameWindow::on_zoomSet_valueChanged(int arg1)
{
    updateScreen();
}

