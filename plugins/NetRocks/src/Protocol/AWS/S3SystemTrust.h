#pragma once
#include <neon/ne_ssl.h>

// macOS only: verifies server certificate chain using system trust (Security.framework).
// Signature matches ne_ssl_verify_fn. userdata must point to a NUL-terminated hostname
// that outlives the ne_session.
int S3SystemTrustVerify(void *userdata, int failures, const ne_ssl_certificate *cert);
