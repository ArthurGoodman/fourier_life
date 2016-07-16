#include "widget.h"

#include <QtWidgets>

Widget::Widget(QWidget *parent)
    : QWidget(parent) {
    resize(qApp->desktop()->size() / 3 * 2);
    setMinimumSize(qApp->desktop()->size() / 4);
    move(qApp->desktop()->rect().center() - rect().center());

    setCursor(Qt::OpenHandCursor);

    qsrand(QTime::currentTime().msec());

    thread = new QThread;

    worker = new Worker;
    worker->setInterval(5);
    worker->moveToThread(thread);

    connect(thread, SIGNAL(started()), worker, SLOT(run()));

    defaults();

    startTimer(16);
}

Widget::~Widget() {
    worker->abort();

    while (thread->isRunning()) {
    }

    delete thread;
    delete worker;
}

void Widget::timerEvent(QTimerEvent *) {
    update();
}

void Widget::mousePressEvent(QMouseEvent *e) {
    lastPos = e->pos();

    setCursor(Qt::ClosedHandCursor);
}

void Widget::mouseMoveEvent(QMouseEvent *e) {
    offset += QPointF(lastPos - e->pos()) / (cellSize + 1);
    lastPos = e->pos();
}

void Widget::mouseReleaseEvent(QMouseEvent *) {
    setCursor(Qt::OpenHandCursor);
}

void Widget::keyPressEvent(QKeyEvent *e) {
    switch (e->key()) {
    case Qt::Key_Escape:
        isFullScreen() ? showNormal() : qApp->quit();
        break;

    case Qt::Key_F11:
        isFullScreen() ? showNormal() : showFullScreen();
        break;

    case Qt::Key_Backspace:
        defaults();
        break;

    case Qt::Key_R:
        worker->randomize();
        break;

    case Qt::Key_Space:
        worker->isRunning() ? worker->abort() : thread->start();
        break;
    }
}

void Widget::wheelEvent(QWheelEvent *e) {
    if (e->delta() > 0)
        cellSize++;
    else if (cellSize > 0)
        cellSize--;
}

void Widget::paintEvent(QPaintEvent *) {
    static const QColor bgColor = QColor(64, 64, 64), gridColor = QColor(48, 48, 48), fgColor = QColor(200, 200, 200);

    QPainter p(this);
    p.fillRect(rect(), gridColor);

    int hw = width() / 2, hh = height() / 2;

    p.translate(hw, hh);

    hw /= (cellSize + 1);
    hh /= (cellSize + 1);

    for (int y = -hh - 1; y <= hh + 1; y++)
        for (int x = -hw - 1; x <= hw + 1; x++) {
            int bit = worker->at(x + (int)offset.x(), y - (int)offset.y());

            if (bit == -1)
                continue;

            p.fillRect(x * (cellSize + 1) - cellSize / 2, -y * (cellSize + 1) - cellSize / 2, qMax(cellSize, 1), qMax(cellSize, 1), bit ? fgColor : bgColor);
        }
}

void Widget::defaults() {
    offset = QPointF(worker->width() / 2, -worker->height() / 2);
    cellSize = 4;
}
