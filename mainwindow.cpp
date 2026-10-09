#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QPixmap>
#include <QPainter>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_gameTimer(new QTimer(this))
    , m_isGridVisible(false)
    , m_gridCellSize(2)
    , m_baseBufferWidth(640)
    , m_baseBufferHeight(360)
{
    ui->setupUi(this);
    setFocusPolicy(Qt::StrongFocus);

    m_rasterBuffer = std::make_unique<RasterBuffer>(m_baseBufferWidth, m_baseBufferHeight);
    m_gameEngine = std::make_unique<GameEngine>(m_baseBufferWidth, m_baseBufferHeight);

    // Setup 60 FPS precise game loop timer
    m_gameTimer->setTimerType(Qt::PreciseTimer);
    connect(m_gameTimer, &QTimer::timeout, this, &MainWindow::onGameTick);
    m_lastFrameTime = std::chrono::high_resolution_clock::now();
    m_gameTimer->start(16); // ~60 FPS

    // Gas & Brake button interactions for mouse/touch
    connect(ui->gasBtn, &QPushButton::pressed, this, &MainWindow::on_gasBtn_pressed);
    connect(ui->gasBtn, &QPushButton::released, this, &MainWindow::on_gasBtn_released);
    connect(ui->brakeBtn, &QPushButton::pressed, this, &MainWindow::on_brakeBtn_pressed);
    connect(ui->brakeBtn, &QPushButton::released, this, &MainWindow::on_brakeBtn_released);

    ui->zoomSet->setValue(m_gridCellSize);
}

MainWindow::~MainWindow()
{
    m_gameTimer->stop();
    delete ui;
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    updateCanvasDimensions();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateCanvasDimensions();
}

void MainWindow::updateCanvasDimensions()
{
    int cWidth = ui->canvas->width();
    int cHeight = ui->canvas->height();

    if (cWidth < 100 || cHeight < 100) return;

    // Calculate grid buffer resolution based on cell size / zoom
    int bufW = std::max(320, cWidth / m_gridCellSize);
    int bufH = std::max(180, cHeight / m_gridCellSize);

    if (bufW != m_baseBufferWidth || bufH != m_baseBufferHeight) {
        m_baseBufferWidth = bufW;
        m_baseBufferHeight = bufH;
        m_rasterBuffer->resize(bufW, bufH);
        m_gameEngine->resize(bufW, bufH);
    }
}

void MainWindow::onGameTick()
{
    auto now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
    m_lastFrameTime = now;

    // Clamp dt to avoid physics blowups on lag spikes or window moves
    dt = std::clamp(dt, 0.001f, 0.04f);

    m_gameEngine->update(dt);
    renderFrame();

    // Update status text on bottom bar
    if (m_gameEngine->isGameOver()) {
        ui->gameStatusLabel->setText(QString("GAME OVER: %1 | Press [R] to Retry")
                                     .arg(QString::fromStdString(m_gameEngine->getGameOverReason())));
        ui->gameStatusLabel->setStyleSheet("color: #ff5252; font-size: 14px; font-weight: bold;");
    } else if (m_gameEngine->isPaused()) {
        ui->gameStatusLabel->setText("GAME PAUSED | Press [Space] to Resume");
        ui->gameStatusLabel->setStyleSheet("color: #ffd54f; font-size: 14px; font-weight: bold;");
    } else {
        ui->gameStatusLabel->setText(QString("Distance: %1m | Coins: %2 | Fuel: %3% | Speed: %4 KM/H")
                                     .arg(m_gameEngine->getDistance())
                                     .arg(m_gameEngine->getCoins())
                                     .arg(static_cast<int>(m_gameEngine->getFuel()))
                                     .arg(static_cast<int>(m_gameEngine->getSpeedKmh())));
        ui->gameStatusLabel->setStyleSheet("color: #64b5f6; font-size: 14px; font-weight: bold;");
    }
}

