#include "post.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <cstring>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QTextCodec>
#include <QFile>
#include <QDataStream>
#include <QApplication>

// 韭菜集中贴tid
const int tid[] = { 8869728, 10811588, 13196858, 0 };
const char nga_json_shift[] = "window.script_muti_get_var_store=";

QList<QNetworkCookie>& cookie() {
    static QList<QNetworkCookie> _c;
    if (_c.empty()) {
        _c.push_back(QNetworkCookie("ngaPassportCid", ""));
        _c.push_back(QNetworkCookie("ngaPassportUid", ""));
    }
    return _c;
}

PostReader::PostReader()
{
    _qnam = new QNetworkAccessManager(this);
    connect(_qnam, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(network_recieved(QNetworkReply*)));
    _state = 0;
    _to_process = 0;
    _processed = 0;
    for (int i = 0; tid[i]; i++)
        _posts.emplace_back();
    for (int i = 0; tid[i]; i++)
        _pages_checked.emplace_back();
    QFile f(QApplication::applicationDirPath() + "\\posts.dat");
    f.open(QIODevice::ReadOnly);
    QDataStream s(&f);
    int numthread = 0;
    s >> numthread;
    std::vector<int> post_size;
    for (int i = 0; i < numthread; i++) {
        int t;
        s >> t;
        _pages_checked[i].resize(t);
        s >> t;
        post_size.push_back(t);
    }
    for (int i = 0; i < numthread; i++) {
        for (auto&& j : _pages_checked[i]) {
            s >> j;
        }
    }
    for (int i = 0; i < numthread; i++) {
        for (int j = 0; j < post_size[i]; j++) {
            int ts, uid, length;
            s >> ts >> uid >> length;
            _posts[i].emplace_back(ts, uid, length);
        }
    }
    f.close();
}

PostReader::~PostReader() {
    delete _qnam;
}

int PostReader::processed() const {
    return _processed;
}

int PostReader::to_process() const {
    return _to_process;
}

void PostReader::network_recieved(QNetworkReply* reply) {
    QByteArray data = reply->readAll();
    data.remove(0, strlen(nga_json_shift));
    QTextCodec *gbk = QTextCodec::codecForName("gb18030");
    QString datautf8 = gbk->toUnicode(data);
    QJsonDocument j = QJsonDocument::fromJson(datautf8.toUtf8());
    switch(_state) {
    case 1: {
        int rec_tid = j["data"]["__T"]["tid"].toInt();
        for (int i = 0; tid[i]; i++) {
            if (rec_tid == tid[i]) {
                _rows[i] = j["data"]["__ROWS"].toInt();
                _r_rows_page[i] = j["data"]["__R__ROWS_PAGE"].toInt();
            }
        }
        _processed++;
        if (_processed == _to_process) {
            _processed = 0;
            _to_process = 0;
            _state = 2;
            for (int i = 0; tid[i]; i++) {
                int total = _rows[i];
                int pages = (total /*+ _r_rows_page[i] - 1*/) / _r_rows_page[i];
                if (pages > _pages_checked[i].size()) _pages_checked[i].resize(pages);
                for (int page = 1; page <= pages; page++) {
                    if (_pages_checked[i][page-1]) continue;
                    QUrl url = QString("http://bbs.nga.cn/read.php?tid=")
                        + QString::number(tid[i]) + QString("&lite=js&page=")
                        + QString::number(page);
                    QNetworkRequest req(url);
                    _qnam->get(req);
                    _to_process++;
                }
            }
            if (_to_process <= 0) {
                _to_process = 1;
                QUrl url = QString("http://aean.net/hello?page=-1");
                QNetworkRequest req(url);
                _qnam->get(req);
            }
            emit progress_made();
        }
    } break;
    case 2: {
        int page = reply->url().toString().split("page=").back().toInt();
        QJsonValue jp = j["data"]["__R"];
        for (int i = 0; i < jp.toObject().size(); i++) {
            int ts = jp[QString::number(i)]["postdatetimestamp"].toInt();
            int uid = jp[QString::number(i)]["authorid"].toInt();
            int length = jp[QString::number(i)]["content_length"].toInt();
            int ptid = jp[QString::number(i)]["tid"].toInt();
            for (int j = 0; tid[j]; j++) {
                if (ptid == tid[j] && page > 0) {
                    _pages_checked[j][page-1] = 1;
                    _posts[j].emplace_back(ts, uid, length);
                }
            }
        }
        _processed++;
        if (_processed >= _to_process) {
            for (int i = 0; tid[i]; i++)
                std::sort(_posts[i].begin(), _posts[i].end());
            QFile f(QApplication::applicationDirPath() + "\\posts.dat");
            f.open(QIODevice::WriteOnly);
            QDataStream s(&f);
            int numthread = 0;
            for (; tid[numthread]; numthread++);
            s << numthread;
            for (int i = 0; i < numthread; i++) {
                s << _pages_checked[i].size();
                s << _posts[i].size();
            }
            for (int i = 0; i < numthread; i++) {
                for (auto&& j : _pages_checked[i])
                    s << j;
            }
            for (int i = 0; i < numthread; i++) {
                for (auto&& p : _posts[i]) {
                    s << p.ts << p.uid << p.length;
                }
            }
            f.close();
            _state = 0;
        }
        emit progress_made();
    } break;
    }
    reply->deleteLater();
}

void PostReader::refresh() {
    _to_process = 0;
    _rows.clear();
    _r_rows_page.clear();
    _processed = 0;
    _state = 1;
    for (int i = 0; tid[i]; i++) {
        _to_process++;
        _rows.push_back(0);
        _r_rows_page.push_back(20);
        QUrl url = QString("http://bbs.nga.cn/read.php?tid=") + QString::number(tid[i]) + QString("&lite=js");
        _qnam->cookieJar()->setCookiesFromUrl(cookie(), QUrl("http://bbs.nga.cn"));
        _qnam->get(QNetworkRequest(url));
    }
    emit progress_made();
}

bool PostReader::finished() const {
    return _state == 0;
}
