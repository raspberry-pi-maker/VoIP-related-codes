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

<br><br>

## Speech Command by Pytorch

* [Speech Command by Pytorch](https://github.com/raspberry-pi-maker/VoIP-related-codes/blob/main/Tools%20and%20Tips/Speech%20Command/SpeechCommand-PyTorch.md)

<br><br>

