/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *mainVerticalLayout;
    QFrame *topPanel;
    QHBoxLayout *topHorizontalLayout;
    QPushButton *restartBtn;
    QPushButton *gridBtn;
    QLabel *zoomLabel;
    QSpinBox *zoomSet;
    QPushButton *fullscreenBtn;
    QSpacerItem *horizontalSpacer;
    QLabel *controlsHintLabel;
    QLabel *canvas;
    QFrame *bottomPanel;
    QHBoxLayout *bottomHorizontalLayout;
    QPushButton *brakeBtn;
    QSpacerItem *bottomSpacer;
    QLabel *gameStatusLabel;
    QSpacerItem *bottomSpacer2;
    QPushButton *gasBtn;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1200, 760);
        MainWindow->setStyleSheet(QString::fromUtf8("QMainWindow {\n"
"    background-color: #1a1e24;\n"
"}\n"
"QWidget#centralwidget {\n"
"    background-color: #1a1e24;\n"
"}\n"
"QPushButton {\n"
"    background-color: #2b3543;\n"
"    color: #f0f4f8;\n"
"    border: 1px solid #4a5a70;\n"
"    border-radius: 5px;\n"
"    padding: 6px 14px;\n"
"    font-weight: 600;\n"
"    font-size: 13px;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #3b495c;\n"
"    border-color: #607d9b;\n"
"}\n"
"QPushButton:pressed {\n"
"    background-color: #202833;\n"
"}\n"
"QSpinBox {\n"
"    background-color: #2b3543;\n"
"    color: #f0f4f8;\n"
"    border: 1px solid #4a5a70;\n"
"    border-radius: 5px;\n"
"    padding: 4px 8px;\n"
"    font-size: 13px;\n"
"    font-weight: 600;\n"
"}\n"
"QLabel {\n"
"    color: #e0e6ed;\n"
"    font-size: 13px;\n"
"}"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        mainVerticalLayout = new QVBoxLayout(centralwidget);
        mainVerticalLayout->setSpacing(6);
        mainVerticalLayout->setObjectName("mainVerticalLayout");
        mainVerticalLayout->setContentsMargins(8, 8, 8, 8);
        topPanel = new QFrame(centralwidget);
        topPanel->setObjectName("topPanel");
        topPanel->setFrameShape(QFrame::Shape::StyledPanel);
        topPanel->setStyleSheet(QString::fromUtf8("QFrame#topPanel {\n"
"    background-color: #242c38;\n"
"    border-radius: 6px;\n"
"    border: 1px solid #364456;\n"
"}"));
        topHorizontalLayout = new QHBoxLayout(topPanel);
        topHorizontalLayout->setObjectName("topHorizontalLayout");
        topHorizontalLayout->setContentsMargins(-1, 6, -1, 6);
        restartBtn = new QPushButton(topPanel);
        restartBtn->setObjectName("restartBtn");
        restartBtn->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        restartBtn->setStyleSheet(QString::fromUtf8("background-color: #c93b3b; color: white; border: 1px solid #e05555;"));

        topHorizontalLayout->addWidget(restartBtn);

        gridBtn = new QPushButton(topPanel);
        gridBtn->setObjectName("gridBtn");
        gridBtn->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        gridBtn->setStyleSheet(QString::fromUtf8("background-color: #2e7d32; color: white; border: 1px solid #43a047;"));

        topHorizontalLayout->addWidget(gridBtn);

        zoomLabel = new QLabel(topPanel);
        zoomLabel->setObjectName("zoomLabel");

        topHorizontalLayout->addWidget(zoomLabel);

        zoomSet = new QSpinBox(topPanel);
        zoomSet->setObjectName("zoomSet");
        zoomSet->setMinimum(1);
        zoomSet->setMaximum(8);
        zoomSet->setValue(2);

        topHorizontalLayout->addWidget(zoomSet);

        fullscreenBtn = new QPushButton(topPanel);
        fullscreenBtn->setObjectName("fullscreenBtn");
        fullscreenBtn->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        fullscreenBtn->setStyleSheet(QString::fromUtf8("background-color: #1976d2; color: white; border: 1px solid #42a5f5;"));

        topHorizontalLayout->addWidget(fullscreenBtn);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        topHorizontalLayout->addItem(horizontalSpacer);

        controlsHintLabel = new QLabel(topPanel);
        controlsHintLabel->setObjectName("controlsHintLabel");
        controlsHintLabel->setStyleSheet(QString::fromUtf8("color: #a0b2c6; font-size: 12px; font-weight: bold;"));

        topHorizontalLayout->addWidget(controlsHintLabel);


        mainVerticalLayout->addWidget(topPanel);

        canvas = new QLabel(centralwidget);
        canvas->setObjectName("canvas");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(canvas->sizePolicy().hasHeightForWidth());
        canvas->setSizePolicy(sizePolicy);
        canvas->setMinimumSize(QSize(640, 360));
        canvas->setStyleSheet(QString::fromUtf8("background-color: #0c1017; border: 2px solid #2a3545; border-radius: 4px;"));
        canvas->setAlignment(Qt::AlignmentFlag::AlignCenter);

        mainVerticalLayout->addWidget(canvas);

        bottomPanel = new QFrame(centralwidget);
        bottomPanel->setObjectName("bottomPanel");
        bottomPanel->setFrameShape(QFrame::Shape::StyledPanel);
        bottomPanel->setStyleSheet(QString::fromUtf8("QFrame#bottomPanel {\n"
"    background-color: #242c38;\n"
"    border-radius: 6px;\n"
"    border: 1px solid #364456;\n"
"}"));
        bottomHorizontalLayout = new QHBoxLayout(bottomPanel);
        bottomHorizontalLayout->setObjectName("bottomHorizontalLayout");
        bottomHorizontalLayout->setContentsMargins(-1, 6, -1, 6);
        brakeBtn = new QPushButton(bottomPanel);
        brakeBtn->setObjectName("brakeBtn");
        brakeBtn->setMinimumSize(QSize(180, 44));
        brakeBtn->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        brakeBtn->setStyleSheet(QString::fromUtf8("background-color: #d32f2f; color: white; font-size: 14px; font-weight: bold; border-radius: 6px;"));

        bottomHorizontalLayout->addWidget(brakeBtn);

        bottomSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        bottomHorizontalLayout->addItem(bottomSpacer);

        gameStatusLabel = new QLabel(bottomPanel);
        gameStatusLabel->setObjectName("gameStatusLabel");
        gameStatusLabel->setStyleSheet(QString::fromUtf8("color: #64b5f6; font-size: 14px; font-weight: bold;"));
        gameStatusLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

        bottomHorizontalLayout->addWidget(gameStatusLabel);

        bottomSpacer2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        bottomHorizontalLayout->addItem(bottomSpacer2);

        gasBtn = new QPushButton(bottomPanel);
        gasBtn->setObjectName("gasBtn");
        gasBtn->setMinimumSize(QSize(180, 44));
        gasBtn->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        gasBtn->setStyleSheet(QString::fromUtf8("background-color: #388e3c; color: white; font-size: 14px; font-weight: bold; border-radius: 6px;"));

        bottomHorizontalLayout->addWidget(gasBtn);


        mainVerticalLayout->addWidget(bottomPanel);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Hill Climb Racing - Pure Raster Graphics", nullptr));
        restartBtn->setText(QCoreApplication::translate("MainWindow", "Restart (R)", nullptr));
        gridBtn->setText(QCoreApplication::translate("MainWindow", "Show Grid (G)", nullptr));
        zoomLabel->setText(QCoreApplication::translate("MainWindow", "Grid Cell Size:", nullptr));
        fullscreenBtn->setText(QCoreApplication::translate("MainWindow", "Fullscreen (F11)", nullptr));
        controlsHintLabel->setText(QCoreApplication::translate("MainWindow", "Controls: [D / ->] Gas  |  [A / <-] Brake  |  [Space] Pause  |  [F11] Fullscreen", nullptr));
        canvas->setText(QString());
        brakeBtn->setText(QCoreApplication::translate("MainWindow", "<< BRAKE / TILT [A]", nullptr));
        gameStatusLabel->setText(QCoreApplication::translate("MainWindow", "Hill Climb Racing - Pure Raster Graphics", nullptr));
        gasBtn->setText(QCoreApplication::translate("MainWindow", "GAS / TILT >> [D]", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
