#include "Span.h"

#include "../Base/Common.h"

namespace LowLevel {

Span *
SplitSpan(Span * ps)
{
    Span * psSecond;
    size_t nPage;

    nPage = ps->nPage >> 1;

    psSecond = (Span *)((char *)ps + nPage * PAGE_SIZE);
    psSecond->nPage = nPage;

    ps->psNext = psSecond;
    ps->nPage = nPage;

    return psSecond;
}

Span *
MergeSpan(Span * psLeft, Span * psRight)
{
    ASSERT(psLeft && psRight && psLeft->nPage == psRight->nPage);

    if ((char *)psLeft + psLeft->nPage * PAGE_SIZE == (char *)psRight)
    {
        psLeft->nPage <<= 1;
        return psLeft;
    }
    else
    {
        return nullptr;
    }
}

}
