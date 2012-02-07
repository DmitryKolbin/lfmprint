#ifndef __LFM_PRINT
#define __LFM_PRINT

#include <string>

struct fingerprint_data {
  std::string fingerprint;
  int duration;
};

fingerprint_data getFingerprint(std::string file);
void lfmprint_init();
void lfmprint_destroy();


#endif
