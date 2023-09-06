#include "FISHClient.h"
#include <utils.h>

static void ForkSafePrint(const char *str)
{
	if (write(STDOUT_FILENO, str, strlen(str)) <= 0) {
		perror("write");
	}
}

bool FISHClient::OpenApp(const char *app, const char *arg)
{
	if (pipe(_stderr_pipe) < 0) {
		std::cerr << "pipe failed\n";
		return false;
	}

	_pid = forkpty(&_master_fd, nullptr, nullptr, nullptr);

	if (_pid == (pid_t)-1) {
		std::cerr << "forkpty failed\n";
		return false;
	}

	if (_pid == 0) {
		// Child process
		// ***
		// Получаем текущие атрибуты терминала
		struct termios term;
		if (tcgetattr(STDIN_FILENO, &term) == -1) {
			ForkSafePrint("tcgetattr failed\n");
			_exit(-1);
		}

		// Отключаем эхо
		term.c_lflag &= ~ECHO;

		// Применяем новые атрибуты
		if (tcsetattr(STDIN_FILENO, TCSANOW, &term) == -1) {
			ForkSafePrint("tcsetattr failed\n");
			_exit(-1);
		}
		// ***

		CheckedCloseFD(_stderr_pipe[0]); // close read end
		dup2(_stderr_pipe[1], STDERR_FILENO); // redirect stderr to the pipe
		CheckedCloseFD(_stderr_pipe[1]); // close write end, as it's now duplicated
		execlp(app, app, arg, (char*) nullptr);
		ForkSafePrint("execlp failed\n");
		_exit(-1);
	}

	// Parent process
	CheckedCloseFD(_stderr_pipe[1]); // close write end

	// Now you can read from _stderr_pipe[0] to get the stderr output
	// and from _master_fd to get the stdout output

	// Учтём, что в fish_pipeopen, на основе которого написана эта функция, stderr
	// просто отбрасывается.
	// Так что мы, по идее, могли бы использовать и предыдущий (проверенный) вариант openApp(),
	// где stdout и stderr смешиваются:
	/*
		bool openApp(const char* app, const char* arg) {
			_pid = forkpty(&_master_fd, nullptr, nullptr, nullptr);

			if (_pid < 0) {
				std::cerr << "forkpty failed\n";
				return false;
			} else if (_pid == 0) {
				// Child process
				execlp(app, app, arg, (char*) nullptr);
				std::cerr << "execlp failed\n";
				return false;
			}
			return true;
		}
		*/
	return true;
}

WaitResult FISHClient::WaitFor(const std::vector<std::string>& commands, const std::vector<std::string>& expectedStrings)
{
	WaitResult result;
	result.error_code = 0;

	for (const auto& command : commands) {
		if (write(_master_fd, command.c_str(), command.length()) < 0) {
			result.error_code = errno;
			return result;
		}
	}

	struct pollfd fds[2];
	fds[0].fd = _master_fd;
	fds[0].events = POLLIN;
	fds[1].fd = _stderr_pipe[0];
	fds[1].events = POLLIN;

	std::string stdout_buffer, stderr_buffer;

	while (true) {
		int ret = poll(fds, 2, -1);
		if (ret > 0) {
			char buffer[2048];

			if (fds[0].revents & POLLIN) {
				int n = read(_master_fd, buffer, sizeof(buffer));
				if (n > 0) {
					stdout_buffer.append(buffer, n);
					//std::cout << "Debug (stdout): " << buffer << std::endl;  // ***
				}
			}

			if (fds[1].revents & POLLIN) {
				int n = read(_stderr_pipe[0], buffer, sizeof(buffer));
				if (n > 0) {
					stderr_buffer.append(buffer, n);
					//std::cout << "Debug (stderr): " << buffer << std::endl;  // ***
				}
			}

			for (size_t i = 0; i < expectedStrings.size(); ++i) {
				if (stdout_buffer.find(expectedStrings[i]) != std::string::npos) {
					result.index = i;
					result.output_type = STDOUT;
					result.stdout_data = stdout_buffer;
					result.stderr_data = stderr_buffer;
					return result;
				}

				if (stderr_buffer.find(expectedStrings[i]) != std::string::npos) {
					result.index = i;
					result.output_type = STDERR;
					result.stdout_data = stdout_buffer;
					result.stderr_data = stderr_buffer;
					return result;
				}
			}
		} else if (ret < 0) {
			result.error_code = errno;
			result.stdout_data = stdout_buffer;
			result.stderr_data = stderr_buffer;
			return result;
		}
	}
}

std::vector<FileInfo> FISHClient::ParseLs(const std::string& buffer)
{
	std::vector<FileInfo> files;
	std::istringstream stream(buffer);
	std::string line;
	FileInfo fileInfo = {0};

	while (std::getline(stream, line)) {
		if (line.empty()) continue;

		char type = line[0];
		std::string data = line.substr(1);

		// убираем \r (откуда он там берется вообще?)
		data.erase(std::remove(data.begin(), data.end(), '\r'), data.end());

		switch (type) {
			case ':': {

				if (!fileInfo.path.empty()) {
					files.push_back(fileInfo);
				}
				fileInfo = FileInfo();
					
				// Парсинг имени файла и пути символической ссылки
				size_t arrow_pos = data.find(" -> ");
				if (arrow_pos != std::string::npos) {
					fileInfo.path = data.substr(0, arrow_pos);
					fileInfo.symlink_path = data.substr(arrow_pos + 4);
				} else {
					fileInfo.path = data;
				}

				break;
			}

			case 'S':
				fileInfo.size = std::stol(data);
				break;

			// ... другие case ...

			default:
				break;
		}
	}

	if (!fileInfo.path.empty()) {
		files.push_back(fileInfo);
	}

	return files;
}

FISHClient::~FISHClient()
{
   if (_pid > 0) {
		// Проверяем, завершен ли дочерний процесс
		int status;
		if (waitpid(_pid, &status, WNOHANG) == 0) {
			// Если нет, отправляем сигнал для завершения
			kill(_pid, SIGTERM);
			waitpid(_pid, &status, 0); // Ожидаем завершения
		}
		std::cout << "Child exited with status " << status << '\n';
	}

	// Закрыть файловые дескрипторы
	CheckedCloseFD(_master_fd);
	CheckedCloseFD(_stderr_pipe[0]);
}

