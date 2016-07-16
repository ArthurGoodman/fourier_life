#include "worker.h"

#include <QTimer>
#include <QThread>
#include <QTime>
#include <omp.h>

Worker::Worker(QObject *parent)
    : QObject(parent), running(false), abortRequested(false), interval(0), c(0) {
    init();

    setBit(fieldWidth / 2, fieldHeight / 2);
    setBit(fieldWidth / 2, fieldHeight / 2 + 1);
    setBit(fieldWidth / 2 - 1, fieldHeight / 2 + 1);
    setBit(fieldWidth / 2, fieldHeight / 2 - 1);
    setBit(fieldWidth / 2 + 1, fieldHeight / 2);
}

Worker::~Worker() {
    release();
}

int Worker::width() const {
    return fieldWidth;
}

int Worker::height() const {
    return fieldHeight;
}

void Worker::setInterval(int interval) {
    this->interval = interval;
}

int Worker::at(int x, int y) const {
    if (x < 0 || y < 0 || x > fieldWidth || y > fieldHeight)
        return -1;

    return buffer[c][index(x, y)][0];
}

bool Worker::isRunning() const {
    return running;
}

void Worker::abort() {
    abortRequested = true;
}

void Worker::randomize() {
    if (!running)
        performRandomization();
    else
        randomizationRequested = true;
}

void Worker::run() {
    qsrand(QTime::currentTime().msec());

    abortRequested = false;
    running = true;

    while (!abortRequested) {
        QThread::msleep(interval);

        if (randomizationRequested)
            performRandomization();

        advance();
    }

    running = false;

    QThread::currentThread()->quit();
}

void Worker::init() {
    fftwf_init_threads();

    fftwf_plan_with_nthreads(omp_get_max_threads());

    buffer[0] = (fftwf_complex *)fftwf_malloc(fieldWidth * fieldHeight * sizeof(fftwf_complex));
    buffer[1] = (fftwf_complex *)fftwf_malloc(fieldWidth * fieldHeight * sizeof(fftwf_complex));
    filter = (fftwf_complex *)fftwf_malloc(fieldWidth * fieldHeight * sizeof(fftwf_complex));
    sum = (fftwf_complex *)fftwf_malloc(fieldWidth * fieldHeight * sizeof(fftwf_complex));

    forward_plan[0] = fftwf_plan_dft_2d(fieldHeight, fieldWidth, buffer[0], sum, FFTW_FORWARD, FFTW_MEASURE);
    forward_plan[1] = fftwf_plan_dft_2d(fieldHeight, fieldWidth, buffer[1], sum, FFTW_FORWARD, FFTW_MEASURE);
    backward_plan = fftwf_plan_dft_2d(fieldHeight, fieldWidth, sum, sum, FFTW_BACKWARD, FFTW_MEASURE);

    std::fill((float *)buffer[0], (float *)(buffer[0] + fieldWidth * fieldHeight), 0.0f);
    std::fill((float *)buffer[1], (float *)(buffer[1] + fieldWidth * fieldHeight), 0.0f);

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
}

void Worker::release() {
    fftwf_free(buffer[0]);
    fftwf_free(buffer[1]);
    fftwf_free(filter);
    fftwf_free(sum);

    fftwf_destroy_plan(forward_plan[0]);
    fftwf_destroy_plan(forward_plan[1]);
    fftwf_destroy_plan(backward_plan);

    fftwf_cleanup_threads();
}

int Worker::index(int x, int y) const {
    return (x + fieldWidth) % fieldWidth + (y + fieldHeight) % fieldHeight * fieldWidth;
}

void Worker::setBit(int x, int y) {
    buffer[c][index(x, y)][0] = 1;
}

int Worker::swap(int i) const {
    return (i + 1) % 2;
}

void Worker::performRandomization() {
    randomizationRequested = false;

    for (int x = 0; x < fieldWidth; x++)
        for (int y = 0; y < fieldHeight; y++)
            buffer[swap(c)][index(x, y)][0] = qrand() % 2;

    c = swap(c);
}

void Worker::advance() {
    fftwf_execute(forward_plan[c]);

    for (int x = 0; x < fieldWidth; x++)
        for (int y = 0; y < fieldHeight; y++) {
            if (abortRequested)
                return;

            sum[index(x, y)][0] *= filter[index(x, y)][0];
            sum[index(x, y)][1] *= filter[index(x, y)][0];
        }

    fftwf_execute(backward_plan);

    for (int x = 0; x < fieldWidth; x++)
        for (int y = 0; y < fieldHeight; y++) {
            if (abortRequested)
                return;

            int s = round(sum[index(x, y)][0]) / fieldWidth / fieldHeight;

            if (buffer[c][index(x, y)][0])
                buffer[swap(c)][index(x, y)][0] = s == 2 || s == 3;
            else
                buffer[swap(c)][index(x, y)][0] = s == 3;
        }

    c = swap(c);
}
