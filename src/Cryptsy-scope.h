#ifndef DEMOSCOPE_H
#define DEMOSCOPE_H

#include <unity/scopes/ScopeBase.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/ReplyProxyFwd.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/PreviewQueryBase.h>
#include <QApplication>

using namespace unity::scopes;

class CryptsyScope : public ScopeBase
{
public:
    virtual int start(std::string const&, RegistryProxy const&) override;

    virtual void stop() override;

    PreviewQueryBase::UPtr preview(const Result&,
                                   const ActionMetadata&) override;

    virtual SearchQueryBase::UPtr search(CannedQuery const& q,
                                         SearchMetadata const&) override;
    void run() override;

private:
    QApplication *app;
};

#endif
