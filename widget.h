#pragma once

#include <QWidget>
#include <fftw3.h>

class Widget : public QWidget {
    Q_OBJECT

    const int fieldWidth = 300;
    const int fieldHeight = 300;

    int cellSize = 4;

    fftwf_complex *field, *sum, *filter, *temp;
    fftwf_plan forward_plan, backward_plan;

    bool running;

    QPoint lastPos;
    QPointF offset;

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
    void randomize();
    void createFilter();
    int index(int x, int y);
    int at(int x, int y);
    void setBit(int x, int y);
    void advance();

    void save(fftwf_complex *data, int dim, QString fileName);
};
