#include "dc_runtime.hpp"
#include <cstdlib>
namespace dcrecomp_generated {
bool dc_ct2_wait_coalesce_enabled() noexcept {
  const char* v=std::getenv("DCR_CT2_WAIT_COALESCE");
  return v && v[0]=='1' && v[1]==0;
}
}
