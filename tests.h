#pragma once

double getModule(std::array<double, 3> vec)
{
    return std::sqrt(vec[0]*vec[0] + vec[1]*vec[1] * vec[2]*vec[2]);
}

double getScalar(std::array<double, 3> vec1, std::array<double, 3> vec2)
{
    return vec1[0] * vec2[0] + vec1[1] * vec2[1] + vec1[2] * vec2[2];
}

bool compare(std::array<double, 3> v1, std::array<double, 3> v2, double threshold)
{
    return (std::fabs(getModule(v1) - getModule(v2)) / getModule(v2)) < threshold;
}

int test1_1(Test test)
{
	SRPEngine engine(test.models[0]);
    SRPResult result, result_rtx, result_cuda, result_cpu;
    double force_module, true_force_module;
    double momentum_module, true_momentum_module;
    double force_direction;
    std::vector<double> true_force;
    bool test_passed = true;

    std::ofstream verdict(output_folder + "/verdict.txt");
    if (!verdict.is_open())
    {
    	std::cerr << "Can't create verdict " << output_folder + "/verdict.txt" << std::endl;
    	return 1;
    }
    verdict << "Тест 1.1 ";

    // -------------------- DEFAULT VALUES TO TEST ----------------//
    std::vector<std::vector<double>> solar_directions{{0.0, 0.0, 1.0}, {0.866025, 0.0, 0.5}, {0.0, 0.0, -1.0}};
    std::vector<std::vector<double>> true_forces{{0.0, 0.0, -8.879727}, {-2.737537, 0, -2.665780}, {0, 0, 8.879727}};
    std::vector<double> true_force_modules{8.879727, 3.821059, 8.879727};
    std::vector<double> true_momentum_modules{0.0, 0.0, 0.0};

    // -----------------------------------------------------------//

    for (int k = 0; k < (int)solar_directions.size(); k++)
    {
        if (!test_passed) break;
        engine.setSunDirection(solar_directions[k][0], solar_directions[k][1], solar_directions[k][2]);
        true_force = true_forces[k];
        true_force_module = true_force_modules[k];
        true_momentum_module = true_momentum_modules[k];

        for (int j = 0; j < (int)test.reflections.size(); ++j)
        {
            auto refl = test.reflections[j];
            engine.setMaxReflections(refl);

            // -------------------- RTX ---------------- //
            force_module = 0.0;
            force_direction = 0.0;
            momentum_module = 0.0;
            if (is_rtx_device_available())
            {
                result = engine.compute(SRPMethod::CentroidRTX);
                for (int i = 0; i < 3; i++)
                {
                    force_module += result.total_force[i] * result.total_force[i];
                    momentum_module += result.total_moment[i] * result.total_moment[i];
                    
                    force_direction += result.total_force[i] * true_force[i];
                }
                force_module = std::sqrt(force_module);
                momentum_module = std::sqrt(momentum_module);

                if (std::fabs(force_module - true_force_module) >= test.tol_force)
                {
                    verdict << "НЕ ПРОЙДЕН: расхождение модуля силы в методе CentroidRTX больше " << test.tol_force << '\n';
                    test_passed = false;
                    break;
                }

                if (std::fabs(momentum_module - true_momentum_module) >= test.tol_moment)
                {
                    verdict << "НЕ ПРОЙДЕН: расхождение модуля момента в методе CentroidRTX больше " << test.tol_moment << '\n';
                    test_passed = false;
                    break;
                }

                if (force_direction < 0.0)
                {
                    verdict << "НЕ ПРОЙДЕН: не совпадает напраление силы в методе CentroidRTX\n";
                    break;
                }

                if (j)
                {
                    if (!((result.total_force[0] == result_rtx.total_force[0]) && (result.total_force[1] == result_rtx.total_force[1]) && (result.total_force[2] == result_rtx.total_force[2])))
                    {
                        verdict << "НЕ ПРОЙДЕН: несовпадение результатов в методе CentroidRTX при " << test.reflections[j-1] << " and " << refl << " отражениях\n";
                        test_passed = false;
                        break;
                    }
                }
                result_rtx = result;

            }
            
            // -------------------- GPU ---------------- //
            force_module = 0.0;
            momentum_module = 0.0;
            force_direction = 0.0;
            if (is_cuda_device_available())
            {
                result = engine.compute(SRPMethod::CentroidGPU);
                for (int i = 0; i < 3; i++)
                {
                    force_module += result.total_force[i] * result.total_force[i];
                    momentum_module += result.total_moment[i] * result.total_moment[i];

                    force_direction += result.total_force[i] * true_force[i];
                }
                force_module = std::sqrt(force_module);
                momentum_module = std::sqrt(momentum_module);
                
                if (std::fabs(force_module - true_force_module) >= test.tol_force)
                {
                    verdict << "НЕ ПРОЙДЕН: расхождение модуля силы в методе CentroidGPU больше " << test.tol_force << '\n';
                    test_passed = false;
                    break;
                }

                if (std::fabs(momentum_module - true_momentum_module) >= test.tol_moment)
                {
                    verdict << "НЕ ПРОЙДЕН: расхождение модуля момента в методе CentroidGPU больше " << test.tol_moment << '\n';
                    test_passed = false;
                    break;
                }

                if (force_direction < 0.0)
                {
                    verdict << "НЕ ПРОЙДЕН: не совпадает напраление силы в методе CentroidGPU\n";
                    break;
                }

                if (j)
                {
                    if (!((result.total_force[0] == result_cuda.total_force[0]) && (result.total_force[1] == result_cuda.total_force[1]) && (result.total_force[2] == result_cuda.total_force[2])))
                    {
                        verdict << "НЕ ПРОЙДЕН: несовпадение результатов в методе CentroidGPU при " << test.reflections[j-1] << " and " << refl << " отражениях\n";
                        test_passed = false;
                        break;
                    }
                }
                result_cuda = result;
            }

            // -------------------- CPU ---------------- //
            force_module = 0.0;
            momentum_module = 0.0;
            force_direction = 0.0;
            result = engine.compute(SRPMethod::CentroidCPU);
            for (int i = 0; i < 3; i++)
            {
                force_module += result.total_force[i] * result.total_force[i];
                momentum_module += result.total_moment[i] * result.total_moment[i];

                force_direction += result.total_force[i] * true_force[i];
            }
            force_module = std::sqrt(force_module);
            momentum_module = std::sqrt(momentum_module);
            
            if (std::fabs(force_module - true_force_module) >= test.tol_force)
            {
                verdict << "НЕ ПРОЙДЕН: расхождение модуля силы в методе CentroidCPU больше " << test.tol_force << '\n';
                test_passed = false;
                break;
            }

            if (std::fabs(momentum_module - true_momentum_module) >= test.tol_moment)
            {
                verdict << "НЕ ПРОЙДЕН: расхождение модуля момента в методе CentroidCPU больше " << test.tol_moment << '\n';
                test_passed = false;
                break;
            }

            if (force_direction < 0.0)
            {
                verdict << "НЕ ПРОЙДЕН: не совпадает напраление силы в методе CentroidCPU\n";
                break;
            }


            if (j)
            {
                if (!((std::fabs(result.total_force[0] - result_cpu.total_force[0]) < 1e-10) && (std::fabs(result.total_force[1] - result_cpu.total_force[1]) < 1e-10) && (std::fabs(result.total_force[2] - result_cpu.total_force[2]) < 1e-10)))
                {
                    verdict << "НЕ ПРОЙДЕН: несовпадение результатов в методе CentroidCPU при " << test.reflections[j-1] << " and " << refl << " отражениях\n";
                    test_passed = false;
                    break;
                }
            }
            result_cpu = result;
        }
    }

    if (test_passed)
    {
        verdict << "ПРОЙДЕН по критериям:\n";
        verdict << "  - отколнение модуля силы от табличного значения не превосходит " << test.tol_force << '\n';
        verdict << "  - отколнение модуля момента от табличного значения не превосходит " << test.tol_moment << '\n';
        verdict << "  - значения при всех количествах отражения совпадают\n";
    }

    verdict.close();
    return 0;
}

