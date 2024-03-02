#include "modules/remote_bitrate_estimator/tradeoff_control.h"

#include <stdint.h>
#include <algorithm>
#include <cstdint>
#include <deque>
#include <numeric>
#include <vector>

#include "absl/types/optional.h"
#include "api/transport/network_types.h"
#include "api/transport/webrtc_key_value_config.h"
#include "api/units/data_rate.h"
#include "api/units/timestamp.h"
#include "modules/congestion_controller/goog_cc/link_capacity_estimator.h"
#include "modules/remote_bitrate_estimator/include/bwe_defines.h"
#include "rtc_base/experiments/field_trial_parser.h"
#include "rtc_base/logging.h"

constexpr double MAX_TRADE_OFF = 1.0;
constexpr int change_point_interval = 3000;

namespace webrtc {
TradeOffControl::TradeOffControl()
    : time_last_decrease_bitrate(Timestamp::MinusInfinity()),
      time_last_change_tradeoff(Timestamp::MinusInfinity()),
      last_period_bitrate(DataRate::KilobitsPerSec(1000)),
      last_period_tput(DataRate::KilobitsPerSec(1000)),
      last_period_rtt(TimeDelta::Millis(200)) {}

void TradeOffControl::UpdateTradeOffEstimate(TimeDelta rtt,
                                             DataRate throughput) {
  if (rtt.IsZero()) {
    return;
  }
  network_rtts.push_back(rtt.ms());
  network_tputs.push_back(throughput.bps());
}

void TradeOffControl::UpdateTargetBitrate(Timestamp at_time_,
                                          DataRate target_bitrate) {
  if (target_bitrate.bps() > 0) {
    if (!target_bitrates.empty())
    if (target_bitrate.bps() < target_bitrates.back() || time_last_decrease_bitrate.IsInfinite())
      time_last_decrease_bitrate = at_time_;
    target_bitrates.push_back(target_bitrate.bps());
  }
  RTC_LOG(LS_WARNING) << "updatetargetbitrate ";
}

void TradeOffControl::AdjustProbingParameters(double* beta,
                                              double* alpha,
                                              double* additive_coef) {
  // GCC: trade-off = 0.8
  *beta = 0.93 - 0.1 * cur_trade_off;
  // *beta = 0.77 + 0.1 * cur_trade_off;
  *alpha = 1.01 + 0.3 * (1.0 - cur_trade_off);
  *additive_coef = 0.7 + 20 * (1.0 - cur_trade_off);
  RTC_LOG(LS_WARNING) << "set probing param with trade-off " << cur_trade_off;
}

void TradeOffControl::GetQoeLambda() {
  // double delay_punish_coef = 1.0 / 3.0;
  // double bitrate_reward_coef = 5.0;
  // if (last_period_bitrate.bps() < 800000)
  //   bitrate_reward_coef = 50.0;
  // else if (last_period_bitrate.bps() < 1000000)
  //   bitrate_reward_coef = 36.0;
  // else if (last_period_bitrate.bps() < 1500000)
  //   bitrate_reward_coef = 14.0;
  // else if (last_period_bitrate.bps() < 2000000)
  //   bitrate_reward_coef = 10.0;
  // else if (last_period_bitrate.bps() < 3000000)
  //   bitrate_reward_coef = 3.3;
  // else
  //   bitrate_reward_coef = 1.0;

  // bitrate_reward_coef = bitrate_reward_coef * 0.5;
  // delay_punish_coef = delay_punish_coef * 0.3;
  qoe_lambda = (last_period_bitrate.kbps() / 1000.0 + 0.15) / 150.0;
}

// if updated, return true
bool TradeOffControl::UpdateTradeoff(Timestamp at_time_) {
  if (time_last_change_tradeoff.IsMinusInfinity())
    time_last_change_tradeoff = at_time_;


  if (at_time_.ms() - time_last_change_tradeoff.ms() < 1000) {
    if (at_time_.ms() - time_last_decrease_bitrate.ms() < change_point_interval)
      return false;
    // bandwidth increase a lot
    else if (tradeoff_state == TradeoffProbingState::kProbing) {
      last_trade_off = cur_trade_off;
      // TODO: better tuning
      if (last_period_bitrate.kbps() < 1000)
        cur_trade_off = 0.2 + last_period_bitrate.kbps() / 5000.0;
      else
        cur_trade_off = 0.3 + last_period_bitrate.kbps() / 5000.0;
      cur_trade_off = std::min(0.9, cur_trade_off);
      time_last_change_tradeoff = at_time_;
      // TODO: change point detection
      tradeoff_state = TradeoffProbingState::kChangePointDetected;
      RTC_LOG(LS_WARNING) << "change point detection: bandwidth increase up";
      target_bitrates.clear();
      network_rtts.clear();
      network_tputs.clear();
      return true;
    }
    return false;
  } else {
    time_last_change_tradeoff = at_time_;
    if (target_bitrates.empty())
      return false;

  int64_t cur_period_bitrate =
        std::accumulate(target_bitrates.begin(), target_bitrates.end(), 0) /
        target_bitrates.size();
    int64_t cur_period_tput =
        std::accumulate(network_tputs.begin(), network_tputs.end(), 0) /
        network_tputs.size();
    int64_t cur_period_rtt =
        std::accumulate(network_rtts.begin(), network_rtts.end(), 0) /
        network_rtts.size();

    DataRate cur_tput = DataRate::BitsPerSec(cur_period_tput);
    DataRate cur_bitrate = DataRate::BitsPerSec(cur_period_bitrate);
    TimeDelta cur_rtt = TimeDelta::Millis(cur_period_rtt);

    target_bitrates.clear();
    network_rtts.clear();
    network_tputs.clear();

    double cur_qoe = ((double)cur_bitrate.kbps() / 1000.0) -
                     qoe_lambda * (double)cur_rtt.ms();
    double last_qoe = ((double)last_period_bitrate.kbps() / 1000.0) -
                      qoe_lambda * (double)last_period_rtt.ms();

    // prevent it from being over aggressive
    double min_delta = 0.15 * std::min((double)cur_rtt.ms(), 250.0) / 50.0 + 0.15;
    // TODO: already high enough
    if (cur_bitrate.kbps() > 1700)
      min_delta = 0.7;
    if (cur_bitrate.kbps() > 2200)
      min_delta = 0.8;
    if (cur_bitrate.kbps() > 2500)
      min_delta = 0.9;

    RTC_LOG(LS_WARNING) << "cur qoe " << cur_qoe << " last qoe " << last_qoe;
    RTC_LOG(LS_WARNING) << "cur_bitrate " << cur_bitrate.bps() << " last_bitrate "
                        << last_period_bitrate.bps();
    RTC_LOG(LS_WARNING) << "cur_rtt " << cur_rtt.ms() << " last_rtt "
                        << last_period_rtt.ms();

    last_period_rtt = cur_rtt;
    last_period_tput = cur_tput;
    last_period_bitrate = cur_bitrate;

    // if at change point, check if it has reached new stable state
    if (tradeoff_state == TradeoffProbingState::kChangePointDetected) {
      if (at_time_.ms() - time_last_decrease_bitrate.ms() < change_point_interval) {
        last_trade_off = cur_trade_off;
        cur_trade_off = std::max(cur_trade_off, min_delta);
        cur_trade_off = std::min(MAX_TRADE_OFF, cur_trade_off);
        RTC_LOG(LS_WARNING) << "cur time " << at_time_.ms() << "last decrease at " << time_last_decrease_bitrate.ms();
        tradeoff_state = TradeoffProbingState::kProbing;
        return true;
      } else { 
        last_trade_off = cur_trade_off;
        cur_trade_off = cur_trade_off / 1.81;
        RTC_LOG(LS_WARNING) << "min delta" << min_delta;
        cur_trade_off = std::max(min_delta, cur_trade_off);
        cur_trade_off = std::min( MAX_TRADE_OFF, cur_trade_off);
        return true;
      }
    }

    if ((cur_trade_off > last_trade_off && cur_qoe > last_qoe) ||
        (cur_trade_off < last_trade_off && cur_qoe < last_qoe)) {
      last_trade_off = cur_trade_off;
      cur_trade_off = cur_trade_off + 0.2;
      cur_trade_off = std::min(MAX_TRADE_OFF, cur_trade_off);
      return true;
    } else if ((cur_trade_off < last_trade_off && cur_qoe > last_qoe) ||
               (cur_trade_off > last_trade_off && cur_qoe < last_qoe)) {
      last_trade_off = cur_trade_off;
      cur_trade_off = cur_trade_off / 1.78;
      cur_trade_off = std::max(min_delta, cur_trade_off);
      cur_trade_off = std::min(MAX_TRADE_OFF, cur_trade_off);
      return true;
    } else {
      return false;
    }
  }
}

}  // namespace webrtc