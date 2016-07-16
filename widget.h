#pragma once

#include <QWidget>
#include <fftw3.h>

class Widget : public QWidget {
    Q_OBJECT

    const int fieldWidth = 300;
    const int fieldHeight = 300;

    fftwf_complex *field, *filter, *sum;
    fftwf_plan forward_plan, backward_plan;

    bool running;

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
    void init();
    void release();
    void defaults();

    int index(int x, int y);
    int at(int x, int y);
    void setBit(int x, int y);
    void randomize();

    void advance();
};
