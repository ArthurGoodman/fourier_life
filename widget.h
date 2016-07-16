#pragma once

#include <QWidget>

#include "worker.h"

class Widget : public QWidget {
    Q_OBJECT

    QThread *thread;
    Worker *worker;

    QPoint lastPos;
    QPointF offset;
    int cellSize;

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

protected:
    void timerEvent(QTimerEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void wheelEvent(QWheelEvent *e);
    void paintEvent(QPaintEvent *e);

private:
    void defaults();
};
