// NB: this file must not include WinPort or other far2l headers:
// Security/CoreFoundation types and macros conflict with WinAPI-style definitions.
#include "S3SystemTrust.h"
#include <neon/ne_string.h>
#include <Security/Security.h>
#include <CoreFoundation/CoreFoundation.h>
#include <string>
#include <cstdio>
#include <cstdlib>

namespace
{
	template <class T>
	class CFHolder
	{
		T _ref;

	public:
		explicit CFHolder(T ref = nullptr) : _ref(ref) {}
		~CFHolder() { if (_ref) CFRelease(_ref); }
		CFHolder(const CFHolder &) = delete;
		CFHolder &operator=(const CFHolder &) = delete;

		T get() const { return _ref; }
		T *ptr() { return &_ref; }
		explicit operator bool() const { return _ref != nullptr; }
	};

	const int MAX_CHAIN_LENGTH = 16;

	SecCertificateRef NeonCertToSec(const ne_ssl_certificate *cert)
	{
		char *b64 = ne_ssl_cert_export(cert);
		if (!b64) {
			return nullptr;
		}

		unsigned char *der = nullptr;
		size_t der_len = ne_unbase64(b64, &der);
		free(b64);
		if (!der || der_len == 0) {
			if (der) ne_free(der);
			return nullptr;
		}

		CFHolder<CFDataRef> data(CFDataCreate(kCFAllocatorDefault, der, (CFIndex)der_len));
		ne_free(der);
		if (!data) {
			return nullptr;
		}

		return SecCertificateCreateWithData(kCFAllocatorDefault, data.get());
	}

	std::string CFErrorToString(CFErrorRef error)
	{
		if (!error) {
			return "unknown error";
		}
		CFHolder<CFStringRef> desc(CFErrorCopyDescription(error));
		if (!desc) {
			return "unknown error";
		}
		char buf[1024];
		if (!CFStringGetCString(desc.get(), buf, sizeof(buf), kCFStringEncodingUTF8)) {
			return "unknown error";
		}
		return buf;
	}
}

int S3SystemTrustVerify(void *userdata, int failures, const ne_ssl_certificate *cert)
{
	// Hostname mismatch, expired/not-yet-valid server cert, revocation detected by OpenSSL:
	// keep rejecting. Chain problems are re-evaluated by the system, which may build
	// a different valid chain (e.g. via cross-signed roots).
	if ((failures & ~(NE_SSL_UNTRUSTED | NE_SSL_BADCHAIN)) != 0) {
		return failures;
	}

	const char *host = (const char *)userdata;
	if (!host || !*host || !cert) {
		return failures;
	}

	CFHolder<CFMutableArrayRef> certs(CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks));
	if (!certs) {
		return failures;
	}

	int count = 0;
	for (const ne_ssl_certificate *c = cert; c; c = ne_ssl_cert_signedby(c)) {
		if (++count > MAX_CHAIN_LENGTH) {
			fprintf(stderr, "S3SystemTrust: certificate chain too long for '%s'\n", host);
			return failures;
		}
		CFHolder<SecCertificateRef> sec_cert(NeonCertToSec(c));
		if (!sec_cert) {
			fprintf(stderr, "S3SystemTrust: failed to convert certificate #%d for '%s'\n", count, host);
			return failures;
		}
		CFArrayAppendValue(certs.get(), sec_cert.get());
	}

	CFHolder<CFStringRef> cf_host(CFStringCreateWithCString(kCFAllocatorDefault, host, kCFStringEncodingUTF8));
	if (!cf_host) {
		return failures;
	}

	CFHolder<SecPolicyRef> policy(SecPolicyCreateSSL(true, cf_host.get()));
	if (!policy) {
		return failures;
	}

	CFHolder<SecTrustRef> trust;
	OSStatus status = SecTrustCreateWithCertificates(certs.get(), policy.get(), trust.ptr());
	if (status != errSecSuccess || !trust) {
		fprintf(stderr, "S3SystemTrust: SecTrustCreateWithCertificates failed (%d) for '%s'\n",
			(int)status, host);
		return failures;
	}

	CFHolder<CFErrorRef> error;
	if (!SecTrustEvaluateWithError(trust.get(), error.ptr())) {
		fprintf(stderr, "S3SystemTrust: '%s' rejected: %s\n", host, CFErrorToString(error.get()).c_str());
		return failures;
	}

	return 0;
}
