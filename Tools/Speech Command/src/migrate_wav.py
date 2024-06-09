'''
python migrate_wav.py --src d:/tmp --target D:/sounds --temp d:/temp
'''


import argparse
import subprocess
import os, sys
import shutil
import wave
import random

parser = argparse.ArgumentParser(description="Speech Command용 wav File Migration")
parser.add_argument("--src", type=str, required = True, help="source audio file directory()")
parser.add_argument("--target", type=str, default = 1, help="Target Audio Directory")
parser.add_argument("--temp", type=str, default = "c:\\temp", help="temporary directory")
args = parser.parse_args()

labels =['hi', 'hello']
path_src = args.src
path_target = args.target

def do_extract(filename, target):
    #print(f"Extract Loudest section file[{filename}] to [{target}] start")
    conv = False
    process = subprocess.Popen(["sox", "--i", filename], stdout=subprocess.PIPE)
    output, err = process.communicate()
    exit_code = process.wait()

    #print(output)
    lines = output.splitlines()
    for line in lines:
        line = line.decode()
        kv = line.split(":")
        if(len(kv) != 2): continue

        kv[0] = kv[0].strip()
        kv[1] = kv[1].strip()
        #print(kv)

        if kv[0] == "Channels":
            if kv[1] != "1":
                conv = True 
                break
        if kv[0] == "Sample Rate":
            if kv[1] != "8000":
                conv = True 
                break

        if kv[0] == "Precision":
            if kv[1] != "16-bit":
                conv = True 
                break

    tmpfile = os.path.join(args.temp, "tmp.wav")
    if conv == True:
        #print(f"wav format conversion  file[{tmpfile}] to 8000,16bit, mono")
        print(f"sox {filename}  -b 16 -e signed-integer -c 1 -r 8k -t {tmpfile}")
        process = subprocess.Popen(["sox", filename, "-b", "16", "-e", "signed-integer", "-c", "1", "-r", "8k", tmpfile], stdout=subprocess.PIPE)
        output, err = process.communicate()
        exit_code = process.wait()
    else:
        print(f"wav format  OK[{tmpfile}] ")
        shutil.copy(filename, tmpfile)


    #If the length of the sound source file exceeds 1 second, anything after 1 second is cut and discarded.
    f = wave.open(tmpfile, 'r')
    frames = f.getnframes()
    rate = f.getframerate()
    duration = frames / float(rate)
    f.close()

    if duration > 1. :
        process = subprocess.Popen(["sox", tmpfile, target, "trim" , "0", "1"], stdout=subprocess.PIPE)
        output, err = process.communicate()
        exit_code = process.wait()
    else:    
        shutil.copy(tmpfile, target)
    os.remove(tmpfile)




if False==  os.path.isdir(path_src):
    print(f"Source directory [{path_src}] does not exist")
    sys.exit(1)

if False == os.path.isdir(path_target):
    print(f"Target directory [{path_target}] does not exist. => created")
    os.makedirs(path_target)    # makedirs : subdirectory까지 만듦

if False == os.path.isdir(args.temp):
    print(f"Temporary directory [{args.temp}] does not exist. => created")
    os.makedirs(args.temp)      # makedirs : Create a subdirectory

print(f"Working from source directory [{path_src}] target directory [{args.temp}] temporary directory [{args.temp}]")


File_list = {}
File_list['hi'] = []
File_list['hello'] = []

for label in labels:
    dir_name = os.path.join(path_src, label)
    if False==  os.path.isdir(dir_name):
        continue

    target_dir = os.path.join(path_target, label)
    if False==  os.path.isdir(target_dir):
        os.makedirs(target_dir)

    file_list = os.listdir(dir_name)
    for file in file_list:
        if ".wav" != os.path.splitext(file)[1]:
            continue

        file_name = os.path.join(dir_name, file)
        target_file_name = os.path.join(target_dir, file)
        do_extract(file_name, target_file_name)
        File_list[label].append(file)

random.shuffle(File_list['hi'])
random.shuffle(File_list['hello'])

test_hi = int(len(File_list['hi']) /10)
valid_hi = int(len(File_list['hi']) /10)
train_hi = len(File_list['hi']) - test_hi - valid_hi

test_hello = int(len(File_list['hello']) /10)
valid_hello = int(len(File_list['hello']) /10)
train_hello = len(File_list['hello']) - test_hello - valid_hello


with open(os.path.join(path_target, "validation_list.txt"), "w") as f:
    for x in range(valid_hi):
        f.write(f"hi/{File_list['hi'][x]}\n")

    for x in range(valid_hello):
        f.write(f"hello/{File_list['hello'][x]}\n")


with open(os.path.join(path_target, "testing_list.txt"), "w") as f:
    for x in range(valid_hi):
        f.write(f"hi/{File_list['hi'][x + valid_hi]}\n")

    for x in range(valid_hello):
        f.write(f"hello/{File_list['hello'][x + valid_hello]}\n")


with open(os.path.join(path_target, "training_list.txt"), "w") as f:
    for x in range(valid_hi):
        f.write(f"hi/{File_list['hi'][x + valid_hi + test_hi]}\n")

    for x in range(valid_hello):
        f.write(f"hello/{File_list['hello'][x + valid_hello + test_hello]}\n")


print("==== work done ====")
print(f"hi {len(File_list['hi'])}")
print(f"hello {len(File_list['hello'])}")

