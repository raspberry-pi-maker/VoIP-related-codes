# Speech Command

Speech Command is a speech recognition solution originally released by Google's Tensorflow.
Instead of an STT solution that converts all speech to text, this is a solution that finds results from a dataset consisting of dozens of words. Therefore, it is sufficient if the input voice is also about 1 second long.

This solution is particularly useful in IVR. In certain situations, when only a simple response such as Yes or No is input, a specific number can be input using DTMF, but this technology can be used in interactive IVR services.
Another use is to determine whether a call made to a customer in an outbound solution is connected to an answering machine, fax, etc.
Fax can be implemented without the help of voice recognition by using a specific tone recognition function, but Answering Machine cannot be processed with tone recognition.
In Korea, where I live, home answering machines are rarely used. When making a mobile call, if the other party's cell phone is busy or turned off, a message may appear and the call may be connected to an answering machine. In this case as well, an announcement message is played at the same time as the answering machine is connected. Since the number of telecommunication companies is limited, it can be useful to collect the first part of these announcements and apply the Speech Command solution.
And the Speech Command solution has the advantage of being very easy to learn and apply. If you want to use a typical voice recognition solution, you will probably have to go through an incredibly difficult process. Otherwise, you will have to pay a significant fee.
<br><br>

## Speech Command by Tensorflow


