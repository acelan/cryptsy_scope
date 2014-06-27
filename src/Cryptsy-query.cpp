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
#include <QThread>

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

    for(auto i = trades_.begin(); i != trades_.end(); ++i)
    {
        QVector<Trade*> trades = i.value();
        for(auto j = trades.begin(); j != trades.end(); j++)
            delete *j;
    }
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
    out << rawdata_;
}

void CryptsyQuery::readData()
{
    QFile datafile(getDataPath() + "/data.dat");
    if(!datafile.open(QIODevice::ReadOnly)){
        return;
    }
    QDataStream in (&datafile);
    in >> rawdata_;
}

QString CryptsyQuery::getDataPath()
{
    QString path = DATA_PATH.arg(QDir::homePath());
    return path;
}

void CryptsyQuery::updateImage(QString name, bool chart=false)
{
    QString imgpath = getDataPath() + "/imgs";
    QDir imgdir(imgpath);
    if (!imgdir.exists())
    {
        imgdir.mkdir(getDataPath());
        imgdir.mkdir(imgpath);
    }

    QFileInfo fi(name);
    QString label = fi.baseName();
    const QVector<Trade*>& trades = trades_[label];

    QImage img(200,200,QImage::Format_RGB16);

    double current_price = 0;
    double previous_price = 0;
    Qt::GlobalColor bgcolor = Qt::darkBlue;
    if(trades.size() > 2)
    {
        current_price = trades[0]->price_.toDouble();
        previous_price = trades[1]->price_.toDouble();
        if(current_price - previous_price > 0.0000001)
            bgcolor = Qt::darkGreen;
        else if(previous_price - current_price > 0.0000001)
            bgcolor = Qt::darkRed;
    }

    img.fill(bgcolor);

    QPainter p(&img);
    p.setPen(QPen(Qt::white));
    if(!chart)
    {
        p.setFont(QFont("Times", 28, QFont::Bold));
        p.drawText(img.rect(), Qt::AlignCenter, QString::number(current_price));
    }
    else
    {
        QFileInfo fi(name);
        name = fi.baseName() + "_chart." + fi.suffix();
        p.setFont(QFont("Times", 16, QFont::Bold));
        p.drawText(img.rect(), Qt::AlignBottom | Qt::AlignRight, QString::number(current_price));

        double max = 0, min = 99999;
        int i = 0;
        for( i = 0 ; i < trades.size(); i++)
        {
            if(trades[i]->price_.toDouble() > max)
                max = trades[i]->price_.toDouble();
            else if(trades[i]->price_.toDouble() < min)
                min = trades[i]->price_.toDouble();
        }

        int x = 0;
        double y = 100;
        for( i = 0; i < trades.size(); i++)
        {
            int x2 = x + 200/50;
            double y2 = 100;
            if( max - min > 0.0000001)
                y2 = 160 - (trades[i]->price_.toDouble()-min)/(max-min)*150;
            p.drawLine(x, y, x2, y2);
            x = x2;
            y = y2;
        }
    }
    p.end();

    QString filename = imgpath + "/" + name;
    img.save(filename, "PNG");
}

QString CryptsyQuery::getImageFile(QString name, bool chart = false)
{
    QString imgpath = getDataPath() + "/imgs";
    QString label = name;

    if(chart)
    {
        QFileInfo fi(name);
        name = fi.baseName() + "_chart." + fi.suffix();
    }
    QString filename = imgpath + "/" + name;

    QDir imgdir(imgpath);
    if (!imgdir.exists() || !QFile(filename).exists())
        updateImage(label, chart);


    return filename;
}

void CryptsyQuery::run(SearchReplyProxy const& reply)
{
    CategoryRenderer rdr(CR_GRID);
    auto catGrid = reply->register_category("Virtual Currency", "Virtual Currency", "", rdr);

    qDebug() << __func__ << " : " << __LINE__ << " - rawdata.size() = " << rawdata_.size();

    if(rawdata_.size() == 0)
        updateData(reply, catGrid);
    reportData(reply, catGrid);
}

void CryptsyQuery::reportData(SearchReplyProxy const& reply, std::shared_ptr<const Category>& cat)
{
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(rawdata_ , &err);
        QJsonObject obj = doc.object();
        QJsonObject returnObj = obj["return"].toObject();
        QJsonObject marketsObj = returnObj["markets"].toObject();

        for( int i = 0; i < marketsObj.count(); i++) {
            CategorisedResult catres(cat);
            QJsonObject resJ = marketsObj[marketsObj.keys().at(i)].toObject();

            auto title = resJ["label"].toString();

            QString imglabel(title);
            imglabel.replace('/','_');

            QVector<Trade*> recenttrades;
            QJsonArray trades = resJ["recenttrades"].toArray();
            QString desc = "price\tquantity\ttotal\n";
            for(const auto &trade: trades)
            {
                QJsonObject tradeJ = trade.toObject();
                recenttrades.push_back(new Trade(tradeJ["id"].toString(), tradeJ["time"].toString(), tradeJ["price"].toString(), tradeJ["quantity"].toString(), tradeJ["total"].toString()));
                desc += QString::number(tradeJ["price"].toString().toDouble()) + "\t";
                desc += QString::number(tradeJ["quantity"].toString().toDouble()) + "\t";
                desc += QString::number(tradeJ["total"].toString().toDouble()) + "\n";
            }
            trades_[imglabel] = recenttrades;

            if(!query_.isEmpty() && !title.contains(query_, Qt::CaseInsensitive))
                continue;

            auto price = resJ["lasttradeprice"].toString();
            auto uri = VIEW_MARKET.arg(resJ["marketid"].toString());
            auto image = getImageFile(imglabel+".png");
            auto artist = "Market ID: " + resJ["marketid"].toString();
            auto image2 = getImageFile(imglabel+".png", true);

            //set our CateogroisedResult object with out searchresults values
            catres.set_uri(uri.toStdString());
            catres.set_dnd_uri(uri.toStdString());
            catres.set_title(title.toStdString());
            catres.set_art(image.toStdString());

            catres["description"] = Variant(desc.toStdString());
            catres["artist"] = Variant(artist.toStdString());
            catres["art2"] = Variant(image2.toStdString());

            //push the categorized result to the client
            if (!reply->push(catres)) {
                break; // false from push() means search waas cancelled
            }
        }

        // update date after shown old data
        updateData(reply, cat);
}

void CryptsyQuery::updateData(SearchReplyProxy const& reply, std::shared_ptr<const Category>& cat)
{
    QEventLoop loop;

    QNetworkAccessManager manager;
    QObject::connect(&manager, SIGNAL(finished(QNetworkReply*)), &loop, SLOT(quit()));

    QObject::connect(&manager, &QNetworkAccessManager::finished,
            [reply, cat, this](QNetworkReply *msg){
                rawdata_ = msg->readAll();
                writeData();

                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(rawdata_ , &err);
                QJsonObject obj = doc.object();
                QJsonObject returnObj = obj["return"].toObject();
                QJsonObject marketsObj = returnObj["markets"].toObject();
                for( int i = 0; i < marketsObj.count(); i++) {
                    CategorisedResult catres(cat);
                    QJsonObject resJ = marketsObj[marketsObj.keys().at(i)].toObject();

                    QString title = resJ["label"].toString();
                    QString imglabel(title);
                    imglabel.replace('/','_');
                    updateImage(imglabel+".png");
                    updateImage(imglabel+".png", true);
                }
    });

    QString queryUri(ALL_MARKET_URI);
    manager.get(QNetworkRequest(QUrl(queryUri)));
    loop.exec();
}
