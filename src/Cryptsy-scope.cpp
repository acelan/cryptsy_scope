#include "Cryptsy-scope.h"
#include "Cryptsy-query.h"
#include "Cryptsy-preview.h"
#include <unity-scopes.h>

using namespace unity::scopes;

int CryptsyScope::start(std::string const&, RegistryProxy const&)
{
    qDebug() << __func__ << " : " << __LINE__;
    return VERSION;
}

void CryptsyScope::stop()
{
    /* The stop method should release any resources, such as network connections where applicable */
    delete app;
}

void CryptsyScope::run()
{
    // an instance of QApplication is needed to make Qt happy
    int zero = 0;
    qDebug() << __func__ << " : " << __LINE__;
    app = new QApplication(zero, nullptr);
}

SearchQueryBase::UPtr CryptsyScope::search(CannedQuery const &q,
                                           SearchMetadata const&)
{
    SearchQueryBase::UPtr query(new CryptsyQuery(q.query_string()));
    qDebug() << __func__ << " : " << __LINE__;
    return query;
}

PreviewQueryBase::UPtr CryptsyScope::preview(Result const& result, ActionMetadata const& /*metadata*/) {
    PreviewQueryBase::UPtr preview(new CryptsyPreview(result.uri()));
    qDebug() << __func__ << " : " << __LINE__;
    return preview;
}

#define EXPORT __attribute__ ((visibility ("default")))

extern "C"
{
    // cppcheck-suppress unusedFunction
    EXPORT ScopeBase* UNITY_SCOPE_CREATE_FUNCTION()
    {
        return new CryptsyScope();
    }

    // cppcheck-suppress unusedFunction
    EXPORT void UNITY_SCOPE_DESTROY_FUNCTION(ScopeBase* scope_base)
    {
        delete scope_base;
    }
}
