#ifndef MODULES_REMOTE_BITRATE_ESTIMATOR_TRADEOFF_CONTROL_H_
#define MODULES_REMOTE_BITRATE_ESTIMATOR_TRADEOFF_CONTROL_H_


#include <netinet/ip.h>
#include <stdint.h>
#include <cstdint>
#include <deque>

#include "absl/types/optional.h"
#include "api/transport/network_types.h"
#include "api/transport/webrtc_key_value_config.h"
#include "api/units/data_rate.h"
#include "api/units/timestamp.h"
#include "modules/congestion_controller/goog_cc/link_capacity_estimator.h"
#include "modules/remote_bitrate_estimator/include/bwe_defines.h"
#include "rtc_base/experiments/field_trial_parser.h"


namespace webrtc {
class TradeOffControl {
public:
    TradeOffControl();
    
    enum class TradeoffProbingState {
        kProbing,
        kChangePointDetected,
    } tradeoff_state;
    
    Timestamp time_last_decrease_bitrate;
    Timestamp time_last_change_tradeoff;

    double cur_trade_off;
    double last_trade_off;
    double qoe_lambda;

    DataRate last_period_bitrate;
    DataRate last_period_tput;
    TimeDelta last_period_rtt;

    std::deque<int64_t> network_rtts;
    std::deque<int64_t> target_bitrates; 
    std::deque<int64_t>  network_tputs;

    void UpdateTradeOffEstimate(TimeDelta rtt, DataRate throughput);
    void UpdateTargetBitrate(Timestamp at_time_, DataRate target_bitrate);
    void AdjustProbingParameters(double *beta, double *alpha, double *additive_coef);
    void GetQoeLambda();
    // if updated, return true
    bool UpdateTradeoff(Timestamp at_time_);
};
}

#endif