//==========================================================================
//  AIDA Detector description implementation 
//--------------------------------------------------------------------------
// Copyright (C) Organisation europeenne pour la Recherche nucleaire (CERN)
// All rights reserved.
//
// For the licensing terms see $DD4hepINSTALL/LICENSE.
// For the list of contributors see $DD4hepINSTALL/doc/CREDITS.
//
// Author     : M.Frank
//
//==========================================================================

// Framework include files
#include <DDG4/RunMetadataStore.h>

// C/C++ include files
#include <map>
#include <memory>

using namespace dd4hep::sim;
using namespace dd4hep;

namespace {
  /// Global map of stores (one per detector instance)
  /// Using shared_ptr for automatic cleanup
  std::map<Detector*, std::shared_ptr<RunMetadataStore>> g_stores;
}

/// Get the singleton instance for a given detector
RunMetadataStore& RunMetadataStore::instance(Detector& detector) {
  auto it = g_stores.find(&detector);
  if (it == g_stores.end()) {
    auto store = std::make_shared<RunMetadataStore>();
    g_stores[&detector] = store;
    return *store;
  }
  return *it->second;
}