int test1_3(Test test)
{
    std::ofstream verdict(output_folder + "/verdict.txt");
    if (!verdict.is_open())
    {verdict << " - расхождение по углу силы не превосходит " << test.tol_force << '\n';
        std::cerr << "Can't create verdict " << output_folder + "/verdict.txt" << '\n';
        return 1;
    }
    verdict << "Тест 1.3 ";

    SRPResult result_cpu, result_rtx, result_gpu;
    bool test_passed = true;
    
    std::array<double, 3> f, m, f0, m0;
    double m_mod, m0_mod, f_mod, f0_mod;
    double force_deviation, force_angle;
    double momentum_deviation, momentum_angle;
    double momentum_force_deviation;

    for (auto model : test.models)
    {
        if (!test_passed) break;
        SRPEngine engine(model);
        for (auto refl: test.reflections)
        {
            if (!test_passed) break;
            engine.setMaxReflections(refl);
            for (int sun_idx = 1; sun_idx <= 20; sun_idx++)
            {
                engine.setSunDirection(sun_directions[sun_idx][2], sun_directions[sun_idx][3], sun_directions[sun_idx][4]);

                if (is_rtx_device_available())
                {
                    result_rtx = engine.compute(SRPMethod::CentroidRTX);
                    f = result_rtx.total_force;
                    m = result_rtx.total_moment;
                    f_mod = getModule(result_rtx.total_force);
                    m_mod = getModule(result_rtx.total_moment);
                }
                else if (is_cuda_device_available())
                {
                    result_gpu = engine.compute(SRPMethod::CentroidGPU);
                    f = result_gpu.total_force;
                    m = result_gpu.total_moment;
                    f_mod = getModule(result_gpu.total_force);
                    m_mod = getModule(result_gpu.total_moment);
                }
                else
                {
                    test_passed = false;
                    verdict << "НЕ ПРОЙДЕН: отсутвтует CUDA/RTX\n";
                    break;
                }

                result_cpu = engine.compute(SRPMethod::CentroidCPU);

                f0 = result_cpu.total_force;
                m0 = result_cpu.total_moment;
                f0_mod = getModule(result_cpu.total_force);
                m0_mod = getModule(result_cpu.total_moment);

                force_deviation = std::fabs(f_mod - f0_mod) / f0_mod * 100.0;
                force_angle = std::acos(getScalar(f, f0) / (f_mod * f0_mod)) * 180.0 / M_PI * 1000.0;
                momentum_deviation = std::fabs(m_mod - m0_mod) / std::max(std::max(m0_mod, m_mod), f0_mod * 0.001) * 100.0;
                momentum_angle = std::acos(getScalar(m, m0) / (m_mod * m0_mod)) * 180.0 / M_PI / 1000.0;
                momentum_force_deviation = std::fabs(getModule(std::array<double, 3>{m[0] - m0[0], m[1] - m0[1], m[2] - m0[2]})) / f0_mod * 1000.0;

                if (force_deviation > test.tol_force)
                {
                    test_passed = false;
                    verdict << "НЕ ПРОЙДЕН: расхождение модуля силы\n";
                    verdict << "  - при числе отражений " << refl << "\n";
                    verdict << "  - модели " << model << "\n";
                    verdict << "  - направлении на Солнце " << sun_directions[sun_idx][2] << " " <<  sun_directions[sun_idx][3] << " " <<  sun_directions[sun_idx][4] << '\n';
                    break;
                }
            }
        }
    }

    if (test_passed)
    {
        verdict << "ПРОЙДЕН по критериям:\n";
        verdict << " - расхождение по модулю силы не превосходит " << test.tol_force << '\n';
        //verdict << " - расхождение по модулю момента не превосходит " << test.tol_force << '\n';
        //verdict << " - расхождение по углу силы не превосходит " << test.tol_force << '\n';
        //verdict << " - расхождение по углу момента не превосходит " << test.tol_force << '\n';
    }

    verdict.close();
    return !test_passed;
}

