#pragma once

namespace fs = std::filesystem;

#ifdef _WIN32
    using ProcessHandle = HANDLE;
    const ProcessHandle INVALID_PROCESS = nullptr;
#else
    using ProcessHandle = pid_t;
    const ProcessHandle INVALID_PROCESS = -1;
#endif

std::string cleanName(std::string name) {
    std::replace(name.begin(), name.end(), ' ', '_');
    name.erase(std::remove_if(name.begin(), name.end(), [](char c) { return c == '\n' || c == '\r'; }), name.end());
    
    size_t pos;
    while ((pos = name.find("__")) != std::string::npos) {
        name.replace(pos, 2, "_");
    }
    
    if (!name.empty() && name[0] == '_') name.erase(0, 1);
    if (!name.empty() && name.back() == '_') name.pop_back();
    return name;
}


std::string getProcessorName() {
    std::string processorName;
#ifdef _WIN32
    
    std::stringstream cmd;
    cmd << "wmic cpu get name /value";
    FILE* pipe = _popen(cmd.str().c_str(), "r");
    if (pipe) {
        char buffer[256];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }
        _pclose(pipe);
        
        size_t pos = result.find("Name=");
        if (pos != std::string::npos) {
            pos += 5; // Ïðîïóñòèòü "Name="
            size_t end = result.find("\n", pos);
            processorName = result.substr(pos, end - pos);
        }
    }
#else
    FILE* pipe = popen("grep 'model name' /proc/cpuinfo | head -1 | cut -d: -f2 | tr -d '\\n'", "r");
    if (pipe) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            processorName = std::string(buffer);
        }
        pclose(pipe);
    }
#endif
    if (processorName.empty()) {
        processorName = "Unknown_CPU";
    }
    return cleanName(processorName);
}

std::string getGpuName() {
    std::string gpuName;

#ifdef _WIN32
    // (Windows part stays the same as before)
    std::stringstream cmd;
    cmd << "wmic path win32_VideoController get name /value";
    FILE* pipe = _popen(cmd.str().c_str(), "r");
    if (pipe) {
        char buffer[512];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        _pclose(pipe);

        size_t pos = 0;
        std::vector<std::string> names;
        while ((pos = result.find("Name=", pos)) != std::string::npos) {
            pos += 5;
            size_t end = result.find_first_of("\r\n", pos);
            if (end == std::string::npos) end = result.length();
            std::string name = result.substr(pos, end - pos);

            size_t start = name.find_first_not_of(" \t");
            if (start != std::string::npos) {
                size_t finish = name.find_last_not_of(" \t");
                name = name.substr(start, finish - start + 1);
            }

            if (!name.empty() && name != "Name") {
                names.push_back(name);
            }
            pos = end;
        }

        if (!names.empty()) {
            for (const auto& n : names) {
                std::string lower = n;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.find("nvidia") != std::string::npos ||
                    lower.find("amd")    != std::string::npos ||
                    lower.find("radeon") != std::string::npos ||
                    lower.find("geforce")!= std::string::npos) {
                    gpuName = n;
                    break;
                }
            }
            if (gpuName.empty())
                gpuName = names.front();
        }
    }
#else
    // 1. Primary source – works for Intel, AMD, NVIDIA, etc.
    FILE* pipe = popen(
        "lspci 2>/dev/null | grep -iE 'vga|3d|display' | head -1 | "
        "cut -d: -f3- | sed 's/^[ \\t]*//;s/[ \\t]*$//'",
        "r");
    if (pipe) {
        char buffer[512];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            gpuName = buffer;
            if (!gpuName.empty() && gpuName.back() == '\n')
                gpuName.pop_back();
        }
        pclose(pipe);
    }

    // 2. Only try nvidia-smi when:
    //    - lspci gave us nothing, OR
    //    - lspci already indicated an NVIDIA card (to get a cleaner name)
    bool tryNvidia = gpuName.empty() ||
                     (gpuName.find("NVIDIA") != std::string::npos) ||
                     (gpuName.find("nvidia") != std::string::npos);

    if (tryNvidia) {
        FILE* nPipe = popen(
            "nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null | head -1",
            "r");
        if (nPipe) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), nPipe) != nullptr) {
                std::string nName = buffer;
                if (!nName.empty() && nName.back() == '\n')
                    nName.pop_back();

                // Only accept the result if it looks like a real GPU name
                // (reject the long error message)
                if (!nName.empty() &&
                    nName.find("NVIDIA-SMI") == std::string::npos &&
                    nName.find("failed") == std::string::npos &&
                    nName.find("couldn't communicate") == std::string::npos) {
                    gpuName = nName;
                }
            }
            pclose(nPipe);
        }
    }
