# EXPERIMENT

## Build

1. Install [depot_tools](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up)
2. `sudo apt install python-is-python3` if don't have executable `python`
3. `make docker-sync` and wait tens of minutes

WebRTC defaultly uses VP8 as the video codec. You can modify `media/engine/internal_encoder_factory.cc` to select video codec. We modified it to use VP9 as the video codec by default.

### Build with H264

Refer to [webrtc.gni](./webrtc.gni), H264 requires ffmpeg built with H264 support:

``` gni
# Enable this to build OpenH264 encoder/FFmpeg decoder. This is supported on
# all platforms except Android and iOS. Because FFmpeg can be built
# with/without H.264 support, |ffmpeg_branding| has to separately be set to a
# value that includes H.264, for example "Chrome". If FFmpeg is built without
# H.264, compilation succeeds but |H264DecoderImpl| fails to initialize.
# CHECK THE OPENH264, FFMPEG AND H.264 LICENSES/PATENTS BEFORE BUILDING.
# http://www.openh264.org, https://www.ffmpeg.org/
#
# Enabling H264 when building with MSVC is currently not supported, see
# bugs.webrtc.org/9213#c13 for more info.
rtc_use_h264 =
    proprietary_codecs && !is_android && !is_ios && !(is_win && !is_clang)
```

If ffmpeg requirement is satisfied, we can enable H264 by setting `proprietary_codecs=true` in `gn args` like this:

1. `gn gen out/H264 --args='is_debug=false proprietary_codecs=true'`
2. `ninja -C out/H264 peerconnection_gcc`

Artifacts are in `out/H264/` directory. We can use `peerconnection_gcc` to run with gcc.

`mkdir run` and `cp out/H264/peerconnection_gcc run/ && cd run`, then run it.

### Build without H264

Without H264, we will use VP9 as the video codec.

We can directly `make docker-peerconnection` to build artifacts in `out/Default/` directory.

Or we can manually build artifacts like this:

1. `gn gen out/VP9 --args='is_debug=false'`
2. `ninja -C out/VP9 peerconnection_gcc`

Artifacts are in `out/VP9/` directory. We can use `peerconnection_gcc` to run with gcc.

`mkdir run` and `cp out/VP9/peerconnection_gcc run/ && cd run`, then run it.

## Run

### Prepare Media

#### Related Tools

