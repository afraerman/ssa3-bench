#pragma once

void strip_in_place(std::string& s) {
    // Удаляем с конца
    size_t end = s.find_last_not_of(" \n\r\t\f\v");
    if (end != std::string::npos) {
        s.erase(end + 1);
    } else {
        s.clear(); // Строка состоит только из пробелов
        return;
    }

    // Удаляем с начала
    size_t start = s.find_first_not_of(" \n\r\t\f\v");
    if (start != std::string::npos) {
        s.erase(0, start);
    }
}

struct Test
{
    std::vector<std::string> models;
    std::vector<std::string> models_dirs;

    std::vector<int> reflections;
    std::vector<std::string> methods;
    double tol_moment;
    double tol_force;
    std::vector<int> sun_directions;
    int calls;
    bool quick;
    double grid;
    std::vector<double> grids;
    std::vector<double> pauses;
};

int read_config_file(std::string filename, std::map<int, std::map<int, Test>>& config)
{
    int current_stage = 0;
    int current_test = 0;
    bool general = false;
    std::string data_root = "";

    std::ifstream input_file(filename);
    if (!input_file)
    {
        std::cerr << "unable to open " << filename << std::endl;
        return 1;
    }

    for (std::string line; getline(input_file, line);)
    {
        if (line.starts_with("[general"))
        {
            general = true;
            continue;
        }

        if (line.starts_with("[test"))
        {
            const size_t point_pos = line.find('.');
            const size_t _  = line.find('_') + 1;
            const size_t end = line.find(']');
            
            current_stage = std::stoi(line.substr(_, point_pos - _));
            current_test = std::stoi(line.substr(point_pos + 1, end - point_pos - 1));
            Test new_test;

            auto stage = config.find(current_stage);
            if (stage != config.end())
            {
                config[current_stage].insert_or_assign(current_test, new_test);
            }
            else
            {
                std::map<int, Test> new_stage;
                new_stage.insert_or_assign(current_test, new_test);
                config.insert_or_assign(current_stage, new_stage);
            }
            continue;
        }

        auto pos = line.find('=');
        if (pos == std::string::npos)
        {
            general = false;
            continue;
        }
        auto key = line.substr(0, pos);
        strip_in_place(key);
        const size_t comment_start = line.find('#');
        std::string val;
        if (comment_start != std::string::npos)
            { val = line.substr(pos + 1, comment_start - pos - 1); }
        else
            { val = line.substr(pos + 1); }

        strip_in_place(val);

        if (general)
        {
            if (key == "data_root")
            {
                strip_in_place(val);
                data_root = val;
            }

            if (key == "results_root")
            {
                strip_in_place(val);
                output_root = val;
            }
            continue;
        }

        if (key == "models")
        {
            std::vector<std::string> models;
            
            std::stringstream ss(val);
            std::string token;
            
            // Read tokens separated by a comma
            while (std::getline(ss, token, ','))
            {
                strip_in_place(token);
                models.push_back(data_root + "/" + token);
            }
            config[current_stage][current_test].models = models;
            continue;
        }

        if (key == "models_dirs")
        {
            std::vector<std::string> models_dirs;
            std::stringstream ss(val);
            std::string token;

            while (std::getline(ss, token, ','))
            {
                strip_in_place(token);
                models_dirs.push_back(token);
            }
            config[current_stage][current_test].models_dirs = models_dirs;
            continue;
        }

        if (key == "reflections")
        {
            std::vector<int> reflections;
            std::stringstream ss(val);
            std::string token;

            while (std::getline(ss, token, ','))
            {
                strip_in_place(token);
                reflections.push_back(std::stoi(token));
            }
            config[current_stage][current_test].reflections = reflections;
            continue;
        }

        if (key == "methods")
        {
            std::vector<std::string> methods;
            std::stringstream ss(val);
            std::string token;

            while (std::getline(ss, token, ','))
            {
                strip_in_place(token);
                methods.push_back(token);
            }
            config[current_stage][current_test].methods = methods;
            continue;
        }

        if (key == "tol_moment")
        {
            double tol_moment = std::stod(val);
            config[current_stage][current_test].tol_moment = tol_moment;
            continue;
        }

        if ((key == "tol_force") || (key == "tol_force_rel"))
        {
            double tol_force = std::stod(val);
            config[current_stage][current_test].tol_force = tol_force;
            continue;
        }

        if ((key == "sun_directions" ) || (key == "directions"))
        {
            std::vector<int> directions;
            std::stringstream ss(val);
            std::string token;

            while (std::getline(ss, token, ','))
            {
                strip_in_place(token);
                directions.push_back(std::stoi(token));
            }
            config[current_stage][current_test].sun_directions = directions;
            continue;
        }

        if (key == "calls")
        {
            config[current_stage][current_test].calls = std::stoi(val);
            continue;
        }

        if (key == "grid")
        {
            config[current_stage][current_test].grid = std::stod(val);
            continue;
        }

        if (key == "grids")
        {
            std::vector<double> grids;
            std::stringstream ss(val);
            std::string token;

            while (std::getline(ss, token, ','))
            {
                strip_in_place(token);
                grids.push_back(std::stod(token));
            }
            config[current_stage][current_test].grids = grids;
            continue;
        }

        if (key == "pauses")
        {
            std::vector<double> pauses;
            std::stringstream ss(val);
            std::string token;

            while (std::getline(ss, token, ','))
            {
                strip_in_place(token);
                pauses.push_back(std::stod(token));
            }
            config[current_stage][current_test].pauses = pauses;
            continue;
        }
    }
    return 0;
}