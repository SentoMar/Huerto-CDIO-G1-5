
#ifndef HUERTO_YA_INCLUIDO
#define HUERTO_YA_INCLUIDO
#include <Arduino.h>

class huerto{
    private:
    void HTTPPost(String *, int);
    int inter(double);
    double averageSample(int, int *);

    public:
    huerto(int);
    void connectWiFi();
    double tempValue(int);
    double phValue(int);
    double salinityValue(int);
    double humiditymap(int);
    double luminity(int);
    void HTTPGet(String *,int);
    void regulador(double, double, double, double, double);
};
#endif