- [yt-dlp](https://github.com/yt-dlp/yt-dlp) - Download video from Youtube
- [ffmpeg](https://ffmpeg.org/) - Convert video to raw format

**Attention:** [YUView](https://github.com/IENT/YUView) cannot properly play YUV videos in experiments. It is recommended to use a player based on ffmpeg instead.

#### Example Steps

##### Our Experiment Example

``` bash
# Download video with frame number
wget https://media.xiph.org/video/derf/twitch/H264/GTAV.mp4

# Convert video to YUV format with 30fps frame rate
# ATTENTION: After this, the frame number will only have even numbers
ffmpeg -i GTAV.mp4 -filter:v fps=30 -f yuv4mpegpipe -pix_fmt yuv420p 1080p.yuv
```

##### Youtube Video Example

``` bash
# Download video from Youtube
yt-dlp -f "bv[height<=?720]" https://www.youtube.com/watch?v=LXb3EKWsInQ -o "720p.%(ext)s"

# Cut video to 20 seconds
ffmpeg -i 720p.webm -t 20 -map 0 -c copy 720p-20s.webm

# Convert video to YUV format with 10fps frame rate
ffmpeg -i 720p-20s.webm -filter:v fps=10 -f yuv4mpegpipe -pix_fmt yuv420p 720p-20s.yuv

# --- If you don't want to use audio, you can generate a silent audio file like this ---
# Generate a 1 second silent audio file.
ffmpeg -f lavfi -i anullsrc -t 1 -c:a pcm_s16le silent-1s.wav

# --- If you want to use audio, you can download audio from Youtube like this ---
# Download audio
yt-dlp -f "ba" https://www.youtube.com/watch?v=LXb3EKWsInQ -o "sound.%(ext)s"

# Cut audio to 20 seconds
ffmpeg -i sound.webm -t 20 -map 0 -c copy sound-20s.webm

# Convert audio to wav format
ffmpeg -i sound-20s.webm sound-20s.wav
```

##### Manually Draw Frame Number Example

``` bash
# Download Minecraft video (1920x1080, 60fps)
wget https://media.xiph.org/video/derf/twitch/y4m/MINECRAFT.y4m

# Convert video to 960x540 YUV format with 30fps frame rate 
# Add frame number at the bottom of the video
ffmpeg -i MINECRAFT.y4m -t 60 -filter:v fps=30,scale=-1:540,"drawtext=text='%{frame_num}': start_number=1: x=(w-tw)/2: y=h-(2*lh): fontcolor=white: fontsize=48: box=1: boxcolor=0x00000000@1" -f yuv4mpegpipe -pix_fmt yuv420p 540p-60s.yuv
```

**Attention:**

1. When converting the video to YUV format, the video format should be **yuv4mpegpipe**, and the pixel format should be **yuv420p**.
2. The configuration file below only supports **integer** FPS.

### Configure

Refer to [README](./README.md) for more details.

In our experiment, we use `receiver_gcc.json` and `sender_gcc.json` as configuration files.

Notes:

- Please change the `dest_ip` of the sender to the IP address of the receiver.
- `autoclose` could be set larger than video or audio duration. Video or audio will be repeated automatically.

`receiver_gcc.json`

``` json
{
    "serverless_connection": {
        "autoclose": 60,
        "sender": {
            "enabled": false
        },
        "receiver": {
            "enabled": true,
            "listening_ip": "0.0.0.0",
            "listening_port": 8000
        }
    },
    "bwe_feedback_duration": 200,
    "video_source": {
        "video_disabled": {
            "enabled": true
        },
        "webcam": {
            "enabled": false
        },
        "video_file": {
            "enabled": false
        }
    },
    "audio_source": {
        "microphone": {
            "enabled": false
        },
        "audio_file": {
            "enabled": true,
            "file_path": "silent-1s.wav"
        }
    },
    "save_to_file": {
        "enabled": true,
        "audio": {
            "file_path": "outaudio.wav"
        },
        "video": {
            "width": 1920,
            "height": 1080,
            "fps": 30,
            "file_path": "unlimited-60s.yuv"
        }
    },
    "logging": {
        "enabled": true,
        "log_output_path": "receiver.log"
    }
}
```

`sender_gcc.json`

``` json
{
    "serverless_connection": {
        "autoclose": 60,
        "sender": {
            "enabled": true,
            "dest_ip": "100.64.0.1",
            "dest_port": 8000
        },
        "receiver": {
            "enabled": false
        }
    },
    "bwe_feedback_duration": 200,
    "video_source": {
        "video_disabled": {
            "enabled": false
        },
        "webcam": {
            "enabled": false
        },
        "video_file": {
            "enabled": true,
            "width": 1920,
            "height": 1080,
            "fps": 30,
            "file_path": "1080p.yuv"
        }
    },
    "audio_source": {
        "microphone": {
            "enabled": false
        },
        "audio_file": {
            "enabled": true,
            "file_path": "silent-1s.wav"
        }
    },
    "save_to_file": {
        "enabled": false
    },
    "logging": {
        "enabled": true,
        "log_output_path": "sender.log"
    }
}
```

### Run GCC Experiment

Run receiver first, then run sender.

Receiver: `rm ./receiver.log; ./peerconnection_gcc receiver_gcc.json 2>receiver_warn.log`

Sender: `rm ./sender.log; ./peerconnection_gcc sender_gcc.json 2>sender_warn.log`

After 60 seconds, the program will exit automatically.

`outvideo.yuv` and `outaudio.wav` are the output files.

`receiver.log` and `sender.log` are the log files.

`receiver_warn.log` and `sender_warn.log` are the warning log files.

YUV video could be very large. If you want to play it, you can use `ffmpeg` to convert it to mp4 format like this: `ffmpeg -i outvideo.yuv outvideo.mp4`. Keep the YUV file if you want to do the evaluation.

### Run HRCC Experiment

[A Hybrid Receiver-side Congestion Control Scheme for Web Real-time Communication](https://dl.acm.org/doi/abs/10.1145/3458305.3479970).

[HRCC Source Code](https://github.com/thegreatwb/HRCC)

We conduct HRCC experiments on the `hrcc_baseline` branch.

Build:

``` bash
git checkout hrcc_baseline
git submodule init  
git submodule update
# after this, related code will be in `HRCC/` directory

gn gen out/Default --args='is_debug=false'

# compiling steps refer to `make docker-peerconnection_serverless`
ninja -C out/Default peerconnection_serverless
cp out/Default/peerconnection_serverless HRCC/peerconnection_serverless.origin
cp examples/peerconnection/serverless/peerconnection_serverless HRCC/
cp modules/third_party/cmdinfer/*.py HRCC/

export LD_LIBRARY_PATH=`pwd`/modules/third_party/onnxinfer/lib:$LD_LIBRARY_PATH
cd HRCC/
```

HRCC need numpy and torch. You should install them before running HRCC.

Make shure you have proper `LD_LIBRARY_PATH` before running HRCC. You can set it like above.

Then you can run it in `HRCC/` directory similar to GCC experiment. The configuration file can also use `receiver_gcc.json` and `sender_gcc.json`.

Receiver: `rm ./receiver.log; ./peerconnection_serverless receiver_gcc.json 2>receiver_warn.log 1>/dev/null`

Sender: `rm ./sender.log; ./peerconnection_serverless sender_gcc.json 2>sender_warn.log 1>/dev/null`

## Evaluate

`evaluate.py` is a script to evaluate the QoE metrics, including network performance and video quality.

Network score comes from logs in `SENDER_LOG` and `RECEIVER_LOG`. You may need to set `--max_delay` and `--ground_recv_rate` accdording to your situation.

Video quality score includes [VMAF](https://github.com/Netflix/vmaf) and other metrics. Our script will align the frame according to the log file `SENDER_LOG` and use the binary specified by `--vmaf` to calculate the VMAF score. VMAF score will be in `--vmaf_output` file, defaultly `vmaf.json`.

Build VMAF binary from source code [libvmaf](https://github.com/Netflix/vmaf/blob/master/libvmaf/README.md) or download the pre-built binary from [VMAF Releases](https://github.com/Netflix/vmaf/releases).

QoE metrics refer to [PACC: Perception Aware Congestion Control for Real-time Communication](https://ieeexplore.ieee.org/document/10219915).

Our script refers to [Challenge-Environment](https://github.com/OpenNetLab/Challenge-Environment), [e2e_delay.py](https://github.com/OpenNetLab/AlphaRTC/blob/gcc_baseline/examples/peerconnection/gcc/corpus/e2e_delay.py) and [lossless_frame_delay.py](https://github.com/OpenNetLab/AlphaRTC/blob/gcc_baseline/examples/peerconnection/gcc/corpus/lossless_frame_delay.py).

### Usage

``` text
usage: evaluate.py [-h] [-o OUTPUT] -s SENDER_LOG -r RECEIVER_LOG [--max_delay MAX_DELAY] [--ground_recv_rate GROUND_RECV_RATE] --vmaf VMAF --sender_video SENDER_VIDEO --receiver_video RECEIVER_VIDEO
                   [--vmaf_output VMAF_OUTPUT] [--threads THREADS]

options:
  -h, --help            show this help message and exit
  -o OUTPUT, --output OUTPUT
                        the path of output file. It will print the result in terminal if you don't specify its value.
  -s SENDER_LOG, --sender_log SENDER_LOG
                        the path of sender log.
  -r RECEIVER_LOG, --receiver_log RECEIVER_LOG
                        the path of receiver log.
  --max_delay MAX_DELAY
                        the max packet delay.
  --ground_recv_rate GROUND_RECV_RATE
                        the receive rate of a special scenario ground truth.
  --vmaf VMAF           the path of vmaf
  --sender_video SENDER_VIDEO
                        the path of sender video
  --receiver_video RECEIVER_VIDEO
                        the path of receiver video
  --vmaf_output VMAF_OUTPUT
                        the path to save vmaf json output
  --threads THREADS     the number of threads for vmaf
```

Example:

``` bash
./evaluate.py -r receiver.log -s sender.log --vmaf ./vmaf --sender_video ./1080p.yuv --receiver_video ./unlimited-60s.yuv
```

### Notes

You should set correct `--max_delay` and `--ground_recv_rate` according to your situation.

You had better check the vmaf json output to make sure the vmaf score is correct.
