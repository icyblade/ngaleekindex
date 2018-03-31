#ifndef POST_H
#define POST_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <cstdio>

struct Post {
    int ts;
    int uid;
    int length;
    Post(int ts, int uid, int length) : ts(ts), uid(uid), length(length) { }
    bool operator<(const Post& rhs) { return ts < rhs.ts; }
    bool operator<(int rhs) { return ts < rhs; }
    friend bool operator<(int lhs, const Post& rhs) { return lhs < rhs.ts; }
};

class PostReader : public QObject
{
    Q_OBJECT

public:
    PostReader();
    ~PostReader();
    int processed() const;
    int to_process() const;
    bool finished() const;
    void refresh();
    std::vector<std::vector<Post>> _posts;

public slots:
    void network_recieved(QNetworkReply* reply);

signals:
    void progress_made();

private:
    int _state;
    int _to_process;
    int _processed;
    std::vector<int> _rows;
    std::vector<int> _r_rows_page;
    std::vector<std::vector<int>> _pages_checked;
    QNetworkAccessManager* _qnam;
};

#endif // POST_H
