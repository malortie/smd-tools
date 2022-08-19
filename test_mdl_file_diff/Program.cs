using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;

namespace mdl_file_diff
{
    class Program
    {
        const string valve_model_directory = "D:/SteamLibrary/steamapps/common/Half-Life/valve/models";
        const string valve_hd_model_directory = "D:/SteamLibrary/steamapps/common/Half-Life/valve_hd/models";
        const string gearbox_model_directory = "D:/SteamLibrary/steamapps/common/Half-Life/gearbox/models";
        const string gearbox_hd_model_directory = "D:/SteamLibrary/steamapps/common/Half-Life/gearbox_hd/models";
        const string bshift_model_directory = "D:/SteamLibrary/steamapps/common/Half-Life/bshift/models";
        const string bshift_hd_model_directory = "D:/SteamLibrary/steamapps/common/Half-Life/bshift_hd/models";

        static void PrintModelDiff_All(string output_file)
        {
            using (StreamWriter sw = new StreamWriter(output_file))
            {
                var valve_model_files = Directory.GetFiles(valve_model_directory, " *.mdl").Select(f => Path.GetFileName(f));
                var valve_hd_model_files = Directory.GetFiles(valve_hd_model_directory, "*.mdl").Select(f => Path.GetFileName(f));

                var gearbox_model_files = Directory.GetFiles(gearbox_model_directory, "*.mdl").Select(f => Path.GetFileName(f));
                var gearbox_hd_model_files = Directory.GetFiles(gearbox_hd_model_directory, "*.mdl").Select(f => Path.GetFileName(f));

                var bshift_model_files = Directory.GetFiles(bshift_model_directory, "*.mdl").Select(f => Path.GetFileName(f));
                var bshift_hd_model_files = Directory.GetFiles(bshift_hd_model_directory, "*.mdl").Select(f => Path.GetFileName(f));

                var l = new List<string>();
                l.AddRange(valve_model_files);
                l.AddRange(valve_hd_model_files);
                l.AddRange(gearbox_model_files);
                l.AddRange(gearbox_hd_model_files);
                l.AddRange(bshift_model_files);
                l.AddRange(bshift_hd_model_files);
                l.Sort();

                var unique_model_files = l.Distinct<string>();

                int longest_file_name = unique_model_files.Aggregate("", (max, cur) => max.Length > cur.Length ? max : cur).Length;

                string fmt_model = "{0,-*}".Replace("*", (longest_file_name + 1).ToString()) + 
                                                                               "|    {1}    |    {2}    |    {3}    |    {4}    |    {5}    |    {6}    |     {7}     |";
                string fmt_table_header_gap = string.Concat(Enumerable.Repeat(" ", longest_file_name + 1));
                string table_header_title             = fmt_table_header_gap + "|       Valve       |      Gearbox      |       BShift      |           |";
                string table_header_ld_hd             = fmt_table_header_gap + "|   LD    |    HD   |    LD   |    HD   |   LD    |   HD    |           |";
                string table_header_present_identical = fmt_table_header_gap + "| present | present | present | present | present | present | identical |";
                string table_header_line              = fmt_table_header_gap + string.Concat(Enumerable.Repeat("-", 73));

                sw.WriteLine(table_header_title);
                sw.WriteLine(table_header_line);
                sw.WriteLine(table_header_ld_hd);
                sw.WriteLine(table_header_line);
                sw.WriteLine(table_header_present_identical);
                sw.WriteLine(table_header_line);

                using (var md5 = MD5.Create())
                {
                    foreach (var model_file_name in unique_model_files)
                    {
                        bool model_present_in_valve = false;
                        bool model_present_in_valve_hd = false;
                        bool model_present_in_gearbox = false;
                        bool model_present_in_gearbox_hd = false;
                        bool model_present_in_bshift = false;
                        bool model_present_in_bshift_hd = false;
                        bool identical = true;

                        var presentModels = new List<string>();

                        if (File.Exists(Path.Combine(valve_model_directory, model_file_name)))
                        {
                            model_present_in_valve = true;
                            presentModels.Add(Path.Combine(valve_model_directory, model_file_name));
                        }
                        if (File.Exists(Path.Combine(valve_hd_model_directory, model_file_name)))
                        {
                            model_present_in_valve_hd = true;
                            presentModels.Add(Path.Combine(valve_hd_model_directory, model_file_name));
                        }
                        if (File.Exists(Path.Combine(gearbox_model_directory, model_file_name)))
                        {
                            model_present_in_gearbox = true;
                            presentModels.Add(Path.Combine(gearbox_model_directory, model_file_name));
                        }
                        if (File.Exists(Path.Combine(gearbox_hd_model_directory, model_file_name)))
                        {
                            model_present_in_gearbox_hd = true;
                            presentModels.Add(Path.Combine(gearbox_hd_model_directory, model_file_name));
                        }
                        if (File.Exists(Path.Combine(bshift_model_directory, model_file_name)))
                        {
                            model_present_in_bshift = true;
                            presentModels.Add(Path.Combine(bshift_model_directory, model_file_name));
                        }
                        if (File.Exists(Path.Combine(bshift_hd_model_directory, model_file_name)))
                        {
                            model_present_in_bshift_hd = true;
                            presentModels.Add(Path.Combine(bshift_hd_model_directory, model_file_name));
                        }

                        foreach (var file1 in presentModels)
                        {
                            if (!identical)
                                break;

                            foreach (var file2 in presentModels)
                            {
                                if (file1 == file2)
                                    continue;

#if true
                                var file1info = new FileInfo(file1);
                                var file2info = new FileInfo(file2);
                                if (file1info.Length != file2info.Length)
                                {
                                    identical = false;
                                    break;
                                }

                                var file1bytes = File.ReadAllBytes(file1);
                                var file2bytes = File.ReadAllBytes(file2);

                                if (file1bytes.Length != file2bytes.Length)
                                {
                                    identical = false;
                                    break;
                                }

                                for (int i = 0; i < file1bytes.Length; ++i)
                                {
                                    if (file1bytes[i] != file2bytes[i])
                                    {
                                        identical = false;
                                        break;
                                    }
                                }

                                if (!identical)
                                    break;
#else
                                string file1Hash = string.Empty;
                                string file2Hash = string.Empty;
                                using (var stream = File.OpenRead(file1))
                                {
                                    file1Hash = BitConverter.ToString(md5.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
                                }
                                using (var stream = File.OpenRead(file2))
                                {
                                    file2Hash = BitConverter.ToString(md5.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
                                }

                                if (file1Hash != file2Hash)
                                {
                                    identical = false;
                                    break;
                                }
#endif

                                int a = 2;
                                a++;
                            }

                            int b = 2;
                            b++;
                        }


                        sw.WriteLine(fmt_model,
                            model_file_name,
                            model_present_in_valve ? 'X' : ' ',
                            model_present_in_valve_hd ? 'X' : ' ',
                            model_present_in_gearbox ? 'X' : ' ',
                            model_present_in_gearbox_hd ? 'X' : ' ',
                            model_present_in_bshift ? 'X' : ' ',
                            model_present_in_bshift_hd ? 'X' : ' ',
                            identical ? 'X' : ' '
                            );
                    }
                }
            }
        }

        static void PrintModelDiff(string output_file, string valve_models_dir, string gearbox_models_dir, string bshift_models_dir)
        {
            using (StreamWriter sw = new StreamWriter(output_file))
            {

                var valve_model_files = Directory.GetFiles(valve_models_dir, " *.mdl").Select(f => Path.GetFileName(f));
                var gearbox_model_files = Directory.GetFiles(gearbox_models_dir, "*.mdl").Select(f => Path.GetFileName(f));
                var bshift_model_files = Directory.GetFiles(bshift_models_dir, "*.mdl").Select(f => Path.GetFileName(f));

                var l = new List<string>();
                l.AddRange(valve_model_files);
                l.AddRange(gearbox_model_files);
                l.AddRange(bshift_model_files);
                l.Sort();

                var unique_model_files = l.Distinct<string>();
                var unique_non_identical_models = new HashSet<string>();

                int longest_file_name = unique_model_files.Aggregate("", (max, cur) => max.Length > cur.Length ? max : cur).Length;

                string fmt_model = "{0,-*}".Replace("*", (longest_file_name + 1).ToString()) +
                                                                                 "|    {1}    |    {2}    |    {3}    |     {4}     |";
                string fmt_table_header_gap = string.Concat(Enumerable.Repeat(" ", longest_file_name + 1));
                string table_header_title               = fmt_table_header_gap + "|  Valve  | Gearbox | BShift  |           |";
                string table_header_present_identical   = fmt_table_header_gap + "| present | present | present | identical |";
                string table_header_line = fmt_table_header_gap + string.Concat(Enumerable.Repeat("-", 43));

                sw.WriteLine(table_header_title);
                sw.WriteLine(table_header_line);
                sw.WriteLine(table_header_present_identical);
                sw.WriteLine(table_header_line);

                using (var md5 = MD5.Create())
                {
                    foreach (var model_file_name in unique_model_files)
                    {
                        bool model_present_in_valve = false;
                        bool model_present_in_gearbox = false;
                        bool model_present_in_bshift = false;
                        bool identical = true;

                        var presentModels = new List<string>();

                        if (File.Exists(Path.Combine(valve_models_dir, model_file_name)))
                        {
                            model_present_in_valve = true;
                            presentModels.Add(Path.Combine(valve_models_dir, model_file_name));
                        }
                        if (File.Exists(Path.Combine(gearbox_models_dir, model_file_name)))
                        {
                            model_present_in_gearbox = true;
                            presentModels.Add(Path.Combine(gearbox_models_dir, model_file_name));
                        }
                        if (File.Exists(Path.Combine(bshift_models_dir, model_file_name)))
                        {
                            model_present_in_bshift = true;
                            presentModels.Add(Path.Combine(bshift_models_dir, model_file_name));
                        }

                        foreach (var file1 in presentModels)
                        {
                            if (!identical)
                                break;

                            foreach (var file2 in presentModels)
                            {
                                if (file1 == file2)
                                    continue;

#if true
                                var file1info = new FileInfo(file1);
                                var file2info = new FileInfo(file2);
                                if (file1info.Length != file2info.Length)
                                {
                                    identical = false;
                                    unique_non_identical_models.Add(Path.GetFileName(file1));
                                    break;
                                }

                                var file1bytes = File.ReadAllBytes(file1);
                                var file2bytes = File.ReadAllBytes(file2);

                                if (file1bytes.Length != file2bytes.Length)
                                {
                                    identical = false;
                                    unique_non_identical_models.Add(Path.GetFileName(file1));
                                    break;
                                }

                                for (int i = 0; i < file1bytes.Length; ++i)
                                {
                                    if (file1bytes[i] != file2bytes[i])
                                    {
                                        identical = false;
                                        break;
                                    }
                                }

                                if (!identical)
                                {
                                    unique_non_identical_models.Add(Path.GetFileName(file1));
                                    break;
                                }
#else
                            string file1Hash = string.Empty;
                            string file2Hash = string.Empty;
                            using (var stream = File.OpenRead(file1))
                            {
                                file1Hash = BitConverter.ToString(md5.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
                            }
                            using (var stream = File.OpenRead(file2))
                            {
                                file2Hash = BitConverter.ToString(md5.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
                            }

                            if (file1Hash != file2Hash)
                            {
                                identical = false;
                                break;
                            }
#endif

                                int a = 2;
                                a++;
                            }

                            int b = 2;
                            b++;
                        }


                        sw.WriteLine(fmt_model,
                            model_file_name,
                            model_present_in_valve ? 'X' : ' ',
                            model_present_in_gearbox ? 'X' : ' ',
                            model_present_in_bshift ? 'X' : ' ',
                            identical ? 'X' : ' '
                            );
                    }
                }

                sw.WriteLine(table_header_line);
                sw.WriteLine("");

                if (unique_non_identical_models.Count > 0)
                {
                    sw.WriteLine("Binary unequal models found:");
                    sw.WriteLine("");

                    foreach (var different_model in unique_non_identical_models)
                    {
                        sw.WriteLine(different_model);
                    }
                }
                else
                {
                    sw.WriteLine("All models are binary equal");
                }
            }
        }


        static void Main(string[] args)
        {
            PrintModelDiff("models_diff_ld.txt", valve_model_directory, gearbox_model_directory, bshift_model_directory);
            PrintModelDiff("models_diff_hd.txt", valve_hd_model_directory, gearbox_hd_model_directory, bshift_hd_model_directory);
            //PrintModelDiff_All("models_diff_all.txt");


            Console.WriteLine("Hello World!");
        }
    }
}
