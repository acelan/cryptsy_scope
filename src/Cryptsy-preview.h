#ifndef DEMOPREVIEW_H
#define DEMOPREVIEW_H

#include <unity/scopes/PreviewQueryBase.h>

using namespace unity::scopes;

class CryptsyPreview : public PreviewQueryBase
{
public:
    CryptsyPreview(std::string const& uri);
    ~CryptsyPreview();

    virtual void cancelled() override;
    virtual void run(PreviewReplyProxy const& reply) override;

private:
    std::string uri_;
};

#endif
