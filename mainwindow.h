#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QKeyEvent>
#include <QResizeEvent>
#include <chrono>
#include <memory>
#include "rasterengine.h"
#include "gameengine.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onGameTick();
    void on_gridBtn_clicked();
    void on_fullscreenBtn_clicked();
    void on_restartBtn_clicked();
    void on_zoomSet_valueChanged(int arg1);
    void on_gasBtn_pressed();
    void on_gasBtn_released();
    void on_brakeBtn_pressed();
    void on_brakeBtn_released();

private:
    void updateCanvasDimensions();
    void renderFrame();

    Ui::MainWindow *ui;
    std::unique_ptr<RasterBuffer> m_rasterBuffer;
    std::unique_ptr<GameEngine> m_gameEngine;

    QTimer *m_gameTimer;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;

    bool m_isGridVisible;
    int m_gridCellSize;
    int m_baseBufferWidth;
    int m_baseBufferHeight;
};

#endif // MAINWINDOW_H
