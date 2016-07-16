#include "widget.h"

#include <QtWidgets>

Widget::Widget(QWidget *parent)
    : QWidget(parent), running(false) {
    resize(qApp->desktop()->size() / 3 * 2);
    setMinimumSize(qApp->desktop()->size() / 4);
    move(qApp->desktop()->rect().center() - rect().center());

    setCursor(Qt::OpenHandCursor);

    qsrand(QTime::currentTime().msec());

    init();
    defaults();

    setBit(fieldWidth / 2, fieldHeight / 2);
    setBit(fieldWidth / 2, fieldHeight / 2 + 1);
    setBit(fieldWidth / 2 - 1, fieldHeight / 2 + 1);
    setBit(fieldWidth / 2, fieldHeight / 2 - 1);
    setBit(fieldWidth / 2 + 1, fieldHeight / 2);

    startTimer(16);
}

Widget::~Widget() {
    release();
}

void Widget::timerEvent(QTimerEvent *) {
    if (running)
        advance();

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
        randomize();
        break;

    case Qt::Key_Space:
        running = !running;
        break;
    }
}

void Widget::wheelEvent(QWheelEvent *e) {
    if (e->delta() > 0)
        cellSize++;
    else if (cellSize > 1)
        cellSize--;
}

void Widget::paintEvent(QPaintEvent *) {
    static const QColor bgColor = QColor(64, 64, 64), gridColor = QColor(48, 48, 48), fgColor = QColor(200, 200, 200);

    QPainter p(this);
    p.fillRect(rect(), gridColor);

    int hw = width() / 2, hh = height() / 2;

    p.translate(hw, hh);

    hw /= cellSize;
    hh /= cellSize;

    for (int y = -hh; y <= hh; y++)
        for (int x = -hw; x <= hw; x++) {
            int bit = at(x + (int)offset.x(), y - (int)offset.y());

            if (bit == -1)
                continue;

            p.fillRect(x * (cellSize + 1) - cellSize / 2, -y * (cellSize + 1) - cellSize / 2, cellSize, cellSize, bit ? fgColor : bgColor);
        }
}

void Widget::init() {
    field = (fftwf_complex *)fftwf_malloc(fieldWidth * fieldHeight * sizeof(fftwf_complex));
    sum = (fftwf_complex *)fftwf_malloc(fieldWidth * fieldHeight * sizeof(fftwf_complex));
    filter = (fftwf_complex *)fftwf_malloc(fieldWidth * fieldHeight * sizeof(fftwf_complex));
    temp = (fftwf_complex *)fftwf_malloc(fieldWidth * fieldHeight * sizeof(fftwf_complex));

    forward_plan = fftwf_plan_dft_2d(fieldHeight, fieldWidth, field, temp, FFTW_FORWARD, FFTW_MEASURE);
    backward_plan = fftwf_plan_dft_2d(fieldHeight, fieldWidth, temp, sum, FFTW_BACKWARD, FFTW_MEASURE);

    std::fill((float *)field, (float *)(field + fieldWidth * fieldHeight), 0.0f);

    createFilter();
}

void Widget::release() {
    fftwf_free(field);
    fftwf_free(sum);
    fftwf_free(filter);
    fftwf_free(temp);

    fftwf_destroy_plan(forward_plan);
    fftwf_destroy_plan(backward_plan);
}

void Widget::defaults() {
    offset = QPointF(fieldWidth / 2, -fieldHeight / 2);
    cellSize = 4;
}

void Widget::randomize() {
    for (int x = 0; x < fieldWidth; x++)
        for (int y = 0; y < fieldHeight; y++)
            field[index(x, y)][0] = qrand() % 2;
}

void Widget::createFilter() {
    fftwf_plan filter_plan = fftwf_plan_dft_2d(fieldHeight, fieldWidth, filter, filter, FFTW_FORWARD, FFTW_MEASURE);

    std::fill((float *)filter, (float *)(filter + fieldWidth * fieldHeight), 0.0f);

    filter[index(-1, -1)][0] = 1;
    filter[index(0, -1)][0] = 1;
    filter[index(1, -1)][0] = 1;
    filter[index(-1, 0)][0] = 1;
    filter[index(1, 0)][0] = 1;
    filter[index(-1, 1)][0] = 1;
    filter[index(0, 1)][0] = 1;
    filter[index(1, 1)][0] = 1;

    fftwf_execute(filter_plan);
    fftwf_destroy_plan(filter_plan);

    save(filter, 0, "filter.png");
}

int Widget::index(int x, int y) {
    return (x + fieldWidth) % fieldWidth + (y + fieldHeight) % fieldHeight * fieldWidth;
}

int Widget::at(int x, int y) {
    if (x < 0 || y < 0 || x > fieldWidth || y > fieldHeight)
        return -1;

    return field[index(x, y)][0];
}

void Widget::setBit(int x, int y) {
    field[index(x, y)][0] = 1;
}

void Widget::advance() {
    fftwf_execute(forward_plan);

    for (int x = 0; x < fieldWidth; x++)
        for (int y = 0; y < fieldHeight; y++) {
            temp[index(x, y)][0] *= filter[index(x, y)][0];
            temp[index(x, y)][1] *= filter[index(x, y)][0];
        }

    fftwf_execute(backward_plan);

    for (int x = 0; x < fieldWidth; x++)
        for (int y = 0; y < fieldHeight; y++) {
            int s = (int)round(sum[index(x, y)][0]) / fieldWidth / fieldHeight;

            if (field[index(x, y)][0])
                field[index(x, y)][0] = s == 2 || s == 3;
            else
                field[index(x, y)][0] = s == 3;
        }
}

void Widget::save(fftwf_complex *data, int dim, QString fileName) {
    QImage image(fieldWidth, fieldHeight, QImage::Format_RGB32);

    double max = *std::max_element((double *)data, (double *)(data + fieldWidth * fieldHeight));

    for (int x = 0; x < fieldWidth; x++)
        for (int y = 0; y < fieldHeight; y++) {
            double f = data[index(x, y)][dim] / max;
            image.setPixel(x, y, qRgb(f * 255, f * 255, f * 255));
        }

    image.save(fileName);
}
