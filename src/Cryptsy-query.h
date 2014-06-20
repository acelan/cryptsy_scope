#ifndef DEMOQUERY_H
#define DEMOQUERY_H

#include <unity/scopes/SearchQueryBase.h>
#include <unity/scopes/ReplyProxyFwd.h>

#include <QApplication>
#include <QNetworkReply>
#include <QVector>
#include <QThread>

using namespace unity::scopes;

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

class CryptsyQuery : public SearchQueryBase
{
public:
    CryptsyQuery(std::string const& query);
    ~CryptsyQuery();
    virtual void cancelled() override;

    virtual void run(SearchReplyProxy const& reply) override;

    QString getImageFile(QString name);
    void updateImage(QString name);
    QString getDataPath();
    void writeData();
    void readData();

protected:
    void reportData(SearchReplyProxy const& reply, std::shared_ptr<const Category>& cat);
    void updateData(SearchReplyProxy const& reply, std::shared_ptr<const Category>& cat);

private:
    QByteArray rawdata_;
    //QMap<QString,QByteArray> data_;
    QMap<QString,QVector<Trade*> > trades_;
    QString query_;

};

#endif