#endif

    if (gpuName.empty()) {
        gpuName = "Unknown_GPU";
    }
    return cleanName(gpuName);
}

std::string getPcName() {
    std::string systemName;

#ifdef _WIN32
    FILE* pipe = _popen("wmic computersystem get model /value", "r");
    if (pipe) {
        char buffer[256];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        _pclose(pipe);

        size_t pos = result.find("Model=");
        if (pos != std::string::npos) {
            pos += 6; // skip "Model="
            size_t end = result.find_first_of("\r\n", pos);
            if (end == std::string::npos) end = result.length();
            systemName = result.substr(pos, end - pos);

            // trim
            size_t start = systemName.find_first_not_of(" \t");
            if (start != std::string::npos) {
                size_t finish = systemName.find_last_not_of(" \t");
                systemName = systemName.substr(start, finish - start + 1);
            }
        }
    }
#else
    // No root required on modern kernels
    FILE* pipe = popen("cat /sys/class/dmi/id/product_name 2>/dev/null", "r");
    if (pipe) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            systemName = buffer;
            if (!systemName.empty() && systemName.back() == '\n')
                systemName.pop_back();
        }
        pclose(pipe);
    }

    // Fallback
    if (systemName.empty()) {
        FILE* pipe2 = popen("cat /sys/class/dmi/id/board_name 2>/dev/null", "r");
        if (pipe2) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), pipe2) != nullptr) {
                systemName = buffer;
                if (!systemName.empty() && systemName.back() == '\n')
                    systemName.pop_back();
            }
            pclose(pipe2);
        }
    }
#endif

    if (systemName.empty()) {
        systemName = "Unknown_System";
    }
    return cleanName(systemName);
}

std::string getOsName() {
    std::string osName;

#ifdef _WIN32
    // Caption gives a nice human-readable string: "Microsoft Windows 11 Pro"
    FILE* pipe = _popen("wmic os get Caption /value", "r");
    if (pipe) {
        char buffer[256];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        _pclose(pipe);

        size_t pos = result.find("Caption=");
        if (pos != std::string::npos) {
            pos += 8; // skip "Caption="
            size_t end = result.find_first_of("\r\n", pos);
            if (end == std::string::npos) end = result.length();
            osName = result.substr(pos, end - pos);

            // trim
            size_t start = osName.find_first_not_of(" \t");
            if (start != std::string::npos) {
                size_t finish = osName.find_last_not_of(" \t");
                osName = osName.substr(start, finish - start + 1);
            }
        }
    }
#else
    // Prefer /etc/os-release (standard on modern Linux)
    FILE* pipe = popen(
        "grep -E '^(PRETTY_NAME|NAME)=' /etc/os-release 2>/dev/null | "
        "head -1 | cut -d= -f2- | tr -d '\"'",
        "r");
    if (pipe) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            osName = buffer;
            if (!osName.empty() && osName.back() == '\n')
                osName.pop_back();
        }
        pclose(pipe);
    }

    // Fallback for older systems
    if (osName.empty()) {
        FILE* pipe2 = popen("lsb_release -ds 2>/dev/null | tr -d '\"'", "r");
        if (pipe2) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), pipe2) != nullptr) {
                osName = buffer;
                if (!osName.empty() && osName.back() == '\n')
                    osName.pop_back();
            }
            pclose(pipe2);
        }
    }

    // Last resort
    if (osName.empty()) {
        FILE* pipe3 = popen("uname -srm 2>/dev/null", "r");
        if (pipe3) {
            char buffer[128];
            if (fgets(buffer, sizeof(buffer), pipe3) != nullptr) {
                osName = buffer;
                if (!osName.empty() && osName.back() == '\n')
                    osName.pop_back();
            }
            pclose(pipe3);
        }
    }
#endif

    if (osName.empty()) {
        osName = "Unknown_OS";
    }
    return cleanName(osName);
}

