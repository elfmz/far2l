#include <iostream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "powerline.hpp"

/**
 * Powerline uses Abstract Sockets. 
 * The path is "\0powerline-ipc-<UID>"
 */
static std::string get_powerline_socket_path() {
    return "powerline-ipc-" + std::to_string(getuid());
}

typedef enum {
	Unknown = -1, NotAvailable, Works
} PowerLineStatus;

static PowerLineStatus daemonResponsive = PowerLineStatus::Unknown;
static int sock = -1;
static struct sockaddr_un addr;

static PowerLineStatus connectToPowerLine() {
	if (daemonResponsive == PowerLineStatus::NotAvailable) return daemonResponsive;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1) {
    	perror("GetPowerlinePrompt::socket");
        daemonResponsive = PowerLineStatus::NotAvailable;
        return daemonResponsive;
	}

	
    std::string path = get_powerline_socket_path();
	memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

	// Abstract socket trick: The first byte of sun_path must be '\0'
	addr.sun_path[0] = '\0'; 
    strncpy(addr.sun_path + 1, path.c_str(), sizeof(addr.sun_path) - 2);

    // Length must include the leading null byte + path string length
	if (connect(sock, (struct sockaddr*)&addr, sizeof(short) + path.length() + 1) == -1) {
    	std::cerr << "GetPowerlinePrompt::Could not connect to powerline daemon. Is it running?\n";
        daemonResponsive = PowerLineStatus::NotAvailable;
        return daemonResponsive;
	}
    daemonResponsive = PowerLineStatus::Works;
	return daemonResponsive;
}

std::string GetPowerlinePrompt(const std::string& target_cwd)
{
	if(connectToPowerLine() != PowerLineStatus::Works)
		return "";

    // --- Prepare the Request ---
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));

	std::vector<std::string> cli_args = {
		"shell", "left"
	};

	std::vector<std::string> env_vars = {
		std::string("PWD=") + target_cwd, 
		"TERM=xterm-256color",
		"COLORTERM=truecolor"
	};

	std::string request;
	std::stringstream ss;
	ss << std::hex << std::setw(2) << std::setfill('0') << cli_args.size();
	request += ss.str() + '\0';
	for (const auto& arg : cli_args) {
    	request += arg + '\0';
	}
	request += target_cwd + '\0';
	for (const auto& env : env_vars) {
    	request += env + '\0';
	}
	request += '\0';

    // Send the full packet
    send(sock, request.data(), request.size(), 0);
    shutdown(sock, SHUT_WR);

    char buffer[8192];
    std::string response;
    ssize_t n;
    while ((n = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        response.append(buffer, n);
    }

    close(sock);
    return response;
}

/* Mini-parsder of colors to map it dirtectly to RGB */

static FarTrueColor index_to_FarTrueColor(int i) {
    if (i < 16) {
        // Standard 16 colors (approximate values)
        static const FarTrueColor system_colors[16] = {
            {0,0,0,1}, {128,0,0,1}, {0,128,0,1}, {128,128,0,1}, {0,0,128,1}, {128,0,128,1}, {0,128,128,1}, {192,192,192,1},
            {128,128,128,1}, {255,0,0,1}, {0,255,0,1}, {255,255,0,1}, {0,0,255,1}, {255,0,255,1}, {0,255,255,1}, {255,255,255,1}
        };
        return system_colors[i];
    } else if (i < 232) {
        // 6x6x6 Color Cube
        int r = (i - 16) / 36;
        int g = ((i - 16) % 36) / 6;
        int b = (i - 16) % 6;
        return { r ? r * 40 + 55 : 0, g ? g * 40 + 55 : 0, b ? b * 40 + 55 : 0, 1 };
    } else {
        // Grayscale ramp
        int gray = (i - 232) * 10 + 8;
        return { gray, gray, gray, 1 };
    }
}

std::vector<TextSegment> ParseColorizedText(const std::wstring& input) {
    TextState current;

    std::vector<TextSegment> v;
    
for (size_t i = 0; i < input.size(); ++i) {
        // Look for ESC [ (Wide version)
        if (input[i] == L'\x1b' && i + 1 < input.size() && input[i+1] == L'[') {
            size_t start = i + 2;
            size_t end = input.find(L'm', start);
            if (end == std::string::npos) break;

            std::wstring sequence = input.substr(start, end - start);
            std::wstringstream wss(sequence);
            std::wstring segment;
            std::vector<int> codes;

            // Split by semicolon
            while (std::getline(wss, segment, L';')) {
                if (!segment.empty()) {
                    try {
                        codes.push_back(std::stoi(segment));
                    } catch (...) { continue; }
                }
            }

            // Process SGR codes
            for (size_t c = 0; c < codes.size(); ++c) {
                int code = codes[c];
                if (code == 0) {
                    current = TextState(); // Reset to default
                } else if (code == 1) {
                    current.bold = true;
                } else if (code == 38 || code == 48) {
                    // 38 = Foreground, 48 = Background
                    bool is_fg = (code == 38);
                    
                    if (c + 2 < codes.size() && codes[c+1] == 5) {
                        // 256-color mode: ESC[38;5;Nm
                        FarTrueColor color = index_to_FarTrueColor(codes[c+2]);
                        if (is_fg) current.fg = color; else current.bg = color;
                        c += 2;
                    } else if (c + 4 < codes.size() && codes[c+1] == 2) {
                        // TrueColor mode: ESC[38;2;R;G;Bm
                        FarTrueColor color = {codes[c+2], codes[c+3], codes[c+4]};
                        if (is_fg) current.fg = color; else current.bg = color;
                        c += 4;
                    }
                }
                // Handle basic 30-37 (FG) and 40-47 (BG) if needed
                else if (code >= 30 && code <= 37) current.fg = index_to_FarTrueColor(code - 30);
                else if (code >= 40 && code <= 47) current.bg = index_to_FarTrueColor(code - 40);
            }
            i = end; // Jump past the 'm'
        } else {
           	TextSegment x { current, input[i]};
            v.push_back(x);
        }
    }
    return v;
}
