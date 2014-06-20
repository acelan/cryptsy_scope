#ifndef DEMOQUERY_H
#define DEMOQUERY_H

#include <unity/scopes/SearchQueryBase.h>
#include <unity/scopes/ReplyProxyFwd.h>

#include <QCoreApplication>
#include <QNetworkReply>
#include <QVector>

class Trade
{
public:
    Trade(QString id, QString time, QString price, QString quantity, QString total): id_(id), time_(time), price_(price), quantity_(quantity), total_(total) {}
    QString id_;
    QString time_;
    QString price_;
    QString quantity_;
    QString total_;
private:
    Trade() {}
};

class CryptsyQuery : public unity::scopes::SearchQueryBase
{
public:
    CryptsyQuery(std::string const& query);
    ~CryptsyQuery();
    virtual void cancelled() override;

    virtual void run(unity::scopes::SearchReplyProxy const& reply) override;

    QString getImageFile(QString name);
    QString getDataPath();
    void writeData();
    void readData();

private:
    QMap<QString,QString> data_;
    QMap<QString,QVector<Trade*> > trades_;
    QString query_;
};

#endif
