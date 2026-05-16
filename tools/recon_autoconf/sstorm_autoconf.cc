#include "rt_recon/influx_reader.h"
#include "rt_recon/config_generator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

static std::string trim(const std::string& s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  size_t end = s.find_last_not_of(" \t\r\n");
  if (start == std::string::npos) return "";
  return s.substr(start, end - start + 1);
}

static bool load_yaml_config(const std::string& path, rt_recon::InfluxDbConfig& cfg) {
  std::ifstream f(path);
  if (!f.is_open()) {
    std::cerr << "[sstorm_autoconf] Cannot open " << path << std::endl;
    return false;
  }

  std::string line;
  bool in_influx = false;
  while (std::getline(f, line)) {
    if (line[0] == '#' || line.empty()) continue;

    if (line.find("influxdb:") == 0) { in_influx = true; continue; }

    if (!in_influx) continue;

    auto colon = line.find(':');
    if (colon == std::string::npos) continue;

    std::string key = trim(line.substr(0, colon));
    std::string val = trim(line.substr(colon + 1));

    if (key == "host")    cfg.host = val;
    else if (key == "port") cfg.port = (uint32_t)std::stoul(val);
    else if (key == "org") cfg.org = val;
    else if (key == "token") cfg.token = val;
    else if (key == "bucket") cfg.bucket = val;
    else if (key == "data_id") cfg.data_id = val;
  }

  return true;
}

static void print_usage(const char* prog) {
  std::cerr << "sstorm-rectest Auto-Configuration Tool\n"
            << "Usage: " << prog << " [options]\n\n"
            << "Options:\n"
            << "  --config <file>     YAML config file with influxdb section\n"
            << "  --output <dir>      Output directory (default: .)\n"
            << "  --host <host>       InfluxDB host\n"
            << "  --port <port>       InfluxDB port\n"
            << "  --org <org>         InfluxDB org\n"
            << "  --token <token>     InfluxDB token\n"
            << "  --bucket <bucket>   InfluxDB bucket (default: rtusystem)\n"
            << "  --data-id <id>      Recon data ID\n"
            << "  --enb-only          Generate only eNB configs\n"
            << "  --ue-only           Generate only UE configs\n"
            << "  --help              Show this help\n";
}

int main(int argc, char** argv) {
  rt_recon::InfluxDbConfig db_cfg;
  db_cfg.bucket = "rtusystem";
  std::string output_dir = ".";
  std::string config_file;
  bool enb_only = false;
  bool ue_only = false;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--help") { print_usage(argv[0]); return 0; }
    else if (arg == "--config" && i + 1 < argc) config_file = argv[++i];
    else if (arg == "--output" && i + 1 < argc) output_dir = argv[++i];
    else if (arg == "--host" && i + 1 < argc) db_cfg.host = argv[++i];
    else if (arg == "--port" && i + 1 < argc) db_cfg.port = (uint32_t)std::stoul(argv[++i]);
    else if (arg == "--org" && i + 1 < argc) db_cfg.org = argv[++i];
    else if (arg == "--token" && i + 1 < argc) db_cfg.token = argv[++i];
    else if (arg == "--bucket" && i + 1 < argc) db_cfg.bucket = argv[++i];
    else if (arg == "--data-id" && i + 1 < argc) db_cfg.data_id = argv[++i];
    else if (arg == "--enb-only") enb_only = true;
    else if (arg == "--ue-only") ue_only = true;
  }

  if (enb_only && ue_only) {
    std::cerr << "Cannot specify both --enb-only and --ue-only\n";
    return 1;
  }

  if (!config_file.empty()) {
    if (!load_yaml_config(config_file, db_cfg)) {
      return 1;
    }
  }

  std::cout << "sstorm-rectest autoconf\n"
            << "  InfluxDB: " << db_cfg.host << ":" << db_cfg.port << "\n"
            << "  Bucket: " << db_cfg.bucket << "\n"
            << "  Data ID: " << db_cfg.data_id << "\n"
            << "  Output: " << output_dir << "\n\n";

  rt_recon::InfluxReader reader(db_cfg);

  rt_recon::CellConfig cell;
  if (!reader.pull_cell_config(cell)) {
    std::cerr << "ERROR: No recon data found in InfluxDB for data_id='" << db_cfg.data_id << "'\n";
    std::cerr << "Make sure sni5gect-compact shadower is running and pushing data.\n";
    return 1;
  }

  std::cout << "  Recon data loaded:\n";
  std::cout << "    Band: " << cell.band << "\n";
  std::cout << "    PRBs: " << cell.nof_prb << "\n";
  std::cout << "    DL ARFCN: " << cell.dl_arfcn << "\n";
  std::cout << "    UL ARFCN: " << cell.ul_arfcn << "\n";
  std::cout << "    DL Freq: " << cell.dl_freq / 1e6 << " MHz\n";
  std::cout << "    UL Freq: " << cell.ul_freq / 1e6 << " MHz\n";
  std::cout << "    Sample Rate: " << cell.sample_rate / 1e6 << " MHz\n";
  std::cout << "    RX Gain: " << cell.rx_gain << " dB\n";
  std::cout << "    TX Gain: " << cell.tx_gain << " dB\n";
  std::cout << "    SCS: " << cell.scs << "\n";
  std::cout << "    SSB Pattern: " << cell.ssb_pattern << "\n";

  rt_recon::PrachConfig prach;
  rt_recon::MibData mib;
  reader.pull_prach_config(prach);
  reader.pull_mib(mib);

  rt_recon::SstormConfig sscfg = rt_recon::ConfigGenerator::from_cell_config_full(cell, prach, mib);

  bool ok = true;
  if (!enb_only) {
    if (ue_only) {
      ok &= rt_recon::ConfigGenerator::generate_ue_conf(sscfg, output_dir + "/ue.conf");
    } else {
      ok &= rt_recon::ConfigGenerator::generate_all(sscfg, output_dir);
    }
  }
  if (!ue_only && !enb_only) {
    ok &= rt_recon::ConfigGenerator::generate_enb_conf(sscfg, output_dir + "/enb.conf");
    ok &= rt_recon::ConfigGenerator::generate_rr_conf(sscfg, output_dir + "/rr.conf");
    ok &= rt_recon::ConfigGenerator::generate_sib_conf(sscfg, output_dir + "/sib.conf");
  } else if (enb_only) {
    ok &= rt_recon::ConfigGenerator::generate_enb_conf(sscfg, output_dir + "/enb.conf");
    ok &= rt_recon::ConfigGenerator::generate_rr_conf(sscfg, output_dir + "/rr.conf");
    ok &= rt_recon::ConfigGenerator::generate_sib_conf(sscfg, output_dir + "/sib.conf");
  }

  if (ok) {
    std::cout << "\nConfig files generated successfully in " << output_dir << "\n";
    return 0;
  }

  std::cerr << "Failed to generate some config files\n";
  return 1;
}
