#include "headers.hpp"

#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common.hpp"
#include "archive.hpp"
#include "msearch.hpp"

typedef unsigned short State;

#pragma pack(push, 1)
struct StateElement
{
	State next_state;
	StrIndex str_index;
};
#pragma pack(pop)

const unsigned c_alphabet_size = 256;

class StateMatrix
{
private:
	StateElement *data;

public:
	StateMatrix(size_t max_states)
	{
		assert(max_states <= std::numeric_limits<State>::max());
		data = new StateElement[max_states * c_alphabet_size];
		memset(data, 0, max_states * c_alphabet_size * sizeof(StateElement));
	}
	~StateMatrix() { delete[] data; }
	StateElement &at(State state, unsigned char value) { return data[state * c_alphabet_size + value]; }
	const StateElement &at(State state, unsigned char value) const
	{
		return data[state * c_alphabet_size + value];
	}
};

static std::unique_ptr<StateMatrix> create_state_matrix(const std::vector<SigData> &str_list)
{
	size_t max_states = 0;
	for (unsigned i = 0; i < str_list.size(); i++) {
		max_states += str_list[i].signature.size();
	}
	//  auto matrix = std::make_unique<StateMatrix>(max_states);
	std::unique_ptr<StateMatrix> matrix(new StateMatrix(max_states));
	State current_state = 0;
	for (StrIndex i = 0; i < str_list.size(); i++) {
		const ByteVector &str = str_list[i].signature;
		State state = 0;
		for (unsigned j = 0; j + 1 < str.size(); j++) {
			StateElement &st_elem = matrix->at(state, str[j]);
			if (st_elem.next_state) {	 // state already present
				state = st_elem.next_state;
			} else {
				current_state++;
				state = current_state;
				st_elem.next_state = state;
			}
		}
		if (str.size()) {
			matrix->at(state, str[str.size() - 1]).str_index = i + 1;
		}
	}
	return matrix;
}

