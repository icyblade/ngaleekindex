#ifndef INDEXFORMULA_H
#define INDEXFORMULA_H

#include "post.h"

struct CandleStick {
    int timestamp;
    double open;
    double close;
    double high;
    double low;
    double volume;
    double holder;
    CandleStick(int timestamp, double open, double close, double high, double low, double volume, double holder)
        : timestamp(timestamp), open(open), close(close), high(high), low(low), volume(volume), holder(holder) { }
};

class IndexFormula
{
public:
    IndexFormula();
    std::vector<CandleStick> k_day;
    void calculate(PostReader* pr);
};

#endif // INDEXFORMULA_H
