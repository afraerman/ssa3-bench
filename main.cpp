#define _USE_MATH_DEFINES
#include<iostream>
#include<format>
#include<string>
#include<vector>
#include<array>
#include<CLI/CLI.hpp>
#include<chrono>
#include<filesystem>
#include<map>
#include<fstream>
#include<sstream>
#include<ranges>
#include<thread>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>

#include "SRPLibrary.h"
#include "default_parameters.h"
#include "input.h"
#include "utilities.h"
#include "tests.h"

//#include<sofa.h>
//#include<sofam.h>
//#include<SpiceUsr.h>


int show_tests();
int run_all_tests();
int run_test(int stage, int test);
int debug_func();
int main_menu(int argc, char** argv);

std::map<int, std::map<int, Test>> config;



int main(int argc, char** argv)
{	


	CLI::App app{"SSA3_bench"};

 	// What parameters does the program have
    bool list  = false;
    bool quick = false;
    bool show  = false;
    bool debug = false;
    std::string run;
    std::string config_file;
    std::string out_path;

    app.add_flag("-l,--list", list, "Show tests with brief descriptions");
    app.add_flag("-q,--quick", quick, "For quick check");
    app.add_flag("-s,--show", show, "Visualize results");
    app.add_flag("-d,--debug", debug, "Enter debugging function");
    app.add_option("-r,--run", run, "Specifies the run");
    app.add_option("-c,--config", config_file, "Config file path");
    app.add_option("-o,--out", out_path, "Specifies output folder");
    
    //app.add_option("-t,--threads", threads, "Number of threads")->check(CLI::PositiveNumber);
    //app.add_option("files", input_files, "Input files")->required();

    CLI11_PARSE(app, argc, argv);
    
    int result;

    if (debug)
    {
        result = debug_func();
        return result;
    }
 	if (list)
 	{
 		result = show_tests();
 		return result;
 	}
 	if (!config_file.empty())
 	{
 		// set the config path to a new value
 	}
 	if (!out_path.empty())
 	{
 		output_folder = out_path;
 	}
 	if (run == "all")
 	{
 		result = run_all_tests();
 		return result;
 	}

 	if (!run.empty())
 	{
		const size_t point = run.find_last_of('.') + 1;          // npos + 1 == 0
		int stage = 0;
		int test = -1; // signal to run all tests of the stage

    	if (point == 0)
    	{
    		stage = std::stoi(run);
    	}
    	else
    	{
    		stage = std::stoi(run.substr(0, point-1));
    		test = std::stoi(run.substr(point));
    	}

        result = read_config_file(input_path, config);
        if (result)
            return result;

 		result = run_test(stage, test);
 		return result;
 	}

    result = main_menu(argc, argv);
    return result;
}

int show_tests()
{
	return 0;
}

int run_all_tests()
{
    int result = read_config_file(input_path, config);
    if (result)
        return 1;

    for (const auto& [stage, tests] : config) {
        for (const auto& [test, _] : tests)
        {
            run_test(stage, test);
        }
    }

	return 0;
}

int run_test(int stage, int test)
{
	if (!config.contains(stage))
    {
        std::cerr << "No stage " << stage << " in the config file" << std::endl;
        return 1;
    }
    auto tests = config[stage];

    if ((!tests.contains(test)) && (test != -1))
    {
        std::cerr << "No test " << test << " in stage " << stage << " in the config file" << std::endl;
        return 1;
    }

    std::vector<int> tests_to_run;
    if (test == -1) // run all tests of this stage
    {
        for (auto& [t, _]: tests)        
            tests_to_run.push_back(t);
    }
    else
        tests_to_run.push_back(test);

    for (auto t : tests_to_run)
    {
        Test real_test = tests[t];

        std::string time = getTime();
        create_output_folder(output_root, std::vector<std::string>{time, "test" + std::to_string(stage) + "_" + std::to_string(t)});

        switch(stage)
        {
        case 1:
            switch(t)
            {
            case 1:
                test1_1(real_test);
                break;
            case 3:
                test1_3(real_test);
                break;
            case 4:
                test1_4(real_test);
                break;
            case 5:
                test1_5(real_test);
                break;
            default:
                std::cout << "Unknown test " << t << " in stage " << stage << std::endl;
            }
            break;
        case 2:
            switch(t)
            {
            case 1:
                test2_1(real_test);
                break;
            }
            break;
        default:
            std::cout << "Unknown stage " << stage << std::endl;
        }
    }

	return 0;
}

int debug_func()
{
    return 0;
}

int main_menu(int argc, char** argv)
{
    std::cout << "Input path: " << input_path << "\n";
    std::cout << "Output folder: " << output_folder << '\n';
    // enter choice
    return 0;
}