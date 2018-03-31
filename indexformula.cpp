#include "indexformula.h"
#include <ctime>
#include <set>

const double coeff = -1.0 / (86400.0 * 14.0);

IndexFormula::IndexFormula()
{

}

void IndexFormula::calculate(PostReader *pr) {
    int t = pr->_posts[0][0].ts;
    time_t rawtime = t;
    struct tm * tinfo = localtime(&rawtime);
    tinfo->tm_hour = 0;
    tinfo->tm_min = 0;
    tinfo->tm_sec = 0;
    rawtime = mktime(tinfo);
    t = rawtime;
    double idx = 4000.0;

    auto next_open = [&]()->int {
        rawtime = t;
        tinfo = localtime(&rawtime);
        tinfo->tm_hour = 9;
        tinfo->tm_min = 30;
        tinfo->tm_sec = 0;
        rawtime = mktime(tinfo);
        return rawtime > t ? rawtime : rawtime + 86400;
    };
    auto next_close = [&]()->int {
        rawtime = t;
        tinfo = localtime(&rawtime);
        tinfo->tm_hour = 15;
        tinfo->tm_min = 0;
        tinfo->tm_sec = 0;
        rawtime = mktime(tinfo);
        return rawtime > t ? rawtime : rawtime + 86400;
    };
    auto next_day = [&]()->int {
        rawtime = t;
        tinfo = localtime(&rawtime);
        tinfo->tm_hour = 0;
        tinfo->tm_min = 0;
        tinfo->tm_sec = 0;
        rawtime = mktime(tinfo);
        return rawtime > t ? rawtime : rawtime + 86400;
    };
    auto denorm = [](double x) { return std::abs(x) < 1e-5 ? 0 : x; };
    std::vector<Post> p;
    for (auto&& thr : pr->_posts) {
        p.insert(p.end(), thr.begin(), thr.end());
    }
    std::sort(p.begin(), p.end());

    auto i = p.begin();
    while(i != p.end()) {
        int timestamp;
        double open, close, high, low, volume, td;
        std::set<int> holders;
        high = 0;
        low = 1e+6;
        volume = 0;
        timestamp = t;
        holders.clear();
        int ttar = next_open();
        auto lb = std::lower_bound(i, p.end(), ttar);
        for (auto j = i; j != lb; j++) {
            td = j->ts - t;
            t = j->ts;
            idx *= std::exp(coeff * td);
            idx = denorm(idx);
            low = std::min(low, idx);
            idx += 0.5 + 0.5 * std::tanh((double)j->length * 0.01);
            volume += 1.0;
            high = std::max(high, idx);
            holders.insert(j->uid);
        }
        td = ttar - t;
        t = ttar;
        idx *= std::exp(coeff * td);
        idx = denorm(idx);
        open = idx;
        i = lb;

        ttar = next_close();
        lb = std::lower_bound(i, p.end(), ttar);
        for (auto j = i; j != lb; j++) {
            td = j->ts - t;
            t = j->ts;
            idx *= std::exp(coeff * td);
            idx = denorm(idx);
            low = std::min(low, idx);
            idx += 0.5 + 0.5 * std::tanh((double)j->length * 0.01);
            volume += 1.0;
            high = std::max(high, idx);
            holders.insert(j->uid);
        }
        td = ttar - t;
        t = ttar;
        idx *= std::exp(coeff * td);
        idx = denorm(idx);
        close = idx;
        i = lb;

        ttar = next_day();
        lb = std::lower_bound(i, p.end(), ttar);
        for (auto j = i; j != lb; j++) {
            td = j->ts - t;
            t = j->ts;
            idx *= std::exp(coeff * td);
            idx = denorm(idx);
            low = std::min(low, idx);
            idx += 0.5 + 0.5 * std::tanh((double)j->length * 0.01);
            volume += 1.0;
            high = std::max(high, idx);
            holders.insert(j->uid);
        }
        td = ttar - t;
        t = ttar;
        idx *= std::exp(coeff * td);
        idx = denorm(idx);
        low = std::min(low, idx);
        i = lb;
        qDebug() << timestamp << "," << open << "," << close << "," << high << "," << low << "," << volume << "," << holders.size();
        k_day.emplace_back(timestamp, open, close, high, low, volume, holders.size());
    }
}
