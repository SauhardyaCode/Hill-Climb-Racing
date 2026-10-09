#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QMainWindow>
#include <QColor>
#include <QPixmap>
#include <QPainter>
#include <QPen>

QT_BEGIN_NAMESPACE
namespace Ui {
class GameWindow;
}
QT_END_NAMESPACE

class GameWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GameWindow(QWidget *parent = nullptr);
    ~GameWindow() override;

private:
    Ui::GameWindow *ui;
    int canvasWidth;
    int canvasHeight;
    bool isGridVisible;
    QMap<QPair<int, int>, QColor> pixelMap;

    void updateScreen();

protected:
    void showEvent(QShowEvent *event) override;
private slots:
    void on_gridBtn_clicked();
    void on_zoomSet_valueChanged(int arg1);
};
#endif // GAMEWINDOW_H
