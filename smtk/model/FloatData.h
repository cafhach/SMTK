#ifndef __smtk_model_FloatData_h
#define __smtk_model_FloatData_h

#include "smtk/SystemConfig.h"

#include "smtk/common/UUID.h"

#include "sparsehash/sparse_hash_map"

#include <string>
#include <vector>

namespace smtk {
  namespace model {

    typedef double Float;
    typedef std::vector<Float> FloatList;
    typedef google::sparse_hash_map<std::string,FloatList> FloatData;
    typedef google::sparse_hash_map<smtk::common::UUID,FloatData> UUIDsToFloatData;

    typedef UUIDsToFloatData::iterator UUIDWithFloatProperties;
    typedef FloatData::iterator PropertyNameWithFloats;
    typedef FloatData::const_iterator PropertyNameWithConstFloats;

  } // namespace model
} // namespace smtk

#endif // __smtk_model_FloatData_h