* [tensorflow/tensorflow/examples/speech_commands](https://github.com/tensorflow/tensorflow/tree/master/tensorflow/examples)
* [Simple audio recognition: Recognizing keywords](https://www.tensorflow.org/tutorials/audio/simple_audio)
* [docs/site/en/tutorials/audio/simple_audio.ipynb](https://github.com/tensorflow/docs/blob/master/site/en/tutorials/audio/simple_audio.ipynb)

Previously, in Tensorflow 1.X, I used the Speech Command to learn Yes and No necessary for IVR. The last of the links above is an example introduced in the first version of Speech Command upgraded to TensorFlow 2.X. The command dataset used for Speech Command is also much simpler than the current example.

Today I will be working in PyTorch, not TensorFlow, so if you are a TensorFlow user, please check the details through the link above.
<br><br>

## Speech Command by Pytorch

I will skip explaining things such as installing the software required to run PyTorch and get straight to the point.
An example similar to TensorFlow is introduced on the next page. This example consists of a dataset of 35 instructions. The example explains everything well, from downloading the dataset to converting it to the format required for learning and learning.

* [Speech Command Classification with torchaudio](https://pytorch.org/tutorials/intermediate/speech_command_recognition_with_torchaudio.html)

<br><br>

### Preparing the sound source
Let's assume that you have prepared 500 sound sources each using the following two commands.

* Hi
* Hello

<br>

Save 50 files each in the original directory names before learning, d:/tmp/src_org/hi and d:/tmp/src_org/hello, as follows. Let's assume that the sound source length of the files is about 1 second. There is no need to create a label file yet.

### Increase the number of sound sources for learning

500 sound sources for learning is too small a number. You can increase the number of sound sources by adding slight modifications to the learning sound source. Some variations are as follows:

* add noise
* adjust voice speed.
* add sound effect

To run the following Python file, ffmpeg must be installed. Also, the path to the ffmpeg executable file must be added to the path variable.

<br><br>

```python
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

```

<br>

If the above program works without problems, 3000 new learning sound sources will be created per command. Therefore, the 1,000 pieces of training data increased to a total of 7,000 pieces.

<br>

### Label creation and training sound file correction

<br>
A 1 second long sound source will be used for learning. Voices of less than 1 second will be automatically supplemented to 1 second in length at the time of learning. Therefore, the adjustment here will cut out the portion after 1 second for sound files that are longer than 1 second. And we will create the labels needed for learning. 
This Python program uses the sox program. Therefore, be sure to install sox in advance.

<br>

```python
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

```
<br>

You are now ready to proceed with learning. Among the original sound files, files longer than 1 second were length-corrected, and labeling for 7,000 sound sources was completed.
The example below was conducted with a batch size of 256 and an epoch of 300.

<br>

### training

<br>
Proceed with learning to create a model. It is very different from the original PyTorch example.

```python
'''
python training.py

'''
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
import torchaudio
import sys
import argparse

from tqdm import tqdm
#from torchaudio.datasets import SPEECHCOMMANDS
import os


parser = argparse.ArgumentParser(description="Speech Command training")
parser.add_argument("--src", type=str, default = "./sounds", help="source audio direcotry")
args = parser.parse_args()



labels =['hi', 'hello']
tensor_size= 16000  #8000,16bit 1 second length

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(device)

class SubsetSC():
    def __init__(self, subset: str = None):
        self.path = args.src
        self.walker = []
        self.waveform = []
        self.sample_rate = []
        self.label = []
        self.filename = []


        def load_list(filename):
            filepath = os.path.join(self.path, filename)
            with open(filepath) as fileobj:
                for line in fileobj:
                    info = line.strip() # Delete \n at the end of the line
                    dir, name = info.split('/')
                    waveform, sample_rate = torchaudio.load(os.path.join(self.path, info))
                    self.waveform.append(waveform)
                    self.sample_rate.append(sample_rate)
                    self.walker.append(os.path.normpath(os.path.join(self.path, info)))
                    self.label.append(dir)
                    self.filename.append(info)
                    if dir not in labels:
                        print(f"dir [{dir}] not in label list")
                        sys.exit()

        if subset == "validation":
            load_list("validation_list.txt")
        elif subset == "testing":
            load_list("testing_list.txt")
        elif subset == "training":
            load_list("training_list.txt")
    def __getitem__(self, index):
        return (self.waveform[index], self.sample_rate[index], self.label[index], self.filename[index], 0)
    def __len__(self):
        return len(self.walker)


train_set = SubsetSC("training")
test_set = SubsetSC("testing")

print(type(test_set))
print(test_set[0])
print(len(test_set[0]))

waveform, sample_rate, label, filename, _ = train_set[0]

print("Shape of waveform: {}".format(waveform.size()))
print("Sample rate of waveform: {}".format(sample_rate))


def label_to_index(word):
    return torch.tensor(labels.index(word))


def index_to_label(index):
    return labels[index]



def pad_sequence(batch):
    # Make all tensor in a batch the same length by padding with zeros
    batch = [item.t() for item in batch]
    batch = torch.nn.utils.rnn.pad_sequence(batch, batch_first=True, padding_value=0.)
    return batch.permute(0, 2, 1)


def collate_fn(batch):

    tensors, targets = [], []

    # Gather in lists, and encode labels as indices
    for waveform, _, label, *_ in batch:
        tensors += [waveform]
        targets += [label_to_index(label)]

    # Group the list of tensors into a batched tensor
    tensors = pad_sequence(tensors)
    targets = torch.stack(targets)

    return tensors, targets


batch_size = 256

if device == "cuda":
    num_workers = 1
    pin_memory = True
else:
    num_workers = 0
    pin_memory = False

train_loader = torch.utils.data.DataLoader(
    train_set,
    batch_size=batch_size,
    shuffle=True,
    collate_fn=collate_fn,
    num_workers=num_workers,
    pin_memory=pin_memory,
)
test_loader = torch.utils.data.DataLoader(
    test_set,
    batch_size=batch_size,
    shuffle=False,
    drop_last=False,
    collate_fn=collate_fn,
    num_workers=num_workers,
    pin_memory=pin_memory,
)




class M5(nn.Module):
    def __init__(self, n_input=1, n_output=35, stride=16, n_channel=32):
        super().__init__()
        self.conv1 = nn.Conv1d(n_input, n_channel, kernel_size=80, stride=stride)
        self.bn1 = nn.BatchNorm1d(n_channel)
        self.pool1 = nn.MaxPool1d(4)
        self.conv2 = nn.Conv1d(n_channel, n_channel, kernel_size=3)
        self.bn2 = nn.BatchNorm1d(n_channel)
        self.pool2 = nn.MaxPool1d(4)
        self.conv3 = nn.Conv1d(n_channel, 2 * n_channel, kernel_size=3)
        self.bn3 = nn.BatchNorm1d(2 * n_channel)
        self.pool3 = nn.MaxPool1d(4)
        self.conv4 = nn.Conv1d(2 * n_channel, 2 * n_channel, kernel_size=3)
        self.bn4 = nn.BatchNorm1d(2 * n_channel)
        self.pool4 = nn.MaxPool1d(4)
        self.fc1 = nn.Linear(2 * n_channel, n_output)

    def forward(self, x):
        x = self.conv1(x)
        x = F.relu(self.bn1(x))
        x = self.pool1(x)
        x = self.conv2(x)
        x = F.relu(self.bn2(x))
        x = self.pool2(x)
        x = self.conv3(x)
        x = F.relu(self.bn3(x))
        x = self.pool3(x)
        x = self.conv4(x)
        x = F.relu(self.bn4(x))
        x = self.pool4(x)
        x = F.avg_pool1d(x, x.shape[-1])
        x = x.permute(0, 2, 1)
        x = self.fc1(x)
        return F.log_softmax(x, dim=2)


model = M5(n_output=len(labels))    #You shoud pass 2 
model.to(device)

def count_parameters(model):
    return sum(p.numel() for p in model.parameters() if p.requires_grad)


n = count_parameters(model)

optimizer = optim.Adam(model.parameters(), lr=0.01, weight_decay=0.0001)
scheduler = optim.lr_scheduler.StepLR(optimizer, step_size=20, gamma=0.1)  # reduce the learning after 20 epochs by a factor of 10



def train(model, epoch, log_interval):
    model.train()
    for batch_idx, (data, target) in enumerate(train_loader):

        data = data.to(device)
        target = target.to(device)

        # apply transform and model on whole batch directly on device
        #data = transform(data)
        output = model(data)

        # negative log-likelihood for a tensor of size (batch x 1 x n_output)
        loss = F.nll_loss(output.squeeze(), target)

        optimizer.zero_grad()
        loss.backward()
        optimizer.step()

        # print training stats
        if batch_idx % log_interval == 0:
            print(f"Train Epoch: {epoch} [{batch_idx * len(data)}/{len(train_loader.dataset)} ({100. * batch_idx / len(train_loader):.0f}%)]\tLoss: {loss.item():.6f}")

        # update progress bar
        pbar.update(pbar_update)
        # record loss
        losses.append(loss.item())


def number_of_correct(pred, target):
    # count number of correct predictions
    return pred.squeeze().eq(target).sum().item()


def get_likely_index(tensor):
    # find most likely label index for each element in the batch
    return tensor.argmax(dim=-1)


def test(model, epoch):
    model.eval()
    correct = 0
    for data, target in test_loader:

        data = data.to(device)
        target = target.to(device)

        # apply transform and model on whole batch directly on device
        #data = transform(data)
        output = model(data)

        pred = get_likely_index(output)
        correct += number_of_correct(pred, target)

        # update progress bar
        pbar.update(pbar_update)

    print(f"\nTest Epoch: {epoch}\tAccuracy: {correct}/{len(test_loader.dataset)} ({100. * correct / len(test_loader.dataset):.1f}%)\n")


log_interval = 20
n_epoch = 300

pbar_update = 1 / (len(train_loader) + len(test_loader))
losses = []

# The transform needs to live on the same device as the model and the data.
#transform = transform.to(device)
with tqdm(total=n_epoch) as pbar:
    for epoch in range(1, n_epoch + 1):
        train(model, epoch, log_interval)
        test(model, epoch)
        scheduler.step()


torch.save(model.state_dict(), "./speechcommand.pt")

def adjust(waveform):
    sample_wavform = torch.zeros(1, 16000)
    if waveform.size(dim=1) < 16000:
        print(f"Resize Tensor Size[{waveform.size(dim=1)}] => [16000] ")
        sample_wavform[:, :waveform.size(dim=1)] = waveform[:, :waveform.size(dim=1)]
        sample_wavform[:, waveform.size(dim=1):] = sample_wavform[:, waveform.size(dim=1):]
        waveform = sample_wavform
        print(f"Shape of resized waveform: {waveform.size()}")
    elif waveform.size(dim=1) > 16000:
        sample_wavform = waveform[:, :16000]
        waveform = sample_wavform    
    return   waveform  

def predict(tensor):
    # Use the model to predict the label of the waveform
    tensor = adjust(tensor)

    tensor = tensor.to(device)
    #tensor = transform(tensor)
    tensor = model(tensor.unsqueeze(0))
    tensor = get_likely_index(tensor)
    tensor = index_to_label(tensor.squeeze())
    return tensor


waveform, sample_rate, utterance, *_ = train_set[-1]

print(f"Expected: {utterance}. Predicted: {predict(waveform)}.")

for i, (waveform, sample_rate, utterance, fileanme, *_) in enumerate(test_set):
    #print(f"Test [{filename}]")
    output = predict(waveform)
    if output != utterance:
        print(f"Data point #{i}. Expected: {utterance}. Predicted: {output}.")
        break
    else:
        #print("All examples in this dataset were correctly classified!")
        #print("In this case, let's just look at the last data point")
        print(f"Data point #{i}. Expected: {utterance}. Predicted: {output}.")

```

<br>

### test Speech Command  model

<br>

The model created earlier can be tested as follows.

```python
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
import torchaudio
import sys, os
from torchaudio.datasets import SPEECHCOMMANDS
import torch.nn.functional as nnf
import argparse

class M5(nn.Module):
    def __init__(self, n_input=1, n_output=35, stride=16, n_channel=32):
        super().__init__()
        self.conv1 = nn.Conv1d(n_input, n_channel, kernel_size=80, stride=stride)
        self.bn1 = nn.BatchNorm1d(n_channel)
        self.pool1 = nn.MaxPool1d(4)
        self.conv2 = nn.Conv1d(n_channel, n_channel, kernel_size=3)
        self.bn2 = nn.BatchNorm1d(n_channel)
        self.pool2 = nn.MaxPool1d(4)
        self.conv3 = nn.Conv1d(n_channel, 2 * n_channel, kernel_size=3)
        self.bn3 = nn.BatchNorm1d(2 * n_channel)
        self.pool3 = nn.MaxPool1d(4)
        self.conv4 = nn.Conv1d(2 * n_channel, 2 * n_channel, kernel_size=3)
        self.bn4 = nn.BatchNorm1d(2 * n_channel)
        self.pool4 = nn.MaxPool1d(4)
        self.fc1 = nn.Linear(2 * n_channel, n_output)

    def forward(self, x):
        x = self.conv1(x)
        x = F.relu(self.bn1(x))
        x = self.pool1(x)
        x = self.conv2(x)
        x = F.relu(self.bn2(x))
        x = self.pool2(x)
        x = self.conv3(x)
        x = F.relu(self.bn3(x))
        x = self.pool3(x)
        x = self.conv4(x)
        x = F.relu(self.bn4(x))
        x = self.pool4(x)
        x = F.avg_pool1d(x, x.shape[-1])
        x = x.permute(0, 2, 1)
        x = self.fc1(x)
        return F.log_softmax(x, dim=2)


labels =['hi', 'hello']

def index_to_label(index):
    return labels[index]

def label_to_index(word):
    return torch.tensor(labels.index(word))        

def get_likely_index(tensor):
    # find most likely label index for each element in the batch
    return tensor.argmax(dim=-1)

parser = argparse.ArgumentParser(description="Speech Command test model")
parser.add_argument("--src", type=str, default = "D:/study/python/PDSSpeechCommand/sounds/hi/hi100.wav", help="test wav file")
args = parser.parse_args()

filepath = args.src
print(filepath)

waveform, sample_rate = torchaudio.load(filepath)
sample_wavform = torch.zeros(1, 16000)

if waveform.size(dim=1) < 16000:
    print(f"Resize Tensor Size[{waveform.size(dim=1)}] => [16000] ")
    sample_wavform[:, :waveform.size(dim=1)] = waveform[:, :waveform.size(dim=1)]
    sample_wavform[:, waveform.size(dim=1):] = sample_wavform[:, waveform.size(dim=1):]
    waveform = sample_wavform
    print(f"Shape of resized waveform: {waveform.size()}")
elif waveform.size(dim=1) > 16000:
    sample_wavform = waveform[:, :16000]
    waveform = sample_wavform


device = torch.device('cpu')
model = M5(n_output=5)
model.load_state_dict(torch.load("./speechcommand.pt"))
model.eval()
data = waveform.to(device)

if sample_rate != new_sample_rate:
    data = transform(data)
result = model(data.unsqueeze(0))
print(result)
print("Size:",result.size())

result = result.permute(0, 2, 1)[:, :, -1]  
print("Size:",result.size())

prob = nnf.softmax(result, dim=1, dtype=torch.float)
print("prob",prob)
top_p, top_class = prob.topk(1, dim = 1)
print("top_p",top_p)

data = get_likely_index(result)
print(data.item())
data = index_to_label(data.item())

print(data)
```



<br>

