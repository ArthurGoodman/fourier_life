#pragma once

#include <QObject>
#include <fftw3.h>

class Worker : public QObject {
    Q_OBJECT

    const int fieldWidth = 300;
    const int fieldHeight = 300;

    fftwf_complex *field, *filter, *sum;
    fftwf_plan forward_plan, backward_plan;

    bool running, abortRequested, randomizationRequested;

public:
    explicit Worker(QObject *parent = 0);
    ~Worker();

    int width() const;
    int height() const;

    int at(int x, int y) const;

    bool isRunning() const;
    void abort();

    void randomize();

public slots:
    void run();

private:
    void init();
    void release();

    int index(int x, int y) const;
    void setBit(int x, int y);
    void performRandomization();

    void advance();
};