std::string getGpuDriverVersion() {
    std::string driverVersion;

#ifdef _WIN32
    FILE* pipe = _popen("wmic path win32_VideoController get DriverVersion /value", "r");
    if (pipe) {
        char buffer[256];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        _pclose(pipe);

        // Prefer the first non-empty DriverVersion (usually the discrete GPU)
        size_t pos = 0;
        while ((pos = result.find("DriverVersion=", pos)) != std::string::npos) {
            pos += 14;
            size_t end = result.find_first_of("\r\n", pos);
            if (end == std::string::npos) end = result.length();
            std::string ver = result.substr(pos, end - pos);

            // trim
            size_t start = ver.find_first_not_of(" \t");
            if (start != std::string::npos) {
                size_t finish = ver.find_last_not_of(" \t");
                ver = ver.substr(start, finish - start + 1);
            }

            if (!ver.empty() && ver != "DriverVersion") {
                driverVersion = ver;
                break;          // take the first one
            }
            pos = end;
        }
    }
#else
    // Prefer NVIDIA (most common case when people ask for driver version)
    FILE* pipe = popen("nvidia-smi --query-gpu=driver_version --format=csv,noheader 2>/dev/null | head -1", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            driverVersion = buffer;
            if (!driverVersion.empty() && driverVersion.back() == '\n')
                driverVersion.pop_back();
        }
        pclose(pipe);
    }

    // Generic fallback (AMD / Intel / Nouveau)
    if (driverVersion.empty()) {
        FILE* pipe2 = popen(
            "lspci -k 2>/dev/null | grep -A2 -iE 'vga|3d|display' | grep 'Kernel driver in use' | head -1 | cut -d: -f2 | tr -d ' \\n'",
            "r");
        if (pipe2) {
            char buffer[128];
            if (fgets(buffer, sizeof(buffer), pipe2) != nullptr) {
                driverVersion = buffer;   // just the module name, e.g. "amdgpu" / "i915"
            }
            pclose(pipe2);
        }
    }
#endif

    if (driverVersion.empty()) {
        driverVersion = "Unknown_Driver";
    }
    return cleanName(driverVersion);
}

std::string getCudaVersion() {
    std::string cudaVersion;

    // nvidia-smi reports the highest CUDA version the *driver* supports.
    // This is the value most people want.
#ifdef _WIN32
    FILE* pipe = _popen("nvidia-smi 2>nul", "r");
#else
    FILE* pipe = popen("nvidia-smi 2>/dev/null", "r");
#endif
    if (pipe) {
        char buffer[512];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
#ifdef _WIN32
        _pclose(pipe);
#else
        pclose(pipe);
#endif

        // Look for the line that contains "CUDA Version: XX.X"
        size_t pos = output.find("CUDA Version:");
        if (pos != std::string::npos) {
            pos += 13; // skip "CUDA Version:"
            // skip spaces
            while (pos < output.size() && (output[pos] == ' ' || output[pos] == '\t'))
                ++pos;

            size_t end = pos;
            while (end < output.size() && (isdigit(output[end]) || output[end] == '.'))
                ++end;

            if (end > pos) {
                cudaVersion = output.substr(pos, end - pos);
            }
        }
    }

    // Optional: also try the toolkit version (nvcc) if the user has CUDA Toolkit installed
    if (cudaVersion.empty()) {
#ifdef _WIN32
        FILE* pipe2 = _popen("nvcc --version 2>nul", "r");
#else
        FILE* pipe2 = popen("nvcc --version 2>/dev/null", "r");
#endif
        if (pipe2) {
            char buffer[512];
            std::string output;
            while (fgets(buffer, sizeof(buffer), pipe2) != nullptr) {
                output += buffer;
            }
#ifdef _WIN32
            _pclose(pipe2);
#else
            pclose(pipe2);
#endif

            size_t pos = output.find("release ");
            if (pos != std::string::npos) {
                pos += 8;
                size_t end = pos;
                while (end < output.size() && (isdigit(output[end]) || output[end] == '.'))
                    ++end;
                if (end > pos) {
                    cudaVersion = output.substr(pos, end - pos);
                }
            }
        }
    }

    if (cudaVersion.empty()) {
        cudaVersion = "N/A";          // no NVIDIA / no CUDA
    }
    return cleanName(cudaVersion);
}

std::string getTime()
{
	auto now = std::chrono::system_clock::now();
 	auto local_time = std::chrono::current_zone()->to_local(now);
 	auto seconds_only = std::chrono::floor<std::chrono::seconds>(local_time);
 	std::string current_time = std::format("{:%Y%m%d_%H%M%S}", seconds_only);
 	return current_time;
}

