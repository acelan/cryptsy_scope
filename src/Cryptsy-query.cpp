#include "Cryptsy-query.h"
#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QDir>
#include <QColor>
#include <QVector>

using namespace unity::scopes;

const QString DATA_PATH = "%1/.cache/Cryptsy";
const QString ALL_MARKET_URI = "http://pubapi.cryptsy.com/api.php?method=marketdatav2";
const QString SINGLE_MARKET_URI = "http://pubapi.cryptsy.com/api.php?method=singlemarketdata&marketid=%1";
const QString VIEW_MARKET = "http://www.cryptsy.com/markets/view/%1";

std::string CR_GRID = R"(
    {
        "schema-version" : 1,
        "template" : {
            "category-layout" : "grid",
            "card-size": "small"
        },
        "components" : {
            "title" : "title",
            "art" : {
                "field": "art",
                "aspect-ratio": 1.6,
                "fill-mode": "fit"
            }
        }
    }
)";

CryptsyQuery::CryptsyQuery(std::string const& query) :
    query_(query.c_str())
{
    qDebug() << __func__ << " : " << __LINE__;
    readData();
}

CryptsyQuery::~CryptsyQuery()
{
    qDebug() << __func__ << " : " << __LINE__;
}

void CryptsyQuery::cancelled()
{
    qDebug() << __func__ << " : " << __LINE__;
}

void CryptsyQuery::writeData()
{
    QFile datafile(getDataPath() + "/data.dat");
    if(!datafile.open(QIODevice::WriteOnly)){
        return;
    }
    QDataStream out (&datafile);
    out << data_;
}

void CryptsyQuery::readData()
{
    QFile datafile(getDataPath() + "/data.dat");
    if(!datafile.open(QIODevice::ReadOnly)){
        return;
    }
    QDataStream in (&datafile);
    in >> data_;
}

QString CryptsyQuery::getDataPath()
{
    QString path = DATA_PATH.arg(QDir::homePath());
    return path;
}

QString CryptsyQuery::getImageFile(QString name)
{
    QString imgpath = getDataPath() + "/imgs";
    QDir imgdir(imgpath);
    if (!imgdir.exists())
    {
        imgdir.mkdir(getDataPath());
        imgdir.mkdir(imgpath);
    }

    QString filename = imgpath + "/" + name;
    QImage img(200,200,QImage::Format_RGB16);

    QFileInfo fi(name);
    QString label = fi.baseName();
    const QVector<Trade*>& trades = trades_[label];

    qDebug() << __func__ << " : " << __LINE__ << "label: " << label << " : size = " << trades.size();
    double current_price = 0;
    double previous_price = 0;
    Qt::GlobalColor bgcolor = Qt::darkBlue;
    if(trades.size() > 2)
    {

        current_price = trades[0]->price_.toDouble();
        previous_price = trades[1]->price_.toDouble();
        if(current_price > previous_price)
            bgcolor = Qt::darkGreen;
        else if(current_price < previous_price)
            bgcolor = Qt::darkRed;
    }

    img.fill(bgcolor);

    QPainter p(&img);
    p.setPen(QPen(Qt::white));
    p.setFont(QFont("Times", 28, QFont::Bold));
    p.drawText(img.rect(), Qt::AlignCenter, QString::number(current_price));
    p.end();

    img.save(filename, "PNG");

    return filename;
}

void CryptsyQuery::run(SearchReplyProxy const& reply)
{
    CategoryRenderer rdr(CR_GRID);
    auto catGrid = reply->register_category("Virtual Currency", "All Markets", "", rdr);

    qDebug() << __func__ << " : " << __LINE__;

    QEventLoop loop;

    QNetworkAccessManager manager;
    QObject::connect(&manager, SIGNAL(finished(QNetworkReply*)), &loop, SLOT(quit()));

    QObject::connect(&manager, &QNetworkAccessManager::finished,
            [reply, catGrid, this](QNetworkReply *msg){
                QByteArray data = msg->readAll();
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(data, &err);
                QJsonObject obj = doc.object();

                if (err.error != QJsonParseError::NoError) {
                    qCritical() << "Failed to parse server data: " << err.errorString();
                } else {
                    // Find the "market" array of results
                    QJsonObject returnObj = obj["return"].toObject();
                    QJsonObject marketsObj = returnObj["markets"].toObject();
                    CategorisedResult catres((catGrid));

                    //loop through results of our web query with each result called 'result'
                    for( int i = 0; i < marketsObj.count(); i++) {
                        //treat result as Q JSON
                        QJsonObject resJ = marketsObj[marketsObj.keys().at(i)].toObject();

                        // load up vars with from result
                        auto title = resJ["label"].toString();
                        if(!query_.isEmpty() && !title.contains(query_, Qt::CaseInsensitive))
                            continue;
                        QString imglabel(title);
                        imglabel.replace('/','_');

                        QVector<Trade*> recenttrades;
                        QJsonArray trades = resJ["recenttrades"].toArray();
                        for(const auto &trade: trades)
                        {
                            QJsonObject tradeJ = trade.toObject();
                            recenttrades.push_back(new Trade(tradeJ["id"].toString(), tradeJ["time"].toString(), tradeJ["price"].toString(), tradeJ["quantity"].toString(), tradeJ["total"].toString()));
                        }
                        trades_[imglabel] = recenttrades;
                        qDebug() << __func__ << " : " << __LINE__ << "label: " << title << " : size = " << recenttrades.size();

                        auto price = resJ["lasttradeprice"].toString();

                        auto uri = VIEW_MARKET.arg(resJ["marketid"].toString());

                        auto image = getImageFile(imglabel+".png");
                        auto desc = resJ["label"].toString();

                        //set our CateogroisedResult object with out searchresults values
                        catres.set_uri(uri.toStdString());
                        catres.set_dnd_uri(uri.toStdString());
                        catres.set_title(title.toStdString());
                        catres.set_art(image.toStdString());

                        data_[imglabel] = price;

                        //push the categorized result to the client
                        if (!reply->push(catres)) {
                            break; // false from push() means search waas cancelled
                        }
                    }
                    writeData();
                }
            }
            );

    QString queryUri(ALL_MARKET_URI);
    manager.get(QNetworkRequest(QUrl(queryUri)));
    loop.exec();
}