void MainWindow::renderFrame()
{
    m_gameEngine->render(*m_rasterBuffer);

    // Convert raster buffer to QImage
    // When grid mode is enabled, it renders authentic cell grid lines!
    QImage frameImg = m_rasterBuffer->toQImage(m_isGridVisible, m_gridCellSize, 0xFF1E2630);

    QPixmap pix;
    if (m_isGridVisible) {
        pix = QPixmap::fromImage(frameImg);
    } else {
        // Nearest-neighbor integer scaling for razor-sharp pixel raster graphics
        QImage scaledImg = frameImg.scaled(ui->canvas->size(), Qt::KeepAspectRatio, Qt::FastTransformation);
        pix = QPixmap::fromImage(scaledImg);
    }

    ui->canvas->setPixmap(pix);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;

    switch (event->key()) {
    case Qt::Key_D:
    case Qt::Key_Right:
        m_gameEngine->setGas(true);
        ui->gasBtn->setDown(true);
        break;
    case Qt::Key_A:
    case Qt::Key_Left:
        m_gameEngine->setBrake(true);
        ui->brakeBtn->setDown(true);
        break;
    case Qt::Key_R:
        on_restartBtn_clicked();
        break;
    case Qt::Key_Space:
    case Qt::Key_P:
        m_gameEngine->togglePause();
        break;
    case Qt::Key_F11:
    case Qt::Key_F:
        on_fullscreenBtn_clicked();
        break;
    case Qt::Key_G:
        on_gridBtn_clicked();
        break;
    case Qt::Key_Escape:
        if (isFullScreen()) {
            on_fullscreenBtn_clicked();
        } else {
            m_gameEngine->togglePause();
        }
        break;
    default:
        QMainWindow::keyPressEvent(event);
        break;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;

    switch (event->key()) {
    case Qt::Key_D:
    case Qt::Key_Right:
        m_gameEngine->setGas(false);
        ui->gasBtn->setDown(false);
        break;
    case Qt::Key_A:
    case Qt::Key_Left:
        m_gameEngine->setBrake(false);
        ui->brakeBtn->setDown(false);
        break;
    default:
        QMainWindow::keyReleaseEvent(event);
        break;
    }
}

void MainWindow::on_gridBtn_clicked()
{
    m_isGridVisible = !m_isGridVisible;
    if (m_isGridVisible) {
        ui->gridBtn->setText("Hide Grid (G)");
        ui->gridBtn->setStyleSheet("background-color: #f57c00; color: white; border: 1px solid #ff9800;");
    } else {
        ui->gridBtn->setText("Show Grid (G)");
        ui->gridBtn->setStyleSheet("background-color: #2e7d32; color: white; border: 1px solid #43a047;");
    }
    updateCanvasDimensions();
}

void MainWindow::on_fullscreenBtn_clicked()
{
    if (isFullScreen()) {
        showNormal();
        ui->fullscreenBtn->setText("Fullscreen (F11)");
        ui->fullscreenBtn->setStyleSheet("background-color: #1976d2; color: white; border: 1px solid #42a5f5;");
    } else {
        showFullScreen();
        ui->fullscreenBtn->setText("Windowed (F11)");
        ui->fullscreenBtn->setStyleSheet("background-color: #7b1fa2; color: white; border: 1px solid #ab47bc;");
    }
    updateCanvasDimensions();
}

void MainWindow::on_restartBtn_clicked()
{
    m_gameEngine->reset();
    setFocus();
}

void MainWindow::on_zoomSet_valueChanged(int arg1)
{
    m_gridCellSize = std::clamp(arg1, 1, 8);
    updateCanvasDimensions();
    setFocus();
}

void MainWindow::on_gasBtn_pressed()
{
    m_gameEngine->setGas(true);
}

void MainWindow::on_gasBtn_released()
{
    m_gameEngine->setGas(false);
    setFocus();
}

void MainWindow::on_brakeBtn_pressed()
{
    m_gameEngine->setBrake(true);
}

void MainWindow::on_brakeBtn_released()
{
    m_gameEngine->setBrake(false);
    setFocus();
}
