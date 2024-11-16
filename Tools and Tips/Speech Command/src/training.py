'''
python speech.py

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

