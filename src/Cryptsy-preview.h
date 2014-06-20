#ifndef DEMOPREVIEW_H
#define DEMOPREVIEW_H

#include<unity/scopes/PreviewQueryBase.h>

class CryptsyPreview : public unity::scopes::PreviewQueryBase
{
public:
    CryptsyPreview(std::string const& uri);
    ~CryptsyPreview();

    virtual void cancelled() override;
    virtual void run(unity::scopes::PreviewReplyProxy const& reply) override;

private:
    std::string uri_;
};

#endif