int test1_4(Test test)
{
    std::ofstream verdict(output_folder + "/verdict.txt");
    if (!verdict.is_open())
    {
        std::cerr << "Can't create verdict " << output_folder + "/verdict\n";
        return 1;
    }
    verdict << "Тест 1.4 ";
    test_passed = true;

    SRPEngine engine(test.models[0]);
    engine.setGridStep(0.05);

    SRPResult result_rtx, result_gpu, result_cpu;
    SRPResult result_pixel_rtx, result_pixel_gpu, result_pixel_cpu;
    std::vector<std::array<double, 3>> rtx_f, gpu_f, cpu_f, rtx_p_f, gpu_p_f, cpu_p_f;
    std::vector<std::array<double, 3>> r_rtx_f, r_gpu_f, r_cpu_f, r_rtx_p_f, r_gpu_p_f, r_cpu_p_f;

    // phase 0
    for (auto direction : test.sun_directions)
    {
        if (!test_passed) break;
        engine.setSunDirection(sun_directions[direction][2], sun_directions[direction][3], sun_directions[direction][4]);

        result_cpu = engine.compute(SRPMethod::CentroidCPU);
        cpu_f.push_back(result_cpu.total_force);

        result_pixel_cpu = engine.compute(SRPMethod::PixelGridCPU);
        cpu_p_f.push_back(result_pixel_cpu.total_force);
        
        if (is_rtx_device_available())
        {
            result_rtx = engine.compute(SRPMethod::CentroidRTX);
            rtx_f.push_back(result_rtx.total_force);

            result_pixel_rtx = engine.compute(SRPMethod::PixelGridRTX);
            rtx_p_f.push_back(result_pixel_rtx.total_force);
        }
        else if (is_cuda_device_available())
        {
            result_gpu = engine.compute(SRPMethod::CentroidGPU);
            gpu_f.push_back(result_gpu.total_force);

            result_pixel_gpu = engine.compute(SRPMethod::PixelGridGPU);
            gpu_p_f.push_back(result_pixel_gpu.total_force);
        }
        else
        {
            verdict << "НЕ ПРОЙДЕН: отсутвтует CUDA/RTX\n";
            test_passed = false;
            break;
        }
    }

    engine.dataset().setJointValue("SolarArrayGroup_L", 0.7853981633974483);

    // phase 1
    for (auto direction : test.sun_directions)
    {
        if (!test_passed) break;
        engine.setSunDirection(sun_directions[direction][2], sun_directions[direction][3], sun_directions[direction][4]);

        result_cpu = engine.compute(SRPMethod::CentroidCPU);
        result_pixel_cpu = engine.compute(SRPMethod::PixelGridCPU);
        
        if (is_rtx_device_available())
        {
            result_rtx = engine.compute(SRPMethod::CentroidRTX);
            result_pixel_rtx = engine.compute(SRPMethod::PixelGridRTX);

            if (!compare(result_rtx.total_force, result_cpu.total_force, test.tol_force))
            {
                verdict << "НЕ ПРОЙДЕН: расхождение модуля силы в методе CentroidRTX при направлении " << direction << "\n";
                test_passed = false;
                break;
            }
            if (!compare(result_pixel_rtx.total_force, result_pixel_cpu.total_force, 0.01))
            {
                verdict << "НЕ ПРОЙДЕН: расхождение модуля силы в методе PixelGridRTX при направлении " << direction << "\n";
                test_passed = false;
                break;
            }
        }
        else if (is_cuda_device_available())
        {
            result_gpu = engine.compute(SRPMethod::CentroidGPU);
            result_pixel_gpu = engine.compute(SRPMethod::PixelGridGPU);

            if (!compare(result_gpu.total_force, result_cpu.total_force, test.tol_force))
            {
                verdict << "НЕ ПРОЙДЕН: расхождение модуля силы в методе CentroidGPU при направлении " << direction << "\n";
                test_passed = false;
                break;
            }
            if (!compare(result_pixel_gpu.total_force, result_cpu.total_force, test.tol_force))
            {
                verdict << "НЕ ПРОЙДЕН: расхождение модуля силы в методе PixelGridGPU при направлении " << direction << "\n";
                test_passed = false;
                break;
            }
        }
        else
        {
            verdict << "НЕ ПРОЙДЕН: отсутвтует CUDA/RTX\n";
            test_passed = false;
            break;
        }
    }

    engine.dataset().clearJoint("SolarArrayGroup_L");
    
    // phase 2
    int i = 0;
    for (auto direction : test.sun_directions)
    {
        if (!test_passed) break;
        engine.setSunDirection(sun_directions[direction][2], sun_directions[direction][3], sun_directions[direction][4]);

        result_cpu = engine.compute(SRPMethod::CentroidCPU);
        result_pixel_cpu = engine.compute(SRPMethod::PixelGridCPU);

        if (!compare(result_cpu.total_force, cpu_f[i], 1e-10))
        {
            verdict << "НЕ ПРОЙДЕН: расхождение модуля силы при обратном повороте в методе CentroidCPU\n";
            test_passed = false;
            break;
        }
        if (!compare(result_pixel_cpu.total_force, cpu_p_f[i], 1e-10))
        {
            verdict << "НЕ ПРОЙДЕН: расхождение модуля силы при обратном повороте в методе PixelGridCPU\n";
            test_passed = false;
            break;
        }

        if (is_rtx_device_available())
        {
            result_rtx = engine.compute(SRPMethod::CentroidRTX);
            result_pixel_rtx = engine.compute(SRPMethod::PixelGridRTX);
            
            if (!compare(result_rtx.total_force, rtx_f[i], 1e-10))
            {
                verdict << "НЕ ПРОЙДЕН: расхождение модуля силы при обратном повороте в методе CentroidRTX\n";
                test_passed = false;
                break;
            }
            if (!compare(result_pixel_rtx.total_force, rtx_p_f[i], 1e-10))
            {
                verdict << "НЕ ПРОЙДЕН: расхождение модуля силы при обратном повороте в методе PixelGridRTX\n";
                test_passed = false;
                break;
            }
        }
        else if (is_cuda_device_available())
        {
            result_gpu = engine.compute(SRPMethod::CentroidGPU);
            result_pixel_gpu = engine.compute(SRPMethod::PixelGridGPU);
            
            if (!compare(result_gpu.total_force, gpu_f[i], 1e-10))
            {
                verdict << "НЕ ПРОЙДЕН: расхождение модуля силы при обратном повороте в методе CentroidGPU\n";
                test_passed = false;
                break;
            }
            if (!compare(result_pixel_gpu.total_force, gpu_p_f[i], 1e-10))
            {
                verdict << "НЕ ПРОЙДЕН: расхождение модуля силы при обратном повороте в методе PixelGridGPU\n";
                test_passed = false;
                break;
            }
        }
        else
        {
            verdict << "НЕ ПРОЙДЕН: отсутвтует CUDA/RTX\n";
            test_passed = false;
            break;
        }
        i++;
    }

    //phase 3 (rotation towards the sun)
    double gamma;
    std::array<double, 3> avg_normal{0.0, 0.0, 0.0};
    std::array<double, 3> sun_dir;
    double count = 0.0;

    for (auto direction : test.sun_directions)
    {
        if (!test_passed) break;
        engine.setSunDirection(sun_directions[direction][2], sun_directions[direction][3], sun_directions[direction][4]);
        gamma = std::atan2(-1.0 * sun_directions[direction][3], sun_directions[direction][4]);

        sun_dir[0] = sun_directions[direction][2];
        sun_dir[1] = sun_directions[direction][3];
        dun_dir[2] = sun_directions[direction][4];

        engine.dataset().setJointValue("SolarArrayGroup_L", gamma);

        auto faces = engine.dataset().componentTriangles("Panel_L");

        for (auto tr : faces)
        {
            if (getScalar(sun_dir, std::array<double,3>{tr.normal_x, tr.normal_y, tr.normal_z}) > 0)
            {
                count += 1.0;
                avg_normal[0] += tr.normal_x;
                avg_normal[1] += tr.normal_y;
                avg_normal[2] += tr.normal_z;
            }
        }

        avg_normal[0] /= count; avg_normal[1] /= count; avg_normal[2] /= count;

        if (!compare(getScalar(avg_normal, sun_dir), getNormal(std::array<double, 3>{0.0, sun_directions[direction][3], sun_directions[direction][4]}), 1e-6))
        {
            verdict << "НЕ ПРОЙДЕН: средняя нормаль смотрит не на Солнце\n";
            test_passed = false;
            break;
        }

    }


    if (test_passed)
    {
        verdict << "ПРОЙДЕН по критериям:\n";
    }

    verdict.close();
    return !test_passed;
}