#include "worker.h"

#include <omp.h>

Worker::Worker(QObject *parent)
    : QObject(parent), running(false), abortRequested(false) {
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

int Worker::at(int x, int y) const {
    if (x < 0 || y < 0 || x > fieldWidth || y > fieldHeight)
        return -1;

    return field[index(x, y)][0];
}

bool Worker::isRunning() const {
    return running;
}

void Worker::abort() {
    abortRequested = true;
}

void Worker::randomize() {
    randomizationRequested = true;
}

void Worker::run() {
    abortRequested = false;
    running = true;

    while (!abortRequested) {
        if (randomizationRequested)
            performRandomization();

        advance();
    }

    running = false;
}

void Worker::init() {
    fftwf_init_threads();

    fftwf_plan_with_nthreads(omp_get_max_threads());

    field = (fftwf_complex *)fftwf_malloc(fieldWidth * fieldHeight * sizeof(fftwf_complex));
    field = (fftwf_complex *)fftwf_malloc(fieldWidth * fieldHeight * sizeof(fftwf_complex));
    filter = (fftwf_complex *)fftwf_malloc(fieldWidth * fieldHeight * sizeof(fftwf_complex));
    sum = (fftwf_complex *)fftwf_malloc(fieldWidth * fieldHeight * sizeof(fftwf_complex));

    forward_plan = fftwf_plan_dft_2d(fieldHeight, fieldWidth, field, sum, FFTW_FORWARD, FFTW_MEASURE);
    backward_plan = fftwf_plan_dft_2d(fieldHeight, fieldWidth, sum, sum, FFTW_BACKWARD, FFTW_MEASURE);

    std::fill((float *)field, (float *)(field + fieldWidth * fieldHeight), 0.0f);

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
    fftwf_free(field);
    fftwf_free(filter);
    fftwf_free(sum);

    fftwf_destroy_plan(forward_plan);
    fftwf_destroy_plan(backward_plan);

    fftwf_cleanup_threads();
}

int Worker::index(int x, int y) const {
    return (x + fieldWidth) % fieldWidth + (y + fieldHeight) % fieldHeight * fieldWidth;
}

void Worker::setBit(int x, int y) {
    field[index(x, y)][0] = 1;
}

void Worker::performRandomization() {
    randomizationRequested = false;

    for (int x = 0; x < fieldWidth; x++)
        for (int y = 0; y < fieldHeight; y++)
            field[index(x, y)][0] = qrand() % 2;
}

void Worker::advance() {
    fftwf_execute(forward_plan);

    for (int x = 0; x < fieldWidth; x++)
        for (int y = 0; y < fieldHeight; y++) {
            sum[index(x, y)][0] *= filter[index(x, y)][0];
            sum[index(x, y)][1] *= filter[index(x, y)][0];
        }

    fftwf_execute(backward_plan);

    for (int x = 0; x < fieldWidth; x++)
        for (int y = 0; y < fieldHeight; y++) {
            int s = round(sum[index(x, y)][0]) / fieldWidth / fieldHeight;

            if (field[index(x, y)][0])
                field[index(x, y)][0] = s == 2 || s == 3;
            else
                field[index(x, y)][0] = s == 3;
        }
}
