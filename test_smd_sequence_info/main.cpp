// test_mdl_sequence_info.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <cassert>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "studiomodel.h"

#if 1
#define MDL_FILE_EXPECTED "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/console_civ_scientist/console_civ_scientist_ld.mdl"
//#define MDL_FILE_ACTUAL "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/console_civ_scientist/console_civ_scientist_ld_bshift.mdl"
#define MDL_FILE_ACTUAL "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/console_civ_scientist/console_civ_scientist_hd.mdl"
#endif
#if 0
#define MDL_FILE_EXPECTED "C:/Users/marc-/Bureau/hl_decompiled_models_backup/hl1/ld/bullsquid/bullsquid.mdl"
#define MDL_FILE_ACTUAL "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/bullsquid/bullsquid_hd.mdl"
#endif
#if 0
#define MDL_FILE_EXPECTED "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/dead_barney/dead_barney_ld.mdl"
#define MDL_FILE_ACTUAL "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/dead_barney/dead_barney_hd.mdl"
#endif
#if 0
#define MDL_FILE_EXPECTED "C:/Users/marc-/Bureau/hl_decompiled_models_backup/hl1/ld/agrunt/agrunt.mdl"
#define MDL_FILE_ACTUAL "C:/Users/marc-/Bureau/hl_decompiled_models_backup/hl1/hd/agrunt/agrunt.mdl"
#endif
#if 0
#define MDL_FILE_EXPECTED "C:/Users/marc-/Bureau/hl_decompiled_models_backup/bshift/ld/wrangler/wrangler.mdl"
#define MDL_FILE_ACTUAL "C:/Users/marc-/Bureau/hl_decompiled_models_backup/bshift/hd/wrangler/wrangler.mdl"
#endif
#if 0
#define MDL_FILE_EXPECTED "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/hgrunt/hgrunt_ld.mdl"
#define MDL_FILE_ACTUAL "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/hgrunt/hgrunt_hd.mdl"
#endif
#if 0
#define MDL_FILE_EXPECTED "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/gman/gman_ld.mdl"
#define MDL_FILE_ACTUAL "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/gman/gman_hd.mdl"
#endif
#if 0
#define MDL_FILE_EXPECTED "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/houndeye/houndeye_ld.mdl"
#define MDL_FILE_ACTUAL "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/houndeye/houndeye_hd.mdl"
#endif
#if 0
#define MDL_FILE_EXPECTED "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/islave/islave_ld.mdl"
#define MDL_FILE_ACTUAL "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/islave/islave_hd.mdl"
#endif
#if 0
#define MDL_FILE_EXPECTED "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/civ_paper_scientist/civ_paper_scientist_ld.mdl"
#define MDL_FILE_ACTUAL "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/civ_paper_scientist/civ_paper_scientist_hd.mdl"
#endif
#if 0
#define MDL_FILE_EXPECTED "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/civ_coat_scientist/civ_coat_scientist_ld.mdl"
#define MDL_FILE_ACTUAL "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/civ_coat_scientist/civ_coat_scientist_hd.mdl"
#endif
#if 0
#define MDL_FILE_EXPECTED "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/zombie/zombie_ld.mdl"
#define MDL_FILE_ACTUAL "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/zombie/zombie_hd.mdl"
#endif
#if 0
#define MDL_FILE_EXPECTED "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/barney/barney_ld.mdl"
#define MDL_FILE_ACTUAL "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/barney/barney_hd.mdl"
#endif
#if 0
#define MDL_FILE_EXPECTED "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/scientist/scientist_ld.mdl"
#define MDL_FILE_ACTUAL "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/scientist/scientist_ld_bshift.mdl"
//#define MDL_FILE_ACTUAL "C:/Users/marc-/Documents/GitHub/custom-hl-viewmodels/modelsrc/scientist/scientist_hd.mdl"
#endif

