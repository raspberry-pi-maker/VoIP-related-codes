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
