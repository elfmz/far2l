#pragma once
#include <string>
#include <vector>

namespace VTLog
{
	// Начинает и останавливает логирование. Теперь они просто создают/удаляют менеджер.
	void Start();
	void Stop();

	// Приостанавливает и возобновляет добавление строк в лог.
	void Pause();
	void Resume();

	// Вызывается при присоединении к существующей консоли.
	void ConsoleJoined(HANDLE con_hnd);

	// Очищает лог для указанной консоли.
	void Reset(HANDLE con_hnd);

	// Добавляет новую логическую строку в историю.
	void AddLogicalLine(HANDLE con_hnd, const std::vector<CHAR_INFO>& line);

	// Формирует файл из истории вывода.
	std::string GetAsFile(HANDLE con_hnd, bool colored, bool append_screen_lines = true, const char *wanted_path = nullptr);
}