std::vector<StrPos> msearch(unsigned char *data, size_t size, const std::vector<SigData> &str_list, bool eof)
{
	fprintf(stderr, "==== msearch() START ====\n");
	fprintf(stderr, "  Input buffer size: %zu\n", size);
	fprintf(stderr, "  EOF flag: %s\n", eof ? "true" : "false");
	fprintf(stderr, "  Number of signatures to search: %zu\n", str_list.size());

	fprintf(stderr, "  Buffer prefix (first 32 bytes): ");
	for (size_t k = 0; k < std::min(size, static_cast<size_t>(32)); ++k) {
		fprintf(stderr, "%02X ", data[k]);
	}
	fprintf(stderr, "\n");

#if 0
	for (size_t idx = 0; idx < str_list.size(); ++idx) {
		const auto& sig_data = str_list[idx];
		const auto& format = sig_data.format;
		if (format.ClassID == c_sqfs) {
			fprintf(stderr, "  Searching for SquashFS (idx=%zu):\n", idx);
			fprintf(stderr, "    Format name: %ls\n", format.name.c_str());
			fprintf(stderr, "    Signature offset: %u\n", format.SignatureOffset);
			fprintf(stderr, "    IsArc function pointer: %p\n", reinterpret_cast<void*>(format.IsArc));
			fprintf(stderr, "    Signature to find: ");
			for (size_t b = 0; b < sig_data.signature.size(); ++b) {
				fprintf(stderr, "%02X ", sig_data.signature[b]);
			}
			std::string sig_str(reinterpret_cast<const char*>(sig_data.signature.data()), sig_data.signature.size());
			bool is_text_sig = true;
			for (unsigned char c : sig_data.signature) {
				if (c < 32 || c > 126) {
					is_text_sig = false;
					break;
				}
			}
			if (is_text_sig) {
				fprintf(stderr, " ('%s')", sig_str.c_str());
			}
			fprintf(stderr, "\n");
		}
	}
#endif
	static bool uniq_by_type = false;
	std::vector<StrPos> result;
	if (str_list.empty())
		return result;
	result.reserve(str_list.size());
	const auto matrix = create_state_matrix(str_list);
	std::vector<bool> found(str_list.size(), false);

	size_t signatures_checked = 0;
	size_t signatures_passed_initial = 0;
	size_t signatures_checked_detailed = 0;
	size_t signatures_added_to_result = 0;

	for (size_t i = 0; i < size; i++) {
		State state = 0;
		for (size_t j = i; j < size; j++) {
			const StateElement &st_elem = matrix->at(state, data[j]);
			if (st_elem.str_index) {	// found signature
				StrIndex str_index = st_elem.str_index - 1;
				auto format = str_list[str_index].format;

				signatures_checked++;
//				fprintf(stderr, "  [msearch] Signature match at pos %zu for format idx %u (%ls)\n", i, str_index, format.name.c_str());
//				fprintf(stderr, "             Offset check: i(%zu) >= SignatureOffset(%u) is %s\n", i, format.SignatureOffset, (i >= format.SignatureOffset) ? "PASS" : "FAIL");

				if (i >= format.SignatureOffset
						&& (!uniq_by_type || !found[str_index])) {	  // more detailed check
					signatures_passed_initial++;
//					fprintf(stderr, "  [msearch] Initial checks passed for idx %u (%ls) at pos %zu\n", str_index, format.name.c_str(), i);
					UInt32 is_arc = k_IsArc_Res_YES;

					if (format.IsArc) {
//							fprintf(stderr, "  [msearch] Calling IsArc (idx %u) at pos %zu...\n", str_index, i);
						size_t check_size = size - i + format.SignatureOffset;
						is_arc = format.IsArc(data + i - format.SignatureOffset, check_size);
//							const char* res_str = "UNKNOWN";
//							if (is_arc == k_IsArc_Res_YES) res_str = "YES";
//							else if (is_arc == k_IsArc_Res_NO) res_str = "NO";
//							else if (is_arc == k_IsArc_Res_NEED_MORE) res_str = "NEED_MORE";
//							fprintf(stderr, "  [msearch] SquashFS IsArc result: %s (%u)\n", res_str, is_arc);
//							fprintf(stderr, "             Data ptr offset: %td, Check size: %zu\n", data + i - format.SignatureOffset - data, check_size);
					} else {
//							fprintf(stderr, "  [msearch] no IsArc function, assuming k_IsArc_Res_YES\n");
					}
					signatures_checked_detailed++;

					if (is_arc == k_IsArc_Res_YES || (is_arc == k_IsArc_Res_NEED_MORE && !eof)) {
						found[str_index] = true;
						result.emplace_back(StrPos(str_index, i));
						signatures_added_to_result++;
//						fprintf(stderr, "  [msearch] >>> ADDED to result: idx %u (%ls) at pos %zu\n", str_index, format.name.c_str(), i);
					} else {
//						const char* reason = (is_arc == k_IsArc_Res_NO) ? "IsArc returned NO" : "IsArc returned NEED_MORE but EOF is true";
//						fprintf(stderr, "  [msearch] DISCARDED by IsArc: idx %u (%ls) at pos %zu (%s)\n", str_index, format.name.c_str(), i, reason);
					}
				} else {
//					fprintf(stderr, "  [msearch] DISCARDED by initial checks: idx %u (%ls) at pos %zu\n", str_index, format.name.c_str(), i);
				}

			}
			if (st_elem.next_state) {
				state = st_elem.next_state;
			} else
				break;
		}
	}
	std::sort(result.begin(), result.end());
	result.shrink_to_fit();

	fprintf(stderr, "==== msearch() END ====\n");
	fprintf(stderr, "  Total signatures checked: %zu\n", signatures_checked);
	fprintf(stderr, "  Passed initial checks: %zu\n", signatures_passed_initial);
	fprintf(stderr, "  Detailed IsArc checks: %zu\n", signatures_checked_detailed);
	fprintf(stderr, "  Added to result: %zu\n", signatures_added_to_result);
	fprintf(stderr, "  Final result size: %zu\n", result.size());
	for (const auto& res : result) {
		const auto& format = str_list[res.idx].format;
		fprintf(stderr, "    Result: idx=%u, pos=%zu, format=%ls\n", res.idx, res.pos, format.name.c_str());
	}
	fprintf(stderr, "========================\n");

	return result;
}
