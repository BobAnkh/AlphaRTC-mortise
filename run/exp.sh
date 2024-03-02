#!/bin/bash
traces=()
ITERATION=5
cd traces || exit 1
for trace in ./hsr/*; do
    traces+=("$trace")
done

cd .. || exit 1
MAX_BITRATE="3M"
traces=("lte-trace1" "lte-trace2" "lte-trace3")

for trace in "${traces[@]}"; do
    LOG_DIR="gcc-$MAX_BITRATE.log"
    sed "/constexpr bool kUseTradeoff/c\constexpr bool kUseTradeoff = false;" -i ~/AlphaRTC-mortise/modules/remote_bitrate_estimator/aimd_rate_control.cc
    cd .. && make docker-peerconnection && cd run || exit 1
    echo $trace >> $LOG_DIR
    # 循环10次  
    for ((i=1; i <= ITERATION; i++)); do
        # 计算新的端口号
        new_port=$((i + 5000))
        # 运行实验脚本
        ./scripts/test.sh $trace "$new_port"
        echo "iter: $i" >> tmp-gcc-delay.log
        grep 'E2E' receiver.log | awk '{print $5}' >> tmp-gcc-delay.log
        cd .. || exit 1
        rm source_dropped2.yuv source_dropped1.yuv
        ./evaluate.py -r run/receiver.log -s run/sender.log --vmaf ./vmaf --sender_video ./run/videos/gtav-720p-120s.yuv --receiver_video ./run/videos/outvideo.yuv | tail -n 1 >> ./run/$LOG_DIR
        cd ./run || exit 1
        # 等待一段时间（如果需要的话）
        sleep 2
    done 
    # mortise
    LOG_DIR="mortise-$MAX_BITRATE.log"
    sed "/constexpr bool kUseTradeoff/c\constexpr bool kUseTradeoff = true;" -i ~/AlphaRTC-mortise/modules/remote_bitrate_estimator/aimd_rate_control.cc 
    cd .. && make docker-peerconnection && cd run || exit 1
    echo $trace >> $LOG_DIR
    for ((i=1; i <= 0; i++)); do
        # 计算新的端口号
        new_port=$((i + 5000))
        # 运行实验脚本
        ./scripts/test.sh $trace "$new_port"
        cd .. || exit 1
        rm source_dropped2.yuv source_dropped1.yuv
        ./evaluate.py -r run/receiver.log -s run/sender.log --vmaf ./vmaf --sender_video ./run/videos/gtav-720p-120s.yuv --receiver_video ./run/videos/outvideo.yuv | tail -n 1 >> ./run/$LOG_DIR
        cd ./run || exit 1
        # 等待一段时间（如果需要的话）
        sleep 1
    done
done
