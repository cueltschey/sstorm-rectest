#ifndef SSTORM_RECON_AUTOCONF_H
#define SSTORM_RECON_AUTOCONF_H

#include "rt_recon/recon_data.h"
#include "rt_recon/influx_reader.h"
#include <cstdint>
#include <iostream>
#include <string>

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

inline bool pull_recon_data(const ReconConfig& cfg,
                             rt_recon::CellConfig& cell,
                             rt_recon::PrachConfig& prach,
                             rt_recon::MibData& mib)
{
  if (!cfg.enable) {
    return false;
  }

  rt_recon::InfluxDbConfig db_cfg;
  db_cfg.host    = cfg.host;
  db_cfg.port    = cfg.port;
  db_cfg.org     = cfg.org;
  db_cfg.token   = cfg.token;
  db_cfg.bucket  = cfg.bucket;
  db_cfg.data_id = cfg.data_id;

  rt_recon::InfluxReader reader(db_cfg);

  bool cell_ok = reader.pull_cell_config(cell);
  reader.pull_prach_config(prach);
  reader.pull_mib(mib);

  return cell_ok;
}

inline void print_recon_summary(const rt_recon::CellConfig& cell)
{
  std::cout << "[RECON] Loaded cell configuration from InfluxDB:\n";
  std::cout << "[RECON]   Band: " << cell.band << "\n";
  std::cout << "[RECON]   PRBs: " << cell.nof_prb << "\n";
  std::cout << "[RECON]   DL ARFCN: " << cell.dl_arfcn << "\n";
  std::cout << "[RECON]   UL ARFCN: " << cell.ul_arfcn << "\n";
  std::cout << "[RECON]   DL Freq: " << cell.dl_freq / 1e6 << " MHz\n";
  std::cout << "[RECON]   UL Freq: " << cell.ul_freq / 1e6 << " MHz\n";
  std::cout << "[RECON]   Sample Rate: " << cell.sample_rate / 1e6 << " MHz\n";
  std::cout << "[RECON]   RX Gain: " << cell.rx_gain << " dB\n";
  std::cout << "[RECON]   TX Gain: " << cell.tx_gain << " dB\n";
  std::cout << "[RECON]   SCS: " << cell.scs << "\n";
}

} // namespace sstorm

#endif // SSTORM_RECON_AUTOCONF_H
