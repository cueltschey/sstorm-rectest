#ifndef SSTORM_RECON_AUTOCONF_H
#define SSTORM_RECON_AUTOCONF_H

#include <cstdint>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>

#include "rt-recon-sdk/autoconfig/influxdb.hpp"

namespace sstorm {

struct ReconConfig {
  bool        enable  = false;
  std::string host    = "localhost";
  uint32_t    port    = 8086;
  std::string org;
  std::string token;
  std::string bucket  = "rtusystem";
  std::string data_id;
};

struct ReconData {
  int32_t  dl_arfcn    = 0;
  int32_t  ul_arfcn    = 0;
  int32_t  nof_prb     = 0;
  double   dl_freq     = 0.0;
  double   ul_freq     = 0.0;
  double   sample_rate = 0.0;
  int32_t  scs_khz     = 0;
  int32_t  band        = 0;
  std::string pattern;
  double rx_gain = 0.0;
  double tx_gain = 0.0;
};

inline std::string json_escape(const std::string& s)
{
  std::string out;
  out.reserve(s.size() * 12 / 10);
  for (char c : s) {
    switch (c) {
      case '\"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:   out += c;
    }
  }
  return out;
}

inline std::unordered_map<std::string, std::string> parse_flux_fields(const std::string& resp)
{
  std::unordered_map<std::string, std::string> result;
  std::stringstream ss(resp);
  std::string line;
  int field_col = -1, value_col = -1;

  while (std::getline(ss, line)) {
    if (line.empty() || line[0] == '#') continue;

    std::vector<std::string> cols;
    std::stringstream ls(line);
    std::string item;
    while (std::getline(ls, item, ',')) {
      cols.push_back(item);
    }

    if (field_col == -1) {
      for (size_t i = 0; i < cols.size(); ++i) {
        if (cols[i] == "_field") field_col = (int)i;
        if (cols[i] == "_value") value_col = (int)i;
      }
      continue;
    }

    if (field_col >= 0 && value_col >= 0 &&
        field_col < (int)cols.size() && value_col < (int)cols.size()) {
      result[cols[field_col]] = cols[value_col];
    }
  }

  return result;
}

inline bool query_measurement(const ReconConfig& cfg, const std::string& measurement,
                               std::unordered_map<std::string, std::string>& fields)
{
  influxdb_cpp::server_info si(cfg.host, (int)cfg.port, cfg.org, cfg.token, cfg.bucket);
  if (si.res_ == NULL) {
    std::cerr << "[RECON] Failed to resolve InfluxDB host: " << cfg.host << std::endl;
    return false;
  }

  std::string query_str = "from(bucket: \"" + cfg.bucket + "\") "
    "|> range(start: -10m) "
    "|> filter(fn: (r) => r._measurement == \"" + measurement + "\")";

  if (!cfg.data_id.empty()) {
    query_str += " |> filter(fn: (r) => r.sni5gect_data_id == \"" + cfg.data_id + "\")";
  }

  query_str += " |> last()";

  std::string resp;
  int ret = influxdb_cpp::flux_query(resp, json_escape(query_str), si);
  if (ret != 0) {
    std::cerr << "[RECON] InfluxDB query for " << measurement << " failed: " << resp << std::endl;
    return false;
  }

  fields = parse_flux_fields(resp);
  return !fields.empty();
}

inline bool pull_recon_data(const ReconConfig& cfg, ReconData& data)
{
  if (!cfg.enable) return false;

  std::unordered_map<std::string, std::string> band_fields;
  if (query_measurement(cfg, "band_report", band_fields)) {
    auto get_int = [](const std::unordered_map<std::string, std::string>& m,
                       const std::string& k, int32_t d) -> int32_t {
      auto it = m.find(k);
      return (it != m.end()) ? std::stoi(it->second) : d;
    };
    auto get_dbl = [](const std::unordered_map<std::string, std::string>& m,
                       const std::string& k, double d) -> double {
      auto it = m.find(k);
      return (it != m.end()) ? std::stod(it->second) : d;
    };

    data.dl_arfcn    = get_int(band_fields, "dl_arfcn", 0);
    data.ul_arfcn    = get_int(band_fields, "ul_arfcn", 0);
    data.nof_prb     = get_int(band_fields, "nof_prb", 0);
    data.dl_freq     = get_dbl(band_fields, "dl_freq", 0.0);
    data.ul_freq     = get_dbl(band_fields, "ul_freq", 0.0);
    data.sample_rate = get_dbl(band_fields, "sample_rate", 0.0);
    data.scs_khz     = get_int(band_fields, "scs_khz", 0);
    data.band        = get_int(band_fields, "band", 0);

    auto it = band_fields.find("pattern");
    if (it != band_fields.end()) data.pattern = it->second;
  }

  std::unordered_map<std::string, std::string> ch_fields;
  if (query_measurement(cfg, "channel_config", ch_fields)) {
    auto get_dbl = [](const std::unordered_map<std::string, std::string>& m,
                       const std::string& k, double d) -> double {
      auto it = m.find(k);
      return (it != m.end()) ? std::stod(it->second) : d;
    };

    data.rx_gain = get_dbl(ch_fields, "rx_gain", 0.0);
    data.tx_gain = get_dbl(ch_fields, "tx_gain", 0.0);
  }

  return !band_fields.empty() || !ch_fields.empty();
}

inline void print_recon_summary(const ReconData& data)
{
  std::cout << "[RECON] Loaded cell configuration from InfluxDB:\n";
  if (data.band > 0)        std::cout << "[RECON]   Band: " << data.band << "\n";
  if (data.nof_prb > 0)     std::cout << "[RECON]   PRBs: " << data.nof_prb << "\n";
  if (data.dl_arfcn > 0)    std::cout << "[RECON]   DL ARFCN: " << data.dl_arfcn << "\n";
  if (data.ul_arfcn > 0)    std::cout << "[RECON]   UL ARFCN: " << data.ul_arfcn << "\n";
  if (data.dl_freq > 0.0)   std::cout << "[RECON]   DL Freq: " << data.dl_freq / 1e6 << " MHz\n";
  if (data.ul_freq > 0.0)   std::cout << "[RECON]   UL Freq: " << data.ul_freq / 1e6 << " MHz\n";
  if (data.sample_rate > 0.0) std::cout << "[RECON]   Sample Rate: " << data.sample_rate / 1e6 << " MHz\n";
  if (data.rx_gain > 0.0)   std::cout << "[RECON]   RX Gain: " << data.rx_gain << " dB\n";
  if (data.tx_gain > 0.0)   std::cout << "[RECON]   TX Gain: " << data.tx_gain << " dB\n";
  if (data.scs_khz > 0)     std::cout << "[RECON]   SCS: " << data.scs_khz << " kHz\n";
  if (!data.pattern.empty()) std::cout << "[RECON]   Pattern: " << data.pattern << "\n";
}

} // namespace sstorm

#endif // SSTORM_RECON_AUTOCONF_H