int main()
{
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW);
#endif



    {
        StudioModel expected, actual;

        try
        {
            expected.Init(MDL_FILE_EXPECTED);
            actual.Init(MDL_FILE_ACTUAL);

            {
                auto add_unique_sequence = [](
                    const StudioModel& model,
                    const mstudioseqdesc_t* pseqdesc,
                    std::map<std::string, const mstudioseqdesc_t*>& sequences) {

                    if (sequences.find(pseqdesc->label) == sequences.end())
                    {
                        sequences.insert(std::make_pair(pseqdesc->label, pseqdesc));
                    }
                    else
                    {
                        bool already_exists = true;

                        int j = 1;
                        static char new_name[256]{};

                        while (true)
                        {
                            memset(new_name, std::size(new_name), 0);
                            sprintf(new_name, "%s_%d", pseqdesc->label, j);

                            if (sequences.find(new_name) == sequences.end())
                            {
                                sequences.insert(std::make_pair(new_name, pseqdesc));
                                break;
                            }

                            ++j;
                        }
                    }
                };

                std::map<std::string, const mstudioseqdesc_t*> expected_sequences;
                std::map<std::string, const mstudioseqdesc_t*> actual_sequences;

                // Get expected sequences
  
                for (int i = 0; i < expected.GetStudioHdr()->numseq; ++i)
                {
                    auto pseqdesc = expected.GetSequence(i);
                    add_unique_sequence(expected, pseqdesc, expected_sequences);
                }

                // Get actual sequences
                for (int i = 0; i < actual.GetStudioHdr()->numseq; ++i)
                {
                    auto pseqdesc = actual.GetSequence(i);
                    add_unique_sequence(actual, pseqdesc, actual_sequences);
                }

                if (expected_sequences.size() != actual_sequences.size())
                    throw;


                for (auto it_expected = expected_sequences.begin(); it_expected != expected_sequences.end(); ++it_expected)
                {
                    auto it_actual = actual_sequences.find(it_expected->first);
                    if (it_actual == actual_sequences.end())
                        throw;


                    float expected_duration = it_expected->second->numframes / it_expected->second->fps;
                    float actual_duration = it_actual->second->numframes / it_actual->second->fps;

                    expected_duration = static_cast<int>(expected_duration * 100) / 100.0;
                    actual_duration = static_cast<int>(actual_duration * 100) / 100.0;

#define PRINT_SEQUENCES_WITH_IDENTICAL_DURATION 0

                    bool must_print_sequence = true;

                    if (expected_duration == actual_duration && !PRINT_SEQUENCES_WITH_IDENTICAL_DURATION)
                        must_print_sequence = false;

                    if (must_print_sequence)
                    {
                        printf("%s\n", it_expected->first.c_str());
                        printf("numframes: expected: %d actual: %d\n",
                            it_expected->second->numframes,
                            it_actual->second->numframes
                        );
                        printf("fps: expected: %d actual: %d\n",
                            static_cast<int>(it_expected->second->fps),
                            static_cast<int>(it_actual->second->fps)
                        );
                        printf("duration: expected: %.2f actual: %.2f ",
                            expected_duration, actual_duration);

                        if (expected_duration == actual_duration)
                            printf("(OK)");
                        else {
                            printf("(DIFFERENT) ");

                            int prop_fps = static_cast<int>(
                                (it_expected->second->fps * it_actual->second->numframes) / it_expected->second->numframes
                                );

                            float prop_duration = static_cast<float>(it_actual->second->numframes) / prop_fps;
                            float prop_duration_plus = static_cast<float>(it_actual->second->numframes) / (prop_fps + 1);
                            float prop_duration_minus = static_cast<float>(it_actual->second->numframes) / (prop_fps - 1);

                            int best_fps_opt = -1;

                            float prop_delta = std::abs(expected_duration - prop_duration);
                            float prop_plus_delta = std::abs(expected_duration - prop_duration_plus);
                            float prop_minus_delta = std::abs(expected_duration - prop_duration_minus);

                            if (prop_plus_delta < prop_minus_delta)
                            {
                                if (prop_plus_delta < prop_delta)
                                    best_fps_opt = prop_fps + 1;
                                else
                                    best_fps_opt = prop_fps;
                            }
                            else
                            {
                                if (prop_minus_delta < prop_delta)
                                    best_fps_opt = prop_fps - 1;
                                else
                                    best_fps_opt = prop_fps;
                            }

                            float closest_delta = std::min(std::min(prop_delta, prop_plus_delta), prop_minus_delta);

                            if (best_fps_opt != it_actual->second->fps)
                                printf("Try with %d fps", best_fps_opt);
                            else {
                                /*printf("Already closest fps (prop delta: %.2f prop delta + 1: %.2f prop delta - 1: %.2f)",
                                    prop_delta, prop_plus_delta, prop_minus_delta);*/
                                printf("Already closest fps (closest delta: %.2f prop delta: %.2f)",
                                    closest_delta, prop_delta);

                            }
                        }

                        puts("\n");

                    }
                }

                int a = 2;
                a++;
            }

            expected.Free();
            actual.Free();
        }
        catch (const std::exception&)
        {
            expected.Free();
            actual.Free();
        }
    }

#ifdef _DEBUG
    _CrtDumpMemoryLeaks();
#endif

    return 0;
}
