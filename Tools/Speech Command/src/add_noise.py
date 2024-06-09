'''
python add_noise.py --src d:/tmp --label hi --echo 1 --lowfilter 1 --speed 1 --rir 1 --noise 1
python add_noise.py --src d:/tmp --label hello --echo 1 --lowfilter 1 --speed 1 --rir 1 --noise 1

'''
import argparse
import torch
import torch.nn as nn
import torch.optim as optim
import torchaudio
import sys
from torchaudio.utils import download_asset
import os
import torchaudio.functional as F
import platform
import subprocess
import time
import threading

labels =['hi', 'hello']


SAMPLE_RIR = "./rir.wav"
SAMPLE_NOISE = "./noise.wav"
noise, _ = torchaudio.load(SAMPLE_NOISE)
rir_raw, sample_rate = torchaudio.load(SAMPLE_RIR)

parser = argparse.ArgumentParser(description="Speech Command. add noise to make additional traing data")
parser.add_argument("--label", type=str, required = True, help="label name one of two('hi', 'hello')")
parser.add_argument("--src", type=str, required = True, help="source sound directory")
parser.add_argument("--echo", type=int, default = 0, help="echo effect")
parser.add_argument("--lowfilter", type=int, default = 0, help="low filter")
parser.add_argument("--speed", type=int, default = 0, help="Ajjust Speed")
parser.add_argument("--rir", type=int, default = 0, help="Room Impulse Response")
parser.add_argument("--noise", type=int, default = 0, help="Add noise")
args = parser.parse_args()


def usage():
    print(f"--echo 1 : enable echo effect")
    print(f"--lowfilter 1 : enable low filter")
    print(f"--speed 1 : ajjust speed")
    print(f"--rir 1 : enable Room Impulse Response filter")
    print(f"--noise 1 : enable noise filter")


if args.echo == 0 and args.lowfilter == 0 and args.rir == 0 and args.noise == 0:
    usage()
    sys.exit()

def add_echo(filename, outname):
    filter = "aecho=in_gain=0.85:out_gain=0.95:delays=50:decays=0.1|delays=70:decays=0.1"
    process = subprocess.Popen(["ffmpeg", "-y", "-i", filename, "-filter:a", filter, outname ], stdout=subprocess.PIPE)
    output, err = process.communicate()
    exit_code = process.wait()

def add_lowfilter(filename, outname):
    filter = "lowpass=frequency=300:poles=1"
    process = subprocess.Popen(["ffmpeg", "-y", "-i", filename, "-filter:a", filter, outname ], stdout=subprocess.PIPE)
    output, err = process.communicate()
    exit_code = process.wait()

def add_speed(filename, outname, speed):
    filter = f"atempo={speed}"
    process = subprocess.Popen(["ffmpeg", "-y", "-i", filename, "-filter:a", filter, outname ], stdout=subprocess.PIPE)
    output, err = process.communicate()
    exit_code = process.wait()



def add_rir(waveform, sample_rate):
    rir = rir_raw[:, int(sample_rate * 1.01) : int(sample_rate * 1.1)]
    rir = rir / torch.linalg.vector_norm(rir, ord=2)    
    wavform2 = torchaudio.functional.fftconvolve(waveform, rir)    
    return wavform2


def add_noise(waveform, sample_rate):
    noise2 = noise[:, : waveform.shape[1]]
    snr_dbs = torch.tensor([20, 10, 3])
    wavform2 = torchaudio.functional.add_noise(waveform, noise2, snr_dbs)    
    return wavform2


file_path = os.path.join(args.src, args.label)
file_list = os.listdir(file_path)
if len(file_list) == 0:
    print(f"No file found at [{file_path}]")

start = time.time()
my_os = platform.system()
count = 0
for file in file_list:
    if ".wav" != os.path.splitext(file)[1]:
        continue
    file_name = os.path.join(file_path, file)
    waveform, sample_rate = torchaudio.load(file_name)

    processes = []
    if args.echo: 
        new_file = os.path.join(file_path, "echo_" + file)
        p = threading.Thread(target=add_echo, args= (file_name, new_file,))
        p.start()
        processes.append(p)
        #add_echo(file_name, new_file)
        count += 1

    if args.lowfilter: 
        new_file = os.path.join(file_path, "lowfilter_" + file)
        p = threading.Thread(target=add_lowfilter, args= (file_name, new_file,))
        p.start()
        processes.append(p)
        #add_lowfilter (file_name, new_file)
        count += 1

    if args.speed: 
        new_file = os.path.join(file_path, "speed_08_" + file)
        p = threading.Thread(target=add_speed, args= (file_name, new_file, 0.8,))
        p.start()
        processes.append(p)
        #add_speed(file_name, new_file, 0.8)
        count += 1

        new_file = os.path.join(file_path, "speed_12_" + file)
        p = threading.Thread(target=add_speed, args= (file_name, new_file, 1.2,))
        p.start()
        processes.append(p)
        #add_speed(file_name, new_file, 1.2)
        count += 1

    if args.noise: 
        print(f"******* Add Noise {file_name} *******")
        ret = add_noise(waveform, sample_rate)
        new_file = os.path.join(file_path, "noise_" + file)
        torchaudio.save(new_file, ret, sample_rate)
        print(f"save {file_name} to {new_file}")        
        count += 1


    if args.rir: 
        ret = add_rir(waveform, sample_rate)
        new_file = os.path.join(file_path, "rir_" + file)
        torchaudio.save(new_file, ret, sample_rate)
        print(f"save {file_name} to {new_file}") 
        count += 1

    for p in  processes:
        p.join()    

end = time.time()
print(f"\n\nProcess Time : [{end - start}]")
print(f"\n\nOriginal file count[{len(file_list)}] Number of files created[{count}]")
