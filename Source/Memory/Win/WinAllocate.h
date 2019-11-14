#pragma once

namespace memory {

// ===========================================================================
// AddressSpace: reserve, release
// Page: commit, decommit
// ===========================================================================

void * ReserveAddressSpace(size_t nReservedPage);
void * ReserveAddressSpaceAndCommitPages(size_t nReservedPage);
// Also decommits all pages.
void ReleaseAddressSpace(void * pvMemBegin, size_t nReservedPage);

void  CommitPage(void * pvMemBegin, size_t nPage);
// Safe to call over uncommited pages.
void  DecommitPage(void * pvMemBegin, size_t nPage);

}