int make_passport()
{
	std::string time = getTime();
	std::string node_name; // ????
	std::string pc = getPcName();
	std::string os = getOsName();
	std::string processor = getProcessorName();
	std::string gpu = getGpuName();
	std::string driver_version = getGpuDriverVersion();
	std::string cuda_version = getCudaVersion();
	bool rtx_available = is_rtx_device_available();
	std::string hash = GIT_COMMIT_HASH;

	std::ofstream passport(output_folder + "/passport.txt");
	if (!passport.is_open())
	{
		std::cerr << "Can't create passport file " << output_folder + "/passport.txt" << std::endl;
		return 1;
	}

	passport << "Time   : " << time << '\n';
	passport << "OS     : " << os << '\n';
	passport << "PC     : " << pc << '\n';
	passport << "CPU    : " << processor << '\n';
	passport << "GPU    : " << gpu << '\n';
	passport << "Driver : " << driver_version << '\n';
	passport << "CUDA   : " << cuda_version << '\n';
	passport << "RTX On : " << rtx_available << '\n';

	// Harware info
	std::streambuf* originalCoutBuffer = std::cout.rdbuf();
    std::cout.rdbuf(passport.rdbuf());
    print_hardware_info();
    std::cout.rdbuf(originalCoutBuffer);

	passport.close();
	return 0;
}

int create_output_folder(std::string out_path, std::vector<std::string> specifications)
{
	fs::path dir_path = out_path + "/" + specifications[0];
	for (int i = 1; i < specifications.size(); i++)
	{
		dir_path += "_" + specifications[i];
	}
	output_folder = dir_path;
	try {
        // create_directories
        if (fs::create_directories(dir_path)) {
            std::cout << "Folder created: " << dir_path << std::endl;
        } else {
            std::cout << "Folder with such name already exists or can not be created: " << dir_path << std::endl;
        }

        make_passport();

        return 0;
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return 1;
    }
}

ProcessHandle start_gpu_logger(const std::string& log_path, int interval_ms = 100)
{

#ifdef _WIN32
    // ---------- Windows ----------
    std::string cmd =
        "nvidia-smi "
        "--query-gpu=timestamp,index,pstate,clocks.sm,clocks.mem,power.draw,temperature.gpu,utilization.gpu "
        "--format=csv,nounits "
        "-lms " + std::to_string(interval_ms) + " "
        "-f \"" + log_path + "\"";

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    BOOL success = CreateProcessA(
        nullptr,
        cmd.data(),
        nullptr, nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr, nullptr,
        &si, &pi
    );

    if (!success) {
        std::cerr << "CreateProcess failed, error: " << GetLastError() << std::endl;
        return INVALID_PROCESS;
    }

    CloseHandle(pi.hThread);   // поток нам не нужен
    Sleep(50);
    return pi.hProcess;

#else
    // ---------- Linux ----------
    // Запускаем nvidia-smi НАПРЯМУЮ, без оболочки
    pid_t pid = fork();
    if (pid == 0) {
        // Дочерний процесс
        std::string interval_str = std::to_string(interval_ms);

        execlp("nvidia-smi",
               "nvidia-smi",
               "--query-gpu=timestamp,index,pstate,clocks.sm,clocks.mem,power.draw,temperature.gpu,utilization.gpu",
               "--format=csv,nounits",
               "-lms", interval_str.c_str(),
               "-f", log_path.c_str(),
               (char*)nullptr);

        // Если execlp не удался
        std::cerr << "execlp(nvidia-smi) failed: " << strerror(errno) << std::endl;
        _exit(1);
    }
    else if (pid > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return pid;
    }
    else {
        std::cerr << "fork() failed\n";
        return INVALID_PROCESS;
    }
#endif
}

void stop_gpu_logger(ProcessHandle handle)
{
    if (handle == INVALID_PROCESS) return;

#ifdef _WIN32
    // ---------- Windows ----------
    TerminateProcess(handle, 0);
    WaitForSingleObject(handle, 3000);   // ждём до 3 секунд
    CloseHandle(handle);
#else
    // ---------- Linux ----------
    // Теперь handle — это PID самого nvidia-smi
    kill(handle, SIGINT);   // мягко

    int status = 0;
    for (int i = 0; i < 30; ++i) {   // ~3 секунды
        if (waitpid(handle, &status, WNOHANG) == handle) {
            return; // успешно завершился
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Если не остановился — убиваем
    kill(handle, SIGKILL);
    waitpid(handle, &status, 0);
#endif
}